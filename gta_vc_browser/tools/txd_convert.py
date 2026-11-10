#!/usr/bin/env python3
"""Convierte un TXD D3D8 del pack Xbox (DXT1/DXT3, 16 bits) a un TXD
D3D8 de 32 bits BGRA sin comprimir, que es el formato que carga el port web.

No toca los DFF. Conserva nombres, dimensiones, filtros, alfas y mascaras del
TXD de referencia del port cuando existe una textura con el mismo nombre.
Genera la cadena completa de mipmaps para que el motor no muestree negro.

Uso:
    python tools/txd_convert.py origen.txd salida.txd --port txd_pc.txd
"""
import argparse
import os
import struct
import sys

import txd_merge as tm
import txd_raster as tr

DICT = 0x16
STRUCT = 0x01
TEXNATIVE = 0x15
PLATFORM_D3D8 = 8


def dxt3_decode(data, w, h):
    """DXT3/BC2 -> [(r,g,b,a)] en orden de imagen."""
    px = [(0, 0, 0, 0)] * (w * h)
    for by in range(0, h, 4):
        for bx in range(0, w, 4):
            o = ((by // 4) * (w // 4) + (bx // 4)) * 16
            abits = struct.unpack_from("<Q", data, o)[0]
            c0 = struct.unpack_from("<H", data, o + 8)[0]
            c1 = struct.unpack_from("<H", data, o + 10)[0]
            bits = struct.unpack_from("<I", data, o + 12)[0]

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
                    x, y = bx + pxi, by + py
                    if x >= w or y >= h:
                        continue
                    idx = (bits >> (2 * (4 * py + pxi))) & 3
                    alfa = ((abits >> (4 * (4 * py + pxi))) & 0xF) * 17
                    r, g, b = pal[idx]
                    px[y * w + x] = (r, g, b, alfa)
    return px


def dxt5_decode(data, w, h):
    """DXT5/BC3 -> [(r,g,b,a)] en orden de imagen."""
    px = [(0, 0, 0, 0)] * (w * h)
    for by in range(0, h, 4):
        for bx in range(0, w, 4):
            o = ((by // 4) * (w // 4) + (bx // 4)) * 16
            a0, a1 = data[o], data[o + 1]
            abits = struct.unpack_from("<Q", data, o + 2)[0]
            c0 = struct.unpack_from("<H", data, o + 8)[0]
            c1 = struct.unpack_from("<H", data, o + 10)[0]
            bits = struct.unpack_from("<I", data, o + 12)[0]

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
            if a0 > a1:
                apal = [a0, a1,
                        (6 * a0 + a1) // 7, (5 * a0 + 2 * a1) // 7,
                        (4 * a0 + 3 * a1) // 7, (3 * a0 + 4 * a1) // 7,
                        (2 * a0 + 5 * a1) // 7, (a0 + 6 * a1) // 7]
            else:
                apal = [a0, a1,
                        (4 * a0 + a1) // 5, (3 * a0 + 2 * a1) // 5,
                        (2 * a0 + 3 * a1) // 5, (a0 + 4 * a1) // 5,
                        0, 255]

            for py in range(4):
                for pxi in range(4):
                    x, y = bx + pxi, by + py
                    if x >= w or y >= h:
                        continue
                    idx = (bits >> (2 * (4 * py + pxi))) & 3
                    aidx = (abits >> (3 * (4 * py + pxi))) & 7
                    r, g, b = pal[idx]
                    px[y * w + x] = (r, g, b, apal[aidx])
    return px


def decode_level(info, data):
    size, off = info["levels"][0]
    raw = data[off:off + size]
    w, h = info["w"], info["h"]
    if info["compression"] == 1:
        return tr.dxt1_decode(raw, w, h)
    if info["compression"] == 3:
        return dxt3_decode(raw, w, h)
    if info["compression"] == 5:
        return dxt5_decode(raw, w, h)
    if info["compression"] == 0:
        return tr.decode(raw, w, h, info["depth"], info["fmt"],
                         info["has_alpha"], 0)
    raise SystemExit("compresion no soportada: %d" % info["compression"])


def mip_chain(px, w, h):
    """[(w,h,bytes BGRA)] desde el nivel 0 hasta 1x1."""
    levels = []
    while True:
        bgra = bytearray(w * h * 4)
        for i, (r, g, b, a) in enumerate(px):
            bgra[i * 4:i * 4 + 4] = bytes((b, g, r, a))
        levels.append((w, h, bytes(bgra)))
        if w == 1 and h == 1:
            break
        nw, nh = max(1, w // 2), max(1, h // 2)
        out = []
        for y in range(nh):
            for x in range(nw):
                acc = [0, 0, 0, 0]
                n = 0
                for dy in range(2):
                    for dx in range(2):
                        sx, sy = x * 2 + dx, y * 2 + dy
                        if sx < w and sy < h:
                            r, g, b, a = px[sy * w + sx]
                            acc[0] += r; acc[1] += g; acc[2] += b; acc[3] += a
                            n += 1
                out.append((acc[0] // n, acc[1] // n, acc[2] // n, acc[3] // n))
        px, w, h = out, nw, nh
    return levels


def name32(s):
    b = s.encode("latin1", "replace")[:32]
    return b + b"\0" * (32 - len(b))


def convert(src, dst, port=None):
    ver, st_chunk, texs, cola, data = tm.read_txd(src)
    port_info = {}
    if port and os.path.exists(port):
        _, _, ptexs, _, pdata = tm.read_txd(port)
        for pos, _, name in ptexs:
            info = tr.read_native(pdata, pos)
            if info:
                port_info[name] = info

    chunks = []
    print("TXD origen: %s (%d texturas, %d bytes)" % (src, len(texs), os.path.getsize(src)))
    for pos, _, name in texs:
        info = tr.read_native(data, pos)
        if info is None:
            raise SystemExit("textura no entendida: %s" % name)
        px = decode_level(info, data)
        if px is None:
            raise SystemExit("no se pudo decodificar: %s" % name)
        alpha = any(a < 255 for _, _, _, a in px)
        p = port_info.get(name)
        if p:
            fmt = p["fmt"]
            has_alpha = p["has_alpha"]
            filt = p["filt"]
            mask = p["mask"]
            rtype = p["rtype"]
        else:
            fmt = 0x8600 if alpha else 0x8500
            has_alpha = 1 if alpha else 0
            filt = info["filt"]
            mask = info["mask"]
            rtype = info["rtype"]
        if alpha:
            has_alpha = 1
        levels = mip_chain(px, info["w"], info["h"])

        native = struct.pack("<II", PLATFORM_D3D8, filt)
        native += name32(name) + name32(mask)
        native += struct.pack("<I", fmt) + struct.pack("<i", has_alpha)
        native += struct.pack("<HH", info["w"], info["h"])
        native += bytes((32, len(levels), rtype, 0))
        for lw, lh, pixels in levels:
            native += struct.pack("<I", len(pixels)) + pixels
        st = struct.pack("<III", STRUCT, len(native), ver) + native
        chunks.append(struct.pack("<III", TEXNATIVE, len(st), ver) + st)
        print("  %-22s %dx%d fmt=0x%04x alpha=%d niveles=%d alfa_px=%s" % (
            name, info["w"], info["h"], fmt, has_alpha, len(levels), alpha))

    st = bytearray(st_chunk)
    struct.pack_into("<I", st, 12, len(chunks))
    body = bytes(st) + b"".join(chunks) + cola
    out = struct.pack("<III", DICT, len(body), ver) + body
    with open(dst, "wb") as f:
        f.write(out)

    # Validacion: releer y comprobar que todas son 32 bits sin comprimir.
    _, _, chk, _, chkdata = tm.read_txd(dst)
    names = [n for _, _, n in texs]
    got = [n for _, _, n in chk]
    if got != names:
        raise SystemExit("ERROR: nombres distintos: %s" % got)
    for pos, _, name in chk:
        info = tr.read_native(chkdata, pos)
        if info is None or info["compression"] != 0 or info["depth"] != 32:
            raise SystemExit("ERROR: %s sigue comprimida o no es 32 bits" % name)
    print("escrito %s: %d texturas, %d bytes, comp=0/32 bits"
          % (dst, len(chk), len(out)))


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src")
    ap.add_argument("dst")
    ap.add_argument("--port", help="TXD PC de referencia para fmt/filtro/mascara")
    a = ap.parse_args()
    convert(a.src, a.dst, a.port)


if __name__ == "__main__":
    main()
