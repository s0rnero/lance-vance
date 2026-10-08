#!/usr/bin/env node
/*
 * Verifica, SÓLO desde los logs, que los bloques de la SECCIÓN 3 se ejecutan.
 *
 * Para qué: los bloques C1 (1ª persona), C2 (autosave / guardar en cualquier
 * sitio), C3.1 (recarga a mano), C3.3 (depósito), C3.4 (intermitentes) y C3.5
 * (luces que se rompen al disparo) dejan trazas en `odtrace.log` mientras se
 * juega. Este verificador las lee y dice qué se ha visto y qué no, sin lanzar
 * Chrome: se juega una vez y luego se pasa esto sobre el log.
 *
 * Uso:
 *   node gta_vc_browser/tools/seccion3-log-check.mjs                # usa web/odtrace.log
 *   node gta_vc_browser/tools/seccion3-log-check.mjs <ruta.log>
 *   node gta_vc_browser/tools/seccion3-log-check.mjs <ruta.log> --ultimas=5
 *
 * Trazas que busca (las emite el motor):
 *
 *   VICEEXT 1p key ctrl=.. wide=.. ...      la tecla del conmutador (V) se ha
 *          visto pulsada, con el estado que decide si el modo puede aplicarse
 *          (C1b, build ve13+). Es la traza que contesta "¿la tecla llega?".
 *   CAM1P  fp=.. mode=.. tog=.. togkey=..   estado del conmutador de 1ª persona
 *          (C1). Sólo escribe al conmutar o mientras el modo está encendido.
 *          Prueba: tog=1 y luego líneas de paseo -> mode=41 con spd>0; salir ->
 *          tog=0 y mode=4.
 *   VICEEXT camauto auto=.. modo=.. mirando=..
 *          autocentrado de cámara del coche (C1b-2): sólo escribe al cambiar de
 *          estado (autocentrando / dejándolo).
 *   VICEEXT swim enter|move|exit            nadar (C4). `move` va 1/s mientras
 *          se nada; `enter`/`exit` son los cambios de estado.
 *   VICEEXT crouch on|off arma=..           agachado con C (C5).
 *   VICEEXT sprint grupo=.. arma=.. pesada=..  esprint con arma (C6): `grupo` es
 *          el grupo de animación del jugador y `pesada` si el arma es de 2 manos.
 *   FEMENU scr=.. opt=.. only=..             cambios de pantalla del front-end
 *          (distingue "el menú no se abrió" de "el guardado rechazó").
 *   VICEEXT autosave ok=.. slot=.. file=..   autosave de fin de misión (C2).
 *   VICEEXT save-anywhere aceptado|rechazado  guardar desde el menú (C2).
 *   VICEEXT reload start / reload no / reload done manual=1
 *          recarga con la tecla R (C3.1). `done` SÓLO sale si la pidió el
 *          jugador: la recarga automática al vaciar el cargador no la imprime.
 *   VICEEXT gastank hit / reserva              disparo al depósito (C3.3).
 *          `reserva=1` = el modelo no traía el dummy `petrolcap` y el depósito se
 *          ha localizado por la caja de colisión (C3.3c).
 *   VICEEXT luces rota luz=headlight_l...     faro roto por disparo (C3.5).
 *   VICEEXT turners quien=jugador|npc ...     intermitentes (C3.4, define
 *          APAGADO por defecto: si no sale, lo normal es que esté apagado).
 */

import fs from 'node:fs';
import path from 'node:path';

const args = process.argv.slice(2);
const logPath = args.find(a => !a.startsWith('--')) ?? 'gta_vc_browser/web/odtrace.log';
const ultimas = Number((args.find(a => a.startsWith('--ultimas=')) ?? '--ultimas=3').split('=')[1]);
const conPrev = args.includes('--prev');

const leer = p => {
  try {
    return fs.readFileSync(path.resolve(p), 'utf8').split('\n').filter(l => l.length > 0);
  } catch {
    return [];
  }
};

let lines = leer(logPath);
const prevPath = path.join(path.dirname(logPath), 'odtrace.prev.log');
let usadoPrev = false;
if (conPrev || (lines.length < 50 && fs.existsSync(prevPath))) {
  lines = leer(prevPath).concat(lines);   // el log rota solo: sesión anterior + actual
  usadoPrev = true;
}
if (lines.length === 0) {
  console.error(`No puedo leer nada de ${logPath}`);
  process.exit(2);
}

/** Busca líneas que cumplan el predicado. */
const buscar = (re, filtro) =>
  lines.filter(l => re.test(l) && (!filtro || filtro(l)));

const campo = (linea, nombre) => {
  const m = linea.match(new RegExp(`${nombre}=(-?[\\d.]+)`));
  return m ? Number(m[1]) : NaN;
};

// Notas que añaden los propios bloques (p. ej. "esta línea es de un build
// anterior"). Tiene que estar a nivel de módulo: los `extra()` de abajo se
// definen aquí y no verían una variable local del bucle.
let notasExtra = [];

const bloques = [
  {
    id: 'C1',
    titulo: '1ª persona (conmutador)',
    prueba: 'pulsa V (conmutador), anda, apunta y vuelve a pulsar V',
    // C1b: la traza de la tecla es la que distingue "la tecla no llega" de
    // "llega y el modo no puede aplicarse". Cuenta como visto sólo si además el
    // modo se encendió; si no, lo dice con la línea de la tecla delante.
    lineas: () => buscar(/CAM1P /, l => campo(l, 'tog') === 1),
    extra: () => {
      const tecla = buscar(/VICEEXT 1p key/);
      notasExtra.push(tecla.length
        ? `${tecla.length} pulsaciones de la tecla vistas (VICEEXT 1p key); última -> ${tecla[tecla.length - 1].trim().slice(40, 150)}`
        : 'NO hay líneas `VICEEXT 1p key`: o la tecla no llega al motor, o el build es anterior a ve13 (en ve12 el conmutador estaba en B y la traza no existía)');
      const on = buscar(/CAM1P /, l => campo(l, 'tog') === 1);
      const andando = on.filter(l => /spd=(\d+)/.test(l) && Number(l.match(/spd=(\d+)/)[1]) > 5);
      const off = buscar(/CAM1P /, l => campo(l, 'tog') === 0);
      const modos = [...new Set(on.map(l => campo(l, 'mode')))];
      const notas = [];
      notas.push(`líneas con tog=1: ${on.length} (modos vistos: ${modos.join(',') || '—'})`);
      notas.push(`de ellas andando (spd>5): ${andando.length}`);
      if (on.length)
        notas.push(off.length ? 'hay tog=0: el modo se cerró' : 'NO hay tog=0: sólo se encendió');
      else
        notas.push('la tecla del conmutador no se ha pulsado en esta sesión');
      return notas;
    },
  },
  {
    id: 'C2a',
    titulo: 'Autosave al superar misión',
    prueba: 'supera una misión (el guardado se escribe al terminar su script)',
    lineas: () => buscar(/VICEEXT autosave /),
    extra: () => {
      const ok = buscar(/VICEEXT autosave /, l => campo(l, 'ok') === 1);
      const ko = buscar(/VICEEXT autosave /, l => campo(l, 'ok') === 0);
      return [
        `escritos: ${ok.length}${ok.length ? ` -> ${ok[ok.length - 1]}` : ''}`,
        ko.length ? `RECHAZADOS por las guardas: ${ko.length}` : 'ningún rechazo',
        'el fichero del autosave es GTAVCsf9.b (ranura 9 del menú de carga)',
      ];
    },
  },
  {
    id: 'C2b',
    titulo: 'Guardar en cualquier sitio (menú de pausa)',
    prueba: 'Esc -> GUARDAR PARTIDA -> elige ranura; parado, sin misión y sin búsqueda',
    lineas: () => buscar(/VICEEXT save-anywhere /),
    extra: () => {
      const si = buscar(/VICEEXT save-anywhere aceptado/);
      const no = buscar(/VICEEXT save-anywhere rechazado/);
      const pantallas = buscar(/FEMENU /, l => campo(l, 'scr') === 7); // MENUPAGE_CHOOSE_SAVE_SLOT
      return [
        `aceptados: ${si.length}, rechazados: ${no.length}`,
        `pantallas de elegir ranura abiertas (FEMENU scr=7): ${pantallas.length}`,
        pantallas.length === 0
          ? 'no se abrió la pantalla de ranuras: ¿no se llegó a pulsar GUARDAR PARTIDA?'
          : 'la pantalla se abrió (si no hay aceptado/rechazado, no se eligió ranura)',
        'el rechazo sale cuando hay misión en curso, nivel de búsqueda o te estás moviendo',
      ];
    },
  },
  {
    id: 'C3.1',
    titulo: 'Recarga a mano (tecla R)',
    prueba: 'dispara unas balas para dejar el cargador a medias y pulsa R',
    // Ojo: `done` sin `manual=` es de un build anterior a ve12, cuando la traza
    // no distinguía la recarga automática de la del jugador. No cuenta.
    lineas: () => buscar(/VICEEXT reload (start|no|done)/, l => !/reload done (?!manual=)/.test(l)),
    extra: () => {
      const start = buscar(/VICEEXT reload start/);
      const no = buscar(/VICEEXT reload no /);
      const done = buscar(/VICEEXT reload done manual=1/);
      const viejas = buscar(/VICEEXT reload done /, l => !/manual=/.test(l));
      if (viejas.length)
        notasExtra.push(`${viejas.length} líneas \`reload done\` SIN \`manual=\`: son de un build anterior a ve12 y no valen como prueba (la traza no distinguía la recarga automática)`);
      const motivos = {};
      for (const l of no) {
        const m = l.match(/reload no clip=(\d+)\/(\d+) total=(\d+)/);
        if (m) {
          const clave = Number(m[3]) <= 0 ? 'sin munición total' : 'cargador ya lleno';
          motivos[clave] = (motivos[clave] ?? 0) + 1;
        }
      }
      return [
        `pulsaciones aceptadas (start): ${start.length}, rechazadas: ${no.length}, recargas terminadas: ${done.length}`,
        Object.keys(motivos).length ? `motivos del rechazo: ${JSON.stringify(motivos)}` : 'sin rechazos',
        'done sólo cuenta recargas pedidas con la tecla (no la automática al vaciar el cargador)',
      ];
    },
  },
  {
    id: 'C3.3',
    titulo: 'Depósito de gasolina (balancea el coche y dispara al tapón)',
    prueba: 'dispara al tapón del depósito (trasero lateral) de un coche adaptado del mod',
    lineas: () => buscar(/VICEEXT gastank /),
    extra: () => {
      const hit = buscar(/VICEEXT gastank hit /);
      const reserva = buscar(/VICEEXT gastank reserva /);
      const reservaUsada = hit.filter(l => campo(l, 'reserva') === 1).length;
      const viejas = hit.filter(l => !/reserva=/.test(l));
      if (viejas.length)
        notasExtra.push(`${viejas.length} líneas \`gastank hit\` SIN \`reserva=\`: son de un build anterior a ve13 (entonces el coche ardía, no explotaba)`);
      const modelos = [...new Set(reserva.map(l => l.match(/model=(\d+)/)?.[1]))];
      return [
        `impactos en el depósito (explosión inmediata): ${hit.length}${hit.length ? ` -> ${hit[hit.length - 1]}` : ''}`,
        `de ellos por respaldo de caja de colisión (sin dummy petrolcap): ${reservaUsada}`,
        `modelos sin dummy vistos: ${modelos.length ? modelos.join(',') : 'ninguno'} (1 línea por modelo)`,
        'la explosión es la de serie (BlowUpCar): onda, fuego y destrozo',
      ];
    },
  },
  {
    id: 'C3.5',
    titulo: 'Luces que se rompen al disparo',
    prueba: 'dispara al faro (o al piloto trasero) de un coche adaptado del mod',
    lineas: () => buscar(/VICEEXT luces rota /),
    extra: () => {
      const roto = buscar(/VICEEXT luces rota /);
      const luces = {};
      for (const l of roto) {
        const m = l.match(/luz=(\S+)/);
        if (m) luces[m[1]] = (luces[m[1]] ?? 0) + 1;
      }
      return [
        `faros/pilotos rotos a tiros: ${roto.length}`,
        Object.keys(luces).length ? `reparto: ${JSON.stringify(luces)}` : 'ninguno todavía',
        'efecto visible: ese faro deja de alumbrar (el motor ya sabe dibujar una luz rota)',
      ];
    },
  },
  {
    id: 'C3.4',
    titulo: 'Intermitentes (define APAGADO por defecto)',
    prueba: 'recompila con VICEEXT_TURN_SIGNALS y gira el volante',
    lineas: () => buscar(/VICEEXT turners /),
    extra: () => {
      const t = buscar(/VICEEXT turners /);
      const jugador = t.filter(l => /quien=jugador/.test(l)).length;
      const npc = t.filter(l => /quien=npc/.test(l)).length;
      return [
        `líneas: ${t.length} (jugador: ${jugador}, NPC: ${npc})`,
        t.length === 0
          ? 'no hay líneas: o el define está apagado (lo normal) o nadie giró el volante'
          : 'coronas naranjas en los objetos/dummies indicator* del modelo',
        'su features.ini trae StandardCarsUseTurnSignals=0, de ahí que el define nazca apagado',
      ];
    },
  },
  {
    id: 'C1b-2',
    titulo: 'Autocentrado de cámara (sólo en vehículo)',
    prueba: 'conduce recto sin tocar el ratón ~2 s (y luego tócalo: debe dejar de recentrar)',
    lineas: () => buscar(/VICEEXT camauto /, l => campo(l, 'auto') === 1),
    extra: () => {
      const todas = buscar(/VICEEXT camauto /);
      const si = todas.filter(l => campo(l, 'auto') === 1).length;
      const no = todas.filter(l => campo(l, 'auto') === 0).length;
      return [
        `activaciones: ${si}, desactivaciones: ${no}`,
        si === 0 && todas.length === 0
          ? 'sin líneas: o no has conducido, o el build es anterior a ve13'
          : 'a pie NO se recentra nunca (el jugador dijo que ahí no hace falta)',
        'se enciende tras 1,5 s sin tocar la mirada y sin apuntar; el desactivado sale al volver a mirar',
      ];
    },
  },
  {
    id: 'C4',
    titulo: 'Nadar',
    prueba: 'tírate al agua honda (mar) y muévete; luego sal a la orilla',
    lineas: () => buscar(/VICEEXT swim (enter|move|exit)/),
    extra: () => {
      const enter = buscar(/VICEEXT swim enter/);
      const move = buscar(/VICEEXT swim move/);
      const exit = buscar(/VICEEXT swim exit/);
      const spds = move.map(l => campo(l, 'spd')).filter(n => !Number.isNaN(n));
      return [
        `entradas al agua: ${enter.length}, líneas nadando (1/s): ${move.length}, salidas: ${exit.length}`,
        spds.length ? `velocidad nadando: min ${Math.min(...spds)} máx ${Math.max(...spds)} m/s (quieto = 0)` : 'sin datos de velocidad',
        move.length && !enter.length ? 'AVISO: hay `swim move` sin `swim enter` (build anterior a ve13)' : 'el estado de nado se abre y se cierra con su traza',
        'el clip va alternando swim_tread / swim_breast / swim_crawl; los tres están en el ped.ifp del mod',
      ];
    },
  },
  {
    id: 'C5',
    titulo: 'Agachado (tecla C)',
    prueba: 'a pie y sin arma, pulsa C (agacharse), anda agachado y vuelve a pulsar C',
    lineas: () => buscar(/VICEEXT crouch on/),
    extra: () => {
      const on = buscar(/VICEEXT crouch on/);
      const off = buscar(/VICEEXT crouch off/);
      return [
        `agachado ON: ${on.length}, OFF: ${off.length}`,
        on.length && !off.length ? 'te has agachado y no te has levantado (¿el clip se quedó puesto?)' : 'entra y sale con la misma tecla',
        'con arma lo lleva el motor por su camino de siempre; esto es el caso sin arma/cuerpo a cuerpo',
      ];
    },
  },
  {
    id: 'C6',
    titulo: 'Esprintar con arma de 2 manos',
    prueba: 'con un rifle/escopeta en la mano, corre (Shift) hasta esprintar',
    lineas: () => buscar(/VICEEXT sprint grupo=/),
    extra: () => {
      const s = buscar(/VICEEXT sprint grupo=/);
      const pesadas = s.filter(l => campo(l, 'pesada') === 1).length;
      const grupos = [...new Set(s.map(l => campo(l, 'grupo')))].join(',');
      return [
        `entradas en esprint: ${s.length} (con arma pesada: ${pesadas})`,
        grupos ? `grupos del jugador usados: ${grupos}` : 'sin grupos',
        'el hueco RUNFAST del grupo del jugador debe ser sprint_armed/sprint_rocket/sprint_csaw (no run_armed)',
      ];
    },
  },
];

const color = process.stdout.isTTY && !process.env.NO_COLOR;
const verde = s => (color ? `\x1b[32m${s}\x1b[0m` : s);
const amar = s => (color ? `\x1b[33m${s}\x1b[0m` : s);
const gris = s => (color ? `\x1b[90m${s}\x1b[0m` : s);

console.log(`Log: ${logPath}${usadoPrev ? ' (+ odtrace.prev.log)' : ''}`);
console.log(`Líneas: ${lines.length}  (de ${lines[0].slice(0, 24)} a ${lines[lines.length - 1].slice(0, 24)})\n`);

let fallos = 0;
for (const b of bloques) {
  notasExtra = [];
  const encontradas = b.lineas();
  const visto = encontradas.length > 0;
  if (!visto) fallos++;
  console.log(`${visto ? verde('VISTO  ') : amar('FALTA  ')} ${b.id.padEnd(5)} ${b.titulo}`);
  if (visto) {
    for (const l of encontradas.slice(-ultimas)) console.log(gris(`         ${l.trim().slice(0, 150)}`));
    if (encontradas.length > ultimas) console.log(gris(`         … y ${encontradas.length - ultimas} más`));
  } else {
    console.log(gris(`         prueba: ${b.prueba}`));
  }
  for (const n of b.extra().concat(notasExtra)) console.log(`         · ${n}`);
  console.log('');
}

console.log(fallos === 0
  ? verde('Todos los bloques de la sección 3 han dejado su traza.')
  : amar(`Sin traza todavía: ${bloques.filter(b => b.lineas().length === 0).map(b => b.id).join(', ')}.`));
console.log(gris('Recuerda: C3.4 nace apagado a propósito; y el log rota solo, así que lo que no'));
console.log(gris('haya entrado en la sesión actual se puede mirar en odtrace.prev.log.'));
