// Test de regresión: CARGAR LA PARTIDA DEL SLOT 0 y comprobar que el init
// termina (paso 8 "Load animations" -> CPed::Initialise -> bloques de anim)
// sin morir por "memory access out of bounds".
//
// Por qué existe: la build lag2 añadió un tope por TIEMPO dentro de
// LoadAllRequestedModels, que es un punto de sincronía; el corte dejó los
// bloques de animación a medio cargar y CPed::SetAnimOffsetForEnterOrExitVehicle
// leyó basura -> OOB. Este test recorre el flujo real del jugador
// (INICIAR PARTIDA > CARGAR JUEGO > slot 1 = archivo GTAVCsf1.b) y falla si el
// init no completa. Ver .agents/plans/diagnostico-crash-init.md.
//
// Uso:
//   node gta_vc_browser/tools/slot0-load-test.mjs
// Requisitos: servidor de desarrollo en marcha (cd gta_vc_browser/web && npm run dev)
// y Chrome instalado.
//
// Variables de entorno (todas opcionales):
//   VC_URL         default http://localhost:2077   (el server escucha en ::1)
//   VC_SAVE        ruta del save a sembrar (default: busca GTAVCsf1.b en el repo)
//   VC_PROFILE     perfil de Chrome del test (persistente; el save queda sembrado)
//   VC_ODTRACE     ruta de odtrace.log (default gta_vc_browser/web/odtrace.log)
//   PUPPETEER_DIR  node_modules con puppeteer-core (default: arnés de pruebas local)
//   CHROME         ruta de chrome.exe
//
// Salida: PASS/FAIL + extracto del log del motor. Exit code 0 = PASS, 1 = FAIL.

import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));            // gta_vc_browser/tools
const WEB = path.join(HERE, '..', 'web');                             // gta_vc_browser/web
const REPO = path.join(HERE, '..', '..');                             // raíz del repo

const URL_BASE = process.env.VC_URL || 'http://localhost:2077';
const ODTRACE = process.env.VC_ODTRACE || path.join(WEB, 'odtrace.log');
const PROFILE = process.env.VC_PROFILE || path.join(os.tmpdir(), 'vc-slot0-test-profile');
const CHROME = process.env.CHROME || 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';

// Coordenadas del frontend, como FRACCIÓN del canvas (el menú lo dibuja el
// motor, no hay DOM que consultar). Calibradas a 1280x800, que es el viewport
// de este test y, con la librería, el tamaño del canvas (ocupa el viewport).
// Ratón lento a propósito: el motor lee el ratón una vez por frame.
const FRAC = {
  start: [0.5, 325 / 800],      // INICIAR PARTIDA (menú principal)
  load: [0.5, 345 / 800],       // CARGAR PARTIDA (2ª opción del submenú)
  slot1: [0.3125, 180 / 800],   // primera fila del listado (GTAVCsf1.b = slot 0)
  yes: [0.5, 418 / 800],        // SÍ en "¿Cargar la partida y continuar jugando?"
};

const BOOT_TIMEOUT_MS = 150000;   // arranque del wasm + carga del mundo
const LOAD_TIMEOUT_MS = 180000;   // desde el clic en el slot hasta ver FPHASE

function findSave() {
  if (process.env.VC_SAVE) return process.env.VC_SAVE;
  // El save lo saca del navegador tools/extract-save-from-profile.mjs (los
  // guardados viven en IndexedDB, no en disco).
  const testdata = path.join(HERE, 'testdata', 'GTAVCsf1.b');
  if (fs.existsSync(testdata)) return testdata;
  const stack = [REPO];
  while (stack.length) {
    const dir = stack.pop();
    let ents = [];
    try { ents = fs.readdirSync(dir, { withFileTypes: true }); } catch (e) { continue; }
    for (const e of ents) {
      if (e.name === 'node_modules' || e.name === '.git') continue;
      const p = path.join(dir, e.name);
      if (e.isDirectory()) stack.push(p);
      else if (/^GTAVCsf\d\.b$/i.test(e.name)) return p;
    }
  }
  return null;
}

async function loadPuppeteer() {
  try { return (await import('puppeteer-core')).default; } catch (e) { /* fuera del arnés */ }
  const dir = process.env.PUPPETEER_DIR ||
    'C:/Users/s0rno/AppData/Local/Temp/opencode/vc-e2e/node_modules';
  const { createRequire } = await import('node:module');
  const req = createRequire(path.join(dir, 'anchor.js'));
  return req('puppeteer-core');
}

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

function traceTail(fromIso, limit = 40) {
  let txt = '';
  try { txt = fs.readFileSync(ODTRACE, 'utf8'); } catch (e) { return '(sin odtrace)'; }
  if (fromIso) {
    const idx = txt.lastIndexOf(fromIso);
    if (idx > 0) txt = txt.slice(idx);
  }
  return txt.trim().split('\n').filter((l) => !/^[^ ]+ PERF /.test(l)).slice(-limit).join('\n');
}

function traceSince(marker, pattern) {
  let txt = '';
  try { txt = fs.readFileSync(ODTRACE, 'utf8'); } catch (e) { return []; }
  const idx = txt.lastIndexOf(marker);
  const part = idx >= 0 ? txt.slice(idx) : txt;
  return part.split('\n').filter((l) => pattern.test(l));
}

const save = findSave();
if (!save) {
  console.log('FAIL: no encuentro ninguna partida guardada (GTAVCsf?.b) para sembrar.');
  console.log('Pasa la ruta con VC_SAVE=<ruta>GTAVCsf1.b');
  process.exit(1);
}

console.log('== save a cargar: ' + save);
const puppeteer = await loadPuppeteer();
const errors = [];       // errores de página/consola
const console_ = [];     // últimas líneas de consola del juego
let browser;

try {
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

  // Barra de progreso de la librería (la fina rosa del top): SOLO puede estar
  // encendida antes de que el motor arranque (comprobar ficheros + descargar
  // el build). Con el menú en pantalla ya no debe volver a encenderse: la
  // pantalla de carga la pinta el motor (arranque y carga de partida).
  await page.evaluateOnNewDocument(() => {
    window.__barLog = [];
    setInterval(() => {
      const bar = document.querySelector('.vc-bar');
      if (!bar) return;
      window.__barLog.push({ t: Math.round(performance.now()), on: bar.hasAttribute('data-on') });
    }, 100);
  });
  page.on('console', (m) => {
    const t = m.text();
    console_.push(t.slice(0, 300));
    if (/memory access out of bounds|RuntimeError|Aborted|out of bounds/i.test(t)) errors.push(t.slice(0, 300));
  });
  page.on('pageerror', (e) => errors.push('PAGEERROR ' + String(e).slice(0, 300)));

  console.log('== arrancando ' + URL_BASE + ' (perfil ' + PROFILE + ')');
  await page.goto(URL_BASE + '/?autostart=1', { waitUntil: 'domcontentloaded', timeout: 60000 });

  // 0) sembrar la partida en /userfiles EN CUANTO exista el FS virtual: el
  //    frontend lee el listado al entrar en la pantalla de carga, así que
  //    sembrar antes evita depender de que el perfil ya traiga partidas.
  const bytes0 = fs.readFileSync(save);
  const b64seed = bytes0.toString('base64');
  const write = async () => page.evaluate(async (b64) => {
    try {
      if (typeof FS === 'undefined') return 'sin FS';
      const bin = atob(b64);
      const arr = new Uint8Array(bin.length);
      for (let i = 0; i < bin.length; i++) arr[i] = bin.charCodeAt(i);
      FS.writeFile('/userfiles/GTAVCsf1.b', arr);
      await new Promise((res) => FS.syncfs(false, res));
      return 'ok size=' + FS.stat('/userfiles/GTAVCsf1.b').size;
    } catch (e) { return 'ERR ' + e; }
  }, b64seed).catch((e) => 'ERR ' + e);
  let seedEarly = 'no';
  for (let i = 0; i < 60; i++) {
    seedEarly = await write();
    if (String(seedEarly).startsWith('ok')) break;
    await sleep(1000);
  }
  console.log('== save sembrado antes del menú: ' + seedEarly);

  // 1) esperar al MENÚ (GS_FRONTEND = state 7 en el heartbeat de la página).
  //    Antes de eso el motor está en el logo/init del frontend y los clics se
  //    pierden (era el motivo del primer intento fallido de este test).
  const t0 = Date.now();
  let menuReady = false;
  while (Date.now() - t0 < BOOT_TIMEOUT_MS) {
    const log = await page.$eval('#gamelog', (e) => e.textContent).catch(() => '');
    const states = (log.match(/state \d+/g) || []).slice(-3);
    if (states.length === 3 && states.every((s) => s === 'state 7')) { menuReady = true; break; }
    await sleep(2000);
  }
  console.log('== menú listo: ' + (menuReady ? 'sí' : 'NO (sigo, pero los clics pueden perderse)'));
  await page.evaluate(() => { window.__menuAt = Math.round(performance.now()); });
  await sleep(4000);

  // 2) recordar el save (ya sembrado arriba) y el rectángulo del canvas.
  const bytes = fs.readFileSync(save);
  const seeded = await write();
  console.log('== save (' + Math.round(bytes.length / 1024) + ' KB) listo: ' + seeded);
  const rect = await page.evaluate(() => {
    const c = document.querySelector('.vc-canvas');
    if (!c) return null;
    const r = c.getBoundingClientRect();
    return [r.x, r.y, r.width, r.height];
  });
  if (!rect) { console.log('FAIL: no encuentro el canvas del juego (.vc-canvas)'); process.exit(1); }
  console.log('== canvas: ' + rect.join(',') + ' (los clics van por fracción)');
  const at = ([fx, fy]) => [rect[0] + fx * rect[2], rect[1] + fy * rect[3]];

  const slowClick = async (x, y) => {
    await page.mouse.move(x, y, { steps: 3 }).catch(() => {});
    await sleep(700);
    await page.mouse.down().catch(() => {});
    await sleep(350);
    await page.mouse.up().catch(() => {});
    await sleep(3000);
  };

  // 3) flujo del jugador: INICIAR PARTIDA > CARGAR JUEGO > slot 1.
  const stamp = new Date().toISOString().slice(0, 19); // referencia temporal para el log
  const mark = Date.now();
  await slowClick(...at(FRAC.start));
  await slowClick(...at(FRAC.load));
  await slowClick(...at(FRAC.slot1));   // abre el diálogo de confirmación
  await slowClick(...at(FRAC.yes));     // SÍ -> pide la carga

  // 4) confirmar que la carga se pidió de verdad (WR load-req es la marca que
  //    deja el propio motor al aceptar el diálogo).
  let requested = false;
  for (let i = 0; i < 12 && !requested; i++) {
    await sleep(2500);
    requested = traceSince('rotado: sesion nueva', /WR load-req/).length > 0;
    if (!requested && i === 3) await slowClick(...at(FRAC.yes)); // reintento del SÍ
  }

  // 5) esperar a la respuesta: FPHASE = el juego está en partida (los frames se
  //    miden por fases), ODANIMFAIL = los bloques de anim no llegaron.
  const tLoad = Date.now();
  let pass = false, animFail = false;
  while (Date.now() - tLoad < LOAD_TIMEOUT_MS) {
    if (traceSince('rotado: sesion nueva', /ODANIMFAIL/).length) { animFail = true; break; }
    const phases = traceSince('rotado: sesion nueva', /FPHASE/).length;
    if (phases >= 3) { pass = true; break; }
    if (errors.some((e) => /out of bounds|RuntimeError|Aborted/.test(e))) break;
    await sleep(3000);
  }

  // 6) veredicto.
  const crash = errors.filter((e) => /out of bounds|RuntimeError|Aborted/i.test(e));
  const bar = await page.evaluate(() => ({
    log: window.__barLog || [], menuAt: window.__menuAt || 0,
  })).catch(() => ({ log: [], menuAt: 0 }));
  const barEarly = bar.log.some((s) => s.on);
  const barLate = bar.log.filter((s) => s.on && s.t > (bar.menuAt || 0));
  const bart = traceSince('rotado: sesion nueva', /BART /);
  console.log('== barra del top: ' + (barEarly ? 'vista en la preparación previa (bien)' : 'no vista (revisar)') +
    '; encendida con el menú/partida en pantalla: ' + barLate.length + ' muestras' +
    (barLate.length ? ' (NO debería)' : ''));
  // Quién la apagó: 'motor-primer-frame' es la señal buena (el motor ya pinta).
  console.log('== por qué se apagó (BART): ' + (bart.length ? bart.map((l) => l.replace(/^.*?Z /, '')).join(' | ') : 'sin trazas'));
  console.log('\n== petición de carga vista: ' + (requested ? 'sí' : 'NO'));
  console.log('== líneas FPHASE (juego en marcha): ' + traceSince('rotado: sesion nueva', /FPHASE/).length);
  console.log('== errores de página: ' + (crash.length || 'ninguno'));
  console.log('== extracto del motor (desde ' + stamp + 'Z):');
  console.log(traceTail(stamp, 25));
  if (crash.length) console.log('\nERROR:\n' + crash.slice(0, 3).join('\n'));

  const ok = pass && requested && !animFail && !crash.length && !barLate.length;
  if (!ok) {
    const shot = path.join(os.tmpdir(), 'vc-slot0-test-fail.png');
    await page.screenshot({ path: shot, timeout: 20000 }).catch(() => {});
    console.log('captura del fallo: ' + shot);
  }
  console.log('\n' + (ok ? 'PASS' : 'FAIL') + ' — carga del slot 0 ' +
    (ok ? 'completada sin crash y sin barra fuera de sitio'
      : animFail ? 'con bloques de anim sin cargar (ODANIMFAIL)'
      : crash.length ? 'CRASH (memoria fuera de límites en el init)'
      : barLate.length ? 'con la barra del top encendida fuera de la preparación previa'
      : !requested ? 'sin que el menú pidiera la carga (¿clics perdidos?)'
      : 'sin llegar a partida'));
  await browser.close().catch(() => {});
  process.exit(ok ? 0 : 1);
} catch (e) {
  console.log('FAIL: excepción del arnés: ' + String(e).slice(0, 400));
  if (browser) await browser.close().catch(() => {});
  console.log(traceTail(new Date().toISOString().slice(0, 19), 15));
  process.exit(1);
}
