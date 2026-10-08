// Arnés de pruebas de reVC (Vice Extended) — librería compartida.
//
// POR QUÉ EXISTE
// Los nueve *smoke tests* (`*-smoke-test.mjs`) repetían, cada uno, ~150 líneas de
// lo mismo: lanzar Chrome, sembrar la partida, esperar el menú, pulsar el menú,
// esperar la partida y leer la traza. Eso tenía tres precios:
//   1. TIEMPO: cada fichero es una sesión nueva (arranque del motor ~2 min) y usa
//      SU PROPIO perfil de Chrome, así que la caché on-demand no se comparte.
//   2. FLAQUEZA: la sincronía era a base de `sleep(3000)` a ojo, y cada test
//      definía sus propias funciones de traza (con sus variantes y sus fallos).
//   3. VERIFICABILIDAD: el log de cada test no decía sobre qué BINARIO se había
//      medido; ya nos mordió (una sonda midió un wasm viejo en la 13ª partida).
//
// LO QUE HACE ESTA LIBRERÍA
//   - Una SESIÓN reutilizable: un Chrome, un motor arrancado, N escenarios dentro.
//   - Sincronía por TRAZA (`traza.esperar(/regex/)`) en vez de esperas a ojo.
//   - Entradas de juego (`mantener`/`suelta`/`gira`) y capturas solo si fallan.
//   - Comprobaciones con evidencia (`check`) y un informe PASS/FALLO.
//   - Comprobación del BINARIO servido al abrir la sesión (versión + marcas).
//
// USO
//   import { abrirSesion, informe } from './harness/vc-harness.mjs';
//   const s = await abrirSesion({ nombre: 'mi-suite' });
//   const inf = informe('agachado', s);
//   inf.check('la cámara sigue al ped', ...);
//   await s.cerrar();
//
// Variables de entorno: las mismas que usaban los smoke tests (VC_URL, VC_SAVE,
// VC_ODTRACE, CHROME, PUPPETEER_DIR, VC_PERFIL, VC_ARTEFACTOS) más
// VC_MOTOR_TIMEOUT_S / VC_PARTIDA_TIMEOUT_S para los tiempos del arranque.

import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

export const HARNESS = path.dirname(fileURLToPath(import.meta.url));  // tools/harness
export const TOOLS = path.resolve(HARNESS, '..');                     // tools
export const VC = path.resolve(TOOLS, '..');                          // gta_vc_browser
export const WEB = path.join(VC, 'web');

export const URL_BASE = process.env.VC_URL || 'http://localhost:2077';
export const ODTRACE = process.env.VC_ODTRACE || path.join(WEB, 'odtrace.log');
export const SAVE = process.env.VC_SAVE || path.join(TOOLS, 'testdata', 'GTAVCsf1.b');
// Un ÚNICO perfil para todo el arnés: la caché on-demand (IndexedDB) y el
// /userfiles (IDBFS) se reaprovechan entre ejecuciones y entre suites.
export const PERFIL = process.env.VC_PERFIL || path.join(os.tmpdir(), 'vc-harness-profile');
export const CHROME = process.env.CHROME || 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
const PUPPETEER_DIR = process.env.PUPPETEER_DIR ||
  'C:/Users/s0rno/AppData/Local/Temp/opencode/vc-e2e/node_modules';

export const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

// Toda llamada al navegador va con tope de tiempo. Sin esto, una pestaña
// atascada (dos partidas a la vez comiendo la CPU, un cuelgue del motor) deja la
// prueba esperando minutos en lugar de fallar con un motivo.
export function conTiempo(p, ms, etiqueta = 'llamada al navegador') {
  return Promise.race([
    p,
    new Promise((_, rej) => setTimeout(() => rej(new Error('tiempo agotado: ' + etiqueta
      + ' (' + ms + ' ms)')), ms)),
  ]);
}

// ¿Hay otra sesión de juego escribiendo trazas ahora mismo? (el jugador probando
// a la vez, u otra sonda). No impide probar, pero avisa: la CPU va compartida y
// los arranques se alargan.
export function otraSesionActiva(ruta = ODTRACE, segundos = 25) {
  try {
    const st = fs.statSync(ruta);
    return (Date.now() - st.mtimeMs) / 1000 < segundos;
  } catch (e) { return false; }
}

// Filas del menú CARGAR PARTIDA como fracción del canvas. La primera fila es el
// AUTOGUARDADO (si existe) y la partida sembrada es la SEGUNDA (y≈228/800): con
// la fila y≈180 el clic cae en el hueco y la sonda se queda en el menú.
export const FRAC = {
  start: [0.5, 325 / 800],   // INICIAR PARTIDA
  load: [0.5, 345 / 800],    // CARGAR JUEGO
  slot: [0.3125, Number(process.env.VC_SLOT_Y || 228) / 800],
  yes: [0.5, 418 / 800],     // SÍ (¿continuar jugando?)
};

export async function cargarPuppeteer() {
  try { return (await import('puppeteer-core')).default; } catch (e) { /* fuera del arnés */ }
  const { createRequire } = await import('node:module');
  return createRequire(path.join(PUPPETEER_DIR, 'anchor.js'))('puppeteer-core');
}

// --- traza ------------------------------------------------------------------
// El motor escribe en `web/odtrace.log` (una línea por cosa medida). La traza es
// la fuente de verdad de las pruebas: las aserciones se hacen sobre ella, no
// sobre píxeles, y la sincronía también (`esperar`).

export class Traza {
  constructor(ruta = ODTRACE) { this.ruta = ruta; }

  tam() {
    try { return fs.statSync(this.ruta).size; } catch (e) { return 0; }
  }

  // Marca de posición: se pasa a `leer`/`lineas` para quedarse con un tramo.
  marca() { return this.tam(); }

  leer(desde = 0) {
    try {
      const t = fs.readFileSync(this.ruta, 'utf8');
      return t.length > desde ? t.slice(desde) : '';
    } catch (e) { return ''; }
  }

  // Tramo cerrado [desde, hasta): permite delimitar la traza de «andando» y la
  // de «quieto» sin depender de cuándo se lee.
  tramo(desde = 0, hasta = 0) {
    const t = this.leer(desde);
    const n = hasta > desde ? hasta - desde : 0;
    return n && t.length > n ? t.slice(0, n) : t;
  }

  // Líneas con la etiqueta pedida, sin el prefijo de hora.
  lineas(tag, desde = 0) {
    const re = new RegExp('(^|\\s)' + tag + '\\s');
    return this.leer(desde).split('\n').map((l) => l.trim())
      .filter((l) => re.test(l))
      .map((l) => l.replace(/^\S+Z\s+/, ''));
  }

  // Espera a que salga una línea que encaje (sincronía por evento, sin sleeps a
  // ojo). Devuelve {ok, lineas, ms, desde}.
  async esperar(re, { desde = 0, timeoutMs = 20000, intervaloMs = 400 } = {}) {
    const t0 = Date.now();
    // Si el patrón es un texto y ya está en el tramo, no se espera nada.
    for (;;) {
      const txt = this.leer(desde);
      const hay = typeof re === 'string'
        ? (txt.indexOf(re) >= 0 ? [re] : [])
        : txt.split('\n').filter((l) => re.test(l));
      if (hay.length) return { ok: true, lineas: hay, ms: Date.now() - t0, desde };
      if (Date.now() - t0 >= timeoutMs) return { ok: false, lineas: [], ms: Date.now() - t0, desde };
      await sleep(intervaloMs);
    }
  }

  // Última línea de una etiqueta (o null).
  ultima(tag, desde = 0) {
    const ls = this.lineas(tag, desde);
    return ls.length ? ls[ls.length - 1] : null;
  }
}

// Campo `clave=valor` anclado a principio de campo (sin esto, `dist` encajaba
// dentro de `camdist` y la sonda leía la distancia equivocada).
export function campo(linea, nombre) {
  const m = String(linea).match(new RegExp('(^|\\s)' + nombre + '=(-?[\\d.]+)'));
  return m ? Number(m[2]) : NaN;
}

// Campo `clave=valor` de TEXTO (nombres: `arma=beretta`, `txd=ak47`). Igual que
// `campo` pero sin exigir número; devuelve '' si no está (y así `!texto(...)`
// sirve como comprobación de presencia).
export function texto(linea, nombre) {
  const m = String(linea).match(new RegExp('(^|\\s)' + nombre + '=([^\\s]+)'));
  return m ? m[2] : '';
}

export function mediana(v) {
  const s = [...v].filter((x) => !Number.isNaN(x)).sort((a, b) => a - b);
  return s.length ? s[Math.floor(s.length / 2)] : NaN;
}

export function resumen(arr, campoONum) {
  const v = arr.map((l) => (typeof campoONum === 'function' ? campoONum(l) : campo(l, campoONum)))
    .filter((x) => !Number.isNaN(x));
  if (!v.length) return null;
  const s = [...v].sort((a, b) => a - b);
  return {
    n: v.length, min: s[0], max: s[s.length - 1], med: mediana(v),
    // `cuerpo` = muestras sin las dos primeras (arranque de la fase).
    cuerpo: s.slice(Math.min(2, Math.max(0, s.length - 1))),
  };
}

export const num = (v) => (v == null || Number.isNaN(v) ? '—' : v.toFixed(2));

// --- el binario que se está probando ---------------------------------------

// Versión + dataTag + marcas del motor servido. Una prueba sobre un binario que
// no es el que se cree es basura; esto lo comprueba al abrir la sesión.
export function motorServido() {
  let salida = '';
  try {
    salida = execFileSync('bash', [path.join(TOOLS, 'check-served-build.sh')],
      { cwd: VC, encoding: 'utf8', timeout: 120000 });
  } catch (e) {
    salida = String(e.stdout || '') + String(e.stderr || '');
  }
  const faltan = [...salida.matchAll(/FALTA (\S+)\s+(.*)/g)].map((m) => m[1]);
  const ver = (fs.readFileSync(path.join(WEB, 'lib', 'index.js'), 'utf8')
    .match(/VERSION = '([^']+)'/) || [])[1] || '?';
  const dt = (salida.match(/dataTag servido: (\S+)/) || [])[1] || '?';
  return { version: ver, dataTag: dt, faltan, ok: faltan.length === 0, salida };
}

// --- sesión -----------------------------------------------------------------

let contadorArtefactos = 0;

export async function abrirSesion(opts = {}) {
  const {
    nombre = 'suite',
    // Traza propia de la sesión: `/odtrace/<nombre>` -> `odtrace-<nombre>.log`.
    // Sin esto, la sonda y la partida del jugador (o dos sondas a la vez)
    // escriben en el mismo fichero y las medidas se mezclan.
    trazaNombre = (String(nombre).match(/^[\w-]+$/) ? String(nombre) : 'suite'),
    url = URL_BASE,
    save = SAVE,
    perfil = PERFIL,
    artefactos = process.env.VC_ARTEFACTOS,
    timeoutMotorS = Number(process.env.VC_MOTOR_TIMEOUT_S || 240),
    timeoutPartidaS = Number(process.env.VC_PARTIDA_TIMEOUT_S || 240),
    marcas = [],           // marcas que TIENE que llevar el motor servido
    silencioso = true,
  } = opts;

  if (!fs.existsSync(save)) throw new Error('no encuentro la partida a sembrar (' + save + ')');
  const art = artefactos ? path.resolve(artefactos)
    : path.join(os.tmpdir(), 'vc-tests',
      nombre + '-' + new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19));
  fs.mkdirSync(path.join(art, 'escenarios'), { recursive: true });

  const motor = motorServido();
  if (!motor.ok) {
    console.log('AVISO: al motor servido le faltan marcas: ' + motor.faltan.join(', '));
  }
  const faltan = marcas.filter((m) => motor.faltan.some((f) => f.indexOf(m) >= 0 && f !== ''));
  if (marcas.length && faltan.length) {
    throw new Error('el motor servido no lleva: ' + faltan.join(', ')
      + ' (recompila: bash build.sh, y comprueba con tools/check-served-build.sh)');
  }

  const puppeteer = await cargarPuppeteer();
  const errores = [];
  const consola = [];
  const opciones = {
    executablePath: CHROME,
    headless: 'new',
    protocolTimeout: 120000,
    args: ['--no-sandbox', '--disable-dev-shm-usage', '--use-angle=swiftshader',
      '--enable-unsafe-swiftshader', '--mute-audio', '--window-size=1280,800',
      '--user-data-dir=' + perfil],
  };
  // Un cierre a lo bruto (Ctrl+C, un `taskkill` para desatascar) deja el perfil
  // bloqueado y el siguiente arranque muere con "browser is already running".
  // Chrome deja el cerrojo en `Singleton*`: si el lanzamiento falla por eso, se
  // quitan y se reintenta UNA vez (nunca con el navegador del jugador: ese usa
  // otro perfil).
  let browser;
  try {
    browser = await puppeteer.launch(opciones);
  } catch (e) {
    if (!/already running|Singleton/i.test(String(e && e.message))) throw e;
    for (const f of ['SingletonLock', 'SingletonCookie', 'SingletonSocket']) {
      try { fs.rmSync(path.join(perfil, f), { force: true }); } catch (e2) {}
    }
    console.log('   (perfil bloqueado por un cierre anterior: cerrojo borrado, reintentando)');
    browser = await puppeteer.launch(opciones);
  }

  const page = await browser.newPage();
  await conTiempo(page.setViewport({ width: 1280, height: 800 }), 20000, 'setViewport').catch(() => {});
  page.on('console', (m) => {
    const t = m.text();
    consola.push(t.slice(0, 300));
    if (/memory access out of bounds|RuntimeError|Aborted|out of bounds/i.test(t))
      errores.push(t.slice(0, 300));
  });
  page.on('pageerror', (e) => errores.push('PAGEERROR ' + String(e).slice(0, 300)));

  // Cada sesión con su fichero de traza (y su rotación): las medidas de una
  // prueba no pueden depender de lo que haga otra pestaña a la vez.
  const traza = trazaNombre ? new Traza(path.join(WEB, 'odtrace-' + trazaNombre + '.log')) : new Traza();
  const t0 = Date.now();
  const tiempos = {};

  // Cache-buster por build (`b=` = mtime del `.wasm` servido). Sin esto, Chrome
  // puede servir el `reVC.wasm`/`reVC.js` de su caché y la sesión mide un binario
  // que NO es el que se acaba de enlazar: pasó el 23/09 (una corrida del escenario
  // de agachado salió "build sin R21" con el binario recién compilado en disco, y
  // el motor se quedó además atascado a 20 frames). El harness decide con ficheros
  // de disco, así que hay que forzar que la página pida los nuevos.
  const selloBuild = (() => {
    try {
      const st = fs.statSync(path.join(WEB, 'public', 'build', 'reVC.wasm'));
      return String(Math.floor(st.mtimeMs));
    } catch (e) { return '0'; }
  })();
  const urlSesion = url + '/?autostart=1&b=' + selloBuild + (trazaNombre ? '&trace=' + trazaNombre : '');
  if (otraSesionActiva()) {
    console.log('   AVISO: hay otra sesión de juego escribiendo trazas ahora mismo (jugador u otra sonda):'
      + ' la CPU va compartida y el arranque puede tardar más.');
  }
  await conTiempo(page.goto(urlSesion, { waitUntil: 'domcontentloaded', timeout: 60000 }), 90000, 'goto');

  // Sembrar la partida en /userfiles (IDBFS) en cuanto exista el FS. Se insiste
  // hasta que el FS tenga el fichero Y el montaje de IDBFS esté hecho (`saves=`
  // lista lo que hay dentro, que es lo que lee el menú de CARGAR).
  const b64 = fs.readFileSync(save).toString('base64');
  const sembrar = async () => conTiempo(page.evaluate(async (b) => {
    try {
      if (typeof FS === 'undefined') return 'sin FS';
      const bin = atob(b);
      const arr = new Uint8Array(bin.length);
      for (let i = 0; i < bin.length; i++) arr[i] = bin.charCodeAt(i);
      FS.writeFile('/userfiles/GTAVCsf1.b', arr);
      await new Promise((res) => FS.syncfs(false, res));
      const dentro = FS.readdir('/userfiles').filter((f) => /\.b$/i.test(f));
      return 'ok size=' + FS.stat('/userfiles/GTAVCsf1.b').size + ' saves=' + dentro.join(',');
    } catch (e) { return 'ERR ' + e; }
  }, b64), 20000, 'sembrar save').catch((e) => 'ERR ' + e);
  let sembrado = 'no';
  for (let i = 0; i < 120; i++) {
    sembrado = await sembrar();
    if (String(sembrado).startsWith('ok')) break;
    await sleep(1000);
  }

  // Leer el estado del motor de la página (con tope de tiempo: una pestaña
  // atascada no puede dejar la prueba colgada minutos).
  let sinRespuesta = 0;
  const estado = async () => {
    try {
      const log = await conTiempo(page.$eval('#gamelog', (e) => e.textContent), 6000, 'gamelog');
      sinRespuesta = 0;
      const st = (log.match(/state (\d+)/g) || []).map((s) => Number(s.split(' ')[1]));
      ultimoEstado = st.length ? st[st.length - 1] : ultimoEstado;
      return st;
    } catch (e) { sinRespuesta++; return []; }
  };
  let ultimoEstado = -1;

  // Menú (GS_FRONTEND = state 7 x3 en el heartbeat de la página).
  const tMenu = Date.now();
  let menu = false;
  while (Date.now() - tMenu < timeoutMotorS * 1000) {
    const st = await estado();
    if (st.slice(-3).length === 3 && st.slice(-3).every((s) => s === 7)) { menu = true; break; }
    await sleep(1500);
  }
  tiempos.motorMs = Date.now() - t0;
  if (!menu) throw new Error('el motor no llegó al menú en ' + timeoutMotorS + ' s (último estado '
    + ultimoEstado + ', lecturas sin respuesta ' + sinRespuesta + ')');
  await sleep(2500);
  const sembrado2 = await sembrar();
  if (String(sembrado2).indexOf('saves=') >= 0 && String(sembrado2).indexOf('saves=GTAVCsf1.b') < 0) {
    throw new Error('la partida sembrada no está en /userfiles del motor: ' + sembrado2);
  }

  const rect = await conTiempo(page.evaluate(() => {
    const c = document.querySelector('.vc-canvas');
    if (!c) return null;
    const r = c.getBoundingClientRect();
    return [r.x, r.y, r.width, r.height];
  }), 15000, 'medir canvas').catch(() => null);
  if (!rect) throw new Error('no encuentro el canvas del juego (.vc-canvas)');
  const at = ([fx, fy]) => [rect[0] + fx * rect[2], rect[1] + fy * rect[3]];
  const centro = [rect[0] + rect[2] / 2, rect[1] + rect[3] / 2];

  const clic = async ([x, y], esperaMs = 2500) => {
    await conTiempo(page.mouse.move(x, y, { steps: 3 }), 10000, 'mouse.move').catch(() => {});
    await sleep(600);
    await conTiempo(page.mouse.down(), 10000, 'mouse.down').catch(() => {});
    await sleep(320);
    await conTiempo(page.mouse.up(), 10000, 'mouse.up').catch(() => {});
    await sleep(esperaMs);
  };

  // Menú: INICIAR PARTIDA > CARGAR JUEGO > fila de la partida > SÍ. Se repite
  // entero (el menú tiene pantallas previas que se comen clics) pero se corta en
  // cuanto el motor pide la carga (`WR load-req`), que es la señal de verdad.
  // Todo el tramo va dentro de un TOPE: si el menú no cede, se falla con el
  // motivo (antes, con la CPU compartida, esto podía quedarse minutos sin decir
  // nada: le pasó a la primera corrida del arnés).
  const topeMenuMs = Number(process.env.VC_MENU_TIMEOUT_S || 150) * 1000;
  const tClics = Date.now();
  let pedido = false, intentos = 0;
  while (!pedido && Date.now() - tClics < topeMenuMs) {
    intentos++;
    await clic(at(FRAC.start));
    await clic(at(FRAC.load));
    await clic(at(FRAC.slot));
    await clic(at(FRAC.yes));
    for (let i = 0; i < 3 && !pedido; i++) {
      await sleep(2000);
      pedido = traza.leer(Math.max(0, traza.tam() - 300000)).indexOf('WR load-req') >= 0;
      const st = await estado();
      if (st.some((s) => s === 8 || s === 9)) pedido = true;
      if (sinRespuesta >= 6)
        throw new Error('el navegador no responde (' + sinRespuesta + ' lecturas seguidas sin contestar).'
          + ' ¿Hay otra partida pesada a la vez comiendo la CPU?');
    }
  }
  tiempos.menuMs = Date.now() - tClics;
  if (!pedido) {
    await conTiempo(page.screenshot({ path: path.join(art, 'boot-menu-fallo.png'), timeout: 20000 }), 25000, 'foto').catch(() => {});
    throw new Error('el menú no llegó a pedir la partida en ' + (topeMenuMs / 1000) + ' s (' + intentos
      + ' intento(s), último estado ' + ultimoEstado + '). Captura: ' + path.join(art, 'boot-menu-fallo.png'));
  }

  const tPartida = Date.now();
  let enPartida = false;
  while (Date.now() - tPartida < timeoutPartidaS * 1000) {
    const st = await estado();
    if (st.length && st[st.length - 1] === 9) { enPartida = true; break; }
    if (errores.length) break;
    await sleep(2000);
  }
  tiempos.partidaMs = Date.now() - tPartida;
  if (!enPartida) {
    await conTiempo(page.screenshot({ path: path.join(art, 'boot-partida-fallo.png'), timeout: 20000 }), 25000, 'foto').catch(() => {});
    throw new Error('no se llegó a jugar en ' + timeoutPartidaS + ' s (último estado ' + ultimoEstado
      + '). Captura: ' + path.join(art, 'boot-partida-fallo.png'));
  }

  // Foco al canvas (si no, el teclado no llega al motor) y asentar la partida
  // hasta que la traza diga que está estable.
  await conTiempo(page.mouse.move(centro[0], centro[1], { steps: 2 }), 10000, 'mouse.move').catch(() => {});
  await conTiempo(page.mouse.click(centro[0], centro[1]), 10000, 'mouse.click').catch(() => {});
  const tAsentar = Date.now();
  await traza.esperar(/FPHASE/, { timeoutMs: 30000 });
  while (Date.now() - tAsentar < 8000) await sleep(2000);
  tiempos.asentarMs = Date.now() - tAsentar;
  tiempos.totalMs = Date.now() - t0;

  // Versión de la PÁGINA, leída de la página (no de los ficheros): si no coincide
  // con `web/lib/index.js`, lo que se está midiendo no es el build que hay en
  // disco (caché del navegador) y toda la sesión es basura. Es un error, no un
  // aviso: se para aquí en vez de sacar 20 comprobaciones sin sentido.
  const versionPagina = await page.evaluate(() => window.__vcVersion || '').catch(() => '');
  if (versionPagina && versionPagina !== motor.version)
    throw new Error('la página corre ' + versionPagina + ' pero en disco hay ' + motor.version
      + ': el navegador está sirviendo un build viejo (Ctrl+Shift+R, o sube VERSION en web/lib/index.js)');

  const version = (traza.leer(Math.max(0, traza.tam() - 400000)).match(/JS build=(\S+)/) || [])[1] || motor.version;

  const sesion = {
    page, browser, errores, consola, traza, tiempos, motor, version,
    trazaFichero: traza.ruta, urlSesion,
    artefactos: art,
    rect, centro,
    // --- entradas de juego ---
    pulsa: async (key, ms = 140) => {
      await conTiempo(page.keyboard.down(key), 10000, 'keyboard.down').catch(() => {});
      await sleep(ms);
      await conTiempo(page.keyboard.up(key), 10000, 'keyboard.up').catch(() => {});
    },
    abajo: (key) => conTiempo(page.keyboard.down(key), 10000, 'keyboard.down').catch(() => {}),
    suelta: (key) => conTiempo(page.keyboard.up(key), 10000, 'keyboard.up').catch(() => {}),
    // Botón del ratón mantenido (el apuntado de GTA VC es `PED_LOCK_TARGET`, que
    // en el ratón es el botón derecho). El puntero tiene que estar sobre el
    // canvas: `gira()` ya lo deja en el centro, así que se llama antes.
    ratonAbajo: (btn = 'right') => conTiempo(page.mouse.down({ button: btn }), 10000, 'mouse.down').catch(() => {}),
    ratonSuelta: (btn = 'right') => conTiempo(page.mouse.up({ button: btn }), 10000, 'mouse.up').catch(() => {}),
    // Rueda del ratón: es el cambio de arma a pie en VC
    // (`SetMouseButtonAssociatedWithAction(PED_CYCLE_WEAPON_*, 4/5)`), y la única
    // forma de cambiar de arma que funciona en el navegador: el ciclo del teclado
    // (`rsPADDEL`/`rsPADENTER`, Dec y Enter del numérico) no llega al motor.
    ratonRueda: (dy) => conTiempo(page.mouse.wheel(0, dy), 10000, 'mouse.wheel').catch(() => {}),
    // En 3ª persona el ratón gira al ped (px relativos).
    gira: async (px) => {
      await conTiempo(page.mouse.move(centro[0], centro[1], { steps: 3 }), 10000, 'mouse.move').catch(() => {});
      await conTiempo(page.mouse.move(centro[0] + px, centro[1], { steps: 12 }), 10000, 'mouse.move').catch(() => {});
      await sleep(800);
    },
    // Teclado al estilo cheat (letra a letra): CPad lee eventos, no estado. Se
    // usa el NOMBRE de código de tecla (`KeyC`), no la letra: con la letra
    // suelta, puppeteer manda un evento sin `code` y el truco no entra (por eso
    // el escenario de armas salió sin una sola ficha `WINFO` la primera vez).
    // Se usa `press` (abajo+arriba de golpe) como la sonda de armas que ya
    // funcionaba: el motor lee el evento de TECLA, no el estado mantenido.
    cheat: async (txt) => {
      for (const ch of String(txt).toUpperCase()) {
        const tecla = ch === ' ' ? 'Space' : 'Key' + ch;
        await conTiempo(page.keyboard.press(tecla), 10000, 'cheat ' + tecla).catch(() => {});
        await sleep(90);
      }
    },
    foto: async (nombre) => {
      const ruta = path.join(art, 'escenarios', nombre + '.png');
      await conTiempo(page.screenshot({ path: ruta, timeout: 20000 }), 25000, 'screenshot').catch(() => {});
      return ruta;
    },
    // Estado del motor leído de la página (para diagnósticos).
    estado,
    espera: sleep,
    fs: async (expr) => page.evaluate(expr).catch((e) => 'ERR ' + e),
    cerrar: async () => {
      if (!silencioso) console.log('== cerrando sesión (' + nombre + ')');
      await browser.close().catch(() => {});
      // El navegador headless a veces tarda en soltar el perfil; se deja listo
      // para la siguiente corrida sin depender de un cierre limpio.
      await sleep(1500);
      for (const f of ['SingletonLock', 'SingletonCookie', 'SingletonSocket']) {
        try { fs.rmSync(path.join(perfil, f), { force: true }); } catch (e) {}
      }
    },
  };
  return sesion;
}

// --- comprobaciones e informe ----------------------------------------------

// Cada escenario devuelve su informe: las comprobaciones llevan la EVIDENCIA (la
// línea de traza o los números), para que el fallo se lea sin abrir el log.
export function informe(nombre, sesion) {
  const checks = [];
  const marcas = new Map();
  const art = sesion ? path.join(sesion.artefactos, 'escenarios') : null;
  const info = {
    nombre, checks,
    // Marca el comienzo: las trazas del escenario se guardan en disco al cerrar.
    abre(etiqueta) {
      marcas.set(etiqueta, sesion.traza.marca());
      return marcas.get(etiqueta);
    },
    // Traza del escenario (desde la marca) guardada como artefacto.
    cierra(etiqueta) {
      const txt = sesion.traza.leer(marcas.get(etiqueta) || 0);
      if (art) {
        try {
          fs.mkdirSync(art, { recursive: true });
          fs.writeFileSync(path.join(art, nombre + '-' + etiqueta + '.txt'), txt);
        } catch (e) { /* los artefactos no deben tumbar la prueba */ }
      }
      return txt;
    },
    check(nombre, bien, evidencia = '') {
      checks.push({ nombre, bien: !!bien, evidencia: String(evidencia) });
      console.log('   [' + (bien ? 'ok ' : 'MAL') + '] ' + nombre + (evidencia ? ' — ' + evidencia : ''));
      return !!bien;
    },
    // AVISO: lo que la sonda NO ha podido comprobar (no es un fallo del mod, es
    // un límite de la prueba). Sin esta categoría, o se suspende la suite por
    // algo que el arnés no sabe hacer, o se disimula un "ok" que no se ha medido.
    // Se imprimen siempre y el informe dice cuántos van: una prueba con avisos
    // está a medias y eso tiene que verse.
    aviso(nombre, evidencia = '') {
      checks.push({ nombre, bien: true, aviso: true, evidencia: String(evidencia) });
      console.log('   [AVISO] ' + nombre + (evidencia ? ' — ' + evidencia : ''));
      return true;
    },
    get avisos() { return checks.filter((c) => c.aviso).length; },
    noError(evidencia) {
      return info.check('sin errores de página', sesion.errores.length === 0, evidencia
        || (sesion.errores.length ? sesion.errores[0] : '0 errores'));
    },
    get ok() { return checks.length > 0 && checks.every((c) => c.bien); },
  };
  return info;
}

export function imprimirInforme(nombre, info) {
  const malos = info.checks.filter((c) => !c.bien);
  const avisos = info.checks.filter((c) => c.aviso);
  console.log('   ' + (info.ok ? 'OK  ' : 'FALLO') + ' ' + nombre + ' (' + (info.checks.length - malos.length)
    + '/' + info.checks.length + (avisos.length ? ', ' + avisos.length + ' aviso(s)' : '') + ')');
  for (const m of malos) console.log('        · ' + m.nombre + (m.evidencia ? ' — ' + m.evidencia : ''));
  for (const a of avisos) console.log('        ~ ' + a.nombre + ' (sin verificar en esta sonda)');
  return info.ok;
}

export const hayBash = (() => { try { execFileSync('bash', ['-c', 'true']); return true; } catch (e) { return false; } })();
