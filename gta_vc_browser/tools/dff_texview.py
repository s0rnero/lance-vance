#!/usr/bin/env python3
"""Vista texturizada de un DFF: proyecta desde el asiento del conductor y pinta
cada triangulo con la textura que le asigna su material, muestreando las UV del
fichero. Sirve para separar "el dato del DFF/TXD esta mal" de "el motor lo
dibuja mal": si aqui la cabina sale bien mapeada, el fichero esta sano.

Uso:
    python tools/dff_texview.py <modelo.dff> <modelo.txd> <salida.png> [atomic]
    (atomic por defecto: todos)
"""
import importlib.util
import math
import os
import struct
import sys

from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))


def load(name, path, argv=None):
    spec = importlib.util.spec_from_file_location(name, os.path.join(HERE, path))
    mod = importlib.util.module_from_spec(spec)
    if argv is not None:
        old, sys.argv = sys.argv, argv
        try:
            spec.loader.exec_module(mod)
        finally:
            sys.argv = old
    else:
        spec.loader.exec_module(mod)
    return mod


g = load('dffgeom', 'dff_geom.py')
t = load('txdraster', 'txd_raster.py', ['txd_raster.py'])


def textures(path):
    data = open(path, 'rb').read()
    out = {}
    for pos in t.dict_textures(data):
        tex = t.read_native(data, pos)
        if not tex or not tex['levels']:
            continue
        size, off = tex['levels'][0]
        px = t.decode(data[off:off + size], tex['w'], tex['h'], tex['depth'],
                      tex['fmt'], tex['has_alpha'], tex['compression'])
        if px:
            out[tex['nombre'].lower()] = (tex['w'], tex['h'], px)
    return out


def sample(tex, u, v):
    w, h, px = tex
    u -= math.floor(u)
    v -= math.floor(v)
    x = min(w - 1, max(0, int(u * w)))
    y = min(h - 1, max(0, int(v * h)))
    return px[y * w + x][:3]


def main():
    path, txdpath, out = sys.argv[1], sys.argv[2], sys.argv[3]
    only = sys.argv[4] if len(sys.argv) > 4 and not sys.argv[4].startswith('-') else None
    texs = textures(txdpath)
    buf = open(path, 'rb').read()
    frames, geoms, atomics = g.parse(buf)

    seat = None
    for f in frames:
        if f['name'] == 'ped_frontseat':
            seat = g.world_of(frames, f['i'])
    if seat is None:
        raise SystemExit('sin ped_frontseat')

    eye = g.mul(seat, [0.0, 0.0, 0.0])
    eye = [eye[0], eye[1], eye[2] + 0.52]
    fwd = (0.0, 1.0, 0.0)
    up = (0.0, 0.0, 1.0)
    if '--eye' in sys.argv:
        eye = [float(v) for v in sys.argv[sys.argv.index('--eye') + 1].split(',')]
        fwd = [float(v) for v in sys.argv[sys.argv.index('--look') + 1].split(',')]
        l = math.sqrt(sum(c * c for c in fwd)) or 1.0
        fwd = tuple(c / l for c in fwd)

    tris = []
    for fi, gi in atomics:
        name = frames[fi]['name']
        if only and name != only:
            continue
        geo = geoms[gi]
        if not geo['verts'] or not geo['uvs']:
            continue
        m = g.world_of(frames, fi)
        wv = [g.mul(m, v) for v in geo['verts']]
        uvs = geo['uvs'][0]
        for v0, v1, v2, mi in geo['tris']:
            color, texname = geo['mats'][mi]
            tex = texs.get((texname or '').lower())
            tris.append((wv[v0], wv[v1], wv[v2],
                         uvs[v0], uvs[v1], uvs[v2], tex, color))

    W, H = 900, 540
    right = (fwd[1] * up[2] - fwd[2] * up[1],
             fwd[2] * up[0] - fwd[0] * up[2],
             fwd[0] * up[1] - fwd[1] * up[0])
    fov = math.radians(70.0)
    if '--fov' in sys.argv:
        fov = math.radians(float(sys.argv[sys.argv.index('--fov') + 1]))
    scale = (H / 2) / math.tan(fov / 2)
    img = Image.new('RGB', (W, H), (0, 0, 0))
    dr = img.load()

    def proj(p):
        d = (p[0] - eye[0], p[1] - eye[1], p[2] - eye[2])
        z = d[0] * fwd[0] + d[1] * fwd[1] + d[2] * fwd[2]
        if z < 0.02:
            return None
        x = d[0] * right[0] + d[1] * right[1] + d[2] * right[2]
        y = d[0] * up[0] + d[1] * up[1] + d[2] * up[2]
        return (W / 2 + x * scale / z, H / 2 - y * scale / z, z)

    shade = []
    for tri in tris:
        a, b, c = proj(tri[0]), proj(tri[1]), proj(tri[2])
        if a is None or b is None or c is None:
            continue
        shade.append((a[2] + b[2] + c[2], a, b, c, tri))

    shade.sort(key=lambda s: s[0], reverse=True)   # de atras a delante

    for _, a, b, c, tri in shade:
        tex = tri[6]
        col = tri[7]
        base = (col[0], col[1], col[2]) if col else (255, 255, 255)
        xs = [a[0], b[0], c[0]]
        ys = [a[1], b[1], c[1]]
        x0, x1 = max(0, int(min(xs))), min(W - 1, int(max(xs)) + 1)
        y0, y1 = max(0, int(min(ys))), min(H - 1, int(max(ys)) + 1)
        if x1 < x0 or y1 < y0:
            continue
        den = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
        if abs(den) < 1e-9:
            continue
        ua, ub, uc = tri[3], tri[4], tri[5]
        for py in range(y0, y1 + 1):
            for px in range(x0, x1 + 1):
                w0 = ((b[1] - c[1]) * (px - c[0]) + (c[0] - b[0]) * (py - c[1])) / den
                w1 = ((c[1] - a[1]) * (px - c[0]) + (a[0] - c[0]) * (py - c[1])) / den
                w2 = 1.0 - w0 - w1
                if w0 < 0 or w1 < 0 or w2 < 0:
                    continue
                if tex is None:
                    r, gg, bl = (base[0] * 3 // 4, base[1] * 3 // 4, base[2] * 3 // 4)
                else:
                    u = w0 * ua[0] + w1 * ub[0] + w2 * uc[0]
                    v = w0 * ua[1] + w1 * ub[1] + w2 * uc[1]
                    r, gg, bl = sample(tex, u, v)
                    r = r * base[0] // 255
                    gg = gg * base[1] // 255
                    bl = bl * base[2] // 255
                dr[px, py] = (r, gg, bl)

    img.save(out)
    print('escrito %s  (%d triangulos, ojo=(%.2f,%.2f,%.2f))'
          % (out, len(shade), eye[0], eye[1], eye[2]))


if __name__ == '__main__':
    main()
