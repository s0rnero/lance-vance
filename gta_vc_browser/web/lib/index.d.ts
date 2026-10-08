// Tipos públicos de @re3/gtavc-web (GTA Vice City en el navegador).
// Se escriben a mano: la librería es JS puro para poder usarse sin compilar.

// 'probe' y 'build' son la preparación previa al motor (la única que pinta la
// barra del top). 'engine' es el propio motor cargando (arranque o partida):
// su pantalla de carga manda y la barra del top se retira.
export type Phase = 'probe' | 'build' | 'engine';

export interface ProgressInfo {
  /** Fase actual: comprobar ficheros, descargar el build, carga del motor. */
  phase: Phase;
  /** 0..1 dentro de la fase (null si es indeterminado, p.ej. descarga sin total). */
  pct: number | null;
  label: string;
  /** true mientras el trabajo sigue en marcha. */
  busy: boolean;
}

export interface StartGameOptions {
  /** Div donde montar el juego (elemento o selector CSS). Obligatorio. */
  el: HTMLElement | string;
  /** Documento alternativo (iframes/tests). */
  doc?: Document;
  /** Dónde están reVC.js / reVC.wasm / reVC.data. Por defecto '/build/'. */
  buildUrl?: string;
  /** Datos sueltos on-demand. Por defecto '/streamed/'. */
  streamedUrl?: string;
  /** Índice JSON de streamed/. Por defecto '/manifest.json'. */
  manifestUrl?: string;
  /** Copia legal original: SOLO se mira si falta streamed/. Por defecto '/vc/'. */
  assetsUrl?: string | null;
  /** 'viewport' (toda la pantalla, por defecto) | 'parent' (la caja del div). */
  fill?: 'viewport' | 'parent';
  /** Poner el título de la pestaña con el tag de build. Por defecto true. */
  title?: boolean;
  /** Desbloquear el audio con el primer gesto. Por defecto true. */
  audio?: boolean;
  /** Capturar el ratón al hacer clic en el juego. Por defecto true. */
  pointerLock?: boolean;
  /** Pedir confirmación al cerrar/recargar con partida en marcha. Por defecto true. */
  guardUnload?: boolean;
  /** F11 = pantalla completa del juego. Por defecto true. */
  fullscreenKey?: boolean;
  /** Contador de FPS del motor en pantalla. Por defecto false.
   *  (Ya no hace falta el parámetro ?fps de la URL; se enciende por API
   *  o en caliente con game.showFps(true).) */
  showFps?: boolean;
  /** Techo de la caché de datos en IndexedDB, en MB (900 por defecto).
   *  Al arrancar se borra lo más viejo y menos usado hasta bajar del techo.
   *  0 = sin techo. */
  idbCapMB?: number;
  /** Arranque en caliente: MB a adelantar durante la carga del mundo usando
   *  lo aprendido en sesiones anteriores (64 por defecto). 0 = desactivado. */
  warmMB?: number;
  /** Cargar los ficheros en un Web Worker en vez del hilo del juego
   *  (por defecto true). En partida, un fichero que aún no está no congela el
   *  frame: el motor lo reintenta y entra un par de frames después. */
  worker?: boolean;
  /** Endpoint de trazas (p.ej. '/odtrace' en desarrollo). null = no enviar nada. */
  traceUrl?: string | null;
  /** Espejo de los mensajes del juego a la consola. Por defecto true. */
  console?: boolean;
  /** Cargar runtime + FS sin arrancar el juego (diagnóstico). Por defecto false. */
  noInitialRun?: boolean;
  /** Abortar si falta el build. Por defecto true. */
  requireBuild?: boolean;
  /** Abortar si faltan los datos. Por defecto true. */
  requireData?: boolean;

  onProgress?: (info: ProgressInfo) => void;
  onLog?: (line: string) => void;
  onError?: (error: Error) => void;
  onReady?: () => void;
  onAudio?: (active: boolean) => void;
  onFullscreen?: (active: boolean) => void;
}

export type GameEvent = 'progress' | 'log' | 'error' | 'ready' | 'audio' | 'fullscreen' | 'booted';

export interface GameHandle {
  /** false si faltan build o datos (el motivo sale en el log y por onError). */
  ok: boolean;
  version: string;
  el: HTMLElement;
  root: HTMLElement;
  canvas: HTMLCanvasElement;
  config: {
    buildUrl: string;
    streamedUrl: string;
    manifestUrl: string;
    assetsUrl: string | null;
    fill: string;
    traceUrl: string | null;
  };
  /** El Module de Emscripten (null hasta que arranca). */
  readonly module: unknown;
  readonly progress: ProgressInfo;
  readonly logs: string[];
  log(line: string): string;
  showBar(): void;
  hideBar(): void;
  /** El host también puede empujar su propio progreso (fases probe/build). */
  setProgress(phase: Phase, pct: number | null, label?: string, busy?: boolean): void;
  /** Marca la carga como terminada (barra al 100% y se apaga sola). */
  done(): void;
  /** Enciende/apaga el contador de FPS del motor sin recargar la página. */
  showFps(on?: boolean): boolean;
  /** true si el contador de FPS está encendido. */
  fpsVisible(): boolean;
  /** Estado de la caché de datos: techo, presupuesto de calentamiento y
   *  cuántos ficheros borró el último recorte. */
  cacheInfo(): { capMB: number | null; warmMB: number | null; trimmed: number };
  /** Vacía la caché de datos de IndexedDB (los guardados no se tocan). */
  clearDataCache(): Promise<boolean>;
  isFullscreen(): boolean;
  enterFullscreen(): Promise<void>;
  exitFullscreen(): Promise<void>;
  toggleFullscreen(): Promise<void>;
  /** Reanuda el AudioContext (true si suena, string con el motivo si no). */
  unlockAudio(): Promise<true | string>;
  audioActive(): boolean;
  /** Suscripción a eventos; devuelve la función para darse de baja. */
  on(event: GameEvent, fn: (arg: never) => void): () => void;
  /** Diagnóstico del sistema de ficheros virtual. */
  fsProbe(): void;
  /** Para el bucle y limpia el DOM (el wasm no se puede liberar del todo). */
  destroy(): void;
}

export declare const VERSION: string;

/** Monta el juego dentro del div indicado. Sólo puede haber una instancia. */
export declare function startGame(options: StartGameOptions): GameHandle;

/** Instancia viva (o null). */
export declare function getGame(): GameHandle | null;

/** true si el juego arrancó de verdad (build + datos presentes). */
export declare function isRunning(): boolean;

export default startGame;

// --- Plugin de Vite (serve streamed/, assets/ y las trazas) ---------------
export interface VcWebPluginOptions {
  /** Carpeta real con los datos sueltos (por defecto ../streamed). */
  streamedDir?: string;
  /** Carpeta real con la copia original del juego (por defecto ../assets). */
  assetsDir?: string;
  /** Carpeta pública donde vive manifest.json (por defecto <root>/public). */
  publicDir?: string;
  /** Fichero de trazas que se escribe con los POST (por defecto odtrace.log). */
  traceFile?: string;
  /** Rutas públicas (por defecto /streamed/, /vc/, /odtrace). */
  streamedUrl?: string;
  assetsUrl?: string;
  traceUrl?: string;
}

export declare function vcWeb(options?: VcWebPluginOptions): {
  name: string;
  configureServer(server: unknown): void;
  configurePreviewServer(server: unknown): void;
};
