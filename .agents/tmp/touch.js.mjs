const CROSS = 1 << 0;
const SQUARE = 1 << 1;
const CIRCLE = 1 << 2;
const TRIANGLE = 1 << 3;
const LEFT1 = 1 << 4;
const RIGHT1 = 1 << 5;
const LEFT2 = 1 << 6;
const RIGHT2 = 1 << 7;
const SELECT = 1 << 8;
const DPAD_UP = 1 << 10;
const DPAD_DOWN = 1 << 11;
const DPAD_LEFT = 1 << 12;
const DPAD_RIGHT = 1 << 13;
const LEFT_SHOCK = 1 << 14;
const RIGHT_SHOCK = 1 << 15;

const VEHICLE_NAMES = {
  0: 'onfoot',
  1: 'car',
  2: 'bike',
  3: 'boat',
  4: 'heli',
  5: 'plane',
  6: 'tank',
  7: 'train',
  8: 'firetruck',
};

const VEHICLE_ICONS = new Set(['car', 'bike', 'boat', 'heli', 'plane', 'tank']);

const DEFAULT_LAYOUT = {
  'onfoot-run': [93, 79],
  'onfoot-action': [93, 63],
  // Apuntar (autoapuntado al NPC/enemigo mas cercano): solo a pie y con arma.
  'onfoot-aim': [82, 63],
  'onfoot-enter': [93, 47],
  'onfoot-camera': [94, 33],
  'vehicle-left': [7, 82],
  'vehicle-right': [17, 82],
  // Feedback jugador: el de la izquierda es el freno y el de la derecha el
  // acelerador (estaban al revés en pantalla; el mapeo al pad es correcto).
  'vehicle-brake': [76, 83],
  'vehicle-gas': [93, 83],
  'vehicle-handbrake': [94, 63],
  'vehicle-camera': [88, 33],
  'vehicle-horn': [94, 33],
  'vehicle-exit': [94, 44],
  'vehicle-rotate-left': [80, 63],
  'vehicle-rotate-right': [90, 63],
  'vehicle-mission': [7, 47],
  // Plan 64.0 T4: un solo boton de disparo desde vehiculo con el icono real
  // `vehicle-shoot` (antes tres botones inventados fire-left/center/right).
  'vehicle-fire': [76, 62],
};

const VEHICLE_LAYOUT = {
  tank: { 'vehicle-fire': [94, 53] },
  firetruck: { 'vehicle-fire': [94, 53] },
  heli: { 'vehicle-fire-mg': [84, 53] },
};

function clamp(value, min, max) {
  return value < min ? min : value > max ? max : value;
}

function nowMs() {
  return typeof performance !== 'undefined' ? performance.now() : Date.now();
}

// 79.5 (fallo del HUD, 4 avisos): en el movil `setPointerCapture` lanza
// InvalidStateError cuando el puntero ya no esta activo (dedo levantado
// mientras el motor iba a tirones). Esa excepcion saltaba ANTES de guardar el
// toque, asi que el boton o la zona quedaban muertos: en el log sale como
// `JSERR InvalidStateError: Failed to execute 'setPointerCapture'` en
// onButtonDown, onLookDown y onMoveDown. Con esto un fallo de captura nunca
// corta el toque; solo se pierde poder arrastrar fuera del elemento.
function grab(element, id) {
  try {
    if (element && typeof element.setPointerCapture === 'function') element.setPointerCapture(id);
  } catch (e) { /* puntero ya soltado: el toque vale igual */ }
}

function touchDetected() {
  if (typeof window === 'undefined' || typeof navigator === 'undefined') return false;
  const coarse = typeof matchMedia === 'function' && matchMedia('(pointer: coarse)').matches;
  const hover = typeof matchMedia === 'function' && matchMedia('(hover: hover)').matches;
  const max = Number(navigator.maxTouchPoints || 0);
  const ua = String(navigator.userAgent || '');
  const mobile = /Android|iPhone|iPad|iPod|IEMobile|Opera Mini|Mobile|Windows Phone/i.test(ua)
    || !!(navigator.userAgentData && navigator.userAgentData.mobile)
    || (/Macintosh/i.test(ua) && max > 1);
  return mobile || (max > 0 && coarse) || (max > 0 && !hover) || ('ontouchstart' in window && !hover);
}

function escapeHtml(value) {
  return String(value).replace(/[&<>"']/g, (char) => ({
    '&': '&amp;',
    '<': '&lt;',
    '>': '&gt;',
    '"': '&quot;',
    "'": '&#39;',
  })[char]);
}

export { touchDetected };

export function createTouchControls(options) {
  const root = options.root;
  const canvas = options.canvas || root.querySelector('.vc-canvas');
  const doc = root.ownerDocument;
  // Feedback: la traza `vc-touch:` nunca llegó al log del teléfono porque
  // options.log (index.js, función hoisted `log`) solo va al DOM #gamelog.
  // Se envuelve para empujar también a la cola window.__odq que index.js
  // vuelca por beacon a /odtrace (mismo patrón que BART en index.js).
  const baseLog = options.log || (() => {});
  const log = (...args) => {
    try {
      if (typeof window !== 'undefined') (window.__odq = window.__odq || []).push(args.join(' '));
    } catch (e) { /* sin cola: no importa */ }
    try { baseLog(...args); } catch (e) { /* log del host aún no listo: no se corta el arranque */ }
  };
  const getModule = options.getModule || (() => (typeof Module !== 'undefined' ? Module : null));
  const iconBase = String(options.iconBase || '/touch/icons/').replace(/\/?$/, '/');
  const sensitivity = Number(options.sensitivity || 0.9);
  const state = {
    enabled: !!options.enabled,
    mounted: false,
    ready: false,
    bridgeEnabled: false,
    bridgeAutoaim: false,
    bridgeIcons: false,
    mode: 'onfoot',
    context: 0,
    contextAt: 0,
    motorLogged: false,
    ctxSeen: false,
    ctxProbeAt: 0,
    ctxNoExport: false,
    firstFrameAt: 0,
    vehicleType: 0,
    vehicleName: 'onfoot',
    vehicleMission: 0,
    vehicleArmed: 0,
    vehicleTypeAt: 0,
    moveId: null,
    lookId: null,
    dragStart: null,
    moveX: 0,
    moveY: 0,
    lookX: 0,
    lookY: 0,
    lookUntil: 0,
    aimUntil: 0,
    aimTickAt: 0,
    pausePurged: false,
    frameDt: 16.7,
    press: new Set(),
    armed: false,
    firearm: false,
    playing: false,
    lastRun: 0,
    jumpUntil: 0,
    crouchUntil: 0,
    selectUntil: 0,
    triangleUntil: 0,
    lastLeftTap: 0,
    tapStart: 0,
    tapMoved: false,
    lookTravel: 0,
    lookEngaged: false,
    lastSweepLog: 0,
    lastBtnLog: 0,
    lastGasTap: 0,
    lastBrakeTap: 0,
    gasUntil: 0,
    brakeUntil: 0,
    pitch: 0,
    pitchUntil: 0,
    missionUntil: 0,
    rotate: 0,
    confirmUntil: 0,

    menuPointer: null,
    menuGesture: null,
    menuTouch: null,
    bound: false,
    lastTapAt: 0,
    enabledLogged: false,
    frameError: false,
    asyncBridge: false,
    menuIgnoreAt: 0,
    padMode: 0,
    down: new Set(),
    perf: { n: 0, sum: 0, max: 0, fpsEma: 60, lastFrame: 0 },
    frameId: 0,
    layer: null,
    stick: null,
    moveZone: null,
    lookZone: null,
    skipZone: null,
    radarZone: null,
    weaponZone: null,
    radioZone: null,
  };

  const icon = (name) => `<img src="${iconBase}${encodeURIComponent(name)}.png" alt="" draggable="false">`;
  const button = (key, modes, action, iconName, label, vehicles = '') => (
    `<button type="button" class="vc-touch-btn" data-key="${key}" data-modes="${modes}" data-action="${action}"${vehicles ? ` data-vehicles="${vehicles}"` : ''} aria-label="${escapeHtml(label)}">${icon(iconName)}</button>`
  );

  const layer = doc.createElement('div');
  layer.className = 'vc-touch';
  layer.hidden = true;
  // Feedback punto 1: "GIRA EL DISPOSITIVO" debe salir desde minuto 0 en
  // móvil. Estaba dentro de la capa .vc-touch, oculta hasta __vcFirstFrame;
  // ahora cuelga directo de root y el CSS (portrait + pointer coarse) lo
  // muestra desde la carga/portada. pointer-events: none para no bloquear.
  const rotateEl = doc.createElement('div');
  rotateEl.className = 'vc-touch-rotate';
  rotateEl.textContent = 'Gira el dispositivo';
  layer.innerHTML = `
    <div class="vc-touch-zone vc-touch-move" aria-hidden="true"></div>
    <div class="vc-touch-zone vc-touch-look" aria-hidden="true"></div>
    <div class="vc-touch-zone vc-touch-radar" data-modes="onfoot vehicle" aria-hidden="true"></div>
    <div class="vc-touch-zone vc-touch-weapon" data-modes="onfoot" aria-hidden="true"></div>
    <div class="vc-touch-zone vc-touch-radio" data-modes="vehicle" aria-hidden="true"></div>
    <div class="vc-touch-skip" aria-hidden="true"></div>
    <div class="vc-touch-stick" hidden><i></i></div>
    <button type="button" class="vc-touch-btn vc-touch-home" data-key="pause" data-modes="onfoot vehicle" data-action="pause" aria-label="Menú de pausa">${icon('onfoot-menu')}</button>
    ${button('onfoot-run', 'onfoot', 'run', 'onfoot-run-jump', 'Correr o saltar')}
    ${button('onfoot-aim', 'onfoot', 'aim', 'onfoot-scope', 'Apuntar al enemigo más cercano')}
    ${button('onfoot-action', 'onfoot', 'action', 'onfoot-punch', 'Acción')}
    ${button('onfoot-enter', 'onfoot', 'enter', 'onfoot-enter-exit', 'Entrar o salir')}
    ${button('onfoot-camera', 'onfoot', 'camera', 'vehicle-camera', 'Cambiar cámara')}
    ${button('vehicle-left', 'vehicle', 'vehicle-left', 'vehicle-left', 'Izquierda', 'car bike boat tank firetruck')}
    ${button('vehicle-right', 'vehicle', 'vehicle-right', 'vehicle-right', 'Derecha', 'car bike boat tank firetruck')}
    ${button('vehicle-brake', 'vehicle', 'brake', 'vehicle-brake', 'Frenar o descender', 'car bike boat heli plane tank firetruck')}
    ${button('vehicle-gas', 'vehicle', 'gas', 'vehicle-accelerate', 'Acelerar o ascender', 'car bike boat heli plane tank firetruck')}
    ${button('vehicle-handbrake', 'vehicle', 'handbrake', 'vehicle-handbrake', 'Freno de mano', 'car bike boat firetruck')}
    ${button('vehicle-camera', 'vehicle', 'camera', 'vehicle-camera', 'Cambiar cámara', 'car bike boat heli plane tank firetruck')}
    ${button('vehicle-horn', 'vehicle', 'horn', 'vehicle-horn-siren', 'Claxon o sirena', 'car bike boat firetruck')}
    ${button('vehicle-exit', 'vehicle', 'exit', 'onfoot-enter-exit', 'Salir del vehículo', 'car bike boat heli plane tank firetruck')}
    ${button('vehicle-rotate-left', 'vehicle', 'rotate-left', 'vehicle-rotate-left', 'Rotar a la izquierda', 'heli plane tank firetruck')}
    ${button('vehicle-rotate-right', 'vehicle', 'rotate-right', 'vehicle-rotate-right', 'Rotar a la derecha', 'heli plane tank firetruck')}
    ${button('vehicle-fire-mg', 'vehicle', 'fire-mg', 'vehicle-shoot', 'Metralleta', 'heli plane')}
    ${button('vehicle-mission', 'vehicle', 'mission', 'vehicle-activate-mission', 'Misión de vehículo')}
    ${button('vehicle-fire', 'vehicle', 'fire', 'vehicle-shoot', 'Disparar desde el vehículo', 'car bike boat tank firetruck')}
  `;

  state.layer = layer;
  state.stick = layer.querySelector('.vc-touch-stick');
  state.moveZone = layer.querySelector('.vc-touch-move');
  state.lookZone = layer.querySelector('.vc-touch-look');
  state.skipZone = layer.querySelector('.vc-touch-skip');
  state.radarZone = layer.querySelector('.vc-touch-radar');
  state.weaponZone = layer.querySelector('.vc-touch-weapon');    state.radioZone = layer.querySelector('.vc-touch-radio');
    state.actionButton = layer.querySelector('[data-key="onfoot-action"]');
    state.aimButton = layer.querySelector('[data-key="onfoot-aim"]');
    state.actionImg = state.actionButton ? state.actionButton.querySelector('img') : null;
    state.layoutNodes = Array.from(layer.querySelectorAll('[data-key]'));

  function syncActionIcon() {
    const action = state.actionButton;
    if (!action) return;
    const name = state.armed ? 'onfoot-shoot' : 'onfoot-punch';
    if (state.actionImg) {
      const source = iconBase + encodeURIComponent(name) + '.png';
      if (state.actionImg.getAttribute('src') !== source) state.actionImg.src = source;
    }
    const label = state.armed ? 'Disparar' : 'Puñetazo';
    if (action.getAttribute('aria-label') !== label) action.setAttribute('aria-label', label);
  }

  // Plan 64.0 T4: los botones de entrar/salir usan SIEMPRE el icono real unico
  // `onfoot-enter-exit` (el HTML ya lo trae fijo). Los swaps a
  // `vehicle-enter-<clase>` / `vehicle-exit-<clase>` eran iconos inventados
  // y ademas pagaban un cambio de img.src (fetch+decode) por pulsacion.
  function syncVehicleIcons() {}

  // Plan 64.0 T2: los llamadores pasan nombres sin el guion bajo de Emscripten
  // (`ViceExtTouchSkip`), pero los exports reales son `_ViceExtTouchSkip`. La
  // resolucion vive solo aqui; las llamadas directas a `_ViceExtTouchFrame`
  // etc. no se tocan. Si falta el export, se avisa una vez por nombre en vez
  // de fallar en silencio (era la causa de que skip/pausa/arma/radio/mapa no
  // respondieran).
  const missingExports = new Set();
  function callModule(name, ...args) {
    const module = getModule();
    if (!module) return;
    const fn = typeof module[name] === 'function' ? module[name] : module['_' + name];
    if (typeof fn !== 'function') {
      if (!missingExports.has(name)) {
        missingExports.add(name);
        log(`vc-touch: export ausente _${name}`);
      }
      return;
    }
    fn(...args);
  }

  // El motor se compila con JSPI (`-sJSPI_EXPORTS=...ViceExt*`): Emscripten
  // envuelve TODAS las exportaciones `ViceExt*` con `WebAssembly.promising` y
  // devuelven una promesa. Leerlas con `| 0` daba 0 siempre: la capa táctil se
  // creía en juego a pie todo el rato (sin botones de vehículo) y no veía ni el
  // menú ni las cinemáticas. Se pide y se guarda el valor al resolver.
  const valores = new Map();
  function leer(name) {
    const module = getModule();
    if (!module) return undefined;
    const fn = typeof module[name] === 'function' ? module[name] : module['_' + name];
    if (typeof fn !== 'function') return undefined;
    let out;
    try { out = fn(); } catch (e) { return undefined; }
    if (out && typeof out.then === 'function') {
      if (!state.asyncBridge) {
        state.asyncBridge = true;
        log('vc-touch: el motor devuelve promesas (JSPI); se leen al resolver');
      }
      out.then((v) => valores.set(name, Number(v) | 0)).catch(() => {});
      return valores.get(name);
    }
    return Number(out) | 0;
  }

  // 79.0: zonas muertas y ventana de toque. Antes bastaba una deriva de 2-5 px
  // del dedo para mover al personaje o girar la camara, y un toque de mas de
  // 260 ms no contaba como toque: el dedo lento se sentia como un mini-desliz.
  const MOVE_SLOP_PX = 14;
  const LOOK_SLOP_PX = 10;
  const TAP_MAX_MS = 600;
  const MENU_ROW_PX = 30;
  // 79.1: en el menu un toque se estaba tomando por desliz. Con 20 px de
  // umbral, la deriva normal del dedo al apretar (20-30 px en un movil) marcaba
  // "desliz" y el toque no activaba nada; y si tardabas mas de 600 ms en soltar
  // tampoco contaba como toque. Ahora el desliz necesita 34 px de recorrido real
  // y un toque vale hasta 1,2 s: manda el movimiento, no la prisa.
  const MENU_SLIDE_PX = 34;
  const MENU_TAP_MS = 1200;
  // 79.6: mantener el dedo quieto en el menu solo empieza a repetir tras 0,7 s
  // (antes repetia a los 0,2 s y cada 0,11 s: parecia que el menu se disparaba
  // solo). Ademas un desliz mueve UNA fila, no cinco.
  const MENU_HOLD_DELAY_MS = 700;
  const MENU_HOLD_MS = 700;
  // 79.3: cuanto dura la mira fijada al tocar "Apuntar" (autoapuntado).
  const AIM_HOLD_MS = 5000;
  const MENU_REPEAT_MS = 260;

  function menuStep(dir) {
    callModule('ViceExtTouchMenuStep', dir < 0 ? -1 : 1);
  }

  function menuActivate(via = 'touch') {
    const now = nowMs();
    if (now - state.lastTapAt < 400) return;
    state.lastTapAt = now;
    const module = getModule();
    const canPulse = !!(module && typeof module._ViceExtTouchFrame === 'function');
    if (!canPulse) {
      callModule('ViceExtTouchMenuActivate');
      log(`vc-touch: menu toque motor via=${via}`);
      return;
    }
    state.confirmUntil = now + 50;
    log(`vc-touch: menu toque pulse via=${via} modo=${state.mode}`);
  }

  function menuChange(dir) {
    callModule('ViceExtTouchMenuChange', dir < 0 ? -1 : 1);
    log(`vc-touch: menu cambio=${dir < 0 ? -1 : 1}`);
  }

  function menuActiveNow() {
    const module = getModule();
    const fn = module && typeof module._ViceExtTouchContext === 'function' ? module._ViceExtTouchContext : null;
    if (fn) {
      const ctx = leer('ViceExtTouchContext');
      if (ctx !== undefined) return !!(ctx & 4);
    }
    return state.mode === 'menu';
  }

  // El motor puede contestar ctx=0 siempre (nunca enciende el bit 64 de
  // "partida cargada"): entonces `playing` era false toda la partida y la capa
  // tactil se quedaba oculta. Si pasan 4 s con frames y no ha llegado ni un ctx
  // distinto de 0, se da el juego por empezado y se muestran los botones.
  function inGameFallback() {
    if (state.ctxSeen || !window.__vcFirstFrame) return false;
    if (!state.firstFrameAt) state.firstFrameAt = nowMs();
    return nowMs() - state.firstFrameAt > 4000;
  }

  function menuOpen() {
    if (menuActiveNow()) return true;
    if (state.playing || inGameFallback()) return false;
    return state.mode !== 'cutscene';
  }

  function probeModule(tag, readNow = true) {
    const module = getModule();
    const fnOf = (name) => {
      if (!module) return null;
      if (typeof module[name] === 'function') return module[name];
      return typeof module['_' + name] === 'function' ? module['_' + name] : null;
    };
    const ctxFn = fnOf('ViceExtTouchContext');
    let ctx = null;
    if (ctxFn && readNow) {
      const raw = leer('ViceExtTouchContext');
      ctx = raw === undefined ? null : raw;
    }
    log(`vc-touch: monta ${tag} motor=${module ? 1 : 0} enabled=${state.enabled ? 1 : 0}`
      + ` ctx=${!readNow ? 'al-arrancar' : ctx === null ? 'sin-funcion' : ctx} menu=${!readNow || ctx === null || ctx < 0 ? '?' : (ctx & 4 ? 1 : 0)}`
      + ` activar=${fnOf('ViceExtTouchMenuActivate') ? 1 : 0}`
      + ` paso=${fnOf('ViceExtTouchMenuStep') ? 1 : 0}`
      + ` cambio=${fnOf('ViceExtTouchMenuChange') ? 1 : 0}`);
    return ctx !== null && ctx >= 0;
  }

  function onMenuPointerDown(event) {
    if (!state.enabled) return;
    if (!menuOpen()) {
      const now = nowMs();
      if (now - state.menuIgnoreAt > 1000) {
        state.menuIgnoreAt = now;
        log(`vc-touch: puntero sin menu ctx=${state.context} modo=${state.mode} juego=${state.playing ? 1 : 0}`);
      }
      return;
    }
    if (event.isPrimary === false || (typeof event.button === 'number' && event.button !== 0)) return;
    event.preventDefault();
    event.stopPropagation();
    const now = nowMs();
    if (state.menuPointer !== null) {
      state.menuPointer = null;
      state.menuGesture = null;
      log('vc-touch: menu reinicia puntero');
    }
    state.menuPointer = event.pointerId;
    state.menuGesture = {
      startX: event.clientX,
      startY: event.clientY,
      dx: 0,
      dy: 0,
      axis: 0,
      moved: false,
      stepped: false,
      startAt: now,
      lastMove: now,
      lastStep: now,
      rowAccum: 0,
      lastY: event.clientY,
      holdDir: 0,
    };
    log(`vc-touch: menu abajo x=${Math.round(event.clientX)} y=${Math.round(event.clientY)}`);
  }

  function onMenuPointerMove(event) {
    const gesture = state.menuGesture;
    if (!gesture || event.pointerId !== state.menuPointer) return;
    event.preventDefault();
    event.stopPropagation();
    const now = nowMs();
    gesture.dx = event.clientX - gesture.startX;
    gesture.dy = event.clientY - gesture.startY;
    gesture.lastMove = now;
    if (!gesture.axis) {
      if (Math.hypot(gesture.dx, gesture.dy) < MENU_SLIDE_PX) return;
      gesture.axis = Math.abs(gesture.dy) > Math.abs(gesture.dx) ? 1 : 2;
      gesture.moved = true;
      gesture.lastY = event.clientY;
      // El recorrido gastado en decidir tambien cuenta para el primer paso: sin
      // esto, con el umbral nuevo un desliz corto (50-70 px) no movia ninguna
      // fila y parecia que el menu no respondia.
      const pasado = Math.abs(gesture.dy) - MENU_SLIDE_PX;
      gesture.rowAccum = pasado > 0 ? (gesture.dy > 0 ? pasado : -pasado) : 0;
      log(`vc-touch: menu desliz=${gesture.axis === 1 ? 'vertical' : 'horizontal'}`);
    }
    if (gesture.axis !== 1) return;
    gesture.rowAccum += event.clientY - gesture.lastY;
    gesture.lastY = event.clientY;
    // 79.6: un desliz mueve UNA fila. Antes cada 30 px de recorrido movia otra
    // y un desliz normal (150 px) saltaba 5 opciones de golpe: por eso era
    // imposible parar en la que querias.
    if (!gesture.stepped && Math.abs(gesture.rowAccum) >= MENU_ROW_PX) {
      const dir = gesture.rowAccum > 0 ? 1 : -1;
      gesture.rowAccum = 0;
      gesture.lastStep = now;
      gesture.holdDir = dir;
      gesture.stepped = true;
      menuStep(dir);
    }
  }

  function onMenuPointerUp(event) {
    const gesture = state.menuGesture;
    if (!gesture || event.pointerId !== state.menuPointer) return;
    event.preventDefault();
    event.stopPropagation();
    const now = nowMs();
    state.menuPointer = null;
    state.menuGesture = null;
    if (gesture.axis === 2 && Math.abs(gesture.dx) >= MENU_ROW_PX) {
      menuChange(gesture.dx > 0 ? 1 : -1);
      return;
    }
    if (gesture.axis === 1) return;
    if (now - gesture.startAt > MENU_TAP_MS) return;
    menuActivate('pointer');
  }

  function onMenuPointerCancel(event) {
    if (event.pointerId !== state.menuPointer) return;
    event.preventDefault();
    event.stopPropagation();
    state.menuPointer = null;
    state.menuGesture = null;
  }

  function updateMenuHold(now) {
    const gesture = state.menuGesture;
    if (!gesture || gesture.axis !== 1) return;
    if (now - gesture.lastMove < MENU_HOLD_DELAY_MS) return;
    if (now - gesture.lastStep < MENU_REPEAT_MS) return;
    const dir = gesture.holdDir || (gesture.dy > 0 ? 1 : gesture.dy < 0 ? -1 : 0);
    if (!dir) return;
    gesture.lastStep = now;
    gesture.holdDir = dir;
    menuStep(dir);
  }

  function applyLayout() {
    const override = VEHICLE_LAYOUT[state.vehicleName] || null;
    for (const element of state.layoutNodes) {
      const key = element.dataset.key;
      const point = (override && override[key]) || DEFAULT_LAYOUT[key];
      if (!point) continue;
      element.style.left = point[0] + '%';
      element.style.top = point[1] + '%';
    }
  }

  function syncVisibility() {
    // Plan 64.0 T3: en cinematics la capa NO se oculta entera (aunque el bit
    // 64 no este) para que la zona .vc-touch-skip reciba el toque; el CSS
    // oculta botones/zonas/stick con [data-mode="cutscene"].
    const inGame = state.playing || inGameFallback();
    layer.hidden = !state.enabled || !window.__vcFirstFrame || (!inGame && state.mode !== 'cutscene');
    if (!layer.hidden) state.ready = true;
  }

  function setMode(mode) {
    if (state.mode === mode) return;
    state.menuPointer = null;
    state.menuGesture = null;
    state.mode = mode;
    layer.dataset.mode = mode;
    log(`vc-touch: mode=${mode}`);
    if (mode !== 'vehicle') {
      state.rotate = 0;
      // Al bajar del coche, data-vehicle se quedaba con el ultimo valor y en
      // el CSS eso hacia aparecer el boton de disparar desde el vehiculo a
      // pie: dos pistolas en pantalla con arma en mano.
      delete layer.dataset.vehicle;
      delete layer.dataset.vehicleArmed;
    }
    state.press.clear();
    state.moveId = null;
    state.lookId = null;
    state.moveX = 0;
    state.moveY = 0;
    state.lookX = 0;
    state.lookY = 0;
    state.stick.hidden = true;
    for (const element of layer.querySelectorAll('.vc-touch-btn.is-active')) element.classList.remove('is-active');
  }

  function readVehicleType() {
    const module = getModule();
    let type = 0;
    if (state.context & 2 && module && typeof module._ViceExtTouchVehicleType === 'function') {
      type = leer('ViceExtTouchVehicleType') | 0;
    } else if (module && typeof module._ViceExtTouchNearVehicleType === 'function') {
      type = leer('ViceExtTouchNearVehicleType') | 0;
    }
    const previousVehicle = state.vehicleName;
    state.vehicleType = type;
    state.vehicleName = VEHICLE_NAMES[type] || (type ? 'car' : 'onfoot');
    layer.dataset.vehicle = state.vehicleName;
    let armed = 0;
    if (state.context & 2 && module && typeof module._ViceExtTouchArmedVehicle === 'function') {
      armed = leer('ViceExtTouchArmedVehicle') | 0;
    }
    state.vehicleArmed = armed;
    layer.dataset.vehicleArmed = armed ? '1' : '0';
    if (previousVehicle !== state.vehicleName) applyLayout();
    let mission = 0;
    if (state.context & 2 && module && typeof module._ViceExtTouchMissionVehicle === 'function') {
      mission = leer('ViceExtTouchMissionVehicle') | 0;
    }
    state.vehicleMission = mission;
    layer.dataset.mission = mission ? '1' : '0';
    syncVehicleIcons();
  }

  function readContext() {
    const module = getModule();
    if (!module) return state.context;
    if (typeof module._ViceExtTouchContext !== 'function') {
      if (!state.ctxNoExport) {
        state.ctxNoExport = true;
        log('vc-touch: ctx sin export (no hay ViceExtTouchContext en el motor)');
      }
      return state.context;
    }
    const crudo = leer('ViceExtTouchContext');
    if (crudo === undefined) return state.context;
    const context = crudo;
    if (context !== 0 && !state.ctxSeen) {
      state.ctxSeen = true;
      log(`vc-touch: ctx vivo=${context} modo=${state.mode}`);
    }
    if (!state.ctxSeen && nowMs() - state.ctxProbeAt > 5000) {
      state.ctxProbeAt = nowMs();
      log(`vc-touch: ctx leer=${context} respaldo=${inGameFallback() ? 1 : 0} oculto=${layer.hidden ? 1 : 0}`);
    }
    if (!state.motorLogged) {
      state.motorLogged = true;
      probeModule('motor');
    }
    if (context !== state.context) log(`vc-touch: ctx=${context}`);
    state.context = context;
    state.armed = !!(context & 32);
    state.firearm = !!(context & 32);
    const playing = !!(context & 64);
    if (state.playing && !playing) releaseAll();
    state.playing = playing;
    syncActionIcon();
    layer.dataset.armed = state.firearm ? '1' : '0';
    if (nowMs() >= state.vehicleTypeAt) {
      // Plan 64.0 T6: el tipo de vehiculo cercano solo se usa para el layout
      // de botones (ya no para iconos, T4); 1 Hz evita el barrido del pool
      // de vehiculos a 4 Hz que causaba tiron en el telefono.
      state.vehicleTypeAt = nowMs() + 1000;
      readVehicleType();
    }
    // Plan 79.3: en pausa el juego no usa los ficheros grandes, asi que se
    // sueltan de memoria justo al abrir el menu (se releen de la cache del
    // navegador al volver). El cierre del log 21:38-21:42 ocurre al reanudar
    // desde la pausa, con el heap ya en 530 MB.
    if (context & 4) {
      if (!state.pausePurged) {
        state.pausePurged = true;
        purgeOnPause();
      }
    } else {
      state.pausePurged = false;
    }
    if (context & 128) setMode('cutscene');
    else if (context & 4) setMode('menu');
    else if (context & 2) setMode('vehicle');
    else setMode('onfoot');
    return context;
  }

  function purgeOnPause() {
    try {
      const od = typeof OD !== 'undefined' ? OD : null;
      if (!od || typeof od.evict !== 'function' || !od.liveBytes) return;
      if (od.liveBytes < 150 * 1048576) return;
      const before = od.liveBytes;
      od.evict(true, 100 * 1048576);
      if (before - od.liveBytes > 20 * 1048576)
        log('vc-touch: pausa libera ' + Math.round(before / 1048576) + '->' +
            Math.round(od.liveBytes / 1048576) + 'MB');
    } catch (e) { /* sin pool: no importa */ }
  }

  function rememberBridge() {
    const module = getModule();
    if (!module || typeof module._ViceExtTouchSetEnabled !== 'function') return;
    if (state.enabled && !state.bridgeEnabled) {
      module._ViceExtTouchSetEnabled(1);
      state.bridgeEnabled = true;
    }
    if (state.enabled && !state.bridgeAutoaim && typeof module._ViceExtTouchSetAutoaim === 'function') {
      module._ViceExtTouchSetAutoaim(1);
      state.bridgeAutoaim = true;
    }
    if (state.enabled && !state.bridgeIcons) {
      state.bridgeIcons = true;
      // Punto 7: con controles táctiles los mensajes ("Pulsa L o R...")
      // muestran los iconos del propio juego (TOUCHBTNS.TXD), no los de
      // mando (X360BTNS) ni de PC (PCBTNS).
      callModule('ViceExtTouchUseTouchIcons');
    }
    if (!state.enabled && state.bridgeEnabled && typeof module._ViceExtTouchSetEnabled === 'function') {
      module._ViceExtTouchSetEnabled(0);
      state.bridgeEnabled = false;
    }
  }

  function crouchPulse() {
    state.crouchUntil = nowMs() + 140;
  }

  function actionDown(key, action) {
    const now = nowMs();
    if (action === 'run') {
      if (now - state.lastRun < 360) {
        state.jumpUntil = now + 650;
        state.lastRun = 0;
      } else {
        state.lastRun = now;
      }
    }
    // Un toque de disparo tambien engancha el objetivo mas cercano antes de
    // tirar: asi el tiro va al enemigo aunque la mira este algo desviada. Es
    // lo que hace el Vice City movil al tocar disparar (autoapuntado).
    if (action === 'action' && state.firearm) callModule('ViceExtTouchAutoAim');
    // Plan 79.3: apuntado automatico como en el Vice City movil. Un toque
    // FIJA la mira en el enemigo mas cercano y la mantiene (el boton se queda
    // encendido); otro toque la suelta. Antes solo enganchaba mientras el dedo
    // estaba encima, asi que al levantar el dedo se perdia la mira.
    if (action === 'aim') {
      if (now < state.aimUntil) {
        state.aimUntil = 0;
        log('vc-touch: apuntado suelto');
      } else {
        state.aimUntil = now + AIM_HOLD_MS;
        state.aimTickAt = 0;
        callModule('ViceExtTouchAutoAim');
        log('vc-touch: apuntado fijado ' + Math.round(AIM_HOLD_MS / 1000) + 's');
      }
    }
    if (action === 'gas') {
      if (now - state.lastGasTap < 360) {
        state.pitch = 128;
        state.pitchUntil = now + 900;
        state.gasUntil = now + 900;
        state.lastGasTap = 0;
      } else {
        state.lastGasTap = now;
        state.gasUntil = now + 260;
      }
    }
    if (action === 'brake') {
      if (now - state.lastBrakeTap < 360) {
        state.pitch = -128;
        state.pitchUntil = now + 900;
        state.brakeUntil = now + 900;
        state.lastBrakeTap = 0;
      } else {
        state.lastBrakeTap = now;
        state.brakeUntil = now + 260;
      }
    }
    if (action === 'mission') state.missionUntil = now + 450;
    if (action === 'camera') state.selectUntil = now + 450;
    if (action === 'enter' || action === 'exit') state.triangleUntil = now + 450;
    if (action === 'pause') { log('vc-touch: pausa pedida'); callModule('ViceExtTouchPause'); }
    if (action === 'vehicle-left') state.moveX = -128;
    if (action === 'vehicle-right') state.moveX = 128;
    if (action === 'rotate-left') state.rotate = -1;
    if (action === 'rotate-right') state.rotate = 1;

  }

  function actionUp(key, action) {
    if (action === 'vehicle-left' || action === 'vehicle-right') state.moveX = 0;
    if (action === 'rotate-left' || action === 'rotate-right') state.rotate = 0;
  }

  // Pausa/reanudar o un pointerup perdido (cambio de pantalla completa, irse
  // de la app) dejaban un id "pillado": la capa ignoraba todo toque nuevo
  // hasta salir de pantalla completa. Con el registro de punteros vivos se
  // limpia solo en el siguiente frame.
  function sanitizePointers() {
    if (state.moveId !== null && !state.down.has(state.moveId)) {
      state.moveId = null;
      state.moveX = 0;
      state.moveY = 0;
      state.dragStart = null;
      state.stick.hidden = true;
    }
    if (state.lookId !== null && !state.down.has(state.lookId)) {
      state.lookId = null;
      state.lookStart = null;
    }
    if (state.menuPointer !== null && !state.down.has(state.menuPointer)) {
      state.menuPointer = null;
      state.menuGesture = null;
    }
  }

  function onPointerTrackDown(event) { state.down.add(event.pointerId); }
  function onPointerTrackUp(event) { state.down.delete(event.pointerId); }

  function readPadMode() {
    const module = getModule();
    if (!module) return;
    const fn = typeof module.ViceExtTouchPadMode === 'function'
      ? module.ViceExtTouchPadMode : module._ViceExtTouchPadMode;
    if (typeof fn === 'function') state.padMode = leer('ViceExtTouchPadMode') | 0;
  }

  function buildButtons(now) {
    let buttons = 0;
    const classic = state.padMode === 1 || state.padMode === 3;
    if (now < state.confirmUntil) buttons |= CROSS;
    if (state.press.has('onfoot-run')) buttons |= CROSS;
    if (now < state.jumpUntil) buttons |= SQUARE;
    if (now < state.crouchUntil) buttons |= LEFT_SHOCK;
    // Disparar/puñetazo = Circle; apuntar = el boton que el motor lee como
    // objetivo a pie segun el modo (clasico L2, alternativo R1). Antes se
    // mandaba L1+R1, que en modo clasico es mirar a los lados: no apuntaba.
    if (state.press.has('onfoot-action')) buttons |= CIRCLE;
    if ((state.press.has('onfoot-aim') || now < state.aimUntil) && state.firearm) buttons |= classic ? LEFT2 : RIGHT1;
    // El motor elige los bits por modo de control (CURMODE). En móvil el
    // modo clásico (1/3) usa gas R2, freno L2 y freno de mano Cross; en el
    // otro (0/2) gas Cross, freno Square y freno de mano R1. Con los bits
    // cruzados, "derrapar" movía la cámara: R1 es mirar a la derecha en el
    // modo clásico.
    if (state.press.has('vehicle-gas') || now < state.gasUntil) buttons |= classic ? RIGHT2 : CROSS;
    if (state.press.has('vehicle-brake') || now < state.brakeUntil) buttons |= classic ? LEFT2 : SQUARE;
    if (state.press.has('vehicle-handbrake')) buttons |= classic ? CROSS : RIGHT1;
    if (state.press.has('vehicle-horn')) buttons |= LEFT_SHOCK;
    if (now < state.missionUntil) buttons |= RIGHT_SHOCK;
    if (state.press.has('vehicle-fire')) buttons |= CIRCLE;
    if (state.press.has('vehicle-fire-mg')) buttons |= RIGHT1;
    if (now < state.selectUntil) buttons |= SELECT;
    if (now < state.triangleUntil) buttons |= TRIANGLE;
    return buttons;
  }

  function sendFrame(now) {
    const module = getModule();
    if (!module || typeof module._ViceExtTouchFrame !== 'function' || !window.__vcFirstFrame) return;
    const flight = state.mode === 'vehicle' && (state.vehicleName === 'heli' || state.vehicleName === 'plane');
    let leftX = state.moveX;
    let leftY = state.mode === 'vehicle' && !flight ? 0 : state.moveY;
    let lookX = state.lookX;
    let lookY = state.lookY;
    if (state.mode === 'vehicle') {
      if (state.rotate && flight) leftX = state.rotate * 128;
      if (state.rotate && (state.vehicleName === 'tank' || state.vehicleName === 'firetruck')) lookX = state.rotate * 128;
      if (now < state.pitchUntil) leftY = state.pitch;
    }
    // Plan 64.0 T6: instrumentacion agregada (cada 240 envios) del coste de
    // cruce a WASM; no cambia el ritmo de envio, solo mide.
    const t0 = nowMs();
    module._ViceExtTouchFrame(
      Math.round(leftX), Math.round(leftY), 0, 0,
      Math.round(clamp(lookX, -128, 128)), Math.round(clamp(lookY, -128, 128)),
      buildButtons(now) >>> 0
    );
    const dt = nowMs() - t0;
    state.perf.n += 1;
    state.perf.sum += dt;
    if (dt > state.perf.max) state.perf.max = dt;
    if (state.perf.n >= 240) {
      log(`vc-touch: perf camFrame avg=${(state.perf.sum / state.perf.n).toFixed(2)}ms max=${state.perf.max.toFixed(2)}ms fpsEma=${state.perf.fpsEma.toFixed(1)}`);
      state.perf.n = 0; state.perf.sum = 0; state.perf.max = 0;
    }
    if (now >= state.lookUntil) {
      const k = Math.pow(0.55, Math.max(1, state.frameDt) / 16.7);
      state.lookX *= k;
      state.lookY *= k;
      if (Math.abs(state.lookX) < 0.5) state.lookX = 0;
      if (Math.abs(state.lookY) < 0.5) state.lookY = 0;
    }
  }

  function frame() {
    if (!state.enabled) return;
    try {
      frameStep();
    } catch (e) {
      if (!state.frameError) {
        state.frameError = true;
        log('vc-touch: fallo en el bucle ' + ((e && e.message) || e));
      }
    }
    state.frameId = requestAnimationFrame(frame);
  }

  // Un pointerup que se pierde (pausa, mapa, cambio de pantalla completa,
  // volver de segundo plano) dejaba el boton dentro de `state.press` para
  // siempre: el motor seguia viendo ese boton pulsado y el resto del HUD
  // parecia muerto. Sin ningun puntero vivo se suelta todo.
  function healStuckButtons(now) {
    if (!state.press.size || state.down.size) return;
    state.press.clear();
    for (const element of layer.querySelectorAll('.vc-touch-btn.is-active')) element.classList.remove('is-active');
    if (now - state.lastSweepLog > 2000) {
      state.lastSweepLog = now;
      log('vc-touch: botones sueltos sin puntero');
    }
  }

  function frameStep() {
    rememberBridge();
    const now = nowMs();
    if (state.perf.lastFrame) {
      state.frameDt = Math.max(1, now - state.perf.lastFrame);
      const fps = 1000 / state.frameDt;
      state.perf.fpsEma = state.perf.fpsEma ? state.perf.fpsEma * 0.95 + fps * 0.05 : fps;
    }
    state.perf.lastFrame = now;
    if (now >= state.contextAt) {
      state.contextAt = now + 100;
      readContext();
      readPadMode();
    }
    // Mientras la mira esta fijada se reengancha el objetivo cada 400 ms: si
    // el enemigo se mueve, muere o aparece otro mas cerca, la mira salta a el
    // en vez de quedarse apuntando al aire.
    if (now < state.aimUntil && now >= state.aimTickAt) {
      state.aimTickAt = now + 400;
      callModule('ViceExtTouchAutoAim');
    }
    if (state.aimButton) state.aimButton.classList.toggle('is-active', now < state.aimUntil);
    sanitizePointers();
    healStuckButtons(now);
    updateMenuHold(now);
    syncVisibility();
    sendFrame(now);
  }

  function startLoop() {
    if (state.frameId) return;
    state.frameId = requestAnimationFrame(frame);
  }

  function stopLoop() {
    if (state.frameId) cancelAnimationFrame(state.frameId);
    state.frameId = 0;
  }

  function releaseAll() {
    state.press.clear();
    state.moveId = null;
    state.lookId = null;
    state.moveX = 0;
    state.moveY = 0;
    state.lookX = 0;
    state.lookY = 0;
    state.lookUntil = 0;
    state.rotate = 0;
    state.pitch = 0;
    state.pitchUntil = 0;
    state.missionUntil = 0;
    state.gasUntil = 0;
    state.brakeUntil = 0;
    state.stick.hidden = true;
    state.confirmUntil = 0;
    for (const element of layer.querySelectorAll('.vc-touch-btn.is-active')) element.classList.remove('is-active');
  }

  function onMoveDown(event) {
    const flight = state.mode === 'vehicle' && (state.vehicleName === 'heli' || state.vehicleName === 'plane');
    if ((state.mode !== 'onfoot' && !flight) || state.moveId !== null) return;
    event.preventDefault();
    state.moveId = event.pointerId;
    grab(state.moveZone, event.pointerId);
    const rect = layer.getBoundingClientRect();
    state.dragStart = { x: event.clientX - rect.left, y: event.clientY - rect.top };
    state.tapStart = nowMs();
    state.tapMoved = false;
    state.stick.style.left = state.dragStart.x + 'px';
    state.stick.style.top = state.dragStart.y + 'px';
    state.stick.hidden = false;
    state.moveX = 0;
    state.moveY = 0;
  }

  function onMoveMove(event) {
    if (event.pointerId !== state.moveId || !state.dragStart) return;
    event.preventDefault();
    const rect = layer.getBoundingClientRect();
    let dx = event.clientX - rect.left - state.dragStart.x;
    let dy = event.clientY - rect.top - state.dragStart.y;
    const distance = Math.hypot(dx, dy);
    // Zona muerta del stick: hasta MOVE_SLOP_PX el dedo no mueve al personaje.
    if (distance > MOVE_SLOP_PX) {
      state.tapMoved = true;
      const k = (distance - MOVE_SLOP_PX) / distance;
      dx *= k;
      dy *= k;
    } else {
      dx = 0;
      dy = 0;
    }
    const radius = 54;
    const travel = Math.hypot(dx, dy);
    const scale = travel > radius ? radius / travel : 1;
    state.moveX = clamp((dx * scale / radius) * 128, -128, 128);
    state.moveY = clamp((dy * scale / radius) * 128, -128, 128);
    state.stick.querySelector('i').style.transform = `translate(${dx * scale}px, ${dy * scale}px)`;
  }

  function onMoveUp(event) {
    if (event.pointerId !== state.moveId) return;
    const now = nowMs();
    if (!state.tapMoved && now - state.tapStart < TAP_MAX_MS) {
      if (now - state.lastLeftTap < 360) {
        crouchPulse();
        state.lastLeftTap = 0;
      } else {
        state.lastLeftTap = now;
      }
    }
    state.moveId = null;
    state.moveX = 0;
    state.moveY = 0;
    state.stick.hidden = true;
    state.stick.querySelector('i').style.transform = '';
  }

  function onLookDown(event) {
    if (state.mode === 'menu' || state.mode === 'cutscene' || state.lookId !== null) return;
    event.preventDefault();
    state.lookId = event.pointerId;
    grab(state.lookZone, event.pointerId);
    state.lookStart = { x: event.clientX, y: event.clientY, time: nowMs() };
    state.lookMoved = false;
    state.lookTravel = 0;
    state.lookEngaged = false;
  }

  function onLookMove(event) {
    if (event.pointerId !== state.lookId || !state.lookStart) return;
    event.preventDefault();
    const dx = event.clientX - state.lookStart.x;
    const dy = event.clientY - state.lookStart.y;
    state.lookStart.x = event.clientX;
    state.lookStart.y = event.clientY;
    // Zona muerta de la camara: la deriva del dedo no gira nada hasta que el
    // recorrido pasa de LOOK_SLOP_PX.
    state.lookTravel = (state.lookTravel || 0) + Math.abs(dx) + Math.abs(dy);
    if (!state.lookEngaged) {
      if (state.lookTravel < LOOK_SLOP_PX) return;
      state.lookEngaged = true;
      state.lookMoved = true;
    }
    state.lookX = clamp(state.lookX + dx * sensitivity, -128, 128);
    state.lookY = clamp(state.lookY + dy * sensitivity, -128, 128);
    state.lookUntil = nowMs() + 60;
  }

  function onLookUp(event) {
    if (event.pointerId !== state.lookId) return;
    state.lookId = null;
    state.lookStart = null;
    state.lookMoved = false;
    state.lookTravel = 0;
    state.lookEngaged = false;
  }

  function onSkipDown(event) {
    if (state.mode !== 'cutscene') return;
    event.preventDefault();
    event.stopPropagation();
    callModule('ViceExtTouchSkip');
  }

  function bindHudSwipe(zone, exportName) {
    let pointerId = null;
    let startX = 0;
    let startY = 0;
    zone.addEventListener('pointerdown', (event) => {
      pointerId = null;
      if (state.mode === 'menu' || state.mode === 'cutscene') return;
      event.preventDefault();
      event.stopPropagation();
      pointerId = event.pointerId;
      startX = event.clientX;
      startY = event.clientY;
      grab(zone, pointerId);
    });
    zone.addEventListener('pointerup', (event) => {
      if (pointerId === null || event.pointerId !== pointerId) return;
      event.preventDefault();
      event.stopPropagation();
      const dx = event.clientX - startX;
      const dy = event.clientY - startY;
      pointerId = null;
      const direction = Math.abs(dx) > 24 && Math.abs(dx) > Math.abs(dy) ? (dx > 0 ? 1 : -1) : 1;
      callModule(exportName, direction);
    });
    zone.addEventListener('pointercancel', () => { pointerId = null; });
  }

  function bindMapTap() {
    let pointerId = null;
    let startX = 0;
    let startY = 0;
    state.radarZone.addEventListener('pointerdown', (event) => {
      pointerId = null;
      if (state.mode === 'menu' || state.mode === 'cutscene') return;
      event.preventDefault();
      event.stopPropagation();
      pointerId = event.pointerId;
      startX = event.clientX;
      startY = event.clientY;
      grab(state.radarZone, pointerId);
    });
    state.radarZone.addEventListener('pointerup', (event) => {
      if (pointerId === null || event.pointerId !== pointerId) return;
      event.preventDefault();
      event.stopPropagation();
      const dx = event.clientX - startX;
      const dy = event.clientY - startY;
      pointerId = null;
      if (Math.hypot(dx, dy) < 18) { log('vc-touch: mapa pedido'); callModule('ViceExtTouchOpenMap'); }
    });
    state.radarZone.addEventListener('pointercancel', () => { pointerId = null; });
  }

  function onButtonDown(event) {
    const target = event.currentTarget;
    const key = target.dataset.key;
    const action = target.dataset.action;
    if (!key || !action) return;
    event.preventDefault();
    event.stopPropagation();
    // 79.0: una linea por pulsacion (como mucho 2/s) para separar "el toque no
    // llega" de "el motor lo ignora" en el proximo log del movil.
    const nowPress = nowMs();
    if (nowPress - state.lastBtnLog > 500) {
      state.lastBtnLog = nowPress;
      log('vc-touch: boton ' + key + ' modo=' + state.mode + ' oculto=' + (layer.hidden ? 1 : 0) +
        ' ctx=' + state.context);
    }
    state.press.add(key);
    grab(target, event.pointerId);
    target.classList.add('is-active');
    actionDown(key, action);
  }

  function onButtonUp(event) {
    const target = event.currentTarget;
    const key = target.dataset.key;
    const action = target.dataset.action;
    if (!key || !action) return;
    if (state.press.delete(key)) target.classList.remove('is-active');
    actionUp(key, action);
  }

  function onContextMenu(event) {
    event.preventDefault();
  }

  // Pantalla completa + orientacion paisaje + Wake Lock (móvil). El Fullscreen
  // API exige activacion del usuario: se pide al PRIMER toque sobre la capa
  // (gesto real) y el boton ⛶ lo pide de nuevo cuando el usuario lo pulsa.
  // En iPhone (Safari, sin Element.requestFullscreen) no hay a pantalla
  // completa desde la web: se avisa una vez y el juego sigue funcionando.
  // Ctrl+N y demas atajos reservados del navegador NO se pueden bloquear
  // desde la pagina; en móvil no existen y los gestos equivalents
  // (pull-to-refresh, menu de llamada, zoom) se bloquean con CSS/JS abajo.
  let fsTried = false;
  let wakeLockSentinel = null;

  function lockLandscape() {
    try {
      if (screen && screen.orientation && typeof screen.orientation.lock === 'function')
        screen.orientation.lock('landscape').catch(() => {});
    } catch (e) { /* sin soporte: no importa */ }
  }

  function goFullscreen() {
    if (document.fullscreenElement || document.webkitFullscreenElement) {
      lockLandscape();
      return;
    }
    const el = doc.documentElement;
    const req = el.requestFullscreen || el.webkitRequestFullscreen || el.msRequestFullscreen;
    if (typeof req !== 'function') {
      if (!fsTried) { fsTried = true; log('vc-touch: fullscreen no disponible en este navegador/móvil'); }
      return;
    }
    try {
      const p = req.call(el);
      if (p && typeof p.catch === 'function') {
        p.then(lockLandscape).catch(() => { /* el usuario lo rechazo: reintenta el boton */ });
      } else {
        lockLandscape();
      }
    } catch (e) { /* gesto sin activacion: lo reintenta el boton ⛶ */ }
  }

  // Feedback punto 2: fullscreen al tocar en cualquier parte y en cualquier
  // momento (también en carga/portada), no solo tras el primer frame. El
  // listener es permanente y reintenta si el usuario salió de fullscreen.
  function onAnyGesture() {
    if (state.enabled) goFullscreen();
  }

  function onGestureStart(event) {
    if (!layer.hidden) event.preventDefault();   // zoom con pellizco (iOS Safari)
  }

  function onCanvasTouch(event) {
    if (!state.enabled) {
      if (!state.enabledLogged) {
        state.enabledLogged = true;
        log('vc-touch: toque sin capa enabled=0');
      }
      return false;
    }
    const target = event.target;
    if (!target || !root.contains(target)) return false;
    if (target.closest && target.closest('button, a, input')) return false;
    event.preventDefault();
    return true;
  }

  function onCanvasTouchStart(event) {
    if (!onCanvasTouch(event)) return;
    if (!event.touches || event.touches.length !== 1) return;
    if (!menuOpen()) return;
    const t = event.changedTouches && event.changedTouches[0];
    if (!t) return;
    state.menuTouch = { id: t.identifier, x: t.clientX, y: t.clientY, at: nowMs(), moved: false };
  }

  function onCanvasTouchMove(event) {
    const gesture = state.menuTouch;
    if (gesture && event.changedTouches) {
      const t = event.changedTouches[0];
      if (t && t.identifier === gesture.id && Math.hypot(t.clientX - gesture.x, t.clientY - gesture.y) > MENU_SLIDE_PX) gesture.moved = true;
    }
    onCanvasTouch(event);
  }

  function onCanvasTouchEnd(event) {
    const gesture = state.menuTouch;
    state.menuTouch = null;
    if (!onCanvasTouch(event)) return;
    if (!gesture || gesture.moved) return;
    const t = event.changedTouches && event.changedTouches[0];
    if (t && t.identifier !== gesture.id) return;
    if (nowMs() - gesture.at > MENU_TAP_MS) return;
    if (!menuOpen()) {
      log(`vc-touch: tap sin menu ctx=${state.context} modo=${state.mode} juego=${state.playing ? 1 : 0}`);
      return;
    }
    menuActivate('touch');
  }

  async function requestWakeLock() {
    try {
      if (wakeLockSentinel) return;
      if (navigator && navigator.wakeLock && typeof navigator.wakeLock.request === 'function')
        wakeLockSentinel = await navigator.wakeLock.request('screen');
    } catch (e) { /* sin permiso/bateria: no importa */ }
  }

  function onVisibility() {
    if (document.hidden) {
      releaseAll();
      if (wakeLockSentinel) { try { wakeLockSentinel.release(); } catch (e) {} wakeLockSentinel = null; }
    } else {
      requestWakeLock();
    }
  }

  function onBlur() {
    releaseAll();
  }

  function bindWindow() {
    if (state.bound) return;
    state.bound = true;
    window.addEventListener('pointerdown', onAnyGesture, { capture: true });
    window.addEventListener('pointerdown', onPointerTrackDown, { capture: true });
    window.addEventListener('pointerup', onPointerTrackUp, { capture: true });
    window.addEventListener('pointercancel', onPointerTrackUp, { capture: true });
    window.addEventListener('gesturestart', onGestureStart, { passive: false });
    window.addEventListener('touchstart', onCanvasTouchStart, { capture: true, passive: false });
    window.addEventListener('touchmove', onCanvasTouchMove, { capture: true, passive: false });
    window.addEventListener('touchend', onCanvasTouchEnd, { capture: true, passive: false });
    window.addEventListener('touchcancel', onCanvasTouchEnd, { capture: true, passive: false });
    window.addEventListener('pointerdown', onMenuPointerDown, true);
    window.addEventListener('pointermove', onMenuPointerMove, true);
    window.addEventListener('pointerup', onMenuPointerUp, true);
    window.addEventListener('pointercancel', onMenuPointerCancel, true);
    document.addEventListener('visibilitychange', onVisibility);
    window.addEventListener('blur', onBlur);
    log('vc-touch: oyentes listos');
  }

  function mount() {
    if (state.mounted) return;
    state.mounted = true;
    state.motorLogged = probeModule('capa', false);
    root.appendChild(rotateEl);
    root.appendChild(layer);
    applyLayout();
    state.moveZone.addEventListener('pointerdown', onMoveDown);
    state.moveZone.addEventListener('pointermove', onMoveMove);
    state.moveZone.addEventListener('pointerup', onMoveUp);
    state.moveZone.addEventListener('pointercancel', onMoveUp);
    state.lookZone.addEventListener('pointerdown', onLookDown);
    state.lookZone.addEventListener('pointermove', onLookMove);
    state.lookZone.addEventListener('pointerup', onLookUp);
    state.lookZone.addEventListener('pointercancel', onLookUp);
    state.skipZone.addEventListener('pointerdown', onSkipDown);
    bindHudSwipe(state.weaponZone, 'ViceExtTouchWeapon');
    bindHudSwipe(state.radioZone, 'ViceExtTouchRadio');
    bindMapTap();
    layer.addEventListener('contextmenu', onContextMenu);
    bindWindow();
    requestWakeLock();
    for (const element of layer.querySelectorAll('.vc-touch-btn')) {
      element.addEventListener('pointerdown', onButtonDown);
      element.addEventListener('pointerup', onButtonUp);
      element.addEventListener('pointercancel', onButtonUp);
      element.addEventListener('lostpointercapture', onButtonUp);
    }
    syncVisibility();
    if (state.enabled) startLoop();
  }

  function setEnabled(on) {
    const next = !!on;
    if (state.enabled === next) {
      syncVisibility();
      return next;
    }
    state.enabled = next;
    root.dataset.touch = next ? '1' : '0';
    if (!next) {
      state.menuPointer = null;
      state.menuGesture = null;
      state.menuTouch = null;
      releaseAll();
      stopLoop();
    } else {
      startLoop();
    }
    rememberBridge();
    syncVisibility();
    return next;
  }

  function context() {
    return readContext();
  }

  function destroy() {
    stopLoop();
    setEnabled(false);
    window.removeEventListener('pointerdown', onAnyGesture, { capture: true });
    window.removeEventListener('pointerdown', onMenuPointerDown, true);
    window.removeEventListener('pointermove', onMenuPointerMove, true);
    window.removeEventListener('pointerup', onMenuPointerUp, true);
    window.removeEventListener('pointercancel', onMenuPointerCancel, true);
    document.removeEventListener('visibilitychange', onVisibility);
    window.removeEventListener('blur', onBlur);
    if (layer.parentNode) layer.remove();
    if (rotateEl.parentNode) rotateEl.remove();
    state.mounted = false;
  }

  if (state.enabled) root.dataset.touch = '1';
  bindWindow();
  state.layer.dataset.mode = state.mode;
  state.layer.dataset.vehicle = state.vehicleName;
  state.layer.dataset.mission = '0';
  return {
    mount,
    setEnabled,
    context,
    destroy,
    syncVisibility,
    get enabled() { return state.enabled; },
    get mode() { return state.mode; },
    get layer() { return layer; },
  };
}
