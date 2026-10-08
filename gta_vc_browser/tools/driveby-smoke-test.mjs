// Sonda del drive-by (sección 2 del plan de mecánicas, bloque P2).
//
// Qué mide: si con la pistola en la mano se dispara desde coche/moto/barco
// (VICEEXT_DRIVEBY_WIDE) y si la SMG sigue funcionando (no regresión). El motor
// publica la traza por ODTRACES a `odtrace.log` (una línea por segundo
// conduciendo, una por disparo y una por entrada/salida de vehículo; ver
// `CPed::DriveByTraceState`/`DriveByTraceShot` en src/peds/Ped.cpp).
//
// Dos modos (VC_DRIVEBY_MODE):
//   baseline → build con el define APAGADO. PASS = se entró en un vehículo con
//              la pistola (outcome=remove-model, la escondía vanilla), NO hubo ni
//              un disparo con el slot 3 (pistola) pese a tener el gatillo
//              apretado, y SÍ hubo disparos con la SMG en la fase de regresión:
//              eso demuestra que el arnés sabe provocar disparos y que 0 disparos
//              con pistola es la puerta del motor, no un fallo de la sonda.
//   feature  → build con VICEEXT_DRIVEBY_WIDE. PASS = hay disparos con pistola
//              desde coche Y desde moto (con animación lateral/delantera, no la
//              de conducir), la pistola se queda en la mano al entrar
//              (outcome=keep-weapon) y la SMG sigue disparando (slot 5).
//              Si solo se pudo medir una clase de vehículo -> PARCIAL (código 3),
//              que no es FAIL: la corrida no creó la condición, se repite.
//
// AVISO DE COSTE: esta sonda abre Chrome con swiftshader y consume CPU/RAM de
// verdad (4-8 min por corrida). El jugador pidió expresamente no abusar de estas
// sondas. La ruta preferida para confirmar el bloque es jugar una partida y
// pasar `node gta_vc_browser/tools/driveby-log-check.mjs` (lee odtrace.log, no
// abre navegador). Úsala solo con la máquina libre y avisando.
//
// Uso:
//   node gta_vc_browser/tools/driveby-smoke-test.mjs                       (baseline)
//   VC_DRIVEBY_MODE=feature node gta_vc_browser/tools/driveby-smoke-test.mjs
// Requisitos: servidor de desarrollo en marcha (cd gta_vc_browser/web && npm run dev),
// build con la instrumentación y Chrome instalado.
//
// INMUNIDAD A LAS RECOMPILACIONES (importante en este checkout: somos 3 agentes
// compartiendo `web/public/build/` y un build nuevo cada pocos minutos dejaba
// corridas a medias). Al empezar, la sonda copia reVC.{js,wasm,data} a una
// instantánea en %TEMP% y sirve ESA copia desde su propio puerto local
// (redirigiendo las peticiones `/build/reVC.*` de la página). Así el build que
// mide es exactamente el que midió al empezar. `VC_DRIVEBY_LIVE_BUILD=1` vuelve
// al directorio compartido (como las otras sondas).
//
// Variables de entorno (mismas que las otras sondas, más las propias):
//   VC_URL, VC_SAVE, VC_SHOTS, VC_PROFILE, VC_ODTRACE, PUPPETEER_DIR, CHROME
//   VC_DRIVEBY_MODE=baseline|feature, VC_DRIVEBY_SECONDS (fase pistola, 140),
//   VC_DRIVEBY_SMG_SECONDS (fase de regresión, 70), VC_DRIVEBY_TAG,
//   VC_DRIVEBY_LIVE_BUILD=1, VC_DRIVEBY_SNAP_PORT (2078)

import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import http from 'node:http';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const WEB = path.join(HERE, '..', 'web');
const URL_BASE = process.env.VC_URL || 'http://localhost:2077';
const ODTRACE = process.env.VC_ODTRACE || path.join(WEB, 'odtrace.log');
const PROFILE = process.env.VC_PROFILE || path.join(os.tmpdir(), 'vc-driveby-test-profile');
const CHROME = process.env.CHROME || 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
const SAVE = process.env.VC_SAVE || path.join(HERE, 'testdata', 'GTAVCsf1.b');
const BUILD_LIVE = path.join(WEB, 'public', 'build');
const LIVE = process.env.VC_DRIVEBY_LIVE_BUILD === '1';
const SNAP = path.join(os.tmpdir(), 'vc-driveby-snap');
const SNAP_PORT = Number(process.env.VC_DRIVEBY_SNAP_PORT || 2078);
const BUILD_FILES = ['reVC.js', 'reVC.wasm', 'reVC.data'];
const SHOTS = process.env.VC_SHOTS || path.join(os.tmpdir(), 'vc-driveby');
const MODE = process.env.VC_DRIVEBY_MODE === 'feature' ? 'feature' : 'baseline';
const PISTOL_S = Number(process.env.VC_DRIVEBY_SECONDS || 140);
const SMG_S = Number(process.env.VC_DRIVEBY_SMG_SECONDS || 70);
// Etiqueta de sesión: el dev server comparte odtrace.log entre pestañas/agentes.
const TAG = Number(process.env.VC_DRIVEBY_TAG || (1000 + Math.floor(Math.random() * 900000)));

const FRAC = {
  start: [0.5, 325 / 800],
  load: [0.5, 345 / 800],
  slot1: [0.3125, 180 / 800],
  yes: [0.5, 418 / 800],
};

// Los cheats se escriben en claro: el motor guarda las teclas y las compara con
// el literal codificado (ver CPad::AddToPCCheatString).
const CHEAT_HEALTH = 'ASPIRINE';             // sobrevivir al tiroteo
const CHEAT_ARMOUR = 'PRECIOUSPROTECTION';
const CHEAT_RIDES = 'CRAZYRIDES';            // vehículos alrededor del jugador
const CHEAT_PISTOL = 'CRAZYPISTOL';          // P2: SOLO la pistola (WeaponCheat5)
const CHEAT_TOOLS = 'CRAZYTOOLS';            // pistola + SMG (fase de regresión)

// Números de arma del enum (src/weapons/WeaponType.h), solo para el informe.
const WEP_NAMES = { 48: 'Beretta', 51: 'Uziold', 52: 'Ak47' };
const wepName = (n) => WEP_NAMES[n] || ('wep' + n);

const SLOT_HANDGUN = 3;
const SLOT_SMG = 5;

const BOOT_TIMEOUT_MS = 150000;
const LOAD_TIMEOUT_MS = 180000;
const SAMPLE_MS = 750;
// Si no llega ninguna línea `DRIVEBY state` en este margen, se considera que el
// jugador ya no está sentado en un vehículo (la traza solo sale conduciendo).
const IN_VEHICLE_MS = 5000;
const EXIT_EVERY_MS = 28000;   // salir y volver a entrar = probar otro vehículo
const UNKNOWN_ENTER_S = 4;     // reintento de Enter mientras se anda

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

async function loadPuppeteer() {
  try { return (await import('puppeteer-core')).default; } catch (e) { /* fuera del arnés */ }
  const dir = process.env.PUPPETEER_DIR ||
    'C:/Users/s0rno/AppData/Local/Temp/opencode/vc-e2e/node_modules';
  const { createRequire } = await import('node:module');
  const req = createRequire(path.join(dir, 'anchor.js'));
  return req('puppeteer-core');
}

function traceSize() {
  try { return fs.statSync(ODTRACE).size; } catch (e) { return 0; }
}

function traceSince(marker, pattern) {
  let txt = '';
  try { txt = fs.readFileSync(ODTRACE, 'utf8'); } catch (e) { return []; }
  const idx = txt.lastIndexOf(marker);
  const part = idx >= 0 ? txt.slice(idx) : txt;
  return part.split('\n').filter((l) => pattern.test(l));
}

if (!fs.existsSync(SAVE)) {
  console.log('FAIL: no encuentro la partida a sembrar (' + SAVE + '). Pasa VC_SAVE=<ruta>.');
  process.exit(1);
}

// Huella de un directorio de build (tamaño + mtime), para saber si alguien
// recompiló. En modo instantánea se mide la copia (que no puede cambiar) y se
// informa aparte de lo que haga el directorio compartido.
function buildFingerprint(dir) {
  const out = {};
  for (const f of BUILD_FILES) {
    try { const st = fs.statSync(path.join(dir, f)); out[f] = st.size + '@' + st.mtime.toISOString(); }
    catch (e) { out[f] = 'FALTA'; }
  }
  return out;
}

// Instantánea del build: copia los tres artefactos a %TEMP% y los sirve por
// HTTP local. Es lo que hace que una recompilación ajena no invalide la corrida.
if (!LIVE) {
  fs.mkdirSync(SNAP, { recursive: true });
  for (const f of BUILD_FILES) {
    try { fs.copyFileSync(path.join(BUILD_LIVE, f), path.join(SNAP, f)); }
    catch (e) { console.log('FAIL: no puedo copiar ' + f + ' del build compartido: ' + e); process.exit(1); }
  }
}
const SERVED = LIVE ? BUILD_LIVE : SNAP;
const BUILD0 = buildFingerprint(SERVED);
console.log('== build medido: ' + BUILD0['reVC.wasm'] + (LIVE ? ' (directorio compartido)' : ' (instantánea en ' + SNAP + ')'));
const wasmHasTrace = (() => {
  try { return fs.readFileSync(path.join(SERVED, 'reVC.wasm')).includes(Buffer.from('DRIVEBY state tag=')); }
  catch (e) { return false; }
})();
if (!wasmHasTrace) {
  console.log('FAIL: el build medido NO trae la traza de drive-by de Ped.cpp (recompila).');
  process.exit(1);
}
const wasmHasCheat = (() => {
  try { return fs.readFileSync(path.join(SERVED, 'reVC.wasm')).includes(Buffer.from('OT[TVk')); }
  catch (e) { return false; }
})();
if (!wasmHasCheat) {
  console.log('FAIL: el build medido NO trae el cheat CRAZYPISTOL (WeaponCheat5 en Pad.cpp).');
  process.exit(1);
}

// Servidor de la instantánea: los tres ficheros, con rangos (el cargador de
// Emscripten los usa) y CORS (la página los pide desde otro origen).
function startSnapServer() {
  const server = http.createServer((req, res) => {
    const name = path.basename(decodeURIComponent((req.url || '/').split('?')[0]));
    const file = path.join(SNAP, name);
    if (!BUILD_FILES.includes(name) || !fs.existsSync(file)) {
      res.writeHead(404, { 'Access-Control-Allow-Origin': '*' });
      return res.end();
    }
    const st = fs.statSync(file);
    const headers = {
      'Content-Type': name.endsWith('.wasm') ? 'application/wasm'
        : name.endsWith('.js') ? 'text/javascript' : 'application/octet-stream',
      'Access-Control-Allow-Origin': '*',
      'Accept-Ranges': 'bytes',
      'Cache-Control': 'no-store',
    };
    let start = 0, end = st.size - 1, status = 200;
    const range = req.headers.range;
    if (range) {
      const m = /bytes=(\d*)-(\d*)/.exec(range);
      if (m) {
        status = 206;
        if (m[1]) start = Number(m[1]);
        if (m[2]) end = Math.min(Number(m[2]), st.size - 1);
      }
    }
    headers['Content-Length'] = String(Math.max(0, end - start + 1));
    if (status === 206) headers['Content-Range'] = 'bytes ' + start + '-' + end + '/' + st.size;
    res.writeHead(status, headers);
    if (req.method === 'HEAD') return res.end();
    fs.createReadStream(file, { start, end }).pipe(res);
  });
  return new Promise((resolve, reject) => {
    server.on('error', reject);
    server.listen(SNAP_PORT, '127.0.0.1', () => resolve(server));
  });
}
console.log('== modo: ' + MODE + ' (fase pistola ' + PISTOL_S + ' s + regresión SMG ' + SMG_S + ' s)');
console.log('== save a cargar: ' + SAVE);

const puppeteer = await loadPuppeteer();
let snapServer = null;
if (!LIVE) {
  try {
    snapServer = await startSnapServer();
    console.log('== instantánea del build servida en http://127.0.0.1:' + SNAP_PORT + '/ (inmune a recompilaciones)' +
      '; build compartido ahora: ' + buildFingerprint(BUILD_LIVE)['reVC.wasm']);
  } catch (e) {
    console.log('FAIL: no puedo levantar el servidor de la instantánea en el puerto ' + SNAP_PORT +
      ': ' + String(e).slice(0, 200));
    process.exit(1);
  }
}
const errors = [];
const consola = [];
let browser;
let ok = false;
let foreign = 0;

// ---- análisis de la traza ---------------------------------------------------
// Líneas del motor (ver Ped.cpp):
//   DRIVEBY state tag=.. t=.. veh=.. wep=.. slot=.. ammo=.. fire=.. lookL=.. lookR=.. model=.. speed=..
//   DRIVEBY shot tag=.. t=.. veh=.. wep=.. slot=.. anim=.. ammo=..
//   DRIVEBY enter|exit tag=.. t=.. wep=.. slot=.. ammo=.. [rev=..] outcome=..
//     (rev=2 es el build con el arreglo de precedencia; en builds viejos no viene)
const RE_STATE = /^DRIVEBY state tag=(\d+) t=(\d+) veh=(\w+) wep=(-?\d+) slot=(-?\d+) ammo=(-?\d+) fire=(\d) lookL=(\d) lookR=(\d) model=(-?\d+) speed=(-?[\d.]+)$/;
const RE_SHOT = /^DRIVEBY shot tag=(\d+) t=(\d+) veh=(\w+) wep=(-?\d+) slot=(-?\d+) anim=([\w-]+) ammo=(-?\d+)$/;
const RE_VEH = /^DRIVEBY (enter|exit) tag=(\d+) t=(\d+) wep=(-?\d+) slot=(-?\d+) ammo=(-?\d+)(?: rev=(\d+))? outcome=([\w-]+)$/;

function parseLine(line) {
  let m = RE_STATE.exec(line);
  if (m) {
    return { kind: 'state', tag: +m[1], t: +m[2], veh: m[3], wep: +m[4], slot: +m[5],
      ammo: +m[6], fire: +m[7], lookL: +m[8], lookR: +m[9], model: +m[10], speed: +m[11] };
  }
  m = RE_SHOT.exec(line);
  if (m) {
    return { kind: 'shot', tag: +m[1], t: +m[2], veh: m[3], wep: +m[4], slot: +m[5],
      anim: m[6], ammo: +m[7] };
  }
  m = RE_VEH.exec(line);
  if (m) {
    return { kind: m[1], tag: +m[2], t: +m[3], wep: +m[4], slot: +m[5], ammo: +m[6],
      rev: m[7] ? +m[7] : 1, outcome: m[8] };
  }
  return null;
}

// Resumen de la corrida: qué se intentó y qué salió.
function summarise(samples) {
  const states = samples.filter((s) => s.kind === 'state');
  const shots = samples.filter((s) => s.kind === 'shot');
  const enters = samples.filter((s) => s.kind === 'enter');
  const exits = samples.filter((s) => s.kind === 'exit');

  const byKey = new Map();
  for (const s of shots) {
    const k = s.veh + '|slot' + s.slot + '|' + s.anim;
    byKey.set(k, (byKey.get(k) || 0) + 1);
  }
  const pistolShots = shots.filter((s) => s.slot === SLOT_HANDGUN);
  const smgShots = shots.filter((s) => s.slot === SLOT_SMG);
  const pistolVeh = new Set(pistolShots.map((s) => s.veh));
  const smgVeh = new Set(smgShots.map((s) => s.veh));

  // ¿Se llegó a apretar el gatillo con la pistola en un vehículo? Sin esto, un
  // "0 disparos" no significa nada: la sonda pudo no haber creado la condición.
  const pistolTries = states.filter((s) => s.slot === SLOT_HANDGUN && s.fire === 1);
  const smgTries = states.filter((s) => s.slot === SLOT_SMG && s.fire === 1);

  return {
    states, shots, enters, exits, byKey,
    vehClasses: [...new Set(states.map((s) => s.veh))],
    models: [...new Set(states.map((s) => s.model))],
    secondsInVehicle: states.length,
    pistolShots, smgShots, pistolVeh: [...pistolVeh], smgVeh: [...smgVeh],
    pistolTries: pistolTries.length, smgTries: smgTries.length,
    lookSeenL: states.some((s) => s.lookL === 1),
    lookSeenR: states.some((s) => s.lookR === 1),
    fireSeen: states.some((s) => s.fire === 1),
    keepWeapon: enters.filter((e) => e.outcome === 'keep-weapon').length,
    removeModel: enters.filter((e) => e.outcome === 'remove-model').length,
    switchSmg: enters.filter((e) => e.outcome === 'switch-smg').length,
    restoreStored: exits.filter((e) => e.outcome === 'restore-stored').length,
    addModel: exits.filter((e) => e.outcome === 'add-model').length,
    keepSmgOut: exits.filter((e) => e.outcome === 'keep-smg').length,
    outcomes: enters.map((e) => 'slot' + e.slot + ':' + e.outcome),
    maxSpeed: Math.max(0, ...states.map((s) => s.speed)),
  };
}

let odPos = traceSize();

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
  if (!LIVE) {
    // El motor se pide a la instantánea; el resto (streamed/, odtrace, módulos
    // de Vite) sigue saliendo del servidor de desarrollo.
    await page.setRequestInterception(true);
    page.on('request', (req) => {
      const m = /\/build\/(reVC\.(?:js|wasm|data))(\?.*)?$/.exec(req.url());
      if (m && BUILD_FILES.includes(m[1])) {
        return req.continue({ url: 'http://127.0.0.1:' + SNAP_PORT + '/' + m[1] })
          .catch(() => req.continue().catch(() => {}));
      }
      return req.continue().catch(() => {});
    });
  }
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

  const navClick = async () => {
    await slowClick(...at(FRAC.start));
    await slowClick(...at(FRAC.load));
    await slowClick(...at(FRAC.slot1));
    await slowClick(...at(FRAC.yes));
  };
  await navClick();
  let requested = false;
  for (let i = 0; i < 12 && !requested; i++) {
    await sleep(2500);
    requested = traceSince('rotado: sesion nueva', /WR load-req/).length > 0;
    if (!requested && i === 3) { await slowClick(...at(FRAC.yes)); }
    if (!requested && i === 7) {
      await page.screenshot({ path: path.join(SHOTS, 'nav-reintento.png'), timeout: 20000 }).catch(() => {});
      console.log('== sin petición de carga: se repite el flujo del menú');
      await navClick();
    }
  }

  const tLoad = Date.now();
  let pass = false;
  while (Date.now() - tLoad < LOAD_TIMEOUT_MS) {
    if (consola.some((l) => /\[web\] tick .* state 9\b/.test(l))) { pass = true; break; }
    if (traceSince('rotado: sesion nueva', /FPHASE/).length >= 3) { pass = true; break; }
    if (errors.some((e) => /out of bounds|RuntimeError|Aborted/.test(e))) break;
    await sleep(2000);
  }
  console.log('== en partida: ' + (pass ? 'sí' : 'NO'));
  if (!pass) {
    await page.screenshot({ path: path.join(SHOTS, '01-sin-partida.png'), timeout: 20000 }).catch(() => {});
    console.log('FAIL: no se llegó a partida; traza reciente: ' +
      (consola.slice(-6).join(' | ') || '(vacía)'));
    await browser.close().catch(() => {});
    process.exit(1);
  }

  const samples = [];
  let stateCount = 0;              // muestras `state` recibidas (1/s conduciendo)
  const readNew = () => {
    let txt = '';
    try { txt = fs.readFileSync(ODTRACE, 'utf8'); } catch (e) { return; }
    if (txt.length < odPos) odPos = 0;          // el log rota por sesión
    const part = txt.slice(odPos);
    odPos = txt.length;
    if (!part) return;
    for (const line of part.split('\n')) {
      if (!line) continue;
      const m = /^(\d{4}-\d{2}-\d{2}T[\d:.]+Z)\s+(.*)$/.exec(line);
      const payload = m ? m[2] : line;
      if (payload.indexOf('DRIVEBY') !== 0) continue;
      const s = parseLine(payload);
      if (!s) continue;
      if (s.tag !== TAG) { foreign++; continue; }   // otra pestaña/agente
      samples.push(s);
      if (s.kind === 'state') stateCount++;
    }
  };
  const drain = async () => { readNew(); };

  await page.mouse.move(rect[2] / 2, rect[3] / 2, { steps: 2 }).catch(() => {});
  await page.mouse.click(rect[2] / 2, rect[3] / 2).catch(() => {});
  await sleep(800);

  if (!LIVE) {
    // Lo que importa es que lo SERVIDO no haya cambiado (por construcción, la
    // instantánea no cambia). Se informa de si el directorio compartido se movió.
    const liveNow = buildFingerprint(BUILD_LIVE)['reVC.wasm'];
    console.log('== instantánea servida intacta: ' + buildFingerprint(SERVED)['reVC.wasm'] +
      (liveNow === BUILD0['reVC.wasm'] ? '' : '; el build compartido cambió mientras cargaba (' + liveNow + '), lo cual ya no afecta a esta corrida'));
  } else {
    const buildCargado = JSON.stringify(BUILD0) === JSON.stringify(buildFingerprint(SERVED));
    console.log('== build estable durante la carga: ' + (buildCargado ? 'sí' : 'NO (recompilaron cargando: repetir)'));
    if (!buildCargado) {
      await browser.close().catch(() => {});
      process.exit(1);
    }
  }

  await page.evaluate((tag) => { window.__vcDriveByTag = tag; }, TAG).catch(() => {});
  console.log('== etiqueta de sesión: ' + TAG);

  const typeCheat = async (word) => {
    for (const ch of word) {
      await page.keyboard.press('Key' + ch).catch(() => {});
      await sleep(140);
    }
    console.log('== cheat tecleado: ' + word);
    await sleep(600);
  };
  const pressEnter = async () => page.keyboard.press('Enter').catch(() => {});
  // Teclas del mando: disparar en el vehículo = VEHICLE_FIREWEAPON
  // (rsPADINS/'Insert' + LCTRL), mirar de lado = VEHICLE_LOOKLEFT/RIGHT
  // (rsPADEND + 'Q' / rsPADDOWN + 'E'). Se mantienen las dos variantes a la vez
  // (son el mismo mando) para no depender de cuál acepta el port.
  const FIRE_KEYS = ['Insert', 'ControlLeft'];
  const held = [];
  const setHeld = async (keys) => {
    for (const k of held) if (!keys.includes(k)) await page.keyboard.up(k).catch(() => {});
    for (const k of keys) if (!held.includes(k)) await page.keyboard.down(k).catch(() => {});
    held.length = 0;
    held.push(...keys);
  };

  await drain();
  console.log('== traza de arranque: ' + (samples.length ? samples.length + ' líneas' : 'ninguna'));
  await page.screenshot({ path: path.join(SHOTS, '00-antes.png'), timeout: 20000 }).catch(() => {});

  // Supervivencia + vehículos a mano: CRAZYRIDES deja los 8 vehículos del mod
  // alrededor del jugador (coches y la moto policial 6507).
  await typeCheat(CHEAT_ARMOUR);
  await typeCheat(CHEAT_HEALTH);
  await typeCheat(CHEAT_RIDES);
  await typeCheat(CHEAT_PISTOL);

  // Antes de gastar minutos de swiftshader: ¿llega la traza y se puede conducir?
  let sawState = false;
  for (let i = 0; i < 25 && !sawState; i++) {
    await page.keyboard.down('KeyW').catch(() => {});
    if (i % 5 === 4) await pressEnter();
    await sleep(1000);
    await drain();
    sawState = samples.some((s) => s.kind === 'state');
    if (sawState) break;
  }
  await page.keyboard.up('KeyW').catch(() => {});
  if (!sawState) {
    console.log('== aviso: todavía no hay ninguna línea DRIVEBY state (¿se subió a un vehículo?)');
  }

  // ---- fases ---------------------------------------------------------------
  const start = Date.now();
  const phaseEnd = { A: PISTOL_S * 1000, B: (PISTOL_S + SMG_S + 30) * 1000 };
  let phase = 'A';
  let nextHeal = 20, nextEnter = 0, nextExit = 0;
  // "Conduciendo" = llegó una línea `state` (que solo sale dentro de los
  // DoDriveByShootings, es decir, sentado en un vehículo) en los últimos
  // IN_VEHICLE_MS. Con el reloj de pared: el `t=` de la traza es el del motor.
  let lastStateCount = stateCount, lastStateWall = Date.now(), inVehicle = false, exitPending = false;
  let smgCheatDone = false, phaseLogged = '';
  const shotsAt = new Set([10, 40, 80, PISTOL_S, PISTOL_S + 40]);
  const shotSet = () => new Set(samples.filter((s) => s.kind === 'shot').map((s) => s.veh + '|slot' + s.slot));

  while (true) {
    const spent = Date.now() - start;
    // Cambio de fase A -> B: solo cuando ya hay disparos con pistola en las dos
    // clases (o cuando la fase A se agota, que es lo normal).
    const had = shotSet();
    const pistolBoth = had.has('car|slot' + SLOT_HANDGUN) && had.has('bike|slot' + SLOT_HANDGUN);
    if (phase === 'A' && (pistolBoth || spent >= phaseEnd.A)) phase = 'B';
    if (phase === 'B' && (had.has('car|slot' + SLOT_SMG) || had.has('bike|slot' + SLOT_SMG) || spent >= phaseEnd.B)) break;
    if (spent > phaseEnd.B + 30000) break;

    await sleep(SAMPLE_MS);
    await drain();
    const elapsed = Math.round(spent / 1000);
    const now = Date.now();
    if (elapsed >= nextHeal) { nextHeal = elapsed + 20; await typeCheat(CHEAT_HEALTH); }

    if (phase === 'B' && !smgCheatDone) {
      smgCheatDone = true;
      await setHeld([]);
      if (inVehicle) { await pressEnter(); await sleep(4000); }
      console.log('== t=' + elapsed + 's fase B (regresión SMG): cheat ' + CHEAT_TOOLS);
      await typeCheat(CHEAT_TOOLS);
      nextEnter = 0;
    }

    if (stateCount !== lastStateCount) { lastStateCount = stateCount; lastStateWall = now; }
    inVehicle = now - lastStateWall < IN_VEHICLE_MS;

    if (phase !== phaseLogged) {
      phaseLogged = phase;
      console.log('== t=' + elapsed + 's fase ' + phase + (phase === 'A' ? ' (pistola)' : ' (SMG)'));
    }

    if (!inVehicle) {
      // A pie: andar y buscar la puerta de un vehículo, con un poco de zigzag
      // para no entrar siempre en el mismo.
      const zig = Math.floor(elapsed / 6) % 2 ? 'KeyA' : 'KeyD';
      await setHeld(Math.floor(elapsed / 3) % 2 ? ['KeyW'] : ['KeyW', zig]);
      if (elapsed >= nextEnter) {
        nextEnter = elapsed + UNKNOWN_ENTER_S;
        await pressEnter();
      }
    } else {
      // Conduciendo: gatillo apretado y mirada alternando a los dos lados
      // (coche y barco solo disparan mirando de lado; la moto dispara sola hacia
      // delante). Cada ~28 s se sale para poder probar otro vehículo.
      if (elapsed >= nextExit) {
        nextExit = elapsed + EXIT_EVERY_MS / 1000;
        exitPending = true;
      }
      if (exitPending) {
        exitPending = false;
        await setHeld([]);
        await pressEnter();
        await sleep(3500);
        continue;
      }
      const look = Math.floor(elapsed / 8) % 3;
      const lookKey = look === 0 ? 'KeyQ' : look === 1 ? 'KeyE' : null;
      await setHeld(lookKey ? [...FIRE_KEYS, lookKey] : [...FIRE_KEYS]);
    }

    if (shotsAt.has(elapsed)) {
      await page.screenshot({ path: path.join(SHOTS, elapsed + 's.png'), timeout: 20000 }).catch(() => {});
    }
  }
  await setHeld([]);
  await page.screenshot({ path: path.join(SHOTS, '99-final.png'), timeout: 20000 }).catch(() => {});
  const finalLog = await page.evaluate(() => (window.__vcLog || []).slice(-25)).catch(() => []);
  console.log('== consola del motor (últimas): ' + (finalLog.slice(-6).join(' | ') || '(vacía)'));

  // ---- informe ---------------------------------------------------------------
  const r = summarise(samples);
  console.log('== traza: ' + samples.length + ' eventos de esta sesión (' +
    r.states.length + ' s conduciendo, ' + r.shots.length + ' disparos, ' +
    r.enters.length + ' entradas y ' + r.exits.length + ' salidas de vehículo)');
  console.log('== clases de vehículo vistas: ' + (r.vehClasses.length ? r.vehClasses.join(', ') : 'ninguna') +
    ' (modelos ' + (r.models.length ? r.models.join(', ') : '-') + '; velocidad máx ' + r.maxSpeed.toFixed(1) + ')');
  console.log('== entradas: ' + (r.outcomes.length ? r.outcomes.join(', ') : 'ninguna') +
    '  [keep-weapon ' + r.keepWeapon + ', remove-model ' + r.removeModel + ', switch-smg ' + r.switchSmg + ']');
  console.log('== salidas: restore-stored ' + r.restoreStored + ', add-model ' + r.addModel + ', keep-smg ' + r.keepSmgOut);
  console.log('== entradas/salidas con el gatillo apretado: pistola ' + r.pistolTries +
    ' s, SMG ' + r.smgTries + ' s (de ' + r.secondsInVehicle + ' s conduciendo)');
  console.log('== mirada de lado registrada por el motor: izquierda ' + (r.lookSeenL ? 'sí' : 'NO') +
    ', derecha ' + (r.lookSeenR ? 'sí' : 'NO') + '; gatillo ' + (r.fireSeen ? 'sí' : 'NO'));
  if (r.byKey.size) {
    console.log('== disparos por clase/slot/animación:');
    for (const [k, n] of [...r.byKey.entries()].sort()) {
      const [veh, slotS, anim] = k.split('|');
      console.log('   ' + veh + ' ' + slotS.replace('slot', 'slot ') + '  anim ' + anim + '  x' + n);
    }
  } else {
    console.log('== disparos: NINGUNO');
  }
  console.log('== capturas en ' + SHOTS);
  console.log('== errores de página: ' + (errors.length ? errors.slice(0, 3).join(' | ') : 'ninguno'));
  console.log('== líneas de OTRAS sesiones ignoradas: ' + foreign);
  const buildEstable = JSON.stringify(BUILD0) === JSON.stringify(buildFingerprint(SERVED));
  const liveChanged = LIVE ? !buildEstable
    : buildFingerprint(BUILD_LIVE)['reVC.wasm'] !== BUILD0['reVC.wasm'];
  console.log('== build medido intacto al terminar: ' + (buildEstable ? 'sí' : 'no') +
    (LIVE ? '' : ' (el compartido ' + (liveChanged ? 'sÍ cambió mientras corría la sonda' : 'no se movió') +
      '; da igual: la corrida midió la instantánea)'));

  const carPistol = r.pistolShots.filter((s) => s.veh === 'car').length;
  const bikePistol = r.pistolShots.filter((s) => s.veh === 'bike').length;
  const boatPistol = r.pistolShots.filter((s) => s.veh === 'boat').length;
  const carSmg = r.smgShots.filter((s) => s.veh === 'car').length;
  const bikeSmg = r.smgShots.filter((s) => s.veh === 'bike').length;
  const anySmg = r.smgShots.length;
  const animLateral = r.pistolShots.some((s) => ['left', 'right', 'left-lo', 'right-lo'].includes(s.anim));
  const animBike = r.pistolShots.some((s) => s.veh === 'bike' && ['forward', 'lhs', 'rhs'].includes(s.anim));
  console.log('\nRESUMEN ' + MODE + ': pistola -> coche ' + carPistol + ', moto ' + bikePistol +
    ', barco ' + boatPistol + '; SMG (regresión) -> ' + anySmg +
    (anySmg ? ' (coche ' + carSmg + ', moto ' + bikeSmg + ')' : ''));

  if (!r.states.length && !r.enters.length) {
    console.log('\nINCONCLUSA — la corrida no llegó a subirse a ningún vehículo: no hay nada que' +
      ' juzgar. (¿el save dejó al jugador lejos de los vehículos de CRAZYRIDES?)');
    await browser.close().catch(() => {});
    process.exit(3);
  }

  if (MODE === 'baseline') {
    // La línea base necesita las dos mitades: la pistola NO dispara y la SMG SÍ
    // (si la SMG tampoco dispara, el arnés no sabe provocar disparos y el 0 de la
    // pistola no demuestra nada -> INCONCLUSA).
    if (anySmg === 0) {
      console.log('\nINCONCLUSA — ni la pistola ni la SMG dispararon: sin un disparo de control' +
        ' (SMG) no se puede distinguir "la puerta rechaza la pistola" de "el arnés no dispara".' +
        ' Se tecleó el gatillo con pistola ' + r.pistolTries + ' s; mira si llegó la mirada de lado.');
      await browser.close().catch(() => {});
      process.exit(3);
    }
    ok = !errors.length && r.pistolShots.length === 0 && r.removeModel > 0;
    console.log(ok
      ? '\nPASS (línea base) — vanilla con la pistola: entra al vehículo con outcome=remove-model (' +
        r.removeModel + ' veces) y 0 disparos de pistola con el gatillo apretado ' + r.pistolTries +
        ' s, mientras la SMG sí dispara (' + anySmg + '). La puerta de slot 5 es la que corta.'
      : '\nFAIL — ' + (!errors.length
        ? (r.pistolShots.length ? 'la pistola SÍ disparó sin el define (' + r.pistolShots.length + ' veces)'
          : 'no hubo ninguna entrada con outcome=remove-model (¿no se entró con la pistola en la mano?)')
        : 'RuntimeError en la pestaña'));
  } else {
    const mecanismo = carPistol > 0 || bikePistol > 0;
    if (!mecanismo) {
      console.log('\nFAIL — VICEEXT_DRIVEBY_WIDE: no hubo ni un disparo con la pistola' +
        ' (gatillo apretado con pistola ' + r.pistolTries + ' s, mirada de lado ' +
        (r.lookSeenL || r.lookSeenR ? 'sí' : 'NO') + ', entradas ' + n(r.enters.length) + ')' +
        (errors.length ? '; RuntimeError en la pestaña' : ''));
    } else if (carPistol === 0 || bikePistol === 0) {
      console.log('\nPARCIAL — VICEEXT_DRIVEBY_WIDE: disparos con pistola en ' +
        (carPistol ? 'coche (' + carPistol + ')' : 'moto (' + bikePistol + ')') +
        ' pero no en las dos clases (coche ' + carPistol + ', moto ' + bikePistol + ').' +
        ' La mecánica funciona; falta una clase -> repetir la corrida (INCONCLUSA).');
      await browser.close().catch(() => {});
      process.exit(3);
    } else {
      ok = !errors.length && anySmg > 0 && animLateral && animBike && r.keepWeapon > 0;
      console.log(ok
        ? '\nPASS — VICEEXT_DRIVEBY_WIDE: pistola disparando desde coche (' + carPistol + ') y moto (' +
          bikePistol + ') con animación de drive-by (' + (animLateral ? 'lateral' : '?') + '/' +
          (animBike ? 'moto' : '?') + '), la pistola se quedó en la mano al entrar (' + r.keepWeapon +
          ' veces) y la SMG sigue disparando (' + anySmg + ')' +
          (boatPistol ? '; barco también (' + boatPistol + ')' : '')
        : '\nFAIL — ' + (!errors.length
          ? (anySmg === 0 ? 'la SMG dejó de disparar (regresión): 0 disparos con slot 5'
            : !animLateral || !animBike ? 'faltan animaciones de drive-by (lateral ' + animLateral +
              ', moto ' + animBike + ')'
              : 'la pistola no se quedó en la mano al entrar (keep-weapon ' + r.keepWeapon + ')')
          : 'RuntimeError en la pestaña'));
    }
  }

  await browser.close().catch(() => {});
  process.exit(ok ? 0 : 1);
} catch (e) {
  console.log('FAIL: excepción del arnés: ' + String(e).slice(0, 400));
  if (browser) await browser.close().catch(() => {});
  process.exit(1);
}

function n(x) { return x === 1 ? '1 vez' : x + ' veces'; }
