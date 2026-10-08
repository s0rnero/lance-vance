#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Añade animaciones de un .ifp a otro sin tocar las que ya hay.

Por qué existe: el agachado del mod `sa-crouch` pide por NOMBRE dos clips
(`GunCrouchFwd` y `GunCrouchBwd`) que el `ped.ifp` servido no traía, y el motor
empareja por nombre. Sin esos clips en el fichero no hay agachado andando: el
dato tiene que llevar las animaciones.

Cómo: `CAnimManager::LoadAnimFile` avanza por el stream y sólo se guía por el
`num_anims` del primer `INFO`, leyendo animaciones una detrás de otra hasta
agotar ese contador. Así que añadir animaciones es copiar sus bytes (bloque
NAME entero + sus claves, tal y como los lee `ifp_inspect.parse_ifp`) al final
del último bloque y subir ese contador: nada de lo que ya había se mueve.

Uso (desde la raíz del repo):
    python gta_vc_browser/tools/ifp_add.py <destino.ifp> <origen.ifp> --patron "GunCrouch(Fwd|Bwd)"
    python gta_vc_browser/tools/ifp_add.py <destino.ifp> <origen.ifp> --patron "..." --aplicar

Por defecto es un SIMULACRO (no escribe nada): dice qué añadiría, con qué
tamaño y con qué hashes. Con `--aplicar` escribe y deja `<destino>.bak` (una
sola copia; si ya existe, se aborta para no pisar el original del que revertir).

Modo dirigido (reemplazar en el sitio, sin tocar el modo añadir):
    python gta_vc_browser/tools/ifp_add.py <destino.ifp> <origen.ifp> --patron "^Crouch_Roll_[LR]$" --reemplazar --copia <copia.ifp>
    python gta_vc_browser/tools/ifp_add.py <destino.ifp> <origen.ifp> --patron "..." --reemplazar --copia <copia.ifp> --aplicar

`--reemplazar` cambia la acción: por cada animación elegida del origen, sustituye
la del destino que lleva su MISMO nombre (sin distinguir mayúsculas) por los
bytes del origen, conservando orden, `num_anims`, cola y el resto de bloques
byte a byte. El nombre tiene que ser único en el destino. `--copia <ruta>` es
obligatoria en este modo y exige una copia EXCLUSIVA fuera de las raíces
servidas (`streamed/`, `bootseed/`): no puede existir ya (no se pisa una copia
ajena) ni ser el propio destino. El simulacro no crea la copia; `--aplicar`
construye y verifica TODO el resultado en memoria (recorrido ANPK completo,
bytes del origen en lo reemplazado, bytes intactos en todo lo demás) antes de
escribir nada: cualquier fallo deja el destino sin tocar.

Validaciones (fail-closed: cualquiera aborta ANTES de escribir):
  · los dos ficheros se recorren enteros y sin cola rara al final,
  · ningún nombre a añadir existe ya en el destino (el motor empareja por
    nombre, sin distinguir mayúsculas),
  · cada animación elegida tiene claves y traslación de RAÍZ (si no, no es un
    clip de movimiento y no se añade),
  · destino y origen son del mismo bloque de animaciones.

Tras escribir vuelve a recorrer el resultado y comprueba: cada animación que ya
estaba intacta BYTE A BYTE (su rango del fichero; el contador `num_anims` sí
cambia, es el único byte que se toca de lo viejo), el contador subido, las
animaciones nuevas en su sitio y su `raiz` medida (m/s) — el mismo número que
imprime `ifp_inspect.py --curva`.
"""

import hashlib
import os
import re
import shutil
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ifp_inspect


# Cola admitida tras la última animación. El cargador no la lee (deja de leer
# al agotar `num_anims`), así que se conserva tal cual detrás de lo insertado.
COLA_MAX = 8

# El campo `size` del ANPK no lo usa el cargador (sólo se redondea y se
# ignora); lo que importa es que siga la misma convención que traía el destino,
# así que se desplaza exactamente con el fichero.
ANPK_SIZE_OFF = 4


def sha1(b):
    return hashlib.sha1(b).hexdigest()[:12]


def abort(msg):
    print('ABORT: %s' % msg)
    return 1


def carga(path, etiqueta):
    """Recorre un .ifp para escribir en él; aborta si no cuadra."""
    try:
        data, meta, anims = ifp_inspect.parse_ifp(path)
    except (OSError, AssertionError, SystemExit, struct.error, UnicodeDecodeError) as e:
        print('ABORT: %s (%s) no se puede recorrer: %s' % (etiqueta, path, e))
        return None
    if meta['tail'] > COLA_MAX:
        print('ABORT: %s (%s): el recorrido termina %d bytes antes del final '
              '(estructura no reconocida); no se toca nada'
              % (etiqueta, path, meta['tail']))
        return None
    if meta['tail']:
        print('# aviso: %s trae %d byte(s) de cola tras la última animación '
              '(se conservan al final)' % (etiqueta, meta['tail']))
    return data, meta, anims


def _normal(p):
    return os.path.normcase(os.path.abspath(p))


def _dentro_de(p, raiz):
    p = _normal(p)
    raiz = _normal(raiz)
    return p == raiz or p.startswith(raiz + os.sep)


def valida_copia(dst, copia):
    """Aborta si la copia no es exclusiva y fuera de las raíces servidas."""
    if copia is None:
        return abort('--reemplazar exige --copia <ruta>: copia exclusiva fuera '
                     'de las raíces servidas (streamed/, bootseed/)')
    base = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    for raiz in ('streamed', 'bootseed'):
        if _dentro_de(copia, os.path.join(base, raiz)):
            return abort('--copia %s cae dentro de %s/ (raíz servida): elige una '
                         'ruta fuera' % (copia, raiz))
    if _normal(copia) == _normal(dst):
        return abort('--copia no puede ser el propio destino (%s)' % dst)
    if os.path.exists(copia):
        return abort('ya existe %s: no se pisa una copia ajena; elige otro nombre'
                     % copia)
    return None


def verifica_reemplazo(out, data_d, meta_d, anims_d, data_s, reemplazos):
    """Recorre el resultado en memoria y devuelve la lista de fallos."""
    fallos = []
    try:
        data_v, meta_v, anims_v = ifp_inspect.parse_ifp(out)
    except (AssertionError, SystemExit, struct.error, UnicodeDecodeError) as e:
        return ['el resultado no se puede recorrer como ANPK: %s' % e]
    if len(data_v) != len(out):
        fallos.append('el tamaño no es el esperado (%d != %d)' % (len(data_v), len(out)))
    if meta_v['block_name'] != meta_d['block_name']:
        fallos.append('el bloque cambió (%s -> %s)'
                      % (meta_d['block_name'], meta_v['block_name']))
    if meta_v['num_anims'] != meta_d['num_anims']:
        fallos.append('num_anims=%d (se esperaba %d)'
                      % (meta_v['num_anims'], meta_d['num_anims']))
    if meta_v['tail'] != meta_d['tail']:
        fallos.append('la cola cambió (%d -> %d)' % (meta_d['tail'], meta_v['tail']))
    if len(anims_v) != len(anims_d):
        fallos.append('la lista de animaciones cambió (%d -> %d)'
                      % (len(anims_d), len(anims_v)))
        return fallos
    for k, a in enumerate(anims_d):
        if anims_v[k]['name'] != a['name']:
            fallos.append('#%d cambió de nombre (%s -> %s)'
                          % (k + 1, a['name'], anims_v[k]['name']))
            break
    tocados = set()
    for k, b in reemplazos:
        tocados.add(k)
        v = anims_v[k]
        if data_v[v['start']:v['end']] != data_s[b['start']:b['end']]:
            fallos.append('%s no quedó con los bytes del origen' % b['name'])
        elif v['num_frames'] != b['num_frames'] or v['dy'] != b['dy']:
            fallos.append('%s no coincide con su original del origen' % b['name'])
    for k, a in enumerate(anims_d):
        if k in tocados:
            continue
        v = anims_v[k]
        if data_v[v['start']:v['end']] != data_d[a['start']:a['end']]:
            fallos.append('%s (que no se tocaba) cambió' % a['name'])
            break
    return fallos


def ejecuta_reemplazo(dst, copia, data_d, meta_d, anims_d, data_s, meta_s, anims_s,
                      elegidas, aplicar):
    fallo = valida_copia(dst, copia)
    if fallo is not None:
        return fallo
    if not anims_d:
        return abort('el destino no tiene animaciones')

    indices = {}
    for k, a in enumerate(anims_d):
        clave = a['name'].lower()
        if clave in indices:
            return abort('el destino trae %r más de una vez: no hay nombre único '
                         'que reemplazar' % a['name'])
        indices[clave] = k

    reemplazos = []
    vistos = set()
    for b in elegidas:
        clave = b['name'].lower()
        if clave in vistos:
            return abort('el origen trae %r más de una vez: no hay nombre único que '
                         'reemplazar' % b['name'])
        vistos.add(clave)
        k = indices.get(clave)
        if k is None:
            return abort('el destino no tiene ninguna animación llamada %r' % b['name'])
        if b['keys'] is None or b['dy'] is None:
            return abort('%r no es un clip de movimiento (sin claves de raíz con '
                         'traslación): no se reemplaza' % b['name'])
        reemplazos.append((k, b))
    reemplazos.sort()
    nuevo_por_indice = {k: b for k, b in reemplazos}

    print('# reemplazar en %s (bloque %s, %d animaciones):'
          % (dst, meta_d['block_name'], meta_d['num_anims']))
    print('  %-26s %10s %10s  %s' % ('anim', 'antes', 'despues', 'raiz origen'))
    for k, b in reemplazos:
        a = anims_d[k]
        dur = b['dur']
        raiz = ('%+.3f m / %.3f s = %+.2f m/s'
                % (b['dy'], dur, b['dy'] / dur)) if dur > 0.0 else 'sin duración'
        print('  %-26s %10d %10d  %s'
              % (a['name'], a['end'] - a['start'], b['end'] - b['start'], raiz))

    out = bytearray(data_d[:anims_d[0]['start']])
    for k, a in enumerate(anims_d):
        b = nuevo_por_indice.get(k)
        out += data_s[b['start']:b['end']] if b is not None else data_d[a['start']:a['end']]
    out += data_d[meta_d['end']:]

    delta = len(out) - len(data_d)
    ankp_d = struct.unpack_from('<I', data_d, ANPK_SIZE_OFF)[0]
    ankp_n = ankp_d + delta
    if ankp_n < 0 or ankp_n > 0xFFFFFFFF:
        return abort('el tamaño declarado del ANPK no cuadra (%d)' % ankp_n)
    struct.pack_into('<I', out, ANPK_SIZE_OFF, ankp_n)
    print('# bytes %d -> %d (delta %+d); ANPK size %d -> %d'
          % (len(data_d), len(out), delta, ankp_d, ankp_n))

    fallos = verifica_reemplazo(bytes(out), data_d, meta_d, anims_d, data_s, reemplazos)
    if fallos:
        return abort('el resultado en memoria no cuadra: %s' % '; '.join(fallos))

    if not aplicar:
        print('# SIMULACRO: no se ha escrito nada (ni copia). Repite con --aplicar '
              'para escribir.')
        print('OK')
        return 0

    shutil.copy2(dst, copia)
    with open(dst, 'wb') as f:
        f.write(out)
    print('# escrito %s (%d bytes); copia exclusiva del original en %s'
          % (dst, len(out), copia))
    data_v, meta_v, anims_v = ifp_inspect.parse_ifp(dst)
    print('# verificación: %d animaciones, bloque %s, cola %d'
          % (meta_v['num_anims'], meta_v['block_name'], meta_v['tail']))
    for k, b in reemplazos:
        v = anims_v[k]
        dur = v['dur']
        print('  %-26s %+.3f m / %.3f s = %+.2f m/s'
              % (v['name'], v['dy'], dur, (v['dy'] / dur) if dur > 0.0 else 0.0))
    print('# revertir: cp %s %s' % (copia, dst))
    print('OK')
    return 0


def main():
    args = list(sys.argv[1:])
    if not args or args[0] in ('-h', '--help'):
        print(__doc__)
        return 1
    aplicar = '--aplicar' in args
    args = [a for a in args if a != '--aplicar']
    reemplazar = '--reemplazar' in args
    args = [a for a in args if a != '--reemplazar']
    copia = None
    if '--copia' in args:
        j = args.index('--copia')
        if j + 1 >= len(args):
            return abort('falta la ruta de --copia')
        copia = args[j + 1]
        args = args[:j] + args[j + 2:]
    if reemplazar and copia is None:
        return abort('--reemplazar exige --copia <ruta>: copia exclusiva fuera de '
                     'las raíces servidas (streamed/, bootseed/)')
    if copia is not None and not reemplazar:
        return abort('--copia sólo tiene sentido junto a --reemplazar')
    if len(args) < 2 or '--patron' not in args:
        print(__doc__)
        return 1
    i = args.index('--patron')
    if i + 1 >= len(args):
        return abort('falta el patrón')
    patron = args[i + 1]
    resto = args[:i] + args[i + 2:]
    if len(resto) < 2:
        return abort('hacen falta el .ifp destino y el .ifp origen')
    if len(resto) > 2:
        return abort('argumentos de sobra: %s' % ' '.join(resto[2:]))
    dst, src = resto[0], resto[1]

    d = carga(dst, 'destino')
    s = carga(src, 'origen')
    if not d or not s:
        return 1
    data_d, meta_d, anims_d = d
    data_s, meta_s, anims_s = s
    print('# destino %s  bloque=%s  animaciones=%d  bytes=%d'
          % (dst, meta_d['block_name'], meta_d['num_anims'], len(data_d)))
    print('# origen  %s  bloque=%s  animaciones=%d  bytes=%d'
          % (src, meta_s['block_name'], meta_s['num_anims'], len(data_s)))
    if meta_d['block_name'] != meta_s['block_name']:
        return abort('los bloques no son el mismo (%s vs %s): el motor agrupa por '
                     'nombre de bloque' % (meta_d['block_name'], meta_s['block_name']))

    rx = re.compile(patron)
    elegidas = [a for a in anims_s if rx.search(a['name'])]
    if not elegidas:
        return abort('el patrón %r no encuentra ninguna animación en el origen' % patron)

    if reemplazar:
        return ejecuta_reemplazo(dst, copia, data_d, meta_d, anims_d,
                                 data_s, meta_s, anims_s, elegidas, aplicar)

    ya = {a['name'].lower() for a in anims_d}
    for a in elegidas:
        if a['name'].lower() in ya:
            return abort('el destino ya tiene una animación llamada %r' % a['name'])
        if a['keys'] is None or a['dy'] is None:
            return abort('%r no es un clip de movimiento (sin claves de raíz con '
                         'traslación): no se añade' % a['name'])

    insert = b''.join(data_s[a['start']:a['end']] for a in elegidas)
    nuevo = meta_d['num_anims'] + len(elegidas)
    print('# a añadir: %d animación(es) al final (offset %d, %d bytes); '
          'num_anims %d -> %d'
          % (len(elegidas), meta_d['end'], len(insert), meta_d['num_anims'], nuevo))
    print('  %-26s %6s %8s %10s  %s' % ('anim', 'frames', 'bytes', 'sha1', 'raiz'))
    for a in elegidas:
        trozo = data_s[a['start']:a['end']]
        dur = a['dur']
        raiz = ('%+.3f m / %.3f s = %+.2f m/s'
                % (a['dy'], dur, a['dy'] / dur)) if dur > 0.0 else 'sin duración'
        print('  %-26s %6d %8d %10s  %s'
              % (a['name'], a['num_frames'], len(trozo), sha1(trozo), raiz))

    # El contador vive en los bytes VIEJOS, así que se parchea antes de insertar
    # (el offset no se mueve): así lo que sigue es una única operación de bytes.
    out = bytearray(data_d)
    struct.pack_into('<i', out, meta_d['num_anims_off'], nuevo)
    out = bytes(out[:meta_d['end']]) + insert + bytes(out[meta_d['end']:])

    if not aplicar:
        print('# SIMULACRO: no se ha escrito nada. Repite con --aplicar para escribir.')
        print('OK')
        return 0

    bak = dst + '.bak'
    if os.path.exists(bak):
        return abort('ya existe %s: es la copia del original para revertir. Muévelo '
                     'aparte (o bórralo si ya no lo quieres) y repite' % bak)
    shutil.copy2(dst, bak)
    with open(dst, 'wb') as f:
        f.write(out)
    print('# escrito %s (%d bytes); copia del original en %s'
          % (dst, len(out), bak))

    data_v, meta_v, anims_v = ifp_inspect.parse_ifp(dst)
    fallos = []
    if len(data_v) != len(data_d) + len(insert):
        fallos.append('el tamaño no es el esperado (%d != %d)'
                      % (len(data_v), len(data_d) + len(insert)))
    # Lo único que se toca de lo viejo es el contador; cada animación que ya
    # estaba tiene que seguir byte a byte donde estaba.
    for i, b in enumerate(anims_d):
        a = anims_v[i]
        if (a['name'] != b['name'] or a['num_frames'] != b['num_frames']
                or data_v[a['start']:a['end']] != data_d[b['start']:b['end']]):
            fallos.append('la animación #%d que ya estaba (%s) ha cambiado' % (i + 1, b['name']))
            break
    if meta_v['num_anims'] != nuevo:
        fallos.append('num_anims=%d (se esperaba %d)' % (meta_v['num_anims'], nuevo))
    if meta_v['end'] != meta_d['end'] + len(insert):
        fallos.append('el final del bloque no cuadra (%d)' % meta_v['end'])
    nuevas = anims_v[meta_d['num_anims']:]
    if [a['name'] for a in nuevas] != [a['name'] for a in elegidas]:
        fallos.append('las animaciones nuevas no están donde toca: %s'
                      % ', '.join(a['name'] for a in nuevas))
    for a, b in zip(nuevas, elegidas):
        if a['num_frames'] != b['num_frames'] or a['dy'] != b['dy']:
            fallos.append('%s no coincide con su original del origen' % a['name'])
    if fallos:
        print('ABORT: el fichero escrito no pasa la verificación: %s\n'
              '       revertir con: mv %s %s' % ('; '.join(fallos), bak, dst))
        return 1
    print('# verificación: %d animaciones, bytes viejos intactos, raíces:'
          % meta_v['num_anims'])
    for a in nuevas:
        dur = a['dur']
        print('  %-26s %+.3f m / %.3f s = %+.2f m/s'
              % (a['name'], a['dy'], dur, (a['dy'] / dur) if dur > 0.0 else 0.0))
    print('# revertir: mv %s %s' % (bak, dst))
    print('OK')
    return 0


if __name__ == '__main__':
    sys.exit(main())
