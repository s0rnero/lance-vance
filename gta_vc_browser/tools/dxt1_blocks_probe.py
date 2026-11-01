#!/usr/bin/env python3
"""Comprueba si los bloques DXT1 de un TXD corresponden a la imagen de 32 bits.

Compara los endpoints de color de cada bloque 4x4 con el min/max del bloque
equivalente de la version de 32 bits. Si coinciden, los datos DXT1 son los de
esa imagen y el fallo esta en como se leen los indices.

Uso:
    python tools/dxt1_blocks_probe.py <txd_dxt1> <txd_32b> <nombre>
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from txd_raster import dict_textures, read_native, decode  # noqa: E402


def carga(txd, nombre):
    data = open(txd, "rb").read()
    for pos in dict_textures(data):
        t = read_native(data, pos)
        if t is None or t["nombre"].lower() != nombre.lower():
            continue
        size, off = t["levels"][0]
        return t, data[off:off + size]


def main():
    a, b, nombre = sys.argv[1], sys.argv[2], sys.argv[3]
    ta, da = carga(a, nombre)
    tb, db = carga(b, nombre)
    ref = decode(db, tb["w"], tb["h"], tb["depth"], tb["fmt"], tb["has_alpha"], tb["compression"])
    w, h = ta["w"], ta["h"]
    nb = w // 4
    n = w * h
    # luminancia media por bloque de la referencia y de los endpoints DXT1
    ref_lum = []
    for by in range(h // 4):
        for bx in range(w // 4):
            s = 0
            for y in range(by * 4, by * 4 + 4):
                for x in range(bx * 4, bx * 4 + 4):
                    r, g, b, a_ = ref[y * w + x]
                    s += (r * 30 + g * 59 + b * 11) // 100
            ref_lum.append(s / 16.0)
    dxt_lum = []
    for i in range((w // 4) * (h // 4)):
        c0, c1 = struct.unpack_from("<HH", da, i * 8)
        def lum(v):
            r = ((v >> 11) & 0x1F) * 255 // 31
            g = ((v >> 5) & 0x3F) * 255 // 63
            b = (v & 0x1F) * 255 // 31
            return (r * 30 + g * 59 + b * 11) / 100.0
        dxt_lum.append((lum(c0) + lum(c1)) / 2.0)
    # correlacion
    mr = sum(ref_lum) / len(ref_lum)
    md = sum(dxt_lum) / len(dxt_lum)
    cov = sum((ref_lum[i] - mr) * (dxt_lum[i] - md) for i in range(len(ref_lum)))
    vr = sum((v - mr) ** 2 for v in ref_lum) ** 0.5
    vd = sum((v - md) ** 2 for v in dxt_lum) ** 0.5
    print("correlacion luminancia bloque a bloque: %.3f" % (cov / (vr * vd)))
    print("ref  media=%.1f  dxt media=%.1f" % (mr, md))
    print("primeros 12 bloques: ref=%s" % ["%.0f" % v for v in ref_lum[:12]])
    print("                     dxt=%s" % ["%.0f" % v for v in dxt_lum[:12]])


if __name__ == "__main__":
    main()
