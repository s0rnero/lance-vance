#!/usr/bin/env python3
"""Mide donde quedan las MANOS del conductor (clips de direccion de VC).

AVISO: la composicion del esqueleto que hace este script NO reproduce la pose
del retail (el `ped.ifp` guarda el quaternion como delta del reposo y la
cadena de huesos tiene mas de una lectura posible; ver `ped_hands_test.py`, que
compara las lecturas con un clip de pie). Lo que si vale de este script son
las ROTACIONES de los huesos de cada clip (el giro del antebrazo y de la mano
en "Drive_L"/"Drive_R"), que son las que se usan en el informe: de ahi sale
que el brazo del conductor se mueve 14 grados y la mano 90 grados al girar.

Las manos del conductor no son un objeto: son los huesos "L Hand"/"R Hand" del
ped, animados por los clips "CAR_sit" (neutro) y "Drive_L"/"Drive_R" (volante
girado a izquierda/derecha). El motor mezcla esos clips segun el angulo de
direccion (`CPed::ProcessControl`: `blend = Clamp(steerAngle * 100 / 61, 0, 1)`).

Este script reproduce el calculo del motor:
  - lee el esqueleto del ped del DFF (marcos = huesos, con su posicion de
    reposo y su rotacion);
  - lee los clips del .ifp (formato ANPK: KR00 = rotacion, KRT0 = rotacion +
    traslacion, KRTS = rotacion + traslacion + escala) y aplica el mismo
    `CQuaternion::Invert()` que hace `CAnimManager::LoadAnimFile`;
  - compone la cadena de huesos igual que hace el motor
    (`FrameUpdateCallBack`: la rotacion viene de la animacion y la traslacion
    de la posicion de reposo del marco; `RwFrameUpdateObjects` compone
    matrices con la convencion de RenderWare: p' = p * M, filas right/up/at);
  - imprime la posicion de cada mano en el espacio del ped y, a partir de ahi,
    su distancia al centro del volante y el angulo que forman con el eje de la
    columna.

Uso:
    python tools/ped_hands.py --model streamed/models/gta3.img/hmybe.dff \
        --ifp streamed/anim/ped.ifp --clips CAR_sit,Drive_L,Drive_R
"""
import math
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import dff_geom as dg
import ifp_inspect


def quat_rows(qx, qy, qz, qw):
    """Matriz 3x3 (filas right/up/at) de un quaternion, como CQuaternion::Get."""
    x2 = qx + qx
    y2 = qy + qy
    z2 = qz + qz
    x_2x = qx * x2
    x_2y = qx * y2
    x_2z = qx * z2
    y_2y = qy * y2
    y_2z = qy * z2
    z_2z = qz * z2
    w_2x = qw * x2
    w_2y = qw * y2
    w_2z = qw * z2
    return (
        (1.0 - (y_2y + z_2z), x_2y - w_2z, x_2z + w_2y),
        (x_2y + w_2z, 1.0 - (x_2x + z_2z), y_2z - w_2x),
        (x_2z - w_2y, y_2z + w_2x, 1.0 - (x_2x + y_2y)),
    )


def mul_rows(a, b):
    """Producto de dos matrices 3x3 por filas: out[i][j] = sum_k a[i][k]*b[k][j]."""
    return tuple(tuple(sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)) for i in range(3))


def xform(rows, p):
    """p' = p.x*right + p.y*up + p.z*at (convencion RenderWare)."""
    return [p[0] * rows[0][0] + p[1] * rows[1][0] + p[2] * rows[2][0],
            p[0] * rows[0][1] + p[1] * rows[1][1] + p[2] * rows[2][1],
            p[0] * rows[0][2] + p[1] * rows[1][2] + p[2] * rows[2][2]]


def load_skeleton(dff_path):
    """{nombre: (pos de reposo, filas de rotacion, indice del padre)}."""
    buf = open(dff_path, 'rb').read()
    frames, _, _ = dg.parse(buf)
    skel = {}
    for i, f in enumerate(frames):
        r = f['rot']
        skel.setdefault(f['name'], []).append((i, list(f['pos']),
                                               ((r[0], r[1], r[2]), (r[3], r[4], r[5]), (r[6], r[7], r[8])), f['parent']))
    return frames, skel


def clip_keys(ifp_path, clip_name):
    """{nombre de hueso: (quat, trans) de la primera clave} de un clip."""
    data, meta, anims = ifp_inspect.parse_ifp(ifp_path)
    for a in anims:
        if a['name'] != clip_name:
            continue
        out = {}
        for s in a['seqs']:
            if not s['num_frames']:
                continue
            off = s['start']
            # saltar CPAN + ANIM + cabecera hasta las claves
            # (ifp_inspect ya dejo el offset de las claves en 'end')
            # se recalcula: las claves empiezan tras la cabecera de la secuencia
            out[s['name']] = s
        return a, out, data
    return None, None, None


def read_first_key(data, seq, anim_start):
    """Lee la primera clave de una secuencia: (quat motor, traslacion)."""
    # el bloque CPAN/ANIM: el ifp_inspect guardaba 'start' = offset del CPAN
    o = seq['start']
    ident, cpan_size = struct.unpack_from('<4sI', data, o)
    o += 8                         # cabecera CPAN
    ident, anim_size = struct.unpack_from('<4sI', data, o)
    o += 8                         # cabecera ANIM
    o += round4(anim_size)         # payload ANIM (name, numFrames, ...)
    ident, ksize = struct.unpack_from('<4sI', data, o)
    o += 8                         # cabecera del bloque de claves
    if ident == b'KRTS':
        vals = struct.unpack_from('<11f', data, o)
        q, t = vals[0:4], vals[4:7]
    elif ident == b'KRT0':
        vals = struct.unpack_from('<8f', data, o)
        q, t = vals[0:4], vals[4:7]
    else:
        vals = struct.unpack_from('<5f', data, o)
        q, t = vals[0:4], (0.0, 0.0, 0.0)
    # el motor invierte el quaternion (CAnimManager::LoadAnimFile)
    q = (-q[0], -q[1], -q[2], q[3])
    return q, t


def read_last_key(data, seq):
    """Ultima clave de una secuencia: (quat motor, traslacion)."""
    o = seq['start']
    ident, cpan_size = struct.unpack_from('<4sI', data, o)
    o += 8
    ident, anim_size = struct.unpack_from('<4sI', data, o)
    o += 8
    o += round4(anim_size)
    ident, ksize = struct.unpack_from('<4sI', data, o)
    o += 8
    if ident == b'KRTS':
        step = 0x2C
    elif ident == b'KRT0':
        step = 0x20
    else:
        step = 0x14
    o += step * (seq['num_frames'] - 1)
    if ident == b'KRTS':
        vals = struct.unpack_from('<11f', data, o)
        q, t = vals[0:4], vals[4:7]
    elif ident == b'KRT0':
        vals = struct.unpack_from('<8f', data, o)
        q, t = vals[0:4], vals[4:7]
    else:
        vals = struct.unpack_from('<5f', data, o)
        q, t = vals[0:4], (0.0, 0.0, 0.0)
    q = (-q[0], -q[1], -q[2], q[3])
    return q, t


def round4(x):
    return x + (4 - (x & 3) if x & 3 else 0)


def cadena(frames, idx):
    out = []
    i = idx
    while 0 <= i < len(frames):
        out.append(i)
        i = frames[i]['parent']
    return out


def mano_en_ped(frames, skel, bone_name, quats, root_frame=1):
    """Posicion del hueso en el espacio del ped (origen = raiz del ped)."""
    # el hueso pedido
    idx = None
    for i, f in enumerate(frames):
        if f['name'] == bone_name:
            idx = i
            break
    if idx is None:
        return None
    chain = cadena(frames, idx)
    # mundo de cada hueso: (pos, filas)
    world = {0: ([0.0, 0.0, 0.0], ((1.0, 0.0, 0.0), (0.0, 1.0, 0.0), (0.0, 0.0, 1.0)))}
    for fi in reversed(chain[:-1]):
        f = frames[fi]
        pid = f['parent']
        ppos, prows = world[pid]
        q = quats.get(f['name'])
        if q is None:
            rows = ((f['rot'][0], f['rot'][1], f['rot'][2]),
                    (f['rot'][3], f['rot'][4], f['rot'][5]),
                    (f['rot'][6], f['rot'][7], f['rot'][8]))
        else:
            rows = quat_rows(*q)
        pos = [ppos[i] + xform(prows, list(f['pos']))[i] for i in range(3)]
        world[fi] = (pos, mul_rows(rows, prows))
    return world[idx][0]


def main():
    args = sys.argv[1:]
    def opt(name, default=None):
        if name in args:
            return args[args.index(name) + 1]
        return default
    model = opt('--model')
    ifp = opt('--ifp')
    clips = (opt('--clips', 'CAR_sit,Drive_L,Drive_R')).split(',')
    root_frame = int(opt('--root', '1'))
    if not model or not ifp:
        print(__doc__)
        return 1
    frames, skel = load_skeleton(model)
    print('# ped %s  (%d marcos)' % (os.path.basename(model), len(frames)))

    for clip in clips:
        a, seqs, data = clip_keys(ifp, clip)
        if a is None:
            print('# clip %s: no existe' % clip)
            continue
        quats = {}
        quatsFin = {}
        for s in seqs.values():
            if not s['num_frames']:
                continue
            q, t = read_first_key(data, s, a['start'])
            quats[s['name']] = q
            ql = read_last_key(data, s)
            if ql is not None:
                quatsFin[s['name']] = ql[0]
        print('# clip %s (%d secuencias, %d claves raiz)' % (clip, a['num_seqs'], a['num_frames']))
        for bone in ('L Hand', 'R Hand'):
            p = mano_en_ped(frames, skel, bone, quats, root_frame)
            pf = mano_en_ped(frames, skel, bone, quatsFin, root_frame)
            if p is None:
                print('    %-8s: (no esta en el modelo)' % bone)
            else:
                d = ''
                if pf is not None:
                    d = '  ultima clave: %+.4f %+.4f %+.4f  (delta %+.4f %+.4f %+.4f)' % (
                        pf[0], pf[1], pf[2], pf[0] - p[0], pf[1] - p[1], pf[2] - p[2])
                print('    %-8s: %+.4f %+.4f %+.4f%s' % (bone, p[0], p[1], p[2], d))
    return 0


if __name__ == '__main__':
    sys.exit(main())
