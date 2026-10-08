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
`;
