import { getFS, syncFs, saveBlob } from './fsx.js';
import { makeZip } from './zip.js';

function trace(s) {
  try { (globalThis.__odq = globalThis.__odq || []).push('MUS ' + s); } catch (e) {}
}

const DIR = '/MP3';
const CAP = 300 * 1024 * 1024;

function isMp3(u8) {
  if (!u8 || u8.length < 4) return false;
  if (u8[0] === 0x49 && u8[1] === 0x44 && u8[2] === 0x33) return true;
  return u8[0] === 0xFF && (u8[1] & 0xE0) === 0xE0;
}

function clean(name) {
  const base = String(name || '').replace(/[^\w.\- ]+/g, '_').replace(/^\.+/, '').trim();
  const trimmed = base.slice(-64);
  return /\.mp3$/i.test(trimmed) ? trimmed : trimmed.replace(/\.[^.]*$/, '') + '.mp3';
}

function freeName(FS, name) {
  let path = DIR + '/' + name;
  if (!FS.analyzePath(path).exists) return path;
  const stem = name.replace(/\.mp3$/i, '');
  for (let i = 2; i < 1000; i++) {
    path = DIR + '/' + stem + ' (' + i + ').mp3';
    if (!FS.analyzePath(path).exists) return path;
  }
  return DIR + '/' + Date.now() + '.mp3';
}

function list(FS) {
  try { return FS.readdir(DIR).filter((n) => /\.mp3$/i.test(n)); } catch (e) { return []; }
}

function usedBytes(FS) {
  let n = 0;
  for (const name of list(FS)) {
    try { n += FS.stat(DIR + '/' + name).size; } catch (e) {}
  }
  return n;
}

export function rescanUserMusic() {
  try {
    const m = globalThis.Module;
    if (m && typeof m._ViceExtRescanUserMusic === 'function') {
      m._ViceExtRescanUserMusic();
      return true;
    }
  } catch (e) {}
  return false;
}

export function importMusic() {
  return new Promise((resolve) => {
    const FS = getFS();
    if (!FS) {
      resolve({ ok: false, reason: 'nofs', added: [], rejected: [] });
      return;
    }
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.mp3,audio/mpeg';
    input.multiple = true;
    input.style.display = 'none';
    document.body.appendChild(input);
    input.onchange = async () => {
      const files = Array.from(input.files || []);
      input.remove();
      const added = [];
      const rejected = [];
      try { FS.mkdirTree(DIR); } catch (e) {}
      const pending = [];
      for (const f of files) {
        let buf;
        try { buf = new Uint8Array(await f.arrayBuffer()); }
        catch (e) { rejected.push(f.name); continue; }
        if (!/\.mp3$/i.test(f.name) || !isMp3(buf)) { rejected.push(f.name); continue; }
        pending.push({ name: f.name, buf });
      }
      const incoming = pending.reduce((s, p) => s + p.buf.length, 0);
      const used = usedBytes(FS);
      if (used + incoming > CAP) {
        resolve({ ok: false, reason: 'cap', used, incoming, limit: CAP, added: [], rejected: files.map((f) => f.name) });
        return;
      }
      for (const p of pending) {
        try {
          FS.writeFile(freeName(FS, clean(p.name)), p.buf);
          added.push(clean(p.name));
        } catch (e) {
          rejected.push(p.name);
        }
      }
      if (added.length) syncFs('music');
      if (typeof window !== 'undefined') {
        if (added.length) window.__vcWebMusicRescan = 1;
        window.__vcWebMusicImported = added.length;
      }
      trace('import n=' + added.length + ' de ' + files.length);
      resolve({ ok: added.length > 0, reason: added.length ? 'ok' : 'rejected', added, rejected, rescanned: added.length > 0, used: usedBytes(FS) });
    };
    input.oncancel = () => {
      input.remove();
      resolve({ ok: false, reason: 'cancel', added: [], rejected: [] });
    };
    input.click();
  });
}

export function deleteAllMusic() {
  const FS = getFS();
  if (!FS) return 0;
  let n = 0;
  for (const name of list(FS)) {
    try { FS.unlink(DIR + '/' + name); n++; } catch (e) {}
  }
  if (n) syncFs('music');
  if (typeof window !== 'undefined') window.__vcWebMusicRescan = 1;
  trace('borradas ' + n);
  return n;
}

export async function downloadAllMusic() {
  const FS = getFS();
  if (!FS) return 0;
  const entries = [];
  let total = 0;
  for (const name of list(FS)) {
    try {
      const data = new Uint8Array(FS.readFile(DIR + '/' + name));
      if (total + data.length > CAP) return 0;
      total += data.length;
      entries.push({ name, data });
    } catch (e) {}
  }
  if (!entries.length) return 0;
  saveBlob(await makeZip(entries), 'musica-vice-city.zip');
  return entries.length;
}

export function musicInfo() {
  const FS = getFS();
  if (!FS) return { files: [], ready: false, used: 0, cap: CAP };
  return { files: list(FS), ready: true, used: usedBytes(FS), cap: CAP };
}

if (typeof window !== 'undefined' && !window.__vcMusicRescan) window.__vcMusicRescan = rescanUserMusic;
