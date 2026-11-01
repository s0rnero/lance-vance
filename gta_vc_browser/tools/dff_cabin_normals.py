#!/usr/bin/env python3
"""Mide la orientacion de los triangulos del habitaculo de un DFF.

Para cada material del atomic pedido: bbox, triangulos cuyo centro cae dentro de
la caja del habitaculo, y si la normal (segun el orden de vertices) apunta hacia
el centro de la caja (cara interior) o hacia fuera (cara trasera desde dentro).

Uso:
    python tools/dff_cabin_normals.py <fichero.dff> <atomic> [y0 y1 z0 z1 x0 x1]
"""
import os
import sys
import importlib.util

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('dffgeom', os.path.join(HERE, 'dff_geom.py'))
dffgeom = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dffgeom)


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def main():
    path, want = sys.argv[1], sys.argv[2]
    if len(sys.argv) >= 8:
        y0, y1, z0, z1, x0, x1 = [float(v) for v in sys.argv[3:9]]
    else:
        y0, y1, z0, z1, x0, x1 = -0.80, 0.90, 0.55, 1.35, -0.85, 0.85
    buf = open(path, 'rb').read()
    frames, geoms, atomics = dffgeom.parse(buf)
    g = None
    for fi, gi in atomics:
        if frames[fi]['name'] == want:
            g = geoms[gi]
            break
    if g is None:
        raise SystemExit('atomic %s no encontrado' % want)
    center = ((x0 + x1) / 2, (y0 + y1) / 2, (z0 + z1) / 2)
    per = {}
    total_in = 0
    for t in g['tris']:
        a, b, c = g['verts'][t[0]], g['verts'][t[1]], g['verts'][t[2]]
        cen = ((a[0] + b[0] + c[0]) / 3, (a[1] + b[1] + c[1]) / 3, (a[2] + b[2] + c[2]) / 3)
        if not (x0 <= cen[0] <= x1 and y0 <= cen[1] <= y1 and z0 <= cen[2] <= z1):
            continue
        n = cross(sub(b, a), sub(c, a))
        ln = (n[0] ** 2 + n[1] ** 2 + n[2] ** 2) ** 0.5
        if ln < 1e-9:
            continue
        n = (n[0] / ln, n[1] / ln, n[2] / ln)
        d = dot(n, sub(center, cen))
        # d > 0: la normal apunta al centro del habitaculo -> se ve desde dentro
        per.setdefault(t[3], [0, 0, 0])
        per[t[3]][1 if d > 0 else 2] += 1
        per[t[3]][0] += 1
        total_in += 1
    print('%s %s: %d tris en la caja x %.2f..%.2f y %.2f..%.2f z %.2f..%.2f' % (
        os.path.basename(path), want, total_in, x0, x1, y0, y1, z0, z1))
    for mi in sorted(per):
        color, tex = g['mats'][mi]
        n, inward, outward = per[mi]
        print('  mat %2d tris=%-4d visibles_desde_dentro=%-4d traseras_desde_dentro=%-4d color=%s tex=%s' % (
            mi, n, inward, outward, color, tex))


if __name__ == '__main__':
    main()
