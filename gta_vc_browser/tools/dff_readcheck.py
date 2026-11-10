#!/usr/bin/env python3
"""Consume los DFF igual que el motor y senala el byte exacto que lo rompe.

Replica el recorrido de `RpClumpGtaStreamRead1`, `Geometry::streamRead`,
`MaterialList::streamRead`, `Material::streamRead`, `PluginList::streamRead`
y `readMesh` de librw: cabeceras de 12 bytes (tipo, tamano, version-alineada),
busqueda de chunks hermanos saltando cuerpos por su tamano declarado, y
extensiones de plugin que tienen que consumirse exactamente.

Uso:
    python tools/dff_readcheck.py <fichero-o-carpeta> [...]
"""

import os
import struct
import sys

STRUCT = 0x01
FRAMELIST = 0x0E
GEOMETRYLIST = 0x1A
GEOMETRY = 0x0F
MATLIST = 0x08
MATERIAL = 0x07
TEXTURE = 0x06
EXTENSION = 0x03
ATOMIC = 0x14
CLUMP = 0x10
MESH = 0x50E

PRELIT = 0x08
TEXTURED = 0x04
TEXTURED2 = 0x80
NATIVE = 0x01000000


class Bad(Exception):
    def __init__(self, detail, off):
        Exception.__init__(self, detail)
        self.detail = detail
        self.off = off


def unpack_version(libid):
    if libid & 0xFFFF0000:
        return (((libid >> 14) & 0x3FF00) + 0x30000) | ((libid >> 16) & 0x3F)
    return (libid << 8) & 0xFFFFFFFF


def children(buf, start, end):
    out = []
    p = start
    while p < end:
        if p + 12 > end:
            raise Bad("quedan %d bytes sin cabecera" % (end - p), p)
        typ, size, ver = struct.unpack_from("<III", buf, p)
        if size < 0 or p + 12 + size > end:
            raise Bad("chunk 0x%X declara %d y se sale" % (typ, size), p)
        out.append((typ, ver, p + 12, p + 12 + size, p))
        p += 12 + size
    return out


def first(kids, typ):
    for k in kids:
        if k[0] == typ:
            return k
    return None


def check_plugins(buf, start, end, nmats):
    errs = []
    p = start
    while p < end:
        if p + 12 > end:
            errs.append(("plugin", p, "cabecera partida (%d bytes)" % (end - p)))
            return errs
        typ, size, _ver = struct.unpack_from("<III", buf, p)
        if size < 0 or p + 12 + size > end:
            errs.append(("plugin", p, "0x%X declara %d y se sale del cuerpo" % (typ, size)))
            return errs
        if typ == MESH:
            errs.extend(check_mesh(buf, p + 12, p + 12 + size, nmats))
        p += 12 + size
    if p != end:
        errs.append(("plugin", start, "la extension no cuadra: fin=%d esperado=%d" % (p, end)))
    return errs


def check_mesh(buf, start, end, nmats):
    errs = []
    n = end - start
    if n < 12:
        errs.append(("mesh", start, "cuerpo de %d bytes (minimo 12)" % n))
        return errs
    _flags, nmesh, total = struct.unpack_from("<III", buf, start)
    if nmesh > 100000:
        errs.append(("mesh", start, "numMeshes=%d" % nmesh))
        return errs
    # librw lee cada grupo intercalado: cabecera {numIndices, matIndex} y a
    # continuacion SUS indices (Geometry::readMesh). No es todo cabeceras y
    # luego todos los indices: eso desplaza al lector y da MESHBADMAT.
    p = start + 12
    total_sum = 0
    for i in range(nmesh):
        if p + 8 > end:
            errs.append(("mesh", p, "cabeza de malla %d fuera del chunk" % i))
            return errs
        ni, mi = struct.unpack_from("<Ii", buf, p)
        p += 8
        if mi < 0 or mi >= nmats:
            errs.append(("mesh_mat", p - 4, "matIndex=%d mats=%d (malla %d)" % (mi, nmats, i)))
        if ni > 1000000 or p + 4 * ni > end:
            errs.append(("mesh_len", p, "indices=%d no caben (malla %d)" % (ni, i)))
            return errs
        p += 4 * ni
        total_sum += ni
    if total_sum != total:
        errs.append(("mesh_suma", start, "totalIndices=%d suma=%d (meshes=%d)"
                     % (total, total_sum, nmesh)))
    if p != end:
        errs.append(("mesh_len", p, "sobran %d bytes (total=%d suma=%d)"
                     % (end - p, total, total_sum)))
    return errs


def check_material(buf, start, end, idx):
    errs = []
    try:
        kids = children(buf, start, end)
    except Bad as e:
        return [("material", e.off, "#%d %s" % (idx, e.detail))]
    st = first(kids, STRUCT)
    if st is None:
        return [("material", start, "#%d sin STRUCT" % idx)]
    ext = first(kids, EXTENSION)
    if ext is None:
        return [("material", start, "#%d sin extension (Material::streamRead devuelve nil)" % idx)]
    errs.extend(check_plugins(buf, ext[2], ext[3], 0))
    return [("material", off, "#%d %s" % (idx, det)) for _s, off, det in errs]


def check_matlist(buf, start, end):
    errs = []
    try:
        kids = children(buf, start, end)
    except Bad as e:
        return [("matlist", e.off, e.detail)]
    st = first(kids, STRUCT)
    if st is None:
        return [("matlist", start, "sin STRUCT")]
    body_len = st[3] - st[2]
    if body_len < 4:
        return [("matlist", st[2], "STRUCT de %d bytes" % body_len)]
    num = struct.unpack_from("<i", buf, st[2])[0]
    if num < 0 or 4 + 4 * num > body_len:
        return [("matlist", st[2], "numMaterials=%d no cabe en STRUCT de %d" % (num, body_len))]
    if 4 + 4 * num != body_len:
        errs.append(("matlist", st[2], "STRUCT=%d bytes para %d materiales" % (body_len, num)))
    indices = [struct.unpack_from("<i", buf, st[2] + 4 + 4 * i)[0] for i in range(num)]
    mats = [k for k in kids if k[0] == MATERIAL]
    used = sum(1 for i in indices if i < 0)
    if used != len(mats):
        errs.append(("matlist", st[3], "%d materiales referenciados y %d en el chunk" % (used, len(mats))))
    for i, mat in enumerate(mats):
        errs.extend(check_material(buf, mat[2], mat[3], i))
    return errs


def check_struct(buf, start, end, version):
    errs = []
    n = end - start
    if n < 16:
        return [("struct", start, "%d bytes (minimo 16)" % n)]
    flags, ntris, nverts, nmorph = struct.unpack_from("<Iiii", buf, start)
    tex = (flags >> 16) & 0xFFFF
    if tex == 0:
        tex = 1 if flags & TEXTURED else (2 if flags & TEXTURED2 else 0)
    need = 16
    if version < 0x34000:
        need += 12
    if not (flags & NATIVE):
        if flags & PRELIT:
            need += 4 * nverts
        need += 8 * tex * nverts
        need += 8 * ntris
    if nverts > 0:
        need += 24 * nmorph
    if nverts < 0 or ntris < 0 or nmorph <= 0 or nverts > 2000000 or ntris > 4000000:
        errs.append(("struct", start, "verts=%d tris=%d morph=%d flags=%08x" % (nverts, ntris, nmorph, flags)))
    elif need > n:
        errs.append(("struct", start, "need=%d > struct=%d (verts=%d tris=%d)" % (need, n, nverts, ntris)))
    return errs


def mat_count(buf, ml):
    try:
        kids = children(buf, ml[2], ml[3])
    except Bad:
        return 0
    st = first(kids, STRUCT)
    if st is None or st[3] - st[2] < 4:
        return 0
    n = struct.unpack_from("<i", buf, st[2])[0]
    return n if 0 <= n <= 100000 else 0


def check_geometry(buf, start, end, idx):
    errs = []
    try:
        kids = children(buf, start, end)
    except Bad as e:
        return [("chunks", e.off, "geom %d: %s" % (idx, e.detail))], 0
    st = first(kids, STRUCT)
    if st is None:
        return [("struct", start, "geom %d sin STRUCT" % idx)], 0
    errs.extend([("struct", off, "geom %d: %s" % (idx, det)) for _s, off, det in check_struct(buf, st[2], st[3], unpack_version(st[1]))])
    ml = first(kids, MATLIST)
    nmats = 0
    if ml is None:
        errs.append(("matlist", start, "geom %d sin MATLIST" % idx))
    else:
        nmats = mat_count(buf, ml)
        errs.extend([("matlist", off, "geom %d: %s" % (idx, det)) for _s, off, det in check_matlist(buf, ml[2], ml[3])])
    ext = first(kids, EXTENSION)
    if ext is None:
        errs.append(("extension", start, "geom %d sin extension (Geometry::streamRead devuelve nil)" % idx))
    else:
        errs.extend([(s, off, "geom %d: %s" % (idx, det)) for s, off, det in check_plugins(buf, ext[2], ext[3], nmats)])
    return errs, nmats


def check_file(path):
    errs = []
    buf = open(path, "rb").read()
    try:
        top = children(buf, 0, len(buf))
    except Bad as e:
        # Los DFF del juego van alineados a sector: la cola de ceros es
        # relleno valido, no un chunk truncado.
        if any(b for b in buf[e.off:]):
            return [("top", e.off, e.detail)]
        top = children(buf, 0, e.off)
    clump = first(top, CLUMP)
    if clump is None:
        return [("top", 0, "sin clump 0x10")]
    try:
        kids = children(buf, clump[2], clump[3])
    except Bad as e:
        return [("clump", e.off, e.detail)]
    st = first(kids, STRUCT)
    if st is None:
        errs.append(("clump", clump[2], "sin STRUCT"))
    fl = first(kids, FRAMELIST)
    num_frames = 0
    if fl is None:
        errs.append(("clump", clump[2], "sin frame list"))
    else:
        try:
            fkids = children(buf, fl[2], fl[3])
        except Bad as e:
            fkids = []
            errs.append(("clump", e.off, "frame list: %s" % e.detail))
        fst = first(fkids, STRUCT)
        if fst is not None and fst[3] - fst[2] >= 4:
            num_frames = struct.unpack_from("<i", buf, fst[2])[0]
    gl = first(kids, GEOMETRYLIST)
    if gl is None:
        errs.append(("clump", clump[2], "sin geometry list"))
        return errs
    try:
        gkids = children(buf, gl[2], gl[3])
    except Bad as e:
        errs.append(("geomlist", e.off, e.detail))
        return errs
    inside = [k for k in gkids if k[0] == GEOMETRY]
    geoms = list(inside)
    for k in kids:
        if k[0] == GEOMETRY and k[4] > gl[4]:
            geoms.append(k)
    if not geoms:
        errs.append(("geomlist", gl[2], "sin geometrias"))
        return errs
    for i, g in enumerate(geoms):
        gerrs, _n = check_geometry(buf, g[2], g[3], i)
        errs.extend(gerrs)
    atomics = [k for k in kids if k[0] == ATOMIC]
    for i, a in enumerate(atomics):
        try:
            akids = children(buf, a[2], a[3])
        except Bad as e:
            errs.append(("atomic", e.off, "atomic %d: %s" % (i, e.detail)))
            continue
        ast = first(akids, STRUCT)
        if ast is None or ast[3] - ast[2] < 8:
            errs.append(("atomic", a[2], "atomic %d sin STRUCT de 8 bytes" % i))
            continue
        fi, gi = struct.unpack_from("<2I", buf, ast[2])
        if num_frames and fi >= num_frames:
            errs.append(("atomic", ast[2], "atomic %d frame=%d frames=%d" % (i, fi, num_frames)))
        if gi >= len(geoms):
            errs.append(("atomic", ast[2], "atomic %d geom=%d geoms=%d" % (i, gi, len(geoms))))
    return errs


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return 2
    files = []
    for a in args:
        if os.path.isdir(a):
            for root, _dirs, names in os.walk(a):
                for n in sorted(names):
                    if n.lower().endswith(".dff"):
                        files.append(os.path.join(root, n))
        elif os.path.isfile(a):
            files.append(a)
        else:
            print("%s: no existe" % a)
            return 2
    bad = 0
    for f in sorted(files):
        try:
            errs = check_file(f)
        except Bad as e:
            errs = [("crash", e.off, e.detail)]
        except (struct.error, IndexError, ValueError) as e:
            errs = [("crash", 0, "%s: %s" % (type(e).__name__, e))]
        if not errs:
            continue
        bad += 1
        for stage, off, det in errs:
            print("%s off=%d paso=%s %s" % (f, off, stage, det))
    print("%d ficheros, %d con errores" % (len(files), bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
