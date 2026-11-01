#!/usr/bin/env python3
"""Genera el C con los bloques DXT1 de las texturas del retail de PC.

El retail de PC guarda "pointlight" (32x32) y "headlight" (128x128) en DXT1
dentro de models/particle.txd. El motor las genera en memoria (no dependen del
TXD que sirve el mod), asi que aqui se saca el bloque comprimido tal cual y se
emite el array de bytes que va embebido en el motor; el descompresor DXT1 esta
en src/renderer/PointlightTexture.cpp.

Uso:
    python tools/gen_retail_textures.py <particle.txd> > salida.cpp
"""
import argparse
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from txd_raster import dict_textures, read_native  # noqa: E402

TEXTURAS = ("pointlight", "headlight")


def bloque_nivel0(data, txd, nombre):
    for pos in dict_textures(data):
        t = read_native(data, pos)
        if t is None or t["nombre"].lower() != nombre.lower():
            continue
        if t["compression"] != 1:
            raise SystemExit("ERROR: %s no esta en DXT1 (comp=%d)" % (nombre, t["compression"]))
        size, off = t["levels"][0]
        return t, data[off:off + size]
    raise SystemExit("ERROR: no encontrada %s en %s" % (nombre, txd))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("txd")
    a = ap.parse_args()
    data = open(a.txd, "rb").read()
    for nombre in TEXTURAS:
        t, raw = bloque_nivel0(data, a.txd, nombre)
        esperado = t["w"] * t["h"] // 2
        if len(raw) != esperado:
            raise SystemExit("ERROR: %s mide %d bytes, se esperaban %d" % (nombre, len(raw), esperado))
        print("static const uint8 gs_au%sDxt1[%d] = {" % (nombre.capitalize(), len(raw)))
        for i in range(0, len(raw), 16):
            trozo = ", ".join("0x%02X" % b for b in raw[i:i + 16])
            print("\t%s," % trozo)
        print("};")
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
