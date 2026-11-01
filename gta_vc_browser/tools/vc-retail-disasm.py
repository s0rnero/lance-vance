#!/usr/bin/env python3
"""Medida sobre el retail de PC (gta-vc.exe) con Capstone por el PE.

Para que: el motor de RenderWare va ENLAZADO dentro del exe, asi que no se
puede localizar por nombre en la tabla de importacion. Este script permite
medir sobre el retail: desensamblar una direccion, buscar cadenas y localizar
las referencias (xrefs) a una direccion dentro del .text.

Uso:
    python tools/vc-retail-disasm.py dis   <exe> <va_hex> [--bytes N] [--calls N]
    python tools/vc-retail-disasm.py str   <exe> <texto> [--utf16]
    python tools/vc-retail-disasm.py xrefs <exe> <va_hex> [--limit N]
    python tools/vc-retail-disasm.py info  <exe>

Solo lectura. Requiere capstone en gta_vc_browser/tmp/pylibs.
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tmp', 'pylibs'))
import capstone


def load(path):
    b = open(path, 'rb').read()
    e = struct.unpack_from('<I', b, 0x3C)[0]
    nsecs = struct.unpack_from('<H', b, e + 6)[0]
    optsz = struct.unpack_from('<H', b, e + 20)[0]
    opt = e + 24
    imagebase = struct.unpack_from('<I', b, opt + 28)[0]
    base = e + 24 + optsz
    secs = []
    for i in range(nsecs):
        o = base + i * 40
        name = b[o:o + 8].rstrip(b'\0').decode('latin-1')
        vsz, va, rawsz, raw = struct.unpack_from('<IIII', b, o + 8)
        secs.append((name, va, vsz, raw, rawsz))
    return b, imagebase, secs


def va2off(secs, base, va):
    rva = va - base
    for name, sva, vsz, raw, rawsz in secs:
        if sva <= rva < sva + max(vsz, rawsz):
            return raw + (rva - sva)
    return None


def off2va(secs, base, off):
    for name, sva, vsz, raw, rawsz in secs:
        if raw <= off < raw + rawsz:
            return base + sva + (off - raw)
    return None


def sec_by_name(secs, name):
    for s in secs:
        if s[0] == name:
            return s
    return None


def cmd_dis(a):
    va = int(a.addr, 16)
    b, base, secs = load(a.exe)
    off = va2off(secs, base, va)
    if off is None:
        print("direccion %s fuera de las secciones" % a.addr)
        return 1
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    code = b[off:off + a.bytes]
    print("%s @ %s (imagebase 0x%x)" % (a.exe, a.addr, base))
    calls = []
    for ins in md.disasm(code, va):
        print("  %08x  %-10s %s" % (ins.address, ins.mnemonic, ins.op_str))
        if ins.mnemonic == 'call':
            calls.append((ins.address, ins.op_str))
        if ins.mnemonic == 'ret':
            break
    if calls:
        print("  llamadas (%d):" % len(calls))
        for site, dest in calls[:a.calls]:
            print("    0x%08x -> %s" % (site, dest))
    return 0


def cmd_str(a):
    b, base, secs = load(a.exe)
    pat = a.text.encode('latin-1')
    pat16 = a.text.encode('utf-16-le')
    hits = []
    i = 0
    while True:
        j = b.find(pat, i)
        if j < 0:
            break
        va = off2va(secs, base, j)
        hits.append((va, j, 'ascii'))
        i = j + 1
    if a.utf16:
        i = 0
        while True:
            j = b.find(pat16, i)
            if j < 0:
                break
            va = off2va(secs, base, j)
            hits.append((va, j, 'utf16'))
            i = j + 1
    hits.sort(key=lambda h: (h[0] is None, h[0]))
    for va, off, kind in hits:
        print("  %s  off=0x%x  %s" % ('None' if va is None else '0x%08x' % va, off, kind))
    print("  total=%d" % len(hits))
    return 0


def cmd_xrefs(a):
    va = int(a.addr, 16)
    b, base, secs = load(a.exe)
    sec = sec_by_name(secs, '.text')
    if sec is None:
        print("sin .text")
        return 1
    name, sva, vsz, raw, rawsz = sec
    code = b[raw:raw + rawsz]
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    md.skipdata = True
    start = base + sva
    found = []
    for ins in md.disasm(code, start):
        if ins.id == 0:
            continue
        hit = False
        for op in ins.operands:
            if op.type == capstone.x86.X86_OP_IMM and op.imm == va:
                hit = True
            elif op.type == capstone.x86.X86_OP_MEM and op.mem.disp == va:
                hit = True
        if hit:
            found.append(ins)
            if len(found) >= a.limit:
                break
    for ins in found:
        print("  %08x  %-10s %s" % (ins.address, ins.mnemonic, ins.op_str))
    print("  total refs (top %d) = %d" % (a.limit, len(found)))
    return 0


def cmd_info(a):
    b, base, secs = load(a.exe)
    print("%s  size=%d  imagebase=0x%x" % (a.exe, len(b), base))
    for name, va, vsz, raw, rawsz in secs:
        print("  %-10s rva=0x%08x vsz=0x%08x raw=0x%08x rawsz=0x%08x  va=0x%08x"
              % (name, va, vsz, raw, rawsz, base + va))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)

    d = sub.add_parser('dis')
    d.add_argument("exe")
    d.add_argument("addr")
    d.add_argument("--bytes", type=int, default=96)
    d.add_argument("--calls", type=int, default=16)
    d.set_defaults(fn=cmd_dis)

    s = sub.add_parser('str')
    s.add_argument("exe")
    s.add_argument("text")
    s.add_argument("--utf16", action='store_true')
    s.set_defaults(fn=cmd_str)

    x = sub.add_parser('xrefs')
    x.add_argument("exe")
    x.add_argument("addr")
    x.add_argument("--limit", type=int, default=40)
    x.set_defaults(fn=cmd_xrefs)

    i = sub.add_parser('info')
    i.add_argument("exe")
    i.set_defaults(fn=cmd_info)

    a = ap.parse_args()
    return a.fn(a)


if __name__ == '__main__':
    sys.exit(main())
