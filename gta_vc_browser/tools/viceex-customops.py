#!/usr/bin/env python3
"""Vuelca los handlers de los opcodes custom del ViceEx.exe (0x0FA0-0x0FA9).

Para que: el `main.scm` del mod usa 7 de esos 10 opcodes 206 veces y nuestro
interprete tiene 0, asi que hay que portarlos. Esta herramienta hace la parte
mecanica: lee la tabla de saltos del dispatcher, desensambla cada handler con
capstone y resuelve cada `call` (a funcion del binario o a importacion por
nombre, que es lo que da la semantica).

Uso:
    python gta_vc_browser/tools/viceex-customops.py [exe]

Solo lectura. Requiere capstone en gta_vc_browser/tmp/pylibs:
    python -m pip install --target gta_vc_browser/tmp/pylibs capstone
"""
import os
import struct
import sys

EXE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    "mods", "extended", "GameFiles", "ViceEx.exe")
TABLA = 0x494A70          # VA de la tabla de saltos (handoff fase 2, 0x0FA0-0x0FA9)
DISPATCH = 0x4946C0       # VA de ProcessCommands4000To4099
PRIMERO = 0x0FA0
CUANTOS = 10
MAX_INSTR = 90

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                "..", "tmp", "pylibs"))
try:
    import capstone
except ImportError:
    sys.exit("capstone no encontrado: pip install --target gta_vc_browser/tmp/pylibs capstone")

D = open(EXE, "rb").read()
E = struct.unpack_from("<I", D, 0x3C)[0]
IMG = struct.unpack_from("<I", D, E + 24 + 28)[0]
NSEC = struct.unpack_from("<H", D, E + 6)[0]
OPTSZ = struct.unpack_from("<H", D, E + 20)[0]
SECS = []
_o = E + 24 + OPTSZ
for _i in range(NSEC):
    _n = D[_o:_o + 8].rstrip(b"\0").decode("latin-1")
    _vs, _va, _rs, _ra = struct.unpack_from("<IIII", D, _o + 8)
    SECS.append((_n, _va, _vs, _ra, _rs))
    _o += 40


def rva2off(rva):
    for _n, vaddr, _vs, raddr, rsize in SECS:
        if vaddr <= rva < vaddr + rsize:
            return raddr + (rva - vaddr)
    return None


def va2off(va):
    return rva2off(va - IMG)


def cstr(off):
    if off is None:
        return "?"
    fin = D.index(b"\0", off)
    return D[off:fin].decode("latin-1")


# importaciones: VA del slot de IAT -> "DLL!nombre" (los descriptores van en RVA)
IMP = {}
_dir_rva, _dir_sz = struct.unpack_from("<II", D, E + 24 + 96 + 8)
if _dir_rva and _dir_sz:
    _i = 0
    while True:
        _off = rva2off(_dir_rva + 20 * _i)
        if _off is None:
            break
        oft, _ts, _fwd, namerva, firstthunk = struct.unpack_from("<IIIII", D, _off)
        if namerva == 0 and firstthunk == 0:
            break
        dll = cstr(rva2off(namerva))
        thunk, slot = (oft or firstthunk), firstthunk
        while True:
            o = rva2off(thunk)
            if o is None:
                break
            v = struct.unpack_from("<I", D, o)[0]
            if v == 0:
                break
            if not (v & 0x80000000):
                IMP[IMG + slot] = "%s!%s" % (dll, cstr(rva2off(v + 2)))
            thunk += 4
            slot += 4
        _i += 1

md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
md.detail = True

print("%s: %d B, imagebase=0x%X, %d importaciones" % (EXE, len(D), IMG, len(IMP)))
o = va2off(TABLA)
handlers = [struct.unpack_from("<I", D, o + 4 * i)[0] for i in range(CUANTOS)]
print("tabla VA 0x%X -> %d entradas unicas" % (TABLA, len(set(handlers))))
print("dispatch VA 0x%X" % DISPATCH)
for i, h in enumerate(handlers):
    print("  0x%04X -> VA 0x%X" % (PRIMERO + i, h))


def texto_literal(va):
    """Si en `va` hay una cadena C imprimible, la devuelve (para los asserts)."""
    off = va2off(va)
    if off is None:
        return None
    trozo = D[off:off + 120]
    fin = trozo.find(b"\0")
    if fin < 6:
        return None
    cad = trozo[:fin]
    if not all(32 <= c < 127 for c in cad):
        return None
    return cad.decode("latin-1")


def destino(ins):
    """Texto de la resolucion de un call/jmp, o "" si no aplica."""
    if ins.mnemonic not in ("call", "jmp"):
        return ""
    txt = ins.op_str
    if "[" in txt:
        ini = txt.find("[") + 1
        dirv = txt[ini:].split("+")[0].rstrip("]").strip()
        try:
            va = int(dirv, 16)
        except ValueError:
            return ""
        if va in IMP:
            return "  -> %s" % IMP[va]
        return "  -> *[0x%X]" % va
    if txt.startswith("0x"):
        t = int(txt, 16)
        return "  -> %s" % IMP.get(t, "sub_%X" % t)
    return ""


def volcar(va, fin, titulo):
    """Desensambla [va, fin) resolviendo calls e imprimiendo literales de assert."""
    print("\n== %s (VA 0x%X-0x%X) ==" % (titulo, va, fin))
    o = va2off(va)
    for ins in md.disasm(D[o:o + (fin - va)], va):
        nota = destino(ins)
        if ins.mnemonic == "push" and ins.op_str.startswith("0x"):
            lit = texto_literal(int(ins.op_str, 16))
            if lit:
                nota += '   ; "%s"' % lit[:96]
        print("  0x%08X  %-46s%s" % (ins.address, "%s %s" % (ins.mnemonic, ins.op_str), nota))


if "--subs" in sys.argv:
    lista = sys.argv[sys.argv.index("--subs") + 1]
    for par in lista.split(","):
        va, fin = (int(x, 16) for x in par.split("-"))
        volcar(va, fin, "sub")
    sys.exit(0)

for i, h in enumerate(handlers):
    # los handlers son contiguos: cada uno llega hasta el siguiente
    fin = handlers[i + 1] if i + 1 < len(handlers) else h + 0x80
    volcar(h, fin, "0x%04X" % (PRIMERO + i))
