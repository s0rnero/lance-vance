#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Mide el esqueleto de un DFF: cuantos huesos tiene la skin, cuantos nodos
tiene la jerarquia de animacion y si las dos cosas cuadran.

Por que existe: el pipeline de skin del motor (librw `gl3skin.cpp`,
`uploadSkinMatrices`) exige que `skin->numBones == hier->numNodes`. Si no,
las matrices de hueso que se suben al shader son de otro ped (o hay basura
memoria) y la malla del ped sale hecha ovillo o estirada como una bandera.

El DFF de RenderWare 3.7 de los peds lleva:
  - CLUMP (0x10)
      - FRAMELIST (0x0E): 0x01 = numNodes + un registro de 56 bytes por marco,
        y un 0x03 por marco (el primero con la jerarquia, tipo 0x11E).
      - GEOMETRY (0x1A -> 0x0F)
          - 0x03 (extension) -> 0x116 = SKIN (plugin de la skin)
  La cabecera de la skin son 4 bytes: numBones, numUsedBones, numWeights y
  uno sin usar. La jerarquia (0x11E) empieza con la version 0x100, el id, el
  numero de nodos, los flags y el tamano de clave; luego, un id / indice /
  flags por nodo.

Uso:
    python gta_vc_browser/tools/dff_skin.py <fichero.dff> [mas.dff ...]
"""

import struct
import sys


def children(buf, start, end):
    p = start
    while p + 12 <= end:
        typ, size, _lib = struct.unpack_from('<III', buf, p)
        if p + 12 + size > end:
            return
        yield typ, p, p + 12, p + 12 + size
        p += 12 + size


def walk(buf, start, end, out):
    for typ, _p, s, e in children(buf, start, end):
        out.append((typ, s, e))
        walk(buf, s, e, out)


def skin_info(chunks):
    for typ, s, e in chunks:
        if typ == 0x116:
            return buf[ s], buf[s + 1], buf[s + 2]
    return None


def hierarchy_ids(chunks):
    for typ, s, e in chunks:
        if typ == 0x11E and e - s >= 20:
            ver = struct.unpack_from('<i', buf, s)[0]
            if ver == 0x100:
                nnodes = struct.unpack_from('<i', buf, s + 8)[0]
                ids = []
                for i in range(nnodes):
                    nid, _idx, _fl = struct.unpack_from('<3i', buf, s + 20 + i * 12)
                    ids.append(nid)
                return ids
    return None


def analyse(path):
    global buf
    buf = open(path, 'rb').read()
    top = []
    walk(buf, 0, len(buf), top)
    clump = None
    for typ, s, e in top:
        if typ == 0x10:
            clump = (s, e)
            break
    if clump is None:
        print('%s: no es un DFF con clump' % path)
        return
    inner = []
    walk(buf, clump[0], clump[1], inner)
    skin = skin_info(inner)
    ids = hierarchy_ids(inner)
    nb = skin[0] if skin else None
    nu = skin[1] if skin else None
    nw = skin[2] if skin else None
    nn = len(ids) if ids else None
    name = path.replace('\\', '/').split('/')[-1]
    print('%-14s skin: numBones=%s numUsed=%s numWeights=%s | jerarquia: nodos=%s' %
          (name, nb, nu, nw, nn))
    if ids:
        print('               ids=%s' % (ids,))
    if nb is not None and nn is not None and nb != nn:
        print('   !! numBones (%d) != nodos de la jerarquia (%d)' % (nb, nn))
    return nb, nn, ids


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return 1
    for a in args:
        analyse(a)
    return 0


if __name__ == '__main__':
    sys.exit(main())
