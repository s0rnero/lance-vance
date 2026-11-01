#!/usr/bin/env python3
"""Rasteriza un DFF (solo geometria, color plano) en PNG para ver donde esta cada
material o cada atomico. Sin culling: los triangulos se pintan tal cual, de atras
a delante (painter's algorithm).

Uso:
    python tools/dff_material_view.py <fichero.dff> <atomic|*> <salida.png> [vista]
vista: front | rear | side | top | iso
Con `*` pinta cada atomico de un color y lista los nombres en la leyenda.
"""
import os
import sys
import importlib.util
from PIL import Image, ImageDraw

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('dffgeom', os.path.join(HERE, 'dff_geom.py'))
dffgeom = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dffgeom)

PALETTE = [
    (220, 80, 60), (240, 170, 40), (70, 170, 220), (200, 200, 200),
    (230, 60, 200), (40, 40, 40), (120, 220, 90), (140, 90, 220),
    (250, 230, 120), (90, 210, 200), (200, 120, 90), (150, 150, 240),
    (250, 120, 40), (60, 130, 60), (180, 60, 90), (100, 100, 30),
]


def main():
    path, want, out = sys.argv[1], sys.argv[2], sys.argv[3]
    view = sys.argv[4] if len(sys.argv) > 4 else 'iso'
    buf = open(path, 'rb').read()
    frames, geoms, atomics = dffgeom.parse(buf)
    allAtomics = want == '*'
    selected = []
    for fi, gi in atomics:
        if allAtomics or frames[fi]['name'] == want:
            selected.append((frames[fi]['name'], geoms[gi]))
    if not selected:
        raise SystemExit('atomic %s no encontrado' % want)
    verts, tris, colors, labels = [], [], [], []
    if allAtomics:
        for k, (name, g) in enumerate(selected):
            base = len(verts)
            verts.extend(g['verts'])
            col = PALETTE[k % len(PALETTE)]
            colors.append(col)
            labels.append(name)
            for t in g['tris']:
                tris.append((t[0] + base, t[1] + base, t[2] + base, col))
    else:
        g = selected[0][1]
        verts = list(g['verts'])
        for mi, (color, tex) in enumerate(g['mats']):
            colors.append(PALETTE[mi % len(PALETTE)])
            labels.append('mat %d %s' % (mi, tex or '(sin tex)'))
        for t in g['tris']:
            tris.append((t[0], t[1], t[2], PALETTE[t[3] % len(PALETTE)]))
    W = H = 900
    if view == 'front':
        proj = lambda v: (v[0], -v[2])
        eye = (0.0, 1.0, 0.0)
    elif view == 'rear':
        proj = lambda v: (-v[0], -v[2])
        eye = (0.0, -1.0, 0.0)
    elif view == 'side':
        proj = lambda v: (v[1], -v[2])
        eye = (1.0, 0.0, 0.0)
    elif view == 'top':
        proj = lambda v: (v[0], -v[1])
        eye = (0.0, 0.0, 1.0)
    else:
        proj = lambda v: (v[0] + 0.7 * v[1], -(v[2] + 0.35 * v[1]))
        eye = (0.6, 0.6, 0.5)
    pts = [proj(v) for v in verts]
    xs = [p[0] for p in pts]
    ys = [p[1] for p in pts]
    minx, maxx = min(xs), max(xs)
    miny, maxy = min(ys), max(ys)
    scale = min(W / max(1e-6, maxx - minx), H / max(1e-6, maxy - miny)) * 0.9
    ox = (W - (maxx - minx) * scale) / 2 - minx * scale
    oy = (H - (maxy - miny) * scale) / 2 - miny * scale
    img = Image.new('RGB', (W, H), (255, 255, 255))
    dr = ImageDraw.Draw(img)
    order = []
    for t in tris:
        a, b, c = verts[t[0]], verts[t[1]], verts[t[2]]
        depth = (a[0] + b[0] + c[0]) * eye[0] + (a[1] + b[1] + c[1]) * eye[1] + (a[2] + b[2] + c[2]) * eye[2]
        order.append((depth, t))
    order.sort(key=lambda d: d[0])
    for depth, t in order:
        poly = [(ox + pts[t[0]][0] * scale, oy + pts[t[0]][1] * scale),
                (ox + pts[t[1]][0] * scale, oy + pts[t[1]][1] * scale),
                (ox + pts[t[2]][0] * scale, oy + pts[t[2]][1] * scale)]
        dr.polygon(poly, fill=t[3], outline=(0, 0, 0))
    y = 8
    for i, label in enumerate(labels):
        dr.rectangle([8, y, 28, y + 14], fill=colors[i], outline=(0, 0, 0))
        dr.text((34, y), label, fill=(0, 0, 0))
        y += 18
    img.save(out)
    print('escrito %s (%dx%d) vista %s' % (out, W, H, view))


if __name__ == '__main__':
    main()
