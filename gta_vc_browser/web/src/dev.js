// Página de desarrollo: un div + la librería. Nada más.
// (En un proyecto Vue esto es literalmente `startGame({ el: '#vc' })`.)
import { startGame } from '../lib/index.js';

// Sin query params (petición usuario): todo activo por defecto, la página
// siempre arranca el juego y las trazas van a odtrace.log.

// Ajustes de desarrollo desde la consola o antes de cargar el módulo:
//   window.__VC_DEV_OPTS = { idbCapMB: 15, warmMB: 0, showFps: true }
// (las sondas automáticas los usan; sin eso, los valores por defecto)
const dev = (typeof window !== 'undefined' && window.__VC_DEV_OPTS) || {};

const game = startGame({
  ...dev,
  el: '#vc',
  // Contador de FPS: OPCIÓN de JS, nunca por URL. Aquí queda encendido por
  // defecto (y __VC_DEV_OPTS.showFps:false lo apaga). En un host Vue es
  // exactamente startGame({ el, showFps: true }), o game.showFps(true).
  showFps: (dev.showFps !== undefined) ? dev.showFps : true,
  // Dónde están las cosas (en el host puede ser una CDN o una subcarpeta).
  buildUrl: '/build/',
  streamedUrl: '/streamed/',
  manifestUrl: '/manifest.json',
  assetsUrl: '/vc/',           // solo se mira si falta streamed/
  fill: 'viewport',
  // En desarrollo: trazas del motor a odtrace.log (siempre).
  traceUrl: '/odtrace',
  onError: (e) => console.warn('[vc]', e.message),
});

// Comodidad para pruebas/depuración desde la consola del navegador.
window.__vcGame = game;
