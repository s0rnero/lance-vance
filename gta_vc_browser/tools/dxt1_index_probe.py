#!/usr/bin/env python3
"""Busca el mapeo de indices DXT1 que reproduce la version de 32 bits.

Los bloques 4x4 del DXT1 del retail coinciden bloque a bloque con la imagen
reimportada (correlacion 0.999), asi que el fallo esta en como se leen los
16 indices de 2 bits dentro del bloque.

Uso:
    python tools/dxt1_index_probe.py <txd_dxt1> <txd_32b> <nombre>
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


def to_rgb(v):
    return (((v >> 11) & 0x1F) * 255 // 31,
            ((v >> 5) & 0x3F) * 255 // 63,
            (v & 0x1F) * 255 // 31)


def paleta(c0, c1):
    r0, g0, b0 = to_rgb(c0)
    r1, g1, b1 = to_rgb(c1)
    if c0 > c1:
        return [(r0, g0, b0), (r1, g1, b1),
                ((2 * r0 + r1) // 3, (2 * g0 + g1) // 3, (2 * b0 + b1) // 3),
                ((r0 + 2 * r1) // 3, (g0 + 2 * g1) // 3, (b0 + 2 * b1) // 3)]
    return [(r0, g0, b0), (r1, g1, b1),
            ((r0 + r1) // 2, (g0 + g1) // 2, (b0 + b1) // 2),
            (0, 0, 0)]


def rev32(v):
    r = 0
    for i in range(32):
        if v & (1 << i):
            r |= 1 << (31 - i)
    return r


MAPAS = {
    "estandar":      (lambda w, py, pxi: 4 * py + pxi, False, False),
    "transpuesta":   (lambda w, py, pxi: 4 * pxi + py, False, False),
    "flipY":         (lambda w, py, pxi: 4 * (3 - py) + pxi, False, False),
    "flipX":         (lambda w, py, pxi: 4 * py + (3 - pxi), False, False),
    "rot180":        (lambda w, py, pxi: 4 * (3 - py) + (3 - pxi), False, False),
    "revbits":       (lambda w, py, pxi: 4 * py + pxi, True, False),
    "byteswap":      (lambda w, py, pxi: 4 * py + pxi, False, True),
    "rev+transp":    (lambda w, py, pxi: 4 * pxi + py, True, False),
    "rev+flipY":     (lambda w, py, pxi: 4 * (3 - py) + pxi, True, False),
    "byteswap+trans": (lambda w, py, pxi: 4 * pxi + py, False, True),
}


def main():
    a, b, nombre = sys.argv[1], sys.argv[2], sys.argv[3]
    ta, da = carga(a, nombre)
    tb, db = carga(b, nombre)
    ref = decode(db, tb["w"], tb["h"], tb["depth"], tb["fmt"], tb["has_alpha"], tb["compression"])
    w, h = ta["w"], ta["h"]
    nb = w // 4
    for etiqueta, (kf, rev, swap) in MAPAS.items():
        px = [(0, 0, 0, 0)] * (w * h)
        for by in range(h // 4):
            for bx in range(w // 4):
                o = (by * nb + bx) * 8
                c0, c1 = struct.unpack_from("<HH", da, o)
                bits = struct.unpack_from("<I", da, o + 4)[0]
                if swap:
                    bits = struct.unpack(">I", da[o + 4:o + 8])[0]
                if rev:
                    bits = rev32(bits)
                pal = paleta(c0, c1)
                for py in range(4):
                    for pxi in range(4):
                        idx = (bits >> (2 * kf(w, py, pxi))) & 3
                        px[(by * 4 + py) * w + bx * 4 + pxi] = pal[idx]
        n = w * h
        ig = sum(1 for i in range(n) if px[i][:3] == ref[i][:3])
        print("%-16s %d/%d (%.1f%%)" % (etiqueta, ig, n, 100.0 * ig / n))


if __name__ == "__main__":
    main()
