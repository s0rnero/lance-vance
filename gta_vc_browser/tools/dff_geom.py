#!/usr/bin/env python3
"""Lee la GEOMETRIA de un DFF de RenderWare (VC/PC): marcos, atomics, materiales,
vertices y triangulos.

Existe para el trabajo del volante: el mod (Vice Extended) trae 18 coches con el
volante como objeto aparte ("steeringwheel", textura "stwheel"); en los demas el
volante esta fundido en el atomic del salpicadero (textura "vehicledash32"). Para
poder separarlo hay que saber donde esta la rueda: estos son sus vertices.

Formato (RW 3.7, el mismo que lee librw en vendor/librw/src/geometry.cpp):
  chunk GEOMETRY (0x0F)
    chunk STRUCT (0x01): flags(u32) numTriangles(i32) numVertices(i32) numMorph(i32)
        [si version < 0x34000: 12 bytes de surface props]
        [si PRELIT: 4 bytes por vertice]
        [por cada juego de uv: 8 bytes por vertice]
        triangulos: 8 bytes cada uno  (v0=hi16(t0) v1=lo16(t0) v2=hi16(t1) mat=lo16(t1))
        por cada morph target: esfera(16) hasVertices(i32) hasNormals(i32)
                              vertices(12 por vertice) normals(12 por vertice)
    chunk MATLIST (0x08): los materiales

Uso:
    python tools/dff_geom.py <fichero.dff>                 # resumen por atomic
    python tools/dff_geom.py <fichero.dff> --geom <tex>    # vuelca esa geometria
    python tools/dff_geom.py <fichero.dff> --ring           # busca anillos (el volante)
"""
import struct
import sys

F_LIB = 0x1003ffff
PRELIT = 0x08
NATIVE = 0x01000000


class Chunk:
    __slots__ = ('type', 'start', 'end', 'off', 'ver')

    def __init__(self, typ, off, start, end, ver):
        self.type, self.off, self.start, self.end, self.ver = typ, off, start, end, ver


def children(buf, start, end):
    p = start
    while p + 12 <= end:
        typ, size, ver = struct.unpack_from('<III', buf, p)
        if p + 12 + size > end:
            return
        yield Chunk(typ, p, p + 12, p + 12 + size, ver)
        p += 12 + size


def first(buf, parent, typ):
    for c in children(buf, parent.start, parent.end):
        if c.type == typ:
            return c
    return None


def u32(buf, p):
    return struct.unpack_from('<I', buf, p)[0]


def cstr(buf, start, end):
    return buf[start:end].split(b'\0')[0].decode('latin-1')


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


def parse_geometry(buf, geom):
    """Devuelve (mats, verts, tris, uvs, norms, flags, colors) con verts como
    lista de (x,y,z) y tris como (v0, v1, v2, matId).

    Ojo: hay dos formatos de DFF. El de los ficheros del mod ("empaquetado") no
    lleva las 12 bytes de "surface props"; el de los DFF vanilla de PC (formato
    "ext", version de chunk 0xC02FFFF) si. Se decide comprobando que el tamano
    calculado cuadre con el del chunk (ver librw/src/geometry.cpp)."""
    mats = parse_materials(buf, geom)
    st = first(buf, geom, 0x01)
    if st is None:
        return mats, [], [], [], [], 0, []
    p = st.start
    flags, ntris, nverts, nmorph = struct.unpack_from('<Iiii', buf, p)
    ntex = (flags >> 16) & 0xFFFF
    if flags & NATIVE:
        return mats, [], [], [], [], flags, []

    def tamano(extra):
        t = 16 + extra
        if flags & PRELIT:
            t += 4 * nverts
        t += 8 * ntex * nverts
        t += 8 * ntris
        t += 16 + 8 + 12 * nverts * 2 * max(1, nmorph)
        return t

    real = st.end - st.start
    extra = 0
    if tamano(0) != real:
        if tamano(12) == real:
            extra = 12
        else:
            # Ultimo recurso: los 12 bytes de las surface props.
            extra = 12

    p += 16 + extra
    colors = []
    if flags & PRELIT:
        colors = [struct.unpack_from('<4B', buf, p + 4 * i) for i in range(nverts)]
        p += 4 * nverts
    uvs = []
    for _ in range(ntex):
        vals = struct.unpack_from('<%df' % (2 * nverts), buf, p)
        uvs.append([(vals[i * 2], vals[i * 2 + 1]) for i in range(nverts)])
        p += 8 * nverts
    tris = []
    for _ in range(ntris):
        t0, t1 = struct.unpack_from('<II', buf, p)
        p += 8
        tris.append((t0 >> 16, t0 & 0xFFFF, t1 >> 16, t1 & 0xFFFF))
    verts = []
    norms = []
    for _ in range(max(1, nmorph)):
        p += 16                      # esfera envolvente
        hasv, hasn = struct.unpack_from('<ii', buf, p)
        p += 8
        if hasv:
            vs = struct.unpack_from('<%df' % (3 * nverts), buf, p)
            verts = [vs[i * 3:i * 3 + 3] for i in range(nverts)]
            p += 12 * nverts
        if hasn:
            ns = struct.unpack_from('<%df' % (3 * nverts), buf, p)
            norms = [ns[i * 3:i * 3 + 3] for i in range(nverts)]
            p += 12 * nverts
    return mats, verts, tris, uvs, norms, flags, colors


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
        elif c.type == 0x1A:
            # La primera mitad de las geometrias va dentro del chunk de la lista.
            for g in children(buf, c.start, c.end):
                if g.type == 0x0F:
                    mats, verts, tris, uvs, norms, flags, colors = parse_geometry(buf, g)
                    geoms.append({'mats': mats, 'verts': verts, 'tris': tris, 'uvs': uvs,
                                  'norms': norms, 'flags': flags, 'colors': colors})
        elif c.type == 0x0F:
            # La segunda mitad va suelta en el clump (ver GeometryListStreamRead2
            # del port: src/rw/ClumpRead.cpp). El indice del atomic es sobre la
            # lista completa, asi que el orden importa.
            mats, verts, tris, uvs, norms, flags, colors = parse_geometry(buf, c)
            geoms.append({'mats': mats, 'verts': verts, 'tris': tris, 'uvs': uvs,
                          'norms': norms, 'flags': flags, 'colors': colors})
        elif c.type == 0x14:
            st = first(buf, c, 0x01)
            if st is not None:
                atomics.append(struct.unpack_from('<2I', buf, st.start))
    return frames, geoms, atomics


def frame_matrix(f):
    """Matriz 3x3 (filas) y posicion del marco, tal y como esta en el DFF."""
    r = f['rot']
    return (r[0], r[1], r[2]), (r[3], r[4], r[5]), (r[6], r[7], r[8]), f['pos']


def world_of(frames, idx):
    """Matriz 4x4 (filas) del marco idx en el espacio del raiz."""
    m = [[1.0, 0.0, 0.0, 0.0], [0.0, 1.0, 0.0, 0.0], [0.0, 0.0, 1.0, 0.0], [0.0, 0.0, 0.0, 1.0]]
    chain = []
    i = idx
    while 0 <= i < len(frames):
        chain.append(i)
        i = frames[i]['parent']
    for fi in reversed(chain):
        f = frames[fi]
        r0, r1, r2, p = frame_matrix(f)
        loc = [[r0[0], r0[1], r0[2], p[0]],
               [r1[0], r1[1], r1[2], p[1]],
               [r2[0], r2[1], r2[2], p[2]],
               [0.0, 0.0, 0.0, 1.0]]
        m = [[sum(m[a][k] * loc[k][b] for k in range(4)) for b in range(4)] for a in range(4)]
    return m


def mul(m, v):
    return [sum(m[a][k] * v[k] for k in range(3)) for a in range(3)]


def main():
    args = sys.argv[1:]
    path = args[0]
    buf = open(path, 'rb').read()
    frames, geoms, atomics = parse(buf)
    byframe = {}
    for fi, gi in atomics:
        byframe.setdefault(fi, []).append(gi)
    print('%s: %d marcos, %d atomics, %d geometrias' % (path, len(frames), len(atomics), len(geoms)))

    if '--geom' in args:
        want = args[args.index('--geom') + 1]
        for fi, gis in byframe.items():
            for gi in gis:
                g = geoms[gi]
                for mi, (color, tex) in enumerate(g['mats']):
                    if tex == want:
                        print('marco %d (%s) geo %d material %d tex %s: %d verts, %d tris' %
                              (fi, frames[fi]['name'], gi, mi, tex, len(g['verts']), len(g['tris'])))
        return

    if '--ring' in args:
        # Busca anillos: triangulos cuyos 3 vertices estan a una distancia parecida
        # de un centro (rueda de ~0.2 m) y casi coplanares.
        for fi, gis in byframe.items():
            for gi in gis:
                g = geoms[gi]
                for mi, (color, tex) in enumerate(g['mats']):
                    if not tex or 'dash' not in tex.lower():
                        continue
                    vs, ts = g['verts'], g['tris']
                    print('  marco %d (%s) geo %d tex %s: %d verts %d tris (material %d)' %
                          (fi, frames[fi]['name'], gi, tex, len(vs), len(ts), mi))
        return

    if '--alpha' in args:
        # El alfa de vertice (PRELIT) decide que se descarta con el alpha test.
        for fi, gis in byframe.items():
            for gi in gis:
                g = geoms[gi]
                colors = g.get('colors') or []
                if not colors:
                    continue
                print('%3d %-22s geo=%d flags=0x%08X verts=%d tris=%d' %
                      (fi, frames[fi]['name'], gi, g.get('flags', 0), len(g['verts']), len(g['tris'])))
                for mi, (color, tex) in enumerate(g['mats']):
                    idx = set()
                    ntris = 0
                    for t in g['tris']:
                        if t[3] == mi:
                            ntris += 1
                            idx.update([t[0], t[1], t[2]])
                    if not ntris:
                        continue
                    alphas = sorted(set(colors[i][3] for i in idx if i < len(colors)))
                    nlow = sum(1 for t in g['tris']
                               if t[3] == mi and any(colors[v][3] != 255 for v in t[:3] if v < len(colors)))
                    print('        mat %-2d tris=%-5d tex=%-22s color=%-20s vertex_alpha=%s tris_con_alfa_bajo=%d' %
                          (mi, ntris, tex, color, alphas, nlow))
        return

    for fi, gis in byframe.items():
        f = frames[fi]
        pname = frames[f['parent']]['name'] if 0 <= f['parent'] < len(frames) else '-'
        for gi in gis:
            g = geoms[gi]
            print('%3d %-22s parent=%-22s pos=(%.2f,%.2f,%.2f) geo=%d verts=%d tris=%d' %
                  (fi, f['name'], pname, f['pos'][0], f['pos'][1], f['pos'][2], gi,
                   len(g['verts']), len(g['tris'])))
            for mi, (color, tex) in enumerate(g['mats']):
                print('        mat %-2d color=%-20s tex=%s' % (mi, color, tex))


if __name__ == '__main__':
    main()
