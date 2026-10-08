// Verificador de logs del drive-by ampliado (sección 2, bloque P2).
//
// LEE SOLO FICHEROS: no abre navegador, no lanza Chrome, no compila y no toca
// la partida. Sirve para confirmar, con los logs de una partida de verdad (tuya),
// que VICEEXT_DRIVEBY_WIDE se comporta como dice el contrato del bloque:
//
//   - con pistola (slot 3) y el gatillo apretado en coche/moto/barco -> hay
//     líneas `DRIVEBY shot ... slot=3 anim=left|right|left-lo|right-lo|forward|lhs|rhs`;
//   - al entrar en el vehículo, la pistola se queda en la mano
//     (`DRIVEBY enter ... slot=3 ... outcome=keep-weapon`) en vez de esconderse
//     (`outcome=remove-model`, que es lo que hacía vanilla y lo que hace este
//     mismo código con el define apagado);
//   - la SMG (slot 5) sigue igual: `outcome=switch-smg` al entrar y disparos con
//     `slot=5` (no regresión).
//   - P4 (cadencia por arma): cada línea `DRIVEBY shot` lleva `delay=<ms>`. Con pistola
//     debe ser el de su `weapon.dat` (Colt45/Beretta 210, Python 600), NO los 70 ms
//     fijos del SMG. `delay=70` con slot 3 = build sin el arreglo de P4.
//
// Y avisa de las DOS señales de que el motor sigue siendo vanilla (las dos
// aparecen con la precedencia antigua, la que cambiaba de arma al entrar):
//
//   - `DRIVEBY exit ... slot=3 ... outcome=restore-stored`: al salir te devolvió
//     la pistola que había GUARDADO al entrar -> entraste con la pistola en la
//     mano y el motor la cambió por la SMG;
//   - `DRIVEBY enter ... slot=3 ... outcome=remove-model`: al entrar escondió la
//     pistola en vez de dejarla en la mano.
//
// El veredicto imprime además el `JS build=` de la sesión: es lo que dice qué
// binario estaba corriendo (una partida vieja no puede probar un arreglo nuevo).
//
// Cómo generar los logs (no hace falta nada especial):
//   1. juega normal, teclea el cheat `CRAZYPISTOL` (solo la pistola) y dispara
//      desde un coche mirando de lado (Q/E con el gatillo: `Insert`/`CTRL izq`)
//      y desde la moto (ahí dispara solo con el gatillo, hacia delante);
//   2. `node gta_vc_browser/tools/driveby-log-check.mjs`
//
// Uso:
//   node gta_vc_browser/tools/driveby-log-check.mjs            (todas las sesiones)
//   node gta_vc_browser/tools/driveby-log-check.mjs --tag=0    (solo las de partida libre)
//   node gta_vc_browser/tools/driveby-log-check.mjs --file=<ruta a odtrace.log>
//   (o VC_ODTRACE=<ruta>)
//
// Códigos de salida: 0 = la mecánica se ve funcionar; 1 = los logs contradicen
// el contrato; 2 = no hay datos de drive-by todavía (nada que juzgar).

import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const args = process.argv.slice(2);
const argOf = (name) => {
  const a = args.find((x) => x.startsWith('--' + name + '='));
  return a ? a.slice(name.length + 3) : null;
};
const FILE = argOf('file') || process.env.VC_ODTRACE || path.join(HERE, '..', 'web', 'odtrace.log');
const TAG_FILTER = argOf('tag') !== null ? Number(argOf('tag')) : null;
const GAP_S = 180;      // hueco que separa una sesión de la siguiente

const WEP = { 48: 'Beretta', 51: 'Uziold', 52: 'Ak47' };
const SLOT = { 0: 'puños', 1: 'melee', 2: 'proyectil', 3: 'pistola', 4: 'escopeta', 5: 'SMG', 6: 'rifle', 7: 'pesada', 8: 'sniper' };
const wep = (n) => WEP[n] || ('arma ' + n);
const slot = (n) => (SLOT[n] || ('slot ' + n));

const RE_STATE = /^DRIVEBY state tag=(\d+) t=(\d+) veh=(\w+) wep=(-?\d+) slot=(-?\d+) ammo=(-?\d+) fire=(\d) lookL=(\d) lookR=(\d) model=(-?\d+) speed=(-?[\d.]+)$/;
const RE_SHOT = /^DRIVEBY shot tag=(\d+) t=(\d+) veh=(\w+) wep=(-?\d+) slot=(-?\d+) anim=([\w-]+)(?: delay=(\d+))? ammo=(-?\d+)$/;
const RE_VEH = /^DRIVEBY (enter|exit) tag=(\d+) t=(\d+) wep=(-?\d+) slot=(-?\d+) ammo=(-?\d+)(?: rev=(\d+))? outcome=([\w-]+)$/;

function parse(payload) {
  let m = RE_STATE.exec(payload);
  if (m) return { kind: 'state', tag: +m[1], t: +m[2], veh: m[3], wp: +m[4], slot: +m[5], ammo: +m[6], fire: +m[7], lookL: +m[8], lookR: +m[9], model: +m[10], speed: +m[11] };
  m = RE_SHOT.exec(payload);
  // `delay=` (ms) es la cadencia aplicada: lo añadió P4. Los logs antiguos no lo traen
  // (delay=null) y se siguen leyendo igual.
  if (m) return { kind: 'shot', tag: +m[1], t: +m[2], veh: m[3], wp: +m[4], slot: +m[5], anim: m[6], delay: m[7] != null ? +m[7] : null, ammo: +m[8] };
  m = RE_VEH.exec(payload);
  if (m) return { kind: m[1], tag: +m[2], t: +m[3], wp: +m[4], slot: +m[5], ammo: +m[6],
    rev: m[7] ? +m[7] : 1, outcome: m[8] };
  return null;
}

if (!fs.existsSync(FILE)) {
  console.log('No existe el log: ' + FILE + ' (pasa --file=<ruta> o VC_ODTRACE=<ruta>).');
  process.exit(2);
}

// El odtrace.log lleva el sello ISO del servidor delante del payload. Se recorre
// ENTERO (no solo las líneas DRIVEBY) para ir arrastrando el `JS build=<tag>` de
// cada arranque: cada evento se queda con el build que estaba corriendo.
const events = [];
let runningBuild = null;
for (const line of fs.readFileSync(FILE, 'utf8').split('\n')) {
  const m = /^(\d{4}-\d{2}-\d{2}T[\d:.]+Z)\s+(.*)$/.exec(line);
  const payload = m ? m[2] : line;
  const b = /^JS build=(\S+)/.exec(payload);
  if (b) { runningBuild = b[1]; continue; }
  if (payload.indexOf('DRIVEBY') !== 0) continue;
  const e = parse(payload);
  if (!e) continue;
  if (TAG_FILTER !== null && e.tag !== TAG_FILTER) continue;
  e.stamp = m ? m[1] : null;
  e.build = runningBuild;
  events.push(e);
}

if (!events.length) {
  const who = TAG_FILTER === null ? '' : ' con tag=' + TAG_FILTER;
  console.log('No hay ninguna línea DRIVEBY' + who + ' en ' + FILE + '.');
  console.log('=> nada que juzgar (¿la partida fue con un build anterior, o todavía no hubo drive-by?).');
  // El log ROTA por sesión (como máximo dos ficheros): si en el actual no hay
  // nada, lo normal es que la partida que buscas esté en el anterior.
  const prev = FILE.replace(/\.log$/, '.prev.log');
  if (prev !== FILE && fs.existsSync(prev) && fs.readFileSync(prev, 'utf8').includes('DRIVEBY')) {
    console.log('=> el log rota por sesión: la partida anterior está en ' + prev);
    console.log('   relánzalo así:  VC_ODTRACE="' + prev + '" node ' + path.basename(process.argv[1]));
  }
  process.exit(2);
}

// Sesiones: cortes por hueco temporal (varias pestañas/agentes comparten el log).
const sessions = [];
{
  let cur = null, lastStamp = 0;
  for (const e of events) {
    const wall = e.stamp ? Date.parse(e.stamp) : (lastStamp + 1);
    if (!cur || (wall - lastStamp) / 1000 > GAP_S) { cur = { first: e.stamp, last: e.stamp, tag: e.tag, ev: [] }; sessions.push(cur); }
    cur.ev.push(e);
    cur.last = e.stamp;
    lastStamp = wall;
  }
}

console.log('== log: ' + FILE);
console.log('== ' + events.length + ' líneas DRIVEBY en ' + sessions.length + ' sesión(es)' +
  (TAG_FILTER === null ? ' (todas)' : ' (tag=' + TAG_FILTER + ')'));

let sawFeature = false, sawContradiction = false;

sessions.forEach((s, i) => {
  const states = s.ev.filter((e) => e.kind === 'state');
  const shots = s.ev.filter((e) => e.kind === 'shot');
  const enters = s.ev.filter((e) => e.kind === 'enter');
  const exits = s.ev.filter((e) => e.kind === 'exit');
  const t = (stamp) => (stamp ? stamp.slice(11, 19) + 'Z' : '-');

  const byKey = new Map();
  for (const sh of shots) {
    const k = sh.veh + '|' + slot(sh.slot) + ' (' + wep(sh.wp) + ')|' + sh.anim +
      (sh.delay !== null ? '|delay=' + sh.delay + 'ms' : '');
    byKey.set(k, (byKey.get(k) || 0) + 1);
  }
  const pistol = shots.filter((sh) => sh.slot === 3);
  const smg = shots.filter((sh) => sh.slot === 5);
  // P4: con pistola, la cadencia tiene que ser la de su arma (`weapon.dat`:
  // Colt45/Beretta 210 ms), no los 70 ms fijos del SMG (el "bug de disparo
  // rápido" que reportó el jugador: «salen balas como si fuera de SMG»).
  const pistolFast = pistol.filter((sh) => sh.delay !== null && sh.delay <= 70);
  const pistolOwn = pistol.filter((sh) => sh.delay !== null && sh.delay > 70);
  const tries = new Map();          // slot -> segundos con el gatillo apretado
  for (const st of states) if (st.fire === 1) tries.set(st.slot, (tries.get(st.slot) || 0) + 1);
  const keepWeapon = enters.filter((e) => e.outcome === 'keep-weapon');
  const removeModel = enters.filter((e) => e.outcome === 'remove-model');
  const switchSmg = enters.filter((e) => e.outcome === 'switch-smg');
  // Las dos señales del fallo de precedencia (pistola en la mano que el motor
  // apartó al subir al vehículo). Con el arreglo no deben aparecer con slot 3.
  const pistolStoredAway = exits.filter((e) => e.slot === 3 && e.outcome === 'restore-stored');
  const pistolHidden = enters.filter((e) => e.slot === 3 && e.outcome === 'remove-model');
  const builds = [...new Set(s.ev.map((e) => e.build).filter(Boolean))];
  // rev del código de P2 dentro del wasm: 1 = build sin `rev=` (precedencia
  // vanilla), 2 = con el arreglo (la pistola en la mano gana al cambio a SMG).
  const revs = [...new Set([...enters, ...exits].map((e) => e.rev))].sort();
  const lookL = states.some((st) => st.lookL === 1);
  const lookR = states.some((st) => st.lookR === 1);

  console.log('\n--- sesión ' + (i + 1) + ': ' + t(s.first) + ' → ' + t(s.last) +
    '  (tag=' + s.tag + (s.tag === 0 ? ' = partida libre' : '') +
    (builds.length ? '; build ' + builds.join(', ') : '') +
    (revs.length ? '; P2 rev=' + revs.join('/') + (revs.includes(2) ? ' (con arreglo de precedencia)' : ' (sin el arreglo)') : '') +
    ') ---');
  console.log('  ' + states.length + ' s conduciendo (' + [...new Set(states.map((st) => st.veh))].join('/') +
    '; modelos ' + [...new Set(states.map((st) => st.model))].join(', ') +
    '; vel. máx ' + (states.length ? Math.max(...states.map((st) => st.speed)).toFixed(1) : '-') + ')');
  console.log('  ' + shots.length + ' disparos' + (byKey.size ? ':' : ' (ninguno)'));
  for (const [k, n] of [...byKey.entries()].sort()) console.log('     ' + k + '  x' + n);
  console.log('  entradas: ' + (enters.length ? enters.map((e) => slot(e.slot) + '->' + e.outcome).join(', ') : 'ninguna'));
  console.log('  salidas : ' + (exits.length ? exits.map((e) => slot(e.slot) + '->' + e.outcome).join(', ') : 'ninguna'));
  console.log('  gatillo apretado: ' + (tries.size ? [...tries.entries()].map(([sl, n]) => slot(sl) + ' ' + n + ' s').join(', ') : 'nunca') +
    '; mirada de lado: ' + (lookL || lookR ? (lookL ? 'izq ' : '') + (lookR ? 'der' : '') : 'no registrada'));

  // Veredicto de la sesión.
  const pistolaOk = pistol.length > 0;
  const pistolaRechazada = !pistolaOk && (tries.get(3) || 0) >= 2 && states.length > 0;
  const smgOk = smg.length > 0;
  if (pistolaOk) sawFeature = true;
  if (pistolaRechazada) sawContradiction = true;
  console.log('  veredicto: ' + (pistolaOk
    ? 'VICEEXT_DRIVEBY_WIDE EN MARCHA — pistola disparando desde ' +
      [...new Set(pistol.map((sh) => sh.veh))].join('/') +
      ' (animación ' + [...new Set(pistol.map((sh) => sh.anim))].join(', ') + ')' +
      (keepWeapon.length ? ' y la pistola se quedó en la mano (' + keepWeapon.length + ' entradas)' : '')
    : pistolaRechazada
      ? 'pistola RECHAZADA en vehículo: ' + tries.get(3) + ' s de gatillo con el slot 3 y 0 disparos' +
        ' (eso es el comportamiento vanilla: el build medido no lleva el define encendido)'
      : 'sin disparos con pistola (¿no hubo drive-by con pistola en esta sesión?)'));
  if (pistolStoredAway.length) { sawContradiction = true;
    console.log('             ⚠ el motor te CAMBIÓ la pistola al subir: ' + pistolStoredAway.length +
      ' salida(s) con slot 3 + restore-stored (entraste con la pistola en la mano y volvió al salir).' +
      ' Eso es la precedencia vanilla: falta el arreglo de P2 en el build que corrió.'); }
  if (pistolFast.length) { sawContradiction = true;
    console.log('             ⚠ P4: ' + pistolFast.length + ' disparo(s) de pistola con delay=70 ms' +
      ' (la cadencia del SMG): ese build no lleva el arreglo de cadencia por arma.'); }
  if (pistolOwn.length) console.log('             P4 OK: cadencia del arma en la pistola (' +
    [...new Set(pistolOwn.map((sh) => sh.delay))].join('/') + ' ms), y el SMG sigue a ' +
    ([...new Set(smg.filter((sh) => sh.delay !== null).map((sh) => sh.delay))].join('/') || '70') + ' ms.');
  if (pistolHidden.length) console.log('             vanilla visible: ' + pistolHidden.length +
    ' entrada(s) con slot 3 + remove-model (la pistola se escondió al subir)');
  if (removeModel.length) console.log('             vanilla visible: ' + removeModel.length +
    ' entrada(s) con outcome=remove-model (el arma se escondió al subir)');
  if (switchSmg.length) console.log('             SMG: ' + switchSmg.length +
    ' entrada(s) con outcome=switch-smg' + (smgOk ? ' y ' + smg.length + ' disparos con slot 5 (no regresión)' : ''));
});

console.log('\n== RESUMEN: ' + (sawFeature
  ? 'la mecánica se ve funcionar en los logs.'
  : sawContradiction
    ? 'los logs muestran el comportamiento VANILLA (el motor aparta la pistola al subir al vehículo).'
    : 'no hay evidencia de drive-by con pistola en estos logs (nada que juzgar).'));
process.exit(sawFeature ? 0 : sawContradiction ? 1 : 2);
