#!/usr/bin/env python3
"""Mide la pose 3D del volante de un coche: marco, eje y aro.

El mod (Vice Extended) trae 18 coches con el volante como objeto aparte
(marco "steeringwheel", textura "stwheel") y lo gira en PreRender con
`mat.SetRotate(headingX, -m_fSteerAngle * 2.0f, 0.0f)`: eso gira el marco
alrededor de SU ORIGEN (el pivote), no del centro del aro. Si el origen del
marco no coincide con el centro del aro, al girar la rueda se separa de las
manos (que en el juego son fijas).

Este script mide:
  - la matriz local y la posicion en el espacio de la raiz del marco
    "steeringwheel", y de "chassis_hi" y "ped_frontseat";
  - el centro del aro (la banda exterior de los vertices del atomic), su
    radio y el eje de giro, en el espacio del marco y en el de la raiz;
  - el desplazamiento del centro del aro respecto al origen del marco (el
    error de pivote) y cuanto se separa el aro al girar.

El marco del volante tiene el eje de giro en su Y local (adelante) y el aro
en su plano X-Z local, asi que el ajuste del aro se hace en ese sistema.

Uso:
    python tools/dff_wheel_pose.py <coche.dff> [--frame steeringwheel]
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dff_geom as dg


def mul3(m, v):
    return [m[0][0] * v[0] + m[0][1] * v[1] + m[0][2] * v[2],
            m[1][0] * v[0] + m[1][1] * v[1] + m[1][2] * v[2],
            m[2][0] * v[0] + m[2][1] * v[1] + m[2][2] * v[2]]


def frame_index(frames, name):
    for i, f in enumerate(frames):
        if f['name'].lower() == name.lower():
            return i
    return -1


def chain_of(frames, idx):
    out = []
    i = idx
    while 0 <= i < len(frames):
        out.append(i)
        i = frames[i]['parent']
    return out


def world_matrix(frames, idx):
    """Matriz 4x4 del marco idx en el espacio de la raiz del clump."""
    m = [[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 1.0]]
    for fi in reversed(chain_of(frames, idx)):
        f = frames[fi]
        r0, r1, r2, p = dg.frame_matrix(f)
        loc = [[r0[0], r0[1], r0[2], p[0]],
               [r1[0], r1[1], r1[2], p[1]],
               [r2[0], r2[1], r2[2], p[2]],
               [0.0, 0.0, 0.0, 1.0]]
        m = [[sum(m[a][k] * loc[k][b] for k in range(4)) for b in range(4)] for a in range(4)]
    return m


def aro_local(verts):
    """Centro y radio del aro en el espacio local del marco.

    El aro esta en el plano X-Z local (el eje de giro es el Y local). Se
    toma la banda exterior de los vertices (los que estan a mas del 85% del
    radio maximo) y se promedia.
    """
    rs = [math.hypot(v[0], v[2]) for v in verts]
    rmax = max(rs)
    banda = [v for v, r in zip(verts, rs) if r >= 0.85 * rmax]
    cx = sum(v[0] for v in banda) / len(banda)
    cz = sum(v[2] for v in banda) / len(banda)
    cy = sum(v[1] for v in banda) / len(banda)
    radio = sum(math.hypot(v[0] - cx, v[2] - cz) for v in banda) / len(banda)
    return [cx, cy, cz], radio, rmax, len(banda)


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return
    path = args[0]
    want = 'steeringwheel'
    if '--frame' in args:
        want = args[args.index('--frame') + 1]
    buf = open(path, 'rb').read()
    frames, geoms, atomics = dg.parse(buf)
    byframe = {}
    for fi, gi in atomics:
        byframe.setdefault(fi, []).append(gi)

    fi = frame_index(frames, want)
    if fi < 0:
        print('el DFF no tiene el marco "%s"' % want)
        print('marcos: %s' % ', '.join(f['name'] for f in frames))
        return
    f = frames[fi]
    r0, r1, r2, p = dg.frame_matrix(f)
    print('DFF: %s' % path)
    print('marco "%s" (indice %d) padre "%s"' % (f['name'], fi, frames[f['parent']]['name']))
    print('  matriz local:')
    print('    right = %8.4f %8.4f %8.4f' % r0)
    print('    fwd   = %8.4f %8.4f %8.4f' % r1)
    print('    up    = %8.4f %8.4f %8.4f' % r2)
    print('    pos   = %8.4f %8.4f %8.4f' % p)
    at = [r1[0], r1[1], r1[2]]
    headingX = math.atan2(at[2], at[1])
    print('    headingX = atan2(at.z, at.y) = %+.4f rad = %+.2f grados' %
          (headingX, math.degrees(headingX)))
    right = [r0[0], r0[1], r0[2]]
    up = [r2[0], r2[1], r2[2]]
    print('    roll (right vs up): %.2f grados' % math.degrees(math.acos(
        max(-1.0, min(1.0, sum(a * b for a, b in zip(right, up)))))))

    wm = world_matrix(frames, fi)
    print('  posicion en la raiz: %.4f %.4f %.4f' % (wm[0][3], wm[1][3], wm[2][3]))
    for nm in ('chassis_hi', 'ped_frontseat'):
        j = frame_index(frames, nm)
        if j >= 0:
            wmj = world_matrix(frames, j)
            print('  "%s" en la raiz: %.4f %.4f %.4f' % (nm, wmj[0][3], wmj[1][3], wmj[2][3]))

    lm = [[r0[0], r0[1], r0[2]], [r1[0], r1[1], r1[2]], [r2[0], r2[1], r2[2]]]
    for gi in byframe.get(fi, []):
        g = geoms[gi]
        vs = g['verts']
        ts = g['tris']
        if len(vs) < 10:
            continue
        c, radio, rmax, nbanda = aro_local(vs)
        print('  atomic geo %d: %d verts %d tris' % (gi, len(vs), len(ts)))
        for mi, (color, tex) in enumerate(g['mats']):
            print('      mat %d tex %s' % (mi, tex))
        print('      centro del aro (local): %+.4f %+.4f %+.4f' % (c[0], c[1], c[2]))
        print('      radio del aro: %.4f m (max %.4f, banda %d v)' % (radio, rmax, nbanda))
        cw = mul3(wm, c)
        pw = [wm[0][3], wm[1][3], wm[2][3]]
        print('      centro del aro (raiz):  %+.4f %+.4f %+.4f' % (cw[0], cw[1], cw[2]))
        print('      origen del marco (raiz):%+.4f %+.4f %+.4f' % (pw[0], pw[1], pw[2]))
        off = [cw[i] - pw[i] for i in range(3)]
        print('      offset pivote->aro (raiz): %+.4f %+.4f %+.4f  (|.|=%.4f m)' %
              (off[0], off[1], off[2], math.sqrt(sum(x * x for x in off))))
        eje = [r1[0], r1[1], r1[2]]
        print('      eje de giro (raiz): %.4f %.4f %.4f' % tuple(mul3(wm, eje)))
        # cuanto se separa el aro del pivote al girar sobre el eje Y local
        print('      separacion del centro del aro al girar (pivote = origen):')
        for deg in (5, 10, 15, 20, 30, 45, 60, 90):
            th = math.radians(deg)
            v = [c[0], c[1], c[2]]
            # Rodrigues sobre el eje Y local (0,1,0)
            cr = [v[0] * math.cos(th) + v[2] * math.sin(th), v[1], -v[0] * math.sin(th) + v[2] * math.cos(th)]
            d = math.sqrt(sum((cr[i] - c[i]) ** 2 for i in range(3)))
            if deg in (5, 10, 20, 30, 45, 60):
                print('        %2d grados -> %.4f m' % (deg, d))
        # separacion del punto de agarre (a 9 y 3, en el plano X-Z local)
        for deg in (10, 20, 30, 45, 60):
            th = math.radians(deg)
            for signo, nm in ((1, '9 h  (izquierda)'), (-1, '3 h (derecha)')):
                v = [signo * radio, c[1], 0.0]
                cr = [v[0] * math.cos(th) + v[2] * math.sin(th), v[1], -v[0] * math.sin(th) + v[2] * math.cos(th)]
                d = math.sqrt(sum((cr[i] - v[i]) ** 2 for i in range(3)))
                print('        %2d grados: el punto %s se mueve %.4f m' % (deg, nm, d))


if __name__ == '__main__':
    main()
