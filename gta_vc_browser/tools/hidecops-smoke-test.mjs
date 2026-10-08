// Sonda de la policía / esconderse (sección 2 del plan de mecánicas, bloque P1).
//
// Qué mide: con el cheat YOUWONTTAKEMEALIVE (nivel +2) en marcha, cuántos
// segundos tarda en bajar el nivel de búsqueda sin que los policías te vean, y
// qué hacen los CCopPed mientras tanto. Vanilla no lo publica en pantalla, así
// que el motor imprime una línea por segundo (ver `WantedTraceState` en
// src/core/Wanted.cpp) y esta sonda las recolecta de `window.__vcLog`, que es
// el mismo buffer al que va el stdout del motor (Module.print -> lib/index.js).
//
// Dos modos (VC_HIDECOPS_MODE):
//   baseline → la medición del plan ANTES de tocar código. PASS = la corrida
//              es limpia y hay traza; NO exige que el nivel baje (justamente lo
//              que se quiere averiguar es si baja o no).
//   feature  → verificación de VICEEXT_HIDE_COPS: PASS = el nivel baja tras
//              estar X s sin que te vean y no hay RuntimeError. Si la corrida no
//              consigue mantenerte escondido los 15 s del contrato, sale
//              INCONCLUSA (código 3) para repetirla, no FAIL: sin condición
//              creada no hay nada que juzgar.
//
// Uso:
//   node gta_vc_browser/tools/hidecops-smoke-test.mjs                 (baseline)
//   VC_HIDECOPS_MODE=feature node gta_vc_browser/tools/hidecops-smoke-test.mjs
//   VC_HIDECOPS_CAR=1 ...  (enciende el intento de subirse a un coche; por
//                           defecto se mide a pie, que es lo reproducible)
// Requisitos: servidor de desarrollo en marcha (cd gta_vc_browser/web && npm run dev),
// build con la instrumentación y Chrome instalado.
//
// Variables de entorno (mismas que vehicles-smoke-test.mjs, más las propias):
//   VC_URL, VC_SAVE, VC_SHOTS, VC_PROFILE, VC_ODTRACE, PUPPETEER_DIR, CHROME
//   VC_HIDECOPS_MODE=baseline|feature, VC_HIDECOPS_SECONDS (por defecto 120)

import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const WEB = path.join(HERE, '..', 'web');
const URL_BASE = process.env.VC_URL || 'http://localhost:2077';
const ODTRACE = process.env.VC_ODTRACE || path.join(WEB, 'odtrace.log');
const PROFILE = process.env.VC_PROFILE || path.join(os.tmpdir(), 'vc-hidecops-test-profile');
const CHROME = process.env.CHROME || 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
const SAVE = process.env.VC_SAVE || path.join(HERE, 'testdata', 'GTAVCsf1.b');
const BUILD = path.join(WEB, 'public', 'build');
const SHOTS = process.env.VC_SHOTS || path.join(os.tmpdir(), 'vc-hidecops');
const MODE = process.env.VC_HIDECOPS_MODE === 'feature' ? 'feature' : 'baseline';
const WATCH_S = Number(process.env.VC_HIDECOPS_SECONDS || 120);
// Etiqueta de sesión: el dev server comparte odtrace.log entre pestañas/agentes.
const TAG = Number(process.env.VC_HIDECOPS_TAG || (1000 + Math.floor(Math.random() * 900000)));

const FRAC = {
  start: [0.5, 325 / 800],
  load: [0.5, 345 / 800],
  slot1: [0.3125, 180 / 800],
  yes: [0.5, 418 / 800],
};

// El cheat se escribe en claro: el motor guarda las teclas y las compara con el
// literal codificado (ver CPad::AddToPCCheatString).
const CHEAT_WANTED = 'YOUWONTTAKEMEALIVE';   // nivel de búsqueda +2 (hasta 6)
const CHEAT_HEALTH = 'ASPIRINE';             // sobrevivir al tiroteo y poder medir
const CHEAT_ARMOUR = 'PRECIOUSPROTECTION';
const CHEAT_RIDES = 'CRAZYRIDES';            // coche a mano: a pie te detienen en 10 s

// Umbral del contrato: una estrella cada 15 s sin que nadie te vea (ver el plan
// de mecánicas, sección 2). Si la corrida no llega a esconderte tanto tiempo, no
// se puede juzgar la mecánica: es INCONCLUSA, no FAIL.
const HIDE_STAR_S = Number(process.env.VC_HIDECOPS_STAR_S || 15);
// El modo coche (teclear Enter hasta subirse) hace que la sonda conduzca hacia
// los policías y es poco reproducible: apagado por defecto (la medida buena del
// plan es a pie, escondiéndose detrás de un edificio). VC_HIDECOPS_CAR=1 lo enciende.
const TRY_CAR = process.env.VC_HIDECOPS_CAR === '1';
const BOOT_TIMEOUT_MS = 150000;
const LOAD_TIMEOUT_MS = 180000;
const SAMPLE_MS = 750;

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

// El directorio de build es COMPARTIDO con los otros dos agentes: si alguien
// recompila a mitad de la corrida, el wasm que se descarga queda a medias
// ("expected 4 bytes, fell off end") y la medición no vale. Se guarda la huella
// del build al empezar y se comprueba al terminar.
function buildFingerprint() {
  const out = {};
  for (const f of ['reVC.js', 'reVC.wasm', 'reVC.data']) {
    try { const st = fs.statSync(path.join(BUILD, f)); out[f] = st.size + '@' + st.mtime.toISOString(); }
    catch (e) { out[f] = 'FALTA'; }
  }
  return out;
}
const BUILD0 = buildFingerprint();
console.log('== build medido: ' + BUILD0['reVC.wasm']);
const wasmHasTrace = (() => {
  try { return fs.readFileSync(path.join(BUILD, 'reVC.wasm')).includes(Buffer.from('WANTED tag=')); }
  catch (e) { return false; }
})();
if (!wasmHasTrace) {
  console.log('FAIL: el build actual NO trae la instrumentación de Wanted.cpp (recompila).');
  process.exit(1);
}
console.log('== modo: ' + MODE + ' (observando ' + WATCH_S + ' s)');
console.log('== save a cargar: ' + SAVE);

const puppeteer = await loadPuppeteer();
const errors = [];
const consola = [];
let browser;
let ok = false;
let foreign = 0;   // líneas de traza de otra pestaña/agente (comparten odtrace.log)

// ---- análisis de la traza ---------------------------------------------------
// Líneas del motor:  WANTED t=.. lvl=.. chaos=.. minlvl=.. cops=../.. listed=..
//                    presence18=.. seeing=.. near=.. objs=..
const RE_WANTED = /^WANTED tag=(\d+) t=(\d+) lvl=(-?\d+) chaos=(-?\d+) minlvl=(-?\d+) cops=(\d+)\/(\d+) listed=(\d+) presence18=(\d+) seeing=(\d+) near=(-?[\d.]+) unseen=(\d+) hiding=(\d+) veh=(\d+) spd=(-?[\d.]+) objs=(.*)$/;
// El ` tag=N` de las demás líneas es OPCIONAL a propósito: el build puede ser
// anterior a la etiqueta (la traza empezó sin ella), y la sonda tiene que poder
// medir las dos formas. Cuando la línea trae `t=` (reloj del motor) se usa para
// medir latencias exactas en vez de contar muestras.
const RE_CHANGE = /^WANTEDCHANGE(?: tag=(\d+))? (-?\d+)->(-?\d+) chaos=(-?\d+) t=(\d+)/;
const RE_PURGE = /^WANTEDPURGE(?: tag=(\d+))?/;
const RE_SUSPEND = /^WANTEDSUSPEND(?: tag=(\d+))?/;
const RE_COPJOIN = /^WANTEDCOP join(?: tag=(\d+))?/;
const RE_ARREST = /^WANTEDCOP arrest-begin(?: tag=(\d+))?/;
const RE_HIDESTART = /^WANTEDHIDE start(?: tag=(\d+))? lvl=(-?\d+) chaos=(-?\d+)(?: t=(\d+))?/;
const RE_HIDEDROP = /^WANTEDHIDE drop(?: tag=(\d+))? lvl=(-?\d+)->(-?\d+) chaos=(-?\d+)(?: t=(\d+))?/;
const RE_HIDESEEN = /^WANTEDHIDE seen(?: tag=(\d+))?/;
const RE_HIDEEND = /^WANTEDHIDE end(?: tag=(\d+))?/;
const RE_COPSEARCH = /^WANTEDCOP search-last-known(?: tag=(\d+))?/;

function parseLine(line) {
  const w = RE_WANTED.exec(line);
  if (w) {
    return {
      kind: 'wanted', tag: +w[1], t: +w[2], lvl: +w[3], chaos: +w[4], minlvl: +w[5],
      cops: +w[6], maxcops: +w[7], listed: +w[8], presence: +w[9],
      seeing: +w[10], near: +w[11], unseen: +w[12], hiding: +w[13], veh: +w[14], spd: +w[15], objs: w[16],
    };
  }
  const c = RE_CHANGE.exec(line);
  if (c) return { kind: 'change', tag: c[1] !== undefined ? +c[1] : undefined, from: +c[2], to: +c[3], chaos: +c[4], t: +c[5] };
  const tagOf = (m) => (m && m[1] !== undefined ? +m[1] : undefined);
  const p = RE_PURGE.exec(line);
  if (p) return { kind: 'purge', tag: tagOf(p) };
  const su = RE_SUSPEND.exec(line);
  if (su) return { kind: 'suspend', tag: tagOf(su) };
  const cj = RE_COPJOIN.exec(line);
  if (cj) return { kind: 'copjoin', tag: tagOf(cj) };
  const ar = RE_ARREST.exec(line);
  if (ar) return { kind: 'arrest', tag: tagOf(ar) };
  const hs = RE_HIDESTART.exec(line);
  if (hs) return { kind: 'hidestart', tag: tagOf(hs), lvl: +hs[2], chaos: +hs[3], t: hs[4] !== undefined ? +hs[4] : undefined };
  const hd = RE_HIDEDROP.exec(line);
  if (hd) return { kind: 'hidedrop', tag: tagOf(hd), from: +hd[2], to: +hd[3], t: hd[5] !== undefined ? +hd[5] : undefined };
  const sn = RE_HIDESEEN.exec(line);
  if (sn) return { kind: 'hideseen', tag: tagOf(sn) };
  const he = RE_HIDEEND.exec(line);
  if (he) return { kind: 'hideend', tag: tagOf(he) };
  const cs = RE_COPSEARCH.exec(line);
  if (cs) return { kind: 'copsearch', tag: tagOf(cs) };
  return null;
}

// Pega las muestras de traza en una serie temporal y resume la corrida.
function summarise(samples) {
  if (!samples.length) return null;
  const wanted = samples.filter((s) => s.kind === 'wanted');
  if (!wanted.length) return null;
  const first = wanted[0];
  const last = wanted[wanted.length - 1];
  // El nivel que interesa es el del cheat; lo de antes (nivel 0 saliendo de la
  // carga) no cuenta para nada. Muestras "post": desde la primera vez que se
  // alcanza el nivel máximo observado.
  const peak = Math.max(...wanted.map((s) => s.lvl));
  const firstPeak = wanted.findIndex((s) => s.lvl === peak);
  const post = wanted.slice(firstPeak);

  // Segundos con presencia 0 / con nadie viéndote, y la racha más larga sin ver
  // (solo con el nivel del cheat puesto).
  let unseenRun = 0, unseenBest = 0, presence0 = 0, seeing0 = 0;
  let bestUnseen = null, runStart = null;
  for (const s of post) {
    if (s.seeing === 0) {
      unseenRun++;
      if (runStart === null) runStart = s;
      if (unseenRun > unseenBest) {
        bestUnseen = { from: runStart, to: s, s };
        unseenBest = unseenRun;
      }
    } else { unseenRun = 0; runStart = null; }
    if (s.presence === 0) presence0++;
    if (s.seeing === 0) seeing0++;
  }
  if (bestUnseen) {
    bestUnseen.chaosDelta = bestUnseen.to.chaos - bestUnseen.from.chaos;
    bestUnseen.lvlFrom = bestUnseen.from.lvl;
    bestUnseen.lvlTo = bestUnseen.to.lvl;
  }
  // Un descenso de nivel puede ser legítimo sin ser mérito de la mecánica:
  // detención ("busted") o respawn de taller resetean el nivel. Aquí se marca
  // cada bajada como "reseteada" si en las 3 entradas anteriores hubo
  // arrest/purge/suspend, y solo cuentan las que NO lo son.
  const realDrops = [];
  const resetDrops = [];
  let lastWanted = null, lastResetAt = -99;
  post.forEach((s, i) => {
    if (s.kind === 'arrest' || s.kind === 'purge' || s.kind === 'suspend') lastResetAt = i;
    if (s.kind !== 'wanted') return;
    if (lastWanted && s.lvl < lastWanted.lvl) {
      const drop = { from: lastWanted.lvl, to: s.lvl, i, prev: lastWanted };
      (i - lastResetAt <= 3 ? resetDrops : realDrops).push(drop);
    }
    lastWanted = s;
  });
  const unseenToDrop = (() => {
    // ¿Bajó el nivel DESPUÉS del cheat sin que nadie te viera y sin reset?
    const d = realDrops.find((x) => x.prev.seeing === 0);
    if (!d) return null;
    let j = d.i - 1;
    for (let k = d.i - 1; k >= 0; k--) if (post[k].seeing > 0) { j = k; break; }
    return d.i - j;
  })();
  return {
    samples: wanted.length, first, last, peakSample: wanted[firstPeak],
    lvlMin: Math.min(...wanted.map((s) => s.lvl)),
    lvlMax: Math.max(...wanted.map((s) => s.lvl)),
    chaosFirst: first.chaos, chaosLast: last.chaos,
    chaosMin: Math.min(...wanted.map((s) => s.chaos)),
    copsMax: Math.max(...wanted.map((s) => s.cops)),
    nearMin: Math.min(...wanted.map((s) => s.near).filter((n) => n >= 0)),
    presence0, seeing0, unseenBestSamples: unseenBest, bestUnseen,
    // Segundos de la racha más larga escondido, medidos con el reloj del MOTOR:
    // el campo `unseen` de la traza es justo ese contador (una racha que llegue a
    // 15 s ya puede disparar la bajada; menos, no juzga nada).
    maxUnseenS: Math.max(0, ...wanted.map((s) => s.unseen || 0)),
    // Latencia real de la mecánica: /cuánto tarda la primera bajada de nivel
    // desde que empieza la búsqueda? Se mide con el reloj DEL MOTOR (`t=` de
    // WANTEDHIDE start/drop) y no contando muestras: la traza puede venir en
    // ráfagas y el reloj del muro no es el del juego.
    hideLatencyS: (() => {
      const drop = samples.find((s) => s.kind === 'hidedrop');
      if (!drop || drop.t === undefined) return null;
      let start = null;
      for (const s of samples) { if (s === drop) break; if (s.kind === 'hidestart') start = s; }
      return start && start.t !== undefined ? +((drop.t - start.t) / 1000).toFixed(1) : null;
    })(),
    changes: samples.filter((s) => s.kind === 'change'),
    copjoins: samples.filter((s) => s.kind === 'copjoin').length,
    arrests: samples.filter((s) => s.kind === 'arrest').length,
    purges: samples.filter((s) => s.kind === 'purge').length,
    suspends: samples.filter((s) => s.kind === 'suspend').length,
    unseenToDrop,
    hideStarts: samples.filter((s) => s.kind === 'hidestart').length,
    hideDrops: samples.filter((s) => s.kind === 'hidedrop'),
    hideSeen: samples.filter((s) => s.kind === 'hideseen').length,
    hideEnds: samples.filter((s) => s.kind === 'hideend').length,
    copSearches: samples.filter((s) => s.kind === 'copsearch').length,
    hidingSamples: post.filter((s) => s.hiding === 1).length,
    seenSamples: post.filter((s) => s.seeing > 0).length,
    realDrops, resetDrops,
    vehSamples: post.filter((s) => s.kind === 'wanted' && s.veh === 1).length,
    dropsWhileSeenAtHighLvl: realDrops.filter((d) => d.prev.lvl >= 2 && d.prev.seeing > 0).length,
  };
}

// Desde dónde leer odtrace.log: lo escrito antes de esta corrida no es nuestro.
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
    // Los clics del frontend se pierden de vez en cuando (swiftshader lento):
    // si a mitad del margen no hay petición, se repite el flujo entero.
    if (!requested && i === 7) {
      await page.screenshot({ path: path.join(SHOTS, 'nav-reintento.png'), timeout: 20000 }).catch(() => {});
      console.log('== sin petición de carga: se repite el flujo del menú');
      await navClick();
    }
  }

  // "En partida": el propio heartbeat de la página dice el estado del motor
  // (state 9 = juego corriendo). Es más fiable que contar FPHASE en odtrace
  // (ese fichero conserva trazas de corridas anteriores).
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

  // ---- traza del motor ----------------------------------------------------
  // Se lee odtrace.log (el motor la manda por ODTRACES -> window.__odq -> POST
  // /odtrace, que añade la marca de tiempo). Por stdout NO sirve: el printf de C
  // va a bloques y se pierde la mayor parte de las líneas.
  const samples = [];
  const readNew = () => {
    let txt = '';
    try { txt = fs.readFileSync(ODTRACE, 'utf8'); } catch (e) { return; }
    if (txt.length < odPos) odPos = 0;          // el log rota por sesión
    const part = txt.slice(odPos);
    odPos = txt.length;
    if (!part) return;
    for (let line of part.split('\n')) {
      if (!line) continue;
      const m = /^(\d{4}-\d{2}-\d{2}T[\d:.]+Z)\s+(.*)$/.exec(line);
      const stamp = m ? m[1] : null;
      const payload = m ? m[2] : line;
      if (payload.indexOf('WANTED') !== 0) continue;
      const s = parseLine(payload);
      if (!s) continue;
      if (s.tag !== undefined && s.tag !== TAG) { foreign++; continue; }   // otra pestaña/agente
      s.stamp = stamp;
      samples.push(s);
    }
  };
  const drain = async () => {
    const n = samples.length;
    readNew();
    return samples.length - n;
  };

  await page.mouse.move(rect[2] / 2, rect[3] / 2, { steps: 2 }).catch(() => {});
  await page.mouse.click(rect[2] / 2, rect[3] / 2).catch(() => {});
  await sleep(800);

  // Lo que de verdad invalida una corrida es que alguien recompile MIENTRAS se
  // carga el build (el wasm llega a medias). Después de estar en partida el
  // navegador ya no vuelve a pedirlo: si luego cambia, solo se avisa.
  const BUILD_IN = buildFingerprint();
  const buildCargado = JSON.stringify(BUILD0) === JSON.stringify(BUILD_IN);
  console.log('== build estable durante la carga: ' + (buildCargado ? 'sí' : 'NO (recompilaron cargando: repetir)'));
  if (!buildCargado) {
    await browser.close().catch(() => {});
    process.exit(1);
  }

  // Etiqueta de sesión para poder separar mi traza del resto del odtrace.log.
  await page.evaluate((tag) => { window.__vcWantedTag = tag; }, TAG).catch(() => {});
  console.log('== etiqueta de sesión: ' + TAG);

  const typeCheat = async (word) => {
    for (const ch of word) {
      await page.keyboard.press('Key' + ch).catch(() => {});
      await sleep(140);
    }
    console.log('== cheat tecleado: ' + word);
    await sleep(600);
  };

  await drain();
  console.log('== traza de arranque: ' + (samples.length ? samples.length + ' líneas' : 'ninguna'));
  await page.screenshot({ path: path.join(SHOTS, '00-antes-del-cheat.png'), timeout: 20000 }).catch(() => {});

  // Supervivencia: el tiroteo dura minutos y sin esto la corrida acaba en
  // "wasted" o "busted" (y el nivel de búsqueda se resetea solo) antes de poder
  // medir. El coche de CRAZYRIDES cae al lado del jugador: andar hacia delante
  // suele bastar para entrar en él y poder romper la persecución, que es el
  // caso "sin que te vean" que pide el plan.
  await typeCheat(CHEAT_ARMOUR);
  await typeCheat(CHEAT_HEALTH);
  await typeCheat(CHEAT_RIDES);
  await typeCheat(CHEAT_WANTED);

  // Antes de medir: ¿llega la traza? Si no, es un build sin instrumentación y
  // no tiene sentido gastar 2 minutos de swiftshader.
  for (let i = 0; i < 6 && !samples.length; i++) { await sleep(1000); await drain(); }
  if (!samples.length) {
    console.log('FAIL: no llega ninguna línea WANTED del motor (build sin instrumentación).');
    await browser.close().catch(() => {});
    process.exit(1);
  }

  const start = Date.now();
  let nextShot = 5, nextHeal = 20;
  let held = [];
  const setHeld = async (keys) => {
    for (const k of held) if (!keys.includes(k)) await page.keyboard.up(k).catch(() => {});
    for (const k of keys) if (!held.includes(k)) await page.keyboard.down(k).catch(() => {});
    held = keys;
  };
  const shots = new Set([5, 30, 65, 95, WATCH_S]);
  let phaseLogged = '', lastEnter = -99, enterTries = 0, reCheats = 0, lastReCheat = -99;

  while (Date.now() - start < WATCH_S * 1000) {
    await sleep(SAMPLE_MS);
    await drain();
    const elapsed = Math.round((Date.now() - start) / 1000);
    if (elapsed >= nextHeal) { nextHeal = elapsed + 20; await typeCheat(CHEAT_HEALTH); }
    if (shots.has(elapsed)) {
      await page.screenshot({ path: path.join(SHOTS, elapsed + 's.png'), timeout: 20000 }).catch(() => {});
    }

    // Estado real del jugador, leído de la propia traza (realimentación): si va
    // a pie lo detienen en segundos (0.8 m medidos), así que la maniobra es
    // INSISTIR en subirse a un coche y conducir para romper la persecución.
    const last = [...samples].reverse().find((s) => s.kind === 'wanted');
    const inVeh = !!(last && last.veh === 1);
    const lvl = last ? last.lvl : 0;

    // Si te han detenido/matado el nivel queda en 0 y la mecánica no puede
    // probarse: se vuelve a subir (dos veces como mucho).
    if (lvl === 0 && elapsed > 30 && reCheats < 2 && elapsed - lastReCheat > 35) {
      reCheats++; lastReCheat = elapsed;
      console.log('== t=' + elapsed + 's nivel 0 (detenido o respawn): se repite el cheat');
      await typeCheat(CHEAT_WANTED);
    }

    const phase = elapsed < 8 ? 'quieto-viendote' : elapsed < 100 ? 'huyendo' : 'quieto-final';
    if (phase !== phaseLogged) {
      phaseLogged = phase;
      console.log('== t=' + elapsed + 's fase: ' + phase);
    }

    if (phase === 'huyendo') {
      await setHeld(['KeyW']);
      // Enter = VEHICLE_ENTER_EXIT (ControllerConfig). Solo a pie: conduciendo
      // el mismo Enter te baja del coche. Opt-in: acaba conduciendo hacia los
      // policías y estropea la medida (ver VC_HIDECOPS_CAR).
      if (TRY_CAR && !inVeh && elapsed - lastEnter >= 4) {
        lastEnter = elapsed; enterTries++;
        await page.keyboard.press('Enter').catch(() => {});
      }
    } else {
      await setHeld([]);
    }

    if (elapsed % 20 === 0 && !samples.length) {
      console.log('== aviso: todavía sin traza WANTED (¿build instrumentado?)');
    }
  }
  await setHeld([]);
  console.log('== intentos de subir al coche (modo coche: ' + (TRY_CAR ? 'sí' : 'no') + '): ' + enterTries +
    '; cheats de nivel repetidos: ' + reCheats);
  await page.screenshot({ path: path.join(SHOTS, '99-final.png'), timeout: 20000 }).catch(() => {});
  const finalLog = await page.evaluate(() => (window.__vcLog || []).slice(-25)).catch(() => []);
  console.log('== consola del motor (últimas): ' + (finalLog.slice(-6).join(' | ') || '(vacía)'));

  // ---- informe ---------------------------------------------------------------
  const r = summarise(samples);
  console.log('== traza: ' + samples.length + ' eventos (' +
    samples.filter((s) => s.kind === 'wanted').length + ' muestras de 1 s)');
  if (!r) {
    console.log('FAIL: sin traza WANTED. ¿El build incluye la instrumentación de Wanted.cpp?');
    await browser.close().catch(() => {});
    process.exit(1);
  }
  console.log('== nivel de búsqueda: ' + r.first.lvl + ' → ' + r.last.lvl +
    ' (mín ' + r.lvlMin + ', máx ' + r.lvlMax + ')');
  console.log('== chaos: ' + r.chaosFirst + ' → ' + r.chaosLast + ' (mín ' + r.chaosMin + ')');
  console.log('== policías persiguiendo (máx): ' + r.copsMax + '; altas de persecución: ' + r.copjoins);
  console.log('== policía más cercano (mín): ' + (isFinite(r.nearMin) ? r.nearMin.toFixed(1) + ' m' : 'n/a'));
  console.log('== segundos sin nadie en 18 m: ' + r.presence0 + '; sin línea de visión: ' + r.seeing0 +
    ' (racha máx ' + r.unseenBestSamples + ' muestras)');
  console.log('== cambios de nivel: ' + (r.changes.length
    ? r.changes.map((c) => c.from + '->' + c.to + (c.chaos >= 0 ? '@chaos' + c.chaos : '')).join(', ')
    : 'ninguno'));
  console.log('== \"busted\": ' + r.arrests + '; purgas de persecución: ' + r.purges +
    '; suspensions (garaje): ' + r.suspends);
  console.log('== racha más larga sin ver (lvl ' + (r.bestUnseen ? r.bestUnseen.lvlFrom + '→' + r.bestUnseen.lvlTo : '-') +
    ', first ' + (r.bestUnseen ? r.bestUnseen.s.chaos : '-') + ')' +
    ': ' + r.unseenBestSamples + ' muestras, chaos ' + (r.bestUnseen ? r.bestUnseen.chaosDelta : '-'));
  console.log('== muestras a solas antes de que cayera el nivel: ' +
    (r.unseenToDrop === null ? 'nunca cayó' : r.unseenToDrop));
  console.log('== racha más larga escondido (reloj del motor, campo unseen): ' + r.maxUnseenS + ' s');
  console.log('== latencia de la 1ª bajada (reloj del motor, desde el inicio de la búsqueda): ' +
    (r.hideLatencyS === null ? 'n/a' : r.hideLatencyS + ' s'));
  console.log('== mecánica de esconderse: inicios ' + r.hideStarts + '; descensos ' +
    (r.hideDrops.length ? r.hideDrops.map((d) => d.from + '->' + d.to).join(', ') : 'ninguno') +
    '; "te vuelvo a ver" ' + r.hideSeen + '; fin ' + r.hideEnds +
    '; policías yendo a la última posición conocida: ' + r.copSearches);
  console.log('== muestras: viéndote ' + r.seenSamples + ', escondido (hiding=1) ' + r.hidingSamples +
    ', conduciendo ' + r.vehSamples +
    ', descensos reales ' + r.realDrops.length + ' y por detención/taller ' + r.resetDrops.length +
    ', descensos estando visto con nivel >= 2: ' + r.dropsWhileSeenAtHighLvl);
  console.log('== capturas en ' + SHOTS);
  console.log('== errores de página: ' + (errors.length ? errors.slice(0, 3).join(' | ') : 'ninguno'));
  console.log('== líneas de OTRAS sesiones ignoradas: ' + foreign);
  const BUILD1 = buildFingerprint();
  const buildEstable = JSON.stringify(BUILD0) === JSON.stringify(BUILD1);
  console.log('== build intacto al terminar: ' + (buildEstable ? 'sí' : 'no (alguien recompiló; la partida ya estaba cargada)'));

  const dropped = r.last.lvl < r.lvlMax;
  const droppedWhileUnseen = r.unseenToDrop !== null;
  console.log('\nRESUMEN ' + MODE + ': nivel máx observado ' + r.lvlMax + ' → final ' + r.last.lvl +
    (dropped ? (droppedWhileUnseen ? ' (bajó a solas)' : ' (bajó, pero no a solas)') : ' (NO bajó)') +
    '; chaos ' + r.chaosFirst + '→' + r.chaosLast + ' (mín ' + r.chaosMin + ')');

  const droppedReal = r.realDrops.length > 0;
  if (MODE === 'baseline') {
    ok = !errors.length;
    console.log(ok
      ? '\nPASS (medición) — línea base registrada: ' + (droppedReal ? 'el nivel bajó' : 'el nivel NO bajó') +
        ' en ' + WATCH_S + ' s con nivel máx ' + r.lvlMax + ' y ' + r.unseenBestSamples + ' muestras seguidas sin ser visto'
      : '\nFAIL — RuntimeError durante la medición');
  } else {
    // PASS = el mecanismo se dispara (baja una estrella estando sin que te vean)
    // y NUNCA baja estando visto con nivel >= 2. Si la corrida no llegó a darte
    // 15 s escondido, no hay nada que juzgar: INCONCLUSA (repetir), que no FAIL.
    const mecanismo = r.hideDrops.length > 0;
    const condicionCreada = r.maxUnseenS >= HIDE_STAR_S;
    if (r.hideDrops.length === 0 && !condicionCreada) {
      console.log('\nINCONCLUSA — la corrida no llegó a mantenerte sin que te vieran el tiempo del' +
        ' contrato (' + HIDE_STAR_S + ' s): racha máx ' + r.maxUnseenS + ' s, inicios de búsqueda ' +
        r.hideStarts + ', "te vuelvo a ver" ' + r.hideSeen + '. No juzga la mecánica: repetir' +
        (r.vehSamples > 0 ? ' (aquí el jugador acabó conduciendo hacia los policías)' : '') + '.');
      await browser.close().catch(() => {});
      process.exit(3);
    }
    ok = !errors.length && mecanismo && r.dropsWhileSeenAtHighLvl === 0 && r.hidingSamples > 0;
    console.log(ok
      ? '\nPASS — VICEEXT_HIDE_COPS: ' + r.hideDrops.length + ' descenso(s) al quedarte sin que te vean' +
        (r.hideLatencyS !== null ? ' (' + r.hideLatencyS + ' s desde que empezó la búsqueda)' : '') +
        ' y ninguno estando visto'
      : '\nFAIL — ' + (!errors.length
        ? (r.dropsWhileSeenAtHighLvl ? 'el nivel bajó ESTANDO VISTO (' + r.dropsWhileSeenAtHighLvl + ' veces)'
          : !mecanismo ? 'hubo ' + r.maxUnseenS + ' s seguidos sin que te vieran (>= ' + HIDE_STAR_S +
            ' s del contrato) y aun así el nivel no bajó'
            : 'sin descensos')
        : 'RuntimeError en la pestaña'));
  }

  await browser.close().catch(() => {});
  process.exit(ok ? 0 : 1);
} catch (e) {
  console.log('FAIL: excepción del arnés: ' + String(e).slice(0, 400));
  if (browser) await browser.close().catch(() => {});
  process.exit(1);
}
