// Sonda de 1ª persona (bloque C1 del plan de mecánicas, sección 3).
//
// Verifica el CONMUTADOR de vista en 1ª persona (`VICEEXT_FIRST_PERSON`,
// src/core/Camera.cpp): que se encienda con su tecla (B por defecto,
// PED_TOGGLE_1RST_PERSON), que la vista se MANTENGA al caminar (no caduca a los
// 2,85 s ni se cae al moverse), que se pueda disparar dentro del modo y que se
// cierre al apuntar con mira y al subir a un coche.
//
// Evidencia que usa:
//   - odtrace.log: líneas `CAM1P ...` de la instrumentación de
//     `CCamera::CamControl` (src/core/Camera.cpp, sección 3). Emite la primera
//     vez que se evalúa el bloque y cada vez que cambia algo de lo vigilado
//     (no una por frame): `tog=` estado del conmutador, `togkey=` su tecla
//     pulsada, `mode=` modo de cámara (41 = MODE_1STPERSON_RUNABOUT, 4 =
//     MODE_FOLLOWPED), `spd=` velocidad a pie cuantizada (prueba de que anda).
//   - capturas PNG de cada fase en %TEMP%\vc-firstperson.
//
// Uso:
//   node gta_vc_browser/tools/firstperson-smoke-test.mjs
// Requisitos: servidor de desarrollo en marcha (cd gta_vc_browser/web && npm run dev)
// y Chrome instalado. Si hay otra sonda corriendo, espera (swiftshader es CPU).
//
// Variables de entorno (todas opcionales):
//   VC_URL        default http://localhost:2077
//   VC_SAVE       save a sembrar (default: tools/testdata/GTAVCsf1.b)
//   VC_SHOTS      carpeta de capturas (default: <tmp>/vc-firstperson)
//   VC_PROFILE    perfil de Chrome del test (persistente)
//   VC_ODTRACE    ruta de odtrace.log (default gta_vc_browser/web/odtrace.log)
//   VC_PHASE      'base' (sin cheat ni fases de coche) | 'todo' (default)
//   PUPPETEER_DIR node_modules con puppeteer-core
//   CHROME        ruta de chrome.exe
//
// Bindings por defecto del port (ControllerConfig.cpp) que usa la sonda:
//   conmutador de 1ª persona = B, caminar = W/A/S/D, apuntar = botón derecho,
//   disparar = Ctrl izq o Num0, entrar/salir de coche = F (o Enter).
//
// Salida: PASS si llega a partida, enciende el modo, anda dentro de él sin
// perderlo, dispara dentro de él y sale al apuntar; 0 errores de página.
// Exit code 0 = PASS.

import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));            // gta_vc_browser/tools
const WEB = path.join(HERE, '..', 'web');                             // gta_vc_browser/web
const URL_BASE = process.env.VC_URL || 'http://localhost:2077';
const ODTRACE = process.env.VC_ODTRACE || path.join(WEB, 'odtrace.log');
const PROFILE = process.env.VC_PROFILE || path.join(os.tmpdir(), 'vc-firstperson-test-profile2');
const CHROME = process.env.CHROME || 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
const SAVE = process.env.VC_SAVE || path.join(HERE, 'testdata', 'GTAVCsf1.b');
const SHOTS = process.env.VC_SHOTS || path.join(os.tmpdir(), 'vc-firstperson');
const PHASE = process.env.VC_PHASE || 'todo';

// Cheat de armas: hace falta un arma para disparar en 1ª persona.
const CHEAT = 'CRAZYTOOLS';

// Modos de cámara que nos interesan (enum CCam, src/core/Camera.h).
const MODE_FOLLOWPED = 4;             // 3ª persona de serie
const MODE_1P_RUNABOUT = 41;          // 1ª persona de PC (la del conmutador)
const MODE_1P_PEEK = 16;              // 1ª persona del "peek" de serie

// Coordenadas del frontend como fracción del canvas (el menú lo dibuja el motor).
const FRAC = {
  start: [0.5, 325 / 800],      // INICIAR PARTIDA
  load: [0.5, 345 / 800],       // CARGAR PARTIDA
  slot1: [0.3125, 180 / 800],   // primera fila (GTAVCsf1.b = slot 0)
  yes: [0.5, 418 / 800],        // SÍ en "¿Cargar la partida y continuar jugando?"
};

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

function traceSize() {
  try { return fs.statSync(ODTRACE).size; } catch (e) { return 0; }
}

function traceFrom(offset) {
  try {
    const t = fs.readFileSync(ODTRACE, 'utf8');
    return t.length > offset ? t.slice(offset) : '';
  } catch (e) { return ''; }
}

// Líneas CAM1P del tramo pedido, con la marca de tiempo que pone el servidor.
function cam1pLines(txt) {
  return txt.split('\n')
    .map((l) => l.trim())
    .filter((l) => /CAM1P/.test(l))
    .map((l) => {
      const m = l.match(/^(\S+Z)\s+CAM1P\s+(.*)$/);
      return { t: m ? Date.parse(m[1]) : 0, raw: m ? m[2] : l };
    });
}

function cam1pField(line, name) {
  const m = line.match(new RegExp(name + '=(-?[\\d.]+)'));
  return m ? Number(m[1]) : NaN;
}

function cam1pHas(line, name, value) {
  return cam1pField(line, name) === value;
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

// Resultados de cada comprobación del bloque C1.
const checks = [];
const check = (nombre, bien, detalle) => {
  checks.push({ nombre, bien: !!bien, detalle: detalle || '' });
  console.log('   [' + (bien ? 'ok ' : 'MAL') + '] ' + nombre + (detalle ? ' — ' + detalle : ''));
};

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
  // Sin caché HTTP: el servidor manda no-store, pero un perfil persistente con
  // una copia vieja de reVC.js mezclada con un reVC.wasm nuevo aborta el motor
  // (`_asyncify_start_unwind is not a function`).
  await page.setCacheEnabled(false).catch(() => {});
  page.on('console', (m) => {
    const t = m.text();
    consola.push(t.slice(0, 300));
    if (/memory access out of bounds|RuntimeError|Aborted|out of bounds/i.test(t)) {
      errors.push(t.slice(0, 1800));
      // A la vista, con hora y con la pila entera: así se sabe en QUÉ fase se
      // cayó el motor y quién llamaba.
      console.log('!! ' + new Date().toLocaleTimeString() + ' ' + t.slice(0, 1200));
    }
  });
  page.on('pageerror', (e) => errors.push('PAGEERROR ' + String(e).slice(0, 300)));

  console.log('== arrancando ' + URL_BASE + ' (perfil ' + PROFILE + ')');
  await page.goto(URL_BASE + '/?autostart=1', { waitUntil: 'domcontentloaded', timeout: 60000 });

  // 0) sembrar la partida en /userfiles en cuanto exista el FS virtual.
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

  // 1) esperar al MENÚ (GS_FRONTEND = state 7 en el heartbeat de la página).
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
  // Qué hay en el directorio de usuario: los bindings guardados (si existe
  // gta_vc.set) podrían no ser los por defecto.
  const userfiles = await page.evaluate(() => {
    try { return FS.readdir('/userfiles'); } catch (e) { return ['ERR ' + e]; }
  }).catch(() => ['(sin FS)']);
  console.log('== /userfiles: ' + userfiles.join(', '));
  const setinfo = await page.evaluate(() => {
    try { return 'gta_vc.set size=' + FS.stat('/userfiles/gta_vc.set').size; }
    catch (e) { return 'gta_vc.set: no existe'; }
  }).catch(() => '(sin FS)');
  console.log('== ' + setinfo);

  const rect = await page.evaluate(() => {
    const c = document.querySelector('.vc-canvas');
    if (!c) return null;
    const r = c.getBoundingClientRect();
    return [r.x, r.y, r.width, r.height];
  });
  if (!rect) { console.log('FAIL: no encuentro el canvas del juego (.vc-canvas)'); process.exit(1); }
  const at = ([fx, fy]) => [rect[0] + fx * rect[2], rect[1] + fy * rect[3]];
  const centro = [rect[0] + rect[2] / 2, rect[1] + rect[3] / 2];
  const slowClick = async (x, y) => {
    await page.mouse.move(x, y, { steps: 3 }).catch(() => {});
    await sleep(700);
    await page.mouse.down().catch(() => {});
    await sleep(350);
    await page.mouse.up().catch(() => {});
    await sleep(3000);
  };
  const shot = async (name) => {
    await page.screenshot({ path: path.join(SHOTS, name), timeout: 20000 }).catch(() => {});
  };
  const pulsa = async (key, ms) => {          // tecla normal (letras llegan bien)
    await page.keyboard.down(key).catch(() => {});
    await sleep(ms || 120);
    await page.keyboard.up(key).catch(() => {});
  };

  // Teclas del teclado numérico y Escape por CDP crudo, con `location: 3`
  // (bloque numérico): con page.keyboard a secas el navegador las entrega como
  // dígitos de la fila superior y el motor no ve nunca el numérico.
  const cdp = await page.target().createCDPSession();
  const CDP_KEYS = {
    Numpad0: { code: 'Numpad0', key: '0', vk: 96, loc: 3 },
    Numpad4: { code: 'Numpad4', key: '4', vk: 100, loc: 3 },
    Numpad5: { code: 'Numpad5', key: '5', vk: 101, loc: 3 },
    Numpad6: { code: 'Numpad6', key: '6', vk: 102, loc: 3 },
    Numpad8: { code: 'Numpad8', key: '8', vk: 104, loc: 3 },
    Escape: { code: 'Escape', key: 'Escape', vk: 27, loc: 0 },
  };
  const cdpKey = async (name, down) => {
    const d = CDP_KEYS[name];
    if (!d) return;
    await cdp.send('Input.dispatchKeyEvent', {
      type: down ? 'rawKeyDown' : 'keyUp',
      windowsVirtualKeyCode: d.vk,
      nativeVirtualKeyCode: d.vk,
      code: d.code,
      key: d.key,
      location: d.loc,
    }).catch(() => {});
  };

  // 2) INICIAR PARTIDA > CARGAR JUEGO > slot 1 > SÍ (con un reintento del SÍ).
  await slowClick(...at(FRAC.start));
  await slowClick(...at(FRAC.load));
  await slowClick(...at(FRAC.slot1));
  await slowClick(...at(FRAC.yes));
  let requested = false;
  for (let i = 0; i < 12 && !requested; i++) {
    await sleep(2500);
    requested = /WR load-req/.test(traceFrom(Math.max(0, traceSize() - 200000)));
    if (!requested && i === 3) await slowClick(...at(FRAC.yes));
  }

  // 3) esperar a partida. El "¿estoy en partida?" se mira en el heartbeat de
  // MI pestaña (#gamelog: [web] tick N state M ...), no en odtrace.log: otras
  // sondas de otros agentes escriben en el mismo fichero y sus FPHASE
  // enmascararían que mi página no ha cargado.
  const tLoad = Date.now();
  let inGame = false;
  let ultimoEstado = -1;
  while (Date.now() - tLoad < LOAD_TIMEOUT_MS) {
    const log = await page.$eval('#gamelog', (e) => e.textContent).catch(() => '');
    const st = (log.match(/state (\d+)/g) || []).map((s) => Number(s.split(' ')[1]));
    if (st.length) ultimoEstado = st[st.length - 1];
    if (ultimoEstado === 9) { inGame = true; break; }   // GS_PLAYING_GAME
    if (errors.some((e) => /out of bounds|RuntimeError|Aborted/.test(e))) break;
    await sleep(3000);
  }
  console.log('== en partida: ' + (inGame ? 'sí' : 'NO')
    + ' (último estado del motor: ' + ultimoEstado + '; 7=menú, 8=init partida, 9=partida)');
  if (!inGame) {
    console.log('== consola de mi pestaña (últimas líneas):');
    for (const l of consola.slice(-12)) console.log('   ' + l);
  }

  const hoy = () => new Date().toLocaleTimeString();
  const segmentos = [];                 // { nombre, desde, hasta } por fase
  const abreSegmento = (nombre) => segmentos.push({ nombre, desde: traceSize(), hasta: 0 });
  const cierraSegmento = () => { const s = segmentos[segmentos.length - 1]; if (s) s.hasta = traceSize(); };
  const lineasDe = (s) => cam1pLines(traceFrom(s.desde).slice(0, Math.max(0, s.hasta - s.desde)));

  if (inGame) {
    // Foco del canvas (los eventos de teclado van a la página).
    await page.mouse.move(centro[0], centro[1], { steps: 2 }).catch(() => {});
    await page.mouse.click(centro[0], centro[1]).catch(() => {});
    await sleep(1500);

    // --- Fase 0: asentar la partida ------------------------------------------
    // Con la caché on-demand fría, los primeros segundos de partida traen
    // modelos/collision en vuelo y el motor puede caer (`memory access out of
    // bounds` en ProcessLineOfSightSectorList). Dejar reposar la partida antes
    // de medir evita confundir eso con un fallo del bloque C1.
    console.log('\n== FASE 0 (' + hoy() + '): asentar la partida (12 s sin tocar nada)');
    for (let i = 0; i < 3; i++) {
      await sleep(4000);
      if (errors.length) break;
    }
    if (errors.length) console.log('   (el motor se cayó durante el asentamiento)');

    // --- Fase 1: 3ª persona (contraste) --------------------------------------
    console.log('== FASE 1 (' + hoy() + '): 3ª persona (contraste)');
    abreSegmento('base-3a');
    await sleep(1200);
    await shot('00-3a-persona.png');
    cierraSegmento();

    // --- Fase 2: encender el conmutador --------------------------------------
    console.log('== FASE 2 (' + hoy() + '): tecla B -> 1ª persona');
    abreSegmento('encender');
    await pulsa('KeyB', 120);
    await sleep(3500);
    await shot('10-1a-persona.png');
    // Mirar con el ratón dentro del modo (la 1ª persona de PC es la del ratón).
    await page.mouse.move(centro[0], centro[1], { steps: 4 }).catch(() => {});
    await page.mouse.move(centro[0] + 160, centro[1] + 40, { steps: 12 }).catch(() => {});
    await sleep(900);
    await shot('11-1a-mirar-con-raton.png');
    cierraSegmento();

    // --- Fase 3: caminar dentro del modo -------------------------------------
    console.log('== FASE 3 (' + hoy() + '): caminar dentro de la 1ª persona (3,5 s)');
    abreSegmento('caminar');
    await page.keyboard.down('KeyW').catch(() => {});
    await sleep(1000);
    await shot('12-1a-caminando-a.png');
    await sleep(2500);
    await page.mouse.move(centro[0] - 120, centro[1], { steps: 8 }).catch(() => {});
    await shot('13-1a-caminando-b.png');
    await page.keyboard.up('KeyW').catch(() => {});
    await sleep(1200);
    cierraSegmento();

    // --- Fase 4: disparar dentro del modo ------------------------------------
    if (PHASE !== 'base') {
      console.log('== FASE 4 (' + hoy() + '): cheat ' + CHEAT + ' y disparar en 1ª persona');
      for (const ch of CHEAT) { await page.keyboard.press('Key' + ch).catch(() => {}); await sleep(140); }
      console.log('   cheat tecleado, esperando a que lo procese');
      await sleep(9000);
      abreSegmento('disparar');
      await page.keyboard.down('ControlLeft').catch(() => {});
      await sleep(1600);
      await shot('20-1a-disparando.png');
      await page.keyboard.up('ControlLeft').catch(() => {});
      await sleep(400);
      await cdpKey('Numpad0', true); await sleep(1200); await cdpKey('Numpad0', false);
      await sleep(600);
      await shot('21-1a-tras-disparar.png');
      cierraSegmento();
    }

    // --- Fase 5: apuntar con mira debe sacar del modo ------------------------
    console.log('== FASE 5 (' + hoy() + '): apuntar -> debe volver a 3ª persona');
    abreSegmento('apuntar');
    // Primero el botón derecho; si el port no lo tiene atado — ojo:
    // `CMousePointerStateHelper::GetMouseSetUp()` sólo da por buenos LMB/RMB/MMB
    // si el cursor no está en (0,0) al arrancar el motor, y en el arnés headless
    // sí lo está, así que aquí el ratón puede no tener bindings — la tecla de
    // apuntar por defecto (Supr) da la misma señal al pad (RightShoulder1).
    await page.mouse.move(centro[0], centro[1], { steps: 3 }).catch(() => {});
    await page.mouse.down({ button: 'right' }).catch(() => {});
    await sleep(1500);
    await shot('30-tras-apuntar-raton.png');
    await page.mouse.up({ button: 'right' }).catch(() => {});
    await sleep(800);
    await pulsa('Delete', 150);
    await sleep(2500);
    await shot('31-tras-apuntar-tecla.png');
    cierraSegmento();

    // --- Fase 6: coche (mejor esfuerzo: necesita un coche al lado) -----------
    if (PHASE !== 'base') {
      console.log('== FASE 6 (' + hoy() + '): encender, buscar un coche (F)');
      abreSegmento('coche');
      await pulsa('KeyB', 120);
      await sleep(2500);
      await shot('40-1a-antes-de-coche.png');
      // Andar un poco en 1ª persona antes de intentar subir (por si el coche del
      // save queda a un par de metros).
      await page.keyboard.down('KeyW').catch(() => {});
      await sleep(2000);
      await page.keyboard.up('KeyW').catch(() => {});
      await sleep(600);
      await pulsa('KeyF', 150);
      await sleep(3000);
      await pulsa('KeyF', 150);
      await sleep(3000);
      await shot('41-tras-coche.png');
      cierraSegmento();
    }

    // --- Fase 7: apagar el conmutador a mano ---------------------------------
    console.log('== FASE 7 (' + hoy() + '): tecla B de nuevo -> 3ª persona');
    abreSegmento('apagar');
    // Si la fase 5 o 6 ya lo apagaron, la primera B lo encendería: se pulsa
    // hasta que la traza diga que está en 3ª persona (tog=0).
    for (let i = 0; i < 3; i++) {
      await pulsa('KeyB', 120);
      await sleep(2200);
      const l = lineasDe(segmentos[segmentos.length - 1]);
      if (l.length && cam1pHas(l[l.length - 1].raw, 'tog', 0)) break;
    }
    await sleep(2200);
    await shot('50-final-3a-persona.png');
    cierraSegmento();
  }

  // --- Veredicto -------------------------------------------------------------
  console.log('\n== CAM1P por fase:');
  const porFase = {};
  for (const s of segmentos) {
    const l = lineasDe(s);
    porFase[s.nombre] = l;
    console.log('   ' + s.nombre + ': ' + l.length + ' líneas');
    for (const x of l.slice(0, 8)) console.log('      ' + x.raw);
    if (l.length > 8) console.log('      ... (' + (l.length - 8) + ' más)');
  }

  if (inGame) {
    const enc = porFase['encender'] || [];
    const cam = porFase['caminar'] || [];
    const dis = porFase['disparar'] || [];
    const apu = porFase['apuntar'] || [];
    const fin = porFase['apagar'] || [];
    const lineaEncendido = enc.find((l) => cam1pHas(l.raw, 'tog', 1));
    const cfg = enc[0] || cam1pLines(traceFrom(0)).slice(-1)[0];
    if (cfg) console.log('== configuración: ctrl=' + cam1pField(cfg.raw, 'ctrl')
      + ' (0=Standard/ratón, 1=Classic) mouse3d=' + cam1pField(cfg.raw, 'mouse3d'));

    check('la tecla del conmutador llega al motor',
      enc.some((l) => cam1pHas(l.raw, 'togkey', 1)) || !!lineaEncendido,
      'togkey=1 visto en ' + enc.filter((l) => cam1pHas(l.raw, 'togkey', 1)).length + ' líneas');
    check('el modo se enciende al pulsar B', !!lineaEncendido,
      lineaEncendido ? 'mode=' + cam1pField(lineaEncendido.raw, 'mode') : 'sin línea con tog=1');
    check('la 1ª persona es la de PC (mode=' + MODE_1P_RUNABOUT + ')',
      enc.some((l) => cam1pHas(l.raw, 'tog', 1) && cam1pHas(l.raw, 'mode', MODE_1P_RUNABOUT)),
      'modos vistos con tog=1: ' + [...new Set(enc.filter((l) => cam1pHas(l.raw, 'tog', 1))
        .map((l) => cam1pField(l.raw, 'mode')))].join(', '));

    const caminando = cam.filter((l) => cam1pHas(l.raw, 'tog', 1) && cam1pField(l.raw, 'spd') > 0);
    const seCayo = cam.filter((l) => cam1pHas(l.raw, 'tog', 0));
    const andaPeroNoMode = cam.filter((l) => cam1pHas(l.raw, 'tog', 1)
      && cam1pField(l.raw, 'spd') > 0 && !cam1pHas(l.raw, 'mode', MODE_1P_RUNABOUT));
    check('sigue en 1ª persona al caminar', caminando.length > 0 && andaPeroNoMode.length === 0,
      'líneas andando con tog=1 y spd>0: ' + caminando.length
      + (andaPeroNoMode.length ? ' (de ellas, fuera del modo: ' + andaPeroNoMode.length + ')' : ''));
    check('el modo no se cae solo al moverse', seCayo.length === 0,
      seCayo.length ? seCayo[0].raw : 'ninguna línea con tog=0 durante la caminata');

    if (dis.length) {
      check('se puede disparar sin salir del modo',
        dis.some((l) => cam1pHas(l.raw, 'tog', 1)),
        'líneas con tog=1 durante el disparo: ' + dis.filter((l) => cam1pHas(l.raw, 'tog', 1)).length);
    }

    const trasApuntar = apu.filter((l) => cam1pHas(l.raw, 'tog', 0));
    check('apuntar con mira devuelve a 3ª persona', trasApuntar.length > 0,
      trasApuntar.length ? trasApuntar[trasApuntar.length - 1].raw : 'ninguna línea con tog=0 al apuntar');

    const coche = porFase['coche'] || [];
    if (coche.length) {
      const montado = coche.some((l) => cam1pHas(l.raw, 'tog', 0))
        || coche.some((l) => cam1pHas(l.raw, 'mode', MODE_FOLLOWPED));
      console.log('   [info] coche: ' + (montado
        ? 'el modo se cerró (subió al coche o ya estaba fuera)'
        : 'no se cerró (¿no había coche al lado? no es fallo de la sonda)'));
    }

    check('la última línea del bloque acaba en 3ª persona (tog=0 y mode!=41)',
      !!fin.length && cam1pHas(fin[fin.length - 1].raw, 'tog', 0)
        && !cam1pHas(fin[fin.length - 1].raw, 'mode', MODE_1P_RUNABOUT),
      'última línea de la fase: ' + (fin.length ? fin[fin.length - 1].raw : '(sin líneas)'));
  }

  const shots = fs.existsSync(SHOTS) ? fs.readdirSync(SHOTS).filter((f) => f.endsWith('.png')) : [];
  console.log('== capturas (' + shots.length + '): ' + SHOTS);
  console.log('== errores de página: ' + (errors.length ? errors.slice(0, 3).join(' | ') : 'ninguno'));

  const fallos = checks.filter((c) => !c.bien);
  ok = inGame && !errors.length && checks.length > 0 && fallos.length === 0;
  console.log('\n' + (ok ? 'PASS' : 'FAIL') + ' — C1 (conmutador de 1ª persona): '
    + checks.filter((c) => c.bien).length + '/' + checks.length + ' comprobaciones'
    + (fallos.length ? '; fallan: ' + fallos.map((f) => f.nombre).join(', ') : '')
    + (!inGame ? ' (no llegó a partida)' : '')
    + (errors.length ? ' (RuntimeError en la pestaña)' : ''));
  await browser.close().catch(() => {});
  process.exit(ok ? 0 : 1);
} catch (e) {
  console.log('FAIL: excepción del arnés: ' + String(e).slice(0, 400));
  if (browser) await browser.close().catch(() => {});
  process.exit(1);
}
