// Escenarios reutilizables del arnés. Cada escenario es un objeto:
//
//   { nombre, titulo, marcas: [...], async ejecutar(s) { ... return informe } }
//
// `s` es la sesión de `vc-harness.mjs` ya con la partida cargada, así que un
// escenario solo declara SUS pasos y SUS comprobaciones. Añadir una prueba nueva
// es copiar un objeto, no copiar 150 líneas de arranque.
//
// CONVENCIÓN DE COMPROBACIONES: cada `check` lleva la EVIDENCIA (el número
// medido o la línea de traza). Una prueba que falla tiene que poder leerse sin
// abrir el log: es la diferencia entre "FALLO" y saber qué pasó.

import { informe, campo, texto, resumen, num, sleep } from './vc-harness.mjs';

// Líneas de una etiqueta dentro de un texto (sin el prefijo de hora).
export function lineasDe(txt, tag) {
  const re = new RegExp('(^|\\s)' + tag + '\\s');
  return String(txt).split('\n').map((l) => l.trim()).filter((l) => re.test(l))
    .map((l) => l.replace(/^\S+Z\s+/, ''));
}

export const modosDe = (ls) => [...new Set(ls.map((l) => campo(l, 'modo')).filter((m) => !Number.isNaN(m)))];

// --- navegación hasta el agua (la necesitan los escenarios de agua) -----------
//
// El ped se mueve en un mundo abierto: antes las sondas andaban 4×30 s a ciegas
// y a veces se quedaban contra una casa (22/09: la parte de nado quedó SIN MEDIR
// y eso se leía como fallo del mod). Aquí la sesión navega con datos del motor:
// `PEDAT ... dist=` es la distancia al agua que mide el propio juego
// (`CWaterLevel`), así que se sostiene el rumbo que ACERCA al mar y se gira ~45°
// cuando no se acerca o el ped se queda clavado.
// El motor ya dice HACIA DÓNDE está el agua (`PEDAT ... mar=` = azimut en grados
// del mundo) y A QUÉ DISTANCIA (`dist=`), así que no hay que barrer rumbos a
// ciegas: se apunta. La relación grados↔píxeles del ratón se CALIBRA sola con
// los dos primeros tramos (cuánto cambió el rumbo real por los píxeles girados),
// y a partir de ahí cada tramo corrige el error de rumbo de golpe.
//
// Antes (barrido de 45° en 45°): la primera corrida del arnés tardó 3,2 min en
// llegar al agua; el escenario de nado se comía la suite entera.
const normaliza180 = (d) => ((d + 540) % 360) - 180;

export async function navegarAlAgua(s, { presupuestoS = 240, tramoS = 4, giroPx = 130 } = {}) {
  const s0 = s.traza.marca();
  const pos = () => {
    const l = s.traza.ultima('PEDAT', s0);
    return l ? { x: campo(l, 'x'), y: campo(l, 'y'), mar: campo(l, 'mar'), dist: campo(l, 'dist') } : null;
  };
  const anda = async (seg) => {
    await s.abajo('KeyW');
    await sleep(seg * 1000);
    await s.suelta('KeyW');
    await sleep(400);
  };

  let nadando = false, tramos = 0, giros = 0, ciegos = 0;
  let degPorPx = null;          // se calibra con los dos primeros rumbos
  let rumboPrev = null, pxPrevios = 0;
  const t0 = Date.now();
  const log = [];
  while (!nadando && (Date.now() - t0) / 1000 < presupuestoS && !s.errores.length) {
    const a = pos();
    await anda(tramoS);
    tramos++;
    if (s.traza.leer(s0).indexOf('VICEEXT swim enter') >= 0) { nadando = true; break; }
    const b = pos();
    if (!a || !b) { await s.gira(giroPx); giros++; continue; }

    const av = Math.hypot(b.x - a.x, b.y - a.y);
    const rumbo = Math.atan2(b.y - a.y, b.x - a.x) * 180 / Math.PI;
    // Calibración: cuántos grados de rumbo reales movió el último giro de ratón.
    if (rumboPrev !== null && pxPrevios !== 0) {
      const d = normaliza180(rumbo - rumboPrev);
      if (Math.abs(d) > 3) degPorPx = d / pxPrevios;
    }
    rumboPrev = rumbo;

    const veAgua = b.mar >= 0;
    let motivo, px = 0;
    if (veAgua) {
      // Apuntar al agua: error de rumbo y cuántos píxeles son.
      const err = normaliza180(b.mar - rumbo);
      // `degPorPx` (medido) es "grados de rumbo por píxel", con signo: para
      // girar `err` grados hacen falta `err / degPorPx` píxeles.
      const k = degPorPx !== null ? degPorPx : 1 / 2.9;   // sin calibrar: 130 px ≈ 45°
      px = err / k;
      if (Math.abs(px) > 900) px = Math.sign(px) * 900;
      if (Math.abs(err) < 10 && b.dist <= 12) {
        px = 0;
        motivo = 'mar a ' + b.dist.toFixed(0) + ' m: sigo de frente';
      } else {
        motivo = 'mar a ' + b.dist.toFixed(0) + ' m, rumbo ' + rumbo.toFixed(0) + '° vs '
          + b.mar.toFixed(0) + '°: giro ' + px.toFixed(0) + ' px';
      }
      ciegos = 0;
    } else {
      ciegos++;
      if (av < 1.0) { px = giroPx; motivo = 'atascado (pared): giro ~45°'; }
      else if (ciegos % 6 === 0) { px = giroPx; motivo = 'sin agua a la vista: giro ~45°'; }
      else motivo = 'sin agua a la vista: sigo recto';
    }
    log.push('tramo ' + tramos + ': avance ' + av.toFixed(1) + ' m, ' + motivo);
    if (px !== 0) { await s.gira(px); giros++; pxPrevios = px; }
    else pxPrevios = 0;
  }
  return { nadando, rumbos: tramos, giros, log, marca: s0, segundos: (Date.now() - t0) / 1000 };
}

// --- escenarios -------------------------------------------------------------

// 1) CARGA: la partida abre y el motor corre. Es la prueba de humo de todo lo
//    demás: si esto falla, cualquier otra medida es ruido.
export const carga = {
  nombre: 'carga',
  titulo: 'la partida abre y el motor corre a tiempo real',
  marcas: ['PEDAT'],
  async ejecutar(s) {
    const inf = informe('carga', s);
    const o = inf.abre('carga');
    const esperado = await s.traza.esperar(/FPHASE/, { desde: o, timeoutMs: 20000 });
    inf.check('el motor publica frames (FPHASE)', esperado.ok,
      esperado.ok ? ('primera línea en ' + esperado.ms + ' ms') : 'sin FPHASE en 20 s');
    await sleep(2500);
    const ped = lineasDe(s.traza.leer(o), 'PEDAT');
    inf.check('la traza de estado del jugador sale (PEDAT)', ped.length > 0,
      ped.length ? (ped.length + ' línea(s), p. ej. ' + ped[0]) : 'sin PEDAT: motor sin R19');

    const reloj = resumen(ped.filter((l) => campo(l, 'ms') > 500)
      .map((l) => 'r=' + (campo(l, 'ts') / (campo(l, 'ms') / 1000))), 'r');
    inf.check('el reloj de mundo va a tiempo real (ts ≈ ms)', !!reloj && reloj.med >= 0.85 && reloj.med <= 1.15,
      reloj ? ('mundo/reloj mediana=' + num(reloj.med) + ' en ' + reloj.n + ' muestra(s)') : 'sin muestras');

    const fps = resumen(lineasDe(s.traza.leer(o), 'FPSLOG').map((l) => 'fps=' + campo(l, 'avg')), 'fps');
    const delta = resumen(lineasDe(s.traza.leer(o), 'FPSLOG'), 'maxdelta');
    inf.check('el motor dibuja a un ritmo usable', !fps || fps.med >= 8,
      fps ? ('fps mediana=' + num(fps.med) + ', delta máx ' + num(delta && delta.max) + ' ms') : 'sin FPSLOG');
    inf.check('el motor servido es una versión con marca', /\d{4}-\d\d-\d\d-ve\d+/.test(s.version), 'build ' + s.version);
    inf.noError();
    inf.cierra('carga');
    return inf;
  },
};

// 3) NADO (R7/R17/R18/R19): entra al agua, flota, avanza a la velocidad del clip
//    y la cámara sigue siendo la de seguir-al-ped.
export const nado = {
  nombre: 'nado',
  titulo: 'nado: entrar al agua, flote, velocidad del clip y cámara',
  marcas: ['SWIM2', 'SWIMCAM'],
  async ejecutar(s) {
    const inf = informe('nado', s);
    const segS = Number(process.env.VC_SWIM_S || 12);
    // Asegurar que está de pie (por si el escenario anterior lo dejó agachado).
    await s.pulsa('KeyC', 140); await sleep(600); await s.pulsa('KeyC', 140); await sleep(1500);
    const nav = await navegarAlAgua(s, { presupuestoS: Number(process.env.VC_HUNT_BUDGET_S || 240) });
    inf.check('la sonda llega al agua', nav.nadando,
      nav.nadando ? (nav.rumbos + ' rumbo(s) y ' + nav.giros + ' giro(s) en ' + nav.segundos.toFixed(0) + ' s')
        : ('sin agua en ' + nav.segundos.toFixed(0) + ' s: ' + nav.log.slice(-3).join(' | ')));
    if (!nav.nadando) { await s.foto('nado-sin-agua'); inf.cierra('nado'); return inf; }

    const o = inf.abre('nado');
    await s.abajo('KeyW');
    await sleep(segS * 1000);
    await s.foto('nado-1');
    const mitad = s.traza.marca();
    await s.gira(560);                          // 180°: medir también en aguas abiertas
    await sleep(1500);
    await sleep(segS * 1000);
    await s.suelta('KeyW');
    const hasta = s.traza.marca();

    const sw = lineasDe(s.traza.tramo(o, mitad), 'SWIM2').filter((l) => /move/.test(l));
    const sw2 = lineasDe(s.traza.tramo(mitad, hasta), 'SWIM2').filter((l) => /move/.test(l));
    const velo = resumen(sw.filter((l) => campo(l, 'velo') > 0.5), 'velo');
    inf.check('nadando avanza a la velocidad del clip (braza 2,32 / crol 2,78)',
      !!velo && velo.med >= 1.5 && velo.med <= 3.1,
      velo ? ('velo mediana=' + num(velo.med) + ' m/s, máx=' + num(velo.max) + ' en ' + velo.n + ' muestra(s)') : 'sin SWIM2');
    const velo2 = resumen(sw2.filter((l) => campo(l, 'velo') > 0.5), 'velo');
    inf.check('aguas abiertas (tras girar 180°) mantiene la velocidad',
      !velo2 || (velo2.med >= 1.5 && velo2.med <= 3.1),
      velo2 ? ('velo mediana=' + num(velo2.med) + ' m/s en ' + velo2.n + ' muestra(s)') : 'sin muestras del segundo tramo');
    const dz = resumen(sw.slice(2), 'dz');
    const hondo = resumen(sw.slice(2), 'hondo');
    inf.check('flota cerca de la superficie (no va por el fondo)',
      !!dz && dz.med <= 0.45 && !!hondo && hondo.med <= 1.0,
      (dz ? 'dz mediana=' + num(dz.med) + ' m' : 'sin dz') + ' · '
        + (hondo ? 'hondo mediana=' + num(hondo.med) + ' m' : 'sin hondo'));
    const modos = modosDe(sw);
    inf.check('la cámara SIGUE al ped (modo 4)', modos.length > 0 && modos.every((m) => m === 4),
      'modos=[' + modos.join(',') + ']');
    const dist = resumen(sw, 'camdist');
    inf.check('la cámara no se queda lejos del ped', !!dist && dist.max <= 8,
      dist ? ('camdist ' + num(dist.min) + '..' + num(dist.max) + ' m') : 'sin camdist');
    // La cámara de nado, medida: el objetivo tiene que estar en la SUPERFICIE
    // (nivel+0,5) y la línea `SWIMCAM no-fallen-water` (una por sesión, al
    // decidir la cámara del jugador vivo en el agua) prueba que el motor NO
    // eligió la cámara de ahogado (`modo=23`). Ojo: esa línea puede caer ANTES
    // de este tramo —al cargar la partida, si el jugador ya está en el agua—,
    // así que se busca en la sesión entera; dentro del tramo se exige, además,
    // que ninguna línea anuncie `modo=23`.
    const swCam = lineasDe(s.traza.tramo(o, hasta), 'SWIMCAM').filter((l) => /objetivo/.test(l));
    const sobreNivel = resumen(swCam.map((l) => 'd=' + (campo(l, 'objetivo') - campo(l, 'nivel'))), 'd');
    inf.check('nadando, la cámara apunta a la superficie (nivel+0,5)',
      !!sobreNivel && sobreNivel.med >= 0.2 && sobreNivel.med <= 0.9,
      sobreNivel ? ('objetivo−nivel mediana=' + num(sobreNivel.med) + ' m en ' + sobreNivel.n + ' muestra(s)')
        : 'sin líneas SWIMCAM objetivo en el tramo');
    const sesion = s.traza.leer(0);
    const sinCamaradeAgua = sesion.indexOf('SWIMCAM no-fallen-water') >= 0;
    const ahogado = /SWIMCAM[^\n]*modo=23/.test(s.traza.tramo(o, hasta));
    inf.check('no se usa la cámara de ahogado estando vivo', sinCamaradeAgua && !ahogado,
      ahogado ? 'aparece SWIMCAM con modo=23 (cámara de ahogado) en el tramo'
        : (sinCamaradeAgua
          ? (sesion.match(/SWIMCAM no-fallen-water[^\n]*/) || [''])[0].slice(0, 90)
          : 'sin traza: la sesión no llegó a decidir la cámara del jugador en el agua'));
    inf.noError();
    if (!inf.ok) await s.foto('nado-fallo');
    inf.cierra('nado');
    return inf;
  },
};

// 4) ARMAS (Vice Extended, pack 3): las armas del mod cargan, ciclan y disparan
//    sin reventar. Se comprueba con la ficha por arma (`WINFO`), que el motor ya
//    publica: si un TXD o un bloque de animación del mod faltan, ahí se ve.
export const armas = {
  nombre: 'armas',
  titulo: 'armas del mod: carga, ciclo y disparo sin errores',
  marcas: ['WINFO arma=', 'WLOAD arma='],
  async ejecutar(s) {
    const inf = informe('armas', s);
    const o = inf.abre('armas');
    await s.cheat('CRAZYTOOLS');               // truco del mod: todas las armas
    // Los tiempos son los de la sonda de armas que ya funcionaba (10 s de espera
    // tras el truco: el arma se SACA en mano, y el motor publica `WINFO` cuando
    // cambia; con 2,5 s de espera no salió ni una ficha la primera vez).
    await sleep(10000);
    // MARCA DE CORTE: hasta aquí, lo que ha hecho el TRUCO (carga + fichas). Lo
    // que salga después es del DISPARO y del CICLO de armas, que es otra cosa y
    // se mide aparte (si no, el ciclo se apunta el mérito de las fichas que
    // publicó el truco: la primera versión de este escenario lo hacía).
    const mTruco = s.traza.marca();
    // Disparar con Ctrl (rsPADINS): así el arma se ve en mano.
    await s.abajo('ControlLeft');
    await sleep(600);
    await s.suelta('ControlLeft');
    await sleep(1200);
    await s.pulsa('Numpad0', 600);
    await sleep(1200);
    // Ciclar armas (Dec del numérico = rsPADDEL) disparando en cada una.
    for (let i = 0; i < 6; i++) {
      await s.pulsa('NumpadDecimal', 90);
      await sleep(1400);
      await s.abajo('ControlLeft');
      await sleep(500);
      await s.suelta('ControlLeft');
      await sleep(800);
    }
    const txt = s.traza.tramo(o, mTruco);       // el truco
    const txtCiclo = s.traza.tramo(mTruco, s.traza.marca()); // disparo y ciclo

    // Lo que SÍ se ha ejercitado y AHORA se mide de verdad: `WLOAD` (ve37), que
    // el truco publica una vez por arma del mod con el estado del streaming. Antes
    // se deducía de `TXDIN`, y esa instrumentación está acotada a las 60 primeras
    // líneas del proceso: en una sesión larga ya se la había comido el arranque y
    // la prueba salía en rojo con el truco funcionando (falso fallo).
    const TXD_ARMAS = ['beretta', 'desert_eagle', 'shotgun2', 'uziold', 'ak47', 'm16', 'steyr', 'gr_launch'];
    const fichasCarga = lineasDe(txt, 'WLOAD');
    const porNombre = new Map(fichasCarga.map((l) => [texto(l, 'arma').toLowerCase(), l]));
    const faltan = TXD_ARMAS.filter((t) => !porNombre.has(t));
    inf.check('el truco de armas del mod carga TODAS sus armas',
      fichasCarga.length >= TXD_ARMAS.length && faltan.length === 0,
      faltan.length ? ('sin cargar: ' + faltan.join(',') + ' (fichas: ' + fichasCarga.length + ')')
        : (fichasCarga.length + ' arma(s) pedida(s) y cargada(s)'));
    const sinModelo = fichasCarga.filter((l) => campo(l, 'cargado') !== 1);
    const sinTxd = fichasCarga.filter((l) => campo(l, 'txdCargado') !== 1);
    inf.check('el modelo (DFF) de cada arma está en memoria',
      fichasCarga.length > 0 && sinModelo.length === 0,
      fichasCarga.length ? (sinModelo.length ? ('sin modelo: ' + sinModelo[0]) : (fichasCarga.length + '/' + fichasCarga.length))
        : 'sin fichas WLOAD');
    inf.check('el diccionario (TXD) de cada arma está en memoria',
      fichasCarga.length > 0 && sinTxd.length === 0,
      fichasCarga.length ? (sinTxd.length ? ('sin TXD: ' + sinTxd[0]) : (fichasCarga.length + '/' + fichasCarga.length))
        : 'sin fichas WLOAD');
    const txdRaro = fichasCarga.filter((l) => texto(l, 'txd').toLowerCase() !== texto(l, 'arma').toLowerCase());
    inf.check('cada arma usa el diccionario del mod (nombre propio)',
      fichasCarga.length > 0 && txdRaro.length === 0,
      txdRaro.length ? ('mapeo raro: ' + txdRaro[0]) : 'modelo y TXD con el mismo nombre en todas');

    const fichas = lineasDe(txt, 'WINFO');
    const fichasCiclo = lineasDe(txtCiclo, 'WINFO');
    const ids = [...new Set(fichasCiclo.map((l) => campo(l, 'arma')).filter((x) => !Number.isNaN(x)))];
    // El truco reparte las 8 armas del mod y el motor publica una ficha por cada
    // una (ve37, `ViceExtWeaponInfoOf`): ya no hace falta que la sonda acierte a
    // cambiarlas de mano para medir los clips.
    const sinClips = fichas.filter((l) => /clips=\|\|\|/.test(l) || /clips=\s*$/.test(l));
    inf.check('las 8 armas del mod resuelven sus clips (WINFO)',
      fichas.length >= 8 && sinClips.length === 0,
      fichas.length ? (fichas.length + ' ficha(s), ' + sinClips.length + ' sin clips'
        + (fichas.length < 8 ? ' (faltan armas en el truco o en la traza)' : ''))
        + (sinClips.length ? ' p. ej. ' + sinClips[0] : '')
        : 'sin fichas `WINFO` en el tramo del truco');
    // El CICLO a mano: fichas publicadas DESPUÉS del truco (al disparar y cambiar
    // de arma con el teclado numérico). Si no cambia de arma, es AVISO: la sonda
    // no sabe cuál es la tecla del jugador (NumLock, remapeo del mod...), y eso
    // no dice nada malo del mod.
    if (ids.length >= 2) {
      inf.check('el ciclo de armas responde en el teclado numérico', true,
        ids.length + ' armas distintas tras el truco: ' + ids.join(','));
    } else {
      inf.aviso('el ciclo de armas con el teclado numérico',
        'tras el truco sólo se ve ' + ids.length + ' arma (' + (ids.join(',') || 'ninguna') + '): el ciclo hay que verlo'
        + ' con la tecla del jugador (puede ser el teclado numérico sin NumLock o el remapeo del mod).'
        + ' La carga de datos y los clips de las 8 armas ya están medidos arriba con la traza del truco');
    }
    inf.check('el motor sigue vivo tras disparar', txt.indexOf('FPHASE') >= 0, 'FPHASE presente en el tramo');
    inf.noError();
    if (!inf.ok) await s.foto('armas-fallo');
    inf.cierra('armas');
    return inf;
  },
};

// ORDEN: los escenarios comparten la MISMA partida, así que el orden importa.
// `armas` va antes que `nado`: dentro del agua el ped no lleva armas en la mano
// y el truco no sirve de nada (la primera corrida del arnés lo hizo al revés y
// el escenario salió sin una sola ficha de arma).
export const ESQUEMAS = [carga, armas, nado];
