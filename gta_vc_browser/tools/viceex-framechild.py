#!/usr/bin/env python3
"""Vuelca el desensamblado de una funcion de un PE (GTA VC / ViceEx) y sus llamadas.

Para que: el codigo de RenderWare va ENLAZADO dentro del exe (no son imports),
as que no se pueden localizar por nombre en la tabla de importacion. Este
script sirve para medir una funcion concreta por direccion: su desensamblado y
la lista de llamadas que hace (sitio -> destino), que es lo que hace falta
para comparar el binario del mod con el retail o con el codigo del port.

Uso:
    python tools/viceex-framechild.py <exe> <direccion_hex> [--bytes N] [--calls N]

Ejemplo (la rama del extra del `PreprocessHierarchy` del mod, 0x55065c):
    python tools/viceex-framechild.py mods/extended/GameFiles/ViceEx.exe 0x55065c --bytes 48

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


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("exe")
    ap.add_argument("addr", help="direccion virtual (0x...)")
    ap.add_argument("--bytes", type=int, default=64, help="cuanto desensamblar")
    ap.add_argument("--calls", type=int, default=8, help="cuantas llamadas listar")
    a = ap.parse_args()

    va = int(a.addr, 16)
    b, base, secs = load(a.exe)
    off = va2off(secs, base, va)
    if off is None:
        print("direccion %s fuera de las secciones de %s" % (a.addr, a.exe))
        return 1
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
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


if __name__ == '__main__':
    sys.exit(main())
