// Estilos de la librería. Se inyectan una sola vez en el documento del host:
// el consumidor (Vue, React, HTML suelto) NO tiene que traer CSS propio.
// Paleta: rosa Vice City (#ff2e9a) sobre negro.
export const STYLE_ID = 'gtavc-web-style';

export const CSS = `
/* Contenedor del juego. fill:"viewport" (por defecto) = ocupa toda la pantalla.
   fill:"parent" = ocupa la caja del div que da el host (el padre debe tener
   tamaño: el canvas se estira al 100% de esa caja). */
.vc-root {
  position: fixed; inset: 0; width: 100vw; height: 100vh;
  background: #000; overflow: hidden;
  z-index: 2147482000;                 /* encima del contenido del host */
  font-family: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
  -webkit-tap-highlight-color: transparent;
}
.vc-root[data-fill="parent"] { position: relative; inset: auto; width: 100%; height: 100%; z-index: auto; }

/* El canvas ocupa el contenedor entero. */
.vc-canvas {
  position: absolute; inset: 0; width: 100%; height: 100%;
  display: block; background: #000; outline: none;
}

.vc-intro {
  position: absolute; inset: 0; width: 100%; height: 100%;
  object-fit: cover; background: #000; z-index: 40;
}
.vc-skip {
  position: absolute; right: 22px; bottom: 22px; z-index: 51;
  padding: 9px 18px; cursor: pointer;
  background: rgba(10, 10, 14, .72); color: #fff;
  border: 1px solid rgba(255, 255, 255, .35); border-radius: 6px;
  font: 600 13px/1 system-ui, "Segoe UI", Roboto, sans-serif;
  letter-spacing: .08em; text-transform: uppercase;
}
.vc-skip[disabled] { opacity: .45; cursor: default; }

/* Barra de progreso: fina, pegada al top del viewport, rosa VC.
   z-index 50: por encima del canvas y de la portada de carga, pero por DEBAJO
   de lo que el host quiera poner encima (overlays propios con z-index > 50),
   y sin competir con el z-index altísimo del contenedor .vc-root.
   Solo cubre la preparación previa al motor: comprobar ficheros y descargar
   el build. Cuando el motor pinta su propia pantalla de carga (arranque o
   carga de partida) se retira, para no duplicar el loading del juego. */
.vc-bar {
  position: fixed; top: 0; left: 0; right: 0; height: 3px;
  background: rgba(255, 46, 154, .14);
  z-index: 50; overflow: hidden;
  opacity: 0; transition: opacity .3s ease;
  pointer-events: none;
}
/* Empotrado (fill:"parent"): la barra pertenece a la caja del div, no al
   viewport, para no cruzarse con la interfaz del host. */
.vc-root[data-fill="parent"] .vc-bar { position: absolute; }
.vc-bar[data-on] { opacity: 1; }
.vc-bar > i {
  display: block; height: 100%; width: 0%;
  background: #ff2e9a; box-shadow: 0 0 10px #ff2e9a;
  transition: width .25s ease;
}
/* El !important es deliberado: en modo "trabajando" el ancho del CSS no debe
   pelear con un ancho inline de una fase anterior (si gana el inline, el
   barrido mueve una barra llena y se ve como un parpadeo). */
.vc-bar[data-indet] > i { width: 32% !important; animation: vc-scan 1.15s ease-in-out infinite; }
@keyframes vc-scan { from { margin-left: -32%; } to { margin-left: 100%; } }

/* Portada de carga de partida (la del mundo, con barra). La enseña el motor
   vía window.__loadOverlay durante CGame::InitialiseStep. */
.vc-load {
  position: absolute; inset: 0; z-index: 5;
  display: flex; flex-direction: column; align-items: center; justify-content: center; gap: 16px;
  background: #000; color: #ff2e9a; text-align: center;
}
.vc-load[hidden] { display: none; }
.vc-load .vc-load-title {
  font-size: clamp(28px, 6vw, 56px); font-weight: 800; letter-spacing: .22em;
  text-shadow: 0 0 18px rgba(255, 46, 154, .55);
}
.vc-load .vc-load-sub { font-size: 13px; color: #ffb3dc; letter-spacing: .04em; }
.vc-load .vc-load-bar {
  width: min(340px, 64vw); height: 6px; border-radius: 3px;
  background: #2a0018; overflow: hidden;
}
.vc-load .vc-load-bar > i { display: block; height: 100%; width: 0; background: #ff2e9a; transition: width .2s ease; }
.vc-load .vc-load-label { font-size: 12px; color: #ff7ac4; min-height: 15px; }

/* Aviso (falta build o datos). Se ve en lugar del juego, sin romper el host. */
.vc-msg {
  position: absolute; left: 50%; top: 50%; transform: translate(-50%, -50%);
  z-index: 6; width: min(560px, 88vw); padding: 16px 18px;
  background: #14000c; border: 1px solid rgba(255, 46, 154, .45); border-radius: 10px;
  color: #ffd7ec; font-size: 13px; line-height: 1.55;
}
.vc-msg h3 { margin: 0 0 8px; font-size: 14px; color: #ff2e9a; letter-spacing: .04em; }
.vc-msg code { background: #26000f; padding: 1px 5px; border-radius: 4px; color: #ffb3dc; }
.vc-msg ul { margin: 8px 0 0; padding-left: 18px; }

/* (Sin aviso de Esc: mantener Esc 3 s sale de pantalla completa, sin cartel.) */

.vc-root[data-touch="1"] { touch-action: none; cursor: none; }
.vc-touch {
  position: absolute; inset: 0; z-index: 30; pointer-events: none;
  user-select: none; -webkit-user-select: none; touch-action: none;
  color: #fff; font-family: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
}
.vc-touch[hidden] { display: none; }
.vc-touch-zone {
  position: absolute; bottom: 0; height: 78%; z-index: 1;
  pointer-events: auto; touch-action: none;
}
.vc-touch-move { left: 0; width: 50%; }
.vc-touch-look { right: 0; width: 50%; }
.vc-touch-radar {
  bottom: auto; top: 0; left: 0; width: 17%; height: 28%; z-index: 4;
  border-radius: 18px;
}
.vc-touch-weapon {
  bottom: auto; top: 4%; right: 1%; left: auto; width: 16%; height: 22%; z-index: 4;
}
.vc-touch-radio {
  bottom: auto; top: 0; left: 34%; width: 32%; height: 10%; z-index: 4;
}
.vc-touch-skip { display: none; }
.vc-touch-stick {
  position: absolute; z-index: 2; width: 92px; height: 92px; margin: -46px 0 0 -46px;
  border: 2px solid rgba(255, 255, 255, .38); border-radius: 50%;
  background: rgba(0, 0, 0, .22); pointer-events: none;
}
.vc-touch-stick[hidden] { display: none; }
.vc-touch-stick i {
  display: block; width: 46px; height: 46px; margin: 21px;
  border: 2px solid rgba(255, 255, 255, .82); border-radius: 50%;
  background: rgba(255, 46, 154, .35); box-shadow: 0 0 16px rgba(255, 46, 154, .35);
}
.vc-touch-btn {
  position: absolute; z-index: 3; display: flex; align-items: center; justify-content: center;
  width: clamp(48px, 11.5vmin, 82px); aspect-ratio: 1; padding: 0;
  transform: translate(-50%, -50%); pointer-events: auto; touch-action: none;
  border: 0; border-radius: 50%; background: rgba(6, 6, 10, .34); color: #fff;
  box-shadow: 0 3px 12px rgba(0, 0, 0, .28);
  transition: transform .08s ease, background .08s ease;
}
.vc-touch-btn img { width: 100%; height: 100%; object-fit: contain; pointer-events: none; filter: drop-shadow(0 1px 3px rgba(0,0,0,.55)); }
.vc-touch-btn.is-active { transform: translate(-50%, -50%) scale(.9); background: rgba(255, 46, 154, .3); }
.vc-touch-home {
  left: 1.6%; top: 28%; z-index: 5;
  width: clamp(48px, 8vmin, 62px); height: clamp(48px, 8vmin, 62px);
  transform: translate(0, 0); background: transparent; box-shadow: none; border-radius: 0;
}
.vc-touch-home img { width: 100%; height: 100%; filter: none; }
.vc-touch-home.is-active { transform: scale(.92); }
.vc-touch[data-mode="onfoot"] .vc-touch-btn[data-modes]:not([data-modes~="onfoot"]),
.vc-touch[data-mode="vehicle"] .vc-touch-btn[data-modes]:not([data-modes~="vehicle"]),
.vc-touch[data-mode="menu"] .vc-touch-btn,
.vc-touch[data-mode="menu"] .vc-touch-zone,
.vc-touch[data-mode="cutscene"] .vc-touch-btn,
.vc-touch[data-mode="cutscene"] .vc-touch-zone:not(.vc-touch-skip),
.vc-touch[data-mode="cutscene"] .vc-touch-stick,
.vc-touch[data-mode="vehicle"] .vc-touch-move,
.vc-touch[data-mode="vehicle"] .vc-touch-stick,
.vc-touch[data-mode="onfoot"] .vc-touch-zone[data-modes]:not([data-modes~="onfoot"]),
.vc-touch[data-mode="vehicle"] .vc-touch-zone[data-modes]:not([data-modes~="vehicle"]) { display: none; }
/* Puntos 5 y 6: sin botones nuevos; en vehículo la cámara ocupa todo el
   ancho para que deslizar mueva la vista. */
.vc-touch[data-mode="vehicle"]:not([data-vehicle="heli"]):not([data-vehicle="plane"]) .vc-touch-look { left: 0; width: 100%; }
.vc-touch[data-mode="vehicle"][data-vehicle="heli"] .vc-touch-move,
.vc-touch[data-mode="vehicle"][data-vehicle="plane"] .vc-touch-move { display: block; }
.vc-touch[data-mode="vehicle"][data-vehicle="heli"] .vc-touch-stick:not([hidden]),
.vc-touch[data-mode="vehicle"][data-vehicle="plane"] .vc-touch-stick:not([hidden]) { display: block; }

/* Un botón de vehículo solo existe en las clases de vehículo que lo usan. */
.vc-touch[data-mode="vehicle"] .vc-touch-btn[data-vehicles] { display: none; }
.vc-touch[data-mode="vehicle"][data-vehicle="car"] .vc-touch-btn[data-vehicles~="car"],
.vc-touch[data-mode="vehicle"][data-vehicle="bike"] .vc-touch-btn[data-vehicles~="bike"],
.vc-touch[data-mode="vehicle"][data-vehicle="boat"] .vc-touch-btn[data-vehicles~="boat"],
.vc-touch[data-mode="vehicle"][data-vehicle="heli"] .vc-touch-btn[data-vehicles~="heli"],
.vc-touch[data-mode="vehicle"][data-vehicle="plane"] .vc-touch-btn[data-vehicles~="plane"],
.vc-touch[data-mode="vehicle"][data-vehicle="tank"] .vc-touch-btn[data-vehicles~="tank"] { display: flex; }

/* El drive-by solo aparece con arma de fuego; el cañón del tanque siempre.
   Plan 64.0 T4: un solo boton vehicle-fire (icono real vehicle-shoot); se
   retiraron los botones inventados fire-left/center/right. */
.vc-touch-btn[data-action="fire"] { display: none !important; }
.vc-touch-btn[data-action="mission"] { display: none !important; }
.vc-touch[data-mode="vehicle"][data-mission="1"] .vc-touch-btn[data-action="mission"] { display: flex !important; }
.vc-touch[data-armed="1"][data-vehicle="car"] .vc-touch-btn[data-action="fire"],
.vc-touch[data-armed="1"][data-vehicle="bike"] .vc-touch-btn[data-action="fire"],
.vc-touch[data-armed="1"][data-vehicle="boat"] .vc-touch-btn[data-action="fire"],
.vc-touch[data-mode="vehicle"][data-vehicle="tank"] .vc-touch-btn[data-action="fire"],
.vc-touch[data-mode="vehicle"][data-vehicle="firetruck"] .vc-touch-btn[data-action="fire"] { display: flex !important; }
/* El disparo desde vehiculo no existe a pie (era el segundo icono de pistola
   cuando data-vehicle seguia puesto tras bajar del coche). */
.vc-touch:not([data-mode="vehicle"]) .vc-touch-btn[data-action="fire"] { display: none !important; }
/* Apuntar (autoapuntado al enemigo mas cercano): solo a pie y con arma. */
.vc-touch-btn[data-action="aim"] { display: none !important; }
.vc-touch[data-mode="onfoot"][data-armed="1"] .vc-touch-btn[data-action="aim"] { display: flex !important; }
.vc-touch-btn[data-action="fire-mg"] { display: none !important; }
.vc-touch[data-mode="vehicle"][data-vehicle-armed="1"] .vc-touch-btn[data-action="fire-mg"] { display: flex !important; }

.vc-touch[data-mode="cutscene"] .vc-touch-skip {
  display: block; position: absolute; inset: 0; z-index: 20;
  pointer-events: auto; touch-action: none; background: transparent;
}
.vc-touch-rotate { display: none; position: absolute; inset: 0; z-index: 30; align-items: center; justify-content: center; background: #000; color: #ff2e9a; font: 800 16px/1.4 system-ui, sans-serif; letter-spacing: .1em; text-transform: uppercase; text-align: center; padding: 24px; pointer-events: none; }
@media (orientation: portrait) and (pointer: coarse) {
  .vc-touch-rotate { display: flex; }
  .vc-touch-btn, .vc-touch-zone { visibility: hidden; }
}
@media (max-height: 430px) {
  .vc-touch-btn { width: clamp(44px, 10.5vmin, 68px); }
  .vc-touch-home { width: 42px; height: 42px; }
}
/* Pantalla completa + bloqueo de gestos del navegador (móvil).
   Ctrl+N/Ctrl+P y demás atajos reservados NO se pueden bloquear desde la
   página; en móvil los equivalentes son pull-to-refresh, menú de llamada
   al mantener pulsado, zoom con doble toque/pellizco y rebote de
   sobre-desplazamiento — todos bloqueados aqui. */
html, body { overscroll-behavior: none; }
.vc-touch {
  touch-action: none;
  overscroll-behavior: none;
  user-select: none;
  -webkit-user-select: none;
  -webkit-touch-callout: none;
  -webkit-tap-highlight-color: transparent;
}
`;
