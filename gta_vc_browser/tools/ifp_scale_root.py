#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Reescala la traslación de la RAÍZ de clips de movimiento de un .ifp, in situ.

Por qué existe (plan agachado-correcciones-axis, E1): el agachado del mod
`sa-crouch` pide `GunCrouchFwd/Bwd` cuya raíz avanza ±2,740 m por ciclo de
0,731 s (3,75 m/s: son clips de SA para SU sistema de movimiento). A la
velocidad honesta del agachado (1,13 m/s, decisión D1=(a)) NO existe
combinación de ritmo que dé a la vez pies pegados y cadencia de andar con esa
zancada: hace falta reescalar la raíz a ~1,5 m/ciclo en el dato servido (el
mismo sitio donde R27 sirvió los clips, plan agachado-calibrado).

Cómo: la traslación de la raíz son los floats 4:7 de cada clave (maquetación
verificada en `ifp_inspect.parse_ifp`: cuaternión 0:3, traslación 4:7, y luego
—solo en KRTS— escala 7:9 y tiempo 10; en KRT0 el tiempo va en el índice 7;
KR00 no lleva traslación y se rechaza). Se reescala RESPECTO A LA PRIMERA
CLAVE (`v' = v0 + (v - v0) * factor`) para no mover el origen del clip; el
desplazamiento por ciclo queda multiplicado por `factor` exactamente y el
tamaño del fichero NO cambia (se escribe in situ).

Uso (desde la raíz del repo):
    python gta_vc_browser/tools/ifp_scale_root.py <destino.ifp> --patron "GunCrouch(Fwd|Bwd)" --metros 1.5
    python gta_vc_browser/tools/ifp_scale_root.py <destino.ifp> --patron "..." --factor 0.55 --aplicar

Por defecto es un SIMULACRO (no escribe nada): dice qué reescalaría, de cuánto
a cuánto y con qué raíz (m/s) resultante, con sha1 antes/después. Con
`--aplicar` escribe y deja `<destino>.bak` (una sola copia; si ya existe, se
ABORTA para no pisar el original del que revertir: muévelo aparte y repite).

`--metros M` deriva el factor por clip (M / |dy| actual); `--factor F` lo fija
igual para todos.

Validaciones (fail-closed: cualquiera aborta ANTES de escribir):
  · el patrón encuentra al menos una animación y cada una tiene curva de raíz
    (`dy` medido por `ifp_inspect`); el factor sale en (0, 2],
  · la raíz del clip NO es KR00 (sin traslación que reescalar),
  · tras escribir: el fichero tiene el MISMO tamaño, `num_anims` y fin de bloque
    intactos, cada animación NO tocada intacta BYTE A BYTE en su rango
    (`start:end`) y cada tocada con `dy` = el objetivo (misma medida que
    `ifp_inspect.py --curva`).
"""

import hashlib
import os
import re
import shutil
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ifp_inspect


def sha1(b):
    return hashlib.sha1(b).hexdigest()[:12]


def abort(msg):
    print('ABORT: %s' % msg)
    return 1


# Claves por tipo: paso en bytes, floats por clave, índice del tiempo.
KDEF = {
    'KRTS': (0x2C, 11, 10, True),
    'KRT0': (0x20, 8, 7, True),
    'KR00': (0x14, 5, 4, False),
}


def walk_seqs(path):
    """Recorre el .ifp como el cargador y devuelve dónde está la clave N.

    Devuelve (data, meta, anims, seqs) donde `seqs[i]` son las secuencias de la
    animación `i`: {ktype, off, num_frames, step}. `off` es el offset del array
    de claves. Replica los dos quirks del recorrido de `ifp_inspect.parse_ifp`
    (DGAN y CPAN no consumen su payload).
    """
    data, meta, anims = ifp_inspect.parse_ifp(path)
    r = ifp_inspect.Reader(data)
    r.o = 0   # recorrido limpio (parse_ifp ya validó la estructura entera)
    ident, size = r.block()
    assert ident == 'ANPK'
    ident, info_size = r.block()
    r.skip(ifp_inspect.round4(info_size))
    seqs = []
    for a in range(meta['num_anims']):
        ident, name_size = r.block()
        r.skip(ifp_inspect.round4(name_size))
        ident, _dgan = r.block()
        ident, info2_size = r.block()
        num_seqs = struct.unpack_from('<i', data, r.o)[0]
        r.skip(ifp_inspect.round4(info2_size))
        aseqs = []
        for s in range(num_seqs):
            ident, _cpan = r.block()
            ident, anim_size = r.block()
            _nm, nf, _bone = ifp_inspect.read_anim_block(r, ifp_inspect.round4(anim_size))
            if nf == 0:
                # Quirk del cargador (y de parse_ifp): sin fotogramas NO hay
                # bloque de claves que leer. La raíz vacía se marca como None.
                aseqs.append(None)
                continue
            ktype, _ksize = r.block()
            step = KDEF[ktype][0]
            aseqs.append({'ktype': ktype, 'off': r.o, 'num_frames': nf, 'step': step})
            r.skip(step * nf)
        seqs.append(aseqs)
    return data, meta, anims, seqs


def main():
    args = list(sys.argv[1:])
    if not args or args[0] in ('-h', '--help'):
        print(__doc__)
        return 1
    aplicar = '--aplicar' in args
    args = [a for a in args if a != '--aplicar']
    if '--patron' not in args:
        print(__doc__)
        return 1
    i = args.index('--patron')
    if i + 1 >= len(args):
        return abort('falta el patrón')
    patron = args[i + 1]
    resto = args[:i] + args[i + 2:]
    metros = factor = None
    for flag, dest in (('--metros', 'metros'), ('--factor', 'factor')):
        if flag in resto:
            j = resto.index(flag)
            if j + 1 >= len(resto):
                return abort('falta el valor de %s' % flag)
            try:
                val = float(resto[j + 1])
            except ValueError:
                return abort('valor de %s no numérico: %r' % (flag, resto[j + 1]))
            if dest == 'metros':
                metros = val
            else:
                factor = val
            resto = resto[:j] + resto[j + 2:]
    if (metros is None) == (factor is None):
        return abort('hace falta exactamente uno de --metros o --factor')
    if not resto:
        return abort('falta el .ifp destino')
    dst = resto[0]

    try:
        data, meta, anims, seqs = walk_seqs(dst)
    except (OSError, AssertionError, SystemExit, struct.error, KeyError, UnicodeDecodeError) as e:
        return abort('%s no se puede recorrer: %s' % (dst, e))

    print('# destino %s  bloque=%s  animaciones=%d  bytes=%d'
          % (dst, meta['block_name'], meta['num_anims'], len(data)))
    rx = re.compile(patron)
    elegidas = [(a, s[0]) for a, s in zip(anims, seqs)
                if rx.search(a['name'])]
    if not elegidas:
        return abort('el patrón %r no encuentra ninguna animación' % patron)
    for a, seq in elegidas:
        if seq is None:
            return abort('%r no tiene claves en su raíz' % a['name'])
        if a['dy'] is None:
            return abort('%r no tiene curva de raíz medible (dy=None)' % a['name'])
        if seq['ktype'] == 'KR00':
            return abort('%r tiene la raíz en KR00 (sin traslación que reescalar)' % a['name'])
        if a['keys'] is None:
            return abort('%r no es un clip de movimiento (sin claves de raíz)' % a['name'])

    print('# %-24s %6s %10s %10s %10s  %s' %
          ('anim', 'frames', 'dy_antes', 'dy_desp', 'm/s_desp', 'sha1'))
    plan = []   # (anim_index, seq, v0, factor)
    for a, seq in elegidas:
        f = factor if factor is not None else (metros / abs(a['dy']))
        if not (0.0 < f <= 2.0):
            return abort('factor %.4f fuera de (0, 2] para %r' % (f, a['name']))
        dy_desp = a['dy'] * f
        dur = a['dur']
        print('  %-24s %6d %10.3f %10.3f %10.2f  %s'
              % (a['name'], a['num_frames'], a['dy'], dy_desp,
                 (dy_desp / dur) if dur > 0.0 else 0.0,
                 sha1(data[a['start']:a['end']])))
        # v0 = traslación de la primera clave de la raíz (índices 4:7).
        v0 = struct.unpack_from('<3f', data, seq['off'] + 4 * 4)
        plan.append((a, seq, v0, f))

    if not aplicar:
        print('# SIMULACRO: no se ha escrito nada. Repite con --aplicar para escribir.')
        print('OK')
        return 0

    bak = dst + '.bak'
    if os.path.exists(bak):
        return abort('ya existe %s: es la copia del original para revertir. Muévelo '
                     'aparte (o bórralo si ya no lo quieres) y repite' % bak)

    out = bytearray(data)
    for a, seq, v0, f in plan:
        step = seq['step']
        for k in range(seq['num_frames']):
            off = seq['off'] + step * k + 4 * 4
            vx, vy, vz = struct.unpack_from('<3f', out, off)
            struct.pack_into('<3f', out, off,
                             v0[0] + (vx - v0[0]) * f,
                             v0[1] + (vy - v0[1]) * f,
                             v0[2] + (vz - v0[2]) * f)
    assert len(out) == len(data), 'el tamaño ha cambiado: bug del reescalado'

    shutil.copy2(dst, bak)
    with open(dst, 'wb') as fh:
        fh.write(bytes(out))
    print('# escrito %s (%d bytes); copia del original en %s' % (dst, len(out), bak))

    # Verificación: recorrido íntegro + rangos byte a byte + raíces medidas.
    data_v, meta_v, anims_v, seqs_v = walk_seqs(dst)
    fallos = []
    if len(data_v) != len(data):
        fallos.append('el tamaño no es el esperado (%d != %d)' % (len(data_v), len(data)))
    if meta_v['num_anims'] != meta['num_anims']:
        fallos.append('num_anims cambió (%d != %d)' % (meta_v['num_anims'], meta['num_anims']))
    if meta_v['end'] != meta['end']:
        fallos.append('el fin del bloque cambió (%d != %d)' % (meta_v['end'], meta['end']))
    tocados = {a['name'] for a, _s, _v, _f in plan}
    for i, b in enumerate(anims):
        a = anims_v[i]
        if a['name'] != b['name'] or a['num_frames'] != b['num_frames']:
            fallos.append('la animación #%d cambió de nombre/fotogramas' % (i + 1))
            break
        if b['name'] not in tocados and data_v[a['start']:a['end']] != data[b['start']:b['end']]:
            fallos.append('%r (no tocada) ha cambiado' % b['name'])
            break
    for a, seq, v0, f in plan:
        v = next(x for x in anims_v if x['name'] == a['name'])
        if abs(v['dy'] - a['dy'] * f) > 0.002:
            fallos.append('%r: dy=%.4f y se esperaba %.4f' % (a['name'], v['dy'], a['dy'] * f))
        if v['dur'] != a['dur']:
            fallos.append('%r: la duración cambió' % a['name'])
    if fallos:
        print('ABORT: el fichero escrito no pasa la verificación: %s\n'
              '       revertir con: mv %s %s' % ('; '.join(fallos), bak, dst))
        return 1
    print('# verificación: %d animaciones, no tocadas intactas byte a byte, raíces:'
          % meta_v['num_anims'])
    for a, _s, _v, _f in plan:
        v = next(x for x in anims_v if x['name'] == a['name'])
        print('  %-24s %+.3f m / %.3f s = %+.2f m/s'
              % (a['name'], v['dy'], v['dur'], (v['dy'] / v['dur']) if v['dur'] > 0.0 else 0.0))
    print('# revertir: mv %s %s' % (bak, dst))
    print('OK')
    return 0


if __name__ == '__main__':
    sys.exit(main())
