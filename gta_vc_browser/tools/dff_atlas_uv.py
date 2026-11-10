#!/usr/bin/env python3
"""T7: alinea las UVs del atlas del `chassis_hi` del Banshee con el Banshee PC.

El T5 reparte bien los materiales (2010 tris cuerpo / 1946 tris atlas) usando el
Banshee PC como fuente de verdad, pero NO toca las UVs: los triángulos del atlas
siguen usando la UV del cuerpo Xbox, que no es la del atlas. El T6 (`--all-atlas`)
tampoco las alinea; solo pasa todo al atlas y por eso sale rojo/negro plano.

Este script parte del T5 y, SOLO para los triángulos del material atlas:

  * busca el triángulo del atlas PC más cercano al triángulo Xbox (punto a
    triángulo, igual que el reparto del T5);
  * interpola las UVs del atlas PC en cada vértice Xbox con coordenadas
    baricéntricas sobre ese triángulo PC;
  * duplica los tres vértices del triángulo Xbox y les pone esas UVs, dejando los
    vértices originales intactos para los triángulos del cuerpo;
  * actualiza los índices del STRUCT y del grupo atlas de la malla.

No cambia materiales, texturas, posiciones ni normales. El cuerpo conserva sus
UVs y sus vértices compartidos; el atlas deja de muestrear la UV del cuerpo.

Uso:
    python tools/dff_atlas_uv.py \
        --target tmp/base60/pilot/banshee.dff \
        --donor  tmp/base60/backup/streamed/banshee.dff \
        --out    tmp/base60/pilot/banshee-t7.dff
"""
import argparse
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import dff_geom as dg                 # noqa: E402
import dff_body_texture as bt         # noqa: E402
import dff_atlas_split as split       # noqa: E402
import xbox_transplant as xt          # noqa: E402

PRELIT = 0x08
ATLAS = "banshee868bit128"


def parse_struct(raw):
    flags, ntris, nverts, nmorph = struct.unpack_from("<Iiii", raw, 0)
    ntex = (flags >> 16) & 0xFFFF
    if ntex == 0:
        ntex = 1 if flags & 0x04 else (2 if flags & 0x80 else 0)
    prelit = 4 * nverts if flags & PRELIT else 0

    def size(extra):
        return (16 + extra + prelit + 8 * ntex * nverts + 8 * ntris
                + 16 + 8 + 12 * nverts * 2 * max(1, nmorph))

    extra = 12 if size(12) == len(raw) else 0
    p = 16 + extra
    colors_off = p
    p += prelit
    uv_off = p
    p += 8 * ntex * nverts
    tri_off = p
    p += 8 * ntris
    morph_off = p
    if morph_off + 24 > len(raw):
        raise SystemExit("STRUCT truncado antes del morph")
    return {
        "flags": flags, "ntris": ntris, "nverts": nverts, "nmorph": nmorph,
        "ntex": ntex, "prelit": prelit, "extra": extra,
        "colors_off": colors_off, "uv_off": uv_off, "tri_off": tri_off,
        "morph_off": morph_off,
    }


def parse_mesh(mesh):
    flags, num, total = struct.unpack_from("<III", mesh, 0)
    p = 12
    groups = []
    for _ in range(num):
        count, mat = struct.unpack_from("<Ii", mesh, p)
        p += 8
        idx = list(struct.unpack_from("<%dI" % count, mesh, p))
        p += 4 * count
        groups.append({"mat": mat, "idx": idx})
    if p != len(mesh):
        raise SystemExit("malla con %d bytes sobrantes" % (len(mesh) - p))
    if sum(len(g["idx"]) for g in groups) != total:
        raise SystemExit("malla: indices %d != total %d" % (
            sum(len(g["idx"]) for g in groups), total))
    return flags, groups


def rebuild_mesh(flags, groups):
    total = sum(len(g["idx"]) for g in groups)
    out = struct.pack("<III", flags, len(groups), total)
    for g in groups:
        out += struct.pack("<Ii", len(g["idx"]), g["mat"])
        out += struct.pack("<%dI" % len(g["idx"]), *g["idx"])
    return out


def barycentric(p, a, b, c):
    v0 = split.sub(b, a)
    v1 = split.sub(c, a)
    v2 = split.sub(p, a)
    d00 = split.dot(v0, v0)
    d01 = split.dot(v0, v1)
    d11 = split.dot(v1, v1)
    d20 = split.dot(v2, v0)
    d21 = split.dot(v2, v1)
    den = d00 * d11 - d01 * d01
    if abs(den) < 1e-12:
        return (1.0, 0.0, 0.0)
    v = (d11 * d20 - d01 * d21) / den
    w = (d00 * d21 - d01 * d20) / den
    return (1.0 - v - w, v, w)


def closest_on_tri(p, a, b, c):
    return split.closest_point_triangle(p, a, b, c)


def pc_atlas_tris(donor_path):
    pframes, pgeoms, patoms, pfi, pgi = bt.chassis(donor_path)
    pg = pgeoms[pgi]
    atlas_mi = split.atlas_material(pg["mats"], ATLAS)
    if atlas_mi is None:
        raise SystemExit("PC sin material %s" % ATLAS)
    m = dg.world_of(pframes, pfi)
    wv = [dg.mul(m, v) for v in pg["verts"]]
    uvs = pg["uvs"][0]
    out = []
    for t in pg["tris"]:
        if t[3] != atlas_mi:
            continue
        v0, v1, v2 = t[0], t[1], t[2]
        out.append((wv[v0], wv[v1], wv[v2],
                    uvs[v0], uvs[v1], uvs[v2]))
    print("PC atlas: %d triángulos en %s" % (len(out), os.path.basename(donor_path)))
    return out


def align(target_path, donor_path, out_path):
    frames, geoms, atoms, fi, gi = bt.chassis(target_path)
    g = geoms[gi]
    atlas_mi = split.atlas_material(g["mats"], ATLAS)
    if atlas_mi is None:
        raise SystemExit("target sin material %s" % ATLAS)
    m = dg.world_of(frames, fi)
    wv = [dg.mul(m, v) for v in g["verts"]]
    uvs = g["uvs"][0] if g["uvs"] else [(0.0, 0.0)] * len(g["verts"])
    tris = g["tris"]
    atlas_tris = [i for i, t in enumerate(tris) if t[3] == atlas_mi]
    if not atlas_tris:
        raise SystemExit("target sin triángulos del atlas")

    p_tris = pc_atlas_tris(donor_path)
    if not p_tris:
        raise SystemExit("PC sin triángulos del atlas")

    # Para cada triángulo del atlas Xbox: triángulo PC más cercano y UV por
    # esquina. La UV se interpola en el punto más cercano del triángulo PC.
    new_uv = {}
    pc_hit = 0
    for ti in atlas_tris:
        t = tris[ti]
        p = split.centroid((wv[t[0]], wv[t[1]], wv[t[2]]))
        best = None
        best_d = 1e30
        for q in p_tris:
            cp = closest_on_tri(p, q[0], q[1], q[2])
            d = split.dist2(p, cp)
            if d < best_d:
                best_d = d
                best = q
        pc_hit += 1
        uv_corners = []
        for vi in (t[0], t[1], t[2]):
            cp = closest_on_tri(wv[vi], best[0], best[1], best[2])
            bu, bv, bw = barycentric(cp, best[0], best[1], best[2])
            u = bu * best[3][0] + bv * best[4][0] + bw * best[5][0]
            v = bu * best[3][1] + bv * best[4][1] + bw * best[5][1]
            uv_corners.append((u, v))
        new_uv[ti] = uv_corners
    print("atlas target: %d triángulos alineados contra %d PC" % (pc_hit, len(p_tris)))

    target = xt.parse_dff(target_path)
    geom_chunk = target["geoms"][gi]
    st, ext = bt.parse_geom_body(geom_chunk.body)
    st_ver, st_body = st
    ext_ver, ext_body = ext
    meta = parse_struct(st_body)
    if meta["nmorph"] != 1:
        raise SystemExit("nmorph=%d: este script solo soporta 1" % meta["nmorph"])

    nverts = meta["nverts"]
    ntex = meta["ntex"]
    tri_raw = []
    for i in range(meta["ntris"]):
        t0, t1 = struct.unpack_from("<II", st_body, meta["tri_off"] + 8 * i)
        tri_raw.append([t0 >> 16, t0 & 0xFFFF, t1 >> 16, t1 & 0xFFFF])

    # UVs y colores antiguos, para copiar los vértices duplicados.
    old_uvs = []
    for s in range(ntex):
        vals = struct.unpack_from("<%df" % (2 * nverts), st_body,
                                  meta["uv_off"] + 8 * s * nverts)
        old_uvs.append([(vals[2 * i], vals[2 * i + 1]) for i in range(nverts)])
    old_colors = None
    if meta["prelit"]:
        old_colors = [st_body[meta["colors_off"] + 4 * i:meta["colors_off"] + 4 * i + 4]
                      for i in range(nverts)]

    old_verts_raw = st_body[meta["morph_off"] + 24:meta["morph_off"] + 24 + 12 * nverts]
    old_norms_raw = st_body[meta["morph_off"] + 24 + 12 * nverts:
                            meta["morph_off"] + 24 + 24 * nverts]
    if len(old_verts_raw) != 12 * nverts or len(old_norms_raw) != 12 * nverts:
        raise SystemExit("morph de vértices/normales truncado")

    # Nuevos vértices: copia de los originales + UV del atlas PC. Un vértice
    # nuevo por esquina de triángulo evita cualquier costura entre triángulos.
    new_verts_raw = bytearray()
    new_norms_raw = bytearray()
    new_colors = bytearray()
    new_uv_sets = [bytearray() for _ in range(ntex)]
    next_v = nverts
    tri_new = {}
    for ti in atlas_tris:
        t = tri_raw[ti]
        uv_corners = new_uv[ti]
        tri_new[ti] = []
        for corner, vi in enumerate((t[0], t[1], t[2])):
            new_verts_raw += old_verts_raw[12 * vi:12 * vi + 12]
            new_norms_raw += old_norms_raw[12 * vi:12 * vi + 12]
            if old_colors is not None:
                new_colors += old_colors[vi]
            for s in range(ntex):
                uv = uv_corners[corner] if s == 0 else old_uvs[s][vi]
                new_uv_sets[s] += struct.pack("<ff", uv[0], uv[1])
            tri_new[ti].append(next_v)
            next_v += 1

    new_nverts = next_v
    new_st = bytearray()
    new_st += struct.pack("<Iiii", meta["flags"], meta["ntris"], new_nverts, 1)
    new_st += st_body[16:16 + meta["extra"]]
    if old_colors is not None:
        for vi in range(nverts):
            new_st += old_colors[vi]
        new_st += new_colors
    for s in range(ntex):
        for vi in range(nverts):
            new_st += struct.pack("<ff", *old_uvs[s][vi])
        new_st += new_uv_sets[s]
    for ti, t in enumerate(tri_raw):
        if ti in tri_new:
            v0, v1, v2 = tri_new[ti]
            new_st += struct.pack("<II", (v0 << 16) | v1, (v2 << 16) | t[3])
        else:
            new_st += struct.pack("<II", (t[0] << 16) | t[1], (t[2] << 16) | t[3])
    new_st += st_body[meta["morph_off"]:meta["morph_off"] + 16]  # esfera
    new_st += st_body[meta["morph_off"] + 16:meta["morph_off"] + 24]  # hasv/hasn
    new_st += old_verts_raw + bytes(new_verts_raw)
    new_st += old_norms_raw + bytes(new_norms_raw)
    if len(new_st) != (16 + meta["extra"] + (4 * new_nverts if meta["prelit"] else 0)
                       + 8 * ntex * new_nverts + 8 * meta["ntris"]
                       + 16 + 8 + 12 * new_nverts * 2):
        raise SystemExit("STRUCT nuevo con tamaño inesperado: %d" % len(new_st))

    mesh_ver, mesh_body = bt.parse_mesh_ext(ext_body)
    mesh_flags, groups = parse_mesh(mesh_body)
    body_mi = split.atlas_material(g["mats"], split.BODY)
    if body_mi is None:
        raise SystemExit("target sin material %s" % split.BODY)
    old_mats = sorted(gp["mat"] for gp in groups)
    if old_mats != sorted([body_mi, atlas_mi]):
        raise SystemExit("malla del T5 con grupos inesperados: %s" % old_mats)

    # El T5 clasifica los triángulos por el orden de la malla, que no coincide
    # con el orden del STRUCT. Aquí se reconstruyen los dos grupos desde el
    # STRUCT: cuerpo con sus vértices originales y atlas con los duplicados.
    body_idx = []
    atlas_idx = []
    for ti, t in enumerate(tri_raw):
        if ti in tri_new:
            atlas_idx.extend(tri_new[ti])
        else:
            body_idx.extend(t[:3])
    if len(body_idx) != 3 * (len(tri_raw) - len(atlas_tris)) or len(atlas_idx) != 3 * len(atlas_tris):
        raise SystemExit("grupos reconstruidos inconsistentes")
    new_mesh = rebuild_mesh(mesh_flags, [
        {"mat": body_mi, "idx": body_idx},
        {"mat": atlas_mi, "idx": atlas_idx},
    ])
    new_ext = bt.rebuild_ext(ext_body, mesh_ver, new_mesh)
    new_geom = bt.rebuild_geom(geom_chunk.body, st_ver, bytes(new_st), ext_ver, new_ext)
    target["geoms"][gi] = xt.Chunk(0x0F, geom_chunk.ver, new_geom,
                                    geom_chunk.pos, geom_chunk.nxt)
    out = xt.rebuild(target)
    with open(out_path, "wb") as f:
        f.write(out)
    print("escrito %s (%d bytes, %d -> %d vértices)" % (
        out_path, len(out), nverts, new_nverts))

    checks = [("dff_structver.py", ["--check", "--quiet", out_path]),
              ("dff_paint_body.py", ["--check", out_path])]
    for script, args in checks:
        r = subprocess.run([sys.executable, os.path.join(HERE, script)] + args,
                           capture_output=True, text=True)
        print("%s: rc=%d %s" % (script, r.returncode, (r.stdout + r.stderr).strip()[-400:]))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--target", default="tmp/base60/pilot/banshee.dff")
    ap.add_argument("--donor", default="tmp/base60/backup/streamed/banshee.dff")
    ap.add_argument("--out", default="tmp/base60/pilot/banshee-t7.dff")
    a = ap.parse_args()
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    align(a.target, a.donor, a.out)


if __name__ == "__main__":
    main()
