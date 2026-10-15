// Prueba de usuario del plan 6.0: una partida NUEVA en el navegador, sin partida
// sembrada. Teclado y ratón como los usa el jugador; la verdad sale de la traza
// (ODRUMBO / ODCROUCHDIR del motor servido), no de píxeles.
//
// Escenarios:
//   1. Andar W, S, A, D y dos diagonales (4 s cada una). Criterio del plan §8.2:
//      el rumbo real (desplazamiento) coincide con el pedido (vec) en ≤5° tras el
//      primer segundo de cada tramo.
//   2. Agachado andando (C, luego W 4 s). Criterio §7.2: ODCROUCHDIR no debe usar
//      src=2 (cuerpo + a) mientras se camina.
//   3. Apuntar (botón derecho 3 s con W). Criterio §8.1 / §7.2: el destino del
//      cuerpo (dest) no debe quedar a 180° de la cámara (camo) al entrar en apuntado.
//
// Uso (con el servidor de desarrollo en marcha en VC_URL, por defecto :2077):
//   node gta_vc_browser/tools/rumbo-apuntado-smoke-test.mjs
// Salida: 0 = PASS, 1 = FALLO, 2 = NO CONCLUYENTE (no se llegó a jugar o no hay trazas).

import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import {
  URL_BASE, WEB, PERFIL, CHROME, FRAC, Traza, campo, texto, conTiempo,
  cargarPuppeteer, motorServido, sleep,
} from './harness/vc-harness.mjs';

// La página escribe en la traza por defecto (el parámetro `trace=` no se enruta
// en este servidor); las marcas `traza.marca()` aíslan cada escenario.
const ODTRACE = path.join(WEB, 'odtrace.log');
const ART = path.join(os.tmpdir(), 'vc-rumbo-apuntado');
fs.mkdirSync(ART, { recursive: true });

const traza = new Traza(ODTRACE);
const avisos = [];
const checks = [];
const check = (n, bien, ev = '') => { checks.push({ n, bien: !!bien, ev }); console.log('   [' + (bien ? 'ok ' : 'MAL') + '] ' + n + (ev ? ' — ' + ev : '')); return !!bien; };
const aviso = (n, ev = '') => { avisos.push(n); console.log('   [AVISO] ' + n + (ev ? ' — ' + ev : '')); };
const envuelve = (a) => { while (a > 180) a -= 360; while (a <= -180) a += 360; return a; };
const hora = () => new Date().toLocaleTimeString();

const motor = motorServido();
console.log('== motor servido: VERSION ' + motor.version + ' dataTag ' + motor.dataTag
  + (motor.ok ? ' (marcas ok)' : ' — FALTAN: ' + motor.faltan.join(', ')));
if (!motor.ok) { console.log('RESULTADO: NO CONCLUYENTE (el motor servido no lleva las marcas)'); process.exit(2); }

const puppeteer = await cargarPuppeteer();
const browser = await puppeteer.launch({
  executablePath: CHROME, headless: 'new', protocolTimeout: 120000,
  args: ['--no-sandbox', '--disable-dev-shm-usage', '--use-angle=swiftshader',
    '--enable-unsafe-swiftshader', '--mute-audio', '--window-size=1280,800',
    '--user-data-dir=' + PERFIL],
});
let page;
const errores = [];
let estadoUlt = -1;
const estado = async () => {
  try {
    const log = await conTiempo(page.$eval('#gamelog', (e) => e.textContent), 6000, 'gamelog');
    const st = (log.match(/state (\d+)/g) || []).map((s) => Number(s.split(' ')[1]));
    if (st.length) estadoUlt = st[st.length - 1];
    return st;
  } catch (e) { return []; }
};

let enPartida = false;
let rect;
let centro = [0, 0];
try {
  page = await browser.newPage();
  await conTiempo(page.setViewport({ width: 1280, height: 800 }), 20000, 'viewport').catch(() => {});
  page.on('pageerror', (e) => errores.push('PAGEERROR ' + String(e).slice(0, 300)));
  page.on('console', (m) => {
    if (/memory access out of bounds|RuntimeError|Aborted/i.test(m.text())) errores.push(m.text().slice(0, 300));
  });

  const selloBuild = String(Math.floor(fs.statSync(path.join(WEB, 'public', 'build', 'reVC.wasm')).mtimeMs));
  const url = URL_BASE + '/?autostart=1&b=' + selloBuild;
  console.log('== abriendo ' + url);
  await conTiempo(page.goto(url, { waitUntil: 'domcontentloaded', timeout: 60000 }), 90000, 'goto');

  // Motor en el menú (state 7 tres veces seguidas en el heartbeat).
  const t0 = Date.now();
  let menu = false;
  while (Date.now() - t0 < 240000) {
    const st = await estado();
    if (st.slice(-3).length === 3 && st.slice(-3).every((s) => s === 7)) { menu = true; break; }
    await sleep(1500);
  }
  console.log('== menú: ' + (menu ? 'sí' : 'NO') + ' (' + ((Date.now() - t0) / 1000).toFixed(0) + ' s)');
  if (!menu) throw new Error('el motor no llegó al menú');
  await sleep(3000);

  rect = await conTiempo(page.evaluate(() => {
    const c = document.querySelector('.vc-canvas');
    if (!c) return null;
    const r = c.getBoundingClientRect();
    return [r.x, r.y, r.width, r.height];
  }), 15000, 'canvas');
  if (!rect) throw new Error('no encuentro el canvas (.vc-canvas)');
  const at = ([fx, fy]) => [rect[0] + fx * rect[2], rect[1] + fy * rect[3]];
  const clic = async ([x, y]) => {
    await page.mouse.move(x, y, { steps: 3 }).catch(() => {});
    await sleep(600);
    await page.mouse.down().catch(() => {});
    await sleep(320);
    await page.mouse.up().catch(() => {});
    await sleep(2500);
  };

  // Menú: INICIAR PARTIDA (partida nueva, sin cargar nada).
  console.log('== INICIAR PARTIDA (' + hora() + ')');
  await clic(at(FRAC.start));
  // Intro/cinemática: se salta con Enter/Escape hasta que el ped mande de verdad
  // (estado 9 Y cámara en modo 4 = seguir al ped; la cinemática va en modo 17).
  const tIntro = Date.now();
  while (Date.now() - tIntro < 240000) {
    const st = await estado();
    const ult = traza.ultima('PEDAT');
    if (st.length && st[st.length - 1] === 9 && ult && campo(ult, 'modo') === 4) { enPartida = true; break; }
    await page.keyboard.press('Enter').catch(() => {});
    await sleep(700);
    await page.keyboard.press('Escape').catch(() => {});
    await sleep(2000);
  }
  console.log('== en partida: ' + (enPartida ? 'sí' : 'NO') + ' (estado ' + estadoUlt + ')');
  if (!enPartida) throw new Error('no se llegó a jugar (estado ' + estadoUlt + ')');

  centro = [rect[0] + rect[2] / 2, rect[1] + rect[3] / 2];
  await page.mouse.move(centro[0], centro[1], { steps: 2 }).catch(() => {});
  await page.mouse.click(centro[0], centro[1]).catch(() => {});
  await sleep(6000);
} catch (e) {
  console.log('EXCEPCIÓN: ' + (e && e.message ? e.message : e));
}

const ventana = async (nombre, fn) => {
  const desde = traza.marca();
  await fn();
  await sleep(600);
  return { nombre, txt: traza.leer(desde) };
};
const hold = async (tecla, ms) => {
  await page.keyboard.down(tecla).catch(() => {});
  await sleep(ms);
  await page.keyboard.up(tecla).catch(() => {});
};

if (enPartida && !errores.length) {
  // ---- 1. Andar en 4 direcciones y 2 diagonales -------------------------------
  const tramos = [
    ['W', ['KeyW']], ['S', ['KeyS']], ['A', ['KeyA']], ['D', ['KeyD']],
    ['W+D', ['KeyW', 'KeyD']], ['W+A', ['KeyW', 'KeyA']],
  ];
  console.log('\n== ESCENARIO 1: andar (' + hora() + ')');
  const resumenRumbo = [];
  for (const [etiqueta, teclas] of tramos) {
    const w = await ventana(etiqueta, async () => {
      for (const k of teclas) await page.keyboard.down(k).catch(() => {});
      await sleep(4500);
      for (const k of teclas) await page.keyboard.up(k).catch(() => {});
    });
    await sleep(1500);
    // Muestras de andar: br=1 (vector del mod), con desplazamiento real medido.
    const ls = w.txt.split('\n').filter((l) => /ODRUMBO/.test(l));
    const andan = ls.filter((l) => texto(l, 'br') === '1' && Math.abs(campo(l, 'lr')) + Math.abs(campo(l, 'ud')) > 0);
    const conReal = andan.slice(1).filter((l) => campo(l, 'dist') > 0.2);
    const errs = conReal.map((l) => Math.abs(envuelve(campo(l, 'real') - campo(l, 'vec'))));
    const max = errs.length ? Math.max(...errs) : NaN;
    const med = errs.length ? [...errs].sort((a, b) => a - b)[Math.floor(errs.length / 2)] : NaN;
    resumenRumbo.push({ etiqueta, n: errs.length, max, med });
    check('andar ' + etiqueta + ': rumbo real vs pedido ≤5° tras 1 s',
      errs.length >= 2 && max <= 5,
      errs.length ? ('muestras ' + errs.length + ', error mediana ' + med.toFixed(1) + '°, máx ' + max.toFixed(1) + '°')
        : 'sin muestras de andar con desplazamiento (¿no se movió el ped?)');
  }
  fs.writeFileSync(path.join(ART, 'rumbo-andar.txt'), JSON.stringify(resumenRumbo, null, 1));

  // ---- 2. Agachado andando ---------------------------------------------------
  console.log('\n== ESCENARIO 2: agachado andando (' + hora() + ')');
  await page.keyboard.press('KeyC').catch(() => {});
  await sleep(2000);
  const wc = await ventana('agachado-andando', async () => {
    await hold('KeyW', 4500);
  });
  const cd = wc.txt.split('\n').filter((l) => /ODCROUCHDIR/.test(l));
  const src2 = cd.filter((l) => campo(l, 'src') === 2);
  if (!cd.length) {
    aviso('agachado andando: no salió ODCROUCHDIR (¿no estaba agachado?)');
  } else {
    check('agachado andando: no se usa cuerpo + a (src=2) al caminar',
      src2.length === 0, 'muestras ' + cd.length + ', con src=2: ' + src2.length);
  }
  await page.keyboard.press('KeyC').catch(() => {});  // de pie
  await sleep(2000);

  // ---- 3. Apuntar -------------------------------------------------------------
  console.log('\n== ESCENARIO 3: apuntar con W (' + hora() + ')');
  const wa = await ventana('apuntar', async () => {
    await page.mouse.move(centro[0], centro[1], { steps: 2 }).catch(() => {});
    await page.mouse.down({ button: 'right' }).catch(() => {});
    await page.keyboard.down('KeyW').catch(() => {});
    await sleep(3000);
    await page.keyboard.up('KeyW').catch(() => {});
    await page.mouse.up({ button: 'right' }).catch(() => {});
  });
  const ap = wa.txt.split('\n').filter((l) => /ODRUMBO/.test(l) && texto(l, 'br') === '0');
  if (!ap.length) {
    aviso('apuntar: no salieron muestras br=0 (¿sin arma? el botón derecho no apunta a puños)');
  } else {
    const giros = ap.map((l) => envuelve(campo(l, 'dest') - campo(l, 'camo')));
    const cerca180 = giros.filter((g) => Math.abs(Math.abs(g) - 180) < 30);
    check('apuntar: el cuerpo no queda a 180° de la cámara (criterio §7.2)',
      cerca180.length === 0,
      'muestras ' + ap.length + ', dest−camo = [' + giros.map((g) => g.toFixed(0)).join(', ') + ']°');
  }
  fs.writeFileSync(path.join(ART, 'traza-escenarios.txt'), traza.leer(0));
} else if (!enPartida) {
  // no conclusivo
} else {
  check('sin errores de página', false, errores[0]);
}

await browser.close().catch(() => {});

console.log('\n== resumen ==');
const malos = checks.filter((c) => !c.bien);
for (const c of checks) console.log('   ' + (c.bien ? 'OK   ' : 'FALLO') + ' ' + c.n + (c.ev ? '  [' + c.ev + ']' : ''));
for (const a of avisos) console.log('   AVISO ' + a);
console.log('== traza: ' + ODTRACE);
if (!enPartida || !checks.length) { console.log('RESULTADO: NO CONCLUYENTE'); process.exit(2); }
console.log(malos.length ? 'RESULTADO: FALLO (' + malos.length + ')' : 'RESULTADO: PASS');
process.exit(malos.length ? 1 : 0);
