#!/usr/bin/env python3
"""Compara la rama del extra de `PreprocessHierarchy` entre el retail y el mod.

Para que: el mod (Vice Extended) cambia el `PreprocessHierarchy` de los
vehiculos para que el marco del extra NO se suelte del arbol de marcos
(`src/modelinfo/VehicleModelInfo.cpp`):

    atomic = (RpAtomic*)GetFirstObject(assoc.frame);
    RpClumpRemoveAtomic(m_clump, atomic);
#ifndef IMPROVED_VEHICLES // Service lights for service cars
    RwFrameRemoveChild(assoc.frame);
#endif
    SetVehicleComponentFlags(assoc.frame, desc[i].flags);

Ese `#ifndef` (IMPROVED_VEHICLES definido en el mod) es lo que hace que las
luces de servicio del coche de policia (`servicelights_1..3`, hijos de
`extra1..3`) consigan su id de jerarquia y lleguen a `m_aCarNodes`.

El script mide la diferencia con lo medido en el repo (r2 sobre el binario del
mod, receipts en gta_vc_browser/tmp/rea/):
  * mod:    0x55065c call GetFirstObject / 0x55066a call RpClumpRemoveAtomic /
            0x55067d call SetVehicleComponentFlags  -> NO hay llamada intermedia
  * retail: el mismo sitio tiene UNA llamada mas (la que suelta el marco)

El codigo de RenderWare es IDENTICO byte a byte en el retail y en el mod (el
mod parchea el juego, no la libreria), asi que las funciones RW se localizan
en el retail por sus MISMOS bytes y se pueden comparar las dos ramas.

Uso:
    python tools/viceex-preprocess-cmp.py <mod.exe> <retail.exe>
"""
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'tmp', 'pylibs'))
import capstone

# Medido con r2 sobre el binario del mod (gta_vc_browser/tmp/rea/r2_func.txt).
FUNCS = {
    'GetFirstObject': 0x00601370,
    'RpClumpRemoveAtomic': 0x00547190,
    'SetVehicleComponentFlags': 0x00551700,
}
# Sitio medido de la rama del extra en el mod (tmp/rea/r2_func.txt).
MOD_SITE = 0x0055065C
NEEDLE_LEN = 16


class Pe(object):
    def __init__(self, path):
        self.path = path
        self.b = open(path, 'rb').read()
        e = struct.unpack_from('<I', self.b, 0x3C)[0]
        nsecs = struct.unpack_from('<H', self.b, e + 6)[0]
        optsz = struct.unpack_from('<H', self.b, e + 20)[0]
        opt = e + 24
        self.base = struct.unpack_from('<I', self.b, opt + 28)[0]
        base = e + 24 + optsz
        self.secs = []
        for i in range(nsecs):
            o = base + i * 40
            name = self.b[o:o + 8].rstrip(b'\0').decode('latin-1')
            vsz, va, rawsz, raw = struct.unpack_from('<IIII', self.b, o + 8)
            self.secs.append((name, va, vsz, raw, rawsz))

    def off(self, va):
        rva = va - self.base
        for name, sva, vsz, raw, rawsz in self.secs:
            if sva <= rva < sva + max(vsz, rawsz):
                return raw + (rva - sva)
        return None

    def bytes_at(self, va, n):
        o = self.off(va)
        return self.b[o:o + n] if o is not None else b''

    def text_hits(self, needle, limit=8):
        hits = []
        for name, sva, vsz, raw, rawsz in self.secs:
            if name != '.text':
                continue
            data = self.b[raw:raw + rawsz]
            start = 0
            while len(hits) < limit:
                k = data.find(needle, start)
                if k < 0:
                    break
                hits.append(self.base + sva + k)
                start = k + 1
        return hits

    def call_sites(self, target, limit=4000):
        out = []
        for name, sva, vsz, raw, rawsz in self.secs:
            if name != '.text':
                continue
            data = self.b[raw:raw + rawsz]
            for i in range(len(data) - 5):
                if data[i] == 0xE8:
                    rel = struct.unpack_from('<i', data, i + 1)[0]
                    if self.base + sva + i + 5 + rel == target:
                        out.append(self.base + sva + i)
                        if len(out) >= limit:
                            return out
        return out

    def next_call(self, va, back=0, fwd=40):
        """Siguiente llamada relativa en [va+back, va+fwd) -> (sitio, destino)."""
        o = self.off(va + back)
        if o is None:
            return None, None
        for i in range(0, fwd):
            if self.b[o + i] == 0xE8:
                rel = struct.unpack_from('<i', self.b, o + i + 1)[0]
                site = va + back + i
                return site, site + 5 + rel
        return None, None

    def prev_call(self, va, back=40):
        o = self.off(va)
        if o is None:
            return None, None
        for i in range(-back, 0):
            if self.b[o + i] == 0xE8:
                rel = struct.unpack_from('<i', self.b, o + i + 1)[0]
                site = va + i
                return site, site + 5 + rel
        return None, None


def dump(pe, md, va, label, n=10):
    code = pe.bytes_at(va, 80)
    print("    %s (0x%08x):" % (label, va))
    k = 0
    for ins in md.disasm(code, va):
        print("      %08x  %-10s %s" % (ins.address, ins.mnemonic, ins.op_str))
        k += 1
        if k >= n or ins.mnemonic == 'ret':
            break


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 1
    mod = Pe(sys.argv[1])
    retail = Pe(sys.argv[2])
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

    print("mod    : %s (imagebase 0x%x)" % (mod.path, mod.base))
    print("retail : %s (imagebase 0x%x)" % (retail.path, retail.base))

    raddr = {}
    for name, va in sorted(FUNCS.items(), key=lambda kv: kv[1]):
        needle = mod.bytes_at(va, NEEDLE_LEN)
        hits = retail.text_hits(needle, 4)
        raddr[name] = hits
        print("  %-24s mod 0x%08x -> retail %s" % (
            name, va, " ".join("0x%08x" % h for h in hits) or "(sin coincidencia)"))

    print("  MOD, rama del extra (PreprocessHierarchy 0x550560):")
    dump(mod, md, MOD_SITE, "sitio medido", 12)
    _, nxt = mod.next_call(MOD_SITE)
    print("    -> la llamada que sigue a GetFirstObject va a 0x%08x (sin llamada intermedia)"
          % (nxt or 0))

    if not raddr.get('RpClumpRemoveAtomic'):
        print("  RETAIL: la libreria RW no coincide byte a byte con la del mod")
        print("          (build distinto), asi que no se puede mapear por bytes:")
        print("          la comparacion con el retail queda pendiente.")
        return 0

    sites = retail.call_sites(raddr['RpClumpRemoveAtomic'][0])
    print("  retail: %d llamadas a RpClumpRemoveAtomic (0x%08x)" % (
        len(sites), raddr['RpClumpRemoveAtomic'][0]))
    for s in sites:
        at, dest = retail.next_call(s)
        if dest is None:
            continue
        if dest in raddr.get('SetVehicleComponentFlags', []):
            continue
        pat, pdest = retail.prev_call(s)
        if pdest not in raddr.get('GetFirstObject', []):
            continue
        print("  RETAIL, rama del extra (encontrada por GetFirstObject -> RpClumpRemoveAtomic):")
        dump(retail, md, pat, "sitio", 12)
        return 0
    print("  retail: no se ha encontrado la rama con llamada intermedia")
    return 0


if __name__ == '__main__':
    sys.exit(main())
