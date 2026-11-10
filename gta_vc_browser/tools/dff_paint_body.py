#!/usr/bin/env python3
"""Marca el material dominante de `chassis_hi` como material de pintura.

El motor solo repinta los materiales cuyo color es exactamente:
  primario   (60, 255, 0, 255)   -> color1 de carcols.dat
  secundario (255, 0, 175, 255)  -> color2 de carcols.dat
(`src/modelinfo/VehicleModelInfo.cpp:1311-1326`).

El cuerpo del banshee del pack Xbox estaba entero en el material 0 de
`chassis_hi`: blanco (255,255,255,255) y sin textura. Por eso el coche salia
blanco aunque el TXD fuese correcto. Tras recuperar el reparto del Banshee PC,
el cuerpo usa el material con textura `banshee86body128a`; este script marca
esa carroceria (o, si no existe, el material con mas triangulos) sin tocar
geometria, UVs ni texturas.

Uso:
    python tools/dff_paint_body.py --check  <dff...>
    python tools/dff_paint_body.py --paint  <dff...>
    python tools/dff_paint_body.py --paint --secondary <dff...>
"""
import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import dff_geom as dg

PRIMARY = (60, 255, 0, 255)
SECONDARY = (255, 0, 175, 255)
PAINT = (PRIMARY, SECONDARY)


def children(buf, start, end):
    pos = start
    while pos + 12 <= end:
        typ, size, ver = struct.unpack_from("<III", buf, pos)
        nxt = pos + 12 + size
        if nxt > end:
            return
        yield typ, ver, pos, pos + 12, nxt
        pos = nxt


def geom_positions(buf):
    """Chunks 0x0F en el mismo orden que dg.parse(): lista 0x1A y sueltos."""
    clump = next((c for c in children(buf, 0, len(buf)) if c[0] == 0x10), None)
    if clump is None:
        raise SystemExit("sin clump 0x10")
    out = []
    for typ, ver, pos, body, end in children(buf, clump[3], clump[4]):
        if typ == 0x1A:
            for t2, v2, p2, b2, e2 in children(buf, body, end):
                if t2 == 0x0F:
                    out.append((p2, b2, e2))
        elif typ == 0x0F:
            out.append((pos, body, end))
    return out


def color_offset(buf, gbody, gend, mat_index):
    for typ, ver, pos, body, end in children(buf, gbody, gend):
        if typ != 0x08:
            continue
        mats = [c for c in children(buf, body, end) if c[0] == 0x07]
        if mat_index >= len(mats):
            return None
        _, _, mpos, mbody, mend = mats[mat_index]
        for t2, v2, p2, b2, e2 in children(buf, mbody, mend):
            if t2 == 0x01:
                return b2 + 4
    return None


def process(path, paint, secondary):
    buf = bytearray(open(path, "rb").read())
    frames, geoms, atomics = dg.parse(buf)
    fi = next((i for i, f in enumerate(frames) if f["name"] == "chassis_hi"), None)
    if fi is None:
        print("%s: sin frame chassis_hi" % path)
        return 1
    gis = [gi for f, gi in atomics if f == fi and gi < len(geoms)]
    if not gis:
        print("%s: chassis_hi sin atomic" % path)
        return 1
    gi = max(gis, key=lambda i: len(geoms[i]["tris"]))
    g = geoms[gi]
    counts = {}
    for t in g["tris"]:
        counts[t[3]] = counts.get(t[3], 0) + 1
    if not counts:
        print("%s: chassis_hi sin triangulos" % path)
        return 1
    mi = max(counts, key=counts.get)
    # Si existe el material con textura de carroceria, ese es el que debe
    # llevar el marcador de pintura (aunque el atlas tenga mas triangulos).
    body_mi = next((i for i, (_c, t) in enumerate(g["mats"])
                    if t == "banshee86body128a" and counts.get(i, 0) > 0), None)
    if body_mi is not None:
        mi = body_mi
    color = g["mats"][mi][0] if mi < len(g["mats"]) else None
    target = SECONDARY if secondary else PRIMARY
    print("%s: chassis_hi geo=%d mat=%d tris=%d color=%s tex=%s" % (
        path, gi, mi, counts[mi], color,
        g["mats"][mi][1] if mi < len(g["mats"]) else None))
    if color in PAINT:
        print("  ya es marcador de pintura; no se toca")
        return 0
    pos = geom_positions(buf)
    if gi >= len(pos):
        print("  ERROR: no encuentro el chunk de la geometria %d" % gi)
        return 1
    off = color_offset(buf, pos[gi][1], pos[gi][2], mi)
    if off is None:
        print("  ERROR: no encuentro el STRUCT del material %d" % mi)
        return 1
    print("  color objetivo=%s offset=%d" % (target, off))
    if not paint:
        print("  --check: no se escribe")
        return 0
    struct.pack_into("<4B", buf, off, *target)
    with open(path, "wb") as f:
        f.write(buf)
    print("  pintado (mismo tamano)")
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--paint", action="store_true")
    ap.add_argument("--check", action="store_true",
                    help="solo valida (es el comportamiento por defecto)")
    ap.add_argument("--secondary", action="store_true")
    a = ap.parse_args()
    rc = 0
    for path in a.paths:
        if os.path.isdir(path):
            for name in sorted(os.listdir(path)):
                if name.endswith(".dff"):
                    rc |= process(os.path.join(path, name), a.paint, a.secondary)
        else:
            rc |= process(path, a.paint, a.secondary)
    return rc


if __name__ == "__main__":
    sys.exit(main())
