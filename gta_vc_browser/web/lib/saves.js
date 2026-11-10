import { getFS, syncFs, saveBlob } from './fsx.js';
import { makeZip } from './zip.js';

const DIR = '/userfiles';
const SLOTS = 9;
const EXT = /\.b$/i;

function slotFile(n) { return 'GTAVCsf' + n + '.b'; }
function slotPath(n) { return DIR + '/' + slotFile(n); }

export function slotInfo() {
  const FS = getFS();
  const out = [];
  for (let n = 1; n <= SLOTS; n++) {
    let size = 0;
    try { size = FS ? FS.stat(slotPath(n)).size : 0; } catch (e) { size = 0; }
    out.push({ slot: n, file: slotFile(n), size, used: size > 0 });
  }
  return out;
}

export function downloadSlot(n) {
  const FS = getFS();
  if (!FS) return false;
  try {
    const data = FS.readFile(slotPath(n));
    saveBlob(new Blob([data], { type: 'application/octet-stream' }), slotFile(n));
    return true;
  } catch (e) {
    return false;
  }
}

export async function downloadAll() {
  const FS = getFS();
  if (!FS) return 0;
  const entries = [];
  for (const s of slotInfo()) {
    if (!s.used) continue;
    try { entries.push({ name: s.file, data: new Uint8Array(FS.readFile(slotPath(s.slot))) }); } catch (e) {}
  }
  if (!entries.length) return 0;
  saveBlob(await makeZip(entries), 'partidas-vice-city.zip');
  return entries.length;
}

export function rescanSaves() {
  try {
    const m = globalThis.Module;
    if (m && typeof m._ViceExtRescanSaves === 'function') {
      m._ViceExtRescanSaves();
      return true;
    }
  } catch (e) {}
  return false;
}

export async function importSaves(files) {
  const FS = getFS();
  if (!FS) return { ok: false, reason: 'nofs', written: [] };
  const valid = [];
  for (const f of Array.from(files || [])) {
    const name = String(f.name || '').replace(/^.*[\\/]/, '');
    if (EXT.test(name) && f.size > 0) valid.push(f);
  }
  if (!valid.length) return { ok: false, reason: 'invalid', written: [] };
  const pool = slotInfo().filter((s) => !s.used).map((s) => s.slot);
  const count = Math.min(valid.length, pool.length);
  const written = [];
  for (let i = 0; i < count; i++) {
    const target = pool[i];
    try {
      FS.writeFile(slotPath(target), new Uint8Array(await valid[i].arrayBuffer()));
      written.push(target);
    } catch (e) {}
  }
  if (written.length) syncFs('saves');
  const reason = written.length === valid.length ? 'ok' : 'no-space';
  if (reason === 'no-space' && typeof window !== 'undefined') window.__vcWebSaveNoSpace = 1;
  setTimeout(rescanSaves, 0);
  if (written.length && typeof window !== 'undefined') window.__vcWebSaveImported = written.reduce((m, n) => m | (1 << n), 0);
  return { ok: reason === 'ok', reason, written, free: Math.max(0, pool.length - written.length) };
}

function pickFiles(accept) {
  return new Promise((resolve) => {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = accept;
    input.multiple = true;
    input.style.display = 'none';
    document.body.appendChild(input);
    input.onchange = () => { const f = Array.from(input.files || []); input.remove(); resolve(f); };
    input.oncancel = () => { input.remove(); resolve([]); };
    input.click();
  });
}

export async function importSavesFromPicker() {
  const files = await pickFiles('.b');
  if (!files.length) return { ok: false, reason: 'cancel', written: [] };
  const r = await importSaves(files);
  return r;
}
