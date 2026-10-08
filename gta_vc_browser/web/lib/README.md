# @re3/gtavc-web

GTA Vice City (reVC + Emscripten/WebAssembly) dentro de un `div`, como librería.

```js
import { startGame } from '@re3/gtavc-web';

const game = startGame({ el: document.getElementById('vc') });
```

Con eso ya está: la función monta el juego **inmediatamente** dentro de ese div
(canvas a tamaño del viewport, barra de progreso rosa pegada al top, pantalla
completa con F11, y el audio se abre solo con el primer gesto del usuario — no
hay botón de "Jugar" ni de "Audio"). **Arranca solo, al cargar la página.**

## El arranque es siempre JavaScript

No hay modo "solo atributos": el juego se arranca llamando a `startGame`. El
`index.html` de un host es un div y el módulo que llama a la función, igual que
en el proyecto Vue:

```html
<div id="vc"></div>
<script type="module">
  import { startGame } from '@re3/gtavc-web';
  startGame({ el: '#vc' });
</script>
```

## Escuchar sin importar la API

El propio div avisa de lo que pasa (y deja el manejador en `div.__vcGame`):

```js
div.addEventListener('vc-progress', (e) => console.log(e.detail.phase, e.detail.pct));
div.addEventListener('vc-ready', () => console.log('motor listo'));
div.addEventListener('vc-error', (e) => console.warn(e.detail.message));
```

Eventos: `vc-progress`, `vc-log`, `vc-booted`, `vc-ready`, `vc-error`,
`vc-audio`, `vc-fullscreen`.

## Qué necesita el host (lo único que no va en el paquete)

El motor y los datos se sirven por HTTP; la librería solo apunta dónde:

| Opción | Por defecto | Qué es |
| --- | --- | --- |
| `buildUrl` | `/build/` | `reVC.js`, `reVC.wasm`, `reVC.data` (~130 MB) |
| `streamedUrl` | `/streamed/` | datos sueltos on-demand (`models/…`, `Audio/…`) |
| `manifestUrl` | `/manifest.json` | índice de `streamed/` (claves en minúsculas) |
| `assetsUrl` | `/vc/` | copia legal original: **solo se mira si falta `streamed/`** |

Si hay `streamed/` + manifiesto completos, `assetsUrl` **no se pide nunca**
(una sola comprobación por el manifiesto). Si falta, la librería lo dice en
pantalla: "corre el pipeline de datos" o "coloca tu copia legal y corre el
pipeline".

### En Vite (Vue, React…)

El paquete trae el plugin que sirve esas rutas y pone las cabeceras que el
motor necesita (`COOP/COEP` → `SharedArrayBuffer` → pthreads):

```js
// vite.config.js
import { defineConfig } from 'vite';
import { vcWeb } from '@re3/gtavc-web/vite';

export default defineConfig({
  plugins: [vcWeb({
    streamedDir: '/ruta/a/streamed',   // por defecto ../streamed
    assetsDir: '/ruta/a/assets',       // por defecto ../assets
    traceFile: 'odtrace.log',          // POST /odtrace (depuración)
  })],
});
```

Sin Vite, cualquier servidor estático vale **si** envía
`Cross-Origin-Opener-Policy: same-origin` y
`Cross-Origin-Embedder-Policy: require-corp`, y sirve `buildUrl`/`streamedUrl`.

### Instalarlo en el proyecto Vue

```bash
npm i file:../re3/gta_vc_browser/web/lib      # o copia la carpeta lib/
```

Viene con tipos (`index.d.ts`), así que en TS/Vue se autocompleta:

```vue
<script setup lang="ts">
import { onMounted, onBeforeUnmount, ref } from 'vue';
import { startGame, type GameHandle } from '@re3/gtavc-web';

const host = ref<HTMLElement | null>(null);
let game: GameHandle | null = null;

// Al montar la vista arranca solo: no hay botón ni gesto previo.
onMounted(() => {
  game = startGame({
    el: host.value!,
    fill: 'parent',                       // ocupa la caja del div (no toda la pantalla)
    buildUrl: '/game/build/',             // donde sirvas el wasm
    streamedUrl: '/game/streamed/',
    manifestUrl: '/game/manifest.json',
    onProgress: (p) => console.log(p.phase, p.pct),
  });
});

onBeforeUnmount(() => game?.destroy());
</script>

<template>
  <div ref="host" style="width: 100%; height: 100%"></div>
</template>
```

En Vue, `onMounted` es el momento equivalente a "cargar la página": la vista se
monta, se llama a `startGame` y el juego ya está arrancando.

## API

`startGame(options) → GameHandle` (solo puede haber una instancia: el motor es
global; una segunda llamada devuelve la viva).

Opciones de comportamiento: `fill` (`'viewport'` por defecto o `'parent'`),
`title`, `audio`, `pointerLock`, `guardUnload`, `fullscreenKey`, `showFps`,
`idbCapMB`, `warmMB`, `worker`, `traceUrl`, `console`, `noInitialRun`,
`requireBuild`, `requireData`.

### Carga en un hilo aparte

Traer ficheros (leer la caché del navegador y, si falta, bajarlo) lo hace un
Web Worker, no el hilo del juego. En partida, un fichero que aún no está se
contesta "todavía no" en lugar de congelar el frame: el motor lo vuelve a pedir
en la siguiente pasada y entra un par de frames después. Durante la carga
(menú y partida) se espera como siempre, porque ahí el fichero hace falta sí o
sí. Con `worker: false` se vuelve al comportamiento anterior (útil para
comparar). El log deja una línea `ODWK` cada 30 s con las cuentas (peticiones,
aplazamientos, entregas, errores).

### Caché de datos del navegador

El juego guarda en IndexedDB (`vcod2`) una copia de todo lo que va pidiendo; de
ahí salen los datos del primer minuto sin tocar la red.

- `idbCapMB` (900 por defecto) es el **techo** de esa caché: al arrancar se
  borra lo más viejo y menos usado hasta quedar por debajo. `0` = sin techo.
- `warmMB` (64 por defecto) es el **arranque en caliente**: nueva ruta, se
  aprende del uso y la sesión siguiente se adelantan esos ficheros (los más
  pequeños primero) durante la pantalla de carga, para que el primer minuto de
  juego no tenga hipos. `0` = desactivado.
- `game.cacheInfo()` y `await game.clearDataCache()` (vacía la caché; los
  guardados no se tocan, viven en otro almacén).

Ganchos: `onProgress({phase,pct,label,busy})`, `onLog(line)`, `onError(error)`,
`onReady()`, `onAudio(active)`, `onFullscreen(active)`.

En el handle: `ok`, `canvas`, `module`, `progress` (estado), `logs`,
`on(evt, fn)`, `setProgress()`, `showFps()`, `fpsVisible()`, `cacheInfo()`,
`clearDataCache()`, `enterFullscreen()`, `exitFullscreen()`,
`toggleFullscreen()`, `unlockAudio()`, `fsProbe()`, `destroy()`.

El contador de FPS **no** se controla por URL: se pide en el arranque
(`startGame({ showFps: true })`) y se puede
encender/apagar en caliente con `game.showFps(true|false)` (`showFps()` sin
argumento enciende). Leerlo: `game.fpsVisible()`. La opción vieja `?fps` en la
URL solo sigue viva como comodidad de desarrollo.

Detalles que conviene saber:

- **Barra de progreso**: la barra fina rosa del top del viewport solo cubre la
  preparación previa al motor: comprobar ficheros (`probe`) y descargar el
  build (`build`). Se retira en cuanto el motor **presenta su primer frame**
  (el propio motor avisa con `window.__vcFrame` desde `EmscriptenTick`) y ya no
  vuelve: a partir de ahí manda la pantalla del juego (menú, splash de carga de
  partida). Ojo con la señal: las fases de `window.__loadProgress` **no**
  valen como "ya hay juego", porque solo llegan al inicializar el *mundo*
  (empezar/cargar partida), no al arrancar el motor. Si el loader no sabe
  cuánto falta (p.ej. un solo fichero grande), la barra lo dice moviéndose, no
  inventando un porcentaje.
- **Apilado (z-index)**: la barra mide 3 px, va pegada al top y tiene
  **z-index 50**: queda por encima del canvas y de la portada de carga del
  juego, y por debajo de cualquier overlay del host con z-index mayor. El
  contenedor `.vc-root` sí usa un z-index altísimo (para no quedar debajo del
  contenido del host); si montas con `fill: 'parent'`, la barra pasa a ser
  `absolute` dentro de la caja del div en vez de cruzarse por el viewport.
- **Diagnóstico de la barra**: cada encendido/apagado deja una línea `BART`
  (con el motivo: `motor-primer-frame`, `runtime-timeout`, `done-timeout`…) en
  la cola de trazas, así que en el `odtrace.log` se ve quién la apagó y cuándo.
- **F11**: pide pantalla completa del contenedor. El navegador también reserva
  F11 para su propia pantalla completa, así que pueden verse las dos; dentro de
  la pantalla completa del juego, **Esc corto no hace nada y mantener Esc 3 s
  sale**, sin ningún aviso ni barra en pantalla (el cartel "Mantén Esc" se
  quitó: no debe tapar el juego).
- **Audio**: el motor arranca sin audio y se reanuda en el primer gesto
  (`pointerdown` / `keydown` / `touch`), sin diálogos ni botones.
- **`destroy()`** para el bucle y limpia el DOM; el módulo wasm no se puede
  liberar de verdad dentro de una página (limitación de Emscripten).

## Desarrollo en este repo

La página de desarrollo (`gta_vc_browser/web/index.html`) es a propósito **un
div** y usa esta misma librería (`web/src/dev.js`), con `/odtrace` activado
para que el motor escriba `web/odtrace.log`:

```bash
npm run dev            # http://localhost:2077
```
