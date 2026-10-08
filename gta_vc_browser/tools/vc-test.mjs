#!/usr/bin/env node
// Lanzador de pruebas de reVC (Vice Extended).
//
//   node tools/vc-test.mjs --estatico        # lo rápido: sin navegador (~20 s)
//   node tools/vc-test.mjs                   # todo, en UNA sola sesión de juego
//   node tools/vc-test.mjs --escenario nado  # sólo uno (depurando un arreglo)
//   node tools/vc-test.mjs --listar          # qué hay
//
// POR QUÉ ASÍ
//   · `--estatico` no arranca el juego: comprueba el BINARIO servido (versión,
//     dataTag y las marcas de lo implementado), los define del build y, si hay
//     log, pasa el verificador de trazas. Es el bucle del día a día: segundos.
//   · La suite de escenarios abre el motor UNA vez y encadena los escenarios
//     dentro de la misma partida (antes: un arranque de ~2 min por fichero, y
//     cada uno con su propio perfil de Chrome, sin caché compartida).
//   · Cada escenario deja su informe con la evidencia y, si falla, su traza y
//     una captura en `--artefactos` (por defecto, tmp/vc-tests/<suite>-<fecha>).
//
// Variables: VC_URL, VC_SAVE, VC_ODTRACE, VC_PERFIL, CHROME, PUPPETEER_DIR,
//            VC_ARTEFACTOS, VC_CROUCH_S, VC_SWIM_S, VC_HUNT_BUDGET_S.

import fs from 'node:fs';
import path from 'node:path';
import { spawnSync } from 'node:child_process';
import {
  VC, TOOLS, ODTRACE, abrirSesion, imprimirInforme, motorServido, hayBash,
} from './harness/vc-harness.mjs';
import { ESQUEMAS } from './harness/esquemas.mjs';

const args = process.argv.slice(2);
const tiene = (f) => args.includes(f);
const valor = (f) => { const i = args.indexOf(f); return i >= 0 ? args[i + 1] : undefined; };
const soloUno = valor('--escenario') || valor('--solo');

function linea(t) { console.log('\n' + '─'.repeat(72) + '\n' + t); }
const seg = (ms) => (ms / 1000).toFixed(1) + ' s';

// --- suite estática (sin navegador) -----------------------------------------

function suiteEstatica() {
  const t0 = Date.now();
  let fallos = 0;
  const paso = (nombre, bien, detalle) => {
    console.log('   [' + (bien ? 'ok ' : 'MAL') + '] ' + nombre + (detalle ? ' — ' + detalle : ''));
    if (!bien) fallos++;
  };
  // AVISO = no se ha podido comprobar (o el dato es de otro build/sesión). Ni
  // suspende la suite ni se calla: se dice.
  const aviso = (nombre, detalle) => console.log('   [AVISO] ' + nombre + (detalle ? ' — ' + detalle : ''));

  linea('estático · el binario servido');
  let motor = { version: '?', dataTag: '?', faltan: [], ok: false, salida: '' };
  if (!hayBash) {
    paso('build servido', false, 'no hay bash en este entorno');
  } else {
    const m = motorServido();
    motor = m;
    paso('versión con marca en web/lib/index.js', /\d{4}-\d\d-\d\d-ve\d+/.test(m.version), m.version);
    paso('todas las marcas de lo implementado', m.ok,
      m.ok ? (m.salida.match(/OK: .*/) || ['todas'])[0].slice(0, 90) : ('faltan: ' + m.faltan.join(', ')));
    paso('dataTag del motor', !!m.dataTag && m.dataTag !== '?', m.dataTag);
  }

  // Los bloques de Vice Extended que nacen APAGADOS en `config.h` no los
  // compila el build normal: un error de sintaxis o de API dentro de ellos no lo
  // ve nadie hasta que alguien enciende el define. `check-define-build.sh`
  // reutiliza la línea de compilación real de ninja + el define y compila SÓLO
  // ese fichero (segundos). Se comprueban los que están apagados AHORA MISMO.
  linea('estático · los defines apagados compilan');
  const configH = fs.readFileSync(path.join(VC, '..', 'src', 'core', 'config.h'), 'utf8');
  const APAGADOS = [['VICEEXT_TURN_SIGNALS', 'src/vehicles/Automobile.cpp']];
  for (const [def, fuente] of APAGADOS) {
    const apagado = new RegExp('^\\s*//\\s*#define\\s+' + def + '\\b', 'm').test(configH);
    if (!apagado) { paso(def + ' compila con el define', true, 'ya está encendido en config.h'); continue; }
    const r = spawnSync('bash', [path.join(TOOLS, 'check-define-build.sh'), def, fuente],
      { cwd: VC, encoding: 'utf8', timeout: 180000 });
    const salida = ((r.stdout || '') + (r.stderr || '')).trim();
    paso(def + ' compila con el define (está apagado en config.h)', r.status === 0,
      r.status === 0 ? ('sin errores en ' + fuente) : salida.split('\n').filter((l) => /error/.test(l)).slice(-2).join(' '));
  }

  // Verificador sobre el ÚLTIMO log de trazas. Ojo: el log es de la ÚLTIMA
  // sesión, que puede ser la partida del jugador y/o de otro build. Se dice de
  // qué build es y cuándo se escribió, y si no es el build servido sólo se
  // avisa (no se suspende la suite por un log ajeno).
  linea('estático · verificador de trazas del último log');
  const log = valor('--log') || ODTRACE;
  if (fs.existsSync(log)) {
    const txt = fs.readFileSync(log, 'utf8');
    const buildLog = (txt.match(/JS build=(\S+)/) || [])[1] || '? (sin marca)';
    const ultima = txt.trim().split('\n').pop() || '';
    const fecha = (ultima.match(/^(\S+?Z)/) || [])[1];
    const edadMin = fecha ? Math.round((Date.now() - Date.parse(fecha)) / 60000) : null;
    console.log('   log: ' + log + '\n        build ' + buildLog
      + (edadMin != null ? ' · última línea hace ' + edadMin + ' min' : ''));
    const v = spawnSync('python', [path.join(TOOLS, 'viceext-log-check.py'), log],
      { cwd: VC, encoding: 'utf8', timeout: 180000 });
    const salidaV = (v.stdout || '') + (v.stderr || '');
    const resumen = salidaV.split('\n').filter((l) => /^\s{3}\S+\s+(OK|FALLO|SIN DATOS|PARCIAL)/.test(l));
    const malos = resumen.filter((l) => / FALLO\s/.test(l));
    // Un log de OTRA build es de otra sesión: lo que diga el verificador no
    // habla del motor servido. Se avisa (con los hallazgos, que siguen siendo
    // útiles) en vez de suspender la suite por la partida que esté jugando
    // alguien en ese momento. Con `--log=<ruta>` se puede pedir uno concreto.
    const esDelBuild = buildLog.indexOf(motor.version) >= 0;
    if (!esDelBuild) {
      aviso('el verificador no ve fallos en ese log',
        'log de ' + buildLog + ' (≠ ' + motor.version + '), de otra sesión: '
        + (malos.length ? malos.map((l) => l.trim()).slice(0, 3).join(' | ') : 'sin fallos'));
      aviso('el log es del build servido', buildLog + ' ≠ ' + motor.version
        + ' (para medir el servido: `node tools/vc-test.mjs --log=web/odtrace-<sesión>.log`)');
    } else {
      paso('el verificador no ve fallos en ese log', malos.length === 0,
        malos.length ? malos.map((l) => l.trim()).slice(0, 3).join(' | ') : (resumen.length + ' bloque(s) leídos'));
      paso('el log es del build servido', true, buildLog + ' = ' + motor.version);
    }
  } else {
    paso('verificador de trazas', true, 'no hay log todavía (nada que mirar)');
  }

  console.log('\n== estático: ' + (fallos ? 'FALLO (' + fallos + ')' : 'OK')
    + ' en ' + seg(Date.now() - t0));
  return fallos === 0;
}

// --- suite de escenarios (una sesión) ---------------------------------------

async function suiteEscenarios() {
  const nombres = soloUno ? soloUno.split(',') : ESQUEMAS.map((e) => e.nombre);
  const lista = nombres.map((n) => {
    const e = ESQUEMAS.find((x) => x.nombre === n);
    if (!e) throw new Error('no existe el escenario «' + n + '» (prueba --listar)');
    return e;
  });
  const nombreSuite = valor('--suite') || 'completa';
  const t0 = Date.now();
  linea('abriendo la sesión de juego (una vez para los ' + lista.length + ' escenario(s))');
  const marcas = [...new Set(lista.flatMap((e) => e.marcas || []))];
  const s = await abrirSesion({ nombre: nombreSuite, marcas });
  console.log('   motor ' + s.version + ' · traza ' + s.trazaFichero + ' · artefactos en ' + s.artefactos);
  console.log('   arranque: motor ' + seg(s.tiempos.motorMs) + ' · menú ' + seg(s.tiempos.menuMs)
    + ' · carga ' + seg(s.tiempos.partidaMs) + ' · asentar ' + seg(s.tiempos.asentarMs)
    + ' = ' + seg(s.tiempos.totalMs));

  const informes = [];
  for (const e of lista) {
    linea('escenario · ' + e.nombre + ' — ' + e.titulo);
    const tEsc = Date.now();
    let inf;
    try {
      inf = await e.ejecutar(s);
    } catch (err) {
      inf = { nombre: e.nombre, checks: [{ nombre: 'el escenario terminó', bien: false, evidencia: String(err).slice(0, 200) }], ok: false };
      await s.foto(e.nombre + '-excepcion');
    }
    inf.segundos = (Date.now() - tEsc) / 1000;
    informes.push(inf);
  }
  await s.cerrar();

  linea('resultado');
  let ok = true;
  for (const inf of informes) {
    const bien = imprimirInforme(inf.nombre, inf) && inf.segundos < 600;
    if (!bien) ok = false;
    console.log('        ' + inf.checks.length + ' comprobación(es) en ' + inf.segundos.toFixed(0) + ' s');
  }
  console.log('\n== suite «' + nombreSuite + '»: ' + (ok ? 'PASS' : 'FALLO')
    + ' (' + informes.filter((i) => i.ok).length + '/' + informes.length + ' escenarios) en '
    + seg(Date.now() - t0));
  if (s.errores.length) console.log('   errores de página: ' + s.errores.length + ' (el primero: ' + s.errores[0].slice(0, 160) + ')');
  console.log('   artefactos: ' + s.artefactos);
  return ok;
}

// --- main -------------------------------------------------------------------

if (tiene('--listar')) {
  console.log('escenarios:');
  for (const e of ESQUEMAS) console.log('   ' + e.nombre.padEnd(10) + ' ' + e.titulo);
  console.log('\nsuite estática: --estatico');
  process.exit(0);
}

try {
  let ok = true;
  if (tiene('--estatico')) ok = suiteEstatica() && ok;
  if (!tiene('--estatico') || tiene('--todo')) ok = (await suiteEscenarios()) && ok;
  process.exit(ok ? 0 : 1);
} catch (e) {
  console.log('\nFAIL: ' + (e && e.message ? e.message : e));
  process.exit(2);
}
