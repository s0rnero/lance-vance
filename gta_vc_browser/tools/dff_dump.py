#!/usr/bin/env python3
"""Volcado minimo de un DFF de RenderWare (VC/PC): marcos con su padre, atomics
(modelo -> marco + geometria) y materiales con su textura.

Existe para responder preguntas de datos que el juego no traza: que marcos trae
un coche, de que padre cuelgan y que textura usa cada malla. Sirve para los
intermitentes y las ventanillas (ver plan 13.0).

Soporta el volcado "empaquetado" de RW 3.7 que usan los DFF de Vice Extended
(lista de marcos como struct con los 56 bytes de cada marco y los nombres en
chunks 0x03 aparte).

Uso:
    python tools/dff_dump.py <fichero.dff> [filtro]
El filtro (opcional) es una subcadena: solo imprime marcos/materiales que la
contengan, pero los indices y padres siguen siendo los reales.
"""
import struct
import sys

F_LIB = 0x1003ffff


class Chunk:
    __slots__ = ('type', 'start', 'end', 'off')

    def __init__(self, typ, off, start, end):
        self.type, self.off, self.start, self.end = typ, off, start, end

    def __repr__(self):
        return '<%04x @%d %d>' % (self.type, self.off, self.end - self.start)


def children(buf, start, end):
    p = start
    while p + 12 <= end:
        typ, size, _lib = struct.unpack_from('<III', buf, p)
        if p + 12 + size > end:
            return
        yield Chunk(typ, p, p + 12, p + 12 + size)
        p += 12 + size


def first(buf, parent, typ):
    for c in children(buf, parent.start, parent.end):
        if c.type == typ:
            return c
    return None


def u32(buf, p):
    return struct.unpack_from('<I', buf, p)[0]


def cstr(buf, start, end):
    raw = buf[start:end]
    return raw.split(b'\0')[0].decode('latin-1')


def parse_frames(buf, fl):
    st = first(buf, fl, 0x01)
    if st is None:
        return []
    n = u32(buf, st.start)
    recs = st.start + 4
    names = []
    for c in children(buf, fl.start, fl.end):
        if c.type != 0x03:
            continue
        ext = first(buf, c, 0x253F2FE)
        names.append(cstr(buf, ext.start, ext.end) if ext is not None else '')
    frames = []
    for i in range(n):
        base = recs + i * 56
        rot = struct.unpack_from('<9f', buf, base)
        pos = struct.unpack_from('<3f', buf, base + 36)
        parent = struct.unpack_from('<i', buf, base + 48)[0]
        flags = u32(buf, base + 52)
        frames.append({'i': i, 'name': names[i] if i < len(names) else '?%d' % i,
                       'parent': parent, 'pos': pos, 'flags': flags, 'rot': rot})
    return frames


def parse_materials(buf, geom):
    mats = []
    ml = first(buf, geom, 0x08)
    if ml is None:
        return mats
    for mat in children(buf, ml.start, ml.end):
        if mat.type != 0x07:
            continue
        st = first(buf, mat, 0x01)
        color = struct.unpack_from('<4B', buf, st.start + 4) if st else None
        tex = None
        tx = first(buf, mat, 0x06)
        if tx is not None:
            name = first(buf, tx, 0x02)
            if name is not None:
                tex = cstr(buf, name.start, name.end)
        mats.append((color, tex))
    return mats


def parse(buf):
    clump = None
    for c in children(buf, 0, len(buf)):
        if c.type == 0x10:
            clump = c
            break
    if clump is None:
        raise SystemExit('no encuentro el clump (0x10)')

    frames, geoms, atomics = [], [], []
    for c in children(buf, clump.start, clump.end):
        if c.type == 0x0E:
            frames = parse_frames(buf, c)
        elif c.type in (0x1A, 0x0F):
            for g in children(buf, c.start, c.end):
                if g.type == 0x0F:
                    geoms.append(parse_materials(buf, g))
        elif c.type == 0x14:
            st = first(buf, c, 0x01)
            if st is not None:
                atomics.append(struct.unpack_from('<2I', buf, st.start))
    return frames, geoms, atomics


def main():
    path = sys.argv[1]
    filtro = sys.argv[2].lower() if len(sys.argv) > 2 else None
    frames, geoms, atomics = parse(open(path, 'rb').read())
    byframe = {}
    for fi, gi in atomics:
        byframe.setdefault(fi, []).append(gi)
    print('%s: %d marcos, %d atomics, %d geometrias' % (path, len(frames), len(atomics), len(geoms)))
    for f in frames:
        pname = frames[f['parent']]['name'] if 0 <= f['parent'] < len(frames) else '-'
        line = '%3d %-24s parent=%-24s pos=(%.2f, %.2f, %.2f)' % (
            f['i'], f['name'], pname, f['pos'][0], f['pos'][1], f['pos'][2])
        gis = byframe.get(f['i'], [])
        if gis:
            line += '  atomics=' + ','.join(str(g) for g in gis)
        if filtro and filtro in f['name'].lower():
            r = f['rot']
            print('        ejes x=%s y=%s z=%s' % (
                tuple(round(v, 3) for v in r[0:3]),
                tuple(round(v, 3) for v in r[3:6]),
                tuple(round(v, 3) for v in r[6:9])))
        if filtro and filtro not in f['name'].lower():
            if not gis:
                continue
        print(line)
        for gi in gis:
            for color, tex in geoms[gi]:
                print('        geo %-3d color=%-18s tex=%s' % (gi, color, tex))


if __name__ == '__main__':
    main()
