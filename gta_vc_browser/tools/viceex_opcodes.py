#!/usr/bin/env python3
"""Mina los opcodes custom 0FA0-0FA9 del ViceEx.exe del mod (fase 1, solo lectura).

Uso:
    python gta_vc_browser/tools/viceex_opcodes.py [exe]

Busca referencias a los inmediatos 16/32-bit de cada opcode, desensambla la
zona con capstone (PYTHONPATH=gta_vc_browser/tmp/pylibs si se instaló ahi) y
resume: instruccion que porta el inmediato, funciones llamadas desde la zona
(candidatas a CollectParameters = aridad) y cadenas cercanas.
"""
import os
import struct
import sys

EXE = sys.argv[1] if len(sys.argv) > 1 else r'mods\extended\GameFiles\ViceEx.exe'
OPCODES = [0x0FA0, 0x0FA1, 0x0FA2, 0x0FA3, 0x0FA4, 0x0FA5,
           0x0FA6, 0x0FA7, 0x0FA8, 0x0FA9]

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tmp', 'pylibs'))
try:
    import capstone
except ImportError:
    sys.exit('capstone no encontrado: pip install --target gta_vc_browser/tmp/pylibs capstone')


def load(path):
    b = open(path, 'rb').read()
    e_lfanew = struct.unpack_from('<I', b, 0x3C)[0]
    nsecs = struct.unpack_from('<H', b, e_lfanew + 6)[0]
    optsz = struct.unpack_from('<H', b, e_lfanew + 20)[0]
    opt = e_lfanew + 24
    imagebase = struct.unpack_from('<I', b, opt + 28)[0]
    base = e_lfanew + 24 + optsz
    secs = []
    for i in range(nsecs):
        o = base + i * 40
        name = b[o:o + 8].rstrip(b'\0').decode('latin-1')
        vsz, va, rawsz, raw = struct.unpack_from('<IIII', b, o + 8)
        secs.append((name, va, vsz, raw, rawsz))
    return b, imagebase, secs


def off2va(secs, off):
    for name, va, vsz, raw, rawsz in secs:
        if raw <= off < raw + rawsz:
            return va + (off - raw)
    return None


def va2off(secs, va):
    for name, sva, vsz, raw, rawsz in secs:
        if sva <= va < sva + max(vsz, rawsz):
            return raw + (va - sva)
    return None


def find_imm_hits(b, value):
    """Devuelve [(offset, tipo)] para inmediatos LE de 4B (completo) o 2B (u16)."""
    hits = []
    pat4 = struct.pack('<I', value)
    pat2 = struct.pack('<H', value)
    i = 0
    while True:
        i = b.find(pat4, i)
        if i < 0:
            break
        hits.append((i, 'imm32'))
        i += 1
    i = 0
    while True:
        i = b.find(pat2, i)
        if i < 0:
            break
        if not (i + 4 <= len(b) and b[i:i + 4] == pat4):
            hits.append((i, 'imm16'))
        i += 1
    return hits


def disasm_around(md, b, off, back=48, fwd=96):
    start = max(0, off - back)
    blob = b[start:off + fwd]
    return list(md.disasm(blob, start))


def main():
    b, imagebase, secs = load(EXE)
    print(f'{EXE}: {len(b)} bytes, imagebase=0x{imagebase:X}')
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = False

    for op in OPCODES:
        hits = find_imm_hits(b, op)
        print(f'\n===== opcode 0x{op:04X}: {len(hits)} referencias crudas =====')
        shown = 0
        for off, kind in hits:
            va = off2va(secs, off)
            sec = next((n for n, v, vs, r, rs in secs if r <= off < r + rs), '?')
            if sec not in ('.text',):
                continue
            insns = disasm_around(md, b, off)
            # la instruccion que contiene el inmediato y el contexto
            owner = [i for i in insns if i.address <= off < i.address + i.size]
            if not owner:
                continue
            o = owner[0]
            # solo interesa si el inmediato participa de cmp/mov/push/test (no datos)
            if not any(k in o.mnemonic for k in ('cmp', 'mov', 'push', 'test', 'sub', 'add', 'lea')):
                continue
            shown += 1
            if shown > 6:
                continue
            print(f'-- off=0x{off:X} VA=0x{va:08X} ({kind}) en: {o.mnemonic} {o.op_str}')
            for i in insns:
                mark = '>>' if i.address == o.address else '  '
                print(f'   {mark} 0x{i.address:08X}: {i.mnemonic:<8} {i.op_str}')
        if shown == 0:
            print('   (sin referencias ejecutables: probablemente solo datos)')


if __name__ == '__main__':
    main()
