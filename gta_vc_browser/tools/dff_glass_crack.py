#!/usr/bin/env python3
"""Vidrio lateral dañado: la grieta que el modelo ya usa en el parabrisas.

Los coches del port traen la luna de cada puerta como atomic aparte
(`window_*_hi`), colgado del marco `_ok`. Al abollar la puerta el juego oculta
ese marco y el hueco queda sin cristal: se ve el interior (un panel liso). Los
modelos vanilla que si ensenan la grieta llevan una malla `white128a` dentro de
`door_*_hi_dam`; en estos ports no vino.

Aqui se anade, por puerta, una copia de la luna del propio modelo con el
material de grietas del parabrisas danado (`white128a`, misma textura y alfa),
como `window_*_hi_dam`. Solo aparece cuando la puerta esta abollada y
desaparece cuando la puerta vuela.

Escribe en `tmp/glass606/out2/`; no toca `streamed`.

Uso:
    python tools/dff_glass_crack.py admiral police ambulan vicechee
"""
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO = HERE.parent
sys.path.insert(0, str(HERE))

import xbox_transplant as xt   # noqa: E402
import dff_geom as dg          # noqa: E402

SERVED = REPO / "streamed" / "models" / "gta3.img"
OUT = REPO / "tmp" / "glass606" / "out2"

DOORS = ("lf", "rf", "lr", "rr")
CRACK_TEX = "white128a"


def _chunks(buf):
    return list(xt.chunks(buf, 0, len(buf)))


def _texture_name(mat_body):
    tex = xt.find(_chunks(mat_body), 0x06)
    if tex is None:
        return None
    name = xt.find(_chunks(tex.body), 0x02)
    return name.body.split(b"\0")[0].decode("latin-1").lower() if name else None


def crack_material(d):
    for a in d["atoms"]:
        geom = d["geoms"][a["geom"]]
        matlist = xt.find(_chunks(geom.body), 0x08)
        if matlist is None:
            continue
        for mat in _chunks(matlist.body):
            if mat.typ == 0x07 and _texture_name(mat.body) == CRACK_TEX:
                return xt.ch(mat.typ, mat.ver, mat.body)
    raise SystemExit("%s: sin material %s en el DFF" % (d["path"], CRACK_TEX))


def normalize_uvs(struct_body, verts):
    """Lleva el primer juego de UV a 0..1: las lunas sin textura guardan UV
    diminutas y la grieta saldria como un borron de un solo texel. Si el juego
    es un solo punto (policia), se proyecta desde la posicion de los vertices."""
    flags, ntris, nverts, nmorph = struct.unpack_from("<Iiii", struct_body, 0)
    ntex = (flags >> 16) & 0xFFFF
    if ntex < 1 or nverts < 1:
        return struct_body

    def size(extra):
        t = 16 + extra
        if flags & dg.PRELIT:
            t += 4 * nverts
        t += 8 * ntex * nverts + 8 * ntris
        t += 16 + 8 + 12 * nverts * 2 * max(1, nmorph)
        return t

    extra = 0 if size(0) == len(struct_body) else 12
    p = 16 + extra + (4 * nverts if flags & dg.PRELIT else 0)
    vals = list(struct.unpack_from("<%df" % (2 * nverts), struct_body, p))
    us = vals[0::2]
    vs = vals[1::2]
    du = max(us) - min(us)
    dv = max(vs) - min(vs)
    out = bytearray(struct_body)
    if du < 1e-6 or dv < 1e-6:
        if len(verts) < nverts:
            return struct_body
        spans = [max(v[k] for v in verts) - min(v[k] for v in verts)
                 for k in range(3)]
        ax = sorted(range(3), key=lambda k: -spans[k])[:2]
        lo = [min(v[ax[0]] for v in verts), min(v[ax[1]] for v in verts)]
        hi = [max(v[ax[0]] for v in verts), max(v[ax[1]] for v in verts)]
        for i in range(nverts):
            struct.pack_into("<ff", out, p + 8 * i,
                             (verts[i][ax[0]] - lo[0]) / (hi[0] - lo[0] or 1.0),
                             (verts[i][ax[1]] - lo[1]) / (hi[1] - lo[1] or 1.0))
        return bytes(out)
    for i in range(nverts):
        struct.pack_into("<ff", out, p + 8 * i,
                         (us[i] - min(us)) / du, (vs[i] - min(vs)) / dv)
    return bytes(out)


def geom_with(geom, mat_bytes, verts):
    parts = []
    replaced = False
    for c in _chunks(geom.body):
        if c.typ == 0x08:
            sub = _chunks(c.body)
            st = xt.find(sub, 0x01)
            if st is None:
                raise SystemExit("MATLIST sin STRUCT")
            parts.append(xt.ch(0x08, c.ver,
                               xt.ch(0x01, st.ver, st.body) + mat_bytes))
            replaced = True
        elif c.typ == 0x01:
            parts.append(xt.ch(0x01, c.ver, normalize_uvs(c.body, verts)))
        else:
            parts.append(xt.ch(c.typ, c.ver, c.body))
    if not replaced:
        raise SystemExit("geometria sin MATLIST")
    return xt.Chunk(0x0F, geom.ver, b"".join(parts), 0, 0)


def atomic_with(raw, frame, geom):
    parts = []
    for c in _chunks(raw):
        if c.typ == 0x01:
            body = bytearray(c.body)
            struct.pack_into("<II", body, 0, frame, geom)
            parts.append(xt.ch(0x01, c.ver, bytes(body)))
        else:
            parts.append(xt.ch(c.typ, c.ver, c.body))
    return b"".join(parts)


def frame_atom(d, name):
    if name not in d["names"]:
        return None, None
    fi = d["names"].index(name)
    for a in d["atoms"]:
        if a["frame"] == fi:
            return fi, a
    return fi, None


def add_panes(d):
    added = []
    mat = crack_material(d)
    for door in DOORS:
        pane = "window_%s_hi" % door
        damaged = "door_%s_hi_dam" % door
        if damaged not in d["names"]:
            continue
        fi, a = frame_atom(d, pane)
        if a is None:
            continue
        new_name = pane + "_dam"
        if new_name in d["names"]:
            print("  %s: ya existe; se salta" % new_name)
            continue
        new_fi = len(d["records"])
        new_gi = len(d["geoms"])
        rec = bytearray(d["records"][fi])
        struct.pack_into("<i", rec, 48, d["parents"][fi])
        d["records"].append(bytes(rec))
        d["names"].append(new_name)
        d["name_bodies"].append(xt.ch(0x253F2FE, 0xC02FFFF,
                                      new_name.encode("latin-1")))
        pane_geom = dg.parse(open(d["path"], "rb").read())[1][a["geom"]]
        d["geoms"].append(geom_with(d["geoms"][a["geom"]], mat, pane_geom["verts"]))
        ai = d["atoms"].index(a)
        _, raw = d["atomic_raw"][ai]
        d["atomic_raw"].append((a["outer_ver"], atomic_with(raw, new_fi, new_gi)))
        added.append("%s(%dtris)" % (new_name, len(pane_geom["tris"])))
    return added


def fix(model, src=SERVED):
    path = Path(src) / (model + ".dff")
    d = xt.parse_dff(str(path))
    before_frames = len(d["records"])
    before_atoms = len(d["atomic_raw"])
    added = add_panes(d)
    OUT.mkdir(parents=True, exist_ok=True)
    dst = OUT / (model + ".dff")
    dst.write_bytes(xt.rebuild(d))
    print("%-9s frames %d->%d atomics %d->%d | %s"
          % (model, before_frames, len(d["records"]),
             before_atoms, len(d["atomic_raw"]), ", ".join(added) or "-"))
    return dst


if __name__ == "__main__":
    models = sys.argv[1:] or ["admiral", "police", "ambulan", "vicechee"]
    for m in models:
        fix(m)
