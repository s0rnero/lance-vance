#!/usr/bin/env python3
"""Replica en frio el test del motor sobre las filas de `BodyLensTable.h`.

El plan 59.0 declaró filas de carrocería y confió en que `tint=1` encendiera la
cara entera; lo que el jugador vio fueron faros blancos y pilotos rojos
amarilleando. Este lector dice, fila a fila y sin tocar nada, que resuelve el
motor de verdad: que atomic escoge `ViceExtBodyLensByPosCB`, cuantos triángulos
del filtro caen dentro de los 0,35 m de `ViceExtBodyLensExclusiveCB`, y que
textura y cuanta area ajena arrastra ese filtro.

Uso:
    python tools/body_lens_audit.py mrwhoop,rumpo
    python tools/body_lens_audit.py            # los nueve del plan 62.0
"""
import math
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import indicator_audit as ia          # noqa: E402
import indicator_lens45 as L          # noqa: E402

REPO = os.path.dirname(os.path.dirname(HERE))
HDR = os.path.join(REPO, "src", "vehicles", "BodyLensTable.h")
DEFAULT = "mrwhoop,fbiranch,rumpo,pony,topfun,fbicar,boxville,mule,esperant"
ROW = re.compile(
    r'\{\s*"([a-z0-9_]+)",\s*"([a-z0-9_]+)",\s*CAR_INDICATOR_(\w+),\s*(-?\d+),'
    r'\s*\{\s*(\d+),\s*(\d+),\s*(\d+)\s*\},\s*0x([0-9a-fA-F]+),\s*(\d+),'
    r'\s*\{\s*(-?\d+),\s*(-?\d+),\s*(-?\d+)\s*\}(?:,\s*(\d+))?\s*\}')


def rows(models):
    out = []
    with open(HDR, encoding="utf-8") as f:
        for line in f:
            m = ROW.search(line)
            if m and m.group(1) in models:
                out.append({"model": m.group(1), "frame": m.group(2),
                            "node": m.group(3), "side": int(m.group(4)),
                            "rgb": (int(m.group(5)), int(m.group(6)), int(m.group(7))),
                            "mats": int(m.group(8), 16), "area": int(m.group(9)),
                            "xyz": [int(m.group(10)) / 100.0, int(m.group(11)) / 100.0,
                                    int(m.group(12)) / 100.0],
                            "coverage": int(m.group(13) or 0)})
    return out


def dist(a, b):
    return math.sqrt(sum((x - y) ** 2 for x, y in zip(a, b)))


def audit(model, faces):
    rs = [r for r in rows({model}) if r["model"] == model]
    centros = [r["xyz"] for r in rs]
    byframe = {}
    for f in faces:
        byframe.setdefault(f["frame"], []).append(f)
    print("=" * 78)
    print("%s  filas=%d" % (model, len(rs)))
    for r in rs:
        cand = [f for f in byframe.get(r["frame"], []) if (r["mats"] >> f["mi"]) & 1]
        if not cand:
            print("  %-18s %-4s side=%+d  FILA MUERTA: ningun triangulo del "
                  "filtro en %s" % (r["frame"], r["node"], r["side"], r["frame"]))
            continue
        best = None
        for gi in set(f["gi"] for f in cand):
            sub = [f for f in cand if f["gi"] == gi]
            hits = [f for f in sub if dist(f["world"], r["xyz"]) <= 0.25]
            d = min((dist(f["world"], r["xyz"]) for f in hits), default=9.9)
            key = (len(hits), -d)
            if best is None or key > best[0]:
                best = (key, sub, hits, d)
        _k, sub, hits, d = best
        dentro = [f for f in sub if any(dist(f["world"], c) <= 0.35 for c in centros)]
        fuera = [f for f in sub if not any(dist(f["world"], c) <= 0.35 for c in centros)]
        print("  %-18s %-4s side=%+d area=%-5d cov=%d tris=%-4d hits<=025=%-3d "
              "dist=%.3f EXCL %d/%d tint=%d"
              % (r["frame"], r["node"], r["side"], r["area"], r["coverage"],
                 len(sub), len(hits), d, len(dentro), len(fuera),
                 1 if dentro and not fuera else 0))
        for tex in sorted(set((f["mi"], f["tex"]) for f in sub)):
            mi, t = tex
            n = [f for f in sub if f["mi"] == mi]
            am = [f for f in n if L.is_lens_face(f)]
            print("      mat%-2d %-26s tris=%-4d cristal=%-4d area=%8.1f cm2"
                  % (mi, t, len(n), len(am), sum(f["area"] for f in n) * 10000))


def masks(model):
    """La mascara que debe llevar cada fila despues del split del 62.0.

    Lee el DFF servido tal como esta ahora y toma los indices de material de las
    caras de cristal a <= 0,12 m del `xyz` de la fila. Es la unica forma de
    escribir la mascara sin copiar a mano los slots que asigno el split.
    """
    _b, _fr, _ge, _at, faces = L.measure(model)
    print("=" * 78)
    print("%s --masks" % model)
    for r in [x for x in rows({model}) if x["model"] == model]:
        mis = set()
        tris = 0
        for f in faces:
            if f["frame"] != r["frame"] or f["mod"] or "_dam" in f["frame"]:
                continue
            d = dist(f["world"], r["xyz"])
            if d > 0.12:
                continue
            if not L.is_any_lens_face(f):
                continue
            mis.add(f["mi"])
            tris += 1
        nueva = sum(1 << m for m in mis)
        vieja = r["mats"]
        print("  %-18s %-4s side=%+d  vieja=0x%04x nueva=0x%04x  mats=%s tris=%d %s"
              % (r["frame"], r["node"], r["side"], vieja, nueva,
                 sorted(mis), tris, "IGUAL" if nueva == vieja else "CAMBIA"))


def main():
    modo = sys.argv[1] if len(sys.argv) > 1 and sys.argv[1].startswith("--") else None
    modelos = (sys.argv[2] if modo else (sys.argv[1] if len(sys.argv) > 1 else DEFAULT))
    for m in modelos.split(","):
        if modo == "--masks":
            masks(m)
        else:
            _b, _fr, _ge, _at, faces = L.measure(m)
            audit(m, faces)


if __name__ == "__main__":
    main()
