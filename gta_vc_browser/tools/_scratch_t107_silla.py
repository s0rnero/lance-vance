import re, collections

LOG = r'C:\Users\s0rno\OneDrive\Documents\lance-vance\gta_vc_browser\web\odtrace.log'
raw = open(LOG, 'r', encoding='utf-8', errors='replace').read().splitlines()

pat = re.compile(
    r'SILLACAM mod=(\S+) jug=(\d) seat=(\d) antes=(\S+) desp=(\S+) dl=(\S+) '
    r'sitz=(\S+) vehz=(\S+) est=(\d+) mov=(\d) bajo=(\d) van=(\d) nom=(\S+)')

rows = []
for ln in raw:
    m = pat.search(ln)
    if m:
        rows.append(dict(ts=ln[:24], mod=m.group(1), jug=int(m.group(2)), seat=int(m.group(3)),
                         antes=float(m.group(4)), desp=float(m.group(5)), dl=float(m.group(6)),
                         sitz=float(m.group(7)), vehz=float(m.group(8)), est=int(m.group(9)),
                         mov=int(m.group(10)), bajo=int(m.group(11)), van=int(m.group(12)),
                         nom=m.group(13)))

print('filas', len(rows))
print('jug=1 filas', sum(1 for r in rows if r['jug'] == 1))

bymod = collections.defaultdict(lambda: collections.defaultdict(list))
for r in rows:
    bymod[r['mod']][r['jug']].append(r)

print()
print('mod   nom       asiento  hip_z_medio_jugador   hip_z_medio_npc   residuo_jugador residuo_npc  dl_max')
for mod in sorted(bymod, key=lambda k: -len(bymod[k][0]) - len(bymod[k][1])):
    d = bymod[mod]
    n = d[0][0]['nom'] if d[0] else d[1][0]['nom']
    seat = d[0][0]['seat'] if d[0] else d[1][0]['seat']
    def avg(lst, key):
        return sum(x[key] for x in lst) / len(lst) if lst else float('nan')
    def res(lst):
        return avg(lst, 'desp') - avg(lst, 'vehz') - avg(lst, 'sitz') if lst else float('nan')
    dlmax = max((abs(x['dl']) for x in d[0] + d[1]), default=float('nan'))
    j1 = d[1]
    print('%-5s %-9s s=%d  j1=%5d n1=%6d  hip_j1=%s hip_j0=%s  res_j1=%s res_j0=%s dlmax=%.3f bajo=%s van=%s' % (
        mod, n[:9], seat, len(j1), len(d[0]),
        ('%.3f' % avg(j1, 'desp')) if j1 else '   -  ',
        '%.3f' % avg(d[0], 'desp'),
        ('%+.3f' % res(j1)) if j1 else '  -  ',
        '%+.3f' % res(d[0]), dlmax,
        (d[0] or d[1])[0]['bajo'], (d[0] or d[1])[0]['van']))

print()
print('modelos con jug=1 (el jugador sentado):')
for r in rows:
    if r['jug'] == 1:
        print('  ', r['ts'], 'mod=%s nom=%s seat=%d desp=%.3f vehz=%.3f sitz=%.3f est=%d mov=%d' % (
            r['mod'], r['nom'], r['seat'], r['desp'], r['vehz'], r['sitz'], r['est'], r['mov']))
        break
agg1 = collections.defaultdict(list)
for r in rows:
    if r['jug'] == 1:
        agg1[(r['mod'], r['nom'], r['seat'])].append(r)
for k, v in sorted(agg1.items()):
    print('  A/B', k, 'n=%d' % len(v), 'hip=%.3f vehz=%.3f sitz=%.3f' % (
        sum(x['desp'] for x in v) / len(v), sum(x['vehz'] for x in v) / len(v),
        sum(x['sitz'] for x in v) / len(v)))
