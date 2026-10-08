// Verificador de logs de "esconderse de la policía" (sección 2, bloque P1,
// `VICEEXT_HIDE_COPS`).
//
// LEE SOLO FICHEROS: no abre navegador, no lanza Chrome, no compila y no toca la
// partida. Sirve para confirmar, con los logs de una partida de verdad (tuya),
// que la mecánica se comporta como dice el contrato del bloque:
//
//   - con nivel de búsqueda >= 2 y NADIE viéndote, el motor empieza a contar:
//     `WANTEDHIDE start ... lvl=N ... t=<ms>` (una línea por episodio);
//   - a los VICEEXT_HIDE_STAR_MS (15 s de reloj del motor) desde ese instante,
//     cae una estrella: `WANTEDHIDE drop ... lvl=N->N-1 ... t=<ms>`, y en ese
//     momento la traza por segundo tiene que decir `seeing=0`;
//   - si un policía te vuelve a ver, el episodio se corta
//     (`WANTEDHIDE seen`), y si el nivel se queda en 0 sin búsqueda
//     (`WANTEDHIDE end`).
//
// La traza que se lee la deja el propio motor (una línea por segundo):
//   `WANTED tag=.. t=.. lvl=.. chaos=.. minlvl=.. cops=a/b listed=n presence18=..
//    seeing=.. near=.. unseen=.. hiding=.. veh=.. spd=.. objs=..`
// y las líneas de episodio: WANTEDHIDE start/seen/end/drop, WANTEDSUSPEND
// (garaje/guardado), WANTEDPURGE (respray) y WANTEDCHANGE (cambio de nivel).
//
// Cómo generar los logs (no hace falta nada especial):
//   1. juega normal: roba algo / mata a alguien hasta tener 2-3 estrellas;
//   2. escóndete de verdad (callejón, coche parado, sin que te vean) y espera
//      ~20 s sin que te encuentren;
//   3. `node gta_vc_browser/tools/hidecops-log-check.mjs`
//
// Uso:
//   node gta_vc_browser/tools/hidecops-log-check.mjs            (todas las sesiones)
//   node gta_vc_browser/tools/hidecops-log-check.mjs --tag=0    (solo las de partida libre)
//   node gta_vc_browser/tools/hidecops-log-check.mjs --file=<ruta a odtrace.log>
//   (o VC_ODTRACE=<ruta>)
//
// Códigos de salida: 0 = la mecánica se ve funcionar; 1 = los logs contradicen
// el contrato; 2 = todavía no hubo una búsqueda con nivel >= 2 (nada que juzgar).

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
const GAP_S = 180;              // hueco que separa una sesión de la siguiente
const STAR_MS = 15000;          // VICEEXT_HIDE_STAR_MS (contrato del bloque)
const STAR_TOL_MS = 2500;       // margen de la verificación (reloj del motor)

const RE_INIT = /^WANTEDHIDEINIT tag=(\d+) rev=(-?\d+) star_ms=(-?\d+) grace_ms=(-?\d+) sight_m=(-?\d+)(?: sweep_ms=(-?\d+))?(?: seen_ms=(-?\d+))?(?: sight_dot=(-?[\d.]+))? t=(\d+)$/;
const RE_COPJOIN = /^WANTEDCOP join tag=(\d+) slot=(-?\d+) lvl=(-?\d+) cops=(-?\d+)\/(-?\d+) burst=(-?\d+) t=(\d+)$/;
const RE_STATE = /^WANTED tag=(\d+) t=(\d+) lvl=(-?\d+) chaos=(\d+) minlvl=(-?\d+) cops=(-?\d+)\/(-?\d+) listed=(-?\d+) presence18=(-?\d+) seeing=(-?\d+) near=(-?[\d.]+) unseen=(\d+) hiding=(-?\d+) veh=(-?\d+) spd=(-?[\d.]+) objs=(.*)$/;
const RE_CHANGE = /^WANTEDCHANGE tag=(\d+) (-?\d+)->(-?\d+) chaos=(\d+) t=(\d+)$/;
const RE_HIDE = /^WANTEDHIDE (start|seen|end) tag=(\d+)(?: d=(\d+))? t=(\d+)$/;
const RE_HIDE_START = /^WANTEDHIDE start tag=(\d+) lvl=(-?\d+) chaos=(\d+) t=(\d+)$/;
const RE_DROP = /^WANTEDHIDE drop tag=(\d+) lvl=(-?\d+)->(-?\d+) chaos=(\d+) t=(\d+)$/;
const RE_SUSPEND = /^WANTEDSUSPEND tag=(\d+) lvl=(-?\d+) chaos=(\d+) t=(\d+)$/;
const RE_PURGE = /^WANTEDPURGE tag=(\d+) pursuit-cleared cops=(-?\d+) lvl=(-?\d+) chaos=(\d+) t=(\d+)$/;

function parse(payload) {
  let m = RE_INIT.exec(payload);
  if (m) return { kind: 'init', tag: +m[1], rev: +m[2], star_ms: +m[3], grace_ms: +m[4], sight_m: +m[5],
    sweep_ms: m[6] !== undefined ? +m[6] : null,
    seen_ms: m[7] !== undefined ? +m[7] : null,
    sight_dot: m[8] !== undefined ? +m[8] : null, t: +m[9] };
  m = RE_COPJOIN.exec(payload);
  if (m) return { kind: 'join', tag: +m[1], slot: +m[2], lvl: +m[3], cops: +m[4], maxcops: +m[5], burst: +m[6], t: +m[7] };
  m = RE_STATE.exec(payload);
  if (m) return { kind: 'state', tag: +m[1], t: +m[2], lvl: +m[3], chaos: +m[4], minlvl: +m[5],
    cops: +m[6], maxcops: +m[7], listed: +m[8], presence18: +m[9], seeing: +m[10], near: +m[11],
    unseen: +m[12], hiding: +m[13], veh: +m[14], spd: +m[15], objs: m[16] };
  m = RE_HIDE_START.exec(payload);
  if (m) return { kind: 'start', tag: +m[1], lvl: +m[2], chaos: +m[3], t: +m[4] };
  m = RE_DROP.exec(payload);
  if (m) return { kind: 'drop', tag: +m[1], from: +m[2], to: +m[3], chaos: +m[4], t: +m[5] };
  m = RE_HIDE.exec(payload);
  if (m) return { kind: m[1], tag: +m[2], seen_for: m[3] !== undefined ? +m[3] : 0, t: +m[4] };
  m = RE_CHANGE.exec(payload);
  if (m) return { kind: 'change', tag: +m[1], from: +m[2], to: +m[3], chaos: +m[4], t: +m[5] };
  m = RE_SUSPEND.exec(payload);
  if (m) return { kind: 'suspend', tag: +m[1], lvl: +m[2], chaos: +m[3], t: +m[4] };
  m = RE_PURGE.exec(payload);
  if (m) return { kind: 'purge', tag: +m[1], cops: +m[2], lvl: +m[3], chaos: +m[4], t: +m[5] };
  return null;
}

if (!fs.existsSync(FILE)) {
  console.log('No existe el log: ' + FILE + ' (pasa --file=<ruta> o VC_ODTRACE=<ruta>).');
  process.exit(2);
}

// Se recorre el log ENTERO arrastrando el `JS build=<tag>` de cada arranque: una
// partida con un build viejo no puede probar un arreglo nuevo.
const events = [];
let runningBuild = null;
for (const line of fs.readFileSync(FILE, 'utf8').split('\n')) {
  const m = /^(\d{4}-\d{2}-\d{2}T[\d:.]+Z)\s+(.*)$/.exec(line);
  const payload = m ? m[2] : line;
  const b = /^JS build=(\S+)/.exec(payload);
  if (b) { runningBuild = b[1]; continue; }
  if (!payload.startsWith('WANTED')) continue;
  const e = parse(payload);
  if (!e) continue;
  if (TAG_FILTER !== null && e.tag !== TAG_FILTER) continue;
  e.stamp = m ? m[1] : null;
  e.build = runningBuild;
  events.push(e);
}

if (!events.length) {
  const who = TAG_FILTER === null ? '' : ' con tag=' + TAG_FILTER;
  console.log('No hay ninguna línea WANTED' + who + ' en ' + FILE + '.');
  console.log('=> nada que juzgar (¿la partida fue con un build anterior a la instrumentación?).');
  // El log ROTA por sesión (como máximo dos ficheros): si en el actual no hay
  // nada, lo normal es que la partida que buscas esté en el anterior.
  const prev = FILE.replace(/\.log$/, '.prev.log');
  if (prev !== FILE && fs.existsSync(prev) && fs.readFileSync(prev, 'utf8').includes('WANTED')) {
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
console.log('== ' + events.length + ' líneas WANTED en ' + sessions.length + ' sesión(es)' +
  (TAG_FILTER === null ? ' (todas)' : ' (tag=' + TAG_FILTER + ')'));

let sawFeature = false, sawContradiction = false, sawChase = false;

sessions.forEach((s, i) => {
  const states = s.ev.filter((e) => e.kind === 'state');
  const starts = s.ev.filter((e) => e.kind === 'start');
  const drops = s.ev.filter((e) => e.kind === 'drop');
  const seen = s.ev.filter((e) => e.kind === 'seen');
  const ends = s.ev.filter((e) => e.kind === 'end');
  const changes = s.ev.filter((e) => e.kind === 'change');
  const inits = s.ev.filter((e) => e.kind === 'init');
  const joins = s.ev.filter((e) => e.kind === 'join');
  const suspends = s.ev.filter((e) => e.kind === 'suspend');
  const purges = s.ev.filter((e) => e.kind === 'purge');
  const builds = [...new Set(s.ev.map((e) => e.build).filter(Boolean))];
  const t = (stamp) => (stamp ? stamp.slice(11, 19) + 'Z' : '-');
  const s2 = (ms) => (ms / 1000).toFixed(1) + ' s';

  const maxLvl = states.length ? Math.max(...states.map((st) => st.lvl)) : 0;
  const maxSeen = states.length ? Math.max(...states.map((st) => st.lvl)) : 0;
  const anyHiding = states.some((st) => st.hiding === 1);
  const levels = [...new Set([
    ...changes.flatMap((e) => [e.from, e.to]),
    ...states.map((st) => st.lvl),
  ])].sort((a, b) => a - b);

  console.log('\n--- sesión ' + (i + 1) + ': ' + t(s.first) + ' → ' + t(s.last) +
    '  (tag=' + s.tag + (s.tag === 0 ? ' = partida libre' : '') +
    (builds.length ? '; build ' + builds.join(', ') : '') + ') ---');
  console.log('  ' + states.length + ' s de traza; nivel máximo ' + maxLvl +
    '; niveles vistos: ' + (levels.length ? levels.join(',') : '-') +
    '; cambios de nivel: ' + (changes.length ? changes.map((e) => e.from + '->' + e.to).join(', ') : 'ninguno'));
  console.log('  escondido (hiding=1): ' + (anyHiding ? 'sí, ' + states.filter((st) => st.hiding === 1).length + ' s' : 'no') +
    '; episodios start=' + starts.length + ' seen=' + seen.length + ' end=' + ends.length + ' drop=' + drops.length);
  // La revisión del bloque viaja en `WANTEDHIDEINIT` (una línea por sesión): es
  // lo único que prueba QUÉ `.wasm` jugó (la etiqueta `build=` es del JS).
  if (inits.length) console.log('  bloque P1 rev=' + [...new Set(inits.map((e) => e.rev))].join(',') +
    ' (star=' + inits[0].star_ms + 'ms grace=' + inits[0].grace_ms + 'ms sight=' + inits[0].sight_m + 'm' +
    (inits[0].sweep_ms !== null ? ' barrido de visibilidad=' + inits[0].sweep_ms + 'ms' : '') + ')' +
    (inits.some((e) => e.rev < 3) ? '  ⚠ revisión vieja: la traza de unión no está limitada' : '') +
    (inits.some((e) => e.rev < 7)
      ? '  ⚠ rev<7: la última estrella (nivel 1) no entra en la regla de esconderse, así que puede no llegar a 0 nunca (fallo medido el 21/09: bajaba de 3 a 1 y ahí se quedaba)'
      : '') +
    (inits.some((e) => e.rev < 6)
      ? '  ⚠ rev<6: sin el arreglo del 20/09 el "te ve" no exige que el policía mire hacia ti y un destello cuenta como avistamiento: a nivel >=2 la estrella puede no bajar nunca'
      : '') +
    (inits.some((e) => e.rev < 5)
      ? '  ⚠ rev<5: esa sesión puede haber cargado partida y, con el reloj del motor retrocediendo, la traza se quedó muda (0 líneas `WANTED tag=` o `WANTEDCOP join` en toda la sesión)'
      : ''));
  // Las uniones a la persecución se filtran a una línea/s; una ráfaga grande
  // significa que el motor unía y soltaba al policía en cada frame (zona de
  // taller: `CWorld::CallOffChaseForArea` desde Garages.cpp).
  if (joins.length) console.log('  uniones a la persecución: ' + joins.length + ' línea(s), ráfaga máxima ' +
    Math.max(...joins.map((e) => e.burst)) + ' unión(es)/s' +
    (Math.max(...joins.map((e) => e.burst)) > 3 ? ' (el policía no puede perseguir ahí: vanilla)' : ''));
  if (suspends.length) console.log('  garajes/guardados (WANTEDSUSPEND): ' +
    suspends.map((e) => 'lvl=' + e.lvl + ' @' + (e.t / 1000).toFixed(1) + 's').join(', '));
  if (purges.length) console.log('  resprays (WANTEDPURGE): ' +
    purges.map((e) => 'lvl=' + e.lvl + ' cops=' + e.cops).join(', '));

  // Empareja cada `start` con lo primero que le pasa: un `drop`, o un corte
  // (`seen`/`end`) si te vuelven a ver antes de que caiga la estrella.
  const episodes = [];
  let pending = null;
  for (const e of s.ev) {
    if (e.kind === 'start') pending = { start: e, closedBy: null, drop: null };
    else if (pending && (e.kind === 'drop' || e.kind === 'seen' || e.kind === 'end')) {
      pending.closedBy = e;
      if (e.kind === 'drop') pending.drop = e;
      episodes.push(pending);
      pending = null;
    }
  }
  if (pending) episodes.push(pending);

  for (const ep of episodes) {
    const st = ep.start;
    const tag = '    start t=' + (st.t / 1000).toFixed(1) + 's lvl=' + st.lvl;
    if (ep.drop) {
      const latency = ep.drop.t - st.t;
      // Estado justo antes de la caída: ¿había alguien viéndote?
      const before = states.filter((x) => x.t <= ep.drop.t).pop();
      const seeingAtDrop = before ? before.seeing : null;
      const unseenAtDrop = before ? before.unseen : null;
      const lvlOk = ep.drop.to === ep.drop.from - 1;
      const latencyOk = Math.abs(latency - STAR_MS) <= STAR_TOL_MS;
      const blindOk = seeingAtDrop === 0;
      const ok = lvlOk && latencyOk && blindOk;
      if (ok) sawFeature = true; else sawContradiction = true;
      console.log(tag + ' -> drop t=' + (ep.drop.t / 1000).toFixed(1) + 's ' +
        ep.drop.from + '->' + ep.drop.to + ' en ' + s2(latency) +
        (ok ? '  PASS' : '  ⚠ REVISAR') +
        ' (esperado ~' + s2(STAR_MS) + ', ' +
        (latencyOk ? 'latencia ok' : 'latencia fuera de rango') + '; ' +
        (lvlOk ? 'cae 1 estrella' : 'NO cae 1 estrella') + '; ' +
        (blindOk ? 'seeing=0' : 'seeing=' + seeingAtDrop + ' (te veían)') +
        (unseenAtDrop !== null ? '; unseen=' + unseenAtDrop + 's' : '') + ')');
      // La misma caída tiene que verse en la traza por segundo (nivel nuevo).
      const after = states.find((x) => x.t >= ep.drop.t);
      if (after && after.lvl !== ep.drop.to)
        console.log('      ⚠ la traza por segundo dice lvl=' + after.lvl + ' justo después de la caída (esperado ' + ep.drop.to + ')');
    } else if (ep.closedBy) {
      console.log(tag + ' -> ' + ep.closedBy.kind + ' t=' + (ep.closedBy.t / 1000).toFixed(1) + 's en ' +
        s2(ep.closedBy.t - st.t) + ' (episodio cortado: te vieron o el nivel llegó a 0; no es un fallo)');
    } else {
      console.log(tag + ' -> sin cierre en el log (¿seguías escondido al terminar la sesión?)');
    }
  }

  const grande = states.filter((st) => st.lvl >= 2);
  const blind = grande.filter((st) => st.seeing === 0);
  // "Ciego de verdad": ni un perseguidor te ve, ni hay policía a 18 m (unidad de
  // patrulla pasando). Solo ahí el contrato dice que tiene que empezar a contar.
  const blindAlone = blind.filter((st) => st.presence18 === 0);
  const blindWithCopsNear = blind.length - blindAlone.length;
  if (grande.length) sawChase = true;
  console.log('  veredicto: ' + (drops.length
    ? drops.length + ' caída(s) de estrella sin que te vieran'
    : grande.length
      ? 'hubo búsqueda (lvl>=2, ' + grande.length + ' s) pero sin caída de estrella en el log'
      : 'sin búsqueda de nivel >= 2 en esta sesión (nada que juzgar)'));
  if (blind.length && !starts.length)
    console.log('      sin episodio de esconderse: ' + blind.length + ' s sin que te vieran' +
      (blindWithCopsNear ? ', pero ' + blindWithCopsNear + ' de ellos con policía a 18 m (`presence18>0`): no estabas escondido' : '') +
      (blindAlone.length ? '; ' + blindAlone.length + ' s a lvl>=2 sin nadie cerca (aquí sí debería haber empezado el episodio)' : ''));
  if (blindAlone.length >= 10 && !starts.length)
    console.log('      ⚠ ' + blindAlone.length + ' s a lvl>=2 sin policía viéndote ni cerca y ningún `WANTEDHIDE start`: el build que corrió no lleva VICEEXT_HIDE_COPS');
});

console.log('\n== RESUMEN: ' + (sawFeature
  ? 'la mecánica se ve funcionar en los logs (caída de estrella a ~15 s sin que te vean).'
  : sawContradiction
    ? 'hay algo que revisar en las caídas de estrella (mira las líneas ⚠).'
    : sawChase
      ? 'hubo búsqueda pero no llegó a caer ninguna estrella escondido: nada confirmado todavía.'
      : 'no hubo ninguna búsqueda de nivel >= 2 en estos logs (nada que juzgar).'));
process.exit(sawFeature ? 0 : sawContradiction ? 1 : 2);
