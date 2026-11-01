#!/usr/bin/env python3
"""Genera los datos del volante 3D que el motor anade a los coches que no lo
traen (el mod solo se lo puso a 18).

De donde sale:
  - la malla (238 verts / 94 tris) y su textura "stwheel" (64x64) salen de los
    coches que el mod adapto: gta_vc_browser/streamed/models/gta3.img/taxi.dff
    (marco "steeringwheel") y su .txd.

Salida: src/vehicles/SteeringWheelData.h
"""
import struct
import sys

sys.path.insert(0, __file__.rsplit('\\', 1)[0].rsplit('/', 1)[0])
import dff_geom as dg

DFF = r"gta_vc_browser\streamed\models\gta3.img\taxi.dff"
TXD = r"gta_vc_browser\streamed\models\gta3.img\taxi.txd"
OUT = r"src\vehicles\SteeringWheelData.h"


def textura(txd, name):
    buf = open(txd, "rb").read()
    i = buf.find(name.encode() + b"\0")
    assert i > 0, "no encuentro la textura"
    for p in range(i, min(len(buf), i + 8192), 2):
        w, h, depth, ptype = struct.unpack_from('<HHBB', buf, p)
        if 1 <= w <= 256 and 1 <= h <= 256 and depth in (16, 24, 32) and ptype in (0, 1, 2, 4, 5, 6, 7):
            data = buf[p + 16:]
            px = []
            for y in range(h):
                for x in range(w):
                    if depth == 16:
                        v = struct.unpack_from('<H', data, (y * w + x) * 2)[0]
                        r = (v >> 11) & 0x1F
                        g = (v >> 5) & 0x3F
                        b = v & 0x1F
                        px.append((r * 255 // 31, g * 255 // 63, b * 255 // 31, 255))
                    else:
                        v = struct.unpack_from('<I', data, (y * w + x) * 4)[0]
                        px.append((v & 0xFF, (v >> 8) & 0xFF, (v >> 16) & 0xFF, (v >> 24) & 0xFF))
            return w, h, px
    raise SystemExit("no encuentro el raster de la textura")


def main():
    buf = open(DFF, 'rb').read()
    frames, geoms, atomics = dg.parse(buf)
    byframe = {}
    for fi, gi in atomics:
        byframe.setdefault(fi, []).append(gi)
    wheel = None
    for fi, gis in byframe.items():
        if frames[fi]['name'] == 'steeringwheel':
            wheel = geoms[gis[0]]
    assert wheel is not None, "el taxi no tiene marco steeringwheel"
    verts, tris, uvs, norms = wheel['verts'], wheel['tris'], wheel['uvs'][0], wheel['norms']
    tw, th, tpx = textura(TXD, "stwheel")
    alphas = set(p[3] for p in tpx)
    print("malla: %d verts %d tris | textura %dx%d | alphas: %s" %
          (len(verts), len(tris), tw, th, sorted(alphas)[:6]))

    L = []
    L.append("// Generado por gta_vc_browser/tools/gen_steering_wheel.py -- no editar a mano.")
    L.append("//")
    L.append("// Malla y textura del volante 3D que el mod (Vice Extended) solo puso a 18 coches.")
    L.append("// De donde sale: streamed/models/gta3.img/taxi.dff (marco \"steeringwheel\") y su .txd.")
    L.append("// Los demas coches de GTA VC NO tienen geometria de volante (esta pintado en el")
    L.append("// salpicadero, que es un solo quad con la textura compartida \"vehicledash32\"), asi que")
    L.append("// para que gire hay que anadir esta rueda.")
    L.append("")
    L.append("#pragma once")
    L.append("")
    L.append("#define VICEEXT_WHEEL_VERTS %d" % len(verts))
    L.append("#define VICEEXT_WHEEL_TRIS %d" % len(tris))
    L.append("#define VICEEXT_WHEEL_TEX %d" % tw)
    L.append("")
    L.append("// posicion(3) + normal(3) + uv(2)")
    L.append("static const float gs_afSteeringWheel[%d][8] = {" % len(verts))
    for v, n, uv in zip(verts, norms, uvs):
        L.append("\t{ %.5ff, %.5ff, %.5ff,  %.4ff, %.4ff, %.4ff,  %.5ff, %.5ff }," %
                 (v[0], v[1], v[2], n[0], n[1], n[2], uv[0], uv[1]))
    L.append("};")
    L.append("")
    L.append("static const uint16 gs_auSteeringWheelTris[%d][3] = {" % len(tris))
    for t in tris:
        L.append("\t{ %d, %d, %d }," % (t[0], t[1], t[2]))
    L.append("};")
    L.append("")
    L.append("// textura %dx%d en RGB565 (el canal alpha de la original es 255 en todas partes)" % (tw, th))
    L.append("static const uint16 gs_auSteeringWheelTex[%d] = {" % (tw * th))
    for y in range(th):
        row = []
        for x in range(tw):
            r, g, b, a = tpx[y * tw + x]
            row.append("0x%04X" % (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)))
        L.append("\t" + ", ".join(row) + ",")
    L.append("};")
    L.append("")
    with open(OUT, "w", encoding="utf-8", newline="\r\n") as f:
        f.write("\n".join(L))
    print("escrito:", OUT)


if __name__ == '__main__':
    main()
