#!/usr/bin/env python3
"""Fase 2 de la mineria del ViceEx.exe: caza la tabla de dispatch del interprete
SCM, localiza los handlers de los opcodes custom 0xFA0-0xFA9 y resume por
handler: aridad (pushes inmediatos antes de la llamada de recoleccion de args),
imports llamados por nombre y cadenas referenciadas.

Uso:
    python gta_vc_browser/tools/viceex_dispatch.py [exe]

Solo lectura. Requiere capstone en gta_vc_browser/tmp/pylibs.
"""
import os
import struct
import sys

EXE = sys.argv[1] if len(sys.argv) > 1 else r'mods\extended\GameFiles\ViceEx.exe'
CUSTOMS = [0x0FA0, 0x0FA1, 0x0FA2, 0x0FA3, 0x0FA4,
           0x0FA5, 0x0FA6, 0x0FA7, 0x0FA8, 0x0FA9]
VANILLA_REF = {0x0001: 'WAIT?', 0x00D6: 'ANDOR?', 0x059E: 'ultimo?'}

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tmp', 'pylibs'))
import capstone


def load(path):
    b = open(path, 'rb').read()
    e = struct.unpack_from('<I', b, 0x3C)[0]
    nsecs = struct.unpack_from('<H', b, e + 6)[0]
    optsz = struct.unpack_from('<H', b, e + 20)[0]
    opt = e + 24
    imagebase = struct.unpack_from('<I', b, opt + 28)[0]
    dd = opt + 96  # data directories PE32
    imp_rva, imp_sz = struct.unpack_from('<II', b, dd + 8)
    base = e + 24 + optsz
    secs = []
    for i in range(nsecs):
        o = base + i * 40
        name = b[o:o + 8].rstrip(b'\0').decode('latin-1')
        vsz, va, rawsz, raw = struct.unpack_from('<IIII', b, o + 8)
        secs.append((name, va, vsz, raw, rawsz))
    return b, imagebase, secs, (imp_rva, imp_sz)


def va2off(secs, va):
    for name, sva, vsz, raw, rawsz in secs:
        if sva <= va < sva + max(vsz, rawsz):
            off = raw + (va - sva)
            return off
    return None


def off2va(secs, off):
    for name, sva, vsz, raw, rawsz in secs:
        if raw <= off < raw + rawsz:
            return sva + (off - raw)
    return None


def rva2off(secs, rva):
    return va2off(secs, rva)  # imagebase==0 para RVAs en PE... cuidado: secciones dan RVA


def read_cstr(b, off, maxlen=80):
    end = b.find(b'\0', off, off + maxlen)
    if end < 0:
        return None
    s = b[off:end]
    try:
        return s.decode('ascii')
    except UnicodeDecodeError:
        return None


def parse_imports(b, secs, imagebase, imp):
    imp_rva, imp_sz = imp
    names = {}
    if not imp_rva:
        return names
    o = va2off(secs, imagebase + imp_rva) if False else None
    # las secciones en este parser guardan RVA == VA relativa a 0? No: PE usa RVA.
    # va2off asume VAs; en PE32 los VA de seccion son RVAs. imagebase se suma al final.
    o = va2off(secs, imp_rva)
    if o is None:
        return names
    while True:
        oft, _, __, name_rva, fto = struct.unpack_from('<IIIII', b, o)
        if oft == 0 and name_rva == 0:
            break
        dll = read_cstr(b, va2off(secs, name_rva))
        thunk = oft or fto
        t = va2off(secs, thunk)
        i = 0
        while True:
            hint_rva = struct.unpack_from('<I', b, t + i * 4)[0]
            if hint_rva == 0:
                break
            if hint_rva & 0x80000000:
                nm = f'ord_{hint_rva & 0xFFFF}'
            else:
                nm = read_cstr(b, va2off(secs, hint_rva) + 2) or '?'
            names[imagebase + thunk + i * 4] = (dll, nm)
            i += 1
        o += 20
    return names


def find_dispatch_tables(b, secs):
    """Runs largos de punteros a .text en .rdata/.data = candidatas a tabla de salto."""
    text = next((s for s in secs if s[0] == '.text'), None)
    _, tva, tvsz, traw, trsz = text
    tlo, thi = tva, tva + tvsz
    cands = []
    for name, va, vsz, raw, rawsz in secs:
        if name not in ('.rdata', '.data'):
            continue
        data = b[raw:raw + rawsz]
        run_start = None
        for i in range(0, rawsz - 4, 4):
            v = struct.unpack_from('<I', data, i)[0]
            ok = tlo <= v < thi
            if ok and run_start is None:
                run_start = i
            if not ok and run_start is not None:
                n = (i - run_start) // 4
                if n >= 1024:
                    cands.append((name, raw + run_start, va + run_start, n))
                run_start = None
        if run_start is not None:
            n = (rawsz - run_start) // 4
            if n >= 1024:
                cands.append((name, raw + run_start, va + run_start, n))
    return cands


def disasm_handler(md, b, secs, va, imagebase, imports, nbytes=180):
    off = va2off(secs, va)
    if off is None:
        print('    (VA no mapeada)')
        return
    blob = b[off:off + nbytes]
    pushes = []
    calls = []
    strs = []
    for i in md.disasm(blob, va):
        extra = ''
        if i.mnemonic == 'push':
            try:
                pushes.append(int(i.op_str, 16) if i.op_str.startswith('0x') else None)
            except ValueError:
                pushes.append(None)
        if i.mnemonic in ('call', 'jmp') and i.op_str.startswith('0x'):
            tgt = int(i.op_str, 16)
            if tgt in imports:
                dll, nm = imports[tgt]
                extra = f'   ; IMPORT {dll}!{nm}  (args pusheados: {pushes[-3:]})'
                calls.append((dll, nm, [p for p in pushes[-3:]]))
            else:
                extra = f'   ; call 0x{tgt:X} (args pusheados: {pushes[-3:]})'
                calls.append((None, f'0x{tgt:X}', [p for p in pushes[-3:]]))
        # referencias a cadenas
        for tok in i.op_str.replace('[', ' ').replace(']', ' ').split():
            if tok.startswith('0x') and len(tok) >= 6:
                try:
                    a = int(tok, 16)
                except ValueError:
                    continue
                o2 = va2off(secs, a)
                if o2 is not None and 0 <= o2 < len(b) - 2:
                    s = read_cstr(b, o2, 40)
                    if s and sum(c.isprintable() for c in s) == len(s) and len(s) >= 4:
                        strs.append(s)
        print(f'    0x{i.address:08X}: {i.mnemonic:<8} {i.op_str}{extra}')
    if strs:
        print('    cadenas:', sorted(set(strs))[:6])


def main():
    b, imagebase, secs, imp = load(EXE)
    imports = parse_imports(b, secs, imagebase, imp)
    print(f'{EXE}: {len(b)} B, imagebase=0x{imagebase:X}, imports={len(imports)}')
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

    print('\n== tablas de dispatch candidatas (runs >= 1024 punteros a .text) ==')
    cands = find_dispatch_tables(b, secs)
    for name, off, va, n in sorted(cands, key=lambda x: -x[3])[:8]:
        print(f'  {name} off=0x{off:X} VA=0x{va:08X} entradas={n}')
        if n >= 4096 or n >= 0x600:
            for idx in [0, 1, 0x00D6, 0x059E, 0x059F, 0x0600] + CUSTOMS:
                if idx * 4 + 4 <= n * 4:
                    h = struct.unpack_from('<I', b, off + idx * 4)[0]
                    tag = VANILLA_REF.get(idx, 'custom' if idx in CUSTOMS else '')
                    print(f'     [{idx:04X}] -> 0x{h:08X} {tag}')

    # dump amplio del foco cmp edx, 0xfa8
    print('\n== foco 0x12A14 (cmp edx, 0xfa8) ==')
    disasm_handler(md, b, secs, 0x129C0, imagebase, imports, 120)

    # si se encontro tabla, desensamblar handlers custom
    best = max(cands, key=lambda x: x[3], default=None)
    if best and best[3] >= 4096:
        name, off, va, n = best
        print('\n== handlers custom (desde la tabla) ==')
        for idx in CUSTOMS:
            h = struct.unpack_from('<I', b, off + idx * 4)[0]
            print(f'\n-- handler 0x{idx:04X} @ VA 0x{h:08X}')
            disasm_handler(md, b, secs, h, imagebase, imports)
        print('\n== handler vanilla de referencia (idx 0x0001) ==')
        h = struct.unpack_from('<I', b, off + 1 * 4)[0]
        print(f'-- 0x0001 @ VA 0x{h:08X}')
        disasm_handler(md, b, secs, h, imagebase, imports)


if __name__ == '__main__':
    main()
