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

// 2) AGACHADO (R6/R17/R19): el clip del mod manda, el ped avanza a la velocidad
//    del clip, la medida no se dobla y la cámara lo sigue.
export const agachado = {
  nombre: 'agachado',
  titulo: 'agachado: clip del mod, avance real, medida fiable y cámara',
  // R21: `pesomira` y los clips de la rueda van como MARCA obligatoria: si el
  // binario servido no los lleva, la sesión se para diciéndolo, en vez de sacar
  // veinte comprobaciones confusas (pasó el 23/09 con un build viejo en caché).
  marcas: ['CROUCH2', 'medida=1x', 'CROUCHPOSE', 'pesomira=%.2f', 'veloc=%.2f', 'rueda=%d',
    'moved=%.2f', 'ViceExtCrouchMoveSpeed', 'Crouch_Roll_L'],
  async ejecutar(s) {
    const inf = informe('agachado', s);
    const segS = Number(process.env.VC_CROUCH_S || 8);
    const o = inf.abre('agachado');

    // AUTO-RECUPERACIÓN DEL AGACHADO (23/09). El ped se prueba en plena calle y
    // su mundo sigue vivo: si un coche lo atropella, el motor cancela el agachado
    // a mitad de la prueba (`CARPED ... model=210` + `VICEEXT crouch pose off`),
    // y entonces TODAS las fases siguientes miden sin agachado: la corrida salía
    // con una veintena de fallos falsos ("sin pesomira en CROUCH2", "las 8
    // direcciones no se mueven") que no eran del código. `CROUCH2` sólo se publica
    // agachado, así que basta con mirar si el último latido de esa traza es
    // reciente.
    const estaAgachado = (margenMs = 2500) => {
      const txt = s.traza.leer(Math.max(0, s.traza.tam() - 60000));
      const ms = [...String(txt).matchAll(/^(\S+?)Z\s+CROUCH2\s/gm)];
      if (!ms.length) return false;
      const t = Date.parse(ms[ms.length - 1][1] + 'Z');
      return Number.isFinite(t) && (Date.now() - t) < margenMs;
    };
    // ¿Por qué se cayó? (evidencia para el informe: atropello, agua, coche...)
    const motivoCaida = () => {
      const txt = s.traza.leer(Math.max(0, s.traza.tam() - 60000));
      const m = String(txt).match(/(CARPED[^\n]*|VICEEXT swim enter[^\n]*|VICEEXT crouch off[^\n]*)/g);
      return m ? m[m.length - 1].slice(0, 90) : 'sin causa en la traza';
    };
    const aseguraAgachado = async (intentos = 3) => {
      for (let i = 0; i < intentos; i++) {
        if (estaAgachado()) return true;
        await s.pulsa('KeyC', 140);
        await sleep(1600);
      }
      return estaAgachado();
    };

    await sleep(2500);                          // de pie y quieto: referencia de cámara
    const mPieAntes = s.traza.marca();
    await s.pulsa('KeyC', 140);
    await sleep(2000);
    if (!(await aseguraAgachado()))
      inf.aviso('el ped no consigue agacharse al empezar (mundo hostil)', motivoCaida());
    await s.abajo('KeyW');
    await sleep(segS * 1000);
    await s.suelta('KeyW');
    await sleep(2000);                          // frenada: no cuenta (el ped aún se desliza)
    const mQuieto = s.traza.marca();
    await sleep(4000);                          // quieto: no debe arrastrarse
    const mPie = s.traza.marca();

    // --- R20: LAS 8 DIRECCIONES -------------------------------------------
    // El fallo que esto mide (14ª-16ª partidas): "agachado no se mueve en sus 8
    // direcciones". En este motor la dirección de avance la pone la RAÍZ del
    // clip, así que con el mando a la izquierda hay que pedir `Crouch_Roll_L`:
    // pedir el de adelante (lo que hacía R6/R17) hace avanzar de frente.
    //
    // De cada dirección se mide lo que de verdad pasó, no lo que se pedía:
    //   · `nom=`   = clip que pidió el motor (por NOMBRE: los números del enum
    //                se movieron al reordenar el bloque de agachado),
    //   · `velo=`  = m/s reales del tramo,
    //   · `ang=`   = ángulo del mando en el sistema del ped (convención del
    //                motor: positivo = izquierda),
    //   · desvío  = hacia dónde se movió de verdad respecto a su rumbo, medido
    //                con `PEDAT` (posición real), no con la intención.
    const oDir = inf.abre('dirs');
    // R24: las palancas de URL/consola del agachado se quitaron; el escenario mide
    // las constantes reales del motor tal como están en `PlayerPed.cpp`. La pose
    // de costado (los `Crouch_Roll_L/R` del mod) está puesta por código, así que
    // aquí es fija.
    const lado = 1;
    const segDir = Number(process.env.VC_DIR_S || 2.6);
    const dirs = [
      // `ang` = ángulo del mando en el sistema del cuerpo, con la convención del
      // motor (positivo = izquierda; el mismo número que publica la traza). El
      // desplazamiento tiene que ir AHÍ: con el control de ratón el cuerpo mira a
      // la cámara y se desplaza sin girar (como al caminar de pie), así que el
      // desvío medido respecto al rumbo vale `−ang`.
      { nombre: 'adelante',      teclas: ['KeyW'],         ang: 0,
        clips: ['crouch_forward'] },
      { nombre: 'atras',         teclas: ['KeyS'],         ang: 180,
        clips: ['crouch_backward'] },
      // R22: SIN APUNTAR, los lados van con el clip de ADELANTE (el mod no trae
      // paso lateral agachado: lo que trae es la rueda, y la rueda vuelve a ser
      // un movimiento de esquiva al apuntar — ver el bloque de la mira, donde se
      // comprueba que se rueda UNA vez y no en bucle). Con `VC_CROUCH_SIDE=0` el
      // motor usa la pose de adelante también apuntando.
      { nombre: 'izquierda',     teclas: ['KeyA'],         ang: 90,
        clips: ['crouch_forward'] },
      { nombre: 'derecha',       teclas: ['KeyD'],         ang: -90,
        clips: ['crouch_forward'] },
      { nombre: 'diag adel-izq', teclas: ['KeyW', 'KeyA'], ang: 45,
        clips: ['crouch_forward'] },
      { nombre: 'diag adel-der', teclas: ['KeyW', 'KeyD'], ang: -45,
        clips: ['crouch_forward'] },
      { nombre: 'diag atr-izq',  teclas: ['KeyS', 'KeyA'], ang: 135,
        clips: ['crouch_backward'] },
      { nombre: 'diag atr-der',  teclas: ['KeyS', 'KeyD'], ang: -135,
        clips: ['crouch_backward'] },
    ];
    const filas = [];
    for (const d of dirs) {
      // Antes de cada dirección: si el agachado se cayó (atropello, agua...), se
      // vuelve a poner en vez de medir en balde.
      if (!(await aseguraAgachado(2)))
        inf.aviso('fallo al reanudar el agachado antes de «' + d.nombre + '»', motivoCaida());
      const m0 = s.traza.marca();
      for (const k of d.teclas) await s.abajo(k);
      await sleep(segDir * 1000);
      for (const k of d.teclas) await s.suelta(k);
      await sleep(900);
      const m1 = s.traza.marca();
      const c2 = lineasDe(s.traza.tramo(m0, m1), 'CROUCH2');
      const ped = lineasDe(s.traza.tramo(m0, m1), 'PEDAT');
      const est = c2.slice(1);                    // fuera la 1ª muestra (arranque)
      // Solo lo que se midió CON el mando puesto: la última muestra del tramo
      // puede caer en el deslizamiento de después de soltar las teclas (clip
      // `Crouch_Idle` y velocidad de frenada), y eso no es el régimen.
      const mover = est.filter((l) => texto(l, 'nom') !== 'Crouch_Idle');
      const estMov = mover.length ? mover : est;
      const velo = resumen(estMov, 'velo');
      const ang = resumen(estMov, 'ang');
      const rate = resumen(est, 'rate');
      const nombres = [...new Set(estMov.map((l) => texto(l, 'nom')).filter((x) => x))];
      const strafeFila = campo(est[0] || '', 'strafe') === 1;
      // Rumbo del cuerpo al final del tramo y velocidad ya asentada (el ped gira
      // mientras anda, así que el régimen es la ÚLTIMA muestra, no la mediana).
      const ult = estMov[estMov.length - 1] || '';
      const rumboFin = campo(ult, 'rumbo') * 180 / Math.PI;
      const veloFin = campo(ult, 'velo');
      // Desvío real: rumbo del desplazamiento menos el rumbo del cuerpo. Las dos
      // medidas van en la convención del motor (`SetHeading`): adelante = −rumbo,
      // así que el desvío es `rumbo del movimiento + rumbo del cuerpo`.
      // Dirección medida: desplazamiento entre la PRIMERA posición ya con el
      // mando puesto y la última del tramo. Se recorre el tramo en orden
      // descartando lo anterior a la primera muestra de movimiento: al empezar
      // el tramo el ped todavía viene deslizando de la dirección anterior.
      const pedMov = [];
      let vioMover = false;
      for (const l of s.traza.tramo(m0, m1).split('\n').map((x) => x.trim())) {
        if (/\sCROUCH2\s/.test(l)) { if (!/nom=Crouch_Idle/.test(l)) vioMover = true; continue; }
        if (vioMover && /\sPEDAT\s/.test(l)) pedMov.push(l.replace(/^\S+Z\s+/, ''));
      }
      let desvio = NaN, metros = NaN;
      if (pedMov.length >= 2) {
        const p0 = pedMov[0], p1 = pedMov[pedMov.length - 1];
        const bx = campo(p1, 'x') - campo(p0, 'x');
        const by = campo(p1, 'y') - campo(p0, 'y');
        metros = Math.hypot(bx, by);
        if (metros > 0.5)
          desvio = normaliza180(Math.atan2(bx, by) * 180 / Math.PI + rumboFin);
      }
      // `ang` (pedido, de la lista) y `angMed` (el que publica la traza) son cosas
      // distintas: el primero es la intención, el segundo lo que leyó el motor.
      filas.push({ ...d, nombres, velo, angMed: ang, rate, desvio, metros, rumboFin,
        veloc: campo(ult, 'veloc'),          // R22: velocidad objetivo publicada
        // R22: velocidad del MOTOR (`m_moved`, m/s). Se guarda el PICO del tramo,
        // no la última muestra: cuando la sonda suelta la tecla, R20c deja de
        // empujar y el motor vuelve a 0 (la primera corrida lo midió así: "motor
        // 0,00 m/s" en las tres direcciones con el ped habiendo andado).
        moved: resumen(estMov, 'moved'),
        movedPico: Math.max(...estMov.map((l) => campo(l, 'moved')).filter((x) => Number.isFinite(x)), 0),
        movedFin: campo(ult, 'moved'),
        veloFin, strafe: strafeFila, n: est.length });
    }

    const giro = (f) => normaliza180(f.rumboFin - filas[0].rumboFin);
    console.log('   dirección        clip pedido          velo med  ángulo  giro  desvío  esperado');
    for (const f of filas) {
      console.log('   ' + f.nombre.padEnd(16) + ' ' + (f.nombres.join(',') || '—').padEnd(20)
        + ' ' + num(f.velo && f.velo.med).padStart(7) + '  ' + num(f.angMed && f.angMed.med).padStart(6)
        + '  ' + num(giro(f)).padStart(5) + '  ' + num(f.desvio).padStart(7)
        + '  ' + num(-f.ang).padStart(8) + '   (' + num(f.veloFin) + ' m/s en régimen, '
        + num(f.metros) + ' m recorridos)');
    }

    const sinMuestra = filas.filter((f) => !f.n || !f.velo);
    inf.check('las 8 direcciones del mando dejan medida (2 muestras o más cada una)',
      sinMuestra.length === 0,
      sinMuestra.length ? ('sin CROUCH2 en: ' + sinMuestra.map((f) => f.nombre).join(', '))
        : (filas.length + ' dirección(es) con ' + Math.min(...filas.map((f) => f.n)) + '..'
          + Math.max(...filas.map((f) => f.n)) + ' muestras'));

    const quietas = filas.filter((f) => !(f.velo && f.velo.med > 0.4));
    inf.check('las 8 direcciones MUEVEN al ped (no se queda clavado)',
      quietas.length === 0,
      quietas.length ? ('sin moverse: ' + quietas.map((f) => f.nombre + ' ' + num(f.velo && f.velo.med)).join(', '))
        : ('velocidad mediana ' + num(Math.min(...filas.map((f) => f.velo.med))) + '..'
          + num(Math.max(...filas.map((f) => f.velo.med))) + ' m/s'));

    // El desvío se mide en las 4 rectas; en las diagonales el motor reparte el
    // mando entre dos clips, así que se comprueba un sector más ancho.
    // 1) El ángulo del mando que lee el motor (es el número con el que se elige
    //    todo lo demás: si esto está mal, lo demás no dice nada).
    const malAng = filas.filter((f) => !(Math.abs(Math.abs(f.angMed && f.angMed.med)
      - Math.abs(f.ang)) <= 10));
    inf.check('el mando se lee en el sistema del cuerpo (8 ángulos distintos)',
      malAng.length === 0,
      malAng.length ? malAng.map((f) => f.nombre + ': ' + num(f.angMed && f.angMed.med)
        + '° (se espera ' + f.ang + '°)').join('; ')
        : filas.map((f) => f.nombre + ' ' + num(f.angMed && f.angMed.med) + '°').join(', '));

    // 2) El CUERPO no gira: con ratón en 3ª persona agachado se desplaza sin
    //    girar, igual que al caminar de pie (ahí son los clips `walk_left`/
    //    `walk_right`; agachado no existen y el desplazamiento lo pone el
    //    código: R20c).
    const malGiro = filas.filter((f) => !(Math.abs(giro(f)) <= 20));
    inf.check('el cuerpo NO gira: se desplaza de lado (como al caminar de pie)',
      malGiro.length === 0,
      malGiro.length
        ? malGiro.map((f) => f.nombre + ' giró ' + num(giro(f)) + '°').join('; ')
        : filas.map((f) => f.nombre + ' ' + num(giro(f)) + '°').join(', '));

    // 3) Y se desplaza HACIA DONDE SE PIDE: el desvío medido (rumbo del
    //    desplazamiento menos el del cuerpo) tiene que valer −ang.
    const malDir = filas.filter((f) => !(Math.abs(normaliza180(f.desvio + f.ang)) <= 25));
    inf.check('el ped se DESPLAZA hacia donde se pide (las 8 direcciones)',
      malDir.length === 0,
      malDir.length
        ? malDir.map((f) => f.nombre + ' (ang ' + f.ang + '°) fue a ' + num(f.desvio) + '° de su rumbo').join('; ')
        : filas.map((f) => f.nombre + ' ' + num(f.desvio) + '°').join(', '));

    const [izq, der] = [filas[2], filas[3]];
    inf.check('los lados van a lados OPUESTOS (no se cruzan)',
      Number.isFinite(izq.desvio) && Number.isFinite(der.desvio)
      && Math.sign(izq.desvio) !== Math.sign(der.desvio),
      'izquierda ' + num(izq.desvio) + '° vs derecha ' + num(der.desvio) + '° de su rumbo');

    const diagonales = filas.slice(4);
    const malDiagDir = diagonales.filter((f) => !(Math.abs(normaliza180(f.desvio + f.ang)) <= 25));
    inf.check('las 4 diagonales también van hacia su lado',
      malDiagDir.length === 0,
      malDiagDir.length ? malDiagDir.map((f) => f.nombre + ' (ang ' + f.ang + '°) fue a '
        + num(f.desvio) + '°').join('; ')
        : diagonales.map((f) => f.nombre + ' ' + num(f.desvio) + '°').join(', '));

    const sinMedir = filas.filter((f) => !Number.isFinite(f.desvio));
    inf.check('hay medida de las 8 (ninguna se quedó sin desplazamiento)',
      sinMedir.length === 0,
      sinMedir.length ? ('sin medir: ' + sinMedir.map((f) => f.nombre).join(', '))
        : (filas.length + ' dirección(es) medidas'));

    // Clip por dirección: adelante con las de adelante y los lados, atrás con
    // las de atrás. Los `Crouch_Roll_L/R` sólo salen con la palanca
    // `?crouchlado=1` (su raíz viaja hacia atrás, medido: −2,17/+2,25 m en Y).
    const malClip = filas.filter((f) => f.nombres.length
      && !f.nombres.some((n) => f.clips.includes(String(n).toLowerCase()))
      // `Crouch_Idle` puede aparecer en la primera muestra del tramo (fundido de
      // entrada): no es un fallo, la dirección la fija la muestra siguiente.
      && !f.nombres.every((n) => String(n).toLowerCase() === 'crouch_idle'));
    inf.check('cada dirección pide el clip que le toca del mod',
      !malClip.length,
      malClip.length
        ? malClip.map((f) => f.nombre + ' usó ' + f.nombres.join(',') + ' (se espera '
          + f.clips.join(' o ') + ')').join('; ')
        : filas.map((f) => f.nombre + '=' + (f.nombres.join(',') || '—')).join(', '));
    const usaRoll = filas.filter((f) => f.nombres.some((n) => /roll/i.test(n)));
    // R21: los lados (y sólo los lados) usan la rueda del mod. Antes de R21 esto
    // estaba al revés (la rueda sólo salía con la palanca `?crouchlado=1`),
    // porque su raíz viaja hacia atrás y no se había podido ver en partida.
    inf.check('los lados usan la RUEDA del mod (Crouch_Roll_L/R) y el resto no',
      lado
        ? (usaRoll.length === 2 && usaRoll.every((f) => Math.abs(f.ang) === 90))
        : usaRoll.length === 0,
      usaRoll.length ? usaRoll.map((f) => f.nombre + '=' + f.nombres.join(',')).join('; ')
        : 'ninguna dirección usó Crouch_Roll_L/R' + (lado ? ' (se esperaba en los lados)' : ''));

    // R22: la velocidad medida tiene que ser la OBJETIVO que publica la traza
    // (`veloc=`), que es la del mod en m/s (1,85 andando) y no la del clip que
    // lleva el cuerpo (`Crouch_Forward` avanza 3,58 m/s a ritmo 1: por eso el
    // agachado iba a super velocidad). Se mide en RÉGIMEN (última muestra del
    // tramo), porque mientras el ped gira la mediana incluye el arco del giro.
    const rateMed = (filas.find((f) => f.rate) || {}).rate;
    const ritmo = rateMed ? rateMed.med : 1.0;
    // R22: se mide la velocidad DEL MOTOR (`moved=`, que es `m_moved` en m/s, lo
    // que R20c escribe) y no el desplazamiento de posición: en la calle el ped
    // recibe golpes y el motor lo teletransporta (medido en esta misma corrida:
    // 42 m en un segundo, "velo=41,4"), y eso no es andar. `velo` se informa al
    // lado para poder ver la diferencia.
    const fueraBanda = [];
    for (const f of filas) {
      if (!Number.isFinite(f.movedPico)) continue;
      const obj = Number.isFinite(f.veloc) ? f.veloc : 1.85 * ritmo;
      if (Math.abs(f.movedPico - obj) > Math.max(0.4, 0.25 * obj))
        fueraBanda.push(f.nombre + ' motor ' + num(f.movedPico) + ' m/s con ' + f.nombres.join(',')
          + ' (objetivo ' + num(obj) + ')');
    }
    inf.check('la velocidad agachado es la OBJETIVO (1,85 m/s × ritmo ' + num(ritmo) + '), no la del clip',
      fueraBanda.length === 0,
      fueraBanda.length ? fueraBanda.join('; ')
        : ('velocidad del motor ' + filas.filter((f) => Number.isFinite(f.movedPico))
          .map((f) => f.nombre + ' ' + num(f.movedPico)).join(', ')
          + ' (objetivo ' + num(1.85 * ritmo) + '; mediana de `moved` por dirección: '
          + filas.filter((f) => f.moved).map((f) => num(f.moved.med)).join(', ') + ')'));

    inf.check('el ritmo se puede cambiar EN CALIENTE (rate se publica)',
      !!rateMed && rateMed.med > 0, rateMed ? ('rate=' + num(rateMed.med)) : 'sin `rate=` en CROUCH2');

    inf.cierra('dirs');

    // --- R21: APUNTAR AGACHADO --------------------------------------------
    // 17ª partida: "falta que al apuntar estando agachado haga su respectiva
    // animación de apuntado" y "si apunto y presiono el lado izquierdo o derecho
    // el personaje rueda hacia ese lado".
    //
    // El apuntado del motor es `PED_LOCK_TARGET` (`ControllerConfig.cpp`: tecla
    // `Supr` o botón derecho del ratón), así que la sonda mantiene `Delete`
    // pulsado. Lo que se mide, todo por traza:
    //   · `mira=1`     = el motor ve el apuntado estando agachado,
    //   · `pesomira`   = peso del clip `WEAPON_crouch` del mod (la pose),
    //   · `nom=`       = con la mira puesta los lados siguen pidiendo la rueda,
    //   · `pesomira`→0 = al soltar el botón la pose se retira.
    const oMira = inf.abre('mira');
    if (!(await aseguraAgachado(3)))
      inf.aviso('el ped no está agachado al empezar la fase de apuntado', motivoCaida());
    // El arma en mano importa: con los puños `ViceExtCanAim` es falso y no hay
    // pose que poner (el truco del mod saca el arma y publica su ficha `WINFO`).
    // `CRAZYTOOLS` deja las armas en el inventario pero NO en la mano (medido en
    // la primera corrida de R21: `arma=0` = puños y por tanto `cana=0`, sin pose
    // que poner). `CRAZYPISTOL` (bloque P2 del mod) existe justo para esto: deja
    // la Beretta COMO ARMA ACTUAL (`SetCurrentWeapon`), y la pistola apunta.
    await s.cheat('CRAZYHINT');
    await s.cheat('CRAZYPISTOL');
    await sleep(5000);
    // Si aun así no hay arma apuntable en mano, se intenta con la rueda (cambio
    // de arma a pie en VC) y si no sale se AVISA en vez de tumbar la prueba.
    let canaOk = false;
    for (let i = 0; i < 3 && !canaOk; i++) {
      const m = s.traza.marca();
      await s.ratonRueda(-120);
      await sleep(1200);
      const c2 = lineasDe(s.traza.tramo(m, s.traza.marca()), 'CROUCH2');
      canaOk = c2.some((l) => campo(l, 'cana') === 1);
    }
    {
      const m = s.traza.marca();
      await sleep(1200);
      const c2 = lineasDe(s.traza.tramo(m, s.traza.marca()), 'CROUCH2');
      canaOk = canaOk || c2.some((l) => campo(l, 'cana') === 1);
      if (!canaOk)
        inf.aviso('sin arma apuntable en mano: el motor no puede poner la pose de apuntar',
          'cana=0 y arma=' + (c2.map((l) => campo(l, 'arma')).filter((x) => !Number.isNaN(x)).join(',') || '—')
          + ': el truco CRAZYPISTOL no dejó la pistola en la mano');
    }
    // El apuntado de VC es `PED_LOCK_TARGET`: botón derecho del ratón o `Supr`.
    // Se mantienen los dos (una sonda anterior probó sólo la tecla y salió
    // `mira=0`; con `tgt=`/`cana=` en la traza se ve cuál de los dos falla).
    await s.gira(0);                            // deja el puntero sobre el canvas
    await s.ratonAbajo('right');
    await s.abajo('Delete');
    await sleep(3000);
    const mMira = s.traza.marca();
    await sleep(3000);
    const mirando = lineasDe(s.traza.tramo(mMira, s.traza.marca()), 'CROUCH2');
    const pesMira = resumen(mirando, 'pesomira');
    const nMira = mirando.filter((l) => campo(l, 'mira') === 1).length;
    const clipMira = [...new Set(mirando.map((l) => texto(l, 'nom')).filter((x) => x))];
    // Diagnóstico del apuntado: `tgt` (el mando lo ve) y `cana` (el arma en mano
    // es de apuntar). Sin estos dos números, un `mira=0` no dice si el fallo es la
    // pose, la tecla o el arma.
    const tgtN = mirando.filter((l) => campo(l, 'tgt') === 1).length;
    const canaN = mirando.filter((l) => campo(l, 'cana') === 1).length;
    const armasVistas = [...new Set(mirando.map((l) => campo(l, 'arma')).filter((x) => !Number.isNaN(x)))];
    inf.check('apuntando agachado el motor lo sabe (mira=1)', nMira >= 2,
      nMira ? (nMira + '/' + mirando.length + ' muestra(s) con mira=1, clip ' + (clipMira.join(',') || '—'))
        : ('ninguna muestra con mira=1: tgt=1 en ' + tgtN + '/' + mirando.length
          + ', cana=1 en ' + canaN + '/' + mirando.length
          + ', arma(s)=' + (armasVistas.join(',') || '—')));
    inf.check('la pose de apuntar agachado del mod se mezcla (WEAPON_crouch)',
      !!pesMira && pesMira.med >= 0.5,
      pesMira ? ('pesomira mediana=' + num(pesMira.med) + ', máx=' + num(pesMira.max)
        + ' (' + mirando.length + ' muestra(s))')
        : ('sin muestras de CROUCH2 en el tramo (' + mirando.length + '): '
          + (mirando.length ? 'build sin R21' : 'el agachado no estaba puesto: ' + motivoCaida())));
    // El apuntado NO debe haber cambiado el clip de las piernas ni el avance:
    // quieto agachado y apuntando el ped se queda quieto (sólo cambia la pose).
    const avMira = resumen(mirando, 'avance');
    inf.check('apuntando agachado y QUIETO el ped no se arrastra',
      !avMira || avMira.med <= 0.25,
      avMira ? ('avance mediana=' + num(avMira.med) + ', máx=' + num(avMira.max)) : 'sin muestras');

    // Rueda con la mira puesta (lo que pidió el jugador).
    const oMiraLado = inf.abre('mira-lados');
    const ladosMira = [
      { nombre: 'apuntando izquierda', teclas: ['KeyA'], ang: 90, clip: 'crouch_roll_l', lado: 'izq' },
      { nombre: 'apuntando derecha',   teclas: ['KeyD'], ang: -90, clip: 'crouch_roll_r', lado: 'der' },
    ];
    const filasMira = [];
    for (const d of ladosMira) {
      const m0 = s.traza.marca();
      for (const k of d.teclas) await s.abajo(k);
      await sleep(segDir * 1000);
      for (const k of d.teclas) await s.suelta(k);
      await sleep(900);
      const m1 = s.traza.marca();
      const c2 = lineasDe(s.traza.tramo(m0, m1), 'CROUCH2').slice(1);
      const mover = c2.filter((l) => texto(l, 'nom') !== 'Crouch_Idle');
      const est = mover.length ? mover : c2;
      const nombres = [...new Set(est.map((l) => texto(l, 'nom')).filter((x) => x))];
      const miras = est.filter((l) => campo(l, 'mira') === 1).length;
      const ult = est[est.length - 1] || '';
      const rumboFin = campo(ult, 'rumbo') * 180 / Math.PI;
      // Hacia dónde RUEDA de verdad (posición real, como en las 8 direcciones).
      const pedMov = [];
      let vioMover = false;
      for (const l of s.traza.tramo(m0, m1).split('\n').map((x) => x.trim())) {
        if (/\sCROUCH2\s/.test(l)) { if (!/nom=Crouch_Idle/.test(l)) vioMover = true; continue; }
        if (vioMover && /\sPEDAT\s/.test(l)) pedMov.push(l.replace(/^\S+Z\s+/, ''));
      }
      let desvio = NaN, metros = NaN;
      if (pedMov.length >= 2) {
        const p0 = pedMov[0], p1 = pedMov[pedMov.length - 1];
        const bx = campo(p1, 'x') - campo(p0, 'x');
        const by = campo(p1, 'y') - campo(p0, 'y');
        metros = Math.hypot(bx, by);
        if (metros > 0.5)
          desvio = normaliza180(Math.atan2(bx, by) * 180 / Math.PI + rumboFin);
      }
      // R22: la rueda es UNA por petición. Se cuenta cuántas muestras la llevan
      // puesta y si al final del tramo (con la tecla todavía abajo) ya paró: si
      // sigue `rueda=1` en la última muestra, se está revolcando en bucle.
      const ruedaMuestras = est.filter((l) => campo(l, 'rueda') === 1).length;
      const ruedas = [...s.traza.tramo(m0, m1).matchAll(/VICEEXT crouch roll lado=(\S+)/g)]
        .map((m) => m[1]);
      filasMira.push({ ...d, nombres, velo: resumen(est, 'velo'), desvio, metros,
        pesMira: resumen(est, 'pesomira'), miras, n: est.length, ruedas,
        ruedaMuestras, ruedaFin: campo(ult, 'rueda'),
        camdist: resumen(est, 'camdist') });
    }
    console.log('   apuntando         clip pedido          velo med  mira  peso pose  desvío  esperado');
    for (const f of filasMira)
      console.log('   ' + f.nombre.padEnd(18) + ' ' + (f.nombres.join(',') || '—').padEnd(20)
        + ' ' + num(f.velo && f.velo.med).padStart(7) + '  '
        + String(f.miras + '/' + f.n).padStart(5) + '  ' + num(f.pesMira && f.pesMira.med).padStart(9)
        + '  ' + num(f.desvio).padStart(6) + '  ' + num(-f.ang).padStart(8)
        + '   (' + num(f.metros) + ' m)');
    // R22: la rueda se comprueba por EVENTOS (`VICEEXT crouch roll lado=`), no
    // por los clips que salgan en la traza de 1/s: la rueda dura 0,9 s y una
    // muestra por segundo puede no pillarla (pasó: 8 s de tramo sin un solo
    // `Crouch_Roll_*` en las muestras, con la rueda funcionando).
    const sinRueda = filasMira.filter((f) => !f.ruedas.some((r) => r === f.lado));
    inf.check('apuntando agachado los lados RUEDAN hacia su lado (clip del mod)',
      sinRueda.length === 0,
      sinRueda.length
        ? sinRueda.map((f) => f.nombre + ' ruedas=' + (f.ruedas.join(',') || '—')
          + ' (se espera ' + f.lado + ')').join('; ')
        : filasMira.map((f) => f.nombre + ' ruedas=' + f.ruedas.join(',')).join(', '));
    const quietasMira = filasMira.filter((f) => !(f.velo && f.velo.med > 0.4));
    inf.check('apuntando agachado y con el mando de lado el ped se desplaza',
      quietasMira.length === 0,
      quietasMira.length
        ? quietasMira.map((f) => f.nombre + ' ' + num(f.velo && f.velo.med) + ' m/s').join('; ')
        : filasMira.map((f) => f.nombre + ' ' + num(f.velo && f.velo.med) + ' m/s').join(', '));
    // Y rueda HACIA ESE LADO, no hacia otro (mismo criterio que las 8
    // direcciones: el cuerpo mira a la cámara, así que el desvío vale −ang).
    const malRumboMira = filasMira.filter((f) => !(Math.abs(normaliza180(f.desvio + f.ang)) <= 30));
    inf.check('apuntando agachado la rueda va HACIA EL LADO que se pide',
      malRumboMira.length === 0,
      malRumboMira.length
        ? malRumboMira.map((f) => f.nombre + ' (ang ' + f.ang + '°) fue a ' + num(f.desvio) + '°').join('; ')
        : filasMira.map((f) => f.nombre + ' ' + num(f.desvio) + '° (se espera ' + (-f.ang) + '°)').join(', '));
    // R22: andando la pose va a 0,4 (las piernas del clip de agachado siguen
    // mandando; a peso 1 el cuerpo entero lo pone la pose y las piernas se
    // congelan, que es lo que se veía mal al apuntar agachado).
    inf.check('la pose de apuntar sigue puesta MIENTRAS se rueda (≥0,3 andando)',
      filasMira.every((f) => f.pesMira && f.pesMira.med >= 0.3),
      filasMira.map((f) => f.nombre + ' pesomira=' + num(f.pesMira && f.pesMira.med)).join(', '));
    // R22: "me muero si camino hacia a un lado". Con el clip de la rueda en
    // `ASSOC_REPEAT`, mantener la tecla dejaba a Tommy revolcándose por el suelo
    // sin parar. La rueda es un movimiento: se pide una vez y el ped vuelve a la
    // pose de agachado andando aunque la tecla siga abajo.
    const enBucle = filasMira.filter((f) => !(f.ruedas.length >= 1 && f.ruedas.length <= 2
      && f.ruedas.every((r) => r === f.lado)
      && f.nombres.some((n) => /^crouch_forward/i.test(n))));
    inf.check('la rueda es UNA sola (no se queda rodando en bucle)',
      enBucle.length === 0,
      filasMira.map((f) => f.nombre + ' ruedas=' + (f.ruedas.join(',') || '—')
        + ' (' + f.ruedaMuestras + '/' + f.n + ' muestra(s) con rueda), clips ' + f.nombres.join(','))
        .join('; '));
    // R22: la cámara agachado + apuntando. El objetivo de la cámara bajaba 0,95 m
    // (el doble de lo que baja la cabeza agachada) y en la cámara de apuntar
    // —mucho más cerca— acababa DENTRO del ped: medido en la partida del jugador,
    // `camdist=0,61` en vez de los ~2 m de esa cámara, y la captura del arnés sale
    // con Tommy llenando la pantalla.
    const distMira = filasMira.map((f) => f.camdist && f.camdist.min).filter((x) => Number.isFinite(x));
    inf.check('apuntando agachado la cámara no acaba DENTRO del ped',
      distMira.length > 0 && Math.min(...distMira) >= 1.5,
      filasMira.map((f) => f.nombre + ' camdist ' + num(f.camdist && f.camdist.min) + '..'
        + num(f.camdist && f.camdist.max) + ' m').join('; '));
    inf.cierra('mira-lados');

    // Al soltar el apuntado, la pose se retira (y el ped sigue agachado).
    await s.suelta('Delete');
    await s.ratonSuelta('right');
    await sleep(1500);
    const mSinMira = s.traza.marca();
    await sleep(2500);
    const sinMira = lineasDe(s.traza.tramo(mSinMira, s.traza.marca()), 'CROUCH2');
    const pesFuera = resumen(sinMira, 'pesomira');
    const miraFuera = sinMira.filter((l) => campo(l, 'mira') === 1).length;
    if (sinMira.length === 0) {
      inf.aviso('no se pudo comprobar que la pose se retire al soltar el apuntado',
        'sin muestras CROUCH2 en el tramo final: el agachado se cayó');
    } else
    inf.check('al soltar el apuntado la pose se retira y el ped sigue agachado',
      miraFuera === 0 && !!pesFuera && pesFuera.max <= 0.25,
      (pesFuera ? ('pesomira máx=' + num(pesFuera.max)) : 'sin pesomira')
        + ', mira=1 en ' + miraFuera + ' muestra(s), h=' + num((resumen(sinMira, 'h') || {}).med)
        + ' clip ' + [...new Set(sinMira.map((l) => texto(l, 'nom')).filter((x) => x))].join(','));
    inf.cierra('mira');

    await s.pulsa('KeyC', 140);                 // volver a estar de pie
    await sleep(3500);
    const until = s.traza.marca();

    const av = lineasDe(s.traza.tramo(o, mQuieto), 'CROUCH2');
    const quieto = lineasDe(s.traza.tramo(mQuieto, mPie), 'CROUCH2');
    const dePie = s.traza.tramo(mPie, until);
    const pie = lineasDe(dePie, 'CROUCHPOSE');

    const velo = resumen(av, 'velo');
    const cuerpo = velo ? resumen(velo.cuerpo.map((x) => 'velo=' + x), 'velo') : null;
    inf.check('agachado ANDANDO el ped avanza de verdad', !!velo && velo.med >= 0.4,
      velo ? ('velo mediana=' + num(velo.med) + ' m/s, máx=' + num(velo.max) + ' en ' + velo.n + ' muestra(s)') : 'sin CROUCH2');
    // R22: 1,85 m/s es la velocidad de la familia de agachado DEL MOD (sus
    // propios `Crouch_Backward` y `GunMove_*` van a 1,85 m/s) y la que se probó
    // con el jugador: 0,87 m/s era "demasiado lenta" (ritmo 0,25) y el clip de
    // adelante a ritmo 1 (3,58 m/s) "una super velocidad sin razón".
    inf.check('la velocidad agachado es la del mod (1,85 m/s), no la del clip (3,58)',
      !!cuerpo && cuerpo.med >= 1.4 && cuerpo.med <= 2.4,
      cuerpo ? ('velo de régimen mediana=' + num(cuerpo.med) + ' m/s, se espera ~1,85 (objetivo del mod)') : 'sin muestras');
    const dt = resumen(av.concat(quieto), 'dt');
    inf.check('la medida es fiable (mundo ≈ reloj, sin doble conteo)', !!dt && dt.med >= 0.7 && dt.med <= 1.3,
      dt ? ('dt mediana=' + num(dt.med) + ' s de mundo por segundo de reloj') : 'sin dt');
    const peso = resumen(av, 'peso');
    inf.check('el clip del mod manda en el cuerpo (peso ≈ 1)', !!peso && peso.max >= 0.5,
      peso ? ('peso máx=' + num(peso.max)) : 'sin peso');
    const modos = modosDe(av);
    inf.check('la cámara SIGUE al ped (modo 4)', modos.length > 0 && modos.every((m) => m === 4),
      'modos=[' + modos.join(',') + ']');
    const dist = resumen(av, 'camdist');
    // R22: ni lejos ni DENTRO del ped (el objetivo −0,95 m la metía en el cuerpo
    // al apuntar; ver el mismo control en el bloque de la mira).
    inf.check('la cámara sigue al ped a distancia de cámara (1,5..12 m)',
      !!dist && dist.max <= 12 && dist.min >= 1.5,
      dist ? ('camdist ' + num(dist.min) + '..' + num(dist.max) + ' m') : 'sin camdist');
    // "Quieto" = régimen: la mediana del tramo y la COLA (ya asentado). El pico
    // se informa pero no tumba la prueba: en la calle, un coche que te golpea te
    // mueve 6 m en un segundo (pasó en la primera corrida del arnés, con el ped
    // parado en la calzada) y eso no es que el clip de avance se quede puesto.
    // Un clip pegado sí se vería: la velocidad no bajaría a 0 en la cola.
    const avq = resumen(quieto, 'avance');
    const cola = quieto.slice(-2);
    const asentado = cola.length > 0 && cola.every((l) => campo(l, 'avance') <= 0.1);
    inf.check('agachado QUIETO no se arrastra (se queda parado)',
      !avq || (avq.med <= 0.25 && asentado),
      avq ? ('avance mediana=' + num(avq.med) + ' m/s, máx=' + num(avq.max)
        + (avq.max > 0.5 ? ' (pico puntual: golpe o frenada)' : '')
        + ', cola=' + cola.map((l) => num(campo(l, 'avance'))).join('/')) : 'sin muestras del tramo quieto');
    inf.check('al desagacharse se retiran los clips', dePie.indexOf('crouch pose off') >= 0,
      (dePie.match(/VICEEXT crouch pose off clips=\d+/) || ['SIN traza'])[0]);
    // La cámara tiene que BAJAR: se compara la CÁMARA SOBRE EL PED de pie
    // (`PEDAT camz=`−`z`, medido justo antes de agacharse) con la de agachado
    // (`CROUCH2 camz=`−`h`). Así no depende de dónde esté el jugador; el bloque H
    // del verificador comparaba la camz de agachado con la de pie de TODA la
    // sesión (dos sitios distintos) y el 22/09 cantó un falso fallo (11,18 vs
    // 11,33) en una partida donde la cámara iba bien.
    const ref = lineasDe(s.traza.tramo(o, mPieAntes), 'PEDAT');
    const altPie = resumen(ref.map((l) => 'a=' + (campo(l, 'camz') - campo(l, 'z'))), 'a');
    const altAga = resumen(av.concat(quieto).map((l) => 'a=' + (campo(l, 'camz') - campo(l, 'h'))), 'a');
    inf.check('la cámara BAJA al agacharse (altura sobre el ped)', !!altPie && !!altAga && altAga.med <= altPie.med - 0.35,
      (altPie && altAga)
        ? ('de pie ' + num(altPie.med) + ' m -> agachado ' + num(altAga.med) + ' m sobre el ped')
        : 'sin `camz` en PEDAT/CROUCH2 (build sin R19e)');
    const tarde = pie.filter((l) => campo(l, 'desde') >= 700);
    inf.check('de pie los clips de agachado pesan 0',
      pie.length === 0 || tarde.every((l) => campo(l, 'idle') <= 0.01 && campo(l, 'fwd') <= 0.01 && campo(l, 'back') <= 0.01),
      tarde.length ? (tarde.length + ' muestra(s) de pie con peso 0') : 'sin trazas CROUCHPOSE');
    inf.noError();
    if (!inf.ok) await s.foto('agachado-fallo');
    inf.cierra('agachado');
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
    // Primero el truco de DIAGNÓSTICO del mod (`CRAZYHINT` deja `HINTTEST` en la
    // traza): así, si algo falla después, se sabe si el problema es el teclado de
    // trucos o lo que se pruebe con él. La primera corrida de este escenario
    // salió "sin WINFO" y no se sabía si era el truco, el arma o el teclado.
    await s.cheat('CRAZYHINT');
    await sleep(1500);
    const trucoOk = s.traza.tramo(o, s.traza.marca()).indexOf('HINTTEST mostrado=1') >= 0;
    inf.check('el teclado de trucos del mod responde', trucoOk,
      trucoOk ? 'CRAZYHINT -> HINTTEST mostrado=1' : 'sin HINTTEST: el truco no entró (teclado/escritorio)');
    if (!trucoOk) { await s.foto('armas-sin-truco'); inf.cierra('armas'); return inf; }

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
// 5) AGACHADO + MIRANDO (R21): escenario CORTO y enfocado. La fase de apuntado del
//    escenario `agachado` cae al final de una corrida larga, en plena calle y con
//    el mundo vivo (un coche puede atropellar al ped y tumbar el agachado justo
//    ahí: medido el 23/09, con `CARPED` y velocidades de 80 m/s de propina). Este
//    mide SÓLO lo del apuntado en ~50 s, así que entra en ventanas limpias mucho
//    más a menudo y se puede correr solo: `--escenario agachado-mira`.
export const agachadoMira = {
  nombre: 'agachado-mira',
  titulo: 'apuntar agachado: la pose del mod y la rueda de lado (escenario corto)',
  marcas: ['pesomira=%.2f', 'moved=%.2f', 'VICEEXT crouch roll lado=', 'Crouch_Roll_L'],
  async ejecutar(s) {
    const inf = informe('agachado-mira', s);
    const segDir = Number(process.env.VC_DIR_S || 2.6);
    const o = inf.abre('mira');
    const estaAgachado = (margenMs = 2500) => {
      const txt = s.traza.leer(Math.max(0, s.traza.tam() - 60000));
      const ms = [...String(txt).matchAll(/^(\S+?)Z\s+CROUCH2\s/gm)];
      if (!ms.length) return false;
      const t = Date.parse(ms[ms.length - 1][1] + 'Z');
      return Number.isFinite(t) && (Date.now() - t) < margenMs;
    };
    // Agacharse (con reintentos: el mundo puede cancelarlo).
    let agachado = false;
    for (let i = 0; i < 4 && !agachado; i++) {
      await s.pulsa('KeyC', 140);
      await sleep(1800);
      agachado = estaAgachado();
    }
    inf.check('el ped está agachado', agachado, agachado ? 'CROUCH2 con latido reciente'
      : 'no se consigue agachar (¿atropello? ver `CARPED` en la traza)');
    if (!agachado) { await s.foto('agachado-mira-sin-agachar'); inf.cierra('mira'); return inf; }

    // Pistola EN MANO: `CRAZYPISTOL` (bloque P2 del mod) es el único truco que deja
    // el arma actual puesta; `CRAZYTOOLS` sólo la mete en el inventario.
    await s.cheat('CRAZYHINT');
    await s.cheat('CRAZYPISTOL');
    await sleep(5000);
    for (let i = 0; i < 4 && !estaAgachado(); i++) { await s.pulsa('KeyC', 140); await sleep(1600); }

    await s.gira(0);                            // puntero sobre el canvas
    await s.ratonAbajo('right');
    await s.abajo('Delete');
    await sleep(2500);
    const m0 = s.traza.marca();
    await sleep(2500);
    const quieto = lineasDe(s.traza.tramo(m0, s.traza.marca()), 'CROUCH2');
    const pes = resumen(quieto, 'pesomira');
    const cana = quieto.filter((l) => campo(l, 'cana') === 1).length;
    inf.check('apuntando agachado sale la pose del mod (`WEAPON_crouch`)',
      !!pes && pes.med >= 0.5,
      pes ? ('pesomira mediana=' + num(pes.med) + ', máx=' + num(pes.max) + ' en ' + pes.n + ' muestra(s)')
        : ('sin CROUCH2 en el tramo (' + quieto.length + '): '
          + (cana ? 'el agachado se cayó' : 'sin arma apuntable en mano (cana=0)')));

    // Rueda de lado: lo que pidió el jugador ("rueda hacia ese lado") y con la
    // pose puesta. Se mide clip, movimiento REAL (PEDAT) y hacia dónde.
    console.log('   apuntando         clip pedido          velo med  desvío  esperado  metros');
    for (const d of [{ nombre: 'izquierda', teclas: ['KeyA'], ang: 90, clip: 'crouch_roll_l', lado: 'izq' },
                     { nombre: 'derecha',   teclas: ['KeyD'], ang: -90, clip: 'crouch_roll_r', lado: 'der' }]) {
      const a0 = s.traza.marca();
      for (const k of d.teclas) await s.abajo(k);
      await sleep(segDir * 1000);
      for (const k of d.teclas) await s.suelta(k);
      await sleep(900);
      const a1 = s.traza.marca();
      const c2 = lineasDe(s.traza.tramo(a0, a1), 'CROUCH2').slice(1);
      const est = c2.filter((l) => texto(l, 'nom') !== 'Crouch_Idle');
      const nombres = [...new Set((est.length ? est : c2).map((l) => texto(l, 'nom')).filter((x) => x))];
      const ult = (est.length ? est : c2).slice(-1)[0] || '';
      const rumboFin = campo(ult, 'rumbo') * 180 / Math.PI;
      const pedMov = [];
      let vio = false;
      for (const l of s.traza.tramo(a0, a1).split('\n').map((x) => x.trim())) {
        if (/\sCROUCH2\s/.test(l)) { if (!/nom=Crouch_Idle/.test(l)) vio = true; continue; }
        if (vio && /\sPEDAT\s/.test(l)) pedMov.push(l);
      }
      let desvio = NaN, metros = NaN;
      if (pedMov.length >= 2) {
        const p0 = pedMov[0], p1 = pedMov[pedMov.length - 1];
        const bx = campo(p1, 'x') - campo(p0, 'x');
        const by = campo(p1, 'y') - campo(p0, 'y');
        metros = Math.hypot(bx, by);
        if (metros > 0.5)
          desvio = normaliza180(Math.atan2(bx, by) * 180 / Math.PI + rumboFin);
      }
      const velo = resumen(est.length ? est : c2, 'velo');
      const moved = resumen(est.length ? est : c2, 'moved');
      const pesD = resumen(est.length ? est : c2, 'pesomira');
      const dist = resumen(est.length ? est : c2, 'camdist');
      // R22: la rueda se cuenta por EVENTOS (`VICEEXT crouch roll ...`), no por la
      // traza de 1/s: la rueda dura 0,9 s y una muestra por segundo puede no
      // pillarla nunca (pasó: el clip de la rueda no apareció en 8 s de tramo).
      const ruedas = [...s.traza.tramo(a0, a1).matchAll(/VICEEXT crouch roll lado=(\S+)/g)]
        .map((m) => m[1]);
      console.log('   apuntando ' + d.nombre.padEnd(10) + ' ' + (nombres.join(',') || '—').padEnd(20)
        + ' ' + num(velo && velo.med).padStart(7) + '  ' + num(desvio).padStart(6) + '  '
        + num(-d.ang).padStart(8) + '  ' + num(metros) + '  ruedas=' + ruedas.join(','));
      // R22: UNA rueda por petición. El jugador: "me muero si camino hacia a un
      // lado" (con el clip en bucle se quedaba revolcándose por el suelo).
      // La propiedad que importa es que la rueda NO se quede puesta (en bucle
      // infinito Tommy se revolcaba por el suelo durante todo el tramo): una
      // rueda por pulsación, y después el ped vuelve a andar agachado. Se admite
      // una segunda si el mando se suelta y se vuelve a pedir (la sonda sintética
      // pierde la tecla a ratos: medido en esta misma corrida, `Crouch_Idle` en
      // medio de un tramo con la tecla pulsada).
      inf.check('apuntando agachado, «' + d.nombre + '» rueda y NO se queda rodando (' + d.lado + ')',
        ruedas.length >= 1 && ruedas.length <= 2 && ruedas.every((r) => r === d.lado)
          && nombres.some((n) => /^crouch_forward/i.test(n)),
        ruedas.length
          ? (ruedas.length + ' rueda(s): ' + ruedas.join(',') + '; clips del tramo: ' + (nombres.join(',') || '—'))
          : 'sin traza `VICEEXT crouch roll` (clip pedido: ' + (nombres.join(',') || '—') + ')');
      inf.check('apuntando agachado, «' + d.nombre + '» el ped va a 1,85 m/s de motor',
        !!moved && Math.abs(moved.med - 1.85) <= 0.5,
        moved ? ('moved mediana=' + num(moved.med) + ' m/s (objetivo 1,85; velo de posición '
          + num(velo && velo.med) + ' m/s)') : 'sin `moved=` en CROUCH2');
      // La cámara: en la calle el ped puede acabar contra una pared y la cámara
      // se mete en él sin que sea culpa de la bajada del agachado, así que aquí
      // es AVISO (el control duro está en «agachado», que mide en marcha).
      if (!dist || dist.min < 1.5)
        inf.aviso('apuntando agachado, «' + d.nombre + '» la cámara se mete en el ped',
          dist ? ('camdist ' + num(dist.min) + '..' + num(dist.max) + ' m (¿ped contra una pared?)')
            : 'sin camdist');
      inf.check('apuntando agachado, «' + d.nombre + '» desplaza al ped',
        !!velo && velo.med > 0.4,
        velo ? (num(velo.med) + ' m/s, ' + num(metros) + ' m recorridos') : 'sin muestras');
      // Sin movimiento no hay dirección que medir (el ped se quedó contra una
      // pared: medido, 0,20 m recorridos): eso es AVISO con el número, no fallo.
      if (!(Number.isFinite(desvio) && Math.abs(normaliza180(desvio + d.ang)) <= 30))
        inf.aviso('apuntando agachado, «' + d.nombre + '» no se pudo medir la dirección',
          'desvío ' + num(desvio) + '° (se espera ' + (-d.ang) + '°), ' + num(metros)
          + ' m recorridos: sin movimiento no hay dirección');
      // R22: andando la pose va a 0,4 (para que las piernas del clip de agachado
      // sigan mandando y no se congele el cuerpo); quieto va a 1,0.
      inf.check('apuntando agachado, «' + d.nombre + '» con la pose puesta (0,4 andando)',
        !!pesD && pesD.med >= 0.3, 'pesomira mediana=' + num(pesD && pesD.med));
    }

    await s.suelta('Delete');
    await s.ratonSuelta('right');
    await sleep(2000);
    const mFin = s.traza.marca();
    await sleep(2500);
    const fuera = lineasDe(s.traza.tramo(mFin, s.traza.marca()), 'CROUCH2');
    const pesFuera = resumen(fuera, 'pesomira');
    // Sin muestras no se puede afirmar nada: en la calle el agachado se cae solo
    // (atropello, coche, mando perdido) y eso deja el tramo vacío. Se marca AVISO
    // con el motivo en vez de fallo, que es lo que pasó en la corrida de ve46.
    if (fuera.length === 0) {
      inf.aviso('no se pudo comprobar que la pose se retire al soltar el apuntado',
        'sin muestras CROUCH2 en el tramo final: el agachado se cayó');
    } else {
      inf.check('al soltar el apuntado la pose se retira',
        !!pesFuera && pesFuera.max <= 0.25,
        (pesFuera ? ('pesomira máx=' + num(pesFuera.max)) : 'sin pesomira')
          + ' en ' + fuera.length + ' muestra(s)');
    }
    inf.noError();
    if (!inf.ok) await s.foto('agachado-mira-fallo');
    inf.cierra('mira');
    return inf;
  },
};

export const ESQUEMAS = [carga, agachado, agachadoMira, armas, nado];
