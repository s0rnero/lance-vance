#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Inspecciona un .ifp (formato ANPK de VC, el mismo que lee
`CAnimManager::LoadAnimFile`) y dice, por animación:

  - cuántas secuencias (huesos) trae,
  - si la RAÍZ (primera secuencia, el nodo 0 del `CAnimBlendAssociation`)
    tiene claves de TRASLACIÓN (`KRT0`/`KRTS` frente a `KR00`),
  - cuánto se desplaza esa raíz de la primera a la última clave (en metros).

Por qué existe: en III/VC el ped NO se mueve por `m_fMoveSpeed`; se mueve por
la traslación de la RAÍZ de la animación de movimiento, que
`AnimBlendFrameData::VELOCITY_EXTRACTION` convierte en `m_vecAnimMoveDelta`
(FrameUpdate.cpp) -> `CPed::CalculateNewVelocity` -> `m_moved` ->
`CPed::UpdatePosition` -> `m_vecMoveSpeed`. Si el clip que lleva el cuerpo no
tiene esa traslación, el personaje se queda clavado aunque el mando esté a
tope. Esta herramienta dice cuál de los clips del `ped.ifp` la tiene.

Uso (desde la raíz del repo):
    python gta_vc_browser/tools/ifp_inspect.py <fichero.ifp> [patrón]
    python gta_vc_browser/tools/ifp_inspect.py gta_vc_browser/streamed/anim/ped.ifp "crouch|swim|walk"

El recorrido vive en `parse_ifp()` y lo reutiliza `ifp_add.py` (escritor del
mismo formato): un solo recorrido para leer y para escribir, con los mismos
quirks del cargador.
"""

import os
import re
import struct
import sys


CURVA = False   # --curva: volcar la curva de avance de la raíz de cada clip
SEQ_DUMP = False   # --seqs: volcar las secuencias (hueso/tipo/duración) de cada clip


def round4(x):
    return x + (4 - (x & 3) if x & 3 else 0)


class Reader:
    def __init__(self, data):
        self.d = data
        self.o = 0

    def u32(self):
        v = struct.unpack_from('<I', self.d, self.o)[0]
        self.o += 4
        return v

    def i32(self):
        v = struct.unpack_from('<i', self.d, self.o)[0]
        self.o += 4
        return v

    def f32(self):
        v = struct.unpack_from('<f', self.d, self.o)[0]
        self.o += 4
        return v

    def block(self):
        """Lee {char[4] ident, uint32 size} y devuelve (ident, size)."""
        ident = self.d[self.o:self.o + 4].decode('latin-1')
        self.o += 4
        return ident, self.u32()

    def skip(self, n):
        self.o += n


def read_anim_block(r, size):
    """Datos de cabecera de una animación (bloque NAME)."""
    start = r.o
    name = r.d[start:start + 24].split(b'\0')[0].decode('latin-1')
    num_frames = struct.unpack_from('<i', r.d, start + 28)[0]
    bone_tag = struct.unpack_from('<i', r.d, start + 40)[0] if size >= 44 else None
    r.skip(size)
    return name, num_frames, bone_tag


def parse_ifp(path):
    """Recorre el .ifp replicando, paso a paso, lo que hace el cargador.

    Devuelve (data, meta, anims):
      data  = bytes del fichero,
      meta  = {block_name, num_anims, num_anims_off, end, tail},
      anims = una entrada por animación, en el orden del fichero:
              {name, num_seqs, num_frames, keys, dx, dy, dur, root_first,
               root_last, ys, start, end}
              `start`/`end` son los offsets del bloque NAME (cabecera incluida)
              y del final de sus claves: es el rango que el cargador lee para
              esa animación, y lo que hay que copiar para llevarla a otro .ifp.
    """
    data = bytes(path) if isinstance(path, (bytes, bytearray)) else open(path, 'rb').read()
    r = Reader(data)

    ident, size = r.block()
    assert ident == 'ANPK', 'no es un .ifp ANPK: %r' % ident
    size = round4(size)
    _ = size

    ident, info_size = r.block()
    assert ident == 'INFO', ident
    num_anims_off = r.o                     # aquí vive el contador de animaciones
    num_anims = struct.unpack_from('<i', data, r.o)[0]
    block_name = data[r.o + 4:r.o + 4 + 24].split(b'\0')[0].decode('latin-1')
    r.skip(round4(info_size))

    anims = []
    for a in range(num_anims):
        start = r.o
        ident, name_size = r.block()
        anim_name = data[r.o:r.o + name_size].split(b'\0')[0].decode('latin-1')
        r.skip(round4(name_size))

        # DGAN: el cargador del motor NO salta su payload (el `ROUNDSIZE`
        # interno vuelve a usar `dgan.size` y no consume nada): el bloque INFO
        # anidado va justo detrás y el payload de DGAN es el resto del DG. Se
        # replica tal cual.
        ident, _dgan_size = r.block()         # DGAN (size sin usar)
        ident, info2_size = r.block()         # INFO anidado
        num_seqs = struct.unpack_from('<i', data, r.o)[0]
        r.skip(round4(info2_size))

        root_keys = None
        root_ys = None
        root_first = root_last = None
        root_x = None
        root_time = None
        total_time = 0.0
        total_frames = 0
        seqs = []
        for s in range(num_seqs):
            # Ojo: el cargador del motor lee la cabecera de CPAN y NO salta su
            # relleno (en `LoadAnimFile` el ROUNDSIZE de CPAN usa por error
            # `dgan.size`). Se replica tal cual o el recorrido se desalinea.
            seq_start = r.o
            ident, _cpan_size = r.block()     # CPAN
            ident, anim_size = r.block()      # ANIM
            seq_name, num_frames, _bone = read_anim_block(r, round4(anim_size))
            if s == 0:
                total_frames = num_frames
            if num_frames == 0:
                seqs.append({'name': seq_name, 'num_frames': 0, 'bone_tag': _bone,
                             'keys': None, 'dur': 0.0, 'start': seq_start, 'end': r.o})
                continue
            ktype, ksize = r.block()
            if ktype == 'KRTS':
                step, has_trans, floats, t_idx = 0x2C, True, 11, 10
            elif ktype == 'KRT0':
                step, has_trans, floats, t_idx = 0x20, True, 8, 7
            elif ktype == 'KR00':
                step, has_trans, floats, t_idx = 0x14, False, 5, 4
            else:
                raise SystemExit('clave rara %r en %s' % (ktype, anim_name))
            last_t = struct.unpack_from('<f', data, r.o + step * (num_frames - 1) + t_idx * 4)[0]
            if last_t > total_time:
                total_time = last_t
            if s == 0:
                root_keys = ktype
                root_time = last_t
                if has_trans:
                    vals = struct.unpack_from('<%df' % floats, data, r.o)
                    root_first = vals[4:7]      # x, y, z de la primera clave
                    off = r.o + step * (num_frames - 1)
                    vals = struct.unpack_from('<%df' % floats, data, off)
                    root_last = vals[4:7]
                    # La CURVA completa de la raíz (y, el avance): dice si el
                    # clip AVANZA de forma monótona (arrastra los pies) y en
                    # cuántos «pasos» (mesetas) reparte ese avance.
                    root_ys = [struct.unpack_from('<f', data,
                        r.o + step * k + 5 * 4)[0] for k in range(num_frames)]
            seqs.append({'name': seq_name, 'num_frames': num_frames, 'bone_tag': _bone,
                         'keys': ktype, 'dur': last_t, 'start': seq_start, 'end': r.o + step * num_frames})
            r.skip(step * num_frames)

        dx = dy = None
        if root_first is not None and root_last is not None:
            dx = root_last[0] - root_first[0]
            dy = root_last[1] - root_first[1]
        anims.append({'name': anim_name, 'num_seqs': num_seqs,
                      'num_frames': total_frames, 'keys': root_keys,
                      'dx': dx, 'dy': dy, 'dur': total_time,
                      'root_first': root_first, 'root_last': root_last,
                      'ys': root_ys, 'start': start, 'end': r.o, 'seqs': seqs})

    return data, {'block_name': block_name, 'num_anims': num_anims,
                  'num_anims_off': num_anims_off, 'end': r.o,
                  'tail': len(data) - r.o}, anims


def inspect(path, pattern=None):
    data, meta, anims = parse_ifp(path)
    rx = re.compile(pattern, re.I) if pattern else None
    print('# %s  bloque=%s  animaciones=%d' % (os.path.basename(path), meta['block_name'], meta['num_anims']))
    print('# %-26s %5s %5s  %-5s %8s %8s  %8s  %9s  %s' % (
        'anim', 'seqs', 'frm', 'claves', 'raiz_dx', 'raiz_dy', 'duracion', 'velocidad', 'raiz_ini'))

    for a in anims:
        if rx and not rx.search(a['name']):
            continue
        if a['keys'] is None:
            continue
        dur = a['dur']
        if a['dx'] is not None:
            sx = '%8.3f' % a['dx']
            sy = '%8.3f' % a['dy']
            # Velocidad que el motor aplica de verdad: el desplazamiento de la
            # raíz de la primera a la última clave repartido por la duración
            # (es lo que sale del `UpdatePosition`, en m/s).
            spd = ('%6.2f m/s' % (a['dy'] / dur)) if dur > 0.0 else '-'
        else:
            sx = sy = '%8s' % '-'
            spd = '-'
        print('  %-26s %5d %5d  %-5s %8s %8s  %6.3fs  %s  %s' % (
            a['name'], a['num_seqs'], a['num_frames'], a['keys'], sx, sy, dur, spd,
            ('%.3f,%.3f' % (a['root_first'][0], a['root_first'][1])) if a['root_first'] else ''))
        if SEQ_DUMP and a.get('seqs'):
            for s, sq in enumerate(a['seqs']):
                print('        seq%-2d %-24s hueso=%-4s claves=%-4s frm=%4d dur=%.3fs' % (
                    s, sq['name'], sq['bone_tag'] if sq['bone_tag'] is not None else '-',
                    sq['keys'] if sq['keys'] else '-', sq['num_frames'], sq['dur']))
        if CURVA and a['ys']:
            # dy de cada clave: pendiente constante = avance continuo (los pies
            # patinan si el cuerpo va a otra velocidad); mesetas = pasos.
            d = [a['ys'][i + 1] - a['ys'][i] for i in range(len(a['ys']) - 1)]
            print('        dy/clave: %s' % ' '.join('%+.3f' % v for v in d))
            print('        total=%.3f m en %d claves (%.3f s)' % (
                a['ys'][-1] - a['ys'][0], len(a['ys']), dur))


def main():
    global CURVA, SEQ_DUMP
    args = [a for a in sys.argv[1:]]
    if not args:
        print(__doc__)
        return 1
    while '--curva' in args:
        CURVA = True
        args.remove('--curva')
    while '--seqs' in args:
        SEQ_DUMP = True
        args.remove('--seqs')
    path = args[0]
    pattern = args[1] if len(args) > 1 else None
    inspect(path, pattern)
    return 0


if __name__ == '__main__':
    sys.exit(main())
