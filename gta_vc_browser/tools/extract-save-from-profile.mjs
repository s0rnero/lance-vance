// Extrae la partida guardada del juego desde el IndexedDB del navegador del
// jugador (los slots viven en /userfiles dentro de IDBFS, no en disco).
//
// Cómo: copia (solo lee) la carpeta de IndexedDB del perfil de Chrome a un
// perfil temporal, arranca el juego con ese perfil y le pide el fichero al
// sistema de ficheros de Emscripten. No modifica nada del perfil original.
//
// Uso:
//   node gta_vc_browser/tools/extract-save-from-profile.mjs [slot]
//     slot: 1..8 (por defecto 1 -> archivo GTAVCsf1.b = primer slot del menú)
// Salida: gta_vc_browser/tools/testdata/GTAVCsf<slot>.b
//
// Requisitos: servidor de desarrollo en marcha (cd gta_vc_browser/web && npm run dev)
// y Chrome cerrado o al menos sin el perfil en uso intensivo.
//
// Variables: CHROME (ruta de chrome.exe), PUPPETEER_DIR (node_modules con
// puppeteer-core), VC_URL.

import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const REPO = path.join(HERE, '..', '..');
const TESTDATA = path.join(HERE, 'testdata');
const URL_BASE = process.env.VC_URL || 'http://localhost:2077';
const CHROME = process.env.CHROME || 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
const SLOT = process.argv[2] || '1';
const FILE = `GTAVCsf${SLOT}.b`;

const CHROME_USER_DATA =
  process.env.CHROME_USER_DATA ||
  path.join(process.env.LOCALAPPDATA || path.join(os.homedir(), 'AppData', 'Local'), 'Google', 'Chrome', 'User Data');
const IDB_DIR = 'http_localhost_2077.indexeddb.leveldb';
const IDB_BLOB = 'http_localhost_2077.indexeddb.blob';

const PROFILE = path.join(os.tmpdir(), 'vc-save-extract-profile');
const IDB_DST = path.join(PROFILE, 'Default', 'IndexedDB');

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

async function loadPuppeteer() {
  try { return (await import('puppeteer-core')).default; } catch (e) { /* fuera del arnés */ }
  const dir = process.env.PUPPETEER_DIR ||
    'C:/Users/s0rno/AppData/Local/Temp/opencode/vc-e2e/node_modules';
  const { createRequire } = await import('node:module');
  return createRequire(path.join(dir, 'anchor.js'))('puppeteer-core');
}

// --- 1) copiar la IndexedDB del perfil (solo lectura) ---
const srcLevel = path.join(CHROME_USER_DATA, 'Default', 'IndexedDB', IDB_DIR);
const srcBlob = path.join(CHROME_USER_DATA, 'Default', 'IndexedDB', IDB_BLOB);
if (!fs.existsSync(srcLevel)) {
  console.log('FAIL: no encuentro la IndexedDB del juego en ' + srcLevel);
  process.exit(1);
}
fs.mkdirSync(IDB_DST, { recursive: true });
fs.rmSync(path.join(IDB_DST, IDB_DIR), { recursive: true, force: true });
fs.cpSync(srcLevel, path.join(IDB_DST, IDB_DIR), { recursive: true });
if (fs.existsSync(srcBlob)) {
  fs.rmSync(path.join(IDB_DST, IDB_BLOB), { recursive: true, force: true });
  fs.cpSync(srcBlob, path.join(IDB_DST, IDB_BLOB), { recursive: true });
}
console.log('== IndexedDB copiada a ' + IDB_DST + ' (el perfil original no se toca)');

// --- 2) arrancar el juego con ese perfil y pedir el fichero ---
const puppeteer = await loadPuppeteer();
const browser = await puppeteer.launch({
  executablePath: CHROME,
  headless: 'new',
  protocolTimeout: 60000,
  args: ['--no-sandbox', '--disable-dev-shm-usage', '--use-angle=swiftshader',
    '--enable-unsafe-swiftshader', '--mute-audio', '--window-size=1280,800',
    '--user-data-dir=' + PROFILE],
});
try {
  const page = await browser.newPage();
  await page.setViewport({ width: 1280, height: 800 });
  await page.goto(URL_BASE + '/?autostart=1', { waitUntil: 'domcontentloaded', timeout: 60000 });
  // El IDBFS se monta en preRun; basta con que el runtime esté vivo y poblado.
  let listo = false;
  for (let i = 0; i < 60; i++) {
    await sleep(2000);
    const st = await page.evaluate(() => {
      try { return { booted: !!window.__gameBooted, files: FS.readdir('/userfiles') }; }
      catch (e) { return { booted: !!window.__gameBooted, err: String(e) }; }
    }).catch(() => null);
    if (st && st.files && st.files.length) { listo = true; break; }
    if (st && st.booted && i > 20) { listo = true; break; }
  }
  const out = await page.evaluate(async (name) => {
    try {
      const data = FS.readFile('/userfiles/' + name);
      // a trozos: String.fromCharCode(...200k) revienta la pila
      let bin = '';
      for (let i = 0; i < data.length; i += 8192)
        bin += String.fromCharCode.apply(null, data.subarray(i, i + 8192));
      return { ok: true, b64: btoa(bin) };
    } catch (e) { return { ok: false, err: String(e) }; }
  }, FILE);
  if (!out.ok) {
    const ls = await page.evaluate(() => { try { return FS.readdir('/userfiles'); } catch (e) { return String(e); } });
    console.log('FAIL: no pude leer /userfiles/' + FILE + ' (' + out.err + '). Contenido: ' + JSON.stringify(ls));
    await browser.close().catch(() => {});
    process.exit(1);
  }
  fs.mkdirSync(TESTDATA, { recursive: true });
  const dst = path.join(TESTDATA, FILE);
  fs.writeFileSync(dst, Buffer.from(out.b64, 'base64'));
  console.log('== guardado en ' + dst + ' (' + Math.round(fs.statSync(dst).size / 1024) + ' KB)');
  console.log('   El test tools/slot0-load-test.mjs lo encontrará solo.');
} catch (e) {
  console.log('FAIL: ' + String(e).slice(0, 300));
  await browser.close().catch(() => {});
  process.exit(1);
}
await browser.close().catch(() => {});
process.exit(0);
