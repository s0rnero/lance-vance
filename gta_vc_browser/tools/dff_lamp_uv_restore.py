#!/usr/bin/env python3
"""Devuelve a las piezas de faro/piloto la UV del DFF del PC.

Motivo medido (plan 92.0): en el arbol servido hay piezas de lampara cuya UV es
la del DFF de retail con la V espejada (`v -> 1-v`). En las lentes eso las lleva
de la zona roja/blanca del atlas a una zona negra del mismo atlas: el faro se ve
negro. El atlas del TXD es byte a byte el de retail, asi que la diferencia esta
en las UV del DFF, no en la textura.

La herramienta no inventa nada: empareja triangulos por la posicion de sus tres
vertices con el DFF de referencia (el del PC, `tmp/base421`) y escribe la UV de
referencia. Solo cambia los flotantes de la UV; vertices, triangulos,
materiales, normales y prelit quedan intactos.

La restauracion es **todo o nada por pieza de lampara**. El primer intento
(92.0-T1) escribia vertice a vertice y dejaba piezas a medias: el vertice
compartido con una cara que no se restauraba quedaba con la UV espejada y el
triangulo interpolaba entre la zona roja y la negra, que es el corte en
triangulos negros que se vio en los pilotos del cheetah (64 caras mezcladas) y
de otros 17 modelos. Ahora cada atomic `*light*`/`indicator` se escribe entero
o no se toca, y solo se toca si **todas** sus caras existen en el DFF del PC:
una pieza con caras que el trasplante anadio (reveses de lente) se deja como
esta, porque su UV no tiene referencia con la que compararse.

Solo se restaura cuando la texela de referencia no es mas oscura que la que
sirve hoy (`--all` restaura todas, sin esa puerta).

Uso:
    python tools/dff_lamp_uv_restore.py --check
    python tools/dff_lamp_uv_restore.py --models pheonix --apply --out tmp/base92/fix
"""
import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import dff_geom as dg
import txd_raster as tr

PRELIT = 0x08
TOL = 0.001
LUM_TOL = 2.0


def geom_chunks(buf):
    """Chunks GEOMETRY en el mismo orden que dg.parse()."""
    clump = None
    for c in dg.children(buf, 0, len(buf)):
        if c.type == 0x10:
            clump = c
            break
    if clump is None:
        return []
    out = []
    for c in dg.children(buf, clump.start, clump.end):
        if c.type == 0x1A:
            out.extend(g for g in dg.children(buf, c.start, c.end) if g.type == 0x0F)
        elif c.type == 0x0F:
            out.append(c)
    return out


def uv_offset(buf, geom):
    """Offset del primer juego de UV y numero de vertices."""
    st = dg.first(buf, geom, 0x01)
    if st is None:
        return None, 0, 0
    flags, ntris, nverts, nmorph = struct.unpack_from('<Iiii', buf, st.start)
    ntex = (flags >> 16) & 0xFFFF

    def tamano(extra):
        t = 16 + extra
        if flags & PRELIT:
            t += 4 * nverts
        t += 8 * ntex * nverts + 8 * ntris
        t += 16 + 8 + 12 * nverts * 2 * max(1, nmorph)
        return t

    real = st.end - st.start
    extra = 0 if tamano(0) == real else 12
    p = st.start + 16 + extra
    if flags & PRELIT:
        p += 4 * nverts
    return (p if ntex else None), nverts, ntex


def faces(path):
    """{(posiciones): [(geom, idx_tri, {vertice: (u,v)}, textura)]} de un DFF."""
    buf = open(path, 'rb').read()
    chunks = geom_chunks(buf)
    out = {}
    for gi, ch in enumerate(chunks):
        mats, verts, tris, uvs, norms, flags, colors = dg.parse_geometry(buf, ch)
        if not uvs or not verts:
            continue
        for ti, t in enumerate(tris):
            corners = {}
            for k in range(3):
                p = tuple(round(c, 2) for c in verts[t[k]])
                corners[p] = uvs[0][t[k]]
            if len(corners) != 3:
                continue
            tex = mats[t[3]][1] if t[3] < len(mats) else None
            out.setdefault(tuple(sorted(corners)), []).append((gi, ti, corners, tex))
    return buf, chunks, out


def texdict(txd):
    data = open(txd, 'rb').read()
    out = {}
    for pos in tr.dict_textures(data):
        d = tr.read_native(data, pos)
        size, off = d['levels'][0]
        try:
            px = tr.decode(data[off:off + size], d['w'], d['h'], d['depth'],
                           d['fmt'], d['has_alpha'], d.get('compression', 0))
        except Exception:
            px = None
        if px is not None:
            out[d['nombre'].lower()] = (d, px)
    return out


def lum(c):
    return (c[0] * 30 + c[1] * 59 + c[2] * 11) // 100


def sample(tex, name, uv):
    it = tex.get((name or '').lower())
    if it is None:
        return None
    d, px = it
    x = int((uv[0] % 1.0) * d['w']) % d['w']
    y = int((uv[1] % 1.0) * d['h']) % d['h']
    return lum(px[y * d['w'] + x])


def near(a, b):
    return abs(a[0] - b[0]) + abs(a[1] - b[1]) <= TOL


def mirror(uv):
    return (uv[0], 1.0 - uv[1])


def lamp_geoms(buf):
    """Indices de geometria de lampara segun el nombre del marco del atomic."""
    frames, geoms, atomics = dg.parse(buf)
    out = set()
    for fi, gi in atomics:
        if fi >= len(frames):
            continue
        n = frames[fi]['name'].lower()
        if 'light' in n or 'indicator' in n:
            out.add(gi)
    return out


def plan_model(base, served, txd, restore_all=False):
    """Devuelve (escrituras, informe) para un modelo."""
    rbuf, rchunks, rfaces = faces(base)
    sbuf, schunks, sfaces = faces(served)
    tex = texdict(txd) if os.path.exists(txd) else {}
    lamps = lamp_geoms(sbuf)
    info = {'geoms': 0, 'restored': 0, 'reverted': 0, 'blocked': 0,
            'darker': 0, 'same': 0, 'detail': []}
    edits = []
    for gi, ch in enumerate(schunks):
        if gi not in lamps:
            continue
        off, nverts, ntex = uv_offset(sbuf, ch)
        if off is None:
            continue
        mats, verts, tris, uvs, norms, flags, colors = dg.parse_geometry(sbuf, ch)
        if not uvs or not tris:
            continue
        uv0 = uvs[0]
        pos_of = [tuple(round(c, 2) for c in v) for v in verts]
        kind = []
        refuv = {}
        ambig = False
        for ti, t in enumerate(tris):
            corners = {}
            for k in range(3):
                corners[pos_of[t[k]]] = uv0[t[k]]
            if len(corners) != 3:
                kind.append('nomatch')
                continue
            cands = rfaces.get(tuple(sorted(corners)))
            if not cands:
                kind.append('nomatch')
                continue
            pick = next((c for c in cands if c[0] == gi), cands[0])
            rcorners = pick[2]
            if len(rcorners) != 3 or any(p not in rcorners for p in corners):
                kind.append('nomatch')
                continue
            if all(near(corners[p], rcorners[p]) for p in corners):
                kind.append('ref')
            elif all(near(corners[p], mirror(rcorners[p])) for p in corners):
                kind.append('mirror')
            else:
                kind.append('other')
            for p, uv in rcorners.items():
                prev = refuv.get(p)
                if prev is not None and not near(prev, uv):
                    ambig = True
                refuv[p] = uv
        evid = sum(1 for k in kind if k in ('ref', 'mirror'))
        if not evid:
            continue
        info['geoms'] += 1
        completa = 'nomatch' not in kind and not ambig
        restorable = completa
        if restorable and tex and not restore_all:
            now, ref = [], []
            for ti, t in enumerate(tris):
                for k in range(3):
                    p = pos_of[t[k]]
                    if p not in refuv:
                        continue
                    nx = sample(tex, mats[t[3]][1] if t[3] < len(mats) else None,
                                uv0[t[k]])
                    rx = sample(tex, mats[t[3]][1] if t[3] < len(mats) else None,
                                refuv[p])
                    if nx is not None:
                        now.append(nx)
                    if rx is not None:
                        ref.append(rx)
            if now and ref and sum(ref) / len(ref) + LUM_TOL < sum(now) / len(now):
                restorable = False
                info['darker'] += 1
                info['detail'].append((gi, 'mas oscura', round(sum(now) / len(now)),
                                       round(sum(ref) / len(ref))))
        if not restorable:
            info['detail'].append((gi, 'parcial', len(tris), evid))
        if restorable:
            info['restored'] += 1
            info['detail'].append((gi, 'restaurada', len(tris)))
            for vid in range(nverts):
                p = pos_of[vid]
                if p in refuv and not near(uv0[vid], refuv[p]):
                    edits.append((off + 8 * vid, refuv[p]))
            continue
        info['blocked'] += 1
        info['detail'].append((gi, 'bloqueada', len(tris)))
    return edits, info


def apply_edits(served, out, edits):
    data = bytearray(open(served, 'rb').read())
    for off, uv in edits:
        struct.pack_into('<2f', data, off, uv[0], uv[1])
    os.makedirs(os.path.dirname(out) or '.', exist_ok=True)
    open(out, 'wb').write(bytes(data))
    return len(data)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--base', default='tmp/base421')
    ap.add_argument('--root', default='streamed/models/gta3.img')
    ap.add_argument('--out', default='tmp/base92/fix2')
    ap.add_argument('--models', default='')
    ap.add_argument('--apply', action='store_true')
    ap.add_argument('--check', action='store_true')
    ap.add_argument('--all', action='store_true',
                    help='restaura tambien donde la texela de retail es mas oscura')
    ap.add_argument('--verbose', action='store_true')
    a = ap.parse_args()

    names = [x.strip() for x in a.models.split(',') if x.strip()]
    if not names:
        names = sorted(f[:-4] for f in os.listdir(a.base)
                       if f.endswith('.dff')
                       and os.path.exists(os.path.join(a.root, f)))
    total = {}
    for m in names:
        base = os.path.join(a.base, m + '.dff')
        served = os.path.join(a.root, m + '.dff')
        txd = os.path.join(a.root, m + '.txd')
        if not (os.path.exists(base) and os.path.exists(served)):
            continue
        try:
            edits, info = plan_model(base, served, txd, a.all)
        except Exception as e:
            print('%-10s ERROR %s' % (m, e))
            continue
        for k in ('geoms', 'restored', 'reverted', 'blocked', 'darker'):
            total[k] = total.get(k, 0) + info[k]
        if info['geoms']:
            print('%-10s piezas %3d  restauradas %3d  bloqueadas %3d  oscuras %3d  flotantes %3d'
                  % (m, info['geoms'], info['restored'], info['blocked'],
                     info['darker'], len(edits)))
        if a.verbose:
            for d in info['detail']:
                print('           g%-2d %s' % (d[0], ' '.join(str(x) for x in d[1:])))
        if a.apply and edits:
            out = os.path.join(a.out, m + '.dff')
            n = apply_edits(served, out, edits)
            print('           -> %s (%d flotantes, %d bytes)' % (out, len(edits), n))
    print('total: %s' % '  '.join('%s %d' % (k, total.get(k, 0))
                                  for k in ('geoms', 'restored', 'reverted',
                                            'blocked', 'darker')))
    if a.check:
        return 1 if total.get('geoms', 0) and (total.get('restored', 0)
                                               or total.get('reverted', 0)) else 0
    return 0


if __name__ == '__main__':
    sys.exit(main())
