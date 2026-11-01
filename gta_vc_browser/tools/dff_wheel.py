#!/usr/bin/env python3
"""Localiza el volante dentro de la malla de un coche.

El mod (Vice Extended) trae 18 coches con el volante ya separado: un marco
"steeringwheel" con un atomic propio (textura "stwheel", 238 verts / 94 tris).
En los demas el volante esta fundido en el atomic del salpicadero, y hay que
sacarlo de ahi.

Este script lista los componentes conexos de la malla (los triangulos que
comparten vertice) y puntua cada uno como "anillo": la rueda es un anillo de
~90-100 triangulos con el centro vacio y un radio de ~0,2 m.

Con --scan recorre los coches del .ide y dice, para cada uno, si el motor le
va a anadir el volante 3D (src/vehicles/SteeringWheel.cpp) y con que posicion.
Es la misma regla que aplica el motor: el asiento de delante (el motor lo
mueve al lado del conductor con -x, ver src/peds/Ped.cpp) mas los
desplazamientos medidos en los 18 coches del mod.

Con --list da la lista definitiva: que coche tiene el volante que gira (los 18
del mod y los que el motor se lo anade) y cual se queda sin ?l, con el motivo.

Uso:
    python tools/dff_wheel.py <coche.dff>              # lista componentes
    python tools/dff_wheel.py <coche.dff> --stwheel    # el del mod (verdad de campo)
    python tools/dff_wheel.py --scan [modelos_dir]     # tabla de los coches
    python tools/dff_wheel.py --list [modelos_dir]     # lista de quien gira y quien no
"""
import os
import sys
from collections import defaultdict

sys.path.insert(0, __file__.rsplit('\\', 1)[0].rsplit('/', 1)[0])
import dff_geom as dg

WHEEL_DX = 0.008
WHEEL_DY = 0.327
WHEEL_DZ = 0.360
WHEEL_MIN_SEAT = 0.25

HANDLING_IS_BIKE = 0x10000
HANDLING_IS_HELI = 0x20000
HANDLING_IS_PLANE = 0x40000
HANDLING_IS_BOAT = 0x80000


def componentes(tris):
    """Componentes conexas de los triangulos (comparten indice de vertice)."""
    parent = {}

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a

    for t in tris:
        for v in t[:3]:
            parent.setdefault(v, v)
    for t in tris:
        a, b, c = find(t[0]), find(t[1]), find(t[2])
        if a != b:
            parent[b] = a
        b = find(t[1])
        if b != c:
            parent[c] = b
    groups = defaultdict(list)
    for i, t in enumerate(tris):
        groups[find(t[0])].append(i)
    return list(groups.values())


def stats(verts, tri_idx, tris):
    pts = []
    for i in tri_idx:
        for j in range(3):
            pts.append(verts[tris[i][j]])
    if not pts:
        return None
    lo = [min(p[i] for p in pts) for i in range(3)]
    hi = [max(p[i] for p in pts) for i in range(3)]
    size = [hi[i] - lo[i] for i in range(3)]
    c = [(lo[i] + hi[i]) * 0.5 for i in range(3)]
    rmax = 0.0
    for p in pts:
        d = sum((p[i] - c[i]) ** 2 for i in range(3)) ** 0.5
        rmax = max(rmax, d)
    edge = 0
    for i in tri_idx:
        a, b, cc = verts[tris[i][0]], verts[tris[i][1]], verts[tris[i][2]]
        cx = (a[0] + b[0] + cc[0]) / 3.0
        cy = (a[1] + b[1] + cc[1]) / 3.0
        cz = (a[2] + b[2] + cc[2]) / 3.0
        d = sum((cx - c[k]) ** 2 + (cy - c[k]) ** 2 for k in range(0)) or \
            ((cx - c[0]) ** 2 + (cy - c[1]) ** 2 + (cz - c[2]) ** 2) ** 0.5
        if d > rmax * 0.55:
            edge += 1
    return {'n': len(tri_idx), 'lo': lo, 'hi': hi, 'size': size, 'c': c,
            'rmax': rmax, 'ring': edge / max(1, len(tri_idx)), 'tri': tri_idx}


def _frames_index(frames, name):
    for i, f in enumerate(frames):
        if f['name'].lower() == name:
            return i
    return -1


def _world_pos(frames, idx):
    """Posicion de un marco en el espacio de la raiz, como la que calcula el
    motor para m_positions (el marco y luego cada padre hasta la raiz)."""
    p = list(frames[idx]['pos'])
    j = frames[idx]['parent']
    while 0 <= j < len(frames):
        f = frames[j]
        r = f['rot']
        x, y, z = p
        p = [r[0] * x + r[1] * y + r[2] * z + f['pos'][0],
             r[3] * x + r[4] * y + r[5] * z + f['pos'][1],
             r[6] * x + r[7] * y + r[8] * z + f['pos'][2]]
        j = f['parent']
    return p


def _ide_vehicles(ide, tipo='car'):
    """(modelo, handling) de los vehiculos de un tipo del .ide."""
    out = []
    for line in open(ide, errors='replace'):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        p = [x.strip() for x in line.split(',')]
        if len(p) < 5 or p[3] != tipo:
            continue
        out.append((p[1], p[4]))
    return out


def _handling_flags(cfg):
    """Flags de cada handling del handling.cfg (el campo 31, en hexadecimal)."""
    out = {}
    for raw in open(cfg, errors='replace'):
        l = raw.strip()
        if not l or l.startswith(';') or l.startswith('$'):
            continue
        partes = l.split()
        if len(partes) < 31:
            continue
        try:
            out[partes[0].upper()] = int(partes[30], 16)
        except ValueError:
            pass
    return out


def scan(modelos, ide, cfg):
    flags = _handling_flags(cfg)
    print('%-12s %-14s %-24s %-24s %-24s %s' %
          ('modelo', 'estado', 'asiento', 'volante (motor)', 'volante (mod)', 'error'))
    cuentas = {}
    for modelo, handling in _ide_vehicles(ide):
        path = os.path.join(modelos, modelo + '.dff')
        if not os.path.exists(path):
            print('%-12s %-14s (sin dff)' % (modelo, 'sin-dff'))
            continue
        buf = open(path, 'rb').read()
        frames, geoms, atomics = dg.parse(buf)
        i_seat = _frames_index(frames, 'ped_frontseat')
        i_wheel = _frames_index(frames, 'steeringwheel')
        asiento = _world_pos(frames, i_seat) if i_seat >= 0 else None
        real = _world_pos(frames, i_wheel) if i_wheel >= 0 else None
        fl = flags.get(handling.upper(), 0)
        if i_wheel >= 0:
            estado = 'ya-lo-trae'
        elif fl & (HANDLING_IS_HELI | HANDLING_IS_PLANE | HANDLING_IS_BIKE | HANDLING_IS_BOAT):
            estado = 'no-es-coche'
        elif asiento is None or abs(asiento[0]) < WHEEL_MIN_SEAT:
            estado = 'sin-asiento'
        else:
            estado = 'anadida'
        cuentas[estado] = cuentas.get(estado, 0) + 1
        pos = None
        if asiento is not None and abs(asiento[0]) >= WHEEL_MIN_SEAT:
            pos = (-asiento[0] + WHEEL_DX, asiento[1] + WHEEL_DY, asiento[2] + WHEEL_DZ)
        err = ''
        if pos is not None and real is not None:
            err = '%.3f,%.3f,%.3f' % tuple(pos[k] - real[k] for k in range(3))
        print('%-12s %-14s %-24s %-24s %-24s %s' % (
            modelo, estado,
            '%.3f,%.3f,%.3f' % tuple(asiento) if asiento else '-',
            '%.3f,%.3f,%.3f' % tuple(pos) if pos else '-',
            '%.3f,%.3f,%.3f' % tuple(real) if real else '-',
            err))
    print()
    print('resumen: %s' % ', '.join('%s=%d' % (k, v) for k, v in sorted(cuentas.items())))


def lista(modelos, ide, cfg):
    """Lista definitiva de que coche tiene volante que gira y cual no."""
    flags = _handling_flags(cfg)
    del_mod, anadido, sin_coche, sin_asiento = [], [], [], []
    for modelo, handling in _ide_vehicles(ide):
        path = os.path.join(modelos, modelo + '.dff')
        if not os.path.exists(path):
            sin_coche.append((modelo, 'sin DFF servido'))
            continue
        buf = open(path, 'rb').read()
        frames, geoms, atomics = dg.parse(buf)
        i_seat = _frames_index(frames, 'ped_frontseat')
        i_wheel = _frames_index(frames, 'steeringwheel')
        asiento = _world_pos(frames, i_seat) if i_seat >= 0 else None
        fl = flags.get(handling.upper(), 0)
        if i_wheel >= 0:
            del_mod.append(modelo)
        elif fl & (HANDLING_IS_HELI | HANDLING_IS_PLANE):
            if modelo.startswith('rc'):
                sin_coche.append((modelo, 'RC: su handling lleva el flag de helicoptero'))
            else:
                sin_coche.append((modelo, 'helicoptero o avion (handling)'))
        elif fl & (HANDLING_IS_BIKE | HANDLING_IS_BOAT):
            sin_coche.append((modelo, 'moto o barco (handling)'))
        elif asiento is None or abs(asiento[0]) < WHEEL_MIN_SEAT:
            sin_asiento.append((modelo, 'el asiento esta en el origen (RC)'))
        else:
            anadido.append(modelo)
    otros = []
    for tipo in ('bike', 'boat', 'train', 'heli', 'plane'):
        nombres = sorted(m for m, _ in _ide_vehicles(ide, tipo))
        if nombres:
            otros.append((tipo, nombres))
    print('CON VOLANTE QUE GIRA: %d' % (len(del_mod) + len(anadido)))
    print('  del mod (el DFF ya trae el marco "steeringwheel"): %d' % len(del_mod))
    print('    ' + ', '.join(sorted(del_mod)))
    print('  anadido por el motor (el DFF no lo trae): %d' % len(anadido))
    print('    ' + ', '.join(sorted(anadido)))
    print()
    print('SIN VOLANTE (coches de tipo "car"): %d' % (len(sin_coche) + len(sin_asiento)))
    for modelo, motivo in sorted(sin_coche):
        print('  %-14s %s' % (modelo, motivo))
    for modelo, motivo in sorted(sin_asiento):
        print('  %-14s %s' % (modelo, motivo))
    print()
    print('NO SON COCHES (no llevan volante): %d' % sum(len(n) for _, n in otros))
    for tipo, nombres in otros:
        print('  %-6s (%d): %s' % (tipo, len(nombres), ', '.join(nombres)))


def main():
    args = sys.argv[1:]
    if args and args[0] == '--list':
        raiz = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        modelos = args[1] if len(args) > 1 else os.path.join(raiz, 'streamed', 'models', 'gta3.img')
        ide = os.path.join(raiz, 'streamed', 'data', 'default.ide')
        cfg = os.path.join(raiz, 'streamed', 'data', 'handling.cfg')
        lista(modelos, ide, cfg)
        return
    if args and args[0] == '--scan':
        raiz = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
        modelos = args[1] if len(args) > 1 else os.path.join(raiz, 'streamed', 'models', 'gta3.img')
        ide = os.path.join(raiz, 'streamed', 'data', 'default.ide')
        cfg = os.path.join(raiz, 'streamed', 'data', 'handling.cfg')
        scan(modelos, ide, cfg)
        return
    path = args[0]
    buf = open(path, 'rb').read()
    frames, geoms, atomics = dg.parse(buf)
    byframe = {}
    for fi, gi in atomics:
        byframe.setdefault(fi, []).append(gi)
    print('%s: %d marcos, %d atomics, %d geometrias' % (path, len(frames), len(atomics), len(geoms)))

    for fi, gis in byframe.items():
        for gi in gis:
            g = geoms[gi]
            if len(g['tris']) < 40:
                continue
            verts, tris = g['verts'], g['tris']
            mats = {mi: t for mi, (c, t) in enumerate(g['mats'])}
            comps = componentes(tris)
            print('  marco %d (%s) geo %d: %d verts %d tris, %d componentes' %
                  (fi, frames[fi]['name'], gi, len(verts), len(tris), len(comps)))
            filas = []
            for c in comps:
                s = stats(verts, c, tris)
                if s['n'] < 20:
                    continue
                tmats = sorted(set(tris[i][3] for i in c))
                filas.append((s, [mats.get(m) for m in tmats]))
            filas.sort(key=lambda x: -x[0]['n'])
            for s, tm in filas[:14]:
                print('      %4d tris  tam=(%.2f,%.2f,%.2f) centro=(%.2f,%.2f,%.2f) rmax=%.3f anillo=%.2f  mats=%s' %
                      (s['n'], s['size'][0], s['size'][1], s['size'][2],
                       s['c'][0], s['c'][1], s['c'][2], s['rmax'], s['ring'], tm))


if __name__ == '__main__':
    main()
