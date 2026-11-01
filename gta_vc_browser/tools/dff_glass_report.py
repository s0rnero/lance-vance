#!/usr/bin/env python3
"""Lista, por DFF, los atomics con material de cristal (alfa != 255) y avisa de
los que son puerta pero NO tienen cristal.

Para que: el sintoma "se ve la calle en vez de la puerta" en los coches del mod
(Vice Extended) viene de que sus DFF adaptados no llevan el material de cristal
en las puertas: el hueco de la ventanilla queda abierto. En el retail (el DFF
original de PC) la puerta lleva `(106,106,106,76) taximiami86speca`.

Uso:
    python tools/dff_glass_report.py <fichero.dff | directorio> [--only-puertas]
"""
import os
import sys
import importlib.util

HERE = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('dffgeom', os.path.join(HERE, 'dff_geom.py'))
dffgeom = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dffgeom)


def analyse(path):
    with open(path, 'rb') as f:
        buf = f.read()
    frames, geoms, atomics = dffgeom.parse(buf)
    out = []
    for frame_idx, geo_idx in atomics:
        if geo_idx < 0 or geo_idx >= len(geoms):
            continue
        name = frames[frame_idx]['name'] if 0 <= frame_idx < len(frames) else '?'
        g = geoms[geo_idx]
        mats = g.get('mats', [])
        tris = g.get('tris', [])
        used = {}
        for t in tris:
            used[t[3]] = used.get(t[3], 0) + 1
        cristal = []
        for mi in sorted(used):
            if mi < len(mats) and mats[mi][0][3] != 0xFF:
                cristal.append('mat%d:a%d:%s:%dtris' % (mi, mats[mi][0][3], mats[mi][1], used[mi]))
        out.append((name, cristal, used.get(0, 0)))
    return out


def main():
    target = sys.argv[1]
    soloPuertas = '--only-puertas' in sys.argv
    paths = []
    if os.path.isdir(target):
        for root, _, files in os.walk(target):
            for n in sorted(files):
                if n.lower().endswith('.dff'):
                    paths.append(os.path.join(root, n))
    else:
        paths.append(target)

    for p in paths:
        try:
            atoms = analyse(p)
        except Exception as e:
            print('%-24s ERROR %s' % (os.path.basename(p), e))
            continue
        conCristal = [(n, c) for n, c, _ in atoms if c]
        sinCristal = [n for n, c, _ in atoms if not c and 'door' in n.lower()]
        if soloPuertas and not sinCristal:
            continue
        print('%-22s cristal: %s' % (os.path.basename(p),
              ' | '.join('%s{%s}' % (n, ','.join(c)) for n, c in conCristal) or '-'))
        if sinCristal:
            print('%-22s PUERTAS SIN CRISTAL: %s' % ('', ' '.join(sinCristal)))


if __name__ == '__main__':
    main()
