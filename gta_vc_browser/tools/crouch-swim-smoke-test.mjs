// Sonda de agachado y nado (bloque R17 del plan de mecánicas, sección 3).
//
// Verifica EN VIVO, sin que juegue el jugador, lo que reportó en la 13ª/14ª
// partida: «la cámara se queda fija y el personaje no se desplaza al nadar y
// agacharse». Usa el motor servido (web/public/build) y las trazas nuevas:
//
//   - CROUCH2 (src/peds/PlayerPed.cpp, ViceExtCrouchAnim): `avance` = metros/s
//     RECORRIDOS por el ped desde la traza anterior (no lo que pide el mando),
//     `peso` = peso real del clip de agachado (si baja de ~0,5 otro clip se está
//     peleando por el cuerpo), `camdist` = metros entre cámara y ped, `modo` =
//     modo de cámara (4 = MODE_FOLLOWPED).
//   - VICEEXT crouch on/off/pose off: conmutador y retirada de clips (R15).
//   - SWIM2 (ViceExtSwimControl): `avance`, `modo`, `camdist` mientras se nada.
//   - CROUCHPOSE: pesos de los clips de agachado estando de pie (deben ser 0).
//
// Fases:
//   A) agachado andando (C, luego W 6 s): el ped TIENE que avanzar (~0,9 m/s,
//      el clip manda) y la cámara seguirlo (modo 4, camdist pequeña).
//   B) agachado quieto: avance ~0 sin perder la pose (peso alto, no se levanta).
//   C) desagacharse (C): los clips se retiran y el cuerpo vuelve a estar de pie.
//   D) agua: camina en 4 direcciones hasta que el ped entre en agua honda
//      (`VICEEXT swim enter`); entonces mide el nado (avance del clip, modo 4,
//      camdist pequeña). Si no encuentra agua, lo dice (no es fallo del arreglo).
//
// Uso:
//   node gta_vc_browser/tools/crouch-swim-smoke-test.mjs
// Requisitos: servidor de desarrollo en marcha (cd gta_vc_browser/web && npm run dev).
//
// Variables de entorno (opcionales):
//   VC_URL      default http://localhost:2077
//   VC_SAVE     save a sembrar (default tools/testdata/GTAVCsf1.b)
//   VC_ODTRACE  ruta de odtrace.log (default gta_vc_browser/web/odtrace.log)
//   VC_SHOTS    carpeta de capturas (default <tmp>/vc-crouch-swim)
//   VC_PHASE    'crouch' (A-C) | 'swim' (D) | 'todo' (default)
//   VC_CROUCH_S s andando agachado (default 6)
//   VC_HUNT_S   s por dirección buscando agua (default 25)
//   VC_SWIM_S   s midiendo el nado (default 12)

import fs from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const WEB = path.join(HERE, '..', 'web');
const URL_BASE = process.env.VC_URL || 'http://localhost:2077';
const ODTRACE = process.env.VC_ODTRACE || path.join(WEB, 'odtrace.log');
const PROFILE = process.env.VC_PROFILE || path.join(os.tmpdir(), 'vc-crouch-swim-profile');
const CHROME = process.env.CHROME || 'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe';
const SAVE = process.env.VC_SAVE || path.join(HERE, 'testdata', 'GTAVCsf1.b');
const SHOTS = process.env.VC_SHOTS || path.join(os.tmpdir(), 'vc-crouch-swim');
const PHASE = process.env.VC_PHASE || 'todo';
const CROUCH_S = Number(process.env.VC_CROUCH_S || 6);
const HUNT_S = Number(process.env.VC_HUNT_S || 25);
const SWIM_S = Number(process.env.VC_SWIM_S || 12);

const MODE_FOLLOWPED = 4;
const BOOT_TIMEOUT_MS = 180000;
const LOAD_TIMEOUT_MS = 240000;

// Filas del listado de CARGAR PARTIDA medidas en la captura del menú servido:
// la primera fila es «AUTOGUARDADO» (ranura 9) y la partida del jugador
// (`GTAVCsf1.b`, «La fiesta») es la SEGUNDA fila, y≈228 px. Con la fila del
// autoguardado delante, el y≈180 que usaban las sondas viejas cae en el hueco y
// el clic no elige nada: la sonda se quedaba en el menú para siempre.
const FRAC = {
  start: [0.5, 325 / 800],
  load: [0.5, 345 / 800],
  slot1: [0.3125, 228 / 800],   // «La fiesta» = GTAVCsf1.b (2ª fila)
  yes: [0.5, 418 / 800],
};

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
// Líneas del tramo con la etiqueta pedida (CROUCH2, SWIM2, ...), sin el prefijo
// de hora: se parsean los campos `clave=valor` que ya trae cada traza.
function lines(txt, tag) {
  return txt.split('\n').map((l) => l.trim())
    .filter((l) => new RegExp('(^|\\s)' + tag + '\\s').test(l))
    .map((l) => l.replace(/^\S+Z\s+/, ''));
}
// El nombre se ancla a principio de campo (principio de línea o espacio): sin
// esto, `dist` encajaba dentro de `camdist` y la sonda de agua leía 4 m de mar
// en vez de la distancia real (le pasó al estrenar `PEDAT`, 22/09).
function field(line, name) {
  const m = line.match(new RegExp('(^|\\s)' + name + '=(-?[\\d.]+)'));
  return m ? Number(m[2]) : NaN;
}
function has(txt, needle) { return txt.indexOf(needle) >= 0; }

if (!fs.existsSync(SAVE)) {
  console.log('FAIL: no encuentro la partida a sembrar (' + SAVE + '). Pasa VC_SAVE=<ruta>.');
  process.exit(1);
}
if (!fs.existsSync(ODTRACE)) {
  console.log('FAIL: no existe la traza ' + ODTRACE + ' (¿servidor de desarrollo en marcha?)');
  process.exit(1);
}

const puppeteer = await loadPuppeteer();
const errors = [];
const checks = [];
const check = (nombre, bien, detalle) => {
  checks.push({ nombre, bien: !!bien, detalle: detalle || '' });
  console.log('   [' + (bien ? 'ok ' : 'MAL') + '] ' + nombre + (detalle ? ' — ' + detalle : ''));
};
let browser;
let enPartida = false;

const sleepWall = sleep;
// Velocidad de MUNDO preferida: `velo` (metros / tiempo de simulación) con
// `avance` (metros / segundo de reloj) como respaldo en motores sin R18.
const vel = (l) => {
  const v = field(l, 'velo');
  return Number.isNaN(v) ? field(l, 'avance') : v;
};
const resumen = (arr, campo) => {
  const v = arr.map((l) => (typeof campo === 'function' ? campo(l) : field(l, campo)))
    .filter((x) => !Number.isNaN(x));
  if (!v.length) return null;
  const s = [...v].sort((a, b) => a - b);
  return {
    n: v.length, min: s[0], max: s[s.length - 1],
    med: s[Math.floor(s.length / 2)],
    // Valores "de régimen": los del medio, para no mezclar con el arranque de
    // la fase (los primeros ~2 s son aceleración, no velocidad de crucero).
    cuerpo: s.slice(Math.min(2, Math.max(0, s.length - 1))),
  };
};
const txt = (v) => (v == null ? '—' : v.toFixed(2));
// Todas las muestras tal cual, en un fichero, para poder mirarlas después.
const volcar = (name, t) => {
  try { fs.writeFileSync(path.join(SHOTS, name + '.txt'), t); } catch (e) {}
};
const modos = (arr) => [...new Set(arr.map((l) => field(l, 'modo')).filter((x) => !Number.isNaN(x)))].sort((a, b) => a - b);

try {
  fs.mkdirSync(SHOTS, { recursive: true });
  browser = await puppeteer.launch({
    executablePath: CHROME,
    headless: 'new',
    protocolTimeout: 90000,
    args: ['--no-sandbox', '--disable-dev-shm-usage', '--use-angle=swiftshader',
      '--enable-unsafe-swiftshader', '--mute-audio', '--window-size=1280,800',
      '--user-data-dir=' + PROFILE],
  });
  const page = await browser.newPage();
  await page.setViewport({ width: 1280, height: 800 });
  await page.setCacheEnabled(false).catch(() => {});
  page.on('console', (m) => {
    const t = m.text();
    if (/memory access out of bounds|RuntimeError|Aborted|out of bounds/i.test(t)) {
      errors.push(t.slice(0, 600));
      console.log('!! ' + new Date().toLocaleTimeString() + ' ' + t.slice(0, 400));
    }
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
  for (let i = 0; i < 120; i++) { seed = await write(); if (String(seed).startsWith('ok')) break; await sleep(1000); }
  console.log('== save sembrado: ' + seed);

  const t0 = Date.now();
  let menu = false;
  while (Date.now() - t0 < BOOT_TIMEOUT_MS) {
    const log = await page.$eval('#gamelog', (e) => e.textContent).catch(() => '');
    const st = (log.match(/state \d+/g) || []).slice(-3);
    if (st.length === 3 && st.every((s) => s === 'state 7')) { menu = true; break; }
    await sleep(2000);
  }
  console.log('== menú listo: ' + (menu ? 'sí' : 'NO'));
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
    await page.screenshot({ path: path.join(SHOTS, name), timeout: 30000 }).catch(() => {});
  };
  const pulsa = async (key, ms) => {
    await page.keyboard.down(key).catch(() => {});
    await sleep(ms || 140);
    await page.keyboard.up(key).catch(() => {});
  };

  // El menú de VC tiene pantallas previas (intro/legal) que se comen los
  // primeros clics, sobre todo con un perfil recién creado: se repite la
  // secuencia entera hasta que el motor pida la carga.
  let requested = false;
  for (let intento = 0; intento < 8 && !requested; intento++) {
    console.log('== menú, intento ' + (intento + 1) + ' (' + new Date().toLocaleTimeString() + ')');
    await slowClick(...at(FRAC.start));
    await slowClick(...at(FRAC.load));
    await slowClick(...at(FRAC.slot1));
    await slowClick(...at(FRAC.yes));
    for (let i = 0; i < 4 && !requested; i++) {
      await sleep(2500);
      requested = has(traceFrom(Math.max(0, traceSize() - 200000)), 'WR load-req');
      const log = await page.$eval('#gamelog', (e) => e.textContent).catch(() => '');
      const st = (log.match(/state (\d+)/g) || []).slice(-2);
      if (/state (8|9)/.test(st.join(' '))) requested = true;
    }
    if (!requested) {
      await shot('01-menu-intento-' + (intento + 1) + '.png');
      const log = await page.$eval('#gamelog', (e) => e.textContent).catch(() => '');
      console.log('   (estado: ' + ((log.match(/state \d+/g) || []).slice(-3).join(' ')) + ')');
    }
  }

  const tLoad = Date.now();
  let ultimoEstado = -1;
  while (Date.now() - tLoad < LOAD_TIMEOUT_MS) {
    const log = await page.$eval('#gamelog', (e) => e.textContent).catch(() => '');
    const st = (log.match(/state (\d+)/g) || []).map((s) => Number(s.split(' ')[1]));
    if (st.length) ultimoEstado = st[st.length - 1];
    if (ultimoEstado === 9) { enPartida = true; break; }
    if (errors.length) break;
    await sleep(3000);
  }
  console.log('== en partida: ' + (enPartida ? 'sí' : 'NO') + ' (último estado: ' + ultimoEstado + ')');

  const hoy = () => new Date().toLocaleTimeString();
  // Marca de versión del motor servido: sin esto, la sonda podría estar
  // midiendo un binario viejo (el fallo que se coló en la 13ª partida).
  const marca = traceFrom(Math.max(0, traceSize() - 400000));
  const vm = marca.match(/JS build=(\S+)/);
  console.log('== motor servido en esta sesión: ' + (vm ? vm[1] : '(sin marca en la traza)'));

  if (enPartida) {
    await page.mouse.move(centro[0], centro[1], { steps: 2 }).catch(() => {});
    await page.mouse.click(centro[0], centro[1]).catch(() => {});
    await sleep(1500);

    console.log('\n== asentando la partida (12 s)');
    for (let i = 0; i < 3 && !errors.length; i++) await sleep(4000);
    await shot('00-inicio.png');

    const segs = [];
    const abre = (nombre) => { const s = { nombre, desde: traceSize() }; segs.push(s); return s; };
    const cierra = (s) => { s.hasta = traceSize(); return traceFrom(s.desde).slice(0, Math.max(0, s.hasta - s.desde)); };

    if (PHASE === 'crouch' || PHASE === 'todo') {
      // --- Fase A/B/C: agachado ----------------------------------------------
      console.log('\n== FASE A (' + hoy() + '): agacharse (C) y ANDAR agachado ' + CROUCH_S + ' s');
      const sCrouchOn = abre('A0-crouch-on');
      await pulsa('KeyC', 140);
      await sleep(2000);
      await shot('10-agachado-quieto.png');
      const txtOn = cierra(sCrouchOn);
      const sA = abre('A-crouch-walk');
      await page.keyboard.down('KeyW').catch(() => {});
      await sleep(CROUCH_S * 1000);
      await shot('11-agachado-andando.png');
      await page.keyboard.up('KeyW').catch(() => {});
      const txtA = cierra(sA);

      console.log('== FASE B (' + hoy() + '): agachado quieto 4 s');
      await sleep(4000);
      const sB = abre('B-crouch-idle');
      await sleep(3000);
      const txtB = cierra(sB);
      await shot('12-agachado-parado.png');

      console.log('== FASE C (' + hoy() + '): desagacharse (C) y comprobar la pose');
      const sC = abre('C-stand');
      await pulsa('KeyC', 140);
      await sleep(3500);
      const txtC = cierra(sC);
      await shot('13-de-pie.png');

      volcar('traza-A-agachado-andando', txtA);
      volcar('traza-B-agachado-quieto', txtB);
      volcar('traza-C-de-pie', txtC);
      volcar('traza-A0-tecla-C', txtOn);
      const crouchA = lines(txtA, 'CROUCH2');
      const crouchB = lines(txtB, 'CROUCH2');
      const poseC = lines(txtC, 'CROUCHPOSE');
      const offC = lines(txtC, 'VICEEXT');

      console.log('   trazas A: ' + crouchA.length + ' CROUCH2, B: ' + crouchB.length
        + ', C: ' + poseC.length + ' CROUCHPOSE');
      for (const l of crouchA) console.log('      A ' + l);

      const avAall = resumen(crouchA, vel);
      const cuerpoA = avAall ? avAall.cuerpo : null;
      const avAcuerpo = cuerpoA && cuerpoA.length ? resumen(cuerpoA.map((x) => 'velo=' + x), 'velo') : null;
      const avA = avAall;
      const pesoA = resumen(crouchA, 'peso');
      const camA = resumen(crouchA, 'camdist');
      const modosA = modos(crouchA);
      const avB = resumen(crouchB, 'avance');
      const pesoB = resumen(crouchB, 'peso');

      check('agachado: el conmutador entró (C on)',
        has(txtOn, 'VICEEXT crouch on'), offC.concat(lines(txtOn, 'VICEEXT')).join(' | ') || 'SIN traza');
      check('agachado ANDANDO el ped se desplaza de verdad',
        !!avA && avA.med >= 0.4, avA ? ('velo mediana=' + txt(avA.med) + ' m/s, máx=' + txt(avA.max)
          + ' en ' + avA.n + ' muestra(s)') : 'sin trazas de velocidad');
      check('agachado ANDANDO la velocidad es la del clip escalada (≈0,9 m/s)',
        !!avAcuerpo && avAcuerpo.med >= 0.5 && avAcuerpo.med <= 1.3,
        avAcuerpo ? ('velo de régimen mediana=' + txt(avAcuerpo.med) + ' m/s (muestras '
          + avAcuerpo.min.toFixed(2) + '..' + avAcuerpo.max.toFixed(2) + ')') : '—');
      if (avA && avA.max > 2.0)
        console.log('   (aviso) hay una muestra de arranque con avance=' + txt(avA.max)
          + ' m/s: se mira en traza-A-agachado-andando.txt');
      // R19: la velocidad sale de `avance` (posición) partido por el tiempo de
      // MUNDO acumulado. Si ese tiempo se contara dos veces (la función del
      // agachado se llama dos veces por frame: medido el 22/09, dt=2,04), la
      // velocidad saldría a la mitad (0,44 de un andar de 0,9) y la sonda daría
      // un FALSO fallo. Esta marca vigila justo eso: `dt` debe ser ~1 s de mundo
      // por segundo de reloj del motor.
      const dtA = resumen(crouchA.concat(crouchB), 'dt');
      check('agachado: la medida de velocidad es fiable (mundo ≈ reloj, sin doble conteo)',
        !!dtA && dtA.med >= 0.7 && dtA.med <= 1.3,
        dtA ? ('dt mediana=' + dtA.med.toFixed(2) + ' s de mundo por segundo de reloj') : 'sin dt');
      check('agachado: el clip del mod manda en el cuerpo (peso ≈ 1)',
        !!pesoA && pesoA.max >= 0.5, pesoA ? ('peso máx=' + pesoA.max.toFixed(2)) : 'sin peso');
      check('agachado: la cámara SIGUE al ped (modo 4)',
        modosA.length > 0 && modosA.every((m) => m === MODE_FOLLOWPED),
        'modos vistos=[' + modosA.join(',') + ']');
      check('agachado: la cámara no se queda lejos del ped',
        !!camA && camA.max <= 12.0,
        camA ? ('camdist ' + camA.min.toFixed(2) + '..' + camA.max.toFixed(2) + ' m') : 'sin camdist');
      check('agachado QUIETO no se arrastra (avance ≈ 0)',
        !avB || avB.max <= 0.25, avB ? ('avance máx=' + avB.max.toFixed(2) + ' m/s') : 'sin trazas');
      check('agachado QUIETO mantiene la pose (peso ≈ 1)',
        !!pesoB && pesoB.max >= 0.5, pesoB ? ('peso máx=' + pesoB.max.toFixed(2)) : 'sin peso');
      check('al desagacharse se retiran los clips',
        has(txtC, 'crouch pose off'), offC.filter((l) => /crouch pose off/.test(l)).join(' | ') || 'SIN traza');
      const poseOk = poseC.filter((l) => field(l, 'desde') >= 700);
      check('de pie: los clips de agachado pesan 0 (cuerpo erguido)',
        poseC.length === 0 || poseOk.every((l) => field(l, 'idle') <= 0.01 && field(l, 'fwd') <= 0.01 && field(l, 'back') <= 0.01),
        poseC.length === 0 ? 'sin trazas CROUCHPOSE (nada que retirar)' : (poseOk.map((l) => l).slice(0, 4).join(' | ') || 'todas las líneas son <700 ms'));
    }

    if (PHASE === 'swim' || PHASE === 'todo') {
      // --- Fase D: buscar agua y nadar ---------------------------------------
      console.log('\n== FASE D (' + hoy() + '): navegar hasta el agua (presupuesto '
        + HUNT_S + ' s de referencia) y nadar');
      // De pie y sin tocar nada (por si venimos de la fase de agachado).
      await pulsa('KeyC', 140);   // si ya estaba de pie, esto lo agacha...
      await sleep(600);
      await pulsa('KeyC', 140);   // ...y esto lo vuelve a levantar
      await sleep(1500);

      const sD = abre('D-swim');

      // R19: la sonda NAVEGA con la posición real (traza `PEDAT`: x/y/z, la
      // velocidad y `dist` = distancia al agua que mide el motor) en vez de
      // andar a ciegas 4×30 s: el 22/09 se quedó contra una casa y la parte de
      // nado quedó sin medir. Cada prueba de rumbo ANDA unos segundos y
      // comprueba si `dist` (metros hasta el mar) ha bajado: si baja, sigue;
      // si no baja o el ped está clavado (pared), gira la vista ~45°.
      const pedat = () => {
        const ls = lines(traceFrom(sD.desde), 'PEDAT');
        return ls.length ? ls[ls.length - 1] : null;
      };
      const anda = async (s) => {
        await page.keyboard.down('KeyW').catch(() => {});
        const t = Date.now();
        while (Date.now() - t < s * 1000) {
          await sleep(1000);
          if (/VICEEXT swim enter/.test(traceFrom(sD.desde))) break;
          if (errors.length) break;
        }
        await page.keyboard.up('KeyW').catch(() => {});
        await sleep(500);
      };
      const gira = async (px) => {
        // En 3ª persona el ratón gira al ped (no la cámara suelta).
        await page.mouse.move(centro[0], centro[1], { steps: 3 }).catch(() => {});
        await page.mouse.move(centro[0] + px, centro[1], { steps: 12 }).catch(() => {});
        await sleep(900);
      };

      const GAIT_S = 5;   // segundos andando por prueba de rumbo
      const BUDGET_S = Number(process.env.VC_HUNT_BUDGET_S || 300);
      let nadando = false, rumbos = 0, giros = 0, ciegos = 0;
      const tHunt = Date.now();
      while (!nadando && (Date.now() - tHunt) / 1000 < BUDGET_S && !errors.length) {
        const a = pedat();
        await anda(GAIT_S);
        rumbos++;
        if (/VICEEXT swim enter/.test(traceFrom(sD.desde))) { nadando = true; break; }
        const b = pedat();
        let av = 0, d0 = -1, d1 = -1;
        if (a && b) {
          av = Math.hypot(field(b, 'x') - field(a, 'x'), field(b, 'y') - field(a, 'y'));
          d0 = field(a, 'dist');
          d1 = field(b, 'dist');
        }
        // `dist` = metros hasta el agua medidos por el motor (-1 = no se ve
        // agua a menos de 48 m). Atascado (no avanza) = pared: girar. Con agua a
        // la vista, se sigue sólo si la distancia BAJA (voy hacia la orilla).
        const atascado = av < 1.0;
        const ve = d0 >= 0 && d1 >= 0;
        const acerca = ve && (d1 <= d0 - 1.0);
        let accion;
        if (atascado) accion = 'atascado (pared): giro ~45°';
        else if (!ve) {
          ciegos++;
          accion = (ciegos % 6 === 0) ? 'sin agua a la vista: giro ~45°' : 'sin agua a la vista: sigo recto';
          if (ciegos % 6 !== 0) { console.log('   [' + hoy() + '] rumbo ' + rumbos + ': avance '
            + av.toFixed(1) + ' m, mar fuera de alcance (' + accion + ')'); continue; }
        } else if (acerca) {
          ciegos = 0;
          accion = 'mar a ' + d1.toFixed(0) + ' m: sigo';
        } else {
          ciegos = 0;
          accion = 'mar a ' + d1.toFixed(0) + ' m (no me acerco): giro ~45°';
        }
        console.log('   [' + hoy() + '] rumbo ' + rumbos + ': avance ' + av.toFixed(1)
          + ' m, mar ' + (ve ? (d0.toFixed(0) + '→' + d1.toFixed(0) + ' m') : 'fuera de alcance')
          + ' (' + accion + ')');
        await gira(130);
        giros++;
        await shot('20-rumbo-' + giros + '.png');
      }

      if (nadando) {
        console.log('   [' + hoy() + '] ¡agua! nadando ' + SWIM_S + ' s más');
        await shot('21-nadando.png');
        await page.keyboard.down('KeyW').catch(() => {});
        await sleep(SWIM_S * 1000);
        await shot('22-nadando-b.png');
        // Segunda medida: girar 180° y nadar hacia el otro lado. Si el ped
        // entró por la orilla, la primera mitad se mide nadando pegado al
        // fondo en pendiente (fricción) y ésta en aguas abiertas.
        console.log('   [' + hoy() + '] giro 180° y otra medida de ' + SWIM_S + ' s');
        await page.mouse.move(centro[0], centro[1], { steps: 3 }).catch(() => {});
        await page.mouse.move(centro[0] + 560, centro[1], { steps: 22 }).catch(() => {});
        await sleep(1500);
        await shot('22b-girado.png');
        sD.giro = traceSize();
        await sleep(SWIM_S * 1000);
        await shot('22c-nadando-al-reves.png');
        await page.keyboard.up('KeyW').catch(() => {});
      }
      const txtD = cierra(sD);
      await shot('23-final.png');

      const swim = lines(txtD, 'SWIM2').filter((l) => /move/.test(l));
      const swimMov = swim.filter((l) => /camdist=|modo=/.test(l));
      const avD = resumen(swim.filter((l) => vel(l) > 0.05), vel);
      const pesoD = null;
      const camD = resumen(swimMov, 'camdist');
      const modosD = modos(swimMov);

      // Muestras de la segunda mitad (después del giro), si la hubo.
      let txtD2 = '';
      if (sD.giro && sD.hasta > sD.giro) txtD2 = traceFrom(sD.giro).slice(0, sD.hasta - sD.giro);
      const swim2 = lines(txtD2, 'SWIM2').filter((l) => /move/.test(l));
      const avD2 = resumen(swim2.filter((l) => vel(l) > 0.05), vel);
      const pedAll = lines(txtD, 'PEDAT');
      const dbg = lines(txtD, 'SWIMDBG');
      console.log('   trazas de nado: ' + swim.length + ' SWIM2 (camdist/modo: ' + swimMov.length
        + '), diagnóstico: ' + dbg.length);
      for (let i = 0; i < Math.max(swim.length, dbg.length) && i < 8; i++) {
        if (swim[i]) console.log('      D ' + swim[i]);
        if (dbg[i]) console.log('      G ' + dbg[i]);
      }

      // R19: la misma comprobación de reloj en la navegación (traza `PEDAT`:
      // `ts` = segundos de mundo acumulados por frame, `ms` = reloj del motor).
      const relojD = resumen(pedAll.filter((l) => field(l, 'ms') > 500)
        .map((l) => 'r=' + (field(l, 'ts') / (field(l, 'ms') / 1000))), 'r');
      check('el reloj de mundo va a tiempo real (ts ≈ ms: la velocidad medida es m/s de verdad)',
        !!relojD && relojD.med >= 0.85 && relojD.med <= 1.15,
        relojD ? ('mundo/reloj mediana=' + relojD.med.toFixed(2) + ' en ' + relojD.n + ' muestra(s) PEDAT') : 'sin muestras PEDAT');

      if (!nadando) {
        check('nado: la sonda encontró agua', false,
          'no se llegó a agua honda en ' + rumbos + ' rumbo(s) de ' + GAIT_S
            + ' s (presupuesto ' + BUDGET_S + ' s, ' + giros + ' giro(s)): la parte de nado queda SIN MEDIR');
      } else {
        check('nado: entrar al agua se detecta', has(txtD, 'VICEEXT swim enter'), 'traza swim enter');
        const dzD = resumen(swim.slice(2), 'dz');
        const honD = resumen(swim.slice(2), 'hondo');
        // El origen del ped está en los PIES: con `hondo` ≤ 1,0 m la cabeza queda
        // fuera del agua (el ped mide ~1,8 m) y el ped nadie sumergido. `dz` es lo
        // que le falta para el punto de flote ideal (nivel − 0,55).
        check('nado: el ped flota cerca de la superficie (no se va al fondo)',
          !!dzD && dzD.med <= 0.45 && !!honD && honD.med <= 1.00,
          (dzD ? 'dz mediana=' + dzD.med.toFixed(2) + ' m' : 'sin dz') + ' · '
            + (honD ? 'hondo mediana=' + honD.med.toFixed(2) + ' m' : 'sin hondo'));
        check('nado: el ped avanza (velocidad del clip)',
          !!avD && avD.max >= 1.5, avD ? ('velo mediana=' + txt(avD.med) + ' m/s, máx=' + txt(avD.max)
            + ' m/s (clip de braza = 2,32; crol = 2,78)') : 'sin velocidad medida');
        if (avD2)
          check('nado: aguas abiertas (tras girar 180°) avanza a la velocidad del clip',
            avD2.med >= 1.5, 'velo mediana=' + txt(avD2.med) + ' m/s, máx=' + txt(avD2.max)
              + ' m/s en ' + avD2.n + ' muestra(s)');
        check('nado: la cámara SIGUE al ped (modo 4)',
          modosD.length > 0 && modosD.every((m) => m === MODE_FOLLOWPED),
          'modos vistos=[' + modosD.join(',') + ']');
        check('nado: la cámara no se queda lejos del ped',
          !!camD && camD.max <= 12.0,
          camD ? ('camdist ' + camD.min.toFixed(2) + '..' + camD.max.toFixed(2) + ' m') : 'sin camdist');
        check('nado: sin cámara de ahogado',
          has(txtD, 'SWIMCAM no-fallen-water') || !has(txtD, 'modo=23'),
          'línea SWIMCAM no-fallen-water: ' + (has(txtD, 'SWIMCAM no-fallen-water') ? 'sí' : 'no'));
      }
    }
  }
} catch (e) {
  console.log('EXCEPCIÓN: ' + (e && e.stack ? e.stack : e));
  errors.push(String(e));
} finally {
  if (browser) await browser.close().catch(() => {});
}

console.log('\n== resumen ==');
let fallos = 0, sinMedir = 0;
for (const c of checks) {
  if (!c.bien) fallos++;
  console.log('   ' + (c.bien ? 'OK   ' : 'FALLO') + ' ' + c.nombre + (c.detalle ? '  [' + c.detalle + ']' : ''));
}
if (!checks.length) sinMedir++;
console.log('== errores de página: ' + errors.length + (errors.length ? ' -> ' + errors[0].slice(0, 200) : ''));
console.log('== capturas en ' + SHOTS);

if (!enPartida) { console.log('RESULTADO: NO CONCLUYENTE (no se llegó a partida)'); process.exit(2); }
if (sinMedir) { console.log('RESULTADO: NO CONCLUYENTE'); process.exit(2); }
console.log(fallos ? 'RESULTADO: FALLO (' + fallos + ')' : 'RESULTADO: PASS');
process.exit(fallos ? 1 : 0);
