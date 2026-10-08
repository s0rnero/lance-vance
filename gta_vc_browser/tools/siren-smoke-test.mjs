// Sonda de la sirena policial (bloque D3): carga la partida del slot 0, teclea
// CRAZYRIDES (crea los 8 vehículos nuevos en un anillo alrededor del jugador) y
// deja capturas girando la cámara. Después mide las capturas con
// `tools/shot_stats.py` y decide:
//
//   * mide SERIES de la misma escena y misma cámara quieta (`shot_stats.py
//     --parpadeo`) y busca celdas que **alternan rojo y azul**: los dos canales
//     con rango apreciable y anti-correlacionados entre fotogramas. Eso es lo
//     que hace una sirena y lo que no hace nada más en una calle (las nubes, el
//     agua y el tráfico mueven los tres canales a la vez);
//   * la base (misma cámara, sin los vehículos del mod) es el control: tiene
//     que dar CERO celdas alternando. Si también alternara, lo que se mide no
//     es la sirena;
//   * la región medida recorta los márgenes del HUD, que es lo único del juego
//     que alterna rojo y azul por su cuenta (iconos del radar y del arma).
//
// Por qué no vale "píxel azul dominante": la sirena de VC suma ~42 al canal
// que le toca (los colores van divididos por 6 en el propio motor) sobre un
// fondo cálido, así que el azul sube sin llegar a dominar y la métrica no lo
// ve aunque la luz esté dibujándose. Ese fue el falso FAIL del primer arnés.
//
// Qué cubre: la moto policial del mod (6507) tiene el bloque de coronas de
// CAutomobile en CBike y la entrada de `CVehicle::UsesSiren()` detrás de
// VICEEXT_POLICE_BIKE_LIGHTS; el cheat la crea con la sirena encendida para que
// se pueda ver sin tener que conducirla. El coche de policía usa el MISMO
// camino de coronas (CAutomobile/MI_POLICE, código de serie sin tocar), así que
// el mismo arnés vale para comparar.
//
// Uso:
//   node gta_vc_browser/tools/siren-smoke-test.mjs
// Requisitos: servidor de desarrollo en marcha (cd gta_vc_browser/web && npm run dev),
// Chrome y python (para shot_stats.py).
//
// Variables de entorno (opcionales): VC_URL, VC_SAVE, VC_SHOTS, VC_PROFILE,
// VC_ODTRACE, PUPPETEER_DIR, CHROME, VC_PYTHON.

import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { execFileSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const WEB = path.join(HERE, '..', 'web');
const URL_BASE = process.env.VC_URL || 'http://localhost:2077';
const ODTRACE = process.env.VC_ODTRACE || path.join(WEB, 'odtrace.log');
const PROFILE = process.env.VC_PROFILE || path.join(os.tmpdir(), 'vc-siren-test-profile');
const CHROME = process.env.CHROME || 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
const SAVE = process.env.VC_SAVE || path.join(HERE, 'testdata', 'GTAVCsf1.b');
const SHOTS = process.env.VC_SHOTS || path.join(os.tmpdir(), 'vc-siren');
const PYTHON = process.env.VC_PYTHON || 'python';
const STATS = path.join(HERE, 'shot_stats.py');

const FRAC = {
  start: [0.5, 325 / 800],
  load: [0.5, 345 / 800],
  slot1: [0.3125, 180 / 800],
  yes: [0.5, 418 / 800],
};

const CHEAT = 'CRAZYRIDES';
const ANGULOS = 8;      // barridos de cámara (45º cada uno)
const SHOTS_POR_ANGULO = 4;
const MS_ENTRE_CAPTURAS = 450;  // el parpadeo va a 512 ms: muestrea fases distintas
// La serie de cada ángulo se toma con la cámara QUIETA: así lo único que cambia
// de un fotograma a otro es lo que parpadea (más el tráfico). Con la cámara
// girando, el cambio de vista inundaría la métrica.

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

// Mide una serie (misma escena, misma cámara) con tools/shot_stats.py
// `--parpadeo` y devuelve cuántas celdas **alternan rojo y azul** (rango
// apreciable en los dos canales y anti-correlacionados). Ésa es la firma de una
// sirena: nada más en una calle cambia de color así (las nubes y el agua mueven
// los tres canales a la vez; el HUD se recorta con la región).
function medirParpadeo(shots, region) {
  const args = [STATS, '--parpadeo', ...shots, '--celda', '24', '--top', '3'];
  if (region) args.push('--region', region.join(','));
  const out = execFileSync(PYTHON, args, {
    env: { ...process.env, PYTHONIOENCODING: 'utf-8' },
    encoding: 'utf8', maxBuffer: 32 * 1024 * 1024,
  });
  const m = out.match(/alternan[^:]*: (\d+)/);
  const celdas = out.split('\n').filter((l) => /ALTERNA/.test(l))
    .map((l) => l.split(' ').slice(1).join(' ').trim());
  return { texto: out.trim(), alternan: m ? +m[1] : -1, celdas };
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
  console.log('== save listo: ' + (await write()));

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

  // Ojo: `FPHASE` aparece en cuanto el motor dibuja, y con la caché IndexedDB
  // vacía la carga del mundo va por fases (el juego vuelve a "loading" varias
  // veces). Teclear el cheat en ese hueco revienta (los model info del IDE
  // todavía no existen). Se espera a que `WEBHB` diga `state=9 ingame=1` y se
  // mantenga: dos muestras seguidas con el mismo estado.
  const tLoad = Date.now();
  let pass = false, estable = 0;
  while (Date.now() - tLoad < LOAD_TIMEOUT_MS) {
    const webhb = traceSince('rotado: sesion nueva', /WEBHB state=/);
    const ultimo = webhb.length ? webhb[webhb.length - 1] : '';
    if (/state=9 ingame=1/.test(ultimo)) estable++; else estable = 0;
    if (traceSince('rotado: sesion nueva', /FPHASE/).length >= 3 && estable >= 2) {
      pass = true;
      break;
    }
    if (errors.some((e) => /out of bounds|RuntimeError|Aborted/.test(e))) break;
    await sleep(3000);
  }
  console.log('== en partida: ' + (pass ? 'sí (state=9 estable)' : 'NO'));
  if (pass) await sleep(3000);

  // Lo que las trazas del motor dicen del mecanismo (lo rellena el bloque de
  // capturas; el veredicto lo usa aunque no haya capturas).
  const traza = { sirenOn: 0, sirenLineas: 0, coronasMax: -1, coronasMin: -1,
    sinTexturaMax: -1, texNil: -1, texTotal: -1, luz: null };

  // Zona que se mide: el centro del canvas, recortando los márgenes donde vive
  // el HUD (radio/reloj/dinero arriba a la derecha, radar abajo). Sus iconos
  // parpadean solos y son lo único de una escena normal que alterna rojo y
  // azul; fuera de ellos, la alternancia sólo la puede producir una sirena.
  // El recorte va en fracción del canvas para valer con cualquier resolución.
  const REGION = [rect[0] + 0.06*rect[2], rect[1] + 0.06*rect[3],
    rect[0] + 0.78*rect[2], rect[1] + 0.78*rect[3]];
  console.log('== región medida (sin HUD): ' + REGION.map((v) => Math.round(v)).join(','));

  const capturas = [];
  // Posición de la luz en pantalla según el MOTOR, muestreada justo después de
  // cada captura: la traza BSIREN sale cada 20 fotogramas y la cámara no se
  // mueve durante una serie, así que la muestra vale para toda la serie. Esto
  // es lo que sustituye a proyectar la luz por mi cuenta y usar la mediana de
  // todas las poses (el fallo del primer arnés: apuntaba a otro sitio).
  const luzDe = new Map();
  const luzActual = () => {
    const ls = traceSince('rotado: sesion nueva', /BSIREN .*on=1 x=/);
    if (!ls.length) return null;
    const m = ls[ls.length - 1].match(/t=(\d+) on=1 x=(-?[\d.]+) y=(-?[\d.]+)/);
    if (!m) return null;
    const x = +m[2], y = +m[3];
    if (!(x > 0 && x < rect[2] && y > 0 && y < rect[3])) return null;
    return [x, y];
  };
  const mediana = (vals) => vals.slice().sort((a, b) => a - b)[Math.floor(vals.length / 2)];
  const tirar = async (nombre) => {
    const p = path.join(SHOTS, nombre);
    await page.screenshot({ path: p, timeout: 20000 }).catch(() => {});
    capturas.push(p);
    luzDe.set(nombre, luzActual());
    return p;
  };

  const antes = [];
  if (pass) {
    await page.mouse.move(rect[2] / 2, rect[3] / 2, { steps: 2 }).catch(() => {});
    await page.mouse.click(rect[2] / 2, rect[3] / 2).catch(() => {});
    await sleep(600);
    // Base: misma escena y misma cámara, sin vehículos del mod.
    for (let k = 0; k < SHOTS_POR_ANGULO; k++) {
      antes.push(await tirar('00-base-' + k + '.png'));
      if (k < SHOTS_POR_ANGULO - 1) await sleep(MS_ENTRE_CAPTURAS);
    }

    const marca = traceSize();
    for (const ch of CHEAT) {
      await page.keyboard.press('Key' + ch).catch(() => {});
      await sleep(140);
    }
    console.log('== cheat tecleado: ' + CHEAT);
    await sleep(10000);   // los coches caen desde 4 m y la sirena ya va puesta
    console.log('== consola (últimas): ' + (consola.slice(-6).join(' | ') || '(vacía)'));
    console.log('== odtrace (últimas): ' + (traceFrom(marca).split('\n').filter(Boolean).slice(-6).join(' | ') || '(vacío)'));

    // El cheat coloca la moto policial 7 m DELANTE del jugador (con la sirena
    // puesta), así que la primera serie —cámara quieta, sin girar— es la que
    // más probabilidad tiene de tenerla en el encuadre.
    for (let k = 0; k < SHOTS_POR_ANGULO; k++) {
      await tirar('0a-frente-' + k + '.png');
      if (k < SHOTS_POR_ANGULO - 1) await sleep(MS_ENTRE_CAPTURAS);
    }

    for (let g = 0; g < ANGULOS; g++) {
      await page.keyboard.down('ArrowLeft').catch(() => {});
      await sleep(1100);
      await page.keyboard.up('ArrowLeft').catch(() => {});
      await sleep(1600);   // que la cámara se asiente antes de la serie
      for (let k = 0; k < SHOTS_POR_ANGULO; k++) {
        await tirar('1' + g + '-giro-' + k + '.png');
        if (k < SHOTS_POR_ANGULO - 1) await sleep(MS_ENTRE_CAPTURAS);
      }
    }
    console.log('== capturas: ' + capturas.length + ' en ' + SHOTS);
    const desde = traceFrom(marca).split('\n');
    const resumen = (etiqueta, re, ejemplo) => {
      const ls = desde.filter((l) => re.test(l));
      console.log('== traza ' + etiqueta + ': ' + (ls.length
        ? ls.length + ' líneas' + (ejemplo ? ', p.ej. ' + ls[0].split(' ').slice(1).join(' ') : '')
        : 'NINGUNA'));
      return ls;
    };
    const bs = resumen('del bloque de sirena (BSIREN)', /BSIREN/);
    traza.sirenLineas = bs.length;
    traza.sirenOn = bs.filter((l) => /on=1/.test(l)).length;
    // Informativo: dónde cae la luz según el motor, juntando TODAS las poses de
    // cámara de la sesión (mediana). No se usa para medir: mezclar poses daba
    // una posición que no era la de ninguna serie (el fallo del primer arnés).
    // La ventana que decide se calcula por serie, con la muestra de esa pose.
    const dentro = bs.map((l) => l.match(/on=1 x=([\d.]+) y=([\d.]+)/))
      .filter((m) => m && +m[1] > 0 && +m[1] < rect[2] && +m[2] > 0 && +m[2] < rect[3])
      .map((m) => [ +m[1], +m[2] ])
      .sort((a, b) => a[0] - b[0]);
    if (dentro.length) {
      traza.luz = dentro[Math.floor(dentro.length / 2)];
      console.log('== la luz cae en pantalla en (' + traza.luz[0].toFixed(0) + ', ' +
        traza.luz[1].toFixed(0) + ') (mediana de ' + dentro.length + ' muestras encuadradas)');
    }
    console.log('== sirenas con la luz delante de la cámara: ' + traza.sirenOn + '/' + bs.length +
      (bs.length ? ' | última: ' + bs[bs.length - 1].split(' ').slice(1).join(' ') : ''));
    // Las texturas de corona se leen una sola vez, al inicializar el juego: se
    // buscan en toda la sesión, no en la ventana de capturas.
    const tex = traceSince('rotado: sesion nueva', /CORONA tex/);
    console.log('== traza de texturas de corona (CORONA): ' + (tex.length
      ? tex.length + ' líneas, p.ej. ' + tex[0].split(' ').slice(1).join(' ') : 'NINGUNA'));
    traza.texTotal = tex.length;
    traza.texNil = tex.filter((l) => /NIL/.test(l)).length;
    if (tex.length) console.log('== coronas sin textura al inicializar: ' + traza.texNil + '/' + tex.length);
    const rnd = desde.filter((l) => /CORONAREND/.test(l));
    console.log('== traza de dibujo de coronas (CORONAREND): ' + (rnd.length
      ? rnd.length + ' líneas, p.ej. ' + rnd[0].split(' ').slice(1).join(' ') : 'NINGUNA'));
    if (rnd.length) {
      const dib = rnd.map((l) => +(l.match(/dibujadas=(\d+)/) || [0, 0])[1]);
      const sin = rnd.map((l) => +(l.match(/sinTextura=(\d+)/) || [0, 0])[1]);
      traza.coronasMin = Math.min(...dib);
      traza.coronasMax = Math.max(...dib);
      traza.sinTexturaMax = Math.max(...sin);
      console.log('== coronas dibujadas: ' + traza.coronasMin + '..' + traza.coronasMax +
        ' | con textura nula: ' + Math.min(...sin) + '..' + traza.sinTexturaMax);
    }
  }

  // ---- veredicto de píxeles -------------------------------------------------
  // Se mide por SERIES (misma escena y misma cámara quieta). El veredicto es la
  // alternancia rojo/azul descrita arriba, comparada contra la base medida con
  // el mismo código y la misma región.
  const serieDe = (re) => capturas.filter((p) => re.test(path.basename(p)))
    .sort((a, b) => a.localeCompare(b, undefined, { numeric: true }));
  const region = REGION ? REGION.map((v) => Math.round(v)) : null;
  const baseShots = serieDe(/^00-base-/);
  const series = [];
  // Cada serie se mide en una ventana centrada en donde el MOTOR dice que está
  // la luz (mediana de las posiciones muestreadas en sus capturas) y, en esa
  // MISMA ventana, se mide la base como control: si la base también alterna
  // ahí, lo que se está viendo no es la sirena.
  const anadir = (nombre, shots) => {
    if (shots.length < 3) return;
    const luces = shots.map((p) => luzDe.get(path.basename(p))).filter((v) => v);
    let caja = null;
    if (luces.length >= 2) {
      const cx = Math.round(mediana(luces.map((v) => v[0])));
      const cy = Math.round(mediana(luces.map((v) => v[1])));
      caja = [Math.max(0, cx - 60), Math.max(0, cy - 60),
        Math.min(Math.round(rect[2]), cx + 60), Math.min(Math.round(rect[3]), cy + 60)];
    }
    const r = medirParpadeo(shots, caja || region);
    const control = caja && baseShots.length >= 3 ? medirParpadeo(baseShots, caja) : null;
    series.push({ nombre, r, caja, control });
    console.log('== ' + nombre + ': alternan=' + r.alternan +
      (caja ? ' en la ventana de la luz ' + caja.join(',') : ' (SIN posición de luz: región completa)') +
      (control ? ' | control (misma ventana, sin vehículos)=' + control.alternan : '') +
      (r.celdas.length ? ' | ' + r.celdas[0] : ''));
  };
  anadir('frente', serieDe(/^0a-frente-/));
  for (let g = 0; g < ANGULOS; g++) anadir('giro ' + g, serieDe(new RegExp('^1' + g + '-giro-')));
  const conCaja = series.filter((s) => s.caja && s.control);
  const candidatas = conCaja.filter((s) => s.r.alternan >= 1 && s.control.alternan === 0);
  console.log('== series medidas en la ventana de la luz: ' + conCaja.length + '/' + series.length +
    ' | de ellas, alternan con el control quieto: ' + candidatas.length);
  const sinLuz = conCaja.length === 0;

  // Mecanismo: la luz de la moto policial está delante de la cámara (traza
  // BSIREN con on=1) y el dibujo de coronas de ese fotograma dibuja 2 (las dos
  // de la sirena; el resto de la escena no tiene ninguna: la línea sale 0).
  const luzDelante = traza.sirenOn > 0;
  const dibujaDos = traza.coronasMax >= 2;
  // `odtrace.log` es un fichero compartido del servidor: si otro hilo reinicia
  // la sesión, estas trazas pueden quedar fuera de la ventana. Cuando no hay
  // NINGUNA, el veredicto se queda con la medición de píxeles (que es la fuerte:
  // alternancia con sirena y cero en la base) en vez de fallar por falta de
  // evidencia.
  const sinTrazas = traza.sirenLineas === 0 && traza.texTotal <= 0 && traza.coronasMax < 0;
  const mecanismo = sinTrazas || (luzDelante && dibujaDos && traza.texNil === 0);
  if (sinTrazas) console.log('== aviso: sin trazas del motor en la ventana ' +
    '(odtrace.log compartido); el veredicto se apoya sólo en los píxeles');

  // Lo que decide: la ventana donde el MOTOR sitúa la luz alterna rojo/azul en
  // la serie con sirena y está QUIETA en la base medida en esa misma ventana
  // (misma cámara, sin los vehículos del mod). Es un control apareado: lo que
  // se mueve por su cuenta (mar, agua, streaming de texturas) aparece en las
  // dos series y no decide.
  const mejor = candidatas[0] || null;
  console.log('== veredicto de píxeles: ' + (sinLuz
    ? 'SIN posición de la luz en la traza (odtrace.log compartido): no se decide'
    : candidatas.length + ' serie(s) con la ventana alternando y el control quieto' +
      (mejor ? ' — ' + mejor.nombre + ' ventana ' + mejor.caja.join(',') +
        ' | ' + (mejor.r.celdas[0] || '') : '')));
  console.log('== líneas FPHASE: ' + traceSince('rotado: sesion nueva', /FPHASE/).length);
  console.log('== errores de página: ' + (errors.length ? errors.slice(0, 3).join(' | ') : 'ninguno'));
  if (errors.length) console.log('\nERROR:\n' + errors.slice(0, 3).join('\n'));

  // Umbral: al menos una celda (24x24, del tamaño de la corona de cerca) que
  // alterne en la ventana de la luz y ninguna en esa misma ventana sin la
  // sirena. El control apareado es lo que hace innecesarios los umbrales de
  // color; y el techo de amplitud ([4,55]) descarta los cambios de escena.
  ok = pass && requested && !errors.length && !sinLuz && mecanismo && candidatas.length > 0;
  console.log('\n' + (ok ? 'PASS' : 'FAIL') + ' — sirena policial ' + (ok
    ? 'la ventana de la luz (' + mejor.nombre + ', ' + mejor.caja.join(',') + ') alterna ' +
      'rojo/azul en ' + mejor.r.alternan + ' celdas y su control (la misma ventana sin ' +
      'los vehículos del mod) no alterna; luz delante de la cámara y 2 coronas ' +
      'dibujadas' + (sinTrazas ? '' : ' con ' + traza.texTotal + ' texturas de corona cargadas')
    : !pass ? 'sin llegar a partida'
      : errors.length ? 'con RuntimeError en la pestaña'
        : sinLuz ? 'sin posición de la luz en la traza: la ventana no se puede situar ' +
            '(odtrace.log es compartido; reintenta con el servidor para esta sonda)'
          : !mecanismo ? 'el bloque no llega a dibujarse (luz delante: ' + traza.sirenOn +
              ', coronas dibujadas máx: ' + traza.coronasMax +
              ', coronas sin textura: ' + traza.texNil + ')'
            : 'la ventana de la luz no alterna (series medidas: ' + conCaja.length +
              ', candidatas: ' + candidatas.length + ')'));
  await browser.close().catch(() => {});
  process.exit(ok ? 0 : 1);
} catch (e) {
  console.log('FAIL: excepción del arnés: ' + String(e).slice(0, 400));
  if (browser) await browser.close().catch(() => {});
  process.exit(1);
}
