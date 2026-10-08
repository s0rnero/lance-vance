---
name: mejoras-web
status: EXECUTED
type: feature
domain: web
owner_rules: .agents
created: 2026-09-12
---

# Plan de mejora — GTA VC en el navegador (`gta_vc_browser/`)

Objetivo: correr el juego lo mejor posible en el navegador: con sonido,
en HD configurable, sin bloqueos de pestaña, con menos RAM y con mods.
Stack: reVC (`miami`) + librw GL3/GLFW + OpenAL, Emscripten 6.0.9, Vite :2077.

Estado actual (verificado headless): boot → menú ES → input →
`GS_PLAYING_GAME` con 3D a ~60-170 ticks/s. Detalle técnico de lo hecho en
`gta_vc_browser/patches/NOTES.md`.

> Nota de método: se estudió a alto nivel la documentación pública de otros
> ports web (p.ej. pipeline de datos + FS asíncrono bajo demanda). Toda la
> implementación de este plan será original, sin reutilizar código de terceros.

## P0 — Audio real (en curso)

**Causa raíz del "no hay software para el sonido"**: Emscripten no implementa
`ALC_ENUMERATION_EXT`; `ALDeviceList` encontraba 0 dispositivos → 0 providers.
**Aplicado**: `add_providers()` registra el dispositivo por defecto
(`alcOpenDevice(NULL)` = Web Audio) directamente en `__EMSCRIPTEN__`
(`src/audio/sampman_oal.cpp`). Sin EFX (no existe en el port).

Pendiente de validación (headless va muteado; hace falta oído humano):
1. En GPU real: Jugar → 🔊 debe pasar a `🔊 audio activo` → SFX + diálogos
   (tablas PC `.MP3`, ya activas) + ambiente. Radios mudas por diseño
   (excluidas; `--with-radio` las trae, ~504 MB extra).
2. Si el contexto nace `suspended` y no despierta: el `unlockAudio()` ya
   reintenta en clic/tecla/🔊. No existe "permiso" de audio que aceptar.
3. Proveedor visible en Opciones → Sonido (ya no debe decir "sin software").
4. Opcional: normalizar todo el audio a MP3 en el pipeline de datos (abajo),
   eliminando `.adf`/`.wav` sueltos y el port mpg123 como dependencia viva.

## P0 — Carga progresiva, cero "pestaña sin respuesta" (plan concreto)

**Mecánica exacta del problema**: `case GS_INIT_PLAYING_GAME:` llama a
`InitialiseGame()` → `CGame::Initialise()` (222 líneas, `src/core/Game.cpp`)
**entero dentro de un solo tick** de `EmscriptenTick`. Son ~18 secciones
(texturas genéricas, partículas, paths, agua, streaming, animaciones
—`cuts.img` 110 MB—, edificios, semáforos, scripts, objetos, `Load scene`…).
En hardware lento supera los ~10-30 s de hilo principal bloqueado y Chrome
muestra "la página no responde". No es un cuelgue: termina si le das Esperar.

**Lo que se va a hacer (pantalla de carga progresiva de verdad, como piden
todos los juegos)**: NO es "cirugía del motor", es partir UNA función por
sus costuras existentes:
1. Cada sección ya empieza con `LoadingScreen("Loading the Game", "<fase>",
   ...)` (18 puntos de corte naturales, listados arriba). Se convierten en
   pasos `InitStep[0..17]` con un índice estático: cada tick ejecuta UN paso
   y devuelve el control al navegador (la pestaña respira, la barra avanza).
2. Ojo: `DISABLE_LOADING_SCREEN` hoy convierte esos `LoadingScreen(a, b)` en
   no-op (`if (str1 && str2) return`). Para web NO se reactivará el splash
   GL (cuesta subidas de textura): el progreso lo pintará la **barra de la
   página** (`#progress` ya existe) vía callback C→JS (`EM_ASM` con
   paso/total por cada sección).
3. Regla de aceptación: ningún tick >2 s en la máquina de referencia;
   medido con timers por sección (traza `[web]`, barata) antes y después.
4. `InitialiseGame()` queda como envoltura que avanza pasos (compatibilidad
   con el resto del código que la llama).
5. Riesgos: estado estático entre pasos (locales → `static`/struct de
   contexto), reentrancia con `CdStream` (el hilo worker sigue corriendo;
   sincronizar como hoy), y que alguna sección individual siga pesada
   (entonces se subdivide: p.ej. carga de anims por bloques).

**Estado 2026-09-12: implementado y verificado** (19/19 fases → estado 9,
CDP responde en todo momento; detalle en `.agents/HISTORIAL.md`).
Videos: siguen excluidos (`NO_MOVIES`); si algún día se quieren,
`<video>` nativo superpuesto, nunca dentro del loop.

Restricción del navegador (documentar al usuario, no es negociable):
Ctrl+W / cerrar / recargar no se pueden bloquear (reservado anti-malware);
**sí** se puede el diálogo de confirmación → implementado (`beforeunload`
con juego en marcha). Esc-saliendo-de-fullscreen/pointer-lock también es
reservado; implementado Keyboard Lock API (`navigator.keyboard.lock`) al
entrar con ⛶ (Chrome) + re-captura de ratón al clic. F11/F12 son del
navegador y no se pueden interceptar (F12 además es captura del propio juego).

## P1 — HD y opciones gráficas reales

Hoy: 1 solo modo sintetizado (canvas según viewport, clamp 1920×1080).
El menú del juego lista lo que librw reporta → solo 1 opción (comportamiento
correcto dada la entrada, no bug del juego original).
1. **Sintetizar 3 modos** en `makeVideoModeList()` (1280×720, 1600×900,
   1920×1080, todos ventana) para que el menú liste resoluciones.
2. Al cambiar de modo: redimensionar canvas por `EM_ASM` + `rsCAMERASIZE`
   (el circuito `resizeCB` ya existe) + persistir en `reVC.ini` (ya persiste).
3. **Artefactos negros en NPCs**: la página ya reporta GPU + DXT/ASTC/ETC.
   Si en GPU real hay S3TC y persisten → sospechar skinning/normales
   (captura cercana + `glGetError` por frame en build debug). Si NO hay
   S3TC → fallback: decodificación DXT por software al subir texturas
   (stb_dxt-style, solo web) o preconvertir texturas en el pipeline.
4. Fondo de menú gris + quad inclinado: revalidar en GPU real; si persiste,
   revisar viewport/scissor del sprite 2D en librw-GL3.

## P1 — Menos RAM (pipeline de datos propio)

Hoy: `reVC.data` ~970 MB en MEMFS + heap wasm (necesita ~3-4 GB libres).
Método observado fuera (idea general, implementación propia):
arranque con ~50 MB + resto bajo demanda.
1. **Fase 1 (sin refactor de I/O)**: normalizar audio a MP3
   (`sfx.RAW` 324 MB → banco MP3 + adaptar `LoadSampleBank`; radios a MP3
   con el mismo tratamiento) y reempaquetar comprimido por tipos.
   Objetivo: `.data` ≈ 300-400 MB.
2. **Fase 2 (refactor I/O)**: `gta3.img`/`cuts.img` extraídos a ficheros
   sueltos + backend de lectura que sirva desde HTTP-cache/FetchFS con
   reintentos, manteniendo la API síncrona donde el motor la exige y
   moviendo streaming a workers donde sea posible. Requiere rediseño de
   `CdStream` (ver P0.2): es el proyecto grande del plan.
3. Regla permanente: nada que lea el hilo principal sale de MEMFS
   (lección del `createLazyFile` revertido).

## P2 — Entrada y mods

- Mando: el port GLFW no trae gamepad-mapping; implementarlo a mano
  (Gamepad API → tabla estilo `gamecontrollerdb`, capa propia en `web/` +
  `CapturePad` ya degrada bien). Táctil: capa de botones en la página
  (implementación propia).
- Mods de assets: overlay `gta_vc_browser/mods/` + `--mods` ya existe.
  Documentar límites a subir por mod (`config.h`) y matriz de compatibilidad
  (probar Vice City Extended por partes: mapa → texturas → `main.scm`).
  Código (DLL/ASI/CLEO): no soportado, por diseño de re3.

## P3 — Publicación y mantenimiento
- **Salida de pantalla completa por pulsación larga**: toque de Esc = nada;
  mantener Esc **3 s** = salir (+ botón salir en pantalla). Requiere retener
  la tecla a nivel de página; si el navegador lo deniega, aviso en log.
- `vite build` + cabeceras COOP/COEP en el host (requeridas por pthreads).
- Sin assets en el repo (solo `mods/` vacío + docs). Aviso legal en README.
- Candidatos a upstream: NULL-guard `makeVideoModeList`, `RenderCB` con
  retorno, `USE_UNNAMED_SEM` en Emscripten (guiños pequeños y honestos).
- Limpieza: la traza `[web]` de arranque es permanente y de bajo volumen;
  no reintroducir prints por frame.

## Orden de ejecución propuesto

1. Validación humana de audio en GPU real (P0) → 2. Medición de carga por
   secciones (P0.1) → 3. Multi-resolución (P1, rápido y visible) →
   4. Troceado de `InitialiseGame` (P0.2, el grande) → 5. Pipeline de datos
   Fase 1 (P1) → 6. DXT/artefactos según sonda (P1) → 7. Mando/táctil (P2).
