#!/usr/bin/env python3
"""Extrae el raster (pixeles) de una textura nombrada de un TXD de RenderWare.

Reproduce el lector de texturas nativas de librw (`d3d8::readNativeTexture` y
`d3d9::readNativeTexture`) para poder ver los pixeles REALES que llegan al
motor: nombre, plataforma, formato, hasAlpha, tamano, profundidad, niveles y
un volcado ASCII del nivel 0.

Sirve para comparar la textura de retail (16 bits, 565) con la que trae el
mod (32 bits, 8888) sin depender del juego.

Uso:
    python tools/txd_raster.py <fichero.txd> <nombre> [--ascii] [--info]
    python tools/txd_raster.py <fichero.txd> --lista
"""
import argparse
import struct
import sys

ID_STRUCT = 0x01
ID_TEXTURE_NATIVE = 0x15
ID_DICT = 0x16

PLATFORM = {
    0x01000000: "PS2",
    0x02000000: "XBOX",
    0x03000000: "D3D8",
    0x03010000: "D3D9",
    0x04000000: "GL3",
}


def u32(b, o):
    return struct.unpack_from("<I", b, o)[0]


def dict_textures(data):
    """Offsets de cada textura nativa dentro del diccionario."""
    if u32(data, 0) != ID_DICT:
        raise SystemExit("ERROR: no es un TextureDictionary (0x%X)" % u32(data, 0))
    p = 12
    end = min(12 + u32(data, 4), len(data))
    if u32(data, p) != ID_STRUCT:
        raise SystemExit("ERROR: el diccionario no empieza por STRUCT")
    p += 12 + u32(data, p + 4)
    out = []
    while p + 12 <= end:
        t, size = u32(data, p), u32(data, p + 4)
        if t == ID_TEXTURE_NATIVE:
            out.append(p)
        p += 12 + size
    return out


def read_native(data, pos):
    """Lee la textura nativa en pos; devuelve un dict o None si no se entiende."""
    body = pos + 12
    if u32(data, body) != ID_STRUCT:
        return None
    o = body + 12
    platform = u32(data, o)
    o += 4
    filt = u32(data, o)
    o += 4
    nombre = data[o:o + 32].split(b"\0")[0].decode("latin1", "replace")
    o += 32
    mask = data[o:o + 32].split(b"\0")[0].decode("latin1", "replace")
    o += 32
    fmt = u32(data, o)
    o += 4
    has_alpha = struct.unpack_from("<i", data, o)[0]
    o += 4
    w, h = struct.unpack_from("<HH", data, o)
    o += 4
    depth = data[o]
    o += 1
    nlevels = data[o]
    o += 1
    rtype = data[o]
    o += 1
    compression = data[o]
    o += 1

    # El canal de alfa y el filtro solo existen en D3D8/D3D9; el resto de
    # plataformas los ponen en otro sitio, asi que aqui solo se leen las
    # plataformas que usa el juego (D3D8/D3D9).
    if platform in (0x03000000, 0x03010000):
        if fmt & 0x00000400:  # PAL8
            o += 4 * 256
        elif fmt & 0x00000800:  # PAL4
            o += 4 * 32
    levels = []
    for i in range(nlevels):
        size = u32(data, o)
        o += 4
        levels.append((size, o))
        o += size
    return dict(nombre=nombre, mask=mask, platform=platform, filt=filt,
                fmt=fmt, has_alpha=has_alpha, w=w, h=h, depth=depth,
                nlevels=nlevels, rtype=rtype, compression=compression,
                levels=levels)


def dxt1_decode(data, w, h):
    """DXT1 (BC1) -> lista de (r,g,b,a) en orden de imagen (fila a fila)."""
    px = [(0, 0, 0, 0)] * (w * h)
    for by in range(0, h, 4):
        for bx in range(0, w, 4):
            o = ((by // 4) * (w // 4) + (bx // 4)) * 8
            c0 = struct.unpack_from("<H", data, o)[0]
            c1 = struct.unpack_from("<H", data, o + 2)[0]
            bits = struct.unpack_from("<I", data, o + 4)[0]

            def to_rgb(v):
                return (((v >> 11) & 0x1F) * 255 // 31,
                        ((v >> 5) & 0x3F) * 255 // 63,
                        (v & 0x1F) * 255 // 31)

            r0, g0, b0 = to_rgb(c0)
            r1, g1, b1 = to_rgb(c1)
            if c0 > c1:
                pal = [(r0, g0, b0), (r1, g1, b1),
                       ((2 * r0 + r1) // 3, (2 * g0 + g1) // 3, (2 * b0 + b1) // 3),
                       ((r0 + 2 * r1) // 3, (g0 + 2 * g1) // 3, (b0 + 2 * b1) // 3)]
            else:
                pal = [(r0, g0, b0), (r1, g1, b1),
                       ((r0 + r1) // 2, (g0 + g1) // 2, (b0 + b1) // 2),
                       (0, 0, 0)]
            for py in range(4):
                for pxi in range(4):
                    idx = (bits >> (2 * (4 * py + pxi))) & 3
                    r, g, b = pal[idx]
                    px[(by + py) * w + (bx + pxi)] = (r, g, b, 255 if c0 > c1 else (255 if idx != 3 else 0))
    return px


def decode(level0, w, h, depth, fmt, has_alpha, compression=0):
    """Devuelve una lista de (r,g,b,a) por pixel."""
    px = []
    n = w * h
    if compression == 1:
        return dxt1_decode(level0, w, h)
    if depth == 16:
        for i in range(n):
            v = struct.unpack_from("<H", level0, i * 2)[0]
            r = ((v >> 11) & 0x1F) * 255 // 31
            g = ((v >> 5) & 0x3F) * 255 // 63
            b = (v & 0x1F) * 255 // 31
            px.append((r, g, b, 255))
    elif depth == 24:
        for i in range(n):
            b, g, r = level0[i * 3], level0[i * 3 + 1], level0[i * 3 + 2]
            px.append((r, g, b, 255))
    elif depth == 32:
        for i in range(n):
            b, g, r, a = level0[i * 4], level0[i * 4 + 1], level0[i * 4 + 2], level0[i * 4 + 3]
            px.append((r, g, b, a))
    else:
        return None
    return px


def ascii_dump(px, w, h):
    chars = " .:-=+*#%@"
    lines = []
    for y in range(h):
        s = ""
        for x in range(w):
            r, g, b, a = px[y * w + x]
            lum = (r * 30 + g * 59 + b * 11) // 100
            s += chars[min(9, lum * 10 // 256)]
        lines.append("   |%s|" % s)
    return lines


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("txd")
    ap.add_argument("nombre", nargs="?")
    ap.add_argument("--lista", action="store_true", help="solo lista los nombres")
    ap.add_argument("--ascii", action="store_true", help="volcado ASCII del nivel 0")
    ap.add_argument("--info", action="store_true", help="estadisticas de pixeles")
    a = ap.parse_args()

    data = open(a.txd, "rb").read()
    textos = []
    for pos in dict_textures(data):
        t = read_native(data, pos)
        if t is not None:
            textos.append(t)

    if a.lista or not a.nombre:
        for t in textos:
            print("   %-24s %4dx%-4d prof=%2d fmt=0x%04X alfa=%d niveles=%d comp=%d %s"
                  % (t["nombre"], t["w"], t["h"], t["depth"], t["fmt"], t["has_alpha"],
                     t["nlevels"], t["compression"], PLATFORM.get(t["platform"], "?")))
        return 0

    objetivo = a.nombre.lower()
    for t in textos:
        if t["nombre"].lower() != objetivo:
            continue
        print("== %s :: %s" % (a.txd, t["nombre"]))
        print("   plataforma=%s  filtro=0x%X  formato=0x%04X  hasAlpha=%d"
              % (PLATFORM.get(t["platform"], hex(t["platform"])), t["filt"], t["fmt"], t["has_alpha"]))
        print("   %dx%d prof=%d niveles=%d compresion=%d" % (t["w"], t["h"], t["depth"], t["nlevels"], t["compression"]))
        for i, (size, off) in enumerate(t["levels"]):
            print("   nivel %d: %d bytes en 0x%X" % (i, size, off))
        if not t["levels"]:
            return 1
        size, off = t["levels"][0]
        px = decode(data[off:off + size], t["w"], t["h"], t["depth"], t["fmt"], t["has_alpha"], t["compression"])
        if px is None:
            print("   (profundidad %d no soportada por esta herramienta)" % t["depth"])
            return 1
        if a.info:
            n = len(px)
            media = [sum(p[c] for p in px) / n for c in range(4)]
            mx = [max(p[c] for p in px) for c in range(4)]
            nz_a = sum(1 for p in px if p[3] > 0)
            nz_l = sum(1 for p in px if (p[0] * 30 + p[1] * 59 + p[2] * 11) // 100 > 5)
            print("   media R=%.1f G=%.1f B=%.1f A=%.1f  max R=%d G=%d B=%d A=%d"
                  % (media[0], media[1], media[2], media[3], mx[0], mx[1], mx[2], mx[3]))
            print("   pixeles con alfa>0: %d/%d (%.0f%%)   pixeles no negros: %d/%d (%.0f%%)"
                  % (nz_a, n, 100.0 * nz_a / n, nz_l, n, 100.0 * nz_l / n))
        if a.ascii:
            for l in ascii_dump(px, t["w"], t["h"]):
                print(l)
        return 0
    print("no encontrada: %s" % a.nombre)
    return 1


if __name__ == "__main__":
    sys.exit(main())
