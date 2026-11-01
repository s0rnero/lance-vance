#!/usr/bin/env python3
"""Mide que atomico/material de un DFF forma el habitaculo: triangulos dentro de
la caja de la cabina, color, textura y hacia donde apuntan sus normales.

Uso:
    python tools/dff_interior_probe.py <fichero.dff> <atomic|*>

Con `*` recorre todos los atomicos y solo lista los materiales que tienen la
mayoria de sus triangulos dentro de la caja de la cabina.
"""
import os
import sys
import importlib.util

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('dffgeom', os.path.join(HERE, 'dff_geom.py'))
dffgeom = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dffgeom)

CABIN = (-1.00, 1.00, -1.90, 1.60, 0.20, 0.88)


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def norm(a):
    l = (a[0] * a[0] + a[1] * a[1] + a[2] * a[2]) ** 0.5
    return (a[0] / l, a[1] / l, a[2] / l) if l > 1e-9 else (0.0, 0.0, 0.0)


def in_cabin(p):
    x0, x1 = min(v[0] for v in p), max(v[0] for v in p)
    y0, y1 = min(v[1] for v in p), max(v[1] for v in p)
    z0, z1 = min(v[2] for v in p), max(v[2] for v in p)
    return (CABIN[0] <= x0 and x1 <= CABIN[1] and CABIN[2] <= y0 and y1 <= CABIN[3]
            and CABIN[4] <= z0 and z1 <= CABIN[5])


def report(verts, tris, mats, center, onlyCabin):
    used = {}
    for t in tris:
        used[t[3]] = used.get(t[3], 0) + 1
    lines = []
    for mi in sorted(used):
        ntris = inward = outward = incab = 0
        area = 0.0
        zmin, zmax = 9.9, -9.9
        for t in tris:
            if t[3] != mi:
                continue
            p = [verts[t[0]], verts[t[1]], verts[t[2]]]
            cen = ((p[0][0] + p[1][0] + p[2][0]) / 3,
                   (p[0][1] + p[1][1] + p[2][1]) / 3,
                   (p[0][2] + p[1][2] + p[2][2]) / 3)
            cr = cross(sub(p[1], p[0]), sub(p[2], p[0]))
            n = norm(cr)
            area += 0.5 * (cr[0] ** 2 + cr[1] ** 2 + cr[2] ** 2) ** 0.5
            ntris += 1
            if dot(n, sub(center, cen)) > 0:
                inward += 1
            else:
                outward += 1
            if in_cabin(p):
                incab += 1
            zmin = min(zmin, min(v[2] for v in p))
            zmax = max(zmax, max(v[2] for v in p))
        if onlyCabin and incab * 2 < ntris:
            continue
        color, tex = mats[mi]
        lines.append('   mat %2d tris=%3d area=%.3f dentro=%3d fuera=%3d  en_cabina=%3d/%d  z %.2f..%.2f  color=%s tex=%s'
                     % (mi, ntris, area, inward, outward, incab, ntris, zmin, zmax, color, tex))
    return lines


def main():
    path = sys.argv[1]
    want = sys.argv[2] if len(sys.argv) > 2 else 'chassis_hi'
    buf = open(path, 'rb').read()
    frames, geoms, atomics = dffgeom.parse(buf)
    print('%s' % path)
    allAtomics = want == '*'
    for fi, gi in atomics:
        name = frames[fi]['name']
        if not allAtomics and name != want:
            continue
        g = geoms[gi]
        verts = g['verts']
        if not verts:
            continue
        center = ((min(v[0] for v in verts) + max(v[0] for v in verts)) / 2,
                  (min(v[1] for v in verts) + max(v[1] for v in verts)) / 2,
                  max(v[2] for v in verts) - 0.05)
        lines = report(verts, g['tris'], g['mats'], center, allAtomics)
        if lines:
            print('  atomic %s (geo %d, %d tris)' % (name, gi, len(g['tris'])))
            for l in lines:
                print(l)


if __name__ == '__main__':
    main()
