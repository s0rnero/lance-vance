import re, sys, statistics as st

pat = re.compile(r'SILLACAM mod=(\S+) jug=(\d) seat=(\S+) antes=(\S+) desp=(\S+) dl=(\S+) sitz=(\S+) vehz=(\S+) est=(\S+) mov=(\S+) bajo=(\S+) van=(\S+) nom=(\S+)')
rows = []
for line in open('gta_vc_browser/web/odtrace.log', encoding='utf-8', errors='replace'):
    if 'SILLACAM' not in line:
        continue
    if 'ENGSTDOUT' in line:
        continue
    m = pat.search(line)
    if not m:
        continue
    rows.append(dict(mod=m.group(1), jug=int(m.group(2)), seat=m.group(3),
                     antes=float(m.group(4)), desp=float(m.group(5)), dl=float(m.group(6)),
                     sitz=float(m.group(7)), vehz=float(m.group(8)), est=m.group(9),
                     mov=m.group(10), bajo=m.group(11), van=m.group(12), nom=m.group(13)))
print('rows=%d' % len(rows))

def pct(v, q):
    v = sorted(v)
    if not v:
        return 0.0
    i = min(len(v)-1, int(q*(len(v)-1)))
    return v[i]

for jug in (1, 0):
    sub = [r for r in rows if r['jug'] == jug]
    dl = [abs(r['dl']) for r in sub]
    neg = [r for r in sub if r['dl'] < -0.05]
    print('jug=%d n=%d |dl| mean=%.4f p50=%.4f p90=%.4f p99=%.4f max=%.4f  neg(<-0.05)=%d' %
          (jug, len(sub), st.fmean(dl) if dl else 0, pct(dl, .5), pct(dl, .9), pct(dl, .99),
           max(dl) if dl else 0, len(neg)))

print('--- worst negative dl rows (npc) ---')
for r in sorted([r for r in rows if r['jug'] == 0], key=lambda r: r['dl'])[:18]:
    print('mod=%-8s seat=%-2s dl=%+.3f antes=%.3f desp=%.3f est=%-3s mov=%s bajo=%s nom=%s' %
          (r['mod'], r['seat'], r['dl'], r['antes'], r['desp'], r['est'], r['mov'], r['bajo'], r['nom']))
print('--- worst negative dl rows (player) ---')
for r in sorted([r for r in rows if r['jug'] == 1], key=lambda r: r['dl'])[:8]:
    print('mod=%-8s seat=%-2s dl=%+.3f antes=%.3f desp=%.3f est=%-3s mov=%s bajo=%s nom=%s' %
          (r['mod'], r['seat'], r['dl'], r['antes'], r['desp'], r['est'], r['mov'], r['bajo'], r['nom']))

print('--- est distribution by role ---')
for jug in (1, 0):
    d = {}
    for r in rows:
        if r['jug'] == jug:
            d[r['est']] = d.get(r['est'], 0) + 1
    print('jug=%d %s' % (jug, sorted(d.items(), key=lambda kv: -kv[1])[:8]))

print('--- residual desp - vehz - sitz by role and seat ---')
for jug in (1, 0):
    for seat in ('0', '1', '2', '3'):
        v = [r['desp'] - r['vehz'] - r['sitz'] for r in rows if r['jug'] == jug and r['seat'] == seat]
        if v:
            print('jug=%d seat=%s n=%-6d mean=%+.4f min=%+.4f max=%+.4f' % (jug, seat, len(v), st.fmean(v), min(v), max(v)))

print('--- sitz equality per model (both roles present) ---')
mods = {}
for r in rows:
    mods.setdefault(r['mod'], {}).setdefault(r['jug'], set()).add((r['seat'], round(r['sitz'], 4)))
both = 0
for m, d in sorted(mods.items(), key=lambda kv: kv[0]):
    if 0 in d and 1 in d:
        both += 1
        if both <= 20:
            print('mod=%-6s player=%s npc=%s' % (m, sorted(d[1]), sorted(d[0])))
print('models_with_both_roles=%d total_models=%d' % (both, len(mods)))
