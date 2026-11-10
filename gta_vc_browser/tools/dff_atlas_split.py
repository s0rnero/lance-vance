#!/usr/bin/env python3
"""Reparte el atlas `banshee868bit128` del Banshee PC en el `chassis_hi` Xbox.

El `chassis_hi` del pack Xbox viene entero en el material de carroceria
`banshee86body128a`; el atlas de detalles `banshee868bit128` existe en el TXD pero
tiene 0 triangulos. El Banshee del PC (backup) si trae el reparto: 482 de sus 976
triangulos usan el atlas y 384 el cuerpo, ambos con la PRIMERA UV (la segunda UV
del PC esta vacia: todo 0/1). Este script usa ese reparto como fuente de verdad:

  * cada triangulo del `chassis_hi` Xbox busca el triangulo PC mas cercano por
    posicion 3D (punto-triangulo, no centroide a centroide);
  * si el mas cercano es del material `banshee868bit128`, el triangulo Xbox pasa
    al material atlas; si no, se queda en `banshee86body128a`;
  * no toca vertices, UVs, texturas, STRUCT de materiales ni ningun otro atomic.

Con `--all-atlas` no usa el PC: pasa TODO el `chassis_hi` al material atlas y
marca ese material como pintura. Es el reparto del chasis Xbox original: sus
UVs ya son las del atlas `banshee868bit128`; la textura `banshee86body128a`
pertenece a las puertas/aletas y no debe ir en el chasis (T6).

El motor ya pinta el material de carroceria por color (`GetEditableMaterialListCB`);
el atlas queda blanco y conserva su textura oscura de detalles. El volante y los
intermitentes de respaldo no se tocan porque solo se reescribe `chassis_hi`.

Uso:
    python tools/dff_atlas_split.py \
        --target streamed/models/gta3.img/banshee.dff \
        --donor  tmp/base60/backup/streamed/banshee.dff \
        --out    tmp/base60/pilot/banshee.dff
"""
import argparse
import os
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import dff_geom as dg          # noqa: E402
import xbox_transplant as xt   # noqa: E402
import dff_body_texture as bt  # noqa: E402

ATLAS = "banshee868bit128"
BODY = "banshee86body128a"


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def closest_point_triangle(p, a, b, c):
    """Punto del triangulo ABC mas cercano a P (Ericson, Real-Time Collision)."""
    ab = sub(b, a)
    ac = sub(c, a)
    ap = sub(p, a)
    d1 = dot(ab, ap)
    d2 = dot(ac, ap)
    if d1 <= 0.0 and d2 <= 0.0:
        return a
    bp = sub(p, b)
    d3 = dot(ab, bp)
    d4 = dot(ac, bp)
    if d3 >= 0.0 and d4 <= d3:
        return b
    vc = d1 * d4 - d3 * d2
    if vc <= 0.0 and d1 >= 0.0 and d3 <= 0.0:
        v = d1 / (d1 - d3)
        return (a[0] + v * ab[0], a[1] + v * ab[1], a[2] + v * ab[2])
    cp = sub(p, c)
    d5 = dot(ab, cp)
    d6 = dot(ac, cp)
    if d6 >= 0.0 and d5 <= d6:
        return c
    vb = d5 * d2 - d1 * d6
    if vb <= 0.0 and d2 >= 0.0 and d6 <= 0.0:
        w = d2 / (d2 - d6)
        return (a[0] + w * ac[0], a[1] + w * ac[1], a[2] + w * ac[2])
    va = d3 * d6 - d5 * d4
    if va <= 0.0 and (d4 - d3) >= 0.0 and (d5 - d6) >= 0.0:
        w = (d4 - d3) / ((d4 - d3) + (d5 - d6))
        return (b[0] + w * (c[0] - b[0]),
                b[1] + w * (c[1] - b[1]),
                b[2] + w * (c[2] - b[2]))
    denom = 1.0 / (va + vb + vc)
    v = vb * denom
    w = vc * denom
    return (a[0] + ab[0] * v + ac[0] * w,
            a[1] + ab[1] * v + ac[1] * w,
            a[2] + ab[2] * v + ac[2] * w)


def dist2(p, q):
    d = sub(p, q)
    return dot(d, d)


def world_tris(frames, fi, g):
    """Triangulos del atomic `fi` en espacio mundo, con su material."""
    m = dg.world_of(frames, fi)
    wv = [dg.mul(m, v) for v in g["verts"]]
    out = []
    for t in g["tris"]:
        out.append((wv[t[0]], wv[t[1]], wv[t[2]], t[3]))
    return out


def centroid(t):
    return ((t[0][0] + t[1][0] + t[2][0]) / 3.0,
            (t[0][1] + t[1][1] + t[2][1]) / 3.0,
            (t[0][2] + t[1][2] + t[2][2]) / 3.0)


def atlas_material(mats, name):
    return next((i for i, (_c, t) in enumerate(mats) if t == name), None)


def split(target_path, donor_path, out_path, all_atlas=False):
    # ---- destino Xbox -----------------------------------------------------
    xframes, xgeoms, xatoms, xfi, xgi = bt.chassis(target_path)
    xg = xgeoms[xgi]
    xbody_mi = atlas_material(xg["mats"], BODY) if not all_atlas else None
    xatlas_mi = atlas_material(xg["mats"], ATLAS)
    if xatlas_mi is None or (not all_atlas and xbody_mi is None):
        raise SystemExit("chassis_hi Xbox sin material %s o %s" % (BODY, ATLAS))
    x_tris = world_tris(xframes, xfi, xg)
    print("Xbox %s: geo=%d verts=%d tris=%d body_mi=%s atlas_mi=%d" % (
        os.path.basename(target_path), xgi, len(xg["verts"]), len(x_tris),
        xbody_mi, xatlas_mi))

    if all_atlas:
        print("modo --all-atlas: todo el chassis_hi usa %s; no se lee el PC" % ATLAS)
        xmat = [xatlas_mi] * len(x_tris)
        hits = {ATLAS: len(x_tris)}
    else:
        # ---- fuente PC ----------------------------------------------------
        pframes, pgeoms, patoms, pfi, pgi = bt.chassis(donor_path)
        pg = pgeoms[pgi]
        p_atlas_mi = atlas_material(pg["mats"], ATLAS)
        if p_atlas_mi is None:
            raise SystemExit("chassis_hi PC sin material %s" % ATLAS)
        p_tris = world_tris(pframes, pfi, pg)
        pc_atlas = [t for t in p_tris if t[3] == p_atlas_mi]
        print("PC %s: geo=%d tris=%d atlas_mi=%d atlas_tris=%d" % (
            os.path.basename(donor_path), pgi, len(p_tris), p_atlas_mi, len(pc_atlas)))

        # Cada triangulo Xbox toma el material del triangulo PC mas cercano. Los
        # materiales PC que no son el atlas (motor, cristal, negro, salpicadero)
        # se quedan como cuerpo: este paso solo recupera el atlas de detalles.
        xmat = []
        hits = {BODY: 0, ATLAS: 0}
        for i, t in enumerate(x_tris):
            p = centroid(t)
            best = None
            best_d = 1e30
            for q in p_tris:
                cp = closest_point_triangle(p, q[0], q[1], q[2])
                d = dist2(p, cp)
                if d < best_d:
                    best_d = d
                    best = q
            mi = xatlas_mi if best is not None and best[3] == p_atlas_mi else xbody_mi
            xmat.append(mi)
            hits[ATLAS if mi == xatlas_mi else BODY] += 1
            if (i + 1) % 500 == 0:
                print("  %d/%d" % (i + 1, len(x_tris)))
        print("reparto Xbox: body=%d atlas=%d (PC atlas %.1f%% -> Xbox %.1f%%)" % (
            hits[BODY], hits[ATLAS], 100.0 * len(pc_atlas) / len(p_tris),
            100.0 * hits[ATLAS] / len(x_tris)))

    # ---- reescribir mesh + STRUCT del chassis_hi --------------------------
    target = xt.parse_dff(target_path)
    geom_chunk = target["geoms"][xgi]
    st, ext = bt.parse_geom_body(geom_chunk.body)
    st_ver, st_body = st
    ext_ver, ext_body = ext
    mesh_ver, mesh_body = bt.parse_mesh_ext(ext_body)
    flags, num, total = struct.unpack_from("<III", mesh_body, 0)
    if total != 3 * len(x_tris):
        raise SystemExit("malla %d indices para %d tris" % (total, len(x_tris)))
    indices = list(struct.unpack_from("<%dI" % total, mesh_body, 12 + 8 * num))

    if all_atlas:
        body_idx = []
        atlas_idx = list(indices)
        new_mesh = struct.pack("<III", flags, 1, total)
        new_mesh += struct.pack("<Ii", len(atlas_idx), xatlas_mi)
        new_mesh += struct.pack("<%dI" % len(atlas_idx), *atlas_idx)
    else:
        body_idx = []
        atlas_idx = []
        for ti, mi in enumerate(xmat):
            src = indices[3 * ti:3 * ti + 3]
            (atlas_idx if mi == xatlas_mi else body_idx).extend(src)
        if len(body_idx) != 3 * hits[BODY] or len(atlas_idx) != 3 * hits[ATLAS]:
            raise SystemExit("grupos inconsistentes: body=%d atlas=%d" % (
                len(body_idx), len(atlas_idx)))

        new_mesh = struct.pack("<III", flags, 2, total)
        new_mesh += struct.pack("<Ii", len(body_idx), xbody_mi)
        new_mesh += struct.pack("<%dI" % len(body_idx), *body_idx)
        new_mesh += struct.pack("<Ii", len(atlas_idx), xatlas_mi)
        new_mesh += struct.pack("<%dI" % len(atlas_idx), *atlas_idx)
    new_ext = bt.rebuild_ext(ext_body, mesh_ver, new_mesh)

    new_st = bytearray(st_body)
    tri_start, t1_offs = bt.struct_tri_offsets(st_body)
    if tri_start is None:
        raise SystemExit("STRUCT sin triangulos utilizables")
    if len(t1_offs) != len(xmat):
        raise SystemExit("STRUCT %d tris, malla %d" % (len(t1_offs), len(xmat)))
    for off, mi in zip(t1_offs, xmat):
        t1 = struct.unpack_from("<I", st_body, off)[0]
        struct.pack_into("<I", new_st, off, (t1 & 0xFFFF0000) | (mi & 0xFFFF))
    new_geom = bt.rebuild_geom(geom_chunk.body, st_ver, bytes(new_st), ext_ver, new_ext)
    target["geoms"][xgi] = xt.Chunk(0x0F, geom_chunk.ver, new_geom,
                                    geom_chunk.pos, geom_chunk.nxt)
    out = xt.rebuild(target)
    with open(out_path, "wb") as f:
        f.write(out)
    print("escrito %s (%d bytes)" % (out_path, len(out)))

    if all_atlas:
        r = subprocess.run([sys.executable, os.path.join(HERE, "dff_paint_body.py"),
                            "--paint", out_path], capture_output=True, text=True)
        print("dff_paint_body.py --paint: rc=%d %s" % (
            r.returncode, (r.stdout + r.stderr).strip()[-500:]))

    checks = [("dff_structver.py", ["--check", "--quiet", out_path]),
              ("dff_paint_body.py", ["--check", out_path])]
    for script, args in checks:
        r = subprocess.run([sys.executable, os.path.join(HERE, script)] + args,
                           capture_output=True, text=True)
        print("%s: rc=%d %s" % (script, r.returncode, (r.stdout + r.stderr).strip()[-500:]))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--target", default="streamed/models/gta3.img/banshee.dff")
    ap.add_argument("--donor", default="tmp/base60/backup/streamed/banshee.dff")
    ap.add_argument("--out", default="tmp/base60/pilot/banshee.dff")
    ap.add_argument("--all-atlas", action="store_true",
                    help="todo el chassis_hi al atlas (reparto Xbox; T6)")
    a = ap.parse_args()
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    split(a.target, a.donor, a.out, a.all_atlas)


if __name__ == "__main__":
    main()
