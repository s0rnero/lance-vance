#!/usr/bin/env python3
"""Mide la ESTATURA de cada ped servido: a cuanto esta la coronilla de la raiz.

Sirve para el plan 48.0 (apartado A): `CPed::SetPedPositionInCar` coloca la
CADERA (la raiz del esqueleto) en el marco `ped_frontseat` del DFF del coche,
asi que un ped bajito queda con la cabeza por debajo del liston de la ventanilla
y se ve "hundido" — con el mismo anclaje que a Tommy no le pasa porque los
anclajes se autoron contra el modelo estandar. Lo que hace falta para compensar
es el numero real por modelo, no una suposicion.

Que hace:
  - recorre los DFF y se queda con los que tienen esqueleto de ped (marcos
    Pelvis + Head + L Toe0);
  - compone las matrices de la cadena de huesos con la convencion de RenderWare
    que usa el motor: matriz del marco = filas [right, up, at] + traslacion,
    p_parent = p_local*M_local, y LTM_i = M_i * LTM_padre
    (o sea t_i = pos_i*R_padre + t_padre);
  - en los peds de VC el eje vertical del modelo es +X (comprobado en
    player.dff: cabeza +0.605, dedo del pie -1.049, 1.65 m en total), pero aqui
    se detecta por donde se separan cabeza y pie;
  - imprime la tabla de dispersión y la compensacion `(ref - cabeza)` que T7
    sumaria a la Z del anclaje.

Uso:
    python tools/ped_seat_height.py [dirs...]
    python tools/ped_seat_height.py --json tmp/ped_heights.json --ref 0.60
Por defecto recorre streamed/models y bootseed/models.
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dff_dump import children, first, parse_frames, u32  # noqa: E402


def mul(a, b):
    """a aplicada despues b (convencion de filas): t = a.t*b.R + b.t."""
    ar, at = a
    br, bt = b

    def x(v, m):
        return tuple(sum(v[k] * m[j][k] for j in range(3)) for k in range(3))
    return tuple(tuple(x(r, br) for r in ar),), tuple(x(at, br)[k] + bt[k]
                                                      for k in range(3))


def ped_bones(path):
    """(cabeza, pie, cadera) en espacio del modelo, o None si no es un ped."""
    try:
        buf = open(path, 'rb').read()
    except OSError:
        return None
    clump = None
    for c in children(buf, 0, len(buf)):
        if c.type == 0x10:
            clump = c
            break
    if clump is None:
        return None
    fl = None
    for c in children(buf, clump.start, clump.end):
        if c.type == 0x0E:
            fl = c
            break
    if fl is None:
        return None
    frames = parse_frames(buf, fl)
    if len(frames) > 200:
        return None
    idx = {}
    for i, f in enumerate(frames):
        idx.setdefault(f['name'].lower(), i)
    if 'head' not in idx or 'pelvis' not in idx:
        return None
    toe = idx.get('l toe0', idx.get('r toe0'))
    if toe is None:
        return None
    # en el bin empaquetado un negativo (distinto del -1 de la raiz) codifica -p-1
    for i, f in enumerate(frames):
        if f['parent'] < 0 and i != 0:
            f['parent'] = -f['parent'] - 1
    ltm = {}

    def get(i):
        if i in ltm:
            return ltm[i]
        f = frames[i]
        r = f['rot']
        m = ((r[0], r[1], r[2]), (r[3], r[4], r[5]), (r[6], r[7], r[8])), f['pos']
        p = f['parent']
        out = m if p < 0 or p >= len(frames) or p == i else mul(m, get(p))
        ltm[i] = out
        return out

    head = get(idx['head'])[1]
    foot = get(toe)[1]
    pelvis = get(idx['pelvis'])[1]
    if pelvis[0] > head[0]:
        return None
    return head, foot, pelvis


def emit_c(rows, ref, min_lift):
    """Tabla C++ para CPedModelInfo: solo los modelos que hay que subir.

    Es la salida de `--emit-c`; la consume `odSeatLiftForModel()` en
    `src/modelinfo/PedModelInfo.cpp`, porque medir el reposo del clump en
    tiempo de ejecución no es posible (las rotaciones de los marcos las pisa la
    animación, ver plan 48.0 T7).
    """
    keep = [r for r in rows if ref - r['head'] >= min_lift]
    keep.sort(key=lambda r: r['model'])
    print('/* Generado por tools/ped_seat_height.py (ref=%.3f, lift en cm).' % ref)
    print('   Regenerar con: python tools/ped_seat_height.py --emit-c */')
    print('static const struct { char m_name[12]; int16 m_lift_cm; }')
    print('odPedSeatLiftTable[] = {')
    n = 0
    for r in keep:
        cm = int(round((ref - r['head']) * 100.0))
        print('\t{ "%s", %d },' % (r['model'], cm))
        n += 1
    print('};')
    print('/* %d modelos de %d */' % (n, len(rows)))


def main():
    dirs = [a for a in sys.argv[1:] if not a.startswith('--')]
    out_json = ref = None
    emit = '--emit-c' in sys.argv
    min_lift = 0.02
    if '--min-lift' in sys.argv:
        min_lift = float(sys.argv[sys.argv.index('--min-lift') + 1])
        dirs = [d for d in dirs if d != sys.argv[sys.argv.index('--min-lift') + 1]]
    if '--json' in sys.argv:
        out_json = sys.argv[sys.argv.index('--json') + 1]
        dirs = [d for d in dirs if d != out_json]
    if '--ref' in sys.argv:
        ref = float(sys.argv[sys.argv.index('--ref') + 1])
        dirs = [d for d in dirs if d != sys.argv[sys.argv.index('--ref') + 1]]
    dirs = dirs or ['streamed/models/gta3.img', 'bootseed/models/gta3.img']
    seen = set()
    rows = []
    for d in dirs:
        if not os.path.isdir(d):
            continue
        for root, _, files in os.walk(d):
            for fn in files:
                if not fn.lower().endswith('.dff'):
                    continue
                if fn.lower() in seen:
                    continue
                seen.add(fn.lower())
                r = ped_bones(os.path.join(root, fn))
                if r is None:
                    continue
                head, foot, pelvis = r
                ax = max(range(3), key=lambda i: abs(head[i] - foot[i]))
                if head[ax] < foot[ax]:
                    s = -1
                else:
                    s = 1
                rows.append({'model': fn[:-4].lower(), 'file': os.path.join(root, fn),
                             'axis': 'xyz'[ax],
                             'head': s * head[ax], 'pelvis': s * pelvis[ax],
                             'foot': s * foot[ax],
                             'stature': s * (head[ax] - foot[ax]),
                             'legs': -s * foot[ax], 'torso': s * (head[ax] - pelvis[ax])})
    if not rows:
        raise SystemExit('no encuentro ningun DFF de ped')
    odd = [r for r in rows if r['axis'] != 'x']
    rows = [r for r in rows if r['axis'] == 'x']
    rows.sort(key=lambda r: r['head'])
    hs = sorted(r['head'] for r in rows)

    def q(p):
        return hs[min(len(hs) - 1, int(round(p * (len(hs) - 1))))]
    if ref is None:
        ref = q(0.5)
    if emit:
        emit_c(rows, ref, min_lift)
        return
    print('%d peds con eje vertical X' % len(rows))
    print('cabeza sobre la raiz: min=%.3f p10=%.3f med=%.3f p90=%.3f max=%.3f'
          % (hs[0], q(0.10), q(0.5), q(0.90), hs[-1]))
    print('estatura total (planta->coronilla): min=%.3f med=%.3f max=%.3f'
          % (min(r['stature'] for r in rows),
             sorted(r['stature'] for r in rows)[len(rows) // 2],
             max(r['stature'] for r in rows)))
    print('referencia usada = %.3f  (comp = ref - cabeza; solo sube a los bajitos)'
          % ref)
    if odd:
        print('descartados por eje vertical raro (%d): %s'
              % (len(odd), ', '.join('%s/%s' % (o['model'], o['axis']) for o in odd[:12])))
    print()
    print('%-10s %7s %7s %7s %7s %8s' % ('modelo', 'cabeza', 'cadera', 'piernas',
                                         'estatura', 'comp'))
    for r in rows:
        r['comp'] = max(0.0, ref - r['head'])
        print('%-10s %7.3f %7.3f %7.3f %7.3f %8.3f'
              % (r['model'], r['head'], r['pelvis'], r['legs'], r['stature'], r['comp']))
    ups = [r['comp'] for r in rows if r['comp'] > 0]
    if ups:
        print('\nse elevan %d/%d modelos; comp media=%.3f max=%.3f'
              % (len(ups), len(rows), sum(ups) / len(ups), max(ups)))
    if out_json:
        with open(out_json, 'w') as fp:
            json.dump(rows, fp, indent=1)
        print('json -> %s' % out_json)


if __name__ == '__main__':
    main()
