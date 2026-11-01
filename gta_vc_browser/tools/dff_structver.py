#!/usr/bin/env python3
"""Valida y repara la integridad de las geometrias de un DFF.

Dos contratos, los dos aprendidos a base de cuelgues (planes 32.0 y 34.0):

1. VERSION DEL STRUCT. La cabecera de un chunk lleva un *library id* del que se
   desempacan `version` y `build` (librw `libraryIDUnpackVersion`). La version
   decide si el STRUCT lleva delante los 12 bytes de `SurfaceProperties`:

       0xc02ffff  -> version 0x33002  (< 0x34000)  -> CON surface props
       0x1003ffff -> version 0x34003  (>= 0x34000) -> SIN surface props

   Si el cuerpo y la version no cuadran, `streamRead` consume de mas o de menos
   y `findChunk(MATLIST)` falla: el modelo no carga.

2. DISPOSICION DE LA MALLA. `readMesh` lee, por cada grupo, su cabecera
   `{numIndices, matIndex}` y a continuacion SUS indices (uint32). Si el DFF
   guarda todas las cabeceras primero y luego todos los indices, el motor toma
   indices por materiales (`MESHBADMAT matIndex=1270 mats=6`), el modelo falla y
   el clump a medias revienta en `rw::Frame::updateObjects`.

La reparacion es derivada del propio fichero y no cambia tamanos: reescribe el
campo de version (4 bytes) o reordena el cuerpo de la malla. No hay que
reempaquetar el `.dir` por esto.

Uso
---
    python tools/dff_structver.py --check streamed/models/gta3.img
    python tools/dff_structver.py --fix   streamed/models/gta3.img
    python tools/dff_structver.py --check a.dff b.dff
"""
import argparse
import os
import struct
import sys

NATIVE = 0x01000000
PRELIT = 0x08
TEXTURED = 0x04
TEXTURED2 = 0x80

VER_PROPS = 0xC02FFFF       # version 0x33002: el STRUCT lleva 12 bytes de props
VER_NOPROPS = 0x1003FFFF    # version 0x34003: no los lleva

MAX_MESHES = 100000
MAX_INDICES = 4000000


def unpack_version(libid):
    """Igual que libraryIDUnpackVersion de librw (rwbase.h)."""
    if libid & 0xFFFF0000:
        return ((libid >> 14 & 0x3FF00) + 0x30000) | (libid >> 16 & 0x3F)
    return libid << 8


def children(buf, start, end):
    p = start
    while p + 12 <= end:
        typ, size, ver = struct.unpack_from("<III", buf, p)
        if size < 0 or p + 12 + size > end:
            raise ValueError("chunk truncado en %d (tipo %#x)" % (p, typ))
        yield typ, size, ver, p
        p += 12 + size


def struct_start(flags, nverts, ntris):
    """Bytes desde el inicio del cuerpo hasta los triangulos (sin props)."""
    tset = (flags >> 16) & 0xFFFF
    if tset == 0:
        tset = 1 if flags & TEXTURED else (2 if flags & TEXTURED2 else 0)
    p = 16
    if not flags & NATIVE:
        if flags & PRELIT:
            p += 4 * nverts
        p += 8 * tset * nverts
        p += 8 * ntris
    return p


def struct_end_with_morphs(body, p, nverts, nmorph):
    size = len(body)
    for _ in range(max(1, nmorph)):
        if p + 24 > size:
            return None
        p += 16
        hasv, hasn = struct.unpack_from("<ii", body, p)
        p += 8
        if hasv:
            p += 12 * nverts
        if hasn:
            p += 12 * nverts
        if p > size:
            return None
    return p


def mesh_layout(buf, off, size, nmats, ntris):
    """('ok'|'headers'|'unknown', registros, desplazamiento de los indices).

    'ok' = intercalado, lo que lee el motor. 'headers' = todas las cabeceras
    primero (reparable sin cambiar el tamano). 'unknown' = no encaja en ninguna,
    no se toca.
    """
    if size < 24:
        return "unknown", None, None
    flags, num, total = struct.unpack_from("<III", buf, off)
    if num == 0 or num > MAX_MESHES or total > MAX_INDICES:
        return "unknown", None, None

    end = off + size
    # 1) intercalado (el bueno)
    p = off + 12
    seen = 0
    ok = True
    for _ in range(num):
        if p + 8 > end:
            ok = False
            break
        ni, mi = struct.unpack_from("<Ii", buf, p)
        p += 8
        if mi < 0 or mi >= nmats or ni == 0 or ni > total - seen:
            ok = False
            break
        p += ni * 4
        seen += ni
    if ok and p == end and seen == total:
        return "ok", None, None

    # 2) cabeceras primero, luego todos los indices
    p = off + 12
    recs = []
    ok = True
    for _ in range(num):
        if p + 8 > end:
            ok = False
            break
        ni, mi = struct.unpack_from("<Ii", buf, p)
        p += 8
        if mi < 0 or mi >= nmats or ni == 0 or ni > total:
            ok = False
            break
        recs.append((ni, mi))
    if ok and sum(ni for ni, _ in recs) == total and p + total * 4 == end:
        return "headers", recs, p

    return "unknown", None, None


def inspect(path):
    """[(struct_off, nverts, nmorph, want_props, ver_ok,
        mesh_off, mesh_size, nmats, ntris, mesh_status, recs, idx_off)]."""
    buf = bytearray(open(path, "rb").read())
    clump = None
    for typ, size, ver, off in children(buf, 0, len(buf)):
        if typ == 0x10:
            clump = (off, size)
            break
    if clump is None:
        raise ValueError("sin clump")
    off, size = clump
    geomlist = None
    for typ, gsize, ver, goff in children(buf, off + 12, off + 12 + size):
        if typ == 0x1A:
            geomlist = (goff, gsize)
            break
    if geomlist is None:
        raise ValueError("sin lista de geometrias")

    goff, gsize = geomlist
    out = []
    for typ, gsz, gver, gfield in children(buf, goff + 12, goff + 12 + gsize):
        if typ != 0x0F:
            continue
        st = ml = me = None
        for ctyp, csize, cver, coff in children(buf, gfield + 12, gfield + 12 + gsz):
            if ctyp == 0x01 and st is None:
                st = (coff, csize, cver)
            elif ctyp == 0x08 and ml is None:
                ml = (coff, csize)
            elif ctyp == 0x03 and me is None:
                me = (coff, csize)
        if st is None:
            continue
        coff, csize, cver = st
        body = buf[coff + 12:coff + 12 + csize]
        if len(body) < 16:
            continue
        flags, ntris, nverts, nmorph = struct.unpack_from("<Iiii", body, 0)
        if nverts < 0 or ntris < 0 or nmorph < 0:
            continue
        base = struct_start(flags, nverts, ntris)
        end_no = struct_end_with_morphs(body, base, nverts, nmorph)
        end_yes = struct_end_with_morphs(body, base + 12, nverts, nmorph)
        if end_no == csize and end_yes != csize:
            want_props, ver_ok = False, unpack_version(cver) >= 0x34000
        elif end_yes == csize and end_no != csize:
            want_props, ver_ok = True, unpack_version(cver) < 0x34000
        else:
            want_props, ver_ok = None, False       # ambiguo o irreparable

        mesh_off = mesh_size = nmats = 0
        mesh_status, recs, idx_off = "unknown", None, None
        if ml is not None and me is not None:
            nmats = len([c for c in children(buf, ml[0] + 12, ml[0] + 12 + ml[1])
                         if c[0] == 0x07])
            for ctyp, csize, cver, coff in children(buf, me[0] + 12, me[0] + 12 + me[1]):
                if ctyp == 0x50E:
                    mesh_off, mesh_size = coff + 12, csize
                    break
            if mesh_size:
                mesh_status, recs, idx_off = mesh_layout(
                    buf, mesh_off, mesh_size, nmats, ntris)

        out.append((coff, nverts, nmorph, want_props, ver_ok,
                    mesh_off, mesh_size, nmats, ntris, mesh_status, recs, idx_off))
    return buf, out


def repair_mesh(buf, mesh_off, mesh_size, recs, idx_off):
    """Reescribe la malla intercalada. Mismo tamano, solo cambia el orden.

    `mesh_off` es el comienzo del CUERPO del chunk 0x50E (no de su cabecera) y
    `mesh_size` su longitud; los primeros 12 bytes del cuerpo son
    `{flags, numMeshes, totalIndices}`.
    """
    total = sum(ni for ni, _ in recs)
    if mesh_size != 12 + 8 * len(recs) + 4 * total:
        return False
    blob = bytes(buf[idx_off:idx_off + total * 4])
    if len(blob) != total * 4:
        return False
    out = bytearray(buf[mesh_off:mesh_off + 12])
    at = 0
    for ni, mi in recs:
        out += struct.pack("<Ii", ni, mi)
        out += blob[at:at + ni * 4]
        at += ni * 4
    if len(out) != mesh_size:
        return False
    buf[mesh_off:mesh_off + mesh_size] = out
    return True


def process(path, fix):
    buf, geoms = inspect(path)
    if not geoms:
        return 0, 0, 0, 0
    ver_mal = ver_fix = mesh_mal = mesh_fix = 0
    for (coff, nverts, nmorph, want_props, ver_ok,
         mesh_off, mesh_size, nmats, ntris, mesh_status, recs, idx_off) in geoms:
        if want_props is not None and not ver_ok:
            ver_mal += 1
            if fix:
                ver = VER_PROPS if want_props else VER_NOPROPS
                struct.pack_into("<I", buf, coff + 8, ver)
                ver_fix += 1
        if mesh_status == "headers":
            mesh_mal += 1
            if fix and repair_mesh(buf, mesh_off, mesh_size, recs, idx_off):
                mesh_fix += 1
    if fix and (ver_fix or mesh_fix):
        with open(path, "wb") as f:
            f.write(buf)
    return len(geoms), ver_mal, mesh_mal, ver_fix + mesh_fix


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("rutas", nargs="+", help="ficheros .dff o carpetas")
    ap.add_argument("--check", action="store_true",
                    help="solo valida (es el comportamiento por defecto)")
    ap.add_argument("--fix", action="store_true",
                    help="repara version y disposicion de malla")
    ap.add_argument("--quiet", action="store_true", help="solo problemas y resumen")
    a = ap.parse_args()

    files = []
    for r in a.rutas:
        if os.path.isdir(r):
            files += [os.path.join(r, f) for f in sorted(os.listdir(r))
                      if f.lower().endswith(".dff")]
        else:
            files.append(r)

    tot = {"dff": 0, "geom": 0, "ver": 0, "mesh": 0, "fix": 0, "error": 0}
    for f in files:
        try:
            ngeom, ver_mal, mesh_mal, fixed = process(f, a.fix)
        except Exception as e:
            tot["error"] += 1
            print("%s: ERROR %s" % (f, e))
            continue
        tot["dff"] += 1
        tot["geom"] += ngeom
        tot["ver"] += ver_mal
        tot["mesh"] += mesh_mal
        tot["fix"] += fixed
        if ver_mal or mesh_mal:
            print("%s: %d/%d geometrias; version mal %d; malla mal %d%s"
                  % (os.path.basename(f), ver_mal + mesh_mal, ngeom,
                     ver_mal, mesh_mal, (", %d arregladas" % fixed) if fixed else ""))
        elif not a.quiet:
            print("%s: %d geometrias OK" % (os.path.basename(f), ngeom))

    print("--- %d ficheros, %d geometrias, version mal %d, malla mal %d, "
          "arregladas %d, errores %d"
          % (tot["dff"], tot["geom"], tot["ver"], tot["mesh"],
             tot["fix"], tot["error"]))
    return 1 if (tot["ver"] or tot["mesh"] or tot["error"]) else 0


if __name__ == "__main__":
    sys.exit(main())
