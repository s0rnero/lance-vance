#!/usr/bin/env python3
"""Prueba variantes del decode DXT1 contra una version de 32 bits conocida.

El retail de PC guarda estas texturas en DXT1; hace falta saber que variante
de decode reproduce la version reimportada a 32 bits (gamefiles) para saber
cual es la imagen real.

Uso:
    python tools/dxt1_probe.py <txd_dxt1> <txd_32b> <nombre>
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


def paleta(c0, c1):
    def to_rgb(v):
        return (((v >> 11) & 0x1F) * 255 // 31,
                ((v >> 5) & 0x3F) * 255 // 63,
                (v & 0x1F) * 255 // 31)
    r0, g0, b0 = to_rgb(c0)
    r1, g1, b1 = to_rgb(c1)
    if c0 > c1:
        return [(r0, g0, b0), (r1, g1, b1),
                ((2 * r0 + r1) // 3, (2 * g0 + g1) // 3, (2 * b0 + b1) // 3),
                ((r0 + 2 * r1) // 3, (g0 + 2 * g1) // 3, (b0 + 2 * b1) // 3)]
    return [(r0, g0, b0), (r1, g1, b1),
            ((r0 + r1) // 2, (g0 + g1) // 2, (b0 + b1) // 2),
            (0, 0, 0)]


def dec(d, w, h, idx_orden, bloque_orden):
    """Decodifica DXT1 escribiendo los pixeles en su sitio de imagen."""
    px = [(0, 0, 0, 0)] * (w * h)
    nb = w // 4
    for by in range(h // 4):
        for bx in range(w // 4):
            if bloque_orden == "fila":
                bi = by * nb + bx
            elif bloque_orden == "columna":
                bi = bx * (h // 4) + by
            else:  # zigzag
                bi = by * nb + (bx if by % 2 == 0 else nb - 1 - bx)
            o = bi * 8
            c0, c1 = struct.unpack_from("<HH", d, o)
            bits = struct.unpack_from("<I", d, o + 4)[0]
            pal = paleta(c0, c1)
            for py in range(4):
                for pxi in range(4):
                    if idx_orden == "estandar":
                        k = 4 * py + pxi
                    elif idx_orden == "transpuesta":
                        k = 4 * pxi + py
                    else:
                        k = 4 * py + pxi
                    idx = (bits >> (2 * k)) & 3
                    px[(by * 4 + py) * w + bx * 4 + pxi] = pal[idx]
    return px


def main():
    a, b, nombre = sys.argv[1], sys.argv[2], sys.argv[3]
    ta, da = carga(a, nombre)
    tb, db = carga(b, nombre)
    ref = decode(db, tb["w"], tb["h"], tb["depth"], tb["fmt"], tb["has_alpha"], tb["compression"])
    w, h = ta["w"], ta["h"]
    n = w * h
    for idx_orden in ("estandar", "transpuesta"):
        for bloque_orden in ("fila", "columna", "zigzag"):
            px = dec(da, w, h, idx_orden, bloque_orden)
            ig = sum(1 for i in range(n) if px[i][:3] == ref[i][:3])
            print("%-12s %-8s %d/%d (%.1f%%)" % (idx_orden, bloque_orden, ig, n, 100.0 * ig / n))


if __name__ == "__main__":
    main()
