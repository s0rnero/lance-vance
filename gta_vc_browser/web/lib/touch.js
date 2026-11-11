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
    baseLog(...args);
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
    lastGasTap: 0,
    lastBrakeTap: 0,
    gasUntil: 0,
    brakeUntil: 0,
    pitch: 0,
    pitchUntil: 0,
    missionUntil: 0,
    rotate: 0,

    menuPointer: null,
    menuPoint: null,
    menuStart: null,
    menuPulseBit: 0,
    menuPulseUntil: 0,
    menuPending: null,
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

  function tapMenu(point) {
    const module = getModule();
    if (!module || typeof module._ViceExtTouchMenuTap !== 'function') return;
    module._ViceExtTouchMenuTap(
      Math.round(clamp(point.x, 0, 1) * 10000),
      Math.round(clamp(point.y, 0, 1) * 10000)
    );
  }

  function onMenuPointerDown(event) {
    if (!state.enabled || state.mode !== 'menu' || state.menuPointer !== null) return;
    if (event.button !== 0 || !canvas) return;
    const target = event.target;
    if (target !== canvas && !canvas.contains(target)) return;
    const rect = canvas.getBoundingClientRect();
    if (!rect.width || !rect.height) return;
    event.preventDefault();
    event.stopPropagation();
    state.menuPointer = event.pointerId;
    state.menuPoint = {
      x: clamp((event.clientX - rect.left) / rect.width, 0, 1),
      y: clamp((event.clientY - rect.top) / rect.height, 0, 1),
    };
    state.menuStart = { x: event.clientX, y: event.clientY };
  }

  function onMenuPointerUp(event) {
    if (event.pointerId !== state.menuPointer) return;
    event.preventDefault();
    event.stopPropagation();
    const point = state.menuPoint;
    const start = state.menuStart;
    state.menuPointer = null;
    state.menuPoint = null;
    state.menuStart = null;
    if (!point) return;
    // Desliz vertical = scroll siempre. Desliz horizontal y toque = enfocar
    // y, solo si esa fila YA era la enfocada, cambiar el valor.
    if (start) {
      const dx = event.clientX - start.x;
      const dy = event.clientY - start.y;
      if (Math.abs(dy) > 60 && Math.abs(dy) > Math.abs(dx)) {
        menuPulse(dy < 0 ? DPAD_UP : DPAD_DOWN);
        return;
      }
      if (Math.abs(dx) > 60 && Math.abs(dx) > Math.abs(dy)) {
        queueMenuChange(point, dx < 0 ? DPAD_LEFT : DPAD_RIGHT);
        return;
      }
    }
    queueMenuChange(point, DPAD_RIGHT);
  }

  function menuPulse(bit) {
    state.menuPulseBit = bit;
    state.menuPulseUntil = nowMs() + 260;
  }

  function onMenuPointerCancel(event) {
    if (event.pointerId !== state.menuPointer) return;
    event.preventDefault();
    event.stopPropagation();
    state.menuPointer = null;
    state.menuPoint = null;
    state.menuStart = null;
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
    layer.hidden = !state.enabled || !window.__vcFirstFrame || (!state.playing && state.mode !== 'cutscene');
    if (!layer.hidden) state.ready = true;
  }

  function setMode(mode) {
    if (state.mode === mode) return;
    state.menuPointer = null;
    state.menuPoint = null;
    state.menuStart = null;
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
      type = module._ViceExtTouchVehicleType() | 0;
    } else if (module && typeof module._ViceExtTouchNearVehicleType === 'function') {
      type = module._ViceExtTouchNearVehicleType() | 0;
    }
    const previousVehicle = state.vehicleName;
    state.vehicleType = type;
    state.vehicleName = VEHICLE_NAMES[type] || (type ? 'car' : 'onfoot');
    layer.dataset.vehicle = state.vehicleName;
    let armed = 0;
    if (state.context & 2 && module && typeof module._ViceExtTouchArmedVehicle === 'function') {
      armed = module._ViceExtTouchArmedVehicle() | 0;
    }
    state.vehicleArmed = armed;
    layer.dataset.vehicleArmed = armed ? '1' : '0';
    if (previousVehicle !== state.vehicleName) applyLayout();
    let mission = 0;
    if (state.context & 2 && module && typeof module._ViceExtTouchMissionVehicle === 'function') {
      mission = module._ViceExtTouchMissionVehicle() | 0;
    }
    state.vehicleMission = mission;
    layer.dataset.mission = mission ? '1' : '0';
    syncVehicleIcons();
  }

  function readContext() {
    const module = getModule();
    if (!module || typeof module._ViceExtTouchContext !== 'function') return state.context;
    const context = module._ViceExtTouchContext() | 0;
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
    if (context & 128) setMode('cutscene');
    else if (context & 4) setMode('menu');
    else if (context & 2) setMode('vehicle');
    else setMode('onfoot');
    return context;
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
    // Apuntar: engancha el objetivo mas cercano (autoapuntado del motor) y
    // mientras se mantiene pulsado el ped levanta el arma apuntando.
    if (action === 'aim') callModule('ViceExtTouchAutoAim');
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
    if (action === 'pause') callModule('ViceExtTouchPause');
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
      state.menuPoint = null;
      state.menuStart = null;
    }
  }

  function onPointerTrackDown(event) { state.down.add(event.pointerId); }
  function onPointerTrackUp(event) { state.down.delete(event.pointerId); }

  function menuOption() {
    const module = getModule();
    if (!module) return -1;
    const fn = typeof module.ViceExtTouchMenuOption === 'function'
      ? module.ViceExtTouchMenuOption : module._ViceExtTouchMenuOption;
    return typeof fn === 'function' ? fn() : -1;
  }

  // "El recuadro verde" del menú: si la fila tocada NO era la enfocada, el
  // toque solo enfoca; si ya lo estaba, cambia el valor. El foco lo resuelve
  // el motor en el frame siguiente, por eso se confirma con retardo corto.
  function queueMenuChange(point, delta) {
    state.menuPending = { prev: menuOption(), at: nowMs(), delta };
    tapMenu(point);
  }

  function resolveMenuPending(now) {
    const pending = state.menuPending;
    if (!pending || now - pending.at < 120) return;
    state.menuPending = null;
    const cur = menuOption();
    if (pending.prev >= 0 && cur === pending.prev) menuPulse(pending.delta);
  }

  function readPadMode() {
    const module = getModule();
    if (!module) return;
    const fn = typeof module.ViceExtTouchPadMode === 'function'
      ? module.ViceExtTouchPadMode : module._ViceExtTouchPadMode;
    if (typeof fn === 'function') state.padMode = fn() | 0;
  }

  function buildButtons(now) {
    let buttons = 0;
    if (state.press.has('onfoot-run')) buttons |= CROSS;
    if (now < state.jumpUntil) buttons |= SQUARE;
    if (now < state.crouchUntil) buttons |= LEFT_SHOCK;
    // Disparar/puñetazo = Circle; apuntar = el boton que el motor lee como
    // objetivo a pie segun el modo (clasico L2, alternativo R1). Antes se
    // mandaba L1+R1, que en modo clasico es mirar a los lados: no apuntaba.
    if (state.press.has('onfoot-action')) buttons |= CIRCLE;
    if (state.press.has('onfoot-aim') && state.firearm) buttons |= classic ? LEFT2 : RIGHT1;
    // El motor elige los bits por modo de control (CURMODE). En móvil el
    // modo clásico (1/3) usa gas R2, freno L2 y freno de mano Cross; en el
    // otro (0/2) gas Cross, freno Square y freno de mano R1. Con los bits
    // cruzados, "derrapar" movía la cámara: R1 es mirar a la derecha en el
    // modo clásico.
    const classic = state.padMode === 1 || state.padMode === 3;
    if (state.press.has('vehicle-gas') || now < state.gasUntil) buttons |= classic ? RIGHT2 : CROSS;
    if (state.press.has('vehicle-brake') || now < state.brakeUntil) buttons |= classic ? LEFT2 : SQUARE;
    if (state.press.has('vehicle-handbrake')) buttons |= classic ? CROSS : RIGHT1;
    if (state.press.has('vehicle-horn')) buttons |= LEFT_SHOCK;
    if (now < state.missionUntil) buttons |= RIGHT_SHOCK;
    if (state.press.has('vehicle-fire')) buttons |= CIRCLE;
    if (state.press.has('vehicle-fire-mg')) buttons |= RIGHT1;
    if (now < state.selectUntil) buttons |= SELECT;
    if (now < state.triangleUntil) buttons |= TRIANGLE;
    // Puntos 5 y 6, solo gesto sobre el juego (nada de botones nuevos):
    // deslizar mueve el scroll/cambia de fila y tocar el valor pulsa RIGHT.
    // Los bits DPad ya llegan fusionados por TouchFrame.
    if (now < state.menuPulseUntil && state.menuPulseBit === DPAD_UP) buttons |= DPAD_UP;
    if (now < state.menuPulseUntil && state.menuPulseBit === DPAD_DOWN) buttons |= DPAD_DOWN;
    if (now < state.menuPulseUntil && state.menuPulseBit === DPAD_LEFT) buttons |= DPAD_LEFT;
    if (now < state.menuPulseUntil && state.menuPulseBit === DPAD_RIGHT) buttons |= DPAD_RIGHT;
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
    state.lookX *= 0.55;
    state.lookY *= 0.55;
    if (Math.abs(state.lookX) < 0.5) state.lookX = 0;
    if (Math.abs(state.lookY) < 0.5) state.lookY = 0;
  }

  function frame() {
    if (!state.enabled) return;
    rememberBridge();
    const now = nowMs();
    if (state.perf.lastFrame) {
      const fps = 1000 / Math.max(1, now - state.perf.lastFrame);
      state.perf.fpsEma = state.perf.fpsEma ? state.perf.fpsEma * 0.95 + fps * 0.05 : fps;
    }
    state.perf.lastFrame = now;
    if (now >= state.contextAt) {
      state.contextAt = now + 100;
      readContext();
      readPadMode();
    }
    sanitizePointers();
    resolveMenuPending(now);
    syncVisibility();
    sendFrame(now);
    state.frameId = requestAnimationFrame(frame);
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
    state.rotate = 0;
    state.pitch = 0;
    state.pitchUntil = 0;
    state.missionUntil = 0;
    state.gasUntil = 0;
    state.brakeUntil = 0;
    state.stick.hidden = true;
    for (const element of layer.querySelectorAll('.vc-touch-btn.is-active')) element.classList.remove('is-active');
  }

  function onMoveDown(event) {
    const flight = state.mode === 'vehicle' && (state.vehicleName === 'heli' || state.vehicleName === 'plane');
    if ((state.mode !== 'onfoot' && !flight) || state.moveId !== null) return;
    event.preventDefault();
    state.moveId = event.pointerId;
    state.moveZone.setPointerCapture(event.pointerId);
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
    const dx = event.clientX - rect.left - state.dragStart.x;
    const dy = event.clientY - rect.top - state.dragStart.y;
    if (Math.hypot(dx, dy) > 12) state.tapMoved = true;
    const radius = 54;
    const distance = Math.hypot(dx, dy);
    const scale = distance > radius ? radius / distance : 1;
    state.moveX = clamp((dx * scale / radius) * 128, -128, 128);
    state.moveY = clamp((dy * scale / radius) * 128, -128, 128);
    state.stick.querySelector('i').style.transform = `translate(${dx * scale}px, ${dy * scale}px)`;
  }

  function onMoveUp(event) {
    if (event.pointerId !== state.moveId) return;
    const now = nowMs();
    if (!state.tapMoved && now - state.tapStart < 260) {
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
    state.lookZone.setPointerCapture(event.pointerId);
    state.lookStart = { x: event.clientX, y: event.clientY, time: nowMs() };
    state.lookMoved = false;
  }

  function onLookMove(event) {
    if (event.pointerId !== state.lookId || !state.lookStart) return;
    event.preventDefault();
    const dx = event.clientX - state.lookStart.x;
    const dy = event.clientY - state.lookStart.y;
    if (Math.abs(dx) + Math.abs(dy) > 5) state.lookMoved = true;
    state.lookStart.x = event.clientX;
    state.lookStart.y = event.clientY;
    state.lookX = clamp(state.lookX + dx * sensitivity, -128, 128);
    state.lookY = clamp(state.lookY + dy * sensitivity, -128, 128);
  }

  function onLookUp(event) {
    if (event.pointerId !== state.lookId) return;
    state.lookId = null;
    state.lookStart = null;
    state.lookMoved = false;
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
      if (state.mode === 'menu' || state.mode === 'cutscene') return;
      event.preventDefault();
      event.stopPropagation();
      pointerId = event.pointerId;
      startX = event.clientX;
      startY = event.clientY;
      zone.setPointerCapture(pointerId);
    });
    zone.addEventListener('pointerup', (event) => {
      if (event.pointerId !== pointerId) return;
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
      if (state.mode === 'menu' || state.mode === 'cutscene') return;
      event.preventDefault();
      event.stopPropagation();
      pointerId = event.pointerId;
      startX = event.clientX;
      startY = event.clientY;
      state.radarZone.setPointerCapture(pointerId);
    });
    state.radarZone.addEventListener('pointerup', (event) => {
      if (event.pointerId !== pointerId) return;
      event.preventDefault();
      event.stopPropagation();
      const dx = event.clientX - startX;
      const dy = event.clientY - startY;
      pointerId = null;
      if (Math.hypot(dx, dy) < 18) callModule('ViceExtTouchOpenMap');
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
    target.setPointerCapture(event.pointerId);
    state.press.add(key);
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

  function mount() {
    if (state.mounted) return;
    state.mounted = true;
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
    window.addEventListener('pointerdown', onAnyGesture, { capture: true });
    window.addEventListener('pointerdown', onPointerTrackDown, { capture: true });
    window.addEventListener('pointerup', onPointerTrackUp, { capture: true });
    window.addEventListener('pointercancel', onPointerTrackUp, { capture: true });
    window.addEventListener('gesturestart', onGestureStart, { passive: false });
    requestWakeLock();
    for (const element of layer.querySelectorAll('.vc-touch-btn')) {
      element.addEventListener('pointerdown', onButtonDown);
      element.addEventListener('pointerup', onButtonUp);
      element.addEventListener('pointercancel', onButtonUp);
      element.addEventListener('lostpointercapture', onButtonUp);
    }
    window.addEventListener('pointerdown', onMenuPointerDown, true);
    window.addEventListener('pointerup', onMenuPointerUp, true);
    window.addEventListener('pointercancel', onMenuPointerCancel, true);
    document.addEventListener('visibilitychange', onVisibility);
    window.addEventListener('blur', onBlur);
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
      state.menuPoint = null;
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
    window.removeEventListener('pointerup', onMenuPointerUp, true);
    window.removeEventListener('pointercancel', onMenuPointerCancel, true);
    document.removeEventListener('visibilitychange', onVisibility);
    window.removeEventListener('blur', onBlur);
    if (layer.parentNode) layer.remove();
    if (rotateEl.parentNode) rotateEl.remove();
    state.mounted = false;
  }

  if (state.enabled) root.dataset.touch = '1';
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
