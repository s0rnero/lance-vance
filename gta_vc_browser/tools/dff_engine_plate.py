#!/usr/bin/env python3
"""Quita la plancha plana de motor del PC que tapa el motor Xbox trasplantado.

El trasplante (planes 61.0/63.0) mete el motor 3D del donante en `od_engine`,
pero en algunos coches queda en `chassis_hi` la chapa plana con la textura de
motor del PC (`enginetop*`): caras horizontales justo encima del motor nuevo,
que en pantalla se ven como una plancha gris sobre el vano. El lovefist se
limpio asi en 63.0/T3 (5 caras); el washington conserva tres caras
`enginetopb4bit64` de 0,97 m2 sobre el motor `limo` y es lo que el jugador ve
como «la capa de motor encima del motor transplantado».

La herramienta retira solo esas caras horizontales de `enginetop*` dentro del
vano, y solo si el coche trae un motor de donante en otra pieza (si no, quitar
la plancha dejaria el vano vacio). Despues hay que reconstruir la malla 0x50E
con `tools/dff_mesh_rebuild.py <coche> --install`, porque el motor dibuja la
malla y no la lista de triangulos.

Uso:
    python tools/dff_engine_plate.py washing --report
    python tools/dff_engine_plate.py washing --install
"""
import argparse
import shutil
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
import dff_geom as dg
import xbox_transplant as xt

GB = HERE.parent
SERV = GB / 'streamed' / 'models' / 'gta3.img'
TREES = [SERV, GB / 'gamefiles' / 'models' / 'gta3.img',
         GB / 'bootseed' / 'models' / 'gta3.img']
BACKUP = GB / 'tmp' / 'base92' / 'pre_plate'
FLAT_NZ = 0.85
MIN_AREA = 0.02
BAY_Y = (1.2, 2.9)
BAY_Z = (-0.1, 0.5)
DONOR = ('limo', 'v8_motor', 'v12', 'motor', 'csstretchengine', 'cswaltengine')


def geom_parts(g):
    """(mats, verts, tris, uvs, chunk, buffer) de una geometria de xt."""
    buf = xt.ch(0x0F, g.ver, g.body)
    ch = next(dg.children(buf, 0, len(buf)))
    mats, verts, tris, uvs, norms, flags, colors = dg.parse_geometry(buf, ch)
    return mats, verts, tris, uvs, ch, buf


def horizontal(p):
    a = [p[1][k] - p[0][k] for k in range(3)]
    b = [p[2][k] - p[0][k] for k in range(3)]
    n = (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
         a[0] * b[1] - a[1] * b[0])
    ln = (n[0] ** 2 + n[1] ** 2 + n[2] ** 2) ** 0.5
    return 0.0 if ln < 1e-9 else abs(n[2]) / ln


def area(p):
    a = [p[1][k] - p[0][k] for k in range(3)]
    b = [p[2][k] - p[0][k] for k in range(3)]
    n = (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
         a[0] * b[1] - a[1] * b[0])
    return (n[0] ** 2 + n[1] ** 2 + n[2] ** 2) ** 0.5 / 2


def frames_of(d):
    frames = xt._frame_list(d)
    for f in frames:
        f['name'] = d['names'][f['i']] if f['i'] < len(d['names']) else ''
    return frames


def survey(d):
    """Caras de plancha a quitar, texturas de donante y rayos desde arriba."""
    frames = frames_of(d)
    geoms = [geom_parts(g) for g in d['geoms']]
    donor = set()
    for mats, verts, tris, uvs, ch, buf in geoms:
        for c, tex in mats:
            t = (tex or '').lower()
            if t in DONOR or t.endswith('motor') or t.startswith('v8'):
                donor.add(t)
    cut = {}
    for atom in d['atoms']:
        fi, gi = atom['frame'], atom['geom']
        if gi >= len(geoms):
            continue
        mats, verts, tris, uvs, ch, buf = geoms[gi]
        m = dg.world_of(frames, fi)
        pts = [dg.mul(m, v) for v in verts]
        for ti, t in enumerate(tris):
            tex = (mats[t[3]][1] or '').lower() if t[3] < len(mats) else ''
            if not tex.startswith('enginetop'):
                continue
            p = [pts[t[k]] for k in range(3)]
            if horizontal(p) < FLAT_NZ or area(p) < MIN_AREA:
                continue
            y = sum(q[1] for q in p) / 3
            z = sum(q[2] for q in p) / 3
            if BAY_Y[0] < y < BAY_Y[1] and BAY_Z[0] < z < BAY_Z[1]:
                cut.setdefault(gi, set()).add(ti)
    return geoms, frames, cut, donor


def ray_survey(geoms, frames, d, cut, n=11, m=9):
    """Textura de la superficie mas alta del vano vista desde arriba."""
    tops = {}
    for atom in d['atoms']:
        fi, gi = atom['frame'], atom['geom']
        if gi >= len(geoms) or frames[fi]['name'].lower() == 'chassis_vlo':
            continue
        mats, verts, tris, uvs, ch, buf = geoms[gi]
        mm = dg.world_of(frames, fi)
        pts = [dg.mul(mm, v) for v in verts]
        for ti, t in enumerate(tris):
            if ti in cut.get(gi, set()):
                continue
            tex = (mats[t[3]][1] or '?') if t[3] < len(mats) else '?'
            tops.setdefault(tex, []).append([pts[t[k]] for k in range(3)])
    out = {}
    for i in range(n):
        for j in range(m):
            x = -0.6 + 1.2 * i / (n - 1)
            y = BAY_Y[0] + 0.2 + (BAY_Y[1] - BAY_Y[0] - 0.4) * j / (m - 1)
            best = None
            for tex, faces in tops.items():
                for a, b, c in faces:
                    det = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
                    if abs(det) < 1e-9:
                        continue
                    l1 = ((b[1] - c[1]) * (x - c[0]) + (c[0] - b[0]) * (y - c[1])) / det
                    l2 = ((c[1] - a[1]) * (x - c[0]) + (a[0] - c[0]) * (y - c[1])) / det
                    l3 = 1 - l1 - l2
                    if min(l1, l2, l3) < -0.01:
                        continue
                    z = l1 * a[2] + l2 * b[2] + l3 * c[2]
                    if best is None or z > best[0]:
                        best = (z, tex)
            if best:
                out[best[1]] = out.get(best[1], 0) + 1
    return out


def fix(model, install):
    path = SERV / (model + '.dff')
    if not path.exists():
        print('%s: no existe' % model)
        return 1
    d = xt.parse_dff(str(path))
    geoms, frames, cut, donor = survey(d)
    if not donor:
        print('%s: sin motor de donante, no se toca' % model)
        return 0
    if not cut:
        print('%s: sin plancha de motor' % model)
        return 0
    total = sum(len(v) for v in cut.values())
    print('%s: motor %s — %d caras planas de enginetop (%s)'
          % (model, ','.join(sorted(donor)), total,
             ', '.join('geom %d: %d' % (gi, len(v)) for gi, v in sorted(cut.items()))))
    before = ray_survey(geoms, frames, d, {})
    for gi, drop in sorted(cut.items()):
        g = d['geoms'][gi]
        mats, verts, tris, uvs, ch, buf = geoms[gi]
        keep = [t for ti, t in enumerate(tris) if ti not in drop]
        ver, st = dg.split_triangles(buf, ch, keep)
        body = b''
        for c in xt.chunks(g.body, 0, len(g.body)):
            body += xt.ch(0x01, ver, st) if c.typ == 0x01 else xt.ch(c.typ, c.ver, c.body)
        d['geoms'][gi] = xt.Chunk(0x0F, g.ver, body, 0, 0)
    out = xt.rebuild(d)
    BACKUP.mkdir(parents=True, exist_ok=True)
    if not (BACKUP / (model + '.dff')).exists():
        shutil.copy2(path, BACKUP / (model + '.dff'))
    tmp = BACKUP / (model + '_nuevo.dff')
    tmp.write_bytes(out)
    nd = xt.parse_dff(str(tmp))
    ngeoms, nframes, ncut, _ = survey(nd)
    after = ray_survey(ngeoms, nframes, nd, {})
    print('   vano visto desde arriba, antes:   %s' % dict(sorted(before.items())))
    print('   vano visto desde arriba, despues: %s' % dict(sorted(after.items())))
    print('   bytes %d -> %d, planchas restantes %d'
          % (len(d['buf']), len(out), sum(len(v) for v in ncut.values())))
    if install:
        for tree in TREES:
            dst = tree / (model + '.dff')
            if dst.parent.exists():
                dst.write_bytes(out)
        print('   instalado en %d arboles' % sum(1 for t in TREES if t.parent.exists()))
    else:
        print('   (sin instalar: --install)')
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('models', nargs='*')
    ap.add_argument('--install', action='store_true')
    ap.add_argument('--report', action='store_true',
                    help='solo mide e informa (es lo que hace por defecto)')
    a = ap.parse_args()
    rc = 0
    for m in (a.models or ['washing']):
        rc |= fix(m, a.install)
    return rc


if __name__ == '__main__':
    sys.exit(main())
