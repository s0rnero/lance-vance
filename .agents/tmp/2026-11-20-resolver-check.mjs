import fs from 'node:fs';

const root = 'gta_vc_browser';
const js = fs.readFileSync(root + '/web/public/build/reVC.js', 'utf8');
const man = JSON.parse(fs.readFileSync(root + '/web/public/manifest.json', 'utf8'));

const memfs = new Map();
const re = /\{filename:"([^"]+)",start:(\d+),end:(\d+)\}/g;
let m;
while ((m = re.exec(js))) memfs.set(m[1], Number(m[3]) - Number(m[2]));

const norm = (p) => String(p).replace(/\\/g, '/').replace(/^\.\//, '').replace(/^\/+/, '').toLowerCase();
const stat = (p) => {
  if (!memfs.has(p)) throw new Error('ENOENT');
  return { size: memfs.get(p) };
};

function presentSync(reqPath, cwd = '/') {
  const cands = [norm(reqPath), norm(cwd + '/' + reqPath)];
  let want = 0;
  for (const n of cands) {
    const h = man[n];
    if (h && h.s) { want = h.s; break; }
  }
  if (!want) {
    const base = norm(reqPath).split('/').pop();
    for (const k in man) {
      if (k === base || k.slice(-(base.length + 1)) === '/' + base) { want = man[k].s || 0; break; }
    }
  }
  for (const n of cands) {
    const f = '/' + n;
    try {
      if (want && stat(f).size === 0) continue;
      return f;
    } catch (e) {}
  }
  let canon = null;
  for (const n of cands) {
    if (man[n]) { canon = '/' + man[n].p; break; }
  }
  if (canon && memfs.has(canon)) return canon;
  return null;
}

const reqs = [
  'MODELS\\GENERIC\\WHEELS.TXD',
  'MODELS\\GENERIC.TXD',
  'MODELS\\GENERIC\\AIR_VLO.DFF',
  'DATA\\DEFAULT.DAT',
];

console.log('nodos MEMFS:', memfs.size);
for (const r of reqs) {
  const n = norm(r);
  const h = man[n];
  const rp = presentSync(r);
  let sz = null;
  try { sz = stat(rp).size; } catch (e) {}
  let raw = null;
  try { raw = stat(r).size; } catch (e) {}
  console.log('');
  console.log('pedido      :', r);
  console.log('norm        :', n);
  console.log('manifest    :', h ? ('p=' + h.p + ' s=' + h.s + ' pk=' + h.pk) : '(sin entrada)');
  console.log('memfs exacto:', memfs.has('/' + n) ? ('SI ' + memfs.get('/' + n)) : 'NO');
  console.log('resuelto    :', rp);
  console.log('size resuelto:', sz);
  console.log('fopen crudo :', raw === null ? 'FALLA (case-sensitive)' : 'OK ' + raw);
}
