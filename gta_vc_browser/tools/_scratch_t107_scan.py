import re, sys, collections

LOG = r'C:\Users\s0rno\OneDrive\Documents\lance-vance\gta_vc_browser\web\odtrace.log'

lines = open(LOG, 'r', encoding='utf-8', errors='replace').read().splitlines()
print('lineas', len(lines))

tag_re = re.compile(r'\s(?:\d{4}-\d\d-\d\dT[\d:.]+Z)\s+(.*)$')
body = []
for ln in lines:
    m = tag_re.match(ln)
    body.append(m.group(1) if m else ln)

last_engine = None
for i, b in enumerate(body):
    if 'WDTALIVE' not in b and 'ODJSALIVE' not in b:
        last_engine = (i, lines[i][:60], b[:120])
print('ultima linea de motor:', last_engine)

tags = collections.Counter()
for b in body:
    m = re.match(r'([A-Z][A-Z0-9_]{3,})', b)
    if m:
        tags[m.group(1)] += 1
print('tags con mayusculas:', tags.most_common(25))

for key in ['LOOPCOLLIDE', 'LOOPORPHAN', 'LOOPKILL', 'STREAMLOOP', 'STREAMDSYNC',
            'PEDVARFREE', 'SILLACAM', 'PAGEHIDE', 'JSERR']:
    hits = [b for b in body if key in b]
    print('==', key, len(hits))
    for h in hits[-6:]:
        print('   ', h[:200])
