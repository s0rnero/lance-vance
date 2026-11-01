#!/usr/bin/env python3
"""Busca que permutacion relaciona dos texturas (para validar el decode DXT1)."""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from txd_raster import dict_textures, read_native, decode  # noqa: E402
import struct  # noqa: E402


def carga(txd, nombre):
    data = open(txd, "rb").read()
    for pos in dict_textures(data):
        t = read_native(data, pos)
        if t is None or t["nombre"].lower() != nombre.lower():
            continue
        size, off = t["levels"][0]
        return t, data[off:off + size]


def perms(w, h):
    idx = lambda x, y: y * w + x
    out = {}
    out["identidad"] = lambda x, y: idx(x, y)
    out["transpuesta"] = lambda x, y: idx(y, x)
    out["flipX"] = lambda x, y: idx(w - 1 - x, y)
    out["flipY"] = lambda x, y: idx(x, h - 1 - y)
    out["rot180"] = lambda x, y: idx(w - 1 - x, h - 1 - y)
    out["rot90"] = lambda x, y: idx(y, h - 1 - x)
    out["rot270"] = lambda x, y: idx(w - 1 - y, x)
    return out


def main():
    a = sys.argv[1]
    b = sys.argv[2]
    nombre = sys.argv[3]
    ta, ra = carga(a, nombre)
    tb, rb = carga(b, nombre)
    pa = decode(ra, ta["w"], ta["h"], ta["depth"], ta["fmt"], ta["has_alpha"], ta["compression"])
    pb = decode(rb, tb["w"], tb["h"], tb["depth"], tb["fmt"], tb["has_alpha"], tb["compression"])
    w, h = ta["w"], ta["h"]
    n = w * h
    idx = lambda x, y: y * w + x
    for nombre_perm, f in perms(w, h).items():
        iguales = sum(1 for y in range(h) for x in range(w)
                      if pa[idx(x, y)][:3] == pb[f(x, y)][:3])
        print("%-12s %d/%d (%.1f%%)" % (nombre_perm, iguales, n, 100.0 * iguales / n))


if __name__ == "__main__":
    main()
