#!/usr/bin/env python3
"""Localiza la rueda PINTADA del salpicadero en la malla de un coche.

En GTA VC el volante del salpicadero no es un objeto: son unos triangulos
planos (un anillo + radios + la columna) en el plano del salpicadero, en
frente al conductor, pintados con la textura del cuerpo del coche. El mod
(Vice Extended) se lo quito a 18 coches y puso en su lugar un modelo 3D
("steeringwheel", textura "stwheel"). A los demas coches el motor les anade
esa misma rueda 3D, pero la pintada sigue ahi: se ven dos volantes.

Este script encuentra los triangulos de la rueda pintada para poder quitar-
los en el motor (src/vehicles/SteeringWheel.cpp). Es la version python de lo
que hace el motor: misma regla de colocacion (asiento del conductor + los
desplazamientos medidos en los 18 coches del mod).

Uso:
    python tools/dff_wheelpaint.py <coche.dff>              # triangulos candidatos
    python tools/dff_wheelpaint.py <coche.dff> --verbose   # con detalle
    python tools/dff_wheelpaint.py --scan [modelos_dir]    # todos los coches
"""
import os
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dff_geom as dg

WHEEL_DX = 0.008
WHEEL_DY = 0.327
WHEEL_DZ = 0.360
WHEEL_MIN_SEAT = 0.25

HANDLING_IS_BIKE = 0x10000
HANDLING_IS_HELI = 0x20000
HANDLING_IS_PLANE = 0x40000
HANDLING_IS_BOAT = 0x80000

# Radio de busqueda alrededor del centro esperado de la rueda.
R_BUSQUEDA = 0.28
# Lado maximo de un triangulo que puede ser parte de la rueda (el salpicadero
# entero y el parabrisas son triangulos de mas de un metro).
E_MAX = 0.35
# Lo que exige el motor para dar por buena la rueda pintada (mismos valores
# que src/vehicles/SteeringWheel.cpp).
MIN_TRIS = 8
MAX_TRIS = 100
TAMANO_MAX = 0.55


def mul3(a, b):
    """Producto de matrices 3x3 (filas)."""
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]


def frame_mat(f):
    """Matriz 3x3 (filas right/up/at) y posicion de un marco, tal cual."""
    r = f['rot']
    return [[r[0], r[1], r[2]], [r[3], r[4], r[5]], [r[6], r[7], r[8]]], list(f['pos'])


def xform(m, p, pos):
    """p' = M * p + pos (los marcos de GTA van con la convencion de RenderWare:
    la matriz tiene las filas right/up/at y el punto se multiplica por filas)."""
    return [m[0][0] * p[0] + m[1][0] * p[1] + m[2][0] * p[2] + pos[0],
            m[0][1] * p[0] + m[1][1] * p[1] + m[2][1] * p[2] + pos[1],
            m[0][2] * p[0] + m[1][2] * p[1] + m[2][2] * p[2] + pos[2]]


def acc_matrix(frames, idx):
    """Matriz acumulada (marco -> raiz) y posicion: el marco, luego su padre,
    etc. (p' = p*M + pos, la convencion de RenderWare: las filas de la matriz
    son right/up/at)."""
    m, pos = frame_mat(frames[idx])
    j = frames[idx]['parent']
    while 0 <= j < len(frames):
        pm, ppos = frame_mat(frames[j])
        pos = [pos[0] * pm[0][0] + pos[1] * pm[1][0] + pos[2] * pm[2][0] + ppos[0],
               pos[0] * pm[0][1] + pos[1] * pm[1][1] + pos[2] * pm[2][1] + ppos[1],
               pos[0] * pm[0][2] + pos[1] * pm[1][2] + pos[2] * pm[2][2] + ppos[2]]
        m = mul3(m, pm)
        j = frames[j]['parent']
    return m, pos


def world_pos(frames, idx):
    m, pos = acc_matrix(frames, idx)
    return xform(m, [0.0, 0.0, 0.0], pos)


def frame_index(frames, name):
    for i, f in enumerate(frames):
        if f['name'].lower() == name:
            return i
    return -1


def centro_rueda(asiento):
    """Centro esperado de la rueda: el asiento del conductor espejado en X
    (el motor pone al conductor en -x) mas los desplazamientos del mod."""
    return [-asiento[0] + WHEEL_DX, asiento[1] + WHEEL_DY, asiento[2] + WHEEL_DZ]


def componentes(tri_idx, tris):
    """Componentes conexas de los triangulos (comparten indice de vertice)."""
    parent = {}

    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]
            a = parent[a]
        return a

    for i in tri_idx:
        for v in tris[i][:3]:
            parent.setdefault(v, v)
    for i in tri_idx:
        a, b, c = find(tris[i][0]), find(tris[i][1]), find(tris[i][2])
        if a != b:
            parent[b] = a
        b = find(tris[i][1])
        if b != c:
            parent[c] = b
    groups = defaultdict(list)
    for i in tri_idx:
        groups[find(tris[i][0])].append(i)
    return list(groups.values())


def normals(verts, tri):
    a, b, c = [verts[tri[k]] for k in range(3)]
    u = [b[k] - a[k] for k in range(3)]
    v = [c[k] - a[k] for k in range(3)]
    n = [u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]]
    l = sum(x * x for x in n) ** 0.5
    return [x / l for x in n] if l > 1e-9 else [0.0, 0.0, 0.0]


def busca(frames, geoms, atomics):
    """Devuelve (centro esperado, lista de candidatos) donde cada candidato es
    (marco, geo, indice de triangulo, normal, vertices en espacio de raiz)."""
    i_seat = frame_index(frames, 'ped_frontseat')
    if i_seat < 0:
        return None, []
    asiento = world_pos(frames, i_seat)
    centro = centro_rueda(asiento)

    byframe = defaultdict(list)
    for fi, gi in atomics:
        byframe[fi].append(gi)

    out = []
    for fi, gis in byframe.items():
        m, pos = acc_matrix(frames, fi)
        for gi in gis:
            g = geoms[gi]
            if not g['verts']:
                continue
            for ti, tr in enumerate(g['tris']):
                vs = [g['verts'][tr[k]] for k in range(3)]
                cen = [sum(v[k] for v in vs) / 3.0 for k in range(3)]
                cenr = xform(m, cen, pos)
                d = sum((cenr[k] - centro[k]) ** 2 for k in range(3)) ** 0.5
                if d > R_BUSQUEDA:
                    continue
                # Lado mayor del triangulo: el salpicadero entero es enorme.
                lado = 0.0
                for a in range(3):
                    for b in range(a + 1, 3):
                        lado = max(lado, sum((vs[a][k] - vs[b][k]) ** 2 for k in range(3)) ** 0.5)
                if lado > E_MAX:
                    continue
                vsr = [xform(m, v, pos) for v in vs]
                out.append((fi, gi, ti, normals(g['verts'], tr), vsr, d))
    return (asiento, centro), out


def describe(frames, geoms, atomics, verbose=False):
    (asiento, centro), cand = busca(frames, geoms, atomics)
    if asiento is None:
        print('  (sin marco ped_frontseat)')
        return None
    print('  asiento=(%.3f,%.3f,%.3f) centro esperado=(%.3f,%.3f,%.3f) candidatos=%d' %
          (asiento[0], asiento[1], asiento[2], centro[0], centro[1], centro[2], len(cand)))
    bygeo = defaultdict(list)
    for c in cand:
        bygeo[(c[0], c[1])].append(c)
    for (fi, gi), lst in sorted(bygeo.items()):
        tris = [c[2] for c in lst]
        comps = componentes(tris, geoms[gi]['tris'])
        mats = [t for (_, t) in geoms[gi]['mats']]
        print('  marco %d (%s) geo %d: %d triangulos candidatos, %d componentes' %
              (fi, frames[fi]['name'], gi, len(tris), len(comps)))
        for comp in sorted(comps, key=lambda x: -len(x)):
            pts = []
            ns = []
            for i in comp:
                c = [x for x in cand if x[0] == fi and x[1] == gi and x[2] == i][0]
                pts.extend(c[4])
                ns.append(c[3])
            lo = [min(p[k] for p in pts) for k in range(3)]
            hi = [max(p[k] for p in pts) for k in range(3)]
            tam = [hi[k] - lo[k] for k in range(3)]
            cen = [(lo[k] + hi[k]) * 0.5 for k in range(3)]
            rmax = max(sum((p[k] - cen[k]) ** 2 for k in range(3)) ** 0.5 for p in pts)
            rmin = min(sum((p[k] - cen[k]) ** 2 for k in range(3)) ** 0.5 for p in pts)
            # separacion del plano medio (los tres ejes)
            plano = [sum(abs(p[k] - cen[k]) for p in pts) / len(pts) for k in range(3)]
            tm = sorted(set(geoms[gi]['tris'][i][3] for i in comp))
            print('      %3d tris tam=(%.2f,%.2f,%.2f) rmax=%.3f rmin=%.3f plano=%s mats=%s' %
                  (len(comp), tam[0], tam[1], tam[2], rmax, rmin,
                   '%.3f,%.3f,%.3f' % tuple(plano),
                   [mats[m] if m < len(mats) else '?' for m in tm]))
            if verbose:
                for i in comp:
                    c = [x for x in cand if x[0] == fi and x[1] == gi and x[2] == i][0]
                    print('        t%-5d %s  n=(%.2f,%.2f,%.2f)' %
                          (i, '  '.join('(%7.3f,%7.3f,%7.3f)' % tuple(v) for v in c[4]), *c[3]))
    return centro


def _ide_vehicles(ide, tipo='car'):
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
    print('%-14s %-12s %-24s %-24s %-34s %s' %
          ('modelo', 'estado', 'centro esperado', 'centro de la rueda', 'detalle', 'que hace el motor'))
    resumen = {}
    for modelo, handling in _ide_vehicles(ide):
        path = os.path.join(modelos, modelo + '.dff')
        if not os.path.exists(path):
            continue
        buf = open(path, 'rb').read()
        frames, geoms, atomics = dg.parse(buf)
        i_seat = frame_index(frames, 'ped_frontseat')
        i_wheel = frame_index(frames, 'steeringwheel')
        asiento = world_pos(frames, i_seat) if i_seat >= 0 else None
        fl = flags.get(handling.upper(), 0)
        if i_wheel >= 0:
            estado = 'del-mod'
        elif fl & (HANDLING_IS_HELI | HANDLING_IS_PLANE | HANDLING_IS_BIKE | HANDLING_IS_BOAT):
            estado = 'no-coche'
        elif asiento is None or abs(asiento[0]) < WHEEL_MIN_SEAT:
            estado = 'sin-asiento'
        else:
            estado = 'anadida'
        (a, centro), cand = busca(frames, geoms, atomics)
        pts = []
        uvs = []
        for c in cand:
            pts.extend(c[4])
            g = geoms[c[1]]
            if g['uvs'] and g['uvs'][0]:
                for k in range(3):
                    uvs.append(g['uvs'][0][g['tris'][c[2]][k]])
        if pts:
            lo = [min(p[k] for p in pts) for k in range(3)]
            hi = [max(p[k] for p in pts) for k in range(3)]
            cen = [(lo[k] + hi[k]) * 0.5 for k in range(3)]
            tam = max(hi[k] - lo[k] for k in range(3))
            err = sum((cen[k] - centro[k]) ** 2 for k in range(3)) ** 0.5
            real = '%.3f,%.3f,%.3f' % tuple(cen)
            det = '%d tris tam=%.2f err=%.3f' % (len(cand), tam, err)
            if uvs:
                det += ' uv=%.3f-%.3f,%.3f-%.3f' % (min(u[0] for u in uvs), max(u[0] for u in uvs),
                                                    min(u[1] for u in uvs), max(u[1] for u in uvs))
        else:
            real = '-'
            det = '0 tris'
        clave = (estado, bool(cand))
        resumen[clave] = resumen.get(clave, 0) + 1
        # Esto es lo que hace el motor (src/vehicles/SteeringWheel.cpp): si la
        # rueda pintada no aparece con seguridad, no se anade la rueda 3D.
        if estado == 'anadida':
            if not cand:
                decision = 'SIN rueda 3D (no hay rueda pintada)'
            elif not (MIN_TRIS <= len(cand) <= MAX_TRIS) or tam > TAMANO_MAX:
                decision = 'SIN rueda 3D (rueda pintada dudosa)'
            else:
                decision = 'rueda 3D + quitada la pintada'
        elif estado == 'del-mod':
            decision = 'rueda 3D del mod (no se toca)'
        else:
            decision = '-'
        print('%-14s %-12s %-24s %-24s %-34s %s' % (
            modelo, estado,
            '%.3f,%.3f,%.3f' % tuple(centro) if centro else '-',
            real, det, decision))
    print()
    for k in sorted(resumen):
        print('  %-12s rueda-pintada=%s : %d' % (k[0], 'si' if k[1] else 'no', resumen[k]))


def main():
    args = sys.argv[1:]
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
    print(path)
    describe(frames, geoms, atomics, '--verbose' in args)


if __name__ == '__main__':
    main()
