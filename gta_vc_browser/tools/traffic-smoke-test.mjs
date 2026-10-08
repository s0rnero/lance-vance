// Sonda de TRÁFICO (bloque D4): comprueba que los 8 vehículos del mod que
// traen clase de tráfico en su `.ide` aparecen **solos** por la calle, sin
// cheats y sin tocarlos con el jugador.
//
// Cómo mide: carga la partida del slot 0, se queda quieto en la calle N minutos
// y cuenta las líneas `CARSPAWN model=<id>` del motor. Esa traza se emite en
// `CCarCtrl::GenerateOneRandomCar` justo cuando el coche de la calle entra en el
// mundo (`CWorld::Add`), así que es la medición directa de qué circula. Contar
// `TXDIN` no vale: un modelo ya residente no vuelve a pedir su TXD y el tráfico
// recicla los modelos que tiene cargados (medido: 0-1 TXDIN en 4 minutos de
// calle, ninguno de vehículo).
//
//   * los 7 con clase real (`motorbike`, `poorfamily`, `big`, `normal`) tienen
//     que salir alguno; se pide un mínimo para no dar por bueno un solo coche
//     aparcado de las misiones;
//   * `polwintergreen` viene con clase `ignore` en el `.ide` (igual que el coche
//     de policía de serie): **no debe salir** en tráfico normal. Si sale, el
//     dato está mal o alguien lo está metiendo con `CRAZYRIDES`;
//   * se cuenta además cuántos modelos DISTINTOS en total pasan por la calle
//     (dato del motor, no del mod): si el tráfico normal está muerto, la sonda
//     no puede concluir nada de los nuevos.
//
// Uso:
//   node gta_vc_browser/tools/traffic-smoke-test.mjs           (4 minutos)
//   VC_TRAFFIC_MIN=8 node gta_vc_browser/tools/traffic-smoke-test.mjs
// Requisitos: servidor en marcha (cd gta_vc_browser/web && npm run dev) + Chrome.
// Variables: VC_URL, VC_SAVE, VC_SHOTS, VC_PROFILE, VC_ODTRACE, VC_TRAFFIC_MIN,
// PUPPETEER_DIR, CHROME.

import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const WEB = path.join(HERE, '..', 'web');
const URL_BASE = process.env.VC_URL || 'http://localhost:2077';
const ODTRACE = process.env.VC_ODTRACE || path.join(WEB, 'odtrace.log');
const PROFILE = process.env.VC_PROFILE || path.join(os.tmpdir(), 'vc-traffic-test-profile');
const CHROME = process.env.CHROME || 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
const SAVE = process.env.VC_SAVE || path.join(HERE, 'testdata', 'GTAVCsf1.b');
const SHOTS = process.env.VC_SHOTS || path.join(os.tmpdir(), 'vc-traffic');
const MINUTOS = Number(process.env.VC_TRAFFIC_MIN || 4);
const MS_OBSERVACION = MINUTOS * 60 * 1000;
const MS_MUESTREO = 15000;

const FRAC = {
  start: [0.5, 325 / 800],
  load: [0.5, 345 / 800],
  slot1: [0.3125, 180 / 800],
  yes: [0.5, 418 / 800],
};

// modelo del mod -> descripción (el id es el del `default.ide` servido)
const NUEVOS = {
  6500: 'Streetfighter (motorbike)',
  6501: 'Perennial (poorfamily)',
  6502: 'Trashmaster (big)',
  6503: 'Hellenbach (normal)',
  6504: 'Premier (normal)',
  6505: 'Manchez (motorbike)',
  6506: 'Wintergreen (motorbike)',
};
const POLICIA = 6507;               // clase `ignore`: NO debe salir en tráfico
const MIN_MODELOS = 3;              // cuántos de los 7 hay que ver para dar PASS
const MIN_SPAWNS = 20;              // tráfico mínimo para que la medida valga algo
const MIN_TICKS = 4;                // ticks del cargador (CARPED) en la ventana: sin ellos la medida no vale

const BOOT_TIMEOUT_MS = 150000;
const LOAD_TIMEOUT_MS = 300000;

const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

async function loadPuppeteer() {
  try { return (await import('puppeteer-core')).default; } catch (e) { /* fuera del arnés */ }
  const dir = process.env.PUPPETEER_DIR ||
    'C:/Users/s0rno/AppData/Local/Temp/opencode/vc-e2e/node_modules';
  const { createRequire } = await import('node:module');
  const req = createRequire(path.join(dir, 'anchor.js'));
  return req('puppeteer-core');
}

function traceFrom(offset) {
  try {
    const t = fs.readFileSync(ODTRACE, 'utf8');
    return t.length > offset ? t.slice(offset) : '';
  } catch (e) { return ''; }
}

function traceSize() {
  try { return fs.statSync(ODTRACE).size; } catch (e) { return 0; }
}

// `odtrace.log` lo escriben las tres sesiones del checkout a la vez: todo se
// busca a partir de la marca de esta sesión ("rotado: sesion nueva"), que el
// motor escribe al arrancar la partida, y no de la cola del fichero.
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
console.log('== save a cargar: ' + SAVE);

const puppeteer = await loadPuppeteer();
const errors = [];
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
    // El motor pide la carga del mundo ("load-req") al aceptar el save: es la
    // señal de que los clics entraron, antes de que haya nada que medir.
    requested = traceSince('rotado: sesion nueva', /WR load-req/).length > 0;
    if (!requested && i === 3) await slowClick(...at(FRAC.yes));
  }

  const tLoad = Date.now();
  let pass = false, estable = 0, reintentos = 0;
  while (Date.now() - tLoad < LOAD_TIMEOUT_MS) {
    const webhb = traceSince('rotado: sesion nueva', /WEBHB state=/);
    const ultimo = webhb.length ? webhb[webhb.length - 1] : '';
    if (/state=9 ingame=1/.test(ultimo)) estable++; else estable = 0;
    if (traceSince('rotado: sesion nueva', /FPHASE/).length >= 3 && estable >= 2) { pass = true; break; }
    if (errors.some((e) => /out of bounds|RuntimeError|Aborted/.test(e))) break;
    // Los clics del menú se pierden con facilidad (el canvas se re-escala
    // mientras carga): se reintenta el "yes" de vez en cuando.
    reintentos++;
    if (reintentos % 4 === 0) await slowClick(...at(FRAC.yes));
    await sleep(3000);
  }
  console.log('== en partida: ' + (pass ? 'sí (state=9 estable)' : 'NO'));
  if (pass) await sleep(3000);

  // Antes de medir hay que esperar a que la partida esté ASENTADA. Medido en
  // esta misma sonda: si se mide con la escena todavía cargando (o con otro
  // Chrome comiéndose la CPU) el frame se va en espera (`FPHASE wait`) y la
  // puerta del cargador de tráfico se queda cerrada a propósito (el streamer
  // está ocupado). En esa ventana la calle sale con 2 modelos: eso mide el
  // arranque, no el tráfico. Se espera a 5 frames seguidos sin espera.
  let asentado = false;
  const tAsentar = Date.now();
  while (pass && Date.now() - tAsentar < 150000) {
    const waits = traceSince('rotado: sesion nueva', /FPHASE/).slice(-5)
      .map((l) => Number((l.match(/wait=([\d.]+)/) || [])[1] || 9999));
    if (waits.length >= 5 && waits.every((w) => w < 25)) { asentado = true; break; }
    await sleep(3000);
  }
  console.log('== partida asentada (5 frames sin espera): ' + (asentado ? 'sí' : 'NO (se mide igual: el veredicto pedirá ticks del cargador)'));

  // ---- observación: quieto en la calle y a contar lo que se cuela ----------
  const marca = traceSize();
  const visto = {};
  const modelos = {};
  let lineasTxd = 0;
  let ticksCargador = 0;
  const trozos = [];
  const fin = Date.now() + MS_OBSERVACION;
  while (Date.now() < fin && pass) {
    await sleep(MS_MUESTREO);
    const txt = traceFrom(marca);
    if (!txt) continue;
    // Sólo las líneas nuevas desde la última muestra.
    if (txt.length > trozos.reduce((a, s) => a + s.length, 0)) {
      const ya = trozos.join('');
      const nuevo = txt.slice(ya.length);
      trozos.push(nuevo);
      for (const l of nuevo.split('\n')) {
        // Ticks del cargador de vehículos (`CARPED`, uno por modelo pedido): es
        // lo que hace que la calle tenga variedad. Sin ticks, la calle elige
        // sólo entre los modelos que ya estaban residentes.
        if (/CARPED/.test(l)) ticksCargador++;
        const m = l.match(/CARSPAWN model=(\d+)/);
        if (!m) continue;
        lineasTxd++;
        visto[m[1]] = (visto[m[1]] || 0) + 1;
        modelos[m[1]] = true;
      }
    }
    const restante = Math.round((fin - Date.now()) / 1000);
    if (restante % 60 < 20) console.log('== llevo ' + Math.round(MS_OBSERVACION / 1000) + 's-' +
      restante + 's | coches de calle vistos=' + lineasTxd + ' | modelos distintos=' + Object.keys(modelos).length);
  }

  const captura = path.join(SHOTS, 'calle.png');
  await page.screenshot({ path: captura, timeout: 20000 }).catch(() => {});

  // El TXD del vehículo que el jugador conduce también sale en la traza: se
  // avisa para no confundirlo con tráfico.
  console.log('\n== modelos del mod vistos en la calle (CARSPAWN):');
  let cuantos = 0;
  for (const [id, desc] of Object.entries(NUEVOS)) {
    const n = visto[id] || 0;
    if (n) cuantos++;
    console.log('   ' + (n ? 'SÍ' : 'no').padEnd(3) + ' ' + String(n).padStart(3) +
      '  ' + id + ' ' + desc);
  }
  const policia = visto[POLICIA] || 0;
  console.log('   ' + (policia ? '¡SÍ!' : 'no').padEnd(4) + String(policia).padStart(3) +
    '  ' + POLICIA + ' moto policial (clase `ignore`: no debe salir en tráfico)');
  console.log('\n== tráfico medido: ' + lineasTxd + ' coches de calle en ' + MINUTOS +
    ' min | ' + Object.keys(modelos).length + ' modelos distintos en total' +
    ' | ' + ticksCargador + ' ticks del cargador (CARPED)');
  const fp = traceSince('rotado: sesion nueva', /FPHASE/).slice(-30)
    .map((l) => Number((l.match(/wait=([\d.]+)/) || [])[1] || 0)).sort((a, b) => a - b);
  console.log('== frame durante la medida: wait mediana = ' + (fp.length ? fp[Math.floor(fp.length / 2)].toFixed(1) : '?') + ' ms (por debajo de 25 = sin carga dominante)');
  const top = Object.entries(visto).sort((a, b) => b[1] - a[1]).slice(0, 12)
    .map(([k, v]) => k + '=' + v).join(' ');
  console.log('== los que más salen (id=veces): ' + (top || '(ninguno)'));
  console.log('== captura: ' + captura);
  console.log('== errores de página: ' + (errors.length ? errors.slice(0, 3).join(' | ') : 'ninguno'));

  const traficoVivo = lineasTxd >= MIN_SPAWNS && Object.keys(modelos).length >= 3;
  // Un PASS exige que el cargador HAYA rotado modelos en la ventana: con 0 ticks
  // la calle sólo puede elegir entre los que ya estaban residentes, y un "hay
  // variedad" no diría nada de la rotación.
  const cargadorVivo = ticksCargador >= MIN_TICKS;
  ok = pass && !errors.length && traficoVivo && cargadorVivo && cuantos >= MIN_MODELOS && policia === 0;
  console.log('\n' + (ok ? 'PASS' : 'FAIL') + ' — tráfico de los vehículos del mod ' + (ok
    ? 'se ven ' + cuantos + '/' + Object.keys(NUEVOS).length + ' modelos del mod en ' + MINUTOS +
      ' min quieto en la calle, con ' + Object.keys(modelos).length + ' modelos distintos en total' +
      ', y la moto policial (clase `ignore`) NO aparece'
    : !pass ? 'sin llegar a partida'
      : errors.length ? 'con RuntimeError en la pestaña'
        : !traficoVivo ? 'sólo ' + lineasTxd + ' coches de calle en ' + MINUTOS + ' min (' +
          Object.keys(modelos).length + ' modelos distintos, mínimo pedido ' + MIN_SPAWNS +
          '): no se puede concluir nada de los modelos del mod'
          : !cargadorVivo ? 'el cargador de vehículos no rotó modelos en la ventana (' +
            ticksCargador + ' ticks, mínimo ' + MIN_TICKS + '): la calle sólo elige entre los ya residentes'
          : policia > 0 ? 'la moto policial (clase `ignore`) ha salido en tráfico (' + policia + ' veces)'
            : 'sólo ' + cuantos + ' de los 7 modelos del mod con clase de tráfico ' +
              '(mínimo pedido: ' + MIN_MODELOS + ')'));
  await browser.close().catch(() => {});
  process.exit(ok ? 0 : 1);
} catch (e) {
  console.log('FAIL: excepción del arnés: ' + String(e).slice(0, 400));
  if (browser) await browser.close().catch(() => {});
  process.exit(1);
}
