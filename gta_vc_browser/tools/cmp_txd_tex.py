#!/usr/bin/env python3
"""Compara dos texturas con el mismo nombre en dos TXD distintos.

Sirve para responder "¿el mod cambio el dibujo de esta textura o solo la
formato (DXT1 -> 8888)?" pixel a pixel.

Uso:
    python tools/cmp_txd_tex.py <txd_a> <txd_b> <nombre> [--dump-a] [--dump-b]
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from txd_raster import dict_textures, read_native, decode  # noqa: E402


def carga(txd, nombre):
    data = open(txd, "rb").read()
    for pos in dict_textures(data):
        t = read_native(data, pos)
        if t is None:
            continue
        if t["nombre"].lower() != nombre.lower():
            continue
        size, off = t["levels"][0]
        px = decode(data[off:off + size], t["w"], t["h"], t["depth"], t["fmt"],
                    t["has_alpha"], t["compression"])
        return t, px
    return None, None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("a")
    ap.add_argument("b")
    ap.add_argument("nombre")
    ap.add_argument("--dump-a", action="store_true")
    ap.add_argument("--dump-b", action="store_true")
    args = ap.parse_args()

    ta, pa = carga(args.a, args.nombre)
    tb, pb = carga(args.b, args.nombre)
    if ta is None or tb is None:
        print("no encontrada")
        return 1
    print("A: %s %dx%d prof=%d comp=%d" % (args.a, ta["w"], ta["h"], ta["depth"], ta["compression"]))
    print("B: %s %dx%d prof=%d comp=%d" % (args.b, tb["w"], tb["h"], tb["depth"], tb["compression"]))
    if len(pa) != len(pb):
        print("tamanos distintos")
        return 1
    n = len(pa)
    exactos = sum(1 for i in range(n) if pa[i][:3] == pb[i][:3])
    cercanos = sum(1 for i in range(n)
                   if abs(pa[i][0] - pb[i][0]) <= 8 and abs(pa[i][1] - pb[i][1]) <= 8
                   and abs(pa[i][2] - pb[i][2]) <= 8)
    print("pixels iguales: %d/%d (%.1f%%)   cercanos(+-8): %d/%d (%.1f%%)"
          % (exactos, n, 100.0 * exactos / n, cercanos, n, 100.0 * cercanos / n))
    if args.dump_a or args.dump_b:
        from txd_raster import ascii_dump
        if args.dump_a:
            print("--- A ---")
            for l in ascii_dump(pa, ta["w"], ta["h"]):
                print(l)
        if args.dump_b:
            print("--- B ---")
            for l in ascii_dump(pb, tb["w"], tb["h"]):
                print(l)
    return 0


if __name__ == "__main__":
    sys.exit(main())
