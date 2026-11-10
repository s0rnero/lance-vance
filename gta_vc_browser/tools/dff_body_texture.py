#!/usr/bin/env python3
"""Pone la textura de carroceria en todo el `chassis_hi` del Banshee Xbox.

El pack deja el `chassis_hi` entero en el material 0, sin textura. El resto de
piezas (puertas, capo, parachoques, aletas) ya usan `banshee86body128a`. El
`chassis_hi` tiene una sola UV y su material 1 ya es `banshee86body128a`
(verde de pintura); este script le pasa todos los triangulos y el STRUCT a ese
material. No toca vertices, UVs, texturas ni ningun otro atomic, asi que el
volante y los intermitentes se conservan.

Uso:
    python tools/dff_body_texture.py \
        --target streamed/models/gta3.img/banshee.dff \
        --out tmp/base60/pilot/banshee.dff
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

PRELIT = 0x08
NATIVE = 0x01000000


def chassis(target_path):
    frames, geoms, atoms = dg.parse(open(target_path, "rb").read())
    fi = next(i for i, f in enumerate(frames) if f["name"] == "chassis_hi")
    gi = max([g for f, g in atoms if f == fi], key=lambda i: len(geoms[i]["tris"]))
    return frames, geoms, atoms, fi, gi


def parse_geom_body(geom_body):
    st = ext = None
    for c in xt.chunks(geom_body, 0, len(geom_body)):
        if c.typ == 0x01 and st is None:
            st = (c.ver, c.body)
        elif c.typ == 0x03 and ext is None:
            ext = (c.ver, c.body)
    if st is None or ext is None:
        raise SystemExit("geometria sin STRUCT o extension 0x03")
    return st, ext


def parse_mesh_ext(ext_body):
    for c in xt.chunks(ext_body, 0, len(ext_body)):
        if c.typ == 0x50E:
            return c.ver, c.body
    raise SystemExit("extension sin chunk de malla 0x50E")


def struct_tri_offsets(struct_body):
    flags, ntris, nverts, nmorph = struct.unpack_from("<Iiii", struct_body, 0)
    if flags & NATIVE or ntris <= 0 or nverts <= 0:
        return None, []
    ntex = (flags >> 16) & 0xFFFF
    if ntex == 0:
        ntex = 1 if flags & 0x04 else (2 if flags & 0x80 else 0)
    prelit = 4 * nverts if flags & PRELIT else 0
    morph = 16 + 8 + 12 * nverts * 2 * max(1, nmorph)

    def size(extra):
        return 16 + extra + prelit + 8 * ntex * nverts + 8 * ntris + morph

    extra = 12 if size(12) == len(struct_body) else 0
    start = 16 + extra + prelit + 8 * ntex * nverts
    return start, [start + i * 8 + 4 for i in range(ntris)]


def rebuild_geom(geom_body, st_ver, st_body, ext_ver, ext_body):
    out = b""
    for c in xt.chunks(geom_body, 0, len(geom_body)):
        if c.typ == 0x01:
            out += xt.ch(c.typ, st_ver, st_body)
        elif c.typ == 0x03:
            out += xt.ch(c.typ, ext_ver, ext_body)
        else:
            out += xt.ch(c.typ, c.ver, c.body)
    return out


def rebuild_ext(ext_body, mesh_ver, mesh_body):
    out = b""
    for c in xt.chunks(ext_body, 0, len(ext_body)):
        if c.typ == 0x50E:
            out += xt.ch(c.typ, mesh_ver, mesh_body)
        else:
            out += xt.ch(c.typ, c.ver, c.body)
    return out


def body_texture(target_path, out_path):
    frames, geoms, atoms, fi, gi = chassis(target_path)
    g = geoms[gi]
    body_mi = next((i for i, (_c, t) in enumerate(g["mats"])
                    if t == "banshee86body128a"), None)
    if body_mi is None:
        raise SystemExit("chassis_hi sin material banshee86body128a")
    print("chassis_hi geo=%d verts=%d tris=%d -> material %d (%s)" % (
        gi, len(g["verts"]), len(g["tris"]), body_mi, g["mats"][body_mi][1]))

    target = xt.parse_dff(target_path)
    geom_chunk = target["geoms"][gi]
    st, ext = parse_geom_body(geom_chunk.body)
    st_ver, st_body = st
    ext_ver, ext_body = ext
    mesh_ver, mesh_body = parse_mesh_ext(ext_body)
    flags, num, total = struct.unpack_from("<III", mesh_body, 0)
    if total != 3 * len(g["tris"]):
        raise SystemExit("malla %d indices para %d tris" % (total, len(g["tris"])))
    indices = list(struct.unpack_from("<%dI" % total, mesh_body, 12 + 8 * num))
    new_mesh = struct.pack("<III", flags, 1, total)
    new_mesh += struct.pack("<Ii", total, body_mi)
    new_mesh += struct.pack("<%dI" % total, *indices)
    new_ext = rebuild_ext(ext_body, mesh_ver, new_mesh)

    new_st = bytearray(st_body)
    tri_start, t1_offs = struct_tri_offsets(st_body)
    if tri_start is None:
        raise SystemExit("STRUCT sin triangulos utilizables")
    for off in t1_offs:
        t1 = struct.unpack_from("<I", st_body, off)[0]
        struct.pack_into("<I", new_st, off, (t1 & 0xFFFF0000) | (body_mi & 0xFFFF))
    new_geom = rebuild_geom(geom_chunk.body, st_ver, bytes(new_st), ext_ver, new_ext)
    target["geoms"][gi] = xt.Chunk(0x0F, geom_chunk.ver, new_geom,
                                    geom_chunk.pos, geom_chunk.nxt)
    out = xt.rebuild(target)
    with open(out_path, "wb") as f:
        f.write(out)
    print("escrito %s (%d bytes)" % (out_path, len(out)))

    checks = [("dff_structver.py", ["--check", "--quiet", out_path]),
              ("dff_paint_body.py", ["--check", out_path])]
    for script, args in checks:
        r = subprocess.run([sys.executable, os.path.join(HERE, script)] + args,
                           capture_output=True, text=True)
        print("%s: rc=%d %s" % (script, r.returncode, (r.stdout + r.stderr).strip()[-400:]))


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--target", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    os.makedirs(os.path.dirname(os.path.abspath(a.out)), exist_ok=True)
    body_texture(a.target, a.out)


if __name__ == "__main__":
    main()
