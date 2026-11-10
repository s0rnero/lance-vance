#!/usr/bin/env python3
"""T8: UVs reales del Banshee PC para cuerpo y atlas del `chassis_hi` Xbox.

El T7 solo cambiaba las UVs de los triángulos del atlas e interpolaba UVs dentro
del triángulo PC más cercano. El cuerpo seguía usando las UVs Xbox (que en el
pack están asociadas a `belowcars`, no al cuerpo) y el atlas interpolado dejaba
202 triángulos con las tres UVs iguales.

El T5 ya tiene el reparto de materiales correcto (2010 cuerpo / 1946 atlas). Este
script parte del T5 y, para CADA triángulo:

  * busca el triángulo del PC del MISMO material más cercano por posición 3D;
  * elige la permutación de sus tres vértices que mejor encaja con el triángulo
    Xbox (sin invertir la normal);
  * copia las tres UVs REALES de ese triángulo PC a las tres esquinas Xbox;
  * duplica los vértices por esquina para no compartir UVs entre triángulos.

No interpola UVs, no inventa un reparto y no toca el TXD, los materiales ni el
resto de atomics. Si el triángulo PC es degenerado en UV se descarta como
donante; el triángulo Xbox busca el siguiente.

Uso:
    python tools/dff_donor_uv.py \
        --target tmp/base60/pilot/banshee.dff \
        --donor  tmp/base60/backup/streamed/banshee.dff \
        --out    tmp/base60/pilot/banshee-t8.dff
"""
import argparse
import itertools
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import dff_atlas_split as split     # noqa: E402
import dff_atlas_uv as t7           # noqa: E402
import dff_body_texture as bt       # noqa: E402
import dff_geom as dg               # noqa: E402
import xbox_transplant as xt        # noqa: E402

BODY = split.BODY
ATLAS = split.ATLAS


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def donor_tris(path):
    frames, geoms, _atoms, fi, gi = bt.chassis(path)
    g = geoms[gi]
    m = dg.world_of(frames, fi)
    wv = [dg.mul(m, v) for v in g["verts"]]
    uvs = g["uvs"][0]
    out = {BODY: [], ATLAS: []}
    for t in g["tris"]:
        tex = g["mats"][t[3]][1]
        key = BODY if tex == BODY else (ATLAS if tex == ATLAS else None)
        if key is None:
            continue
        p = (wv[t[0]], wv[t[1]], wv[t[2]])
        q = (uvs[t[0]], uvs[t[1]], uvs[t[2]])
        area = abs((q[1][0] - q[0][0]) * (q[2][1] - q[0][1])
                   - (q[2][0] - q[0][0]) * (q[1][1] - q[0][1]))
        if area < 1e-9:
            continue
        out[key].append((p, q))
    return out, len(g["tris"]), g


def best_uv(tp, dp, duv):
    """UVs del triángulo PC en el orden que mejor encaja con el triángulo Xbox."""
    tn = cross(split.sub(tp[1], tp[0]), split.sub(tp[2], tp[0]))
    best = None
    for perm in itertools.permutations(range(3)):
        dn = cross(split.sub(dp[perm[1]], dp[perm[0]]),
                   split.sub(dp[perm[2]], dp[perm[0]]))
        if split.dot(tn, dn) < 0:
            continue
        d = sum(split.dist2(tp[i], dp[perm[i]]) for i in range(3))
        if best is None or d < best[0]:
            best = (d, perm)
    if best is None:
        best = min((sum(split.dist2(tp[i], dp[perm[i]]) for i in range(3)), perm)
                   for perm in itertools.permutations(range(3)))
    perm = best[1]
    return [duv[perm[i]] for i in range(3)]


def nearest(tp, pool):
    p = split.centroid(tp)
    best = None
    best_d = 1e30
    for dp, duv in pool:
        cp = split.closest_point_triangle(p, dp[0], dp[1], dp[2])
        d = split.dist2(p, cp)
        if d < best_d:
            best_d = d
            best = (dp, duv)
    if best is None:
        raise SystemExit("sin donante para un triángulo")
    return best_uv(tp, best[0], best[1])


def transplant(target_path, donor_path, out_path):
    tframes, tgeoms, _tatoms, tfi, tgi = bt.chassis(target_path)
    tg = tgeoms[tgi]
    tm = dg.world_of(tframes, tfi)
    twv = [dg.mul(tm, v) for v in tg["verts"]]
    tris = tg["tris"]
    body_mi = split.atlas_material(tg["mats"], BODY)
    atlas_mi = split.atlas_material(tg["mats"], ATLAS)
    if body_mi is None or atlas_mi is None:
        raise SystemExit("target sin materiales cuerpo/atlas")

    pools, donor_total, dg_geom = donor_tris(donor_path)
    print("PC: %d tris, cuerpo=%d atlas=%d (no degenerados)" % (
        donor_total, len(pools[BODY]), len(pools[ATLAS])))
    print("Xbox T5: %d tris, cuerpo=%d atlas=%d" % (
        len(tris), sum(1 for t in tris if t[3] == body_mi),
        sum(1 for t in tris if t[3] == atlas_mi)))

    new_uv = {}
    counts = {BODY: 0, ATLAS: 0}
    for ti, t in enumerate(tris):
        tex = tg["mats"][t[3]][1]
        if tex == BODY:
            key = BODY
        elif tex == ATLAS:
            key = ATLAS
        else:
            raise SystemExit("triángulo %d con material inesperado %s" % (ti, tex))
        tp = (twv[t[0]], twv[t[1]], twv[t[2]])
        new_uv[ti] = nearest(tp, pools[key])
        counts[key] += 1
    print("UVs copiadas: cuerpo=%d atlas=%d" % (counts[BODY], counts[ATLAS]))

    target = xt.parse_dff(target_path)
    geom_chunk = target["geoms"][tgi]
    st, ext = bt.parse_geom_body(geom_chunk.body)
    st_ver, st_body = st
    ext_ver, ext_body = ext
    mesh_ver, mesh_body = bt.parse_mesh_ext(ext_body)
    mesh_flags, _groups = t7.parse_mesh(mesh_body)
    meta = t7.parse_struct(st_body)
    if meta["nmorph"] != 1:
        raise SystemExit("nmorph=%d: este script solo soporta 1" % meta["nmorph"])
    if meta["ntris"] != len(tris):
        raise SystemExit("STRUCT %d tris, malla %d" % (meta["ntris"], len(tris)))

    nverts = meta["nverts"]
    ntex = meta["ntex"]
    tri_raw = []
    for i in range(meta["ntris"]):
        t0, t1 = struct.unpack_from("<II", st_body, meta["tri_off"] + 8 * i)
        tri_raw.append([t0 >> 16, t0 & 0xFFFF, t1 >> 16, t1 & 0xFFFF])

    old_uvs = []
    for s in range(ntex):
        vals = struct.unpack_from("<%df" % (2 * nverts), st_body,
                                  meta["uv_off"] + 8 * s * nverts)
        old_uvs.append([(vals[2 * i], vals[2 * i + 1]) for i in range(nverts)])
    old_colors = None
    if meta["prelit"]:
        old_colors = [st_body[meta["colors_off"] + 4 * i:
                              meta["colors_off"] + 4 * i + 4]
                      for i in range(nverts)]

    old_verts_raw = st_body[meta["morph_off"] + 24:
                            meta["morph_off"] + 24 + 12 * nverts]
    old_norms_raw = st_body[meta["morph_off"] + 24 + 12 * nverts:
                            meta["morph_off"] + 24 + 24 * nverts]
    if len(old_verts_raw) != 12 * nverts or len(old_norms_raw) != 12 * nverts:
        raise SystemExit("morph de vértices/normales truncado")

    new_verts_raw = bytearray()
    new_norms_raw = bytearray()
    new_colors = bytearray()
    new_uv_sets = [bytearray() for _ in range(ntex)]
    next_v = nverts
    tri_new = {}
    body_idx = []
    atlas_idx = []
    for ti, t in enumerate(tris):
        new_idx = []
        for corner, vi in enumerate(t[:3]):
            new_verts_raw += old_verts_raw[12 * vi:12 * vi + 12]
            new_norms_raw += old_norms_raw[12 * vi:12 * vi + 12]
            if old_colors is not None:
                new_colors += old_colors[vi]
            for s in range(ntex):
                uv = new_uv[ti][corner] if s == 0 else old_uvs[s][vi]
                new_uv_sets[s] += struct.pack("<ff", uv[0], uv[1])
            new_idx.append(next_v)
            next_v += 1
        tri_new[ti] = new_idx
        if t[3] == body_mi:
            body_idx.extend(new_idx)
        elif t[3] == atlas_mi:
            atlas_idx.extend(new_idx)
        else:
            raise SystemExit("triángulo %d sin material cuerpo/atlas" % ti)

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
    for ti, t in enumerate(tris):
        v0, v1, v2 = tri_new[ti]
        new_st += struct.pack("<II", (v0 << 16) | v1, (v2 << 16) | t[3])
    new_st += st_body[meta["morph_off"]:meta["morph_off"] + 16]
    new_st += st_body[meta["morph_off"] + 16:meta["morph_off"] + 24]
    new_st += old_verts_raw + bytes(new_verts_raw)
    new_st += old_norms_raw + bytes(new_norms_raw)
    if len(new_st) != (16 + meta["extra"] + (4 * new_nverts if meta["prelit"] else 0)
                       + 8 * ntex * new_nverts + 8 * meta["ntris"]
                       + 16 + 8 + 12 * new_nverts * 2):
        raise SystemExit("STRUCT nuevo con tamaño inesperado: %d" % len(new_st))

    new_mesh = t7.rebuild_mesh(mesh_flags, [
        {"mat": body_mi, "idx": body_idx},
        {"mat": atlas_mi, "idx": atlas_idx},
    ])
    new_ext = bt.rebuild_ext(ext_body, mesh_ver, new_mesh)
    new_geom = bt.rebuild_geom(geom_chunk.body, st_ver, bytes(new_st),
                               ext_ver, new_ext)
    target["geoms"][tgi] = xt.Chunk(0x0F, geom_chunk.ver, new_geom,
                                    geom_chunk.pos, geom_chunk.nxt)
    out = xt.rebuild(target)
    with open(out_path, "wb") as f:
        f.write(out)
    print("escrito %s (%d bytes, %d -> %d vértices)" % (
        out_path, len(out), nverts, new_nverts))

    # Verificación: UVs por material y triángulos degenerados.
    frames, geoms, _atoms = dg.parse(out)
    gi = next(i for i, g in enumerate(geoms)
              if any(t == ATLAS for _c, t in g["mats"]))
    g = geoms[gi]
    uvs = g["uvs"][0]
    for mi, (_c, tex) in enumerate(g["mats"]):
        ts = [t for t in g["tris"] if t[3] == mi]
        if not ts:
            continue
        deg = 0
        vals = []
        for t in ts:
            q = [uvs[t[k]] for k in range(3)]
            vals.extend(q)
            area = abs((q[1][0] - q[0][0]) * (q[2][1] - q[0][1])
                       - (q[2][0] - q[0][0]) * (q[1][1] - q[0][1]))
            if area < 1e-9:
                deg += 1
        print("  mat %d %-20s tris=%-4d uv_unicas=%-5d degenerados=%d" % (
            mi, tex, len(ts), len(set((round(u, 5), round(v, 5)) for u, v in vals)), deg))

    checks = [("dff_structver.py", ["--check", "--quiet", out_path]),
              ("dff_paint_body.py", ["--check", out_path])]
    for script, args in checks:
        r = subprocess.run([sys.executable, os.path.join(HERE, script)] + args,
                           capture_output=True, text=True)
        print("%s: rc=%d %s" % (script, r.returncode,
                                (r.stdout + r.stderr).strip()[-500:]))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--target", default="tmp/base60/pilot/banshee.dff")
    ap.add_argument("--donor", default="tmp/base60/backup/streamed/banshee.dff")
    ap.add_argument("--out", default="tmp/base60/pilot/banshee-t8.dff")
    a = ap.parse_args()
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    transplant(a.target, a.donor, a.out)


if __name__ == "__main__":
    main()
