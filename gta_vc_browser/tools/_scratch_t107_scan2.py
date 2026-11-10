import re, collections

LOG = r'C:\Users\s0rno\OneDrive\Documents\lance-vance\gta_vc_browser\web\odtrace.log'
raw = open(LOG, 'r', encoding='utf-8', errors='replace').read().splitlines()

def body(ln):
    m = re.match(r'^\S+\s+\S+\s+(.*)$', ln)
    return m.group(1) if m else ln

# --- 1. histograma de tags ---
tags = collections.Counter()
for ln in raw:
    b = body(ln)
    m = re.match(r'([A-Za-z][A-Za-z0-9_]{2,})', b)
    if m:
        tags[m.group(1)] += 1
print('TAGS:', tags.most_common(30))

# --- 2. SILLACAM: sitz por (mod, seat, jug) ---
pat = re.compile(r'SILLACAM mod=(\S+) jug=(\d) seat=(\d) antes=(\S+) desp=(\S+) dl=(\S+) sitz=(\S+) vehz=(\S+) est=(\d+) mov=(\d) bajo=(\d) van=(\d) nom=(\S+)')
g = collections.defaultdict(list)
dl_by_jug = collections.defaultdict(list)
for ln in raw:
    m = pat.search(ln)
    if not m:
        continue
    key = (m.group(11), m.group(13), int(m.group(2)), int(m.group(3)))
    g[key].append((float(m.group(7)), float(m.group(6))))
    dl_by_jug[int(m.group(2))].append(abs(float(m.group(6))))
print()
print('bajo nom     jug seat   n   sitz_medio  |dl|_medio  |dl|>0.3')
for key in sorted(g, key=lambda k: -len(g[k]))[:26]:
    v = g[key]
    sitz = sum(x[0] for x in v) / len(v)
    dls = [abs(x[1]) for x in v]
    print('%s %-9s %d   %d   %5d  %+.4f     %.4f      %d' % (
        key[0], key[1][:9], key[2], key[3], len(v), sitz,
        sum(dls) / len(dls), sum(1 for d in dls if d > 0.3)))
for jug in dl_by_jug:
    v = dl_by_jug[jug]
    print('jug=%d n=%d |dl|medio=%.4f pct>0.3=%.1f%%' % (jug, len(v), sum(v)/len(v),
          100.0*sum(1 for d in v if d > 0.3)/len(v)))

# --- 3. rafagas de mensajes identicos ---
print()
print('RAFAGAS (>=4 lineas consecutivas con el mismo cuerpo normalizado):')
prev, run, start = None, 0, 0
for i, ln in enumerate(raw):
    b = re.sub(r'\d+', 'N', body(ln))[:110]
    if b == prev:
        run += 1
    else:
        if run >= 4:
            print('  x%d @%s  %s' % (run, raw[start][11:24], body(raw[start])[:150]))
        prev, run, start = b, 1, i
if run >= 4:
    print('  x%d @%s  %s' % (run, raw[start][11:24], body(raw[start])[:150]))

# --- 4. trazas de armas y colisiones ---
for key in ['VICEEX sfx', 'ODCOLL', 'SCRAPE', 'sfx arma', 'AUDIOERR', 'OPENAL', 'LOOP']:
    hits = [body(ln) for ln in raw if key in ln]
    print('==', key, len(hits))
    for h in hits[-4:]:
        print('   ', h[:170])
