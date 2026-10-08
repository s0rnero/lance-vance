// Sonda de vehículos nuevos (Vice Extended): carga la partida del slot 0, teclea
// el cheat CRAZYRIDES (crea los 8 coches/motos del mod alrededor del jugador) y
// deja capturas en disco. Falla si algún TXD/modelo nuevo no carga, si falta un
// fichero en el FS virtual o si la pestaña tira un RuntimeError.
//
// Por qué existe: los vehículos del mod no están en su `default.ide` sino en un
// `newVehicles.ide` en formato Maxo's Vehicle Loader, así que su entrada tiene
// tres partes que se pueden romper por separado:
//   1. datos (default.ide + handling.cfg + carcols.dat, convertidos por
//      tools/import_mvl_vehicles.py) y los DFF/TXD importados a gta3.img;
//   2. código (HandlingMgr: 8 IDs nuevos, con las 4 motos fuera del rango
//      contiguo original; AudioLogic: filas de sonido propias porque el índice
//      de esta tabla es el ID de modelo y 6500+ se salía);
//   3. el camino real de creación (CAutomobile/CBike), que es donde un
//      `m_wheelId` inexistente o un handling mal indexado revientan.
// Ver .agents/plans/vice-extended-inclusion.md.
//
// Uso:
//   node gta_vc_browser/tools/vehicles-smoke-test.mjs
// Requisitos: servidor de desarrollo en marcha (cd gta_vc_browser/web && npm run dev)
// y Chrome instalado.
//
// Variables de entorno (todas opcionales):
//   VC_URL, VC_SAVE, VC_SHOTS, VC_PROFILE, VC_ODTRACE, PUPPETEER_DIR, CHROME
//   (mismos valores por defecto que weapons-smoke-test.mjs)

import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const WEB = path.join(HERE, '..', 'web');
const URL_BASE = process.env.VC_URL || 'http://localhost:2077';
const ODTRACE = process.env.VC_ODTRACE || path.join(WEB, 'odtrace.log');
const PROFILE = process.env.VC_PROFILE || path.join(os.tmpdir(), 'vc-vehicles-test-profile');
const CHROME = process.env.CHROME || 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
const SAVE = process.env.VC_SAVE || path.join(HERE, 'testdata', 'GTAVCsf1.b');
const SHOTS = process.env.VC_SHOTS || path.join(os.tmpdir(), 'vc-vehicles');

const FRAC = {
  start: [0.5, 325 / 800],
  load: [0.5, 345 / 800],
  slot1: [0.3125, 180 / 800],
  yes: [0.5, 418 / 800],
};

// El cheat se escribe en claro: el motor guarda las teclas y las compara con el
// literal codificado (ver CPad::AddToPCCheatString).
const CHEAT = 'CRAZYRIDES';
// Nombre del modelo = nombre del .txd, así que el log TXDIN los lista.
const TXD_VEHICULOS = ['streetfi', 'peren2', 'trash2', 'hellenbach', 'premier',
  'manchez', 'wintergreen', 'polwintergreen'];
const FS_FICHEROS = TXD_VEHICULOS.flatMap((n) => [
  '/models/gta3.img/' + n + '.dff', '/models/gta3.img/' + n + '.txd']);

const BOOT_TIMEOUT_MS = 150000;
const LOAD_TIMEOUT_MS = 180000;

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

async function loadPuppeteer() {
  try { return (await import('puppeteer-core')).default; } catch (e) { /* fuera del arnés */ }
  const dir = process.env.PUPPETEER_DIR ||
    'C:/Users/s0rno/AppData/Local/Temp/opencode/vc-e2e/node_modules';
  const { createRequire } = await import('node:module');
  const req = createRequire(path.join(dir, 'anchor.js'));
  return req('puppeteer-core');
}

function traceSince(marker, pattern) {
  let txt = '';
  try { txt = fs.readFileSync(ODTRACE, 'utf8'); } catch (e) { return []; }
  const idx = txt.lastIndexOf(marker);
  const part = idx >= 0 ? txt.slice(idx) : txt;
  return part.split('\n').filter((l) => pattern.test(l));
}

function traceSize() {
  try { return fs.statSync(ODTRACE).size; } catch (e) { return 0; }
}

function traceFrom(offset) {
  try {
    const t = fs.readFileSync(ODTRACE, 'utf8');
    return t.length > offset ? t.slice(offset) : '';
  } catch (e) { return ''; }
}

if (!fs.existsSync(SAVE)) {
  console.log('FAIL: no encuentro la partida a sembrar (' + SAVE + '). Pasa VC_SAVE=<ruta>.');
  process.exit(1);
}
console.log('== save a cargar: ' + SAVE);

const puppeteer = await loadPuppeteer();
const errors = [];
const consola = [];
let browser;
let ok = false;

try {
  fs.mkdirSync(SHOTS, { recursive: true });
  browser = await puppeteer.launch({
    executablePath: CHROME,
    headless: 'new',
    protocolTimeout: 60000,
    args: ['--no-sandbox', '--disable-dev-shm-usage', '--use-angle=swiftshader',
      '--enable-unsafe-swiftshader', '--mute-audio', '--window-size=1280,800',
      '--user-data-dir=' + PROFILE],
  });
  const page = await browser.newPage();
  await page.setViewport({ width: 1280, height: 800 });
  page.on('console', (m) => {
    const t = m.text();
    consola.push(t.slice(0, 300));
    if (/memory access out of bounds|RuntimeError|Aborted|out of bounds/i.test(t)) errors.push(t.slice(0, 300));
  });
  page.on('pageerror', (e) => errors.push('PAGEERROR ' + String(e).slice(0, 300)));

  console.log('== arrancando ' + URL_BASE + ' (perfil ' + PROFILE + ')');
  await page.goto(URL_BASE + '/?autostart=1', { waitUntil: 'domcontentloaded', timeout: 60000 });

  const b64 = fs.readFileSync(SAVE).toString('base64');
  const write = async () => page.evaluate(async (b) => {
    try {
      if (typeof FS === 'undefined') return 'sin FS';
      const bin = atob(b);
      const arr = new Uint8Array(bin.length);
      for (let i = 0; i < bin.length; i++) arr[i] = bin.charCodeAt(i);
      FS.writeFile('/userfiles/GTAVCsf1.b', arr);
      await new Promise((res) => FS.syncfs(false, res));
      return 'ok size=' + FS.stat('/userfiles/GTAVCsf1.b').size;
    } catch (e) { return 'ERR ' + e; }
  }, b64).catch((e) => 'ERR ' + e);
  let seed = 'no';
  for (let i = 0; i < 90; i++) { seed = await write(); if (String(seed).startsWith('ok')) break; await sleep(1000); }
  console.log('== save sembrado: ' + seed);

  const t0 = Date.now();
  let menu = false;
  while (Date.now() - t0 < BOOT_TIMEOUT_MS) {
    const log = await page.$eval('#gamelog', (e) => e.textContent).catch(() => '');
    const st = (log.match(/state \d+/g) || []).slice(-3);
    if (st.length === 3 && st.every((s) => s === 'state 7')) { menu = true; break; }
    await sleep(2000);
  }
  console.log('== menú listo: ' + (menu ? 'sí' : 'NO (los clics pueden perderse)'));
  await sleep(4000);

  const seeded = await write();
  console.log('== save listo: ' + seeded);
  const rect = await page.evaluate(() => {
    const c = document.querySelector('.vc-canvas');
    if (!c) return null;
    const r = c.getBoundingClientRect();
    return [r.x, r.y, r.width, r.height];
  });
  if (!rect) { console.log('FAIL: no encuentro el canvas del juego (.vc-canvas)'); process.exit(1); }
  const at = ([fx, fy]) => [rect[0] + fx * rect[2], rect[1] + fy * rect[3]];
  const slowClick = async (x, y) => {
    await page.mouse.move(x, y, { steps: 3 }).catch(() => {});
    await sleep(700);
    await page.mouse.down().catch(() => {});
    await sleep(350);
    await page.mouse.up().catch(() => {});
    await sleep(3000);
  };

  await slowClick(...at(FRAC.start));
  await slowClick(...at(FRAC.load));
  await slowClick(...at(FRAC.slot1));
  await slowClick(...at(FRAC.yes));
  let requested = false;
  for (let i = 0; i < 12 && !requested; i++) {
    await sleep(2500);
    requested = traceSince('rotado: sesion nueva', /WR load-req/).length > 0;
    if (!requested && i === 3) await slowClick(...at(FRAC.yes));
  }

  const tLoad = Date.now();
  let pass = false, animFail = false;
  while (Date.now() - tLoad < LOAD_TIMEOUT_MS) {
    if (traceSince('rotado: sesion nueva', /ODANIMFAIL/).length) { animFail = true; break; }
    if (traceSince('rotado: sesion nueva', /FPHASE/).length >= 3) { pass = true; break; }
    if (errors.some((e) => /out of bounds|RuntimeError|Aborted/.test(e))) break;
    await sleep(3000);
  }
  console.log('== en partida: ' + (pass ? 'sí' : 'NO'));

  const marcaVehiculos = traceSize();
  if (pass) {
    await page.mouse.move(rect[2] / 2, rect[3] / 2, { steps: 2 }).catch(() => {});
    await page.mouse.click(rect[2] / 2, rect[3] / 2).catch(() => {});
    await sleep(600);
    for (const ch of CHEAT) {
      await page.keyboard.press('Key' + ch).catch(() => {});
      await sleep(140);
    }
    console.log('== cheat tecleado: ' + CHEAT);
    await sleep(6000);
    // Diagnóstico temprano: si el motor se cuelga al aparecer los coches, esto
    // deja ver la última línea de consola y de odtrace ANTES de tocar capturas.
    console.log('== consola (últimas): ' + (consola.slice(-8).join(' | ') || '(vacía)'));
    console.log('== odtrace (últimas): ' + (traceFrom(marcaVehiculos).split('\n').filter(Boolean).slice(-8).join(' | ') || '(vacío)'));
    // Los coches caen desde 4 m: unos segundos de física dejan ver si un
    // handling mal indexado (motos) o una rueda ausente revientan al asentarse.
    await page.screenshot({ path: path.join(SHOTS, '00-caida.png'), timeout: 20000 }).catch(() => {});
    await sleep(12000);
    await page.screenshot({ path: path.join(SHOTS, '01-asentados.png'), timeout: 20000 }).catch(() => {});
    // Un giro de cámara para tener los 8 desde otro ángulo (flechas = cámara).
    for (const tecla of ['ArrowLeft', 'ArrowLeft', 'ArrowRight']) {
      await page.keyboard.down(tecla).catch(() => {});
      await sleep(700);
      await page.keyboard.up(tecla).catch(() => {});
      await sleep(400);
    }
    await sleep(3000);
    await page.screenshot({ path: path.join(SHOTS, '02-otro-angulo.png'), timeout: 20000 }).catch(() => {});
    await sleep(8000);
  }

  const tail = traceFrom(marcaVehiculos);
  const txd = [...new Set((tail.match(/TXDIN txd=([a-z0-9_]+)/gi) || []).map((x) => x.split('=')[1].toLowerCase()))];
  const faltanTxd = TXD_VEHICULOS.filter((t) => !txd.includes(t));
  const fsState = await page.evaluate((files) => files.map((f) => {
    try { return f.replace('/models/gta3.img/', '') + '=' + FS.stat(f).size; } catch (e) { return f.replace('/models/gta3.img/', '') + '=FALTA'; }
  }), FS_FICHEROS).catch((e) => ['(FS no consultable: ' + String(e.message || e).slice(0, 120) + ')']);
  const fsOk = fsState.every((x) => !x.endsWith('=FALTA') && !x.startsWith('('));
  // `stream` de modelos: la carga del DFF deja línea propia en odtrace.
  const dff = TXD_VEHICULOS.filter((n) => tail.includes(n + '.dff') || tail.toLowerCase().includes(n + '.dff'));

  console.log('== vehículos: TXD vistos tras el cheat: ' + (txd.length ? txd.join(', ') : '(ninguno)'));
  console.log('== vehículos: TXD que faltan: ' + (faltanTxd.length ? faltanTxd.join(', ') : 'ninguno'));
  console.log('== vehículos: DFF con línea de carga: ' + (dff.length ? dff.join(', ') : '(ninguno)'));
  console.log('== vehículos: FS -> ' + fsState.length + ' ficheros, ' + fsState.slice(0, 3).join(' ') + ' ...');
  console.log('== vehículos: capturas en ' + SHOTS);
  console.log('== líneas FPHASE: ' + traceSince('rotado: sesion nueva', /FPHASE/).length);
  console.log('== errores de página: ' + (errors.length ? errors.slice(0, 3).join(' | ') : 'ninguno'));
  if (errors.length) console.log('\nERROR:\n' + errors.slice(0, 3).join('\n'));

  ok = pass && requested && !animFail && !errors.length && fsOk &&
    faltanTxd.length === 0 && txd.length > 0;
  console.log('\n' + (ok ? 'PASS' : 'FAIL') + ' — vehículos nuevos ' + (ok
    ? 'creados y asentados sin crash'
    : !pass ? 'sin llegar a partida'
      : animFail ? 'con bloques de anim sin cargar (ODANIMFAIL)'
        : errors.length ? 'con RuntimeError en la pestaña'
          : faltanTxd.length ? 'sin cargar estos TXD: ' + faltanTxd.join(', ')
            : !fsOk ? 'con ficheros nuevos ausentes del FS virtual'
              : 'sin teclear/ejecutar el cheat'));
  await browser.close().catch(() => {});
  process.exit(ok ? 0 : 1);
} catch (e) {
  console.log('FAIL: excepción del arnés: ' + String(e).slice(0, 400));
  if (browser) await browser.close().catch(() => {});
  process.exit(1);
}
