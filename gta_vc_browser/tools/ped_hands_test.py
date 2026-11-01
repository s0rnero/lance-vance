#!/usr/bin/env python3
"""Prueba de interpretacion: como se compone el esqueleto del ped.

Compara las posibles lecturas de los quaternions del .ifp (rotacion absoluta
o delta respecto al reposo, antes o despues) y de las traslaciones de reposo
(espacio del padre animado o del padre en reposo), usando un clip en el que
se sabe la respuesta: el ped de pie tiene la cabeza ~0,65 m ARRIBA de la raiz
y los pies a la altura de la raiz.

    python tools/ped_hands_test.py
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dff_geom as dg
import ifp_inspect
from ped_hands import quat_rows, mul_rows, xform, round4, cadena


def rest_rows(f):
    r = f['rot']
    return ((r[0], r[1], r[2]), (r[3], r[4], r[5]), (r[6], r[7], r[8]))


def claves(data, s):
    o = s['start']
    ident, cs = struct.unpack_from('<4sI', data, o)
    o += 8
    ident, as_ = struct.unpack_from('<4sI', data, o)
    o += 8
    o += round4(as_)
    ident, ks = struct.unpack_from('<4sI', data, o)
    o += 8
    if ident == b'KRTS':
        n = 11
    elif ident == b'KRT0':
        n = 8
    else:
        n = 5
    out = []
    for k in range(s['num_frames']):
        v = struct.unpack_from('<%df' % n, data, o + n * k * 4)
        out.append(((-v[0], -v[1], -v[2], v[3]), v[n - 1]))
    return out


def compose(frames, quats, bone, modo, transl):
    idx = [i for i, f in enumerate(frames) if f['name'] == bone][0]
    chain = cadena(frames, idx)
    world = {0: ([0.0, 0.0, 0.0], ((1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)))}
    restworld = {0: ((1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0))}
    for fi in reversed(chain[:-1]):
        f = frames[fi]
        pid = f['parent']
        ppos, prows = world[pid]
        rest = rest_rows(f)
        q = quats.get(f['name'])
        if q is None:
            rows = rest
        else:
            aq = quat_rows(*q)
            if modo == 'A':
                rows = aq
            elif modo == 'B':
                rows = mul_rows(rest, aq)
            else:
                rows = mul_rows(aq, rest)
        if transl == 'anim':
            basis = prows
        else:
            basis = restworld[pid]
        d = xform(basis, list(f['pos']))
        pos = [ppos[i] + d[i] for i in range(3)]
        world[fi] = (pos, mul_rows(rows, prows))
        restworld[fi] = mul_rows(rest, restworld[pid])
    return world[idx][0]


def main():
    raiz = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    model = os.path.join(raiz, 'streamed', 'models', 'gta3.img', 'hmybe.dff')
    ifp = os.path.join(raiz, 'streamed', 'anim', 'ped.ifp')
    frames, _, _ = dg.parse(open(model, 'rb').read())
    data, meta, anims = ifp_inspect.parse_ifp(ifp)
    for clip in ('WALK_player', 'CAR_sit'):
        a = [x for x in anims if x['name'] == clip]
        if not a:
            continue
        a = a[0]
        quats = {}
        for s in a['seqs']:
            if not s['num_frames']:
                continue
            ks = claves(data, s)
            quats[s['name']] = ks[0][0]
        for modo in ('A', 'B', 'C'):
            for transl in ('anim', 'rest'):
                out = []
                for bone in ('Head', 'L Foot', 'R Foot', 'L Hand', 'R Hand'):
                    p = compose(frames, quats, bone, modo, transl)
                    out.append('%s=(%+.2f,%+.2f,%+.2f)' % (bone.replace(' ', '')[:5], p[0], p[1], p[2]))
                print('%-12s %s/%-4s %s' % (clip, modo, transl, ' '.join(out)))


if __name__ == '__main__':
    main()
