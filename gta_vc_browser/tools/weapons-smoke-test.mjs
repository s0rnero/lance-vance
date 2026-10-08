// Sonda de armas nuevas (Vice Extended, pack 3): carga la partida del slot 0,
// teclea el cheat CRAZYTOOLS, dispara, cicla armas con el teclado numerico y deja
// capturas en disco. Falla si algún TXD/bloque de animación nuevo no carga, si
// algún fichero falta en el FS virtual o si la pestaña tira un RuntimeError.
//
// Por qué existe: las armas nuevas son datos (default.ide/weapon.dat + DFF/TXD
// + IFP en gta3.img) *y* código (enum, sonidos, proyectil propio del
// lanzagranadas). Nada de eso se ve en el test de carga del slot 0, que solo
// comprueba que el init no muere. Ver .agents/plans/vice-extended-inclusion.md.
//
// Uso:
//   node gta_vc_browser/tools/weapons-smoke-test.mjs
// Requisitos: servidor de desarrollo en marcha (cd gta_vc_browser/web && npm run dev)
// y Chrome instalado.
//
// Variables de entorno (todas opcionales):
//   VC_URL        default http://localhost:2077
//   VC_SAVE       save a sembrar (default: tools/testdata/GTAVCsf1.b)
//   VC_SHOTS      carpeta de capturas (default: <tmp>/vc-weapons)
//   VC_PROFILE    perfil de Chrome del test (persistente)
//   VC_ODTRACE    ruta de odtrace.log (default gta_vc_browser/web/odtrace.log)
//   PUPPETEER_DIR node_modules con puppeteer-core
//   CHROME        ruta de chrome.exe
//
// Notas de los bindings (por defecto del port, ControllerConfig.cpp):
//   disparar        = Ctrl izq o Num0   (PED_FIREWEAPON = rsPADINS)
//   ciclar arma izq = Dec del numérico  (PED_CYCLE_WEAPON_LEFT = rsPADDEL), NO Supr
//   cheat           = teclado, letra a letra (CPad::AddToPCCheatString lee el
//                     evento de tecla, no el estado por frame)
//
// Salida: PASS/FAIL + resumen (TXD vistos, bloques de anim, ficheros del FS).
// Exit code 0 = PASS.

import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));            // gta_vc_browser/tools
const WEB = path.join(HERE, '..', 'web');                             // gta_vc_browser/web
const URL_BASE = process.env.VC_URL || 'http://localhost:2077';
const ODTRACE = process.env.VC_ODTRACE || path.join(WEB, 'odtrace.log');
const PROFILE = process.env.VC_PROFILE || path.join(os.tmpdir(), 'vc-weapons-test-profile');
const CHROME = process.env.CHROME || 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
const SAVE = process.env.VC_SAVE || path.join(HERE, 'testdata', 'GTAVCsf1.b');
const SHOTS = process.env.VC_SHOTS || path.join(os.tmpdir(), 'vc-weapons');

// Coordenadas del frontend como fracción del canvas (el menú lo dibuja el motor).
const FRAC = {
  start: [0.5, 325 / 800],      // INICIAR PARTIDA
  load: [0.5, 345 / 800],       // CARGAR PARTIDA
  slot1: [0.3125, 180 / 800],   // primera fila (GTAVCsf1.b = slot 0)
  yes: [0.5, 418 / 800],        // SÍ en "¿Cargar la partida y continuar jugando?"
};

const CHEAT = 'CRAZYTOOLS';
// TXD de las armas nuevas (nombre del modelo = nombre del fichero .txd) y
// bloques de animación propios (deagle.ifp / steyr.ifp). rocket.ifp no se pide
// salvo que el jugador lleve el lanzacohetes, así que va aparte.
const TXD_ARMAS = ['beretta', 'desert_eagle', 'shotgun2', 'uziold', 'ak47', 'm16', 'steyr', 'gr_launch'];
const ANIM_ARMAS = ['deagle', 'steyr'];
const FS_FICHEROS = [
  '/models/gta3.img/deagle.ifp', '/models/gta3.img/steyr.ifp',
  '/models/gta3.img/beretta.dff', '/models/gta3.img/gr_launch.dff',
  '/models/gta3.img/grenade2.dff', '/models/gta3.img/desert_eagle.dff',
  '/models/gta3.img/m16.dff',
];

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

  // 2) recordar el save (ya sembrado) y el rectángulo del canvas.
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

  // 3) INICIAR PARTIDA > CARGAR JUEGO > slot 1 > SÍ (con un reintento del SÍ).
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

  // 4) esperar a partida (FPHASE = los frames se miden por fases).
  const tLoad = Date.now();
  let pass = false, animFail = false;
  while (Date.now() - tLoad < LOAD_TIMEOUT_MS) {
    if (traceSince('rotado: sesion nueva', /ODANIMFAIL/).length) { animFail = true; break; }
    if (traceSince('rotado: sesion nueva', /FPHASE/).length >= 3) { pass = true; break; }
    if (errors.some((e) => /out of bounds|RuntimeError|Aborted/.test(e))) break;
    await sleep(3000);
  }
  console.log('== en partida: ' + (pass ? 'sí' : 'NO'));

  // 5) cheat de armas nuevas, disparo y ciclo de armas.
  const marcaArmas = traceSize();
  if (pass) {
    await page.mouse.move(rect[2] / 2, rect[3] / 2, { steps: 2 }).catch(() => {});
    await page.mouse.click(rect[2] / 2, rect[3] / 2).catch(() => {});
    await sleep(600);
    for (const ch of CHEAT) {
      await page.keyboard.press('Key' + ch).catch(() => {});
      await sleep(140);
    }
    console.log('== cheat tecleado: ' + CHEAT);
    await sleep(10000);
    await page.screenshot({ path: path.join(SHOTS, '00-primera-arma.png'), timeout: 20000 }).catch(() => {});

    // Disparar: así el arma se ve en mano y, con el lanzagranadas, se ejercita
    // el proyectil nuevo (grenade2) que añade este pack.
    for (const tecla of ['ControlLeft', 'Numpad0']) {
      await page.keyboard.down(tecla).catch(() => {});
      await sleep(600);
      await page.screenshot({ path: path.join(SHOTS, '01-disparo-' + tecla + '.png'), timeout: 20000 }).catch(() => {});
      await sleep(900);
      await page.keyboard.up(tecla).catch(() => {});
      await sleep(1200);
    }
    // Ciclo de armas (Dec del numérico), disparando en cada una.
    for (let i = 1; i <= 10; i++) {
      await page.keyboard.press('NumpadDecimal').catch(() => {});
      await sleep(1500);
      await page.keyboard.down('ControlLeft').catch(() => {});
      await sleep(500);
      await page.screenshot({ path: path.join(SHOTS, String(i + 1).padStart(2, '0') + '-arma.png'), timeout: 20000 }).catch(() => {});
      await page.keyboard.up('ControlLeft').catch(() => {});
      await sleep(900);
    }
  }

  // 6) comprobaciones: TXD por odtrace, bloques de anim por consola, FS virtual.
  const tail = traceFrom(marcaArmas);
  const txd = [...new Set((tail.match(/TXDIN txd=([a-z0-9_]+)/gi) || []).map((x) => x.split('=')[1].toLowerCase()))];
  // Los bloques de animación del mod se cargan en su .ifp y el motor lo cuenta
  // por consola (debug()), no en odtrace: de ahí que se mire `consola`.
  const consolaAnim = [...new Set(consola.filter((l) => /stream-anim|ODANIM/i.test(l)))];
  const anims = ANIM_ARMAS.filter((a) => consolaAnim.some((l) => l.includes('stream-anim ' + a)));
  const faltanTxd = TXD_ARMAS.filter((t) => !txd.includes(t));
  const faltanAnim = ANIM_ARMAS.filter((a) => !consolaAnim.some((l) => l.includes('stream-anim ' + a)));
  const fsState = await page.evaluate((files) => files.map((f) => {
    try { return f.replace('/models/gta3.img/', '') + '=' + FS.stat(f).size; } catch (e) { return f.replace('/models/gta3.img/', '') + '=FALTA'; }
  }), FS_FICHEROS).catch(() => ['(FS no consultable)']);
  const fsOk = fsState.every((x) => !x.endsWith('=FALTA') && !x.startsWith('('));

  console.log('== armas: TXD vistos tras el cheat: ' + (txd.length ? txd.join(', ') : '(ninguno)'));
  console.log('== armas: bloques de anim en consola: ' + (anims.length ? anims.join(', ') : '(ninguno)')
    + ' [' + consolaAnim.length + ' líneas de animación]');
  console.log('== armas: TXD que faltan: ' + (faltanTxd.length ? faltanTxd.join(', ') : 'ninguno'));
  console.log('== armas: bloques que faltan: ' + (faltanAnim.length ? faltanAnim.join(', ') : 'ninguno'));
  console.log('== armas: FS -> ' + fsState.join(' '));
  console.log('== armas: capturas en ' + SHOTS);
  console.log('== líneas FPHASE: ' + traceSince('rotado: sesion nueva', /FPHASE/).length);
  console.log('== errores de página: ' + (errors.length ? errors.slice(0, 3).join(' | ') : 'ninguno'));
  if (errors.length) console.log('\nERROR:\n' + errors.slice(0, 3).join('\n'));

  ok = pass && requested && !animFail && !errors.length && fsOk &&
    faltanTxd.length === 0 && faltanAnim.length === 0 && txd.length > 0;
  console.log('\n' + (ok ? 'PASS' : 'FAIL') + ' — armas nuevas ' + (ok
    ? 'cargadas, disparadas y cicladas sin crash'
    : !pass ? 'sin llegar a partida'
      : animFail ? 'con bloques de anim sin cargar (ODANIMFAIL)'
        : errors.length ? 'con RuntimeError en la pestaña'
          : faltanTxd.length ? 'sin cargar estos TXD: ' + faltanTxd.join(', ')
            : faltanAnim.length ? 'sin cargar estos bloques: ' + faltanAnim.join(', ')
              : !fsOk ? 'con ficheros nuevos ausentes del FS virtual'
                : 'sin teclear/ejecutar el cheat'));
  await browser.close().catch(() => {});
  process.exit(ok ? 0 : 1);
} catch (e) {
  console.log('FAIL: excepción del arnés: ' + String(e).slice(0, 400));
  if (browser) await browser.close().catch(() => {});
  process.exit(1);
}
