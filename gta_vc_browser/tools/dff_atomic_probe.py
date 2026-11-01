#!/usr/bin/env python3
"""Medida de un DFF de vehiculo: por atomic, bbox y reparto de triangulos por material.

Para que: para averiguar que atomic es el "plano suelto" que se ve flotando y
que materiales (cristal / chapa) tiene cada malla de la carrocería.

Uso:
    python tools/dff_atomic_probe.py <fichero.dff> [nombre_substring ...]
"""
import sys
import os

sys.path.insert(0, os.path.dirname(__file__))
import importlib.util

spec = importlib.util.spec_from_file_location('dffgeom', os.path.join(os.path.dirname(__file__), 'dff_geom.py'))
dffgeom = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dffgeom)


def bbox(verts):
    xs = [v[0] for v in verts]
    ys = [v[1] for v in verts]
    zs = [v[2] for v in verts]
    return (min(xs), max(xs), min(ys), max(ys), min(zs), max(zs))


def main():
    path = sys.argv[1]
    filters = [a.lower() for a in sys.argv[2:]]
    buf = open(path, 'rb').read()
    frames, geoms, atomics = dffgeom.parse(buf)
    for entry in atomics:
        name = entry['name'] if isinstance(entry, dict) and 'name' in entry else None
        print(entry.keys() if isinstance(entry, dict) else type(entry))
        break
    for gi, g in enumerate(geoms):
        mats = g.get('mats', [])
        verts = g.get('verts', [])
        tris = g.get('tris', [])
        counts = {}
        for t in tris:
            counts[t[3]] = counts.get(t[3], 0) + 1
        bb = bbox(verts) if verts else None
        print('geo %d mats=%d verts=%d tris=%d bbox=(x %.2f..%.2f y %.2f..%.2f z %.2f..%.2f)'
              % (gi, len(mats), len(verts), len(tris), bb[0], bb[1], bb[2], bb[3], bb[4], bb[5]))
        for mi, m in enumerate(mats):
            extra = ''
            if mi in counts:
                used = set()
                for t in tris:
                    if t[3] == mi:
                        used.update((t[0], t[1], t[2]))
                sub = [verts[i] for i in sorted(used) if i < len(verts)]
                if sub:
                    sb = bbox(sub)
                    extra = ' bbox=(x %.2f..%.2f y %.2f..%.2f z %.2f..%.2f)' % (
                        sb[0], sb[1], sb[2], sb[3], sb[4], sb[5])
            print('   mat %2d tris=%-4d color=%s tex=%s%s'
                  % (mi, counts.get(mi, 0), m[0], m[1], extra))


if __name__ == '__main__':
    main()
