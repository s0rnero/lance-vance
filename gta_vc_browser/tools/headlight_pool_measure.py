#!/usr/bin/env python3
"""Cuantifica how much light a headlight pool texture puts on the road.

The pool is drawn additively with vertex color (45,45,45) and vertex alpha 128,
so the contribution of a pixel is tex * 45/255 (alpha does not scale the RGB of
an additive blend). The area covered by the bright part of the texture decides
whether the "foco" in front of the car is visible or not.

Uso:
    python tools/headlight_pool_measure.py <txd> <nombre>
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from txd_raster import dict_textures, read_native, decode  # noqa: E402


def main():
    txd, nombre = sys.argv[1], sys.argv[2]
    data = open(txd, "rb").read()
    for pos in dict_textures(data):
        t = read_native(data, pos)
        if t is None or t["nombre"].lower() != nombre.lower():
            continue
        size, off = t["levels"][0]
        px = decode(data[off:off + size], t["w"], t["h"], t["depth"], t["fmt"],
                    t["has_alpha"], t["compression"])
        w, h = t["w"], t["h"]
        n = w * h
        vals = [(p[0] * 30 + p[1] * 59 + p[2] * 11) // 100 for p in px]
        print("== %s :: %s  %dx%d" % (txd, t["nombre"], w, h))
        print("   media=%d  max=%d  negros=%d (%.0f%%)"
              % (sum(vals) / n, max(vals), sum(1 for v in vals if v == 0),
                 100.0 * sum(1 for v in vals if v == 0) / n))
        for umbral in (16, 64, 128, 192):
            c = sum(1 for v in vals if v > umbral)
            # El quad del charco son 7,0 m de frente x 5,5 m de lado.
            lado = (7.0 * 5.5 / (w * h)) ** 0.5
            print("   pixeles >%3d: %5d (%5.1f%%)  ->  mancha de %.2f m de lado, aporte max +%d"
                  % (umbral, c, 100.0 * c / n, lado * (c ** 0.5), umbral * 45 // 255))
        return 0
    print("no encontrada")
    return 1


if __name__ == "__main__":
    sys.exit(main())
