// GTA Vice City (reVC) en el navegador, como librería.
//
// Uso mínimo (Vue, React, TS o HTML suelto):
//
//   import { startGame } from '@re3/gtavc-web';
//   const game = startGame({ el: document.getElementById('vc') });
//
// La función monta el juego DENTRO del div que le pases: crea el canvas, la
// barra de progreso rosa pegada al top del viewport, y arranca el motor sin
// pedir clic (el audio se desbloquea solo con el primer gesto del usuario).
//
// El motor (wasm) y los datos NO van en el paquete: se sirven por HTTP y aquí
// solo se apunta dónde están (buildUrl / streamedUrl / manifestUrl / assetsUrl).
import { CSS, STYLE_ID } from './styles.js';

// Tag de esta ronda: sale en la consola, en el título y en las trazas.
// Sirve para saber QUÉ código corre (adiós confusión de cachés).
// ve12: código de la sección 3 (C3.5 luces rompibles al disparo + `reload done
// manual=1` + `CAM1P` sin ruido + NPC en los intermitentes). Sólo etiqueta: los
// DATOS no cambian y `dataTag` sigue `ve10`.
// ve14: reenlace del 20/09 14:41 con la sección 3 del PLAN v2 **completa** (que
// ve13 se quedó en el enlace de las 14:20): 1ª persona con V + autocentrado de
// cámara en vehículo + ranura de autoguardado primera/rotulada/protegida +
// `reload key`/`motivo` en todas las salidas + depósito que EXPLOTA (con
// respaldo por caja de colisión para los modelos sin `petrolcap`) + nadar (C4),
// agachado (C5), esprint con arma de 2 manos (C6) y apuntar con la escopeta (C7).
// Sólo etiqueta: sigue sin cambios de datos, `dataTag` = `ve10`.
// ve15: sección 1, 21/09 — sonidos de las 8 armas nuevas del mod (13 muestras
// de su banco, ids 9941..9953 en el banco del port), corte de `CARRATE`
// rehacido y +20 m de modelo bueno antes de que entre el LOD (D10).
// Con CAMBIO DE DATOS: los 13 mp3 nuevos y `Audio/sfx.SDT` ampliado, así que
// `dataTag` sube a `ve11`.
// ve16: sección 1, 21/09 — cierre de los pendientes de la sección: la
// frecuencia de los vehículos del mod vuelve a SUS valores (10 y 7; el 100 era
// temporal para verlos circular, ya visto), con el cargador ponderado que se
// queda. Cambia DATO ⇒ `dataTag` a `ve11`→`ve12`.
// ve17: sección 3, 21/09 (7ª partida del jugador, "todas mentiras") — el fallo
// que más dolía era invisible en el log porque la traza decía una cosa y el
// juego hacía otra. Lo que cambia el CÓDIGO (los datos siguen en `ve12`):
//   * la tecla del conmutador de 1ª persona (V) y la del agachado (C) se leen con
//     **respaldo**: la config de controles que el navegador guardó con un build
//     anterior deja esas acciones SIN tecla (`rsNULL`) y la tecla no llegaba al
//     motor (mismo fallo que ya se corrigió en la recarga con R);
//   * el autocentrado de cámara estaba escrito SÓLO en la cámara LCS, que no es
//     la que se juega (`bFreeCam` nace apagado): ahora también está en la cámara
//     de coche de serie; y el detector de "estoy mirando" pasa de `delta != 0`
//     (con ratón apoyado siempre da "mirando") a una ventana de ~1 s;
//   * el agachado deja de ser el de DISPARO del motor (que clava al jugador en
//     el sitio): ahora permite andar agachado, con los clips del mod;
//   * el nadado reconoce la CAÍDA al agua (antes sólo valía con control a pie,
//     así que el ped caía al vacío sin estado de nado);
//   * el depósito de gasolina sólo explota con el impacto EN el tanque
//     (0,22 m del tapón / el cuadro del respaldo) y en las motos hace falta el
//     tanque real, no la caja;
//   * la lista de CARGAR vuelve a tener 8 filas: autoguardado + ranuras 1..7.
// ve18: sección 1, 21/09 (8ª partida) — **"las armas nuevas sonaban mudas"**: el
// motor descartaba sus 13 muestras. `InitialiseChannel` sólo reproducía ids <
// `SAMPLEBANK_MAX` (el fin de la tabla SDT ORIGINAL) y las del mod son
// 9941..9953 ⇒ caían en la rama de comentarios de ped y devolvían FALSE, sin
// dejar ni una traza (medido: 40 disparos `VICEEX sfx arma=51 sample=9943` y
// cero `ODSFXMISS sfx=9943`). Ahora se sirven por el camino on-demand, y las
// muestras se re-codifican con su frecuencia REAL (el banco del mod declara
// 36000/33000/22000 Hz, que no existen en MP3) ⇒ `dataTag` sube a `ve13`.
// Dato nuevo para no repetirlo: `tools/viceex-sfx-check.py` (tabla + fichero +
// formato + rango del motor) y el bloque D8 de `tools/viceext-log-check.py`, que
// detecta "disparó y no arrancó" = arma muda.
// ve19: 21/09 (9ª partida) — **el paquete NUNCA se había enlazado**: el jugador
// estuvo varias partidas con el `.wasm` del 21/09 08:38 (build `ve17`), así que no
// podía ver nada de lo hecho hoy (R5-R11, D8b, R2/R3 de la capa web). Lección y
// regla nueva: antes de pedir una partida, `tools/check-served-build.sh` dice si
// el motor servido lleva las marcas de lo implementado. Este build las lleva
// todas.
// ve20: 21/09 (sección 1) — pantalla de carga de partida: UNA sola portada y UNA
// sola barra progresiva. Antes cada fase (shutdown, parse, drenado, escena)
// reescribía `gWebLoadFrac` desde abajo y las pantallas vanilla pedían portada
// aleatoria (`loadscN`), con lo que el splash se recargaba a media carga y el
// arte cambiaba ("veo la portada del juego"). Ahora `WebBeginLoadScreen()` fija
// `splash1` y enciende `gWebLoadScreenActive`: `LoadSplash` ignora otro nombre y
// `LoadingScreen` delega en `WebDrawLoadScreen`, que además es monótono (nunca
// retrocede) y no se re-entra. Traza: `LOADSCR begin/end` y `LOADSCR clamp`
// (frontera de fase que intentaba volver atrás; el recorte deja la barra en un
// solo barrido 0->100 y el log lo demuestra).
// ve22: 21/09 (parte B, subagente 3) — H1 cámara del agachado (−0,55 + `camz=`),
// H2 nado (margen 0,25, velocidades en m/frame /50 y cámara a la superficie),
// H3 apuntado (snap del rumbo en el control con ratón + `peso` del
// CROUCHFIRE) y §6 luces de servicio (el fallo ya no se cachea y se buscan
// todos los nombres del `.dff`). Enlazado junto a lo de la sección 1: build
// único con las dos partes. Bloque `H` nuevo en `tools/viceext-log-check.py`
// (agachado: `camz`; nado: `avance` real y salidas del agua).
// ve23: 21/09 (sección 1, 9ª partida) — CAUSA RAÍZ del apuntado y del control
// con ratón: `CPed::CanStrafeOrMouseControl` devolvía false SIEMPRE porque
// `bFreeCam` (que en este port es la cámara moderna, no la libre de depuración)
// la cortaba; con false, el cuerpo no encaraba la cámara (`AIMDIR desv` hasta
// 86°), el pase a pie con ratón no corría y la 1ª persona quedaba bloqueada
// (`1p set ... free=1`). Además: conmutador de 1ª persona sin `bFreeCam` en su
// puerta, recarga a mano con animación (`reload anim`), agachado sin doble
// pulsación (`repetido=`), autocentrado de coche suave + empujón al soltar la
// mirada (`camauto2 ... pedido=`) y apuntado del lanzagranadas.
// ve24: 21/09 (sección 1, 10ª partida) — LA SIRENA DE LAS PATRULLAS. Causa
// raíz encontrada por datos (el `police.dff` servido se parseó entero): los
// `servicelights_1..3` del mod son HIJOS de `extra1..3`, y `PreprocessHierarchy`
// saca esos extras del clump; `CVehicleModelInfo::CreateInstance` clonaba SÓLO
// el atomic del extra, así que los hijos (las lentes de la barra) nunca
// llegaban a la instancia: barra sin luces y `FindDummyFrame` sin encontrar
// nada (`SVLIGHTS dummies=0` en toda partida). Ahora se clona el subárbol
// completo del marco del componente (con el NOMBRE del marco copiado a mano:
// `RwFrameCreate` no lo trae). Además las coronas ya no se anclan al dummy
// (está en (0,0,0): la malla se coloca por vértices) sino al centro de la
// **esfera envolvente** de la malla de las luces ± su radio, en el eje derecho
// del coche; la caché pasa a ser por (modelo, extras elegidos) porque cada
// patrulla lleva una de las tres variantes de barra. Traza con `var=` y
// `extras=` y bloque nuevo `SR` (sirena audible) en el verificador.
// ve25: 21/09 (10ª partida, plan de mods) — primer arreglo portado de
// SilentPatch: el CONTORNO de la barra de carga no escalaba con la resolución
// (`hpos-1.0f`/`top-1.0f` eran 1 px literales). Ahora usa la misma escala que la
// barra (`SCREEN_SCALE_X/Y(1.0)`); traza `LBAR w= h= borde=` y bloque `LB` del
// verificador. Manifiesto de atribución de los tres mods en
// `docs/mods/ATTRIBUTION.md`.
// ve26: 21/09 noche (11ª partida) — el jugador: "vi la v25 y todo sigue igual".
// El log de su sesión (`04-04-07`, 2 min) lo explica: NO hay una sola línea
// `VICEEXT 1p key` ni `v=1` en `CAM1P`, o sea que la 'V' no llegó al motor en
// toda la sesión; y `AIMDIR` ya sale `desv=0.0` en las cinco armas nuevas (el
// cuerpo SÍ encara la cámara: el arreglo anterior funciona). Se atacan las tres
// causas que quedaban sin dato:
//  - R13: la 'V' se lee SIEMPRE por la acción Y por la tecla cruda. La config de
//    controles guardada en el navegador puede tener la acción con OTRA tecla
//    (cuando el hueco era la `B`), y entonces ni la V ni el respaldo la veían.
//    Traza nueva `VICEEXT 1p bind tecla=` (una por sesión) con lo que hay guardado.
//  - `WINFO` (una línea por arma que el jugador saca): flags reales de
//    `weapon.dat`, `canaim`, `witharm`, `reload`, `sight` y los NOMBRES de clip
//    que el grupo resuelve para disparo/agachado/recarga. Es lo que faltaba para
//    decir si "no hay animación de apuntado/recarga" es un flag o un clip ausente.
//  - `AIMDIR` añade `tgt=` y `ik=`, y `SIGHT` añade `por=` (1=conduciendo,
//    2=modo de cámara, 3=apuntando) para saber por qué se pinta la mira.
// ve27: 22/09 (12ª partida, vídeos `pc.mp4` + `browser.mp4`) — vuelta al
// diagnóstico con DATOS y tres causas raíz nuevas:
//  - R14 CÁMARA DE NADO: `CCam::IsTargetInWater` daba verdadero nadando (el ped
//    va unos centímetros bajo la superficie) y el motor pedía
//    `MODE_PLAYER_FALLEN_WATER`: la cámara de "jugador que se ahoga", clavada en
//    la última posición sobre el agua. El vídeo del navegador lo enseña tal cual
//    (la calzada desde arriba y Tommy fuera de cuadro) y el log también
//    (`SWIM2 camz=12.02` constante con el ped a z=5.4 sobre nivel 6.1). Ahora,
//    nadando, ese predicado devuelve falso y la cámara sigue al ped. Traza nueva
//    `SWIMCAM objetivo= nivel= cam= modo=` (1/s nadando).
//  - R14 AUTOCENTRADO DE COCHE: el flanco de soltado que disparaba el empujón de
//    2,2 rad/s miraba también el RATÓN (`camauto2 pedido=1` decenas de veces por
//    minuto con `vel=0.0`). El ratón sólo reinicia el temporizador del retorno
//    suave; el empujón rápido queda para cruzeta/palo. El retorno pasivo pasa a
//    ser proporcional al error (arranca suave y frena al llegar).
//  - R14 RETROCESO: la retícula deja de moverse (`m_f3rdPersonCHairMultY` vuelve
//    a quedarse en 0,400) y el retroceso lo acusa la CÁMARA, con patada visible
//    en todas las armas (0,30° SMG … 1,40° francotirador). Traza `RECOIL3
//    miracheck multY=` para demostrar que la mira no se mueve.
//  - H4 CAMINADO APUNTANDO: el clip lo elige la velocidad SIN tope (el de andar
//    normal) y la cadencia se escala con el cociente real: mismo caminado, más
//    lento, sin patinar.
//  - R14 SONIDOS DE ARMA: las 13 muestras del mod se reservan (no se posponen ni
//    se reciclan) al primer disparo de cada arma; el reparto de decodes sube a 3
//    inline y 2 en cola por frame. Traza `ODSFXPROT sfx=`.
//  - R28 AGACHADO CALIBRADO (crouch2): el agachado va a la MISMA velocidad que
//    el andar de pie (1,13 m/s, era 0,90), la rueda sale con A/D (también en
//    diagonal) manteniendo apuntar, el disparo agachado usa el clip de agachado
//    del ARMA (`colt45_crouchfire`) y la pose del mod cede ante él: dos poses
//    parciales a la vez eran el cuerpo estirado/aplastado y la "lucha entre 2
//    animaciones". Traza: `CROUCH2 ... otros= pose= pesoarma= nomarma=`.
// OJO: `VERSION` va TAMBIÉN en la URL del motor (`reVC.js`/`reVC.wasm` con
// `?v=VERSION`): cambiarla es lo ÚNICO que fuerza al navegador a bajar el .wasm
// nuevo. Un build enlazado con el mismo tag se sigue sirviendo de la caché (pasó
// el 23/09: dos corridas del arnés midieron el binario anterior y salían "sin
// pesomira en CROUCH2" con el build bueno en disco). Subir el tag en CADA build.
export const VERSION = '2026-10-07-plan50';

const DEFAULTS = {
  el: null,                 // div destino (elemento o selector). Obligatorio.
  // --- De dónde salen las cosas ---
  buildUrl: '/build/',      // reVC.js + reVC.wasm + reVC.data
  streamedUrl: '/streamed/',// datos sueltos on-demand
  manifestUrl: '/manifest.json', // índice de streamed/ (claves en minúsculas)
  assetsUrl: '/vc/',        // copia legal original, SOLO se mira si falta streamed/
  // --- Comportamiento ---
  fill: 'viewport',         // 'viewport' (toda la pantalla) | 'parent' (la caja del div)
  title: true,              // poner el título de la pestaña con el tag de build
  audio: true,              // desbloquear audio con el primer gesto (sin botones)
  pointerLock: true,        // capturar el ratón al hacer clic en el canvas
  guardUnload: true,        // pedir confirmación al cerrar/recargar en partida
  fullscreenKey: true,      // F11 = pantalla completa del juego
  showFps: false,           // contador de FPS del motor (esquina superior izq.)
  idbCapMB: 900,            // techo de la caché de datos en IndexedDB (0 = sin techo)
  warmMB: 64,               // MB de "arranque en caliente" durante la carga (0 = off)
  worker: true,             // cargar ficheros en un hilo aparte (false = como antes)
  traceUrl: null,           // p.ej. '/odtrace' en desarrollo; null = sin envíos
  console: true,            // espejo a la consola del navegador
  noInitialRun: false,      // cargar runtime+FS sin arrancar el juego (diagnóstico)
  requireBuild: true,       // si falta el build, no arrancar y avisar
  requireData: true,        // si faltan los datos, no arrancar y avisar
  // --- Ganchos ---
  onProgress: null,         // ({ phase, pct, label, busy })
  onLog: null,              // (line)
  onError: null,            // (error)  -> también por consola
  onReady: null,            // ()  runtime wasm listo
  onAudio: null,            // (active:boolean)
  onFullscreen: null,       // (active:boolean)
};

// El motor emscripten es global (Module, FS, OD): una instancia por página.
let current = null;

const ODPRI = /^(loadtick|webload|BART|ASYNC|RETUNE|RADIOTRACK|FPSLOG|ENGAP|PERF|ODCAP|GEAR|initstep|od-trace|FPHASE|ODLOAD|ODSFXDEFER|ODSFXSTAT|ODSFXBUDGET)/;
const NOISY = /took \d+ ?ms|\[DBG\]: (Request|Remove) Ped|\.txd took |REMOVE_SOUND - Sound doesn't exist/;

function base(url, fallback) {
  if (url === null || url === undefined || url === '') return fallback;
  return String(url).endsWith('/') ? String(url) : String(url) + '/';
}

// HEAD con guardia de content-type: un dev server tipo Vite responde su
// index.html (200 text/html) a rutas inexistentes, así que "ok" no basta.
async function exists(url) {
  try {
    const r = await fetch(url, { method: 'HEAD' });
    if (!r.ok) return false;
    const ct = (r.headers.get('content-type') || '').toLowerCase();
    return !ct.includes('text/html');
  } catch (e) {
    return false;
  }
}

function injectStyles(doc) {
  if (doc.getElementById(STYLE_ID)) return;
  const s = doc.createElement('style');
  s.id = STYLE_ID;
  s.textContent = CSS;
  (doc.head || doc.documentElement).appendChild(s);
}

function markup(root, fill) {
  root.dataset.fill = fill;
  root.innerHTML = `
    <canvas class="vc-canvas" tabindex="0"></canvas>
    <div class="vc-bar" aria-hidden="true"><i></i></div>
    <div class="vc-load" hidden>
      <div class="vc-load-title">VICE CITY</div>
      <div class="vc-load-sub">Cargando… no cierres la pestaña</div>
      <div class="vc-load-bar"><i></i></div>
      <div class="vc-load-label"></div>
    </div>
    <div class="vc-msg" hidden></div>`;
}

// ---------------------------------------------------------------------------
// startGame: única función que necesita el host.
// ---------------------------------------------------------------------------
export function startGame(options = {}) {
  const opts = Object.assign({}, DEFAULTS, options);
  const doc = opts.doc || (typeof document !== 'undefined' ? document : null);
  if (!doc) throw new Error('gtavc-web: hace falta un documento (navegador).');

  if (current) {
    current.log('[lib] ya hay un juego arrancado; se devuelve la instancia viva');
    return current;
  }

  const el = typeof opts.el === 'string' ? doc.querySelector(opts.el) : opts.el;
  if (!el) throw new Error('gtavc-web: falta { el } (el div donde montar el juego).');

  const buildUrl = base(opts.buildUrl, '/build/');
  const streamedUrl = base(opts.streamedUrl, '/streamed/');
  const manifestUrl = opts.manifestUrl || '/manifest.json';
  const assetsUrl = opts.assetsUrl === null ? null : base(opts.assetsUrl, '/vc/');
  const traceUrl = opts.traceUrl || null;
  const recoilTraceId = null;   // Sin query param: id siempre automático.
  // Recoil siempre activo si hay endpoint de trazas: sin query param.
  // ?trace=<nombre> solo etiqueta la sesión (y el fichero en desarrollo).
  const recoilTraceActive = !!traceUrl;
  let recoilEventSeq = 0;
  let recoilDropped = 0;
  let recoilEndSent = false;
  let odDropPending = 0;   // descartes de cola pendientes de constancia
  // Volcado local (E3): si el envío falla, la sesión sobrevive en el
  // navegador y se reenvía al siguiente arranque con endpoint.
  const ODSPILL_KEY = 'vc_od_spill_v1';
  const ODSPILL_CAP = 2000;
  let odSpillLost = 0;
  function odSpill(lines) {
    try {
      const raw = localStorage.getItem(ODSPILL_KEY);
      const arr = raw ? JSON.parse(raw) : [];
      for (const ln of lines) arr.push(ln);
      while (arr.length > ODSPILL_CAP) { arr.shift(); odSpillLost++; }
      localStorage.setItem(ODSPILL_KEY, JSON.stringify(arr));
    } catch (e) {}
  }
  function odReplay() {
    try {
      if (!traceUrl) return;
      const raw = localStorage.getItem(ODSPILL_KEY);
      if (!raw) return;
      const arr = JSON.parse(raw);
      if (!arr.length) return;
      localStorage.removeItem(ODSPILL_KEY);
      const body = ['ODTRACE_REPLAY n=' + arr.length + (odSpillLost ? ' perdidas=' + odSpillLost : '')].concat(arr.slice(0, 3000)).join('\n');
      fetch(traceUrl, { method: 'POST', body, keepalive: true }).then((r) => { if (!r || !r.ok) odSpill(arr); }).catch(() => { odSpill(arr); });
    } catch (e) {}
  }

  // El motor (WebDrawFps en main.cpp) lee este global en cada frame: así el
  // contador de FPS se enciende por API, sin parámetros en la URL.
  globalThis.__vcShowFps = !!opts.showFps;

  injectStyles(doc);
  el.classList.add('vc-root');
  markup(el, opts.fill === 'parent' ? 'parent' : 'viewport');

  const canvas = el.querySelector('.vc-canvas');
  const barEl = el.querySelector('.vc-bar');
  const barFill = barEl.querySelector('i');
  const loadEl = el.querySelector('.vc-load');
  const loadBar = loadEl.querySelector('.vc-load-bar > i');
  const loadLabel = loadEl.querySelector('.vc-load-label');
  const msgEl = el.querySelector('.vc-msg');

  const logLines = [];
  const listeners = { progress: [], log: [], error: [], ready: [], audio: [], fullscreen: [], booted: [] };
  const timers = [];
  let destroyed = false;
  let runtimeReady = false;
  let audioUnlocked = false;
  let idleTimer = 0;
  let lastProgress = { phase: null, pct: 0, label: '', busy: false };

  // Los eventos salen por dos vías: la API (game.on) y el propio div
  // (CustomEvent 'vc-progress', 'vc-ready', 'vc-error', 'vc-booted', ...).
  // Así un index.html cualquiera puede escuchar sin importar la librería.
  const emit = (evt, arg) => {
    for (const fn of listeners[evt] || []) { try { fn(arg); } catch (e) {} }
    try { el.dispatchEvent(new CustomEvent('vc-' + evt, { detail: arg })); } catch (e) {}
  };

  // ---- log --------------------------------------------------------------
  let logBuf = [];
  let logTimer = 0;
  function writeLogDom(lines) {
    const g = doc.getElementById('gamelog');           // opcional: host o página dev
    if (!g) return;
    try {
      const t = performance.now();
      g.textContent += lines.join('\n') + '\n';
      if (window.__jankM) { window.__jankM.domN++; window.__jankM.domMs += performance.now() - t; }
      if (g.textContent.length > 300000) g.textContent = g.textContent.slice(-200000);
      g.scrollTop = g.scrollHeight;
    } catch (e) {}
  }
  function log(t) {
    const s = String(t);
    logLines.push(s);
    if (logLines.length > 4000) logLines.splice(0, 2000);
    emit('log', s);
    logBuf.push(s);
    if (!logTimer) {
      logTimer = setTimeout(() => { logTimer = 0; writeLogDom(logBuf); logBuf = []; }, 500);
      timers.push(logTimer);
    }
    if (opts.console && (/^\[err\]|\[REJECT\]|\[FATAL\]/.test(s) || !NOISY.test(s))) console.log('[game]', s);
    return s;
  }
  function fail(text, detail) {
    log('[err] ' + text);
    if (msgEl) msgEl.hidden = false;
    if (msgEl) msgEl.innerHTML = '<h3>El juego no puede arrancar</h3><p>' + text + '</p>' + (detail || '');
    const err = new Error(text);
    emit('error', err);
    if (opts.onError) opts.onError(err);
  }

  // ---- barra de progreso (rosa, top del viewport) ------------------------
  // Cubre SOLO la preparación previa al motor:
  //   'probe' → comprobación de ficheros (build y datos)
  //   'build' → descarga y arranque del motor (reVC.js/wasm/data)
  // El fin es determinista: el motor avisa por window.__vcFrame la primera vez
  // que presenta un frame (glfw.cpp, EmscriptenTick). Ahí la barra se retira y
  // NO vuelve nunca: a partir de ese momento manda la pantalla del juego (menú,
  // splash de carga de partida). __loadProgress (fases del init) solo llega al
  // inicializar el mundo, así que no sirve como señal de "ya hay juego".
  // Reparto del progreso GLOBAL (21/09, pedido del jugador: "una sola subida de
  // 0 a 100, que solo cargue una vez").
  //
  // Antes cada fase empezaba en 0 — `probe` era un barrido sin medida y `build`
  // volvía a arrancar desde cero — así que la barra parecía cargar DOS veces y no
  // se leía como un progreso. Ahora:
  //   * `probe` (comprobar build y datos) aporta el primer 4%;
  //   * `build` (descargar y arrancar el motor) aporta del 4% al 100%.
  // El valor es MONÓTONO: nunca baja, ni al cambiar de fase ni cuando el cargador
  // reporta algo sin medida (antes eso volvía a poner el barrido y parecía
  // "empezar de nuevo").
  const BAR_TRAMO = { probe: [0.0, 0.04], build: [0.04, 1.0] };
  let barOverall = 0;      // progreso global 0..1 (nunca baja)
  let barMedido = false;   // ya hay una medida real: el barrido indeterminado no vuelve
  let engineUp = false;
  let lastReportKey = '';
  // La descarga pregunta el estado cientos de veces por segundo: la barra se
  // pinta como mucho una vez por frame. Sin esto eran miles de escrituras de
  // DOM por segundo y la barra parpadeaba.
  let barQueued = false;
  let barPending = null;     // { indet: true } | { indet: false, pct }
  let barTraceNext = 0;      // siguiente punto (5%) que se anota en la traza
  function barTrace(msg) {
    try { (window.__odq = window.__odq || []).push('BART ' + msg); } catch (e) {}
  }
  function applyBar() {
    barQueued = false;
    const p = barPending;
    barPending = null;
    if (!p || engineUp) return;
    // Ojo: el atributo vale '' (falso), así que hay que comparar con undefined:
    // si no, se reescribe en cada frame sin necesidad.
    if (barEl.dataset.on === undefined) barEl.dataset.on = '';
    if (p.indet) {
      if (barEl.dataset.indet === undefined) barEl.dataset.indet = '';
      // Sin ancho inline: si queda el '100%' de una fase anterior, gana al 32%
      // del CSS y el barrido se ve como una barra llena que destella.
      barFill.style.width = '';
    } else {
      if (barEl.dataset.indet !== undefined) delete barEl.dataset.indet;
      const pctMostrado = 100 * Math.max(0, Math.min(1, p.pct));
      barFill.style.width = pctMostrado.toFixed(1) + '%';
      // Traza de la subida: el jugador pidió "una sola subida de 0 a 100"; con
      // esto se comprueba en el log, sin mirar la pantalla, que el progreso es
      // monótono y lineal (un `BART pct=…` por cada 5 puntos).
      if (pctMostrado >= barTraceNext) {
        barTraceNext = Math.floor(pctMostrado) + 5;
        barTrace('pct=' + Math.round(pctMostrado));
      }
    }
  }
  function queueBar(p) {
    barPending = p;
    if (barQueued) return;
    barQueued = true;
    if (typeof requestAnimationFrame === 'function') requestAnimationFrame(applyBar);
    else setTimeout(applyBar, 16);
  }
  function showBar() { if (!engineUp) queueBar({ indet: false, pct: 1 }); }
  function hideBar() { delete barEl.dataset.on; delete barEl.dataset.indet; }
  function report(phase, pct, label, busy) {
    lastProgress = { phase, pct, label: label || '', busy: !!busy };
    // Mismo estado repetido (la descarga lo reporta cientos de veces por
    // segundo): no se dispara otro evento.
    const key = phase + '|' + (pct === null || pct === undefined ? 'x' : (+pct).toFixed(2)) + '|' + lastProgress.label + '|' + lastProgress.busy;
    if (key === lastReportKey) return;
    lastReportKey = key;
    emit('progress', lastProgress);
    if (opts.onProgress) opts.onProgress(lastProgress);
  }
  // El motor ya es dueño de la pantalla: la barra del top se apaga y ninguna
  // fuente vuelve a encenderla. 'reason' queda en las trazas.
  function engineOwnsScreen(reason) {
    if (engineUp) return;
    engineUp = true;
    barPending = null;
    clearTimeout(idleTimer);
    hideBar();
    barTrace('off reason=' + (reason || '?'));
  }
  function progress(phase, pct, label, busy) {
    const unknown = pct === null || pct === undefined;
    report(phase, pct, label, unknown ? true : (busy === undefined ? pct < 1 : !!busy));
    if (engineUp || !BAR_TRAMO[phase]) return;
    const [ini, fin] = BAR_TRAMO[phase];
    if (!unknown) {
      const v = ini + (fin - ini) * Math.max(0, Math.min(1, +pct));
      if (v > barOverall) barOverall = v;   // nunca baja
      barMedido = true;
      queueBar({ indet: false, pct: barOverall });
    } else if (!barMedido) {
      // Todavía sin ninguna medida (comprobar ficheros): ahí sí, barrido.
      queueBar({ indet: true });
    }
    // Si ya hay medida y esta llamada no trae número (p. ej. "Descargando datos
    // del juego…" cuando el cargador no da total), la etiqueta cambia y el ancho
    // se queda: antes el barrido reiniciaba la barra a media carga.
    // Red de seguridad: si el motor no llegara a avisar (build antigua), la
    // barra se retira sola un poco después de tener el runtime listo.
    clearTimeout(idleTimer);
    idleTimer = setTimeout(() => { if (runtimeReady) engineOwnsScreen('idle-timeout'); }, 6000);
  }
  // Cierre de la fase de preparación (100% y fuera).
  function done() {
    if (engineUp) return;
    barPending = null;
    barEl.dataset.on = '';
    delete barEl.dataset.indet;
    barFill.style.width = '100%';
    barTrace('on pct=100 (done)');
    clearTimeout(idleTimer);
    idleTimer = setTimeout(() => engineOwnsScreen('done-timeout'), 400);
  }

  // ---- cola de trazas (lo que manda el motor C++) ------------------------
  // window.__odq lo llenan EM_ASM del motor y el pre-js. Aquí se envía por
  // lotes async (nunca XHR síncrono en el hilo del juego) y se acota para que
  // una tormenta de líneas no se coma la memoria.
  const odq = (window.__odq = window.__odq || []);
  const recoilSession = window.__odRecoilSession = { active: recoilTraceActive, id: (recoilTraceId && /^recoil-[a-zA-Z0-9_-]{1,40}$/.test(recoilTraceId)) ? recoilTraceId : ('auto-' + Date.now().toString(36)) };
  const recoilEmit = window.__odRecoilEmit = (event) => {
    if (!recoilSession.active) return;
    const seq = ++recoilEventSeq;
    const line = `RECOIL sessionId=${recoilSession.id} eventSeq=${seq} build=${VERSION} data=${(typeof OD !== 'undefined' && OD.dataTag) || 'unavailable'} wasm=OD_RECOIL_SCHEMA_1 ${String(event).replace(/\\s+/g, ' ').slice(0, 640)}`;
    const q = window.__odq = window.__odq || [];
    if (q.length >= 30000) {
      recoilDropped++;
      return;
    }
    q.push(line);
    if (/^RECOIL_READY\b/.test(event)) { const rc = /recoil=(\d+)/.exec(event); recoilEmit(`RECOIL_DIAG schema=1 sessionId=${recoilSession.id} build=${VERSION} data=${(typeof OD !== 'undefined' && OD.dataTag) || 'unavailable'} wasm=OD_RECOIL_SCHEMA_1 recoilCompiled=${rc ? rc[1] : 'unknown'} enabled=1 dropped=${recoilDropped}`); }
  };
  function endRecoilSession() {
    if (!recoilSession.active || recoilEndSent) return;
    recoilEndSent = true;
    recoilSession.active = false;
    const line = `RECOIL sessionId=${recoilSession.id} eventSeq=${++recoilEventSeq} build=${VERSION} data=${(typeof OD !== 'undefined' && OD.dataTag) || 'unavailable'} wasm=OD_RECOIL_SCHEMA_1 RECOIL_DIAG_END lastSeq=${recoilEventSeq - 1} dropped=${recoilDropped} terminal=pagehide`;
    (window.__odq = window.__odq || []).push(line);
    flush(true);
  }
  function flush(sync) {
    try {
      const q = window.__odq;
      if (!q || !q.length) return;
      if (q.length > 30000) {
        let drop = q.length - 30000;
        for (let i = 0; i < q.length && drop > 0; i++) if (!ODPRI.test(q[i]) && !/^RECOIL\\b/.test(q[i])) { q.splice(i, 1); i--; drop--; }
        if (drop > 0) {
          for (let i = 0; i < q.length && drop > 0; i++) if (!/^RECOIL\\b/.test(q[i])) { q.splice(i, 1); i--; drop--; }
        }
        if (drop > 0) { recoilDropped += drop; odDropPending += drop; q.splice(0, drop); }
      }
      const batch = q.splice(0, 3000);
      if (odDropPending > 0) { batch.unshift('ODTRACE_DROP descartes=' + odDropPending); odDropPending = 0; }
      if (!traceUrl) {
        // Sin endpoint de trazas: se guardan acotadas (para diagnóstico desde
        // la consola) sin inundar ni la consola ni la memoria.
        logLines.push(...batch);
        if (logLines.length > 4000) logLines.splice(0, logLines.length - 4000);
        return;
      }
      batch.sort((a, b) => (ODPRI.test(b) ? 1 : 0) - (ODPRI.test(a) ? 1 : 0));
      const body = batch.join('\n');
      if (sync) { const x = new XMLHttpRequest(); x.open('POST', traceUrl, false); x.send(body); }
      else fetch(traceUrl, { method: 'POST', body, keepalive: true }).then((r) => { if (!r || !r.ok) odSpill(batch); }).catch(() => { odSpill(batch); });
    } catch (e) {}
  }
  timers.push(setInterval(() => flush(false), 1000));
  if (!traceUrl) log('[lib] sin endpoint de trazas: solo memoria (4000 lineas)');
  try { odReplay(); } catch (e) {}
  // Traza a prueba de cuelgue (sección 1, 21/09). El envío normal va cada
  // segundo y luego, si el motor se queda en un bucle infinito DENTRO de un
  // frame, ese segundo no llega nunca: el log se corta y no hay forma de saber
  // en qué punto del juego se quedó.
  //
  // La cola se entrega al navegador en CADA frame con `navigator.sendBeacon`
  // (siempre activo con endpoint: sin query param): aunque el hilo se bloquee
  // o se cierre la pestaña, la última línea emitida SÍ llega.
  if (traceUrl) {
    const beat = () => {
      try {
        const q = window.__odq;
        if (q && q.length && typeof navigator !== 'undefined' && navigator.sendBeacon) {
          const batch = q.splice(0, 400);
          navigator.sendBeacon(traceUrl, batch.join('\n'));
        }
      } catch (e) {}
      if (!destroyed) requestAnimationFrame(beat);
    };
    requestAnimationFrame(beat);
    log('[lib] synctrace activo: cada frame se envía la traza (diagnóstico de cuelgues)');
  }
  // E1: cierre garantizado. El fetch async puede no salir al cerrar la
  // pestaña; el beacon lo entrega el navegador aunque la página muera.
  const onPageHide = () => {
    endRecoilSession();
    try {
      const q = window.__odq;
      if (traceUrl && q && q.length) {
        const body = q.splice(0, q.length).join('\n');
        if (typeof navigator !== 'undefined' && navigator.sendBeacon) navigator.sendBeacon(traceUrl, body);
        else { const x = new XMLHttpRequest(); x.open('POST', traceUrl, false); x.send(body); }
      }
    } catch (e) {}
  };
  // 20/09 (sección 1): el fallo duro DEJA CONSTANCIA en odtrace.log.
  //
  // Hasta ahora el motivo del cuelgue sólo salía por la consola de la página
  // (`[FATAL] ...`): cuando el jugador reportaba "cinemática → crashea" la traza
  // simplemente se cortaba en mitad de un frame y no había forma de saber si fue
  // un assert del motor, un OOM o una excepción de JS. Ahora el texto se encola
  // en la misma cola de trazas (`JSERR`/`ENGERR`) y se manda de forma SÍNCRONA
  // antes de que la página muera; el latido `FPSLOG` de cada segundo que falta
  // después marca el instante exacto del cuelgue.
  const pushFatal = (tag, what) => {
    try {
      const txt = String(what == null ? what : what).replace(/\s+/g, ' ').slice(0, 600);
      if (txt) (window.__odq = window.__odq || []).push(tag + ' ' + txt);
    } catch (e) {}
    try { flush(true); } catch (e) {}
  };
  window.addEventListener('pagehide', onPageHide);
  window.addEventListener('error', (e) => {
    const msg = String(e?.error?.stack || e?.message || e).slice(0, 2000);
    console.log('[FATAL] ' + msg);
    pushFatal('JSERR', msg);
  });
  window.addEventListener('unhandledrejection', (e) => {
    const msg = String(e?.reason?.stack || e?.reason || e).slice(0, 500);
    console.log('[REJECT] ' + msg);
    pushFatal('JSERR_REJ', msg);
  });
  // El motor avisa de sus fallos duros por stderr (emscripten `printErr` →
  // console.error): "Aborted(...)", "RuntimeError: ...", "Out of memory".
  // Se reenvía el texto a la traza y se deja pasar a la consola como siempre.
  try {
    const origErrLog = console.error ? console.error.bind(console) : null;
    console.error = function (...args) {
      try {
        const s = args.map((a) => (typeof a === 'string' ? a : String(a))).join(' ');
        if (/Aborted|RuntimeError|Out of memory|Cannot enlarge memory|abort\(|unreachable/i.test(s)) pushFatal('ENGERR', s);
      } catch (e) {}
      if (origErrLog) origErrLog(...args);
    };
  } catch (e) {}

  // ---- contrato con el motor (globals que llama el C++) ------------------
  // El motor avisa aquí la PRIMERA vez que presenta un frame (glfw.cpp,
  // EmscriptenTick). Es la señal buena de "ya hay juego en pantalla".
  // Identidad del paquete que está corriendo la PÁGINA (para que el arnés pueda
  // comprobar que mide lo que cree: ficheros de disco ≠ lo que sirve la caché).
  window.__vcVersion = VERSION;
  window.__vcFrame = () => {
    barTrace('on motivo=motor-primer-frame');
    log('[lib] el motor está pintando: se retira la barra de preparación.');
    engineOwnsScreen('motor-primer-frame');
  };
  window.__loadProgress = (step, total) => {           // CGame::InitialiseStep
    // El motor está inicializando el mundo (empezar o cargar partida): se
    // acabó la preparación previa -> barra del top fuera.
    engineOwnsScreen('init-step');
    report('engine', total ? step / total : null, `Fase ${step}/${total}`, total ? step < total : true);
  };
  // Portada DOM de carga de partida: reserva para hosts donde el splash GL no
  // se ve (hoy el motor la pinta él mismo y esta no se usa). No toca la barra
  // del top: si aparece, ya es pantalla del motor.
  window.__loadOverlay = (show, pct, label) => {
    engineOwnsScreen('load-overlay');
    loadEl.hidden = !show;
    if (show) {
      loadBar.style.width = Math.max(0, Math.min(100, pct)).toFixed(1) + '%';
      if (label) loadLabel.textContent = label;
      report('engine', pct / 100, label, pct < 100);
    }
  };
  window.__vcLog = logLines;

  // ---- audio: sin botón, se abre con el primer gesto ---------------------
  async function unlockAudio() {
    let ok = false, note = 'sin contexto aún';
    try {
      const ctxs = [];
      if (typeof Module !== 'undefined' && Module?.SDL2?.audioContext) ctxs.push(Module.SDL2.audioContext);
      if (typeof AL !== 'undefined' && AL.currentCtx?.audioCtx) ctxs.push(AL.currentCtx.audioCtx);
      for (const ctx of ctxs) {
        if (ctx.state === 'suspended') await ctx.resume();
        if (ctx.state === 'running') ok = true;
      }
      note = ctxs.length ? ctxs.map((c) => c.state).join(',') : note;
    } catch (e) { note = String(e).slice(0, 80); }
    if (ok !== audioUnlocked) {
      audioUnlocked = ok;
      emit('audio', ok);
      if (opts.onAudio) opts.onAudio(ok);
    }
    return ok ? true : note;
  }
  const gestureEvents = ['pointerdown', 'keydown', 'touchstart'];
  const onGesture = async () => {
    if (!opts.audio || audioUnlocked) return;
    if (await unlockAudio() === true) for (const ev of gestureEvents) window.removeEventListener(ev, onGesture, true);
  };
  if (opts.audio) for (const ev of gestureEvents) window.addEventListener(ev, onGesture, true);

  // ---- pantalla completa (F11 y método público) --------------------------
  async function enterFullscreen() {
    try { await el.requestFullscreen({ navigationUI: 'hide' }); } catch (e) { log('[lib] pantalla completa: ' + e); }
    try { if (navigator.keyboard?.lock) await navigator.keyboard.lock(['Escape', 'KeyW', 'KeyA', 'KeyS', 'KeyD']); } catch (e) {}
  }
  async function exitFullscreen() { try { if (doc.fullscreenElement) await doc.exitFullscreen(); } catch (e) {} }
  function isFullscreen() { return doc.fullscreenElement === el; }
  function toggleFullscreen() { return isFullscreen() ? exitFullscreen() : enterFullscreen(); }
  const onFsChange = () => {
    const on = isFullscreen();
    emit('fullscreen', on);
    if (opts.onFullscreen) opts.onFullscreen(on);
    if (on) hideBar();
  };
  doc.addEventListener('fullscreenchange', onFsChange);

  // Esc corto = nada; mantener Esc 3 s = salir de pantalla completa (Esc sigue
  // sirviendo al juego). SIN aviso en pantalla a propósito: no queremos un
  // cartel ni una barra encima del juego.
  const HOLD_MS = 3000;
  let escTimer = 0;
  const onKeyDown = (e) => {
    if (opts.fullscreenKey && (e.code === 'F11' || e.key === 'F11')) {
      e.preventDefault();
      toggleFullscreen();
      return;
    }
    if (e.key !== 'Escape' || escTimer) return;
    escTimer = setTimeout(() => { escTimer = 0; exitFullscreen(); }, HOLD_MS);
  };
  const onKeyUp = (e) => {
    if (e.key !== 'Escape') return;
    if (escTimer) { clearTimeout(escTimer); escTimer = 0; }
  };
  window.addEventListener('keydown', onKeyDown);
  window.addEventListener('keyup', onKeyUp);

  // ---- ratón (solo al hacer clic en el juego) ----------------------------
  const onCanvasClick = async () => {
    if (opts.audio) await unlockAudio();
    if (!opts.pointerLock) return;
    try { await canvas.requestPointerLock(); } catch (e) { log('pointer lock: ' + e); }
  };
  canvas.addEventListener('click', onCanvasClick);
  const onPointerLockChange = () => {
    canvas.style.cursor = doc.pointerLockElement === canvas ? 'none' : 'default';
  };
  doc.addEventListener('pointerlockchange', onPointerLockChange);

  // ---- protecciones e IDBFS ---------------------------------------------
  const onBeforeUnload = (e) => {
    try { if (typeof OD !== 'undefined') OD.syncUserfiles(); else if (typeof FS !== 'undefined') FS.syncfs(false, () => {}); } catch (err) {}
    if (opts.guardUnload && window.__gameBooted && !window.__quitting) e.preventDefault();
  };
  const onVisibility = () => {
    if (!doc.hidden) return;
    try { if (typeof OD !== 'undefined') OD.syncUserfiles(); } catch (err) {}
  };
  window.addEventListener('beforeunload', onBeforeUnload);
  doc.addEventListener('visibilitychange', onVisibility);

  // ---- jank probe: peor frame de cada segundo a las trazas (F4c/FPSLOG) --
  function jankProbe() {
    if (!traceUrl) return;
    try {
      window.__jankM = { xhrN: 0, xhrMs: 0, domN: 0, domMs: 0 };
      let t0 = performance.now(), maxd = 0, idx = 0, n = 0, over17 = 0, heap0 = 0, minHeap = 0;
      const ivs = [];
      let x0 = { n: 0, ms: 0 }, d0 = { n: 0, ms: 0 };
      const tick = (now) => {
        const d = now - t0; t0 = now; idx++;
        n++; if (d > 17.5) over17++;
        ivs.push(d); if (d > maxd) maxd = d;
        const used = performance.memory ? performance.memory.usedJSHeapSize : 0;
        if (idx === 1) { heap0 = used; minHeap = used; }
        if (used && used < minHeap) minHeap = used;
        if (idx >= 60) {
          const s = ivs.slice().sort((a, b) => a - b);
          const med = s[Math.floor(s.length / 2)];
          const xr2 = (window.__jankM.xhrN - x0.n), xm2 = (window.__jankM.xhrMs - x0.ms);
          const dn = (window.__jankM.domN - d0.n), dm = (window.__jankM.domMs - d0.ms);
          window.__odq.push('FPSLOG frames=' + n + ' over17=' + over17 + ' maxdelta=' + Math.max(0, Math.round(maxd)) +
            ' med=' + Math.round(med) + ' heap0=' + Math.round(heap0 / 1048576) + 'MB freed=' + Math.round((heap0 - minHeap) / 1048576) + 'MB' +
            ' xhr=' + xr2 + '/' + Math.round(xm2 * 10) / 10 + 'ms@' + xr2 + ' dom=' + dn + '/' + Math.round(dm * 10) / 10 + 'ms');
          t0 = now; maxd = 0; idx = 0; n = 0; over17 = 0; ivs.length = 0;
          heap0 = used; minHeap = used;
          x0 = { n: window.__jankM.xhrN, ms: window.__jankM.xhrMs };
          d0 = { n: window.__jankM.domN, ms: window.__jankM.domMs };
        }
        if (!destroyed) requestAnimationFrame(tick);
      };
      requestAnimationFrame(tick);
    } catch (e) {}
  }

  // ---- ¿están el build y los datos? --------------------------------------
  async function probeBuild() {
    const [js, wasm, data] = await Promise.all([
      exists(buildUrl + 'reVC.js'), exists(buildUrl + 'reVC.wasm'), exists(buildUrl + 'reVC.data'),
    ]);
    return { js, wasm, data, ok: js && wasm };
  }

  // Los datos se comprueban contra el manifiesto (1 petición, sin 404s):
  // si trae los ficheros centinela, NO se toca assetsUrl para nada.
  async function probeData() {
    const need = ['models/gta3.dir', 'audio/sfx.sdt', 'data/main.scm', 'text/spanish.gxt'];
    let manifest = null;
    try {
      const r = await fetch(manifestUrl, { cache: 'no-store' });
      if (r.ok) manifest = await r.json();
    } catch (e) {}
    if (manifest) {
      const missing = need.filter((k) => !manifest[k]);
      if (!missing.length) return { ok: true, manifest, missing: [] };
      return { ok: false, manifest, missing, reason: 'streamed-incompleto', assets: assetsUrl ? await exists(assetsUrl + 'data/main.scm') : false };
    }
    return {
      ok: false, manifest: null, missing: need, reason: 'sin-manifiesto',
      assets: assetsUrl ? await exists(assetsUrl + 'data/main.scm') : false,
    };
  }

  // ---- arranque ----------------------------------------------------------
  function boot() {
    canvas.hidden = false;
    window.__gameBooted = true;
    emit('booted', true);

    window.Module = {
      canvas,
      noInitialRun: !!opts.noInitialRun,
      // El build vive en buildUrl, la página en otro sitio: sin esto el .data
      // se pediría a la raíz.
      //
      // ve19: `?v=<VERSION>` SÓLO en el código (`.js`/`.wasm`), no en el `.data`.
      // Motivo (9ª partida): el navegador puede quedarse con el `reVC.wasm`
      // anterior — el nombre de fichero no cambia entre builds — y entonces el
      // jugador prueba código viejo sin saberlo (le pasó: jugó con el motor de
      // por la mañana). El `.data` (161 MB) se deja SIN parámetro a propósito:
      // si no, cada build obligaría a volver a descargarlo entero.
      locateFile: (path) => buildUrl + path + (/\.(js|wasm)$/.test(path) ? '?v=' + VERSION : ''),
      print: (t) => log(t),
      printErr: (t) => log('[err] ' + t),
      // Guardados persistentes: /userfiles vive en IndexedDB (IDBFS). El resto
      // de assets va en MEMFS (lectura síncrona del motor).
      preRun: [() => {
        try {
          FS.mkdir('/userfiles');
          FS.mount(IDBFS, {}, '/userfiles');
          // El juego NO espera solo: sin la dependencia leía MEMFS vacío
          // antes de que bajara IDB (carrera de lectura).
          if (typeof addRunDependency !== 'undefined') addRunDependency('idbfs-populate');
          FS.syncfs(true, (err) => {
            log(err ? '[err] IDBFS load: ' + err : 'IDBFS listo (guardados persistentes).');
            try { if (typeof removeRunDependency !== 'undefined') removeRunDependency('idbfs-populate'); } catch (e) {}
          });
          setTimeout(() => { try { if (typeof removeRunDependency !== 'undefined') removeRunDependency('idbfs-populate'); } catch (e) {} }, 15000);
        } catch (e) { log('[err] IDBFS: ' + e); }
      }],
      // El loader reporta "Downloading data... (123/456)": va a la barra.
      setStatus: (t) => {
        if (!t || engineUp) return;
        const m = t.match(/\((\d+)\s*\/\s*(\d+)\)/);
        // Con varios ficheros el contador es un progreso real. Con uno solo
        // (reVC.data) un "0%" clavado durante toda la descarga sería mentira:
        // la barra pasa a "trabajando" hasta que el motor arranque.
        if (m && +m[2] > 1) {
          progress('build', +m[1] / +m[2], `Descargando datos: ${Math.round(100 * m[1] / m[2])}%`);
        } else if (m) {
          progress('build', null, 'Descargando datos del juego…');
        } else {
          progress('build', null, t);
          if (!/Downloading data/.test(t)) log('[status] ' + t);
        }
      },
      onRuntimeInitialized: () => {
        runtimeReady = true;
        log('Runtime WASM listo, arrancando juego…');
        // El motor avisa por __vcFrame en cuanto presenta su primer frame
        // (unos cientos de ms). Este plazo es solo la red por si esa señal no
        // llegara (build antigua); antes eran 5 s y la barra se quedaba encima
        // del menú ya visible.
        clearTimeout(idleTimer);
        idleTimer = setTimeout(() => engineOwnsScreen('runtime-timeout'), 2500);
        jankProbe();
        emit('ready', true);
        if (opts.onReady) opts.onReady();
      },
    };

    const s = doc.createElement('script');
    // `?v=` = tag de esta ronda: el motor (`reVC.js`/`reVC.wasm`) no se puede
    // quedar en la caché del navegador de un build anterior.
    s.src = buildUrl + 'reVC.js?v=' + VERSION;
    s.async = true;
    s.onerror = () => fail(`No se pudo cargar <code>${buildUrl}reVC.js</code>.`);
    el.appendChild(s);
    log('Descargando datos del juego… (' + buildUrl + ' motor ' + VERSION + ')');
  }

  const handle = {
    ok: true,
    version: VERSION,
    el, root: el, canvas, config: { buildUrl, streamedUrl, manifestUrl, assetsUrl, fill: opts.fill, traceUrl },
    get module() { return typeof Module !== 'undefined' ? Module : null; },
    get progress() { return lastProgress; },
    get logs() { return logLines; },
    log,
    // showBar explícito del host manda (la política automática solo la
    // enciende en la preparación previa al motor).
    showBar: () => { if (!engineUp) { barEl.dataset.on = ''; barTrace('on (host)'); } },
    hideBar, setProgress: progress, done,
    // Caché de datos del navegador (IndexedDB 'vcod2'): la llena el propio
    // juego. Cuánto ocupa y cómo vaciarla sin tocar DevTools.
    cacheInfo() {
      const od = typeof OD !== 'undefined' ? OD : null;
      return { capMB: od?.cfg?.idbCapMB ?? null, warmMB: od?.cfg?.warmMB ?? null, trimmed: od?.trimmed || 0 };
    },
    // Vacía la caché de datos (los ficheros se re-descargan cuando hagan falta)
    // y olvida la lista de arranque en caliente. Los guardados NO se tocan:
    // viven en /userfiles (IDBFS de Emscripten), no en esta caché.
    async clearDataCache() {
      const od = typeof OD !== 'undefined' ? OD : null;
      if (!od) return false;
      try { od.warmSeen = null; od.warmList = null; localStorage.removeItem(od.warmKey); } catch (e) {}
      await od.idbClear();
      log('[lib] caché de datos vaciada (se re-descargará lo que el juego pida)');
      return true;
    },
    // Contador de FPS del motor, en caliente (sin recargar).
    showFps(on = true) { globalThis.__vcShowFps = !!on; return !!on; },
    fpsVisible: () => !!globalThis.__vcShowFps,
    isFullscreen,
    enterFullscreen, exitFullscreen, toggleFullscreen,
    unlockAudio,
    audioActive: () => audioUnlocked,
    on(evt, fn) {
      if (!listeners[evt]) throw new Error('gtavc-web: evento desconocido ' + evt);
      listeners[evt].push(fn);
      return () => { const i = listeners[evt].indexOf(fn); if (i >= 0) listeners[evt].splice(i, 1); };
    },
    // Diagnóstico del FS virtual (con noInitialRun:true, sin arrancar el juego).
    fsProbe() {
      const out = (t) => log('[probe] ' + t);
      try {
        for (const e of performance.getEntriesByType('resource')) {
          if (/reVC\.(data|wasm|js)/.test(e.name)) {
            out(`${e.name.split('/').pop()} transfer=${e.transferSize} encoded=${e.encodedBodySize} decoded=${e.decodedBodySize} dur=${Math.round(e.duration)}ms`);
          }
        }
        out('cwd=' + FS.cwd());
        out('root=' + JSON.stringify(FS.readdir('/').slice(0, 20)));
        out('TEXT=' + JSON.stringify(FS.readdir('/TEXT')));
        out('spanish.gxt size=' + FS.stat('/TEXT/spanish.gxt').size);
        FS.chdir('/TEXT');
        const fd = FS.open('spanish.gxt', 'r');
        const buf = new Uint8Array(12);
        const n = FS.read(fd, buf, 0, 12, 0);
        out(`lower open+read -> n=${n} magic=${String.fromCharCode(...buf.slice(0, 4))}`);
        FS.close(fd);
      } catch (e) { out('FAIL: ' + (e && e.message || e)); }
      out('probe done');
    },
    // Descarga best-effort: el módulo wasm no se puede liberar de verdad en
    // una página, así que se para el bucle y se limpia lo nuestro.
    destroy() {
      if (destroyed) return;
      destroyed = true;
      try { endRecoilSession(); } catch (e) {}
      try { Module?.pauseMainLoop?.(); } catch (e) {}
      for (const t of timers) { clearTimeout(t); clearInterval(t); }
      window.removeEventListener('pagehide', onPageHide);
      window.removeEventListener('error', onAnyError);
      window.removeEventListener('unhandledrejection', onAnyError);
      window.removeEventListener('beforeunload', onBeforeUnload);
      window.removeEventListener('keydown', onKeyDown);
      window.removeEventListener('keyup', onKeyUp);
      for (const ev of gestureEvents) window.removeEventListener(ev, onGesture, true);
      doc.removeEventListener('visibilitychange', onVisibility);
      doc.removeEventListener('fullscreenchange', onFsChange);
      doc.removeEventListener('pointerlockchange', onPointerLockChange);
      el.innerHTML = '';
      el.classList.remove('vc-root');
      window.__gameBooted = false;
      current = null;
    },
  };
  current = handle;
  try { el.__vcGame = handle; } catch (e) {}

  // ---- puesta en marcha (ARRANCA SOLO, sin botones ni gestos) ------------
  (async () => {
    log('[lib] ' + VERSION + ' · build=' + buildUrl + ' streamed=' + streamedUrl);
    if (typeof location !== 'undefined') {
      log('buildTag ' + VERSION + ' (si no ves este tag, recarga con Ctrl+Shift+R)');
      if (opts.title) { try { doc.title = '[vc ' + VERSION + '] GTA Vice City'; } catch (e) {} }
    }
    if (!window.crossOriginIsolated) {
      log('[err] crossOriginIsolated=false: hacen falta COOP/COEP (same-origin + require-corp) para los pthreads del motor.');
    }
    progress('probe', null, 'Comprobando build…');

    const b = await probeBuild();
    if (!b.ok) {
      handle.ok = false;
      if (opts.requireBuild) {
        return fail(
          `Falta el build de WebAssembly en <code>${buildUrl}</code> (reVC.js / reVC.wasm / reVC.data).`,
          '<ul><li>Compílalo: <code>gta_vc_browser\\build.bat</code> + <code>emmake ninja -C gta_vc_browser\\build\\web</code>.</li>' +
          '<li>O apunta <code>buildUrl</code> a donde lo tengas servido.</li></ul>');
      }
      log('[lib] build incompleto: se arranca igual (requireBuild:false)');
    }

    const d = await probeData();
    if (!d.ok) {
      handle.ok = false;
      const list = '<ul>' + d.missing.map((k) => `<li><code>${k}</code></li>`).join('') + '</ul>';
      if (opts.requireData) {
        if (d.reason === 'sin-manifiesto') {
          if (d.assets) return fail(`No se encontró <code>${manifestUrl}</code>: sin ese índice no se pueden pedir los datos sueltos.`,
            '<ul><li>Genera los datos y su manifiesto (pipeline del README, pasos 2-7).</li></ul>');
          return fail('Faltan los datos del juego (ni <code>' + manifestUrl + '</code> ni la copia original en <code>' + assetsUrl + '</code>).',
            '<ul><li>Coloca tu copia legal del juego en <code>' + assetsUrl + '</code> y corre el pipeline de datos (README).</li></ul>');
        }
        if (d.assets) return fail('Los datos sueltos están incompletos en <code>' + streamedUrl + '</code>.', list +
          '<ul><li>Corre el pipeline de datos (README, pasos 2-7).</li></ul>');
        return fail('No hay datos servidos: falta <code>' + streamedUrl + '</code> y no hay copia original en <code>' + assetsUrl + '</code>.', list);
      }
      log('[lib] datos incompletos: se arranca igual (requireData:false)');
    }

    // El pre-js (ondemand.js) lee esta config al cargarse: de aquí salen las
    // rutas de streamed/ y del manifiesto para el motor.
    globalThis.__VC_CFG = {
      streamedUrl, manifestUrl, odtraceUrl: traceUrl, version: VERSION,
      recoilTraceId: recoilTraceActive ? recoilTraceId : null,
      idbCapMB: opts.idbCapMB, warmMB: opts.warmMB, worker: opts.worker !== false,
    };

    progress('build', 0, 'Cargando motor…');
    boot();
  })();

  return handle;
}

export function getGame() { return current; }
export function isRunning() { return !!current && !!current.ok; }

export default startGame;
