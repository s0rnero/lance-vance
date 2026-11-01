#!/usr/bin/env python3
"""Vuelca una textura de un TXD a PNG (para mirarla) y/o hace comparaciones.

Uso:
    python tools/tex_png.py <txd> <nombre> <salida.png> [--scale N] [--raw]
    python tools/tex_png.py <txd> --lista
"""
import argparse
import os
import struct
import sys
import zlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from txd_raster import dict_textures, read_native, decode  # noqa: E402


def escribir_png(ruta, px, w, h, escala=1):
    w2, h2 = w * escala, h * escala
    filas = []
    for y in range(h2):
        fila = bytearray([0])
        sy = y // escala
        for x in range(w2):
            r, g, b, a = px[sy * w + (x // escala)]
            fila += bytes((r, g, b))
        filas.append(bytes(fila))
    crudo = b"".join(filas)

    def trozo(tipo, datos):
        return (struct.pack(">I", len(datos)) + tipo + datos +
                struct.pack(">I", zlib.crc32(tipo + datos) & 0xFFFFFFFF))

    png = b"\x89PNG\r\n\x1a\n"
    png += trozo(b"IHDR", struct.pack(">IIBBBBB", w2, h2, 8, 2, 0, 0, 0))
    png += trozo(b"IDAT", zlib.compress(crudo, 9))
    png += trozo(b"IEND", b"")
    open(ruta, "wb").write(png)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("txd")
    ap.add_argument("nombre", nargs="?")
    ap.add_argument("salida", nargs="?")
    ap.add_argument("--scale", type=int, default=1)
    ap.add_argument("--lista", action="store_true")
    a = ap.parse_args()

    data = open(a.txd, "rb").read()
    textos = [read_native(data, p) for p in dict_textures(data)]
    textos = [t for t in textos if t]
    if a.lista or not a.nombre:
        for t in textos:
            print("%-24s %4dx%-4d prof=%2d comp=%d" % (t["nombre"], t["w"], t["h"], t["depth"], t["compression"]))
        return 0
    for t in textos:
        if t["nombre"].lower() != a.nombre.lower():
            continue
        size, off = t["levels"][0]
        px = decode(data[off:off + size], t["w"], t["h"], t["depth"], t["fmt"], t["has_alpha"], t["compression"])
        escribir_png(a.salida, px, t["w"], t["h"], a.scale)
        print("escrito %s (%dx%d)" % (a.salida, t["w"], t["h"]))
        return 0
    print("no encontrada")
    return 1


if __name__ == "__main__":
    sys.exit(main())
