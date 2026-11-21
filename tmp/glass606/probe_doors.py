import sys, importlib.util, os
spec = importlib.util.spec_from_file_location('g', r'gta_vc_browser/tools/dff_geom.py')
g = importlib.util.module_from_spec(spec); spec.loader.exec_module(g)

def dump(path, filt=('door','window')):
    buf = open(path,'rb').read()
    frames, geoms, atomics = g.parse(buf)
    print('==', os.path.basename(path), 'frames', len(frames), 'atomics', len(atomics))
    for fi, gi in atomics:
        name = frames[fi]['name'] if 0 <= fi < len(frames) else '?'
        if filt and not any(k in name.lower() for k in filt):
            continue
        geo = geoms[gi]; mats = geo.get('mats', []); tris = geo.get('tris', []); verts = geo.get('verts', [])
        print(' %-22s verts=%d tris=%d' % (name, len(verts), len(tris)))
        per = {}
        for t in tris:
            m = t[3]; per.setdefault(m, []).append(t)
        for mi in sorted(per):
            ts = per[mi]
            idx = set()
            for t in ts: idx.update(t[:3])
            xs = [verts[i] for i in idx if i < len(verts)]
            if xs:
                mn = [min(v[k] for v in xs) for k in range(3)]
                mx = [max(v[k] for v in xs) for k in range(3)]
                bb = '[%.2f..%.2f, %.2f..%.2f, %.2f..%.2f]' % (mn[0],mx[0],mn[1],mx[1],mn[2],mx[2])
            else: bb='?'
            mat = mats[mi] if mi < len(mats) else None
            col, tex = (mat[0], mat[1]) if mat else ('?','?')
            print('    mat%-2d rgba=%-22s tex=%-22s tris=%-4d bbox=%s' % (mi, str(tuple(col)), tex, len(ts), bb))
    print()

for p in sys.argv[1:]:
    dump(p)
