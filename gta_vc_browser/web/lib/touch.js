const CROSS = 1 << 0;
const SQUARE = 1 << 1;
const CIRCLE = 1 << 2;
const TRIANGLE = 1 << 3;
const LEFT1 = 1 << 4;
const RIGHT1 = 1 << 5;
const LEFT2 = 1 << 6;
const RIGHT2 = 1 << 7;
const SELECT = 1 << 8;
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
  'onfoot-enter': [93, 47],
  'vehicle-left': [7, 82],
  'vehicle-right': [17, 82],
  'vehicle-brake': [93, 83],
  'vehicle-gas': [76, 83],
  'vehicle-handbrake': [94, 63],
  'vehicle-camera': [88, 33],
  'vehicle-horn': [94, 33],
  'vehicle-exit': [94, 44],
  'vehicle-rotate-left': [80, 63],
  'vehicle-rotate-right': [90, 63],
  'vehicle-mission': [7, 47],
  'vehicle-fire-left': [8, 62],
  'vehicle-fire-center': [90, 63],
  'vehicle-fire-right': [15, 62],
};

const VEHICLE_LAYOUT = {
  bike: { 'vehicle-fire-center': [76, 62] },
  tank: { 'vehicle-fire-center': [94, 53] },
  firetruck: { 'vehicle-fire-center': [94, 53] },
  heli: {
    'vehicle-fire-center': [94, 53],
    'vehicle-fire-mg': [84, 53],
  },
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
  const log = options.log || (() => {});
  const getModule = options.getModule || (() => (typeof Module !== 'undefined' ? Module : null));
  const iconBase = String(options.iconBase || '/touch/icons/').replace(/\/?$/, '/');
  const sensitivity = Number(options.sensitivity || 0.9);
  const state = {
    enabled: !!options.enabled,
    mounted: false,
    ready: false,
    bridgeEnabled: false,
    bridgeAutoaim: false,
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
    driveSide: null,
    menuPointer: null,
    menuPoint: null,
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
    ${button('onfoot-action', 'onfoot', 'action', 'onfoot-punch', 'Acción')}
    ${button('onfoot-enter', 'onfoot', 'enter', 'onfoot-enter-exit', 'Entrar o salir')}
    ${button('vehicle-left', 'vehicle', 'vehicle-left', 'vehicle-left', 'Izquierda', 'car bike boat tank firetruck')}
    ${button('vehicle-right', 'vehicle', 'vehicle-right', 'vehicle-right', 'Derecha', 'car bike boat tank firetruck')}
    ${button('vehicle-brake', 'vehicle', 'brake', 'vehicle-brake', 'Frenar o descender', 'car bike boat heli plane tank firetruck')}
    ${button('vehicle-gas', 'vehicle', 'gas', 'vehicle-accelerate', 'Acelerar o ascender', 'car bike boat heli plane tank firetruck')}
    ${button('vehicle-handbrake', 'vehicle', 'handbrake', 'vehicle-handbrake', 'Freno de mano', 'car bike boat firetruck')}
    ${button('vehicle-camera', 'vehicle', 'camera', 'vehicle-camera', 'Cambiar cámara', 'car bike boat heli plane tank firetruck')}
    ${button('vehicle-horn', 'vehicle', 'horn', 'vehicle-horn-siren', 'Claxon o sirena', 'car bike boat firetruck')}
    ${button('vehicle-exit', 'vehicle', 'exit', 'vehicle-exit-car', 'Salir del vehículo', 'car bike boat heli plane tank firetruck')}
    ${button('vehicle-rotate-left', 'vehicle', 'rotate-left', 'vehicle-rotate-left', 'Rotar a la izquierda', 'heli plane tank firetruck')}
    ${button('vehicle-rotate-right', 'vehicle', 'rotate-right', 'vehicle-rotate-right', 'Rotar a la derecha', 'heli plane tank firetruck')}
    ${button('vehicle-fire-mg', 'vehicle', 'fire-mg', 'vehicle-shoot', 'Metralleta', 'heli plane')}
    ${button('vehicle-mission', 'vehicle', 'mission', 'vehicle-activate-mission', 'Misión de vehículo')}
    ${button('vehicle-fire-left', 'vehicle', 'fire-left', 'vehicle-fire-left', 'Disparar a la izquierda', 'car bike boat')}
    ${button('vehicle-fire-center', 'vehicle', 'fire-center', 'vehicle-fire-center', 'Disparar al frente', 'bike heli tank firetruck')}
    ${button('vehicle-fire-right', 'vehicle', 'fire-right', 'vehicle-fire-right', 'Disparar a la derecha', 'car bike boat')}
    <div class="vc-touch-rotate">Gira el dispositivo</div>
  `;

  state.layer = layer;
  state.stick = layer.querySelector('.vc-touch-stick');
  state.moveZone = layer.querySelector('.vc-touch-move');
  state.lookZone = layer.querySelector('.vc-touch-look');
  state.skipZone = layer.querySelector('.vc-touch-skip');
  state.radarZone = layer.querySelector('.vc-touch-radar');
  state.weaponZone = layer.querySelector('.vc-touch-weapon');
  state.radioZone = layer.querySelector('.vc-touch-radio');

  function syncActionIcon() {
    const action = layer.querySelector('[data-key="onfoot-action"]');
    if (!action) return;
    const image = action.querySelector('img');
    const name = state.armed ? 'onfoot-shoot' : 'onfoot-punch';
    if (image) {
      const source = iconBase + encodeURIComponent(name) + '.png';
      if (image.getAttribute('src') !== source) image.src = source;
    }
    action.setAttribute('aria-label', state.armed ? 'Disparar' : 'Puñetazo');
  }

  function syncVehicleIcons() {
    const exit = layer.querySelector('[data-key="vehicle-exit"] img');
    const enter = layer.querySelector('[data-key="onfoot-enter"] img');
    const detailed = state.vehicleType && VEHICLE_ICONS.has(state.vehicleName);
    const exitName = detailed ? `vehicle-exit-${state.vehicleName}` : 'vehicle-exit-car';
    const enterName = detailed ? `vehicle-enter-${state.vehicleName}` : 'onfoot-enter-exit';
    if (exit) {
      const source = iconBase + encodeURIComponent(exitName) + '.png';
      if (exit.getAttribute('src') !== source) exit.src = source;
    }
    if (enter) {
      const source = iconBase + encodeURIComponent(enterName) + '.png';
      if (enter.getAttribute('src') !== source) enter.src = source;
    }
  }

  function callModule(name, ...args) {
    const module = getModule();
    if (!module || typeof module[name] !== 'function') return;
    module[name](...args);
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
  }

  function onMenuPointerUp(event) {
    if (event.pointerId !== state.menuPointer) return;
    event.preventDefault();
    event.stopPropagation();
    const point = state.menuPoint;
    state.menuPointer = null;
    state.menuPoint = null;
    if (point) tapMenu(point);
  }

  function onMenuPointerCancel(event) {
    if (event.pointerId !== state.menuPointer) return;
    event.preventDefault();
    event.stopPropagation();
    state.menuPointer = null;
    state.menuPoint = null;
  }

  function applyLayout() {
    const override = VEHICLE_LAYOUT[state.vehicleName] || null;
    for (const element of layer.querySelectorAll('[data-key]')) {
      const key = element.dataset.key;
      const point = (override && override[key]) || DEFAULT_LAYOUT[key];
      if (!point) continue;
      element.style.left = point[0] + '%';
      element.style.top = point[1] + '%';
    }
  }

  function syncVisibility() {
    layer.hidden = !state.enabled || !window.__vcFirstFrame || !state.playing;
    if (!layer.hidden) state.ready = true;
  }

  function setMode(mode) {
    if (state.mode === mode) return;
    state.menuPointer = null;
    state.menuPoint = null;
    state.mode = mode;
    layer.dataset.mode = mode;
    if (mode !== 'vehicle') {
      state.rotate = 0;
      state.driveSide = null;
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
      state.vehicleTypeAt = nowMs() + 250;
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
    if (action === 'action') {
      if (state.firearm) callModule('ViceExtTouchAutoAim');
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
    if (action === 'pause') callModule('ViceExtTouchPause');
    if (action === 'vehicle-left') state.moveX = -128;
    if (action === 'vehicle-right') state.moveX = 128;
    if (action === 'rotate-left') state.rotate = -1;
    if (action === 'rotate-right') state.rotate = 1;
    if (action === 'fire-left') state.driveSide = -1;
    if (action === 'fire-right') state.driveSide = 1;
    if (action === 'fire-center') state.driveSide = 0;
  }

  function actionUp(key, action) {
    if (action === 'vehicle-left' || action === 'vehicle-right') state.moveX = 0;
    if (action === 'rotate-left' || action === 'rotate-right') state.rotate = 0;
    if (action === 'fire-left' || action === 'fire-right' || action === 'fire-center') state.driveSide = null;
  }

  function buildButtons(now) {
    let buttons = 0;
    if (state.press.has('onfoot-run')) buttons |= CROSS;
    if (now < state.jumpUntil) buttons |= SQUARE;
    if (now < state.crouchUntil) buttons |= LEFT_SHOCK;
    if (state.press.has('onfoot-action')) {
      if (state.firearm) buttons |= LEFT1 | RIGHT1;
      else buttons |= CIRCLE;
    }
    if (state.press.has('vehicle-gas') || now < state.gasUntil) buttons |= RIGHT2;
    if (state.press.has('vehicle-brake') || now < state.brakeUntil) buttons |= LEFT2;
    if (state.press.has('vehicle-handbrake')) buttons |= RIGHT1;
    if (state.press.has('vehicle-horn')) buttons |= LEFT_SHOCK;
    if (now < state.missionUntil) buttons |= RIGHT_SHOCK;
    if (state.press.has('vehicle-fire-center')) buttons |= CIRCLE;
    if (state.press.has('vehicle-fire-mg')) buttons |= RIGHT1;
    if (state.driveSide !== null) buttons |= CIRCLE;
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
      if (state.driveSide !== null) lookX = state.driveSide * 128;
      if (now < state.pitchUntil) leftY = state.pitch;
    }
    module._ViceExtTouchFrame(
      Math.round(leftX), Math.round(leftY), 0, 0,
      Math.round(clamp(lookX, -128, 128)), Math.round(clamp(lookY, -128, 128)),
      buildButtons(now) >>> 0
    );
    state.lookX *= 0.55;
    state.lookY *= 0.55;
    if (Math.abs(state.lookX) < 0.5) state.lookX = 0;
    if (Math.abs(state.lookY) < 0.5) state.lookY = 0;
  }

  function frame() {
    if (!state.enabled) return;
    rememberBridge();
    const now = nowMs();
    if (now >= state.contextAt) {
      state.contextAt = now + 100;
      readContext();
    }
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
    state.driveSide = null;
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

  function onVisibility() {
    if (document.hidden) releaseAll();
  }

  function onBlur() {
    releaseAll();
  }

  function mount() {
    if (state.mounted) return;
    state.mounted = true;
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
    window.removeEventListener('pointerdown', onMenuPointerDown, true);
    window.removeEventListener('pointerup', onMenuPointerUp, true);
    window.removeEventListener('pointercancel', onMenuPointerCancel, true);
    document.removeEventListener('visibilitychange', onVisibility);
    window.removeEventListener('blur', onBlur);
    if (layer.parentNode) layer.remove();
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
