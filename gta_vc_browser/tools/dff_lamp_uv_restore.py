#!/usr/bin/env python3
"""Devuelve a las caras de faro/piloto la UV del DFF del PC.

Motivo medido (plan 92.0): en el arbol servido hay caras cuya UV es la del DFF
de retail con la V espejada (`v -> 1-v`). En las lentes eso las lleva de la
zona roja/blanca del atlas a una zona negra del mismo atlas: el faro se ve
negro y el piloto solo deja asomar un pico. El atlas del TXD es byte a byte el
de retail, asi que la diferencia esta en las UV del DFF, no en la textura.

La herramienta no inventa nada: empareja triangulos por la posicion de sus tres
vertices con el DFF de referencia (el del PC, `tmp/base421`) y, cuando la UV
servida es exactamente `(u, 1-v)` de la de referencia, escribe la de
referencia. Solo cambia los flotantes de la UV; vertices, triangulos,
materiales, normales y prelit quedan intactos.

Solo se restaura cuando la texela de referencia no es mas oscura que la que
sirve hoy (`--all` restaura todas, sin esa puerta), y nunca se toca un vertice
que compartan caras con UV distinta (se informa como conflicto).

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
    """{(posiciones): [(geom, idx_tri, {vertice: (u,v)})]} de un DFF."""
    buf = open(path, 'rb').read()
    chunks = geom_chunks(buf)
    out = {}
    geoms = []
    for ch in chunks:
        mats, verts, tris, uvs, norms, flags, colors = dg.parse_geometry(buf, ch)
        geoms.append((mats, verts, tris, uvs))
        if not uvs or not verts:
            continue
        for ti, t in enumerate(tris):
            corners = {}
            for k in range(3):
                p = tuple(round(c, 2) for c in verts[t[k]])
                corners[p] = uvs[0][t[k]]
            key = tuple(sorted(corners))
            out.setdefault(key, []).append((len(geoms) - 1, ti, corners,
                                            mats[t[3]][1] if t[3] < len(mats) else None))
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


def plan_model(base, served, txd, restore_all=False):
    """Devuelve (cambios por geometria, informe)."""
    rbuf, rchunks, rfaces = faces(base)
    sbuf, schunks, sfaces = faces(served)
    tex = texdict(txd) if os.path.exists(txd) else {}
    uvofs = [uv_offset(sbuf, ch) for ch in schunks]
    want = {}
    planned = {}
    info = {'mirrored': 0, 'restored': 0, 'darker': 0, 'conflict': 0,
            'unmatched': 0, 'except': []}
    by_key = {k: v[0] for k, v in rfaces.items()}
    for key, items in sfaces.items():
        ref = by_key.get(key)
        if ref is None:
            info['unmatched'] += len(items)
            continue
        rgi, rti, rcorners, rtex = ref
        for gi, ti, corners, stex in items:
            if len(corners) != 3 or len(rcorners) != 3:
                continue
            flipped = {}
            for p, uv in corners.items():
                if p not in rcorners:
                    flipped = None
                    break
                flipped[p] = (rcorners[p][0], 1.0 - rcorners[p][1])
            if not flipped:
                continue
            if max(abs(corners[p][0] - flipped[p][0]) + abs(corners[p][1] - flipped[p][1])
                   for p in corners) > TOL:
                continue
            info['mirrored'] += 1
            if not restore_all and tex:
                now = [sample(tex, stex, uv) for uv in corners.values()]
                ref_l = [sample(tex, stex, uv) for uv in rcorners.values()]
                now = [x for x in now if x is not None]
                ref_l = [x for x in ref_l if x is not None]
                if now and ref_l and sum(ref_l) / len(ref_l) + LUM_TOL < sum(now) / len(now):
                    info['darker'] += 1
                    info['except'].append((gi, ti, sum(now) / len(now), sum(ref_l) / len(ref_l)))
                    continue
            planned.setdefault(gi, set()).add(ti)
            for p, uv in rcorners.items():
                want.setdefault(gi, {}).setdefault(p, set()).add((round(uv[0], 7), round(uv[1], 7)))
    # Resolver vertices: se escribe solo si todas las caras que lo usan piden lo mismo.
    edits = []
    for gi, per_vertex in want.items():
        off, nverts, ntex = uvofs[gi]
        if off is None:
            info['conflict'] += len(per_vertex)
            continue
        mats, verts, tris, uvs, norms, flags, colors = dg.parse_geometry(sbuf, schunks[gi])
        pos_of = [tuple(round(c, 2) for c in v) for v in verts]
        used = {}
        for ti, t in enumerate(tris):
            for k in range(3):
                used.setdefault(t[k], set()).add(ti)
        uv0 = uvs[0]
        done = {}
        for p, vals in per_vertex.items():
            if len(vals) != 1:
                info['conflict'] += 1
                continue
            uv = next(iter(vals))
            vids = [vid for vid in range(len(verts)) if pos_of[vid] == p]
            if not vids:
                info['conflict'] += 1
                continue
            for vid in vids:
                # Ningun vertice compartido con una cara que no se restaura:
                # escribir ahi cambiaria la UV de una cara que hoy esta bien.
                if not used.get(vid, set()) <= planned.get(gi, set()):
                    info['conflict'] += 1
                    done[vid] = None
                    continue
                prev = done.get(vid)
                if prev is not None and prev != uv:
                    info['conflict'] += 1
                    done[vid] = None
                    continue
                done[vid] = uv
        for vid, uv in done.items():
            if uv is None:
                continue
            edits.append((off + 8 * vid, uv))
            info['restored'] += 1
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
    ap.add_argument('--out', default='tmp/base92/fix')
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
        for k in ('mirrored', 'restored', 'darker', 'conflict', 'unmatched'):
            total[k] = total.get(k, 0) + info[k]
        if info['mirrored'] or a.verbose:
            print('%-10s espejadas %3d  restauradas %3d  mas oscuras %3d  conflictos %3d'
                  % (m, info['mirrored'], info['restored'], info['darker'], info['conflict']))
            if a.verbose:
                for gi, ti, now, ref in info['except'][:8]:
                    print('           g%-2d t%-4d lum hoy %.0f -> retail %.0f' % (gi, ti, now, ref))
        if a.apply and edits:
            out = os.path.join(a.out, m + '.dff')
            n = apply_edits(served, out, edits)
            # El vertice compartido que la primera pasada no podia tocar deja de
            # serlo cuando su vecino ya esta restaurado: se repite hasta que no
            # queden flotantes que cambiar (tope de 5 vueltas).
            vueltas = 1
            src = out
            for _ in range(4):
                more, _info = plan_model(base, src, txd, a.all)
                if not more:
                    break
                n = apply_edits(src, out, more)
                vueltas += 1
            print('           -> %s (%d flotantes, %d vueltas, %d bytes)'
                  % (out, len(edits), vueltas, n))
    print('total: %s' % '  '.join('%s %d' % (k, total.get(k, 0))
                                  for k in ('mirrored', 'restored', 'darker', 'conflict')))
    if a.check:
        return 1 if total.get('mirrored', 0) and total.get('restored', 0) else 0
    return 0


if __name__ == '__main__':
    sys.exit(main())
