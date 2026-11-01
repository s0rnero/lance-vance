#!/usr/bin/env python3
"""Orden de atomics de un DFF y alfa minima por atomic.

Para que: el pase transparente de los vehiculos ordena los atomics por distancia
del clump y, si empatan, en el orden del clump. Este informe dice en que orden
aparecen los atomics y cuales llevan material con alfa != 255 (los que van al
pase transparente).

Uso:
    python tools/dff_atomic_order.py <fichero.dff> [...]
"""
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
import importlib.util

spec = importlib.util.spec_from_file_location('dffgeom', os.path.join(os.path.dirname(__file__), 'dff_geom.py'))
dffgeom = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dffgeom)


def fmt_color(c):
    if c is None:
        return '-'
    if isinstance(c, (tuple, list)):
        return '(%d,%d,%d,%d)' % tuple(c)
    return str(c)


def probe(path):
    buf = open(path, 'rb').read()
    frames, geoms, atomics = dffgeom.parse(buf)
    print('%s: %d marcos, %d atomics, %d geometrias' % (path, len(frames), len(atomics), len(geoms)))
    for order, (fi, gi) in enumerate(atomics):
        name = frames[fi]['name'] if fi < len(frames) else '?%d' % fi
        if gi >= len(geoms):
            print('  %2d frame=%-22s geo=%d (sin geometria)' % (order, name, gi))
            continue
        g = geoms[gi]
        counts = {}
        for t in g['tris']:
            counts[t[3]] = counts.get(t[3], 0) + 1
        con_alfa = []
        for mi, (color, tex) in enumerate(g['mats']):
            if color[3] != 255:
                con_alfa.append('%d:%s' % (mi, fmt_color(color)))
        print('  %2d frame=%-22s geo=%-3d verts=%-5d tris=%-5d mats=%-2d alfa=[%s]' % (
            order, name, gi, len(g['verts']), len(g['tris']), len(g['mats']), ' '.join(con_alfa)))
        for mi, (color, tex) in enumerate(g['mats']):
            print('       mat %2d tris=%-4d color=%-18s tex=%s' % (
                mi, counts.get(mi, 0), fmt_color(color), tex))


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    for path in sys.argv[1:]:
        try:
            probe(path)
        except Exception as e:
            print('%s: ERROR %s' % (path, e))
    return 0


if __name__ == '__main__':
    sys.exit(main())
