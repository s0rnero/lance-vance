import { getFS, syncFs } from './fsx.js';

const DIR = '/skins';
const STORE = '/userfiles/skins';
const MAX = 5;
const CAP = 8 * 1024 * 1024;

function safeName(name) {
  const base = String(name || '').replace(/^.*[\\/]/, '').replace(/\.bmp$/i, '');
  const clean = base.replace(/[^A-Za-z0-9_-]/g, '_').slice(0, 40);
  return (clean || 'skin') + '.bmp';
}

function stored(FS) {
  try { return FS.readdir(STORE).filter((n) => /\.bmp$/i.test(n)); } catch (e) { return []; }
}

export function restoreSkins() {
  const FS = getFS();
  if (!FS) return 0;
  try { FS.mkdirTree(DIR); } catch (e) {}
  try { FS.mkdirTree(STORE); } catch (e) {}
  let n = 0;
  for (const name of stored(FS)) {
    try {
      FS.writeFile(DIR + '/' + name, FS.readFile(STORE + '/' + name));
      n++;
    } catch (e) {}
  }
  return n;
}

export function skinInfo() {
  const FS = getFS();
  if (!FS) return [];
  return stored(FS).map((name) => {
    let size = 0;
    try { size = FS.stat(STORE + '/' + name).size; } catch (e) {}
    return { name, size };
  });
}

export function rescanSkins() {
  try {
    const m = globalThis.Module;
    if (m && typeof m._ViceExtRescanSkins === 'function') {
      m._ViceExtRescanSkins();
      return true;
    }
  } catch (e) {}
  return false;
}

export async function importSkins(files) {
  const FS = getFS();
  if (!FS) return { ok: false, reason: 'nofs', written: [] };
  const picked = Array.from(files || []).filter((f) => /\.bmp$/i.test(String(f.name || '')));
  if (!picked.length) return { ok: false, reason: 'invalid', written: [] };
  try { FS.mkdirTree(DIR); } catch (e) {}
  try { FS.mkdirTree(STORE); } catch (e) {}
  const written = [];
  for (const f of picked.slice(0, MAX)) {
    if (f.size < 54 || f.size > CAP) continue;
    const name = safeName(f.name);
    try {
      const data = new Uint8Array(await f.arrayBuffer());
      if (data[0] !== 0x42 || data[1] !== 0x4d) continue;
      FS.writeFile(STORE + '/' + name, data);
      FS.writeFile(DIR + '/' + name, data);
      written.push(name);
    } catch (e) {}
  }
  if (written.length) syncFs('skins');
  setTimeout(rescanSkins, 0);
  return { ok: written.length > 0, reason: written.length ? 'ok' : 'invalid', written, limit: MAX, picked: picked.length };
}

function pickFiles() {
  return new Promise((resolve) => {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.bmp,image/bmp';
    input.multiple = true;
    input.style.display = 'none';
    document.body.appendChild(input);
    input.onchange = () => { const f = Array.from(input.files || []); input.remove(); resolve(f); };
    input.oncancel = () => { input.remove(); resolve([]); };
    input.click();
  });
}

export async function importSkinsFromPicker() {
  const files = await pickFiles();
  if (!files.length) return { ok: false, reason: 'cancel', written: [] };
  const r = await importSkins(files);
  if (typeof window !== 'undefined') {
    const code = r.ok ? 1 : r.picked > r.limit ? 2 : 3;
    window.__vcWebSkinResult = code === 1 ? 1000 + r.written.length : code * 1000;
  }
  return r;
}
