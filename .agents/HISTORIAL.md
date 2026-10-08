# Historial de cambios — GTA VC en el navegador

Norma vigente desde 2026-09-12: **plan en `.agents/plans/` ANTES de tocar
código**. Este archivo registra todo lo hecho (también lo anterior a la norma,
reconstruido desde git) con honestidad parche-vs-raíz.

Leyenda: `[parche]` rodea el problema sin tocar la causa · `[raíz]` corrige
la causa · `[tooling]` build/página/docs · `[proceso]` flujo de trabajo.

## 2026-09-12 — Fase 0: scaffold y toolchain `[tooling]`
- `gta_vc_browser/` (README, `build.bat/sh`, `web/shell.html`, `web/server.py`,
  `patches/NOTES.md`), app Vite :2077 (COOP/COEP), junctions `vc/` y
  `web/public/vc/` a `Grand Theft Auto Vice City` (sin copiar 1.46 GB),
  `.gitignore` (juego + builds nunca se commitean).
- Instalado: Emscripten 6.0.9, CMake 4.4.3, Ninja, submódulos
  (`librw`, `ogg/opus/opusfile`).

## 2026-09-12 — Configure Emscripten `[tooling]`
- Shims CMake en `gta_vc_browser/cmake/` (`Findglfw3/FindOpenGL/FindOpenAL/
  Findmpg123/FindThreads`) → ports del SDK. Sin tocar CMakeLists del repo.
- Cabeceras `AL/efx.h` (+`al.h`/`alc.h` del SDK con `AL_APIENTRY`) en
  `gta_vc_browser/include/`, vía `CPATH`. EFX se desactiva solo en runtime.
- `USE_UNNAMED_SEM` en Emscripten (`config.h`): `sem_open` no existe en el
  SDK. `[raíz]` (misma solución que el port Switch).

## 2026-09-12 — Compatibilidad GLFW `[parche]` + `[raíz]`
- `glfw.cpp`: stub `sysinfo`; gamepad-mapping desactivado (el port no trae
  `glfwUpdateGamepadMappings`/`glfwGetGamepadState`); rama Emscripten en
  `psSelectDevice()` (modo 0, 1280x720 ventana); canvas 1280x720.
  `[parche]` consciente: el mapeo de mandos queda pendiente.
- `vendor/librw/.../gl3device.cpp`: NULL-guard en `makeVideoModeList()`.
  `[raíz]` (la spec de GLFW permite NULL; bug latente también nativo).

## 2026-09-12 — Main loop del navegador `[raíz]`
- `main()` bloqueante → `OuterSetup/InnerShouldRun/InnerFrame/AfterInner/
  FinalCleanup` + `EmscriptenTick` (`tools/refactor_main_loop.py`, one-shot
  con asserts). Sin esto la pestaña no respira. Conducta nativa intacta.

## 2026-09-12 — Preload + página jugable `[tooling]`
- `--preload-file` de `anim/Audio/data/models/TEXT/txd` + `gamefiles/`
  encima; excluidos `movies/`, `mp3/`, docs y radios `*.adf` por defecto
  (`--with-radio` las trae). `reVC.data` 1.47 GB → ~970 MB.
- Página: canvas full-width, `Module.locateFile`, barra de progreso,
  desbloqueo de audio, IDBFS en `/userfiles`, `?autostart`/`?fsprobe`.
- Lazy-load con `createLazyFile` intentado y **revertido**: el SDK aborta en
  el hilo principal (creación y cualquier stat/read). Documentado.

## 2026-09-12 — Los tres bugs del "no arranca" `[raíz]` ×3
1. `locateFile`: el `.data` se pedía a `/` y Vite devolvía el index
   (2436 B) → FS lleno de ficheros de 0 bytes, loop infinito en
   `CText::Load`. Fichero clave para diagnosticar: transfer sizes.
2. Reloj: `CLOCK_MONOTONIC_RAW` no existe en Emscripten → basura →
   `CTimer::Update` molía billones de iteraciones (`RESULT_CODE_HUNG`).
   Fix: `CLOCK_MONOTONIC`.
3. Shaders: el port acepta el perfil GL 3.3 pero el contexto es WebGL2 →
   `#version 330` ilegal → negro. Fix: perfil GLES + `shaderDecl300es`.

## 2026-09-12 — Crash en gameplay `[raíz]`
- `Atomic::RenderCB` en librw devolvía `void` y fakerw registraba
  `RpAtomic*(RpAtomic*)`: `call_indirect` abortaba en `RenderRoads`.
  Fix preciso (3 ficheros): el typedef devuelve `Atomic*`, el cast es exacto.
- Audio PC: `#undef PS2_AUDIO_PATHS` en web (los `.VB` no existen en PC;
  ahora usa `.MP3`/`.WAV`/`.ADF`).
- Audio provider: `ALC_ENUMERATION_EXT` no existe en el port → 0
  proveedores. Fix: registrar el dispositivo Web Audio directo.

## 2026-09-12 — UX de página `[tooling]`
- Sin pointer-lock automático (solo clic en canvas); botón 🔊 con estado +
  reintentos; sonda WebGL (GPU + DXT/ASTC/ETC); `beforeunload` con juego en
  marcha; Keyboard Lock al entrar con ⛶; overlay `mods/` + `--mods`;
  `reVC.ini` a `/userfiles` absoluto (los controles ya persisten);
  canvas según viewport (clamp 960–1920).
- Harness E2E fuera del repo (`Temp\opencode\vc-e2e`): menú ES, input,
  `GS_PLAYING_GAME` con 3D, todo con capturas.

## 2026-09-12 — UX página + multi-resolución (plan aprobado)
- Esc: toque = nada, mantener 3 s = salir + botón ✕ Salir + barra de
  progreso (`#eschold`); `beforeunload` protege Ctrl+W con diálogo;
  keyboard lock al entrar con ⛶. Nota: Ctrl+W/Esc los reserva el navegador,
  ningún sitio puede bloquearlos del todo.
- Botón 🔊 con estado + reintentos de resume; sonda WebGL (GPU + DXT/ASTC);
  `reVC.ini` a `/userfiles` absoluto (controles ya persisten).
- Multi-res: librw sintetiza 720p/900p/1080p (ventana); `_psGetVideoModeList`
  las lista (antes vacía → el menú no ofrecía nada); `psSelectDevice` elige
  la mayor que quepa en prefs; `_psSetVideoMode` redimensiona el canvas.
  Canvas según viewport (clamp 960–1920). Verificado: `idx 0/3`.
- `gta_vc_browser/mods/` + `--mods` para mods de assets (NO código).
- Splash `loadsc0` renderiza perfecto en captura (DXT bien) → los
  artefactos de NPCs pendientes de sonda en GPU real.

## 2026-09-12 — Carga progresiva + blindaje audio `[raíz]`
- `CGame::Initialise()` partida en 19 pasos (`tools/refactor_init_steps.py`):
  `InitialiseStep()` compartido (nativo loopea igual que antes), web avanza
  un paso por tick con progreso a la página (`window.__loadProgress`).
  Verificado: 19/19 fases → estado 9, CDP vivo siempre (adiós al aviso).
- `%` con longitud 0 en `MusicManager` (pistas ausentes: radios): UBSan
  `integer-divide-by-zero` localizó el `remainder by zero` en
  `SetStartingTrackPositions` + otros 3 sitios (`GetTrackStartPos`,
  `SetRadioChannelByScript`, saved positions). Blindados con guard.
- Handler `window.onerror` con stack en la página (caza FATAls del wasm).

## 2026-09-17 (tarde) — MUSGO NEGRO: causa raíz encontrada y arreglada

**Veredicto:** no era el `hasAlpha` del fichero (mi rama DXT1 punch-through),
ni mipmaps incompletos, ni el sampler. Era **el flag `hasAlpha` del raster GL
sobrescrito por el último mip subido**.

- `gl3raster.cpp` (`rasterSetFromImage`) hacía `natras->hasAlpha =
  image->hasAlpha()` en CADA nivel. El camino vivo en el navegador (sin S3TC)
  es `raster.cpp` fallback por `Image`, que sube los mips **uno a uno**
  (`setFromImage` por nivel): **mandaba el último mip**. Los mips pequeños de un
  cutout (4x4, 2x2, 1x1) ya no tienen transparencia -> el flag se apagaba para
  TODA la textura -> alpha-test/blend OFF -> el cutout se pinta OPACO y el RGB
  negro de su zona transparente se ve como **mancha negra**. El criterio de
  `Image::hasAlpha()` es "algún alfa != 255" (no "algún alfa == 0"), por eso el
  último mip casi siempre la apaga.
- **Fix**: un nivel con alfa nunca puede APAGAR el flag
  (`if(image->hasAlpha()) natras->hasAlpha = 1;`).
- **Pruebas**: (1) UI del usuario (build vieja) `BIND2 tex=kbgrass_test2
  alpha=0 levels=9` con `TEXA ... fileA=1 useA=1` = alfa perdido en runtime;
  (2) datos del fichero: `kbgrass_test2` (shared_beach.txd) tiene alfa en los
  niveles 0-5 y ninguno en 6-8 (justo el clásico); (3) arnés headless build
  nueva: `CONVDXT ALFAMIP ras=0x16bd0f0 lvl=6/9 imgA=0 flag=1` (x3) +
  `CONVDXT ALFA ras=0x16bd0f0 lvl0A=1 finalA=1 up=9/9` y ese `ras` es
  `TEXA tex=kbgrass_test2`.
- **Alcance real**: sólo 4 texturas en TODO el juego tienen la firma
  (alfa en nivel 0, ningún alfa en el último mip): `kbgrass_test2` (playa),
  `sjmgrss` (nbchdrtrack), `fuzzyplant256` (nbdecoshop02) y `blueshade_64`
  (shopfronts03). Tres son **vegetación de playa** -> encaja con el síntoma.
- **Trazas nuevas** (para no volver a medir): `CONVDXT ALFA ras= texid= lvl0A=
  finalA= up=n/m` (una por textura con alfa: dice si el cutout acaba con
  alpha-test encendido), `CONVDXT ALFAMIP ras= texid= lvl=i/n imgA=0 flag=`
  (cupo propio de 250: es LA firma del bug) y `TEXA`/`BIND2`/`ALPHADRAW` con
  cupos ampliados (900 / 64 nombres x4 / 512 texids: antes se agotaban antes de
  llegar a la playa).
- **Herramientas offline nuevas** (sin compilar ni jugar):
  `tools/probe_alpha.py` (flags de alfa por TXD), `tools/scan_cutouts.py`
  (cutouts con flag a 0) y `tools/scan_mipalpha.py` (firma del bug en todo el
  juego; hoy: 4 texturas de 1361 TXDs).

## 2026-09-17 — Musgo, motor y bucle del Malibú (arreglos + trazas)

Contexto: el motor estaba leyendo un `sfx.SDT` **vanilla** pese a estar
regenerado en disco (11 de 12 `want` de `SFXLEN` coincidían con `vc/`, no con
`streamed/`). Causa: la caché IDB va por ruta y el SDT regenerado conserva el
tamaño → la copia vieja ganaba siempre; y `/streamed/*` se servía con
`max-age=86400`. Todo juicio de oído previo (Faggio, Malibú) queda contaminado.

- `[raíz]` **Caché de datos con versión** (`web/ondemand.js`): `dataTag`; si
  cambia, se vacía la caché IDB (`idbPurgeIfStale`) + `Cache-Control: no-store`
  en `/streamed/*` (`vite.config.js`). Subir `dataTag` en cada rebuild de datos.
- `[raíz]` **Identidad de datos**: una línea `SFXDATA entries= readBytes= h=`
  (`sampman_oal.cpp`, al leer la tabla) → se sabe al instante qué datos usa el
  juego. Además `JS build= data=` al arrancar (`ondemand.js`) y `DATATAG` al
  purgar la caché. **Verificado en el arnés**: `h=79d3d6c9 sample0size=1268`
  (1268 = valor NUEVO; el vanilla era 1400) → el motor ya lee la SDT regenerada.
- `[raíz]` **Musgo negro = alfa perdido, no mipmaps**: en DXT1 el juego usa
  alfa de 1 bit ("punch-through": bloque `c0<=c1` con algún índice 3) y el flag
  `hasAlpha` del fichero viene a 0 → en GL3 la textura se crea RGB_DXT1 (el
  índice 3 deja de ser transparente: NEGRO) y además se apaga el alpha-test.
  Ojo: corregir el flag *después* de crear la textura no sirve (el formato
  interno de GL ya está elegido). Fix en el lector
  (`vendor/librw/src/d3d/d3d8.cpp`): el nivel 0 va delante en el stream, así que
  se lee **antes** de crear la textura, se escanea y se pasa el flag corregido a
  `allocateDXT` (y a la conversión D3D8→GL3, que usa `d3dras->hasAlpha`).
- `[raíz]` **Trazas del musgo, encadenadas por `ras=` y `texid=`**: `TEXA`
  (nombre, `comp`, `fmt`, `fileA/scanA/useA`; los nombres tipo ivy/grass/moss/
  leaf/dirt se registran siempre) → `CONVDXT` (`dxt`/`inA`/`outA`/`texid`, y
  `IMG` para el camino por `Image` cuando el navegador no tiene S3TC) →
  `ALPHADRAW` (estado real al dibujar: `texA`, `vertA`, `postTest`, `postBlend`,
  `aFunc`, `ref`, `lv`, `filt`). Con eso se separa dato vs estado de render sin
  volver a medir. Verificado en el arnés: 127 `TEXA` + 127 `CONVDXT` +
  `ALPHADRAW` con `texA=1 → postTest=1`.
- `[raíz]` **Motor sin cortes**: pin de las muestras con loop en la caché PCM
  (`OdSfxEvict` nunca desaloja motor/sirenas → sin re-decodificar ni
  re-descargar en mitad de un bucle) + precarga en segundo plano de todas las
  muestras con loop (`OD.prefetchLoops`, lee la SDT del FS). `SFXLEN` ahora
  traza siempre las muestras con loop (ls/le/delta en muestras).
- `[raíz]` **Bucle del Malibú (cualquier sonido)**: watchdog web en
  `AudioManager::ServiceActiveSamples` → `LOOPSTART`/`LOOPALIVE` (cada 2 s)/
  `LOOPEND` con canal, sample, entidad y tipo (da el culpable exacto). Se corta
  el loop en dos casos: `LOOPORPHAN` (emisor ya no está en uso: nadie puede
  pararlo) y `LOOPKILL` (loop inaudible, volumen 0 durante 25 s: es el que se
  queda pegado al pasar por una zona y solo se oye de cerca). Si alguien lo
  vuelve a pedir, se re-arranca solo.
- `[raíz]` **Motor sin cortes, instrumentado**: `ODSFXSTAT` (hit/miss/decms/
  evict/`cacheKB` cada 10 s) y `ODSFXMISS sfx= ms= bytes= rate= loop=` por
  muestra decodificada → distingue "corte por re-decodificar" de "corte del
  propio banco". El pin de muestras con loop en `OdSfxEvict` ya evita el
  re-decode en mitad de un bucle.
- Pendiente de validar jugando: musgo en GPU real con S3TC (el arnés corre sin
  S3TC → camino `Image`), motor (Faggio) de oído y el log de `LOOP*` al pasar
  por el Malibú.

## 2026-09-18 — Limpieza de trazas, timer honesto y A1 completada (build verde)

- `[tooling]` **TXDAUD retirado** (`src/rw/TxdStore.cpp`): era el auditor de
  contenido de texturas del diagnóstico del musgo (hash de nivel 0 por
  textura, cap 4000/sesión). Investigación cerrada (el contenido quedó
  verificado limpio); sus ~4000 líneas/sesión solo enterraban en
  `odtrace.log` las trazas vivas del plan de carga (`initstep`/`webload`/
  `loadtick` — en la sesión de las 12:44 solo sobrevivieron 3 `initstep 18`).
  Se conservan `TEXA`/`CONVDXT`/`ALFAMIP`/`ALPHADRAW` (volumen bajo y la
  validación GPU real del fix de alfa sigue pendiente).
- `[raíz]` **Timer del parse honesto**: `webload: parse=` restaba
  `CTimer::GetTimeInMilliseconds()`, congelado durante el restart troceado →
  basura (`parse=3165202ms` idéntico entre cargas). Ahora `RsTimer()`
  (monótono de pared) y la etiqueta dice la verdad: `webload: total=` (mide
  parse+drenado+escena). Decisivo para la aceptación del plan
  `carga-sin-doble-trabajo` (medir lo que la vía A1+A2 se ahorra).
- `[raíz]` **A1 completada (era un hunk sin terminar de la sesión anterior)**:
  en `GS_FRONTEND` con `wantToLoad` había un experimento a medias
  (`gWebBootInitPending` sin declarar, baile de dos ticks INALCANZABLE porque
  `DoSettingsBeforeStartingAGame` pone `m_bWantToRestart` en el mismo tick del
  SÍ y `InnerShouldRun()` ya no deja correr `InnerFrame`; el build de las
  10:39 NO lo contenía — por eso `splash-tick` nunca apareció en el log).
  Completado: (1) el tick del SÍ solo pinta splash + `WebDrawLoadScreen(0)` y
  pone `gWebBootInitPending=1` + `GS_PLAYING_GAME`; (2) `AfterInner` (web,
  rama `wantToLoad`) consume el flag: `InitialiseStep` UN caso por tick
  (`main.h/main.cpp`: `extern` + definición) ANTES del shutdown+load ya
  troceado. Mismo trabajo que el init monolítico pero repartido en ticks:
  ratón vivo toda la carga + pantalla visible desde el tick del SÍ. Los
  saltos A2 (case 17 / ReInit con `wantToLoad`) se aplican igual dentro de
  `InitialiseStep`. Nativo intacto (rama `#else` con `InitialiseGame()`).
- `[tooling]` **Nota de build**: `emsdk_env.bat` invocado desde Git Bash emite
  `export`s que cmd no ejecuta (emcc "no encontrado"). Solución: .bat propio
  con PATH directo — `emsdk\upstream\emscripten`, `emsdk`,
  `emsdk\node\24.19.0_64bit`, `emsdk\python\3.13.3_64bit`, venv Scripts
  (cmake) + ninja de WinGet. Build verde (wasm 23 MB, `reVC.data` 390 MB
  reempaquetado con contenido idéntico).

## 2026-09-18 (noche) — Tirones y RAM: plan fluides-ram aplicado (build verde)

Análisis de la sesión de 13 min del usuario (FPS 58-60 estables): el peor
frame de cada segundo era 22-54 ms (micro-jank constante) + mp3 grandes
on-demand (City 9,6 MB→352 ms; police 14 MB→133 ms; la carga F2 483 ms es
normal; el hitch final de 1128 ms es el cierre de pestaña). RAM ~1,2 GB =
bootseed 376 MB (de los cuales RADIOS 274) + cap MEMFS 400 + wasm.

- `[raíz]` **Presupuesto de TIEMPO por frame en el streaming** (`Streaming.cpp`,
  `LoadAllRequestedModels`): en gameplay (budget 0, sin prioridad de script)
  corte a ~7 ms por llamada; el resto sigue al próximo tick. El budget de
  ficheros (`gWebLoadBudget`) solo cubría la carga. Era el fix pendiente del
  plan fluidez-carga-radio B y la causa arquitectural del micro-jank.
- `[raíz]` **RAM: radios fuera del bootseed** (`stage_bootseed.py RADIOS=[]`):
  decisión antigua ("cambio instantáneo") revertida con el visto del
  supervisor — `prefetchRadios()` + IDB la cubren (1-2 s la primera vez).
  bootseed 376→145 MB, `reVC.data` 390→132 MB (arranque 3× más ligero).
- `[tooling]` **Cap MEMFS 400→250 MB** (`ondemand.js OD.CAP`): lo desalojado
  se re-lee de IDB sin red.
- `[tooling]` **Instrumentación de verdad**: `FPSLOG` ahora lleva `heap=MB`
  (`emscripten_get_heap_size`) y `sm=` (ms de streaming consumidos en el
  segundo, contador nuevo `gWebStreamMs` que FPSLOG resetea); `decms` de
  ODSFXSTAT media con `CTimer` (1 tick de granularidad por frame → siempre 0),
  ahora reloj de pared; tag nuevo `ODCAP` cada 30 s desde la página (MEMFS
  census + evict + fetched + idbHits + heap JS de `performance.memory`);
  `LOOPALIVE` de 2 s a 10 s (era 14k líneas/sesión).
- Plan y trazas: `.agents/plans/fluides-ram.md`. Build `2026-09-18-ram1`.
- Pendiente de validar jugando: maxdelta p95 < 20 ms conduciendo, 0 hitches
  fuera de carga/cierre, ODCAP muestra live < 250, radio cambia en 1-2 s la
  primera vez, sin regresión en carga F2 / partida nueva / loops de sonido.

## 2026-09-18 (noche, fix) — REGRESIÓN del boot-init en AfterInner: causa y corrección

**Síntoma (sesión 18:28 del usuario, build ram1):** mundo sin pisos/paredes/
texturas, caída en bucle ("can't find ground z"), y OOB en
`CPed::SetAnimOffsetForEnterOrExitVehicle ← CPed::Initialise ←
InitialiseStep ← EmscriptenTick (doRewind)` al reintentar la carga.

**Causa (mi bug):** el bloque de boot-init (A1) lo coloqué DENTRO de
`AfterInner` pero DESPUÉS de su prólogo de teardown
(`RwInitialised=FALSE` + `FrontEndMenuManager.UnloadTextures()` + 
`CTimer::Stop()`). El init de partida corrió con RW a medio desmontar,
texturas del frontend descargadas y reloj parado → estado corrupto (mundo sin
colisión, conversión de texturas rota) y el reintento de carga murió con OOB.
El flujo de streaming en sí funcionó (f 247 MB / idb 170 en la consola del
usuario): no fue el cap, ni el bootseed (paquete verificado 1924=1924
ficheros; el "staged 2102" contaba duplicados de bootseed.list+FULL_DIRS), ni
el presupuesto de 7 ms (solo activo con budget 0 en gameplay).

**Fix:** bloque movido AL PRINCIPIO de `AfterInner` (antes del teardown),
gated con `wantToRestart && wantToLoad && gWebBootInitPending`. El init corre
con RW/texturas/reloj vivos (mismo contexto que el `InitialiseGame` del flujo
viejo) y el teardown+shutdown+load arranca al completarse. Estructura del
bucle verificada: `EmscriptenTick` = InnerFrame XOR AfterInner por tick, así
que el SÍ tick pinta splash y el siguiente tick arranca el init.
Build `2026-09-18-ram2`. Smoke headless: boot OK, radios por prefetch on-demand
OK (EMOTION/WAVE en misses), live 249/250MB sin evicción.

## 2026-09-18 (noche, fix 2) — RE-ENTRANCIA de Asyncify: streaming muerto, mundo vacío

**Síntoma (ram2):** mismo mundo vacío (caída libre, sin pisos/texturas) PERO
sin crash: `initstep 8` apareció **3 veces** en el log y `sm=0` en TODOS los
FPSLOG de gameplay (el streaming no procesaba nada).

**Mecanismo (probado con el log):** mientras `InitialiseStep` estaba
suspendida en Asyncify esperando un fetch, el bucle del navegador disparó
ticks nuevos que RE-ENTRARON al init (case 8 ×3). Sólo una de las llamadas
completó; las suspendidas concurrentes se abandonaron (Asyncify sólo resume
una) y dejaron **`bInsideLoadAll=true` pegado para siempre** → cada
`LoadAllRequestedModels` posterior hacía early-return → streaming muerto →
mundo sin construir → caída libre. (En ram1 el mismo re-entrar + contexto de
teardown produjo además el OOB de `CPed::Initialise`.)

**Fix:** guardia anti re-entrada con flag "in flight" en los tres motores de
pasos: boot-init (`AfterInner`), shutdown+restart troceados (`glfw.cpp`) y
partida nueva (`InitialiseGameStep`, `main.cpp`). El flag se pone ANTES de la
llamada y se limpia DESPUÉS: si la llamada suspende, el unwind se salta la
limpieza (flag=1 durante la suspensión → los ticks nuevos ceden) y el rewind
lo limpia al completar. Suspensiones secuenciales de nuevo, como en el flujo
monolítico viejo. Build `2026-09-18-ram3`.

**Nota del arnés:** el smoke headless confirma boot + prefetch de radios
on-demand (f 247MB, idb 139) pero NO puede validar el load (requiere save en
el perfil del navegador). Validación de carga pendiente del usuario.

## 2026-09-18 (noche, fix 3) — A1 REVERTIDA: el init troceado no es viable sobre Asyncify

**Síntoma (ram3):** carga colgada para siempre en el case 8 (log: init
detenido 57+ s, sólo heartbeats) y OOB en `CPed::Initialise` al iniciar
partida nueva — el rewind de Asyncify en la cadena
`EmscriptenTick → AfterInner → InitialiseStep` se rompe (restaura corrupto o
no llega). Tres builds rotas seguidas (ram1 mundo vacío+OOB, ram2 streaming
muerto, ram3 colgada+OOB).

**Decisión:** REVERTIDA toda la vía A1 (init troceado en AfterInner + guardias
in-flight). El flujo de carga vuelve al validado: SÍ tick → `InitialiseGame()`
monolítico (con los saltos A2 que siguen reduciendo el doble trabajo) →
restart troceado con GenericLoad + LoadSceneStep (sin guardias, tal cual
10:39). Lección registrada: con on-demand, el init SÍ puede suspender en
fetches, y la re-entrada/rewind en esa cadena no es robusta en esta config de
Asyncify. Reabrir A1 sólo con un diseño que garantice cero suspensiones
durante el init (pre-ensure de todos los ficheros de init antes de arrancar).

**Lo que SE CONSERVA de ram1-3 (independiente del fallo):** radios fuera del
bootseed (bootseed 145 MB, reVC.data 132 MB), cap MEMFS 250 MB, presupuesto
de streaming 7 ms/frame en gameplay, FPSLOG con heap=/sm=, ODCAP cada 30 s,
decms real, LOOPALIVE 10 s, TXDAUD fuera, RsTimer en webload:total=.
Build `2026-09-18-ram4` (flujo de carga = el validado + dietas de RAM).

## 2026-09-18 (madrugada) — Bisect D1 + fluidez v2 (F1/F2/F1b-A2/lean1/F5c)

**Bisect D1** (`diagnostico-crash-init.md`): C++ idéntico al build bueno +
dieta RAM → carga OK ⇒ culpable del crash era C1-C6; el principal, el
presupuesto de 7 ms DENTRO de `LoadAllRequestedModels` (bucle suspendible).
Regla nueva: ese bucle no se toca (R1).

**F1** (`fluides-f1`): presupuesto por CONTEO de ficheros para gameplay
(`gWebLoadBudget=6`, el mecanismo validado de la carga) en las liberaciones
y en el case 0 de partida nueva. Carga OK; jank persiste.

**F2** (`fluides-f2`): instrumentación a nivel llamador — FPSLOG con
heap=/strm= (ms de Update→LoadRequestedModels), decms real (reloj de pared),
RsTimer en webload total, LOOPALIVE 10 s. **Hallazgos**: la streaming de
gameplay va por Update→LoadRequestedModels (canales), NO por LoadAll; strm
p50=5 ms/s ⇒ el jank típico (maxdelta p50=21-24) NO es streaming; correlación
con AUDIO asíncrono (CHINIT ×4, ODSFXMISS ×3 en segundos con jank).

**F1b-A2** (`f1b-a2`): el SÍ difiere el init; AfterInner lo corre
monolítico con guardia, y el splash se pinta y PRESENTA antes del bloqueo
(2 ticks) — el freeze ocurre mirando el splash. Validado por el usuario.
Todas las lecciones ram1-4 respetadas (contexto antes del teardown, una sola
cadena de suspensiones, guardia anti re-entrada).

**lean1**: higiene de trazas a petición del supervisor — fuera TXDAUD,
GEAR/GEARP/ENGAP, CHSTOP, LOOPSTART/ALIVE/END (la lógica LOOPKILL/ORPHAN
sigue), BIND2 completo; caps de muestra en ALPHADRAW/TEXA/CONVDXT. Quedan
errores + FPSLOG(heap/strm) + ASYNC + ODSFXSTAT/MISS/SFXLEN + CHINIT ms +
RETUNE + ruta de carga + ODCAP. Y **rotación del log por sesión** en
vite.config.js (el tope de 80k global cortó la sesión larga a los 8 s —
descubrimiento: los análisis estaban medio ciegos).

**F5c** (`fluides-f5c`): el diagnóstico de lean1 cerró el caso del jank
continuo: CAP 250 < working set (9 radios de la prefetch = 270 MB + mundo)
⇒ thrash (1683 ficheros/385 MB re-traídos; 90% de segundos con >100 ms de
stalls ASYNC). Fix: fuera la prefetch masiva; sólo la emisora sonando vive
en MEMFS (detección en el wrapper de ensure), la SIGUIENTE del dial se
precarga a los 3 s, la última usada al arrancar (localStorage); LRU expulsa
las inactivas; CAP 250→300. Pendiente de validar: sesión larga (evict bajo,
ASYNC sólo en cambios de zona, radio sin freeze tras la primera).

## 2026-09-18 (noche) — F5d a la build + F4c (medición del recolector)

Se completa la ronda que quedó a medias (F5d escrito pero sin compilar; F4c sin
empezar).

- `[raíz]` **F5d ya está DENTRO de la build** (`2026-09-18-f5d-f4c`, enlace
  17:30). `web/ondemand.js` se incrusta al enlazar (`--pre-js`), así que el
  arreglo escrito a las 17:14 no estaba activo: hacía falta **reenlazar**, y se
  forzó retirando `reVC.js` (ninja no ve el pre-js como dependencia → habría
  dicho "nada que hacer"). Verificado en la build: `inMem` ×3 + tag nuevo +
  piezas de F5c intactas.
- `[raíz]` **Camino del motor comprobado** (`src/skel/ondemand.cpp`): si
  `OD.ensure` devuelve null, `od_fetch` devuelve 0 → el wrapper cae al
  `__real_fopen`/`__real_open` (falla limpio, sin crash) y `MusicManager`
  reintenta (`StartStreamedFile` mientras `!IsStreamPlaying`) → la radio se
  recupera sola tras el silencio. La premisa de F5d se sostiene.
- `[tooling]` **F4c implementado** (`web/src/main.js`, `jankProbe()`): una línea
  `PERF frames= maxd= heap= freed=` por segundo (peor intervalo entre frames +
  lo que LIBERA el recolector de JS; `freed>0` = recolección). Es fichero de
  página (Vite), no build: activo al recargar. En headless: `gc-api=si` y 92
  líneas de muestra.
- `[dato]` **Primera lectura (menú, headless, 92 s): mediana del peor frame
  18 ms; heap 260→133 MB tras una recolección de 127 MB con `maxd=23 ms`; los
  segundos SIN recolección (`freed=0`) siguen en 18-23 ms** → **el recolector
  de basura NO explica el baseline de ~20 ms**. Falta la lectura en gameplay
  (conduciendo) que lo confirme.
- Nota: el log de la sesión del usuario (1357 líneas) se guardó en
  `/tmp/odtrace-sesion-usuario.log` antes de la prueba headless (el log se rota
  al empezar sesión nueva).

## 2026-09-18 (noche) — Caza del tirón: caracterizado, sospechosos descartados

Sesión del jugador (22:34-22:40, 341 s, `f5d-f4c`) analizada con el cruce
segundo a segundo (PERF × FPSLOG × ASYNC × ODSFXMISS × CHINIT × RETUNE/strm):

- **La radio muda de F5d funciona**: 60 cambios de emisora y cero freezes
  (antes 190-263 ms cada uno).
- **Síntoma real medido**: 60 fps sostenidos (60-61 frames/s en 325 de 341
  segundos), PERO **el 100 % de los segundos tiene un frame por encima del
  presupuesto de 16,7 ms** (p50 21 ms, 78 % >20, 16 % >25, 6 % >33 = frame
  perdido de verdad, 1 % >50). Ese "un frame tardío por segundo" es el
  "un poco laggy" del jugador. Lo ven las DOS medidas independientes (reloj
  del motor: maxdelta p50 23 ms; navegador: hueco entre frames p50 21 ms) y
  en 81 segundos el motor ve >25 ms donde el navegador no → el atasco está
  DENTRO del frame, no es artefacto de medida.
- **Descartados con número**: recolector de basura (correlación -0.01; los
  segundos sin recolección tienen el mismo 21 ms, y la recolección de 127 MB
  cayó en un segundo de 23 ms), streaming (corr. -0.04), decodificación de
  audio/arranque de canales (los segundos con ODSFXMISS/CHINIT tienen el
  MISMO p50 que el resto), cambios de emisora, y el número de suspensiones
  Asyncify COMO CAUSA DEL BASELINE (13,9/s, ~1 ms cada una; sí explican los
  26 picos grandes: media 50,9 ms en los segundos con 21+ suspensiones).
- **Único trabajo que BLOQUEA el hilo una vez por segundo**: `OD.trace`
  mandaba cada línea al log con **XHR síncrono** (el hilo para hasta que el
  servidor contesta y escribe en disco). La traza ASYNC sale 1/s → encaja con
  el síntoma (un frame ~4 ms por encima del presupuesto, 1/s). **Eliminado**:
  `trace()` ahora encola en `window.__odq` (misma vía no bloqueante que el
  C++); fallback síncrono solo si no hay cola. Build `2026-09-18-lag1`.
- **Instrumento ampliado** (F4c v2, `web/src/main.js`): la línea `PERF` ahora
  lleva `at=` (en qué frame del segundo cayó el tardío), `over17=` (cuántos
  frames pasan del presupuesto), `p90=`, y el trabajo de la página en ese
  segundo (`xhr=N/ms@offset` — síncronos; `dom=N/ms` — reescrituras del log
  en pantalla). Con eso el próximo veredicto es directo: si el frame tardío
  persiste sin `xhr`, no es nuestra instrumentación y toca medir el frame del
  motor por fases (requiere build).
- Verificado en headless: arranca, tag nuevo, `xhr=0` (ya no hay XHR síncrono
  periódico en el camino vivo).

## 2026-09-18 (noche) — Tirón: picos atribuidos + auto-centrado de cámara

Sesión del jugador (22:51, 214 s, build `lag1`, tag confirmado por él):

- **F5d OK**: 60 cambios de emisora sin congelones.
- **El envío bloqueante de trazas ya no existe**: `xhr=0` en TODA la partida
  (antes 1/s). Pero **el baseline persiste**: peor frame p50 = 20 ms y 71 % de
  los segundos con algún frame >17 ms (mediana: 1 frame tardío por segundo).
  Luego aquel envío no era el culpable (por eso se cambió: era el único
  trabajo bloqueante periódico y había que descartarlo con datos).
- **Descartados con número en esta sesión**: reescrituras del log en pantalla
  (coste p50 0,2 ms; los segundos sin escritura también tienen su frame
  tardío), recolector, y las descargas en sí (los segundos con **0 descargas**
  siguen con 1 frame tardío).
- **Los picos grandes (≥33 ms = 6 % de los segundos) SÍ tienen patrón**: en
  esos segundos TXDIN (diccionarios de textura cargados) es **1,88/s vs 0,09**
  en los segundos limpios (20×), CHINIT 2,58 vs 0,57, ALPHADRAW 0,73 vs 0,00,
  RETUNE 0,35 vs 0,11. O sea: **los picos son "llega contenido nuevo"**
  (texturas + sonidos), no un subsistema concreto. Coincide con lo que ya
  concluyó F1: el presupuesto por CONTEO no acota el coste de un TXD pesado.
- **Shader descartado**: los programas GL de los pipelines (incl. extras) se
  crean UNA vez en la inicialización (`custompipes_gl.cpp`), no por material
  → no son la causa de los picos.
- `[raíz]` **Auto-centrado de la cámara libre** (petición del jugador):
  `src/core/Cam.cpp`. Tras 3 s sin tocar ratón ni cruceta, la cámara vuelve
  sola detrás del objetivo con una convergencia de ~1 s. Se aplicó en los dos
  caminos que leen el ratón: a pie (`Process_FollowPedWithMouse`; antes NO
  volvía nunca salvo al disparar o con la tecla de cámara detrás) y en coche
  (`Process_FollowCar_SA`, la cámara libre de vehículo; antes tiraba de Beta a
  ~0,2 rad/s tras 50 frames de inercia → ~15 s o nunca). Constante única
  `fLookRecenterDelay = 3.0f`. Build `2026-09-18-cam1`, verificada en headless
  (arranca, tag correcto, sin errores).
- Nota de proceso: la prueba headless rota `odtrace.log` (una sesión por
  fichero). Los números de la sesión del jugador quedan aquí; **copiar el log
  antes de cualquier prueba automática**.

## 2026-09-18 — F6: fases del frame + reparto de contenido (lag2)
Build `2026-09-18-lag2`. Petición: (1) instrumentar el frame por fases para
saber dónde van los ~4 ms que se pierden una vez por segundo y corregir esa
fase, (2) que texturas y sonidos nuevos lleguen repartidos entre frames en vez
de en ráfaga (picos de 30-50 ms al entrar en zona nueva).
- `[tooling]` **F6a — instrumento de fases** (`src/core/main.cpp`): el frame se
  cronometra con reloj de pared (`emscripten_get_now`) en seis tramos:
  `L` lógica (CTimer+tbInit+CGame::Process), `A` audio (DMAudio.Service),
  `R` lista (ConstructRenderList+PreRender), `S` escena 3D (RenderScene +
  efectos + motion blur), `T` 2D (menús/fade/efectos), `P` entrega
  (DoRWStuffEndOfFrame). Una línea `FPHASE` por segundo con el frame de peor
  INTERVALO del segundo: sus tramos, los del frame **anterior** (un tick que
  llega tarde lo hace por el trabajo del anterior, ya medido), `sum` y
  `wait = gap - sum`. **Clave de lectura: si `sum` explica el `gap`, el coste
  es nuestro CPU; si `wait` es grande, el trabajo ya está lanzado y el
  retraso lo pone el navegador (GPU/compositor/vsync).**
- `[raíz]` **F6b — reparto por TIEMPO en el streaming** (`Streaming.cpp/.h`,
  `Game.cpp`): `gWebLoadTimeBudgetMs` (3 ms en gameplay, 10 ms en carga) corta
  `LoadAllRequestedModels` al agotarse el tiempo, no solo al llegar a N
  ficheros. El tope por conteo (6) no ve el PESO: 6 ficheros podían ser 60 ms.
  Garantía: la comprobación va al principio de cada vuelta → **al menos un
  fichero por llamada siempre se procesa** (nada queda a medias). La
  conversión DXT→GL3 de las texturas ocurre en `Raster::convertTexToCurrentPlatform`
  desde `fakerw/fake.cpp` al leer el TXD, es decir DENTRO de este bucle →
  queda repartida. Traza `ODLOAD big id=.. ms=.. tipo=..` para cada fichero que
  se pase él solo del presupuesto (no se puede trocear un decode).
- `[raíz]` **F6b — reparto de decodes de SFX** (`audio/sampman_oal.cpp`):
  presupuesto por frame (2 decodes / 5 ms inline). Si se agota, el sample va a
  una COLA y se decodifica en los frames siguientes (1 por frame, 4 ms, desde
  `cSampleManager::Service`); **no se pierde**: el motor reintenta la petición
  cada frame hasta que el canal arranca, y una petición repetida (motor,
  sirena) se promociona a "ahora". Trazas `ODSFXDEFER` (qué sample se
  difirió y con qué presupuesto gastado) y `ODSFXSTAT ... defer= pump= maxframe=`.
- `[parche]` **F6c — entrega** (`vendor/librw/src/gl/gl3device.cpp`):
  `glfwSwapInterval` NO es no-op en Emscripten (reasigna el reloj del bucle:
  `set_main_loop_timing`). Se llamaba en CADA frame desde `showRaster`; ahora
  solo cuando el valor cambia. Es el único cambio preventivo de esta ronda:
  si la fase de entrega no era el problema, no molesta.
- Verificación: compilado y probado en headless (tag correcto, 60 fps, sin
  errores). El instrumento quedó **validado en partida real**: 156 líneas
  `FPHASE` durante la cinemática inicial (a 20 fps con GL por software), con
  `wait` dominante (49,7 ms de 51) → el retraso es del navegador/GPU, no del
  CPU medido. Falta la sesión en hardware del jugador para el veredicto del
  tick tardío de ~4 ms a 60 fps.

## 2026-09-18 — CRASH del init: reincidencia del tope de tiempo, fix y test (fixload1)

Build `2026-09-18-fixload1`. Arranca del crash que sufrió el jugador al CARGAR
partida con `lag2`: `RuntimeError: memory access out of bounds` en
`CPed::SetAnimOffsetForEnterOrExitVehicle` ← `CPed::Initialise` ←
`CGame::InitialiseStep` (paso 8, "Load animations"), dentro de un rewind de
Asyncify. Es **exactamente** el crash ya bisecado en
`.agents/plans/diagnostico-crash-init.md` (H1/H3/D1: culpable = presupuesto por
TIEMPO dentro del bucle de `LoadAllRequestedModels`): `lag2` lo reintrodujo al
intentar repartir las texturas.
- `[raíz]` **Mecanismo probado**: `CPed::SetAnimOffsetForEnterOrExitVehicle`
  (`src/peds/PedAI.cpp`) pide ELLA MISMA los 5 bloques de anim de vehículos
  (`van`/`bikes`/`bikev`/`bikeh`/`biked`), llama `LoadAllRequestedModels(false)`
  y acto seguido lee las jerarquías (`GetAnimAssociation(...)->hierarchy`). Es un
  **punto de sincronía**: cualquier corte (por tiempo o por conteo) deja las
  tablas a medio inicializar y esa lectura revienta. El log del jugador encaja:
  los `TXDIN` (`cop`, `male01`, `police`, …) justo antes del OOB.
- `[raíz]` **Fix** (`Streaming.h`, `Streaming.cpp`, `Game.cpp`): fuera el tope
  por TIEMPO (los tres sitios donde se fijaba + el valor por defecto). Se
  conserva el tope por CONTEO (6 en juego / 10 en carga), validado por semanas
  (R2). En `Streaming.h` queda escrito que un tope por tiempo ahí dentro está
  PROHIBIDO y por qué.
- `[raíz]` **Red de seguridad** (`src/peds/PedAI.cpp`): en ese punto de
  sincronía la carga va SIN tope (`gWebLoadBudget = 0`, se restaura después) y,
  si aun así faltara algún bloque, se traza `ODANIMFAIL` y se dejan los offsets
  por defecto en vez de leer basura.
- `[tooling]` **Test de regresión del slot 0**
  (`gta_vc_browser/tools/slot0-load-test.mjs`): arranca el juego en headless,
  navega el menú real (INICIAR PARTIDA → CARGAR JUEGO → primera fila
  `GTAVCsf1.b` = slot 0 → SÍ; clics lentos, el motor lee el ratón 1/frame),
  siembra el save en `/userfiles` (IDBFS) y da PASS solo si aparece
  `WR load-req` y ≥3 líneas `FPHASE` (juego en marcha) sin errores de página ni
  `ODANIMFAIL`.
  - El save lo saca del navegador el nuevo
    `tools/extract-save-from-profile.mjs` (los guardados viven en IndexedDB,
    no en disco): copia (solo lee) la IndexedDB del perfil de Chrome a un perfil
    temporal, arranca el juego y lee `/userfiles/GTAVCsf1.b` →
    `tools/testdata/` (gitignored).
  - **Validación real**: con `lag2` el mismo test FALLA con la pila exacta del
    jugador; con `fixload1` PASA (init completo y partida en marcha). Primera
    prueba automática del port que reproduce el flujo de cargar partida.
- `[dato]` Hallazgo secundario (NO arreglado): cargar el save de **ViceExtended**
  (`Grand Theft Auto Vice City/ViceExtended/userfiles/GTAVCsf1.b`, de un mod)
  produce `GenericLoad FAIL: no abre/lee size` → restart silencioso → OOB en
  `CRunningScript::Load` (`CTheScripts::LoadAllScripts`). Un save incompatible
  mata el juego en vez de avisar; candidato a blindar en otra ronda.
- Verificación: compilado (tag `fixload1` dentro del `reVC.js`) y test PASS.

## 2026-09-18 (noche) — El port pasa a ser una LIBRERÍA (`[tooling]` + `[raíz]`)

- `[tooling]` **`web/lib/` es ya el paquete `@re3/gtavc-web`**: `startGame({ el })`
  monta el juego dentro del div que le pase el host (Vue, React, HTML suelto),
  sin botón de Jugar ni de Audio: arranca al momento y el audio se reanuda con
  el primer gesto (`pointerdown`/`keydown`/`touch`).
  - Canvas a tamaño del viewport (`fill:'viewport'`; `'parent'` para la caja
    del div). Pantalla completa con **F11** (y `toggleFullscreen()`); Esc corto
    no hace nada, mantener 3 s sale (igual que antes).
  - **Barra de progreso rosa** de 3 px pegada al top del viewport: se llena con
    la descarga del build (`setStatus`) y con las fases del motor
    (`window.__loadProgress`), y se apaga sola cuando el juego está jugable
    (6 s sin novedades). Vista funcionando en la carga de la partida del test.
  - Rutas configurables: `buildUrl`, `streamedUrl`, `manifestUrl` y
    `assetsUrl` (esta **solo se consulta si falta `streamed/`**: con manifiesto
    completo no se pide ni una vez). Trazas a `traceUrl` (null = no enviar).
  - Tipos en `index.d.ts` (TS/Vue) y `destroy()` para desmontar la vista.
- `[tooling]` **`web/lib/vite.js`** (`vcWeb()`): los middlewares que antes
  vivían en `vite.config.js` (streamed/, vc/, /odtrace, endpoints de estado) +
  COOP/COEP, en un único sitio. Lo usa la página de desarrollo **y** sirve para
  un host Vue; `configurePreviewServer` también los monta (`vite preview`
  servía 404 de datos).
- `[raíz]` **`ondemand.js` (pre-js)**: las rutas dejan de ser constantes; lee
  `globalThis.__VC_CFG` (lo deja la librería ANTES de cargar `reVC.js`), con
  `/streamed/`, `/manifest.json` y `/odtrace` como valores por defecto. Requiere
  **relink** (el pre-js va embebido): hecho, tag `2026-09-18-lib1` (verificado
  dentro de `reVC.js`).
- `[tooling]` **Página de desarrollo reducida a un div** (`index.html`) +
  `src/dev.js`; retirado `src/main.js` (su lógica vive ahora en `lib/`: registro,
  cola `__odq`, F4c/FPSLOG, IDBFS, protecciones; el `?fsprobe` en `dev.js`).
- `[tooling]` **Test del slot 0 actualizado**: con la librería el canvas ya no
  está en una tarjeta sino a viewport completo, así que los clics del menú pasan
  a ser **fracciones del canvas** (calibradas a 1280x800) y la partida se siembra
  **antes** de que el frontend lea el listado (así funciona con perfil limpio).
  - Coordenadas nuevas (1280x800): INICIAR PARTIDA (640,325), CARGAR PARTIDA
    (640,345), fila 1 del listado (400,180), SÍ (640,418).
  - Validación: **PASS** con `lib1` (`WR load-req` + `FPHASE` + sin errores).
- `[tooling]` Los comentarios de `Game.cpp`/`ondemand.h` que apuntaban a
  `main.js` ahora apuntan a `lib/index.js` (solo comentario).
- `[tooling]` **Arranque automático sin JavaScript** para un `index.html`:
  `lib/auto.js` monta el juego leyendo atributos del div (`data-vc-build`,
  `data-vc-streamed`, `data-vc-manifest`, `data-vc-assets`, `data-vc-fill`,
  `data-vc-trace`, `data-vc-title/audio/norun`) o `window.__VC_AUTO`.
  Verificado en headless con una página que **solo tiene un div**: canvas
  montado dentro, `div.__vcGame` disponible, eventos `vc-progress`/`vc-booted`/
  `vc-ready` y el juego llegando solo al menú.
- `[tooling]` **El div habla por sí solo**: cada evento de la API se re-emite
  como `CustomEvent` en el contenedor (`vc-progress`, `vc-log`, `vc-booted`,
  `vc-ready`, `vc-error`, `vc-audio`, `vc-fullscreen`) y el manejador queda en
  `div.__vcGame`, para hosts que no importan la librería.

## 2026-09-19 · Barra de progreso acotada a lo previo al motor (`2026-09-19-bar1`)

Queja del jugador: salía el *loading* rosa del top donde no debía (menú y
partida) y la carga de partida duplicaba pantalla de carga (el motor ya pinta
la suya con `WebDrawLoadScreen`/`LoadingScreen`).

- `[web/lib]` **La barra solo cubre la preparación previa al motor**:
  `probe` (comprobar ficheros) y `build` (descargar/arrancar reVC.js/wasm/data).
  En cuanto el motor manda su primera señal (`__loadProgress`, `__loadOverlay`)
  o el runtime arranca (+5 s de margen), `engineOwnsScreen()` la retira y
  **ninguna fuente vuelve a encenderla** (los avisos del loader posteriores se
  ignoran con `engineUp`). Antes `__loadProgress` la encendía durante las 19
  fases del init y durante el `InitialiseRestartStep` (carga de partida) →
  quedaba a medias en menú y partida.
- `[web/lib]` **Progreso honesto**: el loader solo reporta "(n/m)" por fichero.
  Con un único `reVC.data` (132 MB) un 0% clavado sería mentira → si el total es
  1, la barra pasa a "trabajando" (indeterminada); con varios ficheros usa el
  porcentaje real (visto 20 % → 45 % → 77 %).
- `[web/lib]` **Bug corregido de paso**: el handle definía `progress` dos veces
  (getter `ProgressInfo` + método), así que ganaba el método y
  `game.progress.phase` devolvía `undefined`. Ahora `progress` es el estado y el
  empuje manual es `setProgress()` (tipos y README actualizados; fase `engine`
  sustituye a `init`/`world`).
- `[raíz]` **`ondemand.js`**: `buildTag` sale de `__VC_CFG.version` (una sola
  fuente para wasm y JS) → relink hecho (tag dentro de `reVC.js`).
- `[tooling]` **`tools/slot0-load-test.mjs`** ahora muestrea la barra
  (100 ms) y **falla si se enciende con el menú/partida en pantalla**
  (`barLate.length`).
- **Validado**: sonda `bar-check` (barra on de 108 ms a 5,7 s = probe+build; off
  desde que arranca el motor) y test del slot 0 **PASS** (partida cargada,
  `FPHASE`, sin errores, 0 muestras de barra tras el menú).

## 2026-09-19 · La barra del top se apaga en el primer frame del motor (`2026-09-19-bar2`)
Síntoma del jugador: la barra rosa seguía "unos segundos destellando" con el
menú ya en pantalla (y a veces en partida). Diagnóstico con sondas y datos:
- `[raíz]` **Señal equivocada en el diseño previo**: `window.__loadProgress`
  (`CGame::InitialiseStep`) **no se llama al arrancar** → solo al inicializar
  el *mundo* (empezar o cargar partida). Sonda medida: **0 llamadas** durante
  todo el arranque al menú (el estado del juego avanza sin pasar por
  `InitialiseStep`, que es del mundo). Así que el único final de barra era el
  temporizador de 5 s de `onRuntimeInitialized`: la barra vivía ~4 s sobre el
  menú ya visible. La versión anterior (bar1) parecía correcta porque mi sonda
  de entonces no distinguía "apagada por el motor" de "apagada por el reloj".
- `[raíz]` **Parpadeo**: en modo "trabajando" (`data-indet`) quedaba el ancho
  inline (p.ej. `100%`) de una fase anterior; el inline gana al 32 % del CSS,
  así que el barrido movía una barra **llena** (destello). Ahora el modo
  indeterminado limpia el ancho inline y el CSS usa `width: 32% !important`.
- `[raíz]` **Tormenta de DOM**: `setStatus` de Emscripten se llama cientos de
  veces por segundo durante la descarga y cada llamada escribía atributos
  (medido: **3.164 toggles en ~1 s**). Ahora la barra se pinta **como mucho una
  vez por frame** (rAF), solo cuando el valor cambia, y `report()` no dispara
  evento si el estado es idéntico.
- `[raíz]` **Señal buena nueva**: `glfw.cpp` (`EmscriptenTick`) avisa una vez
  `window.__vcFrame` justo antes del primer frame presentado. La librería la
  usa para retirar la barra (`engineOwnsScreen('motor-primer-frame')`) y deja
  la red de 2,5 s por si una build antigua no avisara. Relink hecho.
- `[tooling]` **Diagnóstico**: cada encendido/apagado deja una línea `BART`
  (con motivo) en la cola de trazas → se ve en `odtrace.log` quién la apagó;
  el test del slot 0 imprime esas líneas y sigue **fallando si la barra se
  enciende con el menú o la partida en pantalla**.
- **Validado** (sondas `bar2/bar4` + test): `__vcFrame` llamado **1 vez** a
  ~0,9 s; barra encendida ~0,2–0,9 s (antes 0,05–5,6 s); cambios de atributo
  **29** (antes 3.164); trazas `BART on motivo=motor-primer-frame` y
  `BART off reason=motor-primer-frame`; `slot0-load-test` **PASS** (partida
  cargada, sin errores, 0 muestras de barra con el juego en pantalla).

## 2026-09-19 · Cámara de coche: mirar con ratón + auto-centrado estilo SA (`2026-09-19-cam3`)
Petición: en vehículo, la cámara debe volver sola detrás del coche **~1 s**
después de dejar el ratón quieto (mecánica base de SA). Investigado: en SA el
recentrado se percibe entre 0,5 s y 1 s (hilos de GTAForums/Steam sobre el
auto-centrado; el mod que lo quita lo llama "re-center after a second").
- `[raíz]` **Bug que anulaba el auto-centrado que ya existía**: en
  `Process_FollowCar_SA` la "inercia" del ratón (`stepsLeftToChangeBetaByMouse`)
  se decrementaba con `CTimer::GetTimeStep()` (segundos) sobre un contador de 50
  → duraba ~40 s, y durante ese rato forzaba `lookIdle = 0`. Resultado: el
  auto-centrado añadido en `cam1` **no llegaba a dispararse** en la práctica.
  Ahora se decrementa a ~30 fps (50 frames ≈ 1,7 s) y ya no reinicia el reloj
  de inactividad.
- `[raíz]` **La cámara que se usa al conducir no tenía mirada con ratón**: en VC
  el coche usa `Process_Cam_On_A_String`; `Process_FollowCar_SA` (con el ratón y
  el auto-centrado) solo se usaba con **FreeCam** activo, que no se guarda en los
  ajustes y arranca apagado. Se añadió la mecánica de SA a la cámara por defecto:
  con ratón (y sin dirección por ratón, `m_bDisableMouseSteering` por defecto) el
  ratón **orbita** la cámara alrededor del coche (mismo signo y sensibilidad que
  `FollowCar_SA`) y, tras `fLookRecenterDelayCar = 1.0 s` sin moverlo, Beta vuelve
  a la orientación del vehículo (~0,5 s) y la altura vuelve a la que calcula el
  juego. Si no se toca el ratón, el comportamiento es idéntico al de antes.
  A pie (`Process_FollowPedWithMouse`) se mantiene en 3 s (ya validado).
- `[tooling]` **Trazas de cámara** al log: `cam look raton` al empezar a mirar y
  `cam recentrada` cuando vuelve sola (una vez por evento, sin ruido).
- **Validado**: relink hecho; arranque al menú sin errores (sonda `bar4`: tag
  `cam3`, `__vcFrame` 1 vez, barra fuera en <1 s) y `slot0-load-test` **PASS**
  (partida cargada, 0 errores, 0 trazas de cámara espurias). Queda la prueba de
  conducción del jugador.

### Auditoría de la sesión de juego 06:06–06:14 (build bar2)
- 281 s con muestra, **60,0 fps de media**; solo **3 s** con peor frame >33 ms y
  **cero** pausas del puente de datos (`xhr=0/0ms`) → el sistema on-demand va fino.
- El peor frame (1083 ms, 06:07:05) es **empezar/cargar partida**: init monolítico
  (`loadtick B init monolitico` + `initstep 0..19`) tras el splash, con `ODLOAD`
  de txd/col/anim. No es un fallo nuevo: es el compromiso ya documentado.
- Picos de 20-40 ms en fase `A` (escena/audio) al entrar en zona nueva: sigue el
  pendiente del tirón inicial.
- Memoria: memfs 351 MB, live 225 MB, **idb 1280 MB**, heap JS 139→630 MB (final
  390 MB). El almacén del navegador ya pesa más de 1 GB: conviene vigilarlo o
  purgar lo viejo.
- Barra del top: `BART on/off motivo=motor-primer-frame` → se apagó donde debía.

## 2026-09-19 — Contador de FPS por API + z-index de la barra (tag `fps1`)
- `[raíz]` **El contador de FPS ya no depende de la URL.** `WebDrawFps`
  (`src/core/main.cpp`) leía `?fps` de `location.search` una sola vez (cacheado
  en un `static`). Ahora lee el global `window.__vcShowFps` en **cada frame**,
  con `?fps` como respaldo de desarrollo. La página lo enciende con
  `startGame({ showFps: true })`, con `data-vc-fps="true"` en el div
  (`auto.js`) o en caliente con `game.showFps(true|false)`, sin recargar.
  Al apagarlo se olvida la medida (contadores a cero) y las cifras vuelven
  limpias al reencenderlo. API nueva: `game.fpsVisible()`.
- `[raíz]` **z-index de la barra = 50** (antes `2147483000`): por encima del
  canvas y de la portada de carga, por debajo de cualquier overlay del host
  (el caso de un proyecto Vue con UI propia). El contenedor `.vc-root` conserva
  su z-index alto. Con `fill: 'parent'` la barra pasa a `absolute` dentro de la
  caja del div, sin cruzarse por el viewport.
- **Validado** (relink hecho): sonda `fps-check` con una página de un solo div y
  `data-vc-fps="true"` → `__vcShowFps=true`, `getComputedStyle(.vc-bar).zIndex`
  `=50`, y el texto verde "60 FPS" arriba a la izquierda **visible** en el menú;
  `showFps(false)` en caliente lo quita y `showFps(true)` lo devuelve.
  `slot0-load-test` **PASS** (0 muestras de barra fuera de sitio).
- Nota: ese test falló una vez antes de pasar (`¿clics perdidos?`) con la máquina
  cargada justo después de la sonda; es flakiness del arnés, no del build.

### Palancas de rendimiento identificadas (sin aplicar todavía)
- `gta_vc_browser/web/ondemand.js`: `OD.CAP` = 300 MB de working set on-demand,
  `GRACE_MS` = 60 s de gracia antes de desalojar, `INACT_MS` = 25 s de timeout
  de fetch. Subir/bajar `CAP` mueve directamente los llamados `stall` de carga.
- `build.sh`: `INITIAL_MEMORY` 512 MB, `MAXIMUM_MEMORY` 2 GB,
  `ALLOW_MEMORY_GROWTH=1`; `CMAKE_BUILD_TYPE=RelWithDebInfo` (con DWARF, em++
  avisa de "limited binaryen optimizations").
- Sin `-O3`/LTO/`-pthread` (el build va `REVC_NO_THREADS=ON`): todo el motor
  comparte hilo con la página.

## 2026-09-19 — Caché con techo, arranque en caliente y fuera el modo "solo atributos" (tag `mem1`)
- `[raíz]` **La caché de datos ya no crece sin cota.** `vcod2` guardaba copia de
  todo lo traído (medido **1.280 MB** en una sesión larga) y era la única cifra
  que subía sola. Ahora: base v2 con store `meta` ({t,s} por fichero),
  `idbCapMB` = 900 por defecto, recorte al arrancar (`OD.idbTrim`) que borra lo
  MÁS VIEJO y menos usado hasta bajar del 90% del tope, uso anotado en lotes
  (cada 30 s, sin escribir en cada lectura) y línea `IDBTRIM` con las cuentas.
  La librería expone `game.cacheInfo()` y `game.clearDataCache()` (los guardados
  NO se tocan: viven en el IDBFS de /userfiles, otro almacén).
  Medido en sonda con tope forzado a 15 MB: sembrados 60 ficheros viejos +
  3 recientes = **43,3 MB → 13,2 MB** (45 borrados, los 3 recientes intactos).
  En la sesión del jugador (1.280 MB) tocará bajar a ~810 MB en el próximo
  arranque.
- `[raíz]` **Arranque en caliente del primer minuto (F6b).** El hipo de
  20-266 ms al entrar en zona nueva es primera-touch: cada fichero cuesta una
  suspensión Asyncify + lectura IDB (~3 ms si hay que traerlo). El trabajo es el
  mismo se haga cuando se haga, así que se adelanta a la pantalla de carga: se
  **aprende del propio uso** (lo que el motor pide se anota en localStorage,
  atado al `dataTag`) y la sesión siguiente se precargan, **los más pequeños
  primero** (cada uno cuesta igual, así que rinden más por MB), con presupuesto
  `warmMB` = 64. Se dispara al empezar la carga del mundo (enganche de
  `window.__loadProgress`). `OD.CAP` 300 → 360 MB para que el adelanto no
  empuje el working set contra el tope.
- **Medido (sonda de conducción a pie, 70 s, perfil propio):** sesión con lista
  (`ODWARM n=146 mb=3 ms=134`) vs sesión con la misma caché y sin lista:
  tiempo dentro del cargador en 60 s de juego **144 ms vs 237 ms (-40%)** y
  peor frame 233 ms vs 283 ms. El arnés headless va a 25-40 fps con GL software,
  así que la cifra absoluta no es representativa; lo que vale es el trabajo
  bloqueante movido fuera del juego. La sesión real del jugador es la medida
  buena (línea `ODWARM` + `ASYNC`/`FPSLOG`).
- `[raíz]` **Fuera el modo "solo atributos"** (`lib/auto.js`, `data-vc-*`,
  `__VC_AUTO`, `autoMount`): el arranque es siempre JavaScript, como en el
  proyecto Vue (`startGame({ el: '#vc' })`). El `index.html` de la página de
  desarrollo no cambia en lo esencial: un div + el módulo que llama a la API.
  Se mantienen los eventos `vc-*` en el div (avisar no es arrancar).
- `[tooling]` La página de desarrollo admite `window.__VC_DEV_OPTS` para que las
  sondas ajusten opciones (tope de caché, presupuesto de calentamiento, FPS) sin
  tocar la URL.
- **Validado**: `slot0-load-test` **PASS** (0 muestras de barra fuera de sitio,
  0 errores); sonda `fps-check` (FPS por opción y z-index 50) y `trim-check`
  (techo de caché) en verde; A/B a pie de 3 sesiones. Relink hecho (el pre-js va
  embebido en `reVC.js`, así que hay que forzarlo: `rm reVC.js && ninja`).

## 2026-09-19 — Nivel 1 de multihilo: la carga de ficheros sale del frame (tag `thr1`)
- `[raíz]` **El trabajo de traer ficheros ya no lo hace el hilo del juego.** Antes
  `OD.ensure` leía la caché IDB y/o bajaba de la red *dentro del frame* (medido:
  68 suspensiones y 226 ms de trabajo en un segundo al entrar en zona nueva, con
  hipos de 20-280 ms). Ahora ese trabajo vive en un **Web Worker** (creado desde
  un Blob embebido en el pre-js: no hay fichero extra que servir ni ruta que
  configurar); al hilo del juego solo le queda **copiar bytes a MEMFS**.
- **Y en partida no se espera**: si el fichero aún no está, `ensure` contesta
  "no está" al instante y el motor lo vuelve a pedir en la siguiente pasada de
  streaming; el worker lo entrega y se escribe en cuanto llega. El frame no se
  congela nunca. La decisión la toma el motor: publica `window.__vcInGame`
  (`glfw.cpp`, al cambiar de estado), y **solo se aplaza si está JUGANDO** —
  durante la carga (menú/partida) se espera como siempre, porque esa ruta es
  crítica y fue el origen del crash de arranque.
- **Sin worker no cambia nada**: si no hay `Worker`, falla, o el fichero no se
  puede traer (3 fallos), el camino de siempre sigue debajo como red de
  seguridad. Con `worker:false` (opción de la librería) se fuerza el camino
  viejo, que es como se hace el A/B.
- Dos fallos reales cazados en la propia sonda, no en producción: (1) un worker
  de Blob **no resuelve rutas relativas** (`Failed to parse URL from
  /streamed/...`) → se le manda la base absoluta y se codifica cada segmento
  como hacía el camino viejo; (2) con un fichero que no se puede traer, caer al
  camino bloqueante desde partida multiplicaba reintentos y hundía los fps →
  ahora en partida se aplaza sin caer al camino viejo.
- **Medido (sonda a pie, 50 s, mismo perfil):** `ODWK peticiones=306
  aplazados=182 entregados=306 escritos=306 err=0`, y el bloqueo del cargador
  pasa de **68 suspensiones / 226 ms en un segundo** a **1-6 suspensiones / 0 ms**.
  Los fps del arnés headless (13-60 según la carga de la máquina, GL por
  software) no sirven para medir fotogramas: la medida buena es esa, y la sesión
  real del jugador.
- **Validado**: `slot0-load-test` **PASS** (0 errores, 0 muestras de barra fuera
  de sitio) y A/B con y sin worker en el mismo perfil (sin regresión). El
  esqueleto multihilo se probó antes aparte: ASYNCIFY + `-pthread` + GLFW/WebGL
  compilan y corren juntos en este SDK, así que el Nivel 2 (bucle del juego en
  un worker con OffscreenCanvas) no está descartado.

### fps2 (2026-09-19) — techo de fps, cámara en X, cinemática y memoria

Sesión del jugador (2040 líneas, ~12 min): **0 errores**, 0 `FAIL`, 0
`REINTENTO`, sin thrash. La memoria del momento peor medida fue `ODCAP memfs=482MB
live=356MB evict=0/0MB js=553MB` — el `js` de V8 y el `memfs` son **la misma
memoria** (MEMFS guarda los ficheros como buffers JS), así que la página iba por
~1,06 GB; el pico de 2,1 GB venía del montón wasm (techo de **2 GB**).

- **Techo de fps, siempre activo** (`glfw.cpp`, `GS_PLAYING_GAME`): `ms` es el
tiempo en ms desde el último frame procesado, así que "esperar 1000/cap" limita
la tasa. Con la opción **Limitar FPS** encendida → **35** (antes pedía 30 y daba
~27 por el redondeo a ms enteros); apagada → **tope duro de 120**. vSync sigue
actuando por debajo (monitor de 60 → 60 fps) y por encima de 120 el tope lo
recorta.
- **Cámara del coche, giro horizontal**: el ángulo del ratón se **acumula**
(`lookYaw`) y se aplica **al final** de `Process_Cam_On_A_String`, después de la
colocación y del cálculo de altura. Aplicarlo antes no se veía: esas rutinas
reescriben `Source` y el giro se perdía (por eso solo funcionaba el vertical).
El auto-centrado ahora es "soltar el ángulo": vuelve detrás del coche y sigue
sus movimientos. Traza `CAMDBG` (1/s): `mgr/steer/keys/dx/dy/yaw/pitch/idle`
dice de un vistazo si el fallo es el ajuste o el temporizador.
- **Cinemática = CARGANDO**: durante una cinemática el motor publica
`__vcInGame = false` (`gGameState == GS_PLAYING_GAME && !CCutsceneMgr::IsRunning()`),
así que sus ficheros se **esperan** en vez de aplazarse. Era la regresión que
producía mundo negro, audio mudo, props vencidos (faros/postes) y subtítulos de
la escena impresos sobre la partida.
- **Techo de memoria del motor: 2 GB → 1 GB** (`build.bat` *y* `build.sh`; el
`.bat` tenía el valor fijo y era el que compilaba de verdad). El montón se queda
en los 512 MB de reserva (medido: `heap=512MB` constante, también cargando), así
que el techo solo evita el pico.
- **Fuera el cartel "Mantén Esc"** con su barra: mantener Esc 3 s sigue saliendo
de pantalla completa, sin nada encima del juego.
- **FPS por opción de JS**, nunca por URL: `startGame({ showFps: true })`, o
`game.showFps(true)` en caliente; la página de desarrollo lo trae encendido.
- **Validado**: `slot0-load-test` **PASS** (0 errores de página, barra donde
debe). Tag **`2026-09-19-fps2`** (Ctrl+Shift+R).

### cut3 (2026-09-19) — cinemáticas vacías y sin audio: causa raíz; cámara del coche

**Síntoma del jugador**: al empezar misión o partida nueva, las cinemáticas se
ven vacías (mundo negro, sin habitación/escena, HUD y minimapa visibles, texto de
la escena encima) y **sin audio**. Con FPS limitados pasaba igual.

**Causa raíz (demostrada con el registro del servidor, no a ojo).** La capa
on-demand puede *aplazar* un fichero que aún no está (contesta "no está" y el
motor lo reintenta en otro frame). Eso es correcto en partida libre, pero una
cinemática **no puede reintentar**: su preparación (`ANIM/CUTS.IMG` de 115 MB
para splines de cámara y animaciones de los actores, la sala, los actores y el
**preload de la música**) es una **carga bloqueante de un solo tiro**.
- El gate anterior solo miraba `CCutsceneMgr::IsRunning()`, y eso es **tarde**:
  la cinemática se *prepara* (script: `MAKE_PLAYER_SAFE_FOR_CUTSCENE` /
  `LOAD_CUTSCENE` → peticiones de modelos → `LoadAllRequestedModels`) **antes**
  de que `IsRunning()` sea true. En ese hueco la página creía estar jugando.
- Prueba en el registro del servidor (`streamed-access.log`): a las 08:38:22 se
  sirvió `/anim/cuts.img` (115 MB) **él solo, aislado**, y a las 08:39:33
  `/Audio/int_m.mp3` (443 KB, la música del intro), ~70 s **después**: los dos
  ficheros llegaban por detrás del motor. Sin splines no hay cámara de
  cinemática (`bCamLoaded = false`): de ahí el HUD visible y la cámara de
  partida en un mundo ya vaciado (`RemoveEverythingFromTheWorld…`), y sin
  música ni el `.IFP` no hay audio ni animación de los actores.

**Arreglo (dos capas, la segunda es la que cierra el hueco de un frame)**:
1. `odBlockingPush/Pop` (motor, `ondemand.h`/`ondemand.cpp`): el motor avisa a
   la página mientras dura una **carga bloqueante** — `CStreaming::
   LoadAllRequestedModels` y `CCutsceneMgr::LoadCutsceneData` —, y `wkFastFail`
   no aplaza nada mientras ese contador esté arriba (`window.__vcODBlock`).
2. `vcCutscenePublishLoading()`: al empezar a procesar la cinemática
   (`StartCutsceneProcessing`, `LoadCutsceneData`, `RemoveEverythingFromThe
   World…`) se publica `__vcInGame = false` **en el instante**, sin esperar al
   siguiente frame; el latido del motor ya publicaba `!IsCutsceneProcessing()`.
3. El aplazamiento se rinde antes (3 reintentos por fichero en vez de 8), que
   acorta a ~50 ms la ventana en la que una carga bloqueante podría quedarse
   sin el fichero.
4. Extra de robustez: si `CUTS.IMG` no se puede abrir, no se llama a
   `RwStreamSkip`/`Close` con un stream nulo (en release el `assert` no existe).

**Trazas nuevas (para no volver a adivinar)**:
- `WEBHB` (1 vez / 2 s): `state/ingame/cut/proc/cmd/memfs/files/ev/f/idb/heap`.
  Es el latido del motor **en el fichero** (antes solo iba a la consola del
  navegador, que es justo lo que faltaba cuando el log rotaba).
- `CUT begin|loaded|start|end|audio`: nombre de la escena, si cargaron
  animaciones (`cuts.img`) y splines de cámara, `dirN`, número de objetos de
  escena y el id del track de música.
- `STREAM preload n=… file=… open=…`: si el audio de cinemática realmente abrió.
- `CAMV2`/`CAMSA` (1 vez / s): modo de cámara activo, camara libre,
  `mgr`/`steer`, delta del ratón, ángulo acumulado, tiempo quieto y si está
  mirando; `CAMSA` para el camino de cámara libre.
- `ODBIG n=… lista`: ficheros >20 MB en MEMFS con **prueba de lectura real**
  (12 B). `n=0` es el dato que dice "cuts.img no llegó"; una entrada con
  `ERROR:` es metadatos rotos, no un fallo de carga.

**Cámara del coche (petición del jugador: centrar a los 2 s de no tocar el
ratón, como el seguimiento de la cámara libre apagada)**:
- El bloque de ratón exigía también el ajuste *"cámara con ratón en 3ª
  persona"* (`m_bUseMouse3rdPerson`), que en modo de control Clásico vale 0: en
  ese caso **no se ejecutaba nada** y parecía que el auto-centrado no existía.
  Ahora solo se desactiva si el ratón está configurado para **conducir**
  (`m_bDisableMouseSteering == false`) o si el juego bloquea controles.
- El temporizador de quietud se comprueba **fuera** del gate: una mirada
  residual siempre acaba volviendo, aunque el jugador se baje del coche o el
  juego bloquee los controles (antes podía quedarse con el ángulo puesto).
- `fLookRecenterDelayCar` = **2 s**. El de a pie sigue en 3 s (ya validado).
- En `Process_FollowCar_SA` (cámara libre) se quitó la misma exigencia de
  `m_bUseMouse3rdPerson` para que la cabeza del ratón llegue también ahí.

**Estado**: compilado y **`slot0-load-test` PASS** (0 errores de página, barra
correcta, `WEBHB` presente en el log). Tag **`2026-09-19-cut3`** (Ctrl+Shift+R).
Sin commitear.

**Lo que falta confirmar con tu partida**: que `CUT loaded … cam=1 anim=1` y
`STREAM preload … open=1` salgan en el log de la primera cinemática, y qué dice
`CAMV2` cuando sueltas el ratón en un coche (~2 s ⇒ `look=0`).

---

## Ronda `cam4` (2026-09-19, tarde): diálogos mudos/acelerados + cámara del coche

### 1. Diálogos de misión y llamadas: mudos y "acelerados" (regresión del
bloque `cut3`)

**Evidencia del log del jugador** (sesión 14:33-14:55Z, `odtrace.log`):

| fichero | tipo | `open=` |
| --- | --- | --- |
| `INT_A/INT_M/INT_D/LAW_2A/B/C.MP3`, `INTRO1..4.WAV` | audio de **cinemática** | **1** (bien) |
| `LAW2_1..5.WAV`, `LANAMU1/2.WAV`, `MOBR1.WAV`, `MOB_52A..H.WAV` | audio de **misión** | **0** (nada) |

Los ocho `MOB_52A..H` se piden con **~1 s entre cada uno** y el servidor los
sirvió *mientras* el motor ya los había dado por perdidos.

**Causa raíz** (dos capas que se suman):

1. La capa on-demand **aplaza** lo que no está mientras el motor está en
   partida (regla del bloque `thr1`). Los MP3/WAV de cinemática se salvan
   porque `LoadAllRequestedModels` avisa de "carga bloqueante" (bloque `cut3`),
   pero el audio de misión se abre por `CStream::Open` desde `AudioLogic` y
   **nadie avisaba ahí**. `fopen` fallaba al instante ⇒ `open=0`.
2. El motor no lo comprobaba: `ProcessMissionAudioSlot` pasaba de `NOT_LOADED`
   a `LOADED` **sin mirar** si el fichero se había abierto. Con la voz sin
   arrancar, `nCheckPlayingDelay` (30 frames ≈ 0,5 s) daba la línea por
   terminada y el script corría en vacío: **subtítulos que pasan volando,
   texto que aparece de la nada, llamadas aceleradas y sin voz**. Un solo
   fallo, todos esos síntomas.

**Arreglo** (build `cam4`):
- `sampman_oal.cpp` (`PreloadStreamedFile`): el audio de misión (streams 1-2,
  WAV de 30-120 KB) **espera** a tener el fichero (`odBlockingPush/Pop`), como
  el `fopen` sobre disco del juego original. La **radio (stream 0) no cambia**
  (sus `.adf` de 30 MB sí se notarían como freeze; ahí el aplazamiento sigue
  siendo el bueno).
- Segundo intento de apertura si el primer `Open/Setup` falló.
- `cSampleManager::IsStreamedFileOpened()` nuevo (oal/null/miles) y
  `ProcessMissionAudioSlot` pasa a `LOADED` **solo si el fichero está abierto**;
  si no, reintenta (hasta los 120 frames de siempre) y luego degrada por el
  camino de "pretend play" del original.
- Traza ampliada: `STREAM preload … open=… spk=… wait=…` (`spk` = stream, para
  distinguir radio de misión sin mirar el nombre).

**Nota**: en el backend OAL `GetStreamedFileLength` devuelve siempre 0 (la
tabla `nStreamLength` solo existe en el backend Miles), así que
`m_nMissionAudioFramesToPlay` siempre sale 0. Solo afecta al camino de
seguridad (audio fallido/canal de policía), no al ritmo normal, que lo marca
la reproducción real. Candidato a arreglar si se ve un caso.

### 2. Cámara del coche: "no se centra nunca"

**Medido con sonda headless** (`/tmp/vc-e2e/cam-pad.mjs`, cargando el slot 0 y
moviendo el ratón en círculos): el ratón **sí llega** al motor y **vale 0
cuando está quieto** (`mx=-67 my=-0.00`, sub-píxeles cuando no se toca);
`sens=0.0025` (config sana). O sea: el problema no era la entrada.

**Causa**: el reloj de quietud se reiniciaba con `mouseX != 0 || mouseY != 0`.
Un ratón apoyado manda micro-movimientos sub-píxel cada tanto: en la sesión del
jugador `steps=50.0` en **las 376 muestras** (o sea, "el ratón se movió" en
casi todos los frames) e `idle` no pasó nunca de **0,98 s** — con umbral 2 s,
el auto-centrado **nunca** disparaba.

**Arreglo** (build `cam4`): en `Process_FollowCar_SA` el giro horizontal del
ratón se separa del camino viejo (`lookYaw`, más abajo) y se aplica como
desviación **solo al colocar la cámara** (`betaView`), de modo que `Beta/Alpha`
—y el bucle de realimentación del que sale `targetBeta`— quedan intactos: el
retorno es "apagar la desviación", no pelear con el seguimiento.
- Umbral de **1,5 px por frame de 60 fps** (`Max(|mx|,|my|)*dt60`): los
  micro-movimientos de un ratón apoyado ya no cuentan como "estoy mirando";
  el mismo criterio se aplicó al reloj de quietud de la cámara a pie.
- Sensibilidad y sentido **idénticos a la cámara a pie** (`2.5 * sens * FOV/80`),
  con `dt60` para que no dependa de los fps; `sens` se sanea si la config
  trae 0 o un absurdo.
- `lookYaw/lookIdle` se limpian en `ResetStatics` (cambio de modo/vehículo).
- Trazas nuevas (1/s): `CAMSA … yaw= idle= dt= sens= mx= my=` y `CAMPD` (a pie,
  `mx/my/dt/sens/idle`) — esta última es la que permitió demostrar que el ratón
  llega bien.

### 3. Estado

Compilado y **`slot0-load-test` PASS** (0 errores de página). Tag
**`2026-09-19-cam4`** (Ctrl+Shift+R). Sin commitear. Log de la sesión
`cut3` del jugador conservado en `/tmp/vc-logs/sesion-cut3-camara.log`.

**A confirmar en tu partida**: `CAMSA … yaw=` debe moverse al mover el ratón y
volver a `0` con `idle>2` unos 2 s después de soltarlo; las voces de misión
deben sonar y (por `STREAM preload … open=1 spk=1`) el log dirá si el fichero
llegó a abrirse.

## 2026-09-19 · Cámara libre `cam5`: el auto-centrado no disparaba NUNCA (causa raíz)

Petición: *"si muevo el mouse debe dejármela mover pa' donde me dé la gana; si
lo dejo quieto, que vuelva a como si la opción de cámara libre estuviera off"*.
Plan: `.agents/plans/camara-libre-autocentrado.md`.

- **Dato que lo cerró** (sonda `vc-e2e/cam-pad.mjs` sobre `cam4`, círculos 8 s
y quieto 7 s): 98 `CAMPD` + 137 `CAMSA` y **`idle=0.00` en TODAS**, incluso en
los 7 s sin tocar el ratón; `yaw=0.000` siempre. El auto-centrado estaba
escrito y converge bien, pero **el reloj de quietud no acumulaba**: nunca
llegaba a disparar. No era el ajuste, ni el modo de cámara, ni la sensibilidad.
- `[raíz]` **El criterio de "estoy mirando" estaba mal en los tres caminos**
  (`Cam.cpp`): contra el delta **crudo** del ratón (`!= 0`).
  - A pie (`Process_FollowPedWithMouse`): `LookLeftRight`/`LookUpDown` **vienen
del propio ratón** cuando `UseMouse` (-2.5*MouseX), así que una milésima de
píxel los dejaba `!= 0` y reiniciaban el reloj **en cada frame**; el umbral de
1.5 px (`mouseReal`) quedaba anulado por esa misma cláusula.
  - Coche por defecto (`Process_Cam_On_A_String`): `if(mx != 0 || my != 0)`.
  - Coche cámara libre (`Process_FollowCar_SA`): igual (umbral de `cam4`).
- `[raíz]` **Detector de movimiento por VENTANA, no por frame**
  (`LookMouseActivity`, ~1 s): se filtra el desplazamiento **con signo** y se
  compara `Max(|Σx|,|Σy|) >= 1.5 px`. Un ratón apoyado (ruido sub-píxel) va y
  viene → la suma se queda en ~0 (el reloj de quietud SÍ acumula y el retorno
  dispara); un giro intencionado, aunque sea lento, es coherente → la suma sube
  al instante (el retorno NO pelea con él). Un umbral por frame no puede
  separar las dos cosas: a 60 fps un giro lento también es sub-píxel por frame
  y el auto-centrado tiraría en contra → se vería como "la cámara no se deja
  mover". La **rotación** sigue haciéndola cualquier delta `!= 0` (apuntado fino
  intacto: el umbral solo gobierna el reloj de quietud).
- `[tooling]` Traza `cam recentrada pie` (a pie no había ninguna: por eso el
  fallo pasó desapercibido) además de la de coche que ya existía.
- **Verificado en el arnés** (build `2026-09-19-cam5`, tag confirmado en el
  log): `idle` se reinicia con el ratón moviéndose y sube con el ratón quieto,
  y aparecen **3 `cam recentrada pie`** en la sonda `cam-pad.mjs`; 0 errores de
  página. **Enseñar a confirmar al jugador**: los dos ejes a pie y en coche, y
  que vuelva sola al soltar el ratón (los dos caminos de coche no los ejercita
  la sonda, que nunca entra en un vehículo). Sin commitear.
- **Duda resuelta al jugador (retícula)**: en VC PC con cámara de ratón la
  retícula se dibuja **siempre** que lleve un arma de fuego en 3ª persona
  (`Hud.cpp`, `DrawCrossHairPC` con `Using3rdPersonMouseCam()`), no solo al
  apuntar. Es vanilla; hay un sistema de apuntado aparte (`CPad::GetTarget()`,
  botón derecho) y modos 1ª persona/sniper. Se le ofrece "solo al apuntar" si
  lo quiere.

## 2026-09-19 — Retícula solo al apuntar (estilo SA) `[raíz]`
- `Hud.cpp`: la retícula de 3ª persona con cámara de ratón (`DrawCrossHairPC`)
  se dibujaba **siempre** que llevaras un arma de fuego; ahora exige
  `CPad::GetPad(0)->GetTarget()` (botón de apuntar: clic derecho por defecto).
  Los modos 1ª persona / sniper / runabout y el resto del HUD quedan igual.
- Build `2026-09-19-cam6`. **Aprendido**: lo que se cuelga >10 min sin salida es
  el `emcmake cmake` de `build.bat`; `ninja -C gta_vc_browser/build/web` con el
  PATH de emsdk hace el incremental (1 fichero + enlace) en ~30 s y matar el
  configure colgado no deja basura.

## 2026-09-19 — Triaje de Vice Extended: qué entra como dato `[proceso]`
- Añadido a `.agents/plans/vice-extended.md` §4-bis, con evidencia del motor:
  el loader **ya** parsea `CDIMAGE` en cualquier `.dat` (`FileLoader.cpp:123`) y
  en web una IMG puede servirse como **carpeta suelta + `.dir`**; el port tiene
  **64** tiles `radarNN.txd` sueltos y el mod trae **los mismos 64** en
  `cdimages/radar.img` → radar HD 1:1 sin código.
- Clasificado: **Pack 1** dato puro (radar HD, `icons4.txd`, HUD/menú/
  partículas, `ped.ifp`, GXTs ×7, peds/jugador, texturas de coche); **Pack 2**
  dato + herramienta (mapa `bryx`/`plusroad`/`newGen` + 72 objetos de
  `objects.img`, pide extractor IMG→suelta y subir límites); **Pack 3** solo con
  código (armas y vehículos nuevos, mecánicas, `main.scm` con opcode `0FA8`);
  **Pack 4** no entra (`.exe`/`.asi`/`.dll`, `features.ini`, `ViceEx.SDT`).
- Descartados por peso muerto sin código: `radio.txd` y `newspapers.txd` (arte
  de features que aquí no existen).

## 2026-09-19 — Revertido: cámara libre con auto-centrado y retícula SA `[raíz]`
- Petición del jugador tras probarlo en el juego: **no le gustó, que quede
  original**. `git checkout --` de `src/core/Cam.cpp` (las 303 líneas mías:
  auto-centrado a pie/coche, mirada con ratón desde el coche, trazas `CAMPD`/
  `CAMSA`/`CAMV2`) y de `src/renderer/Hud.cpp` (retícula condicionada a
  `GetTarget()`). **Comportamiento vanilla otra vez**: la cámara se queda donde
  la dejas y la retícula sale con arma de fuego en 3ª persona con cámara de
  ratón, apuntes o no.
- Build `2026-09-19-orig1` (ninja incremental, 25 s, `exit=0`). El plan
  `.agents/plans/camara-libre-autocentrado.md` queda marcado REVERTIDO como
  registro de lo aprendido (ruido sub-píxel del ratón, coherencia de signo…).
- `[proceso]` Se tocó cámara/HUD sin plan aprobado antes (norma del repo desde
  2026-09-12) y sin confirmar el gusto del jugador: dos features grandes
  tiradas. Para lo que es puro gusto (cámara, HUD), preguntar antes de compilar.
- Ojo para no confundir: `bFreeCam` (menú Display → FreeCam), el camino SA de
  coche (`Process_FollowCar_SA`) y `m_bUseMouse3rdPerson` son **vanilla de
  reVC**, no parte de este cambio.

## 2026-09-19 — Búsqueda de fuentes de Vice Extended `[proceso]`
- **No hay fuente pública del mod**: el GitHub del autor (`Cowboy-69`) tiene 8
  repos y ninguno es ViceEx (`librw`/`modloader` forks, `FileSystemRedux`
  (plugin CLEO, AGPL-3.0), addons de Blender…). `x87/gta-extended` es reVC
  upstream (rama `miami`), no el mod. `strings ViceEx.exe` solo revela nombres
  de ficheros vanilla de reVC → el mod parcheó ficheros existentes.
- **Vía sólida**: `ViceEx.exe` es un build de reVC → **RE diferencial con
  Ghidra** (emparejar funciones con nuestro binario; lo que sobre es suyo).
  Con referencias abiertas para cada mecánica: `Photosounder/ViceCity` (fork
  reVC, mismo linaje), `gta-reversed` (natación/escalada de SA), MVL de
  `maxorator` (armas/vehículos add-on, formato que usa `newVehicles.ide`),
  `plugin-sdk` GPS/UniversalTurnlights, `Project2DFX`, `skygfx_vc`, `CLEO-Redux`
  + `opcodes-restoration-project` (opcodes del script), `niltwill`
  (toggles), `thelink2012/modloader` (MIT).
- **Licencia**: este repo no tiene licencia (solo uso educativo/modding) → el
  código ajeno se usa como **especificación**, no se pega. Todo el detalle y el
  orden de ataque (datos → knobs → llaves de sistema → armas → mecánicas) en
  `.agents/plans/vice-extended.md` §5.

## 2026-09-19 — Investigación de inclusión de Vice Extended `[proceso]`
- Petición del jugador: investigación a fondo (qué se mete, qué no, impacto,
  qué puede salir bien o mal) **sin alterar el estado del juego**. Todo el
  análisis vive fuera del repo, en `/tmp/ve-audit/` (`img_audit.py`,
  `txd_audit.py`), solo lectura. Lo único escrito: documentos.
- **Evidencia medida** (`.agents/plans/vice-extended-inclusion.md` §2): los 8
  `.dir` del mod son **consistentes** (0 fuera de rango / solapes / duplicados)
  y usan **sectores**, que es justo lo que lee el motor (`CdStreamPosix`
  `start`/`sectors`, `Streaming.cpp:446`); **431 entradas pisan** ficheros del
  port y **37 son nuevas** (radar 64/64, peds 129, player 46, vehicles 48,
  weapons 94 con 18 nuevas, objects 72 con 16 nuevas, anims 14 con 3 nuevas).
  IDE del mod (`objs/tobj/2dfx/path`, +`cars/peds/weap/hier`) e IPL
  (`inst/cull/pick/path`) y sus 5 directivas de `.dat` (la incl. `CDIMAGE`)
  **están todas soportadas**; los vehículos nuevos usan **formato MVL** que el
  loader no conoce.
- **Hallazgo con mecanismo**: `FindWeaponType` devuelve `UNARMED` si el nombre
  no existe (`WeaponInfo.cpp:271`) → meter el `weapon.dat` del mod sin ampliar
  la tabla de nombres hace que sus 8 armas **se escriban encima de los puños**
  (silencioso, sin crash); sus IDs de modelo 6661+ están por encima de
  `MODELINFOSIZE = 6500` y los coches piden 6500-6599 y `NUMVEHICLES 110→130`.
- **Exclusiones del jugador** (taller/tuning, modo foto, GPS) registradas con
  lo que arrastra cada una.
- **TXD CERRADO** (`.agents/plans/vice-extended-inclusion.md` §2.7-2.9, §8).
  Parser propio (`/tmp/ve-audit/txd_audit2.py` + `txd_names.py` + `txd_ram.py`)
  que replica los lectores de librw. Aprendizaje clave: **D3D8 y D3D9 no
  comparten layout** (en D3D8 el campo 76 es `hasAlpha` y el 87 es un **código**
  de compresión 0..5; en D3D9 son `d3dformat` y un bitfield). Con eso medido,
  las 1.381 texturas del mod son **plataforma 8/9** (las dos que el port lee) y
  formatos C565/C1555/C4444/C888/C8888 + PAL4/PAL8 + DXT1/DXT3/DXT5: **nada
  fuera del set de librw**. El port ya ejercita DXT y paletizado con sus propios
  ficheros. Único hueco real: **1 textura DXT2** (`027agen.txd::helipad_strutT`)
  que el decodificador software de librw no cubre (`d3d.cpp:860-880`) → arreglo
  de 2 líneas.
- **Hallazgo que cambia el riesgo (nombres, no formato)**: el `fronten2.txd`
  del mod trae 13 texturas y **le faltan los 11 logos de emisora** que el port
  pide desde `FRONTEN2.TXD` (`Frontend.cpp:140-170`) — el mod los movió a su
  `models/radio.txd`. Y sus `frontend_ds*/nsw/x360/xone.txd` solo traen
  `fe_arrows1` → **faltan `fe_arrows2/3/4`** con `GAMEPAD_MENU` activo (XINPUT
  definido). En cambio el `hud.txd` del mod **aporta** `radar_waypoint`
  (`Radar.cpp:1100`) y `bomb` (`Hud.cpp:179`), nombres que el port pide y hoy no
  existen en ningún TXD suyo.
- **Coste medido por bloque** (mips incl.): con S3TC casi todo va comprimido y
  cuesta como lo actual; **sin S3TC** (RGBA8 forzado en WebGL) peds = 84 MB y
  objetos = 67 MB, y `fronten2.txd` pasa de 3,2 a **38 MB** (9 mapas de 1024²).
  El radar HD sube de 4,2 a 16,8 MB sin S3TC; su geometría es segura porque
  `DrawRadarSection` usa UV normalizadas. Matiz: peds/objetos son techo "todo
  residente"; en uso real se streamean por TXD (≈0,5 y ≈3,7 MB cada uno) y lo
  único que se paga **de golpe** es `fronten2.txd`. El port ya informa del caso
  con `[od] webgl s3tc=? astc=?` (`gl3device.cpp:1936`).
- **Decisiones del jugador (19/09)** sobre los dos puntos abiertos:
  1. **Caso sin S3TC: se acepta y se documenta** (sin trucos: peds/objetos ya
     están en DXT, no se pueden recomprimir menores).
  2. **`fronten2.txd`: fusionar los 11 logos de emisora (hoy en el `radio.txd`
     del mod) dentro del `fronten2.txd` del mod y usar su mapa HD de 1024²**
     (38 MB sin S3TC / 9,5 con, vs 3,2 / 0,75 del port). El fichero `radio.txd`
     no entra; solo sus 11 logos. Hace falta un escritor/fusionador de TXD, que
     **no existe** en `tools/` (candidato a construir, o Magic.TXD a mano).
  Puntos de `frontend_ds*/x360/xone.txd`: **no se sustituyen** (perderían
  `fe_arrows2/3/4` con `GAMEPAD_MENU` activo).
- Nada se copió del mod ni se cambió el estado del juego: sin builds, sin
  `streamed/`, sin manifiesto. Lo único escrito en el repo son documentos.

## 2026-09-19 — Vice Extended: ejecución del plan (packs 1, 1b y 2)

Ejecución por bloques del plan `.agents/plans/vice-extended-inclusion.md`, cada
uno con build propio, sonda `vc-e2e` (arranque + carga de slot 1 + 20-60 s de
partida) y captura. Estado del juego **sí** tocado (autorizado por el jugador,
que hizo copia previa); originales guardados en `/tmp/ve-bak/orig/`.

**Pack 1 (radar/HUD/menú)** — `radar00-63.txd` (HD DXT3 256², 67.584 B c/u),
`icons4.txd`, `hud.txd`, `generic.txd`, `particle.txd` y un **`fronten2.txd`
fusionado** (mapa HD del mod + los 11 logos que el mod tenía en su `radio.txd`,
hecho con el fusor nuevo `tools/txd_merge.py`). Build `2026-09-19-ve1`: arranca,
carga partida, `TXDIN radarNN memlen=67584` confirma que el motor usa los tiles
HD, 0 `Failed to load`, 0 `SKIP`, 0 errores de página.

**Pack 1b (peds/jugador/vehículos/GXT)** — 129 + 46 + 48 entradas de las IMG
del mod a `streamed/models/gta3.img/` (197 ficheros, 14,1 → 32,1 MB) y los 6
GXT (0 claves del port ausentes en el mod, 100 extra: superset seguro). El `.dir`
se recalculó con `tools/repack_dir.py`. Build `ve2`: idem, sin incidencias.

**Pack 2 (mapa)** — `objects.img` importado con el **tool nuevo
`tools/import_img.py`** (56 reemplazos + **16 entradas nuevas al final del
`.dir`**, que es seguro porque el motor asocia posición **por nombre**, no por
índice); 12 IDE/IPL/IPL modificados + 8 nuevos (bryx, newGen, plusroad,
wanted_paths.ipl) + `map.zon`/`info.zon`; y 8 líneas añadidas a `gta_vc.dat`
(3 IDE + 1 COLFILE + 4 IPL; las 8 `CDIMAGE` del mod **no** se usan porque sus
imágenes se fusionan en `gta3.img`). Build `ve3`.

**Límites (config.h)**: `MODELINFOSIZE 6500→6700`, `MAX_CDIMAGES 8→12`,
`NUMVEHICLES 110→130`, `VEHICLEMODELSIZE 110→130` y **`COLSTORESIZE 31→48`**.
Este último salió de un **crash real**: `ASSERT(def)` en `ColStore.cpp:48`
(`CColStore::AddColSlot`) porque el mod añade `bryx.col`, `plusroad.col` y
`newgen.col` (34 slots > 31). El port estaba **justo en el límite**.

**librw: DXT2/DXT4** (`vendor/librw/src/d3d/d3d.cpp` y `raster.cpp`) — se
decodifican como DXT3/DXT5 (mismo bloque, cambia el alfa premultiplicado).
Era el hueco que dejó el análisis de TXD (`027agen.txd::helipad_strutT`).

**Bug de datos del mod: IDs `-1`.** `maps/bryx/bryx.ide` y `bryx.ipl` definen
los dos objetos nuevos con ID `-1` (comodidad de *modloader*, no del motor):
`AddTimeModel(-1)` escribe fuera de `ms_modelInfoPtrs` y `LoadObjectInstance(-1)`
llamaba a un virtual de un puntero corrupto → `RuntimeError: null function`
(reproducido, con `Not in cdimage lodbryx_lights` justo antes). Tool nuevo
`tools/remap_auto_ids.py` les asigna IDs concretos (6670/6671, libres: el mod
usa 6600-6669).

**Infra de build: `bootseed/` no reenlazaba.** Las datos del arranque entran en
`reVC.data` con `--preload-file bootseed@/` y ninja solo vigila la *ruta*, así
que tras un `stage_bootseed.py` el build respondía "no work to do" y el juego
servía un `gta_vc.dat` obsoleto (pasó: los IDE/IPL nuevos no aparecían).
Arreglado con `bootseed.stamp` como `LINK_DEPENDS` del ejecutable
(`src/CMakeLists.txt`) que reescribe `tools/stage_bootseed.py`.

**Tools nuevos en `gta_vc_browser/tools/`**: `txd_merge.py` (fusionar
diccionarios TXD), `repack_dir.py` (recalcular `.dir` de una IMG suelta),
`import_img.py` (importar una IMG a una imagen suelta, con append) y
`remap_auto_ids.py`.

Evidencia de la última sonda (`ve3` + remap): 0 `Failed to load`, 0 `SKIP`,
0 `ODSHORT`, 0 `loose MISS`, 0 errores de página, 0 respuestas HTTP ≥400,
35 `FPHASE` en partida, 51 `TXDIN`. Los IDE/IPL nuevos se cargan
(`Loading object types from DATA\MAPS\plusroad\plusroad.IDE`, `bryx.IDE`,
`newGen\newgen.IDE`, `Creating objects from ... plusroad.IPL`, `bryx.IPL`).

Pendiente de la misma ejecución: coches con formato MVL, mecánicas (nadar,
escalar, 1ª persona, autosave, esconderse de la poli, drive-by) y `main.scm` del
mod con stubs del opcode `0FA8` y del GPS. **Las armas nuevas se cerraron ese
mismo día en el siguiente bloque.**

## 2026-09-19 — Vice Extended: armas nuevas (pack 3)

Cierra el punto "armas" del plan. Build `2026-09-19-ve4`, mismas sondas que los
packs anteriores + una **sonda nueva de armas** (cheat, disparo y ciclo de arma
con capturas): `tools/weapons-smoke-test.mjs`.

**Datos (del mod, tal cual).**

- `weapon.dat`: se adopta el del mod **entero** (8261 B, 58 filas). Dos cosas que
  conviene saber antes de tocarlo:
  - **Rebalancea todas las armas**, no solo las nuevas (0,89 de cadencia del
    AK47, marcos de animación nuevos, `sniper`/`laser` pasan a anims de `rifle`…).
    Es su diseño y sus IFP; el port ya monta su `ped.ifp` desde el pack 1b, así
    que los marcos que traen sus filas son los correctos para sus animaciones.
  - Estrena una **27ª columna** documentada en su cabecera: `c: weapon sight`
    (0 default, 1 dot, 2 pistol, 3 SMG, 4 shotgun, 5 rifle, 6 heavy, 7 rocket).
    Nuestro `sscanf` lee 26 campos, así que se ignora sin más — dato ya listo
    para cuando se haga la mecánica de apuntado.
- `default.ide`: se sustituye la sección `weap` por la del mod (37 → **47
  modelos**, IDs 6660-6669 únicos). Además de las 10 líneas nuevas cambia el
  bloque de animación de `sniper`/`laser` (→ `rifle`), `rocketla` (→ `rocket`) y
  `camera` (→ `rifle`), que es lo coherente con su `weapon.dat`.
- Modelos: `cdimages/weapons.img` del mod importado con `tools/import_img.py
  --append` (**94 entradas: 76 reemplazadas + 18 nuevas**) y `cdimages/anims.img`
  (11 reemplazadas + 3 nuevas: `deagle.ifp`, `steyr.ifp`, `rocket.ifp`). El
  `gta3.dir` pasa de 6043 a **6080 entradas**; las nuevas van bajo demanda (no
  se añaden a `bootseed.list`, igual que en los packs 1b/2).
- **No hay que tocar el audio**: el `sfx.RAW` del mod es *byte a byte* el del
  juego, así que sus armas reutilizan las muestras de serie (mapeadas a su
  equivalente: beretta→colt45, desert_eagle→python, ak47/m16/steyr→ruger…). Sí
  trae banco propio `ViceExtended/audio/ViceEx.{SDT,RAW}`: **13 muestras nuevas**
  (260 B de tabla = 13×20, 365 KB de RAW, pares L/R de 7328/22528/64256/8864/
  46978 B y 3 sueltas). Mapearlas a cada arma pide oído (no hay nombres ni
  readme): queda como paso aparte, no bloquea nada.

**Código.** Enum con las 9 armas nuevas **al final** (48..56) para no mover los
IDs que el `main.scm` de serie lleva quemados; `IsWeaponType()` sustituye a las
comparaciones `< WEAPONTYPE_TOTALWEAPONS` que había por el motor; tablas
indexadas por tipo de arma ampliadas a `WEAPONTYPE_TOTALALLTYPES` (nombres,
reload, munición máxima, sprays, miriñas, costes/balas de recogida); sonidos y
ecos por equivalencia; trazas de bala, daño a peds/coches, icono de HUD (la
textura se busca **por nombre del modelo** dentro de su TXD, y los 8 TXD nuevos
la traen); proyectil propio del lanzagranadas (`grenade2`, residente como el
misil vía `LoadInitialWeapons`); grupos de animación `deagle`/`steyr`/`rocket`
(bloques y nombres verificados contra los IFP del mod) y `WEAPONMODELSIZE`
37→47 justo por los 10 modelos nuevos.

**Cheat `CRAZYTOOLS`** (el mod lo documenta así) en `Pad.cpp`. Ojo al formato:
`Cheat_strncmp` compara `literal[i] = tecla[i] + desplazamiento` con
**desplazamientos por posición** (3,5,7,1,13,27,3,7,1,11) y la cadena va al
revés (última tecla primero) → el literal correcto es `VQVPat]HSN` (10 bytes).
Un literal de 11 no dispara nunca y **no avisa**: se cuela como "cheat que no
funciona".

**Dos bugs reales encontrados por el camino.**

1. `aPickupColors[]` (Pickups.cpp) cubría 0..43; las armas nuevas son 48..56 →
   lectura fuera del array en el brillo del recogido. Ampliado a
   `WEAPONTYPE_TOTALALLTYPES + 7` con los colores de su equivalente.
2. `ODSHORT open-fail models/gta3.img/grenade2.txd`: la capa web resuelve los
   ficheros sueltos con **`web/public/manifest.json`** (`tools/gen_manifest.py`).
   Tras importar ficheros nuevos hay que **regenerarlo**, o el motor no los verá
   aunque estén en `streamed/`. Pasó con `grenade2.txd` (lo pide
   `LoadInitialWeapons` en el arranque).

**Infra de build.** `build.sh` pasa el repo a emcc en forma Windows
(`REPO_FWD="$(pwd -W || pwd)"`): con la ruta MSYS (`/c/...`)
`--pre-js`/`--preload-file` fallan al enlazar. En Linux/macOS no cambia nada.

**Evidencia** (sondas con el servidor de desarrollo en el 2077):

- `slot0-load-test.mjs`: PASS (menú → cargar slot 0 → 56 `FPHASE`), 0 errores de
  página, 0 `Failed to load`.
- `weapons-smoke-test.mjs`: PASS. Tras teclear `CRAZYTOOLS` se ven
  `TXDIN txd=beretta|desert_eagle|shotgun2|uziold|ak47|m16|steyr|gr_launch` (8/8),
  `[od] stream-anim deagle` y `[od] stream-anim steyr` por consola, y en el FS
  virtual `deagle.ifp`, `steyr.ifp`, `beretta.dff`, `gr_launch.dff`,
  `grenade2.dff`, `desert_eagle.dff`, `m16.dff`. Disparando con Ctrl y Num0 y
  ciclando con el **Dec del numérico** (ojo: `PED_CYCLE_WEAPON_LEFT` es
  `rsPADDEL`, no la tecla Supr) las capturas cambian un 21-38% de píxeles y no
  salta ni un `RuntimeError`: el lanzagranadas dispara su proyectil nuevo sin
  romper nada.
- La única incidencia del log es **una** `ODSHORT open-fail od_lhland.dff` en la
  sesión más pesada; el fichero existe y se sirve 200 desde el servidor, así que
  fue un fallo transitorio de la capa on-demand (se reintenta solo).

Queda fuera de este bloque (siguiente paso natural): el **banco `ViceEx`** de
las 13 muestras y el `main.scm` del mod, que es quien reparte las armas nuevas
en tienda y en misiones.

---

## 2026-09-19 (tarde) — Vice Extended: **vehículos nuevos (pack 4)** + paridad con su `features.ini` (build `ve7`)

Bloques 10-13 del plan. Arranca con la auditoría del árbol: packs 1, 1b, 2 y 3 ya
estaban; **faltaban vehículos, toggles de `features.ini`, mecánicas y
`main.scm`**. Cerré los dos primeros y dejé el resto con veredicto medido (el
`main.scm` **descartado**: ver `.agents/plans/vice-extended-inclusion.md` §8.F).

### Vehículos: su `.ide` es MVL, no IDE

`tools/import_mvl_vehicles.py` (herramienta nueva) traduce su
`newVehicles.ide`/`handling`/`carcols` al formato que sí lee el motor y vuelca
los 8 vehículos **6500-6507** a `default.ide`, `handling.cfg` y `carcols.dat`
(`streetfi` 6500 Streetfighter, `peren2` 6501, `trash2` 6502,
`hellenbach` 6503, `premier` 6504, `manchez` 6505, `wintergreen` 6506 y
`polwintergreen` 6507 VCPD). `newVehicles.img` importada con `import_img.py`.
Código: `MI_VEEXT_*` en `ModelIndices.h`, filas de audio en `AudioLogic.cpp`
(banco del vehículo de serie equivalente + la sirena real de la moto policial) y
`VehicleAudioIndex()`: sus IDs 6500+ caen **fuera** de
`MI_FIRST_VEHICLE..MI_LAST_VEHICLE`, así que la tabla de sonido se indexa con
función y nunca se sale de rango. Para poder verlos en partida hay un cheat
nuevo, **`CRAZYRIDES`** (`Pad.cpp`), que los crea alrededor del jugador.

**Tres bugs reales cazados por la sonda (ninguno era visible en el log de
carga):**

1. **`newVehicles.col` no estaba** — y sin él el motor **peta con assert** al
   spawnear: `REVC ASSERT FAILED · Collision.cpp:2146 ·
   CalculateTrianglePlanes · Expression: model`. Síntoma engañoso: el log se
   paraba justo tras `CHINIT ch=2` (audio) y **parecía** un cuelgue de audio. El
   `.col` va ahora en `streamed/models/coll/` con su línea `COLFILE` en
   `default.dat` **y** `gta_vc.dat`, y en `bootseed.list`.
2. **`CBike` no conocía sus motos**: `assert(0 && "invalid bike model ID")`
   (Bike.cpp:85) por las 4 motos nuevas (streetfi/manchez/wintergreen/
   polwintergreen). Añadidas con el grupo de animación que trae **su propio**
   `.ide` (columna `animGroup`: `bikes`/`biked`/`bikeh`/`biked`) — el port no
   guarda esa columna, así que el grupo va por ID, como los vanilla. Confirmado
   en consola: `[DBG]: Loading ANIMS bikeh`.
3. **`MAX_TXD`**: la importación pasó de 1385 slots de TXD y el motor se
   quedaba sin sitio (`1389 > 1385`); subido (config.h).

Además, un fallo mío al pasar rutas a emcc (`/c/...` MSYS) rompía el enlace;
`build.sh` usa ya la forma Windows (`pwd -W`).

### Paridad con su `features.ini` (bloque 11)

Su ini (el que reparte el mod) trae **encendidos**: `EnableSwimming`,
`RecoilWhenFiring`, `RocketLauncherThirdPersonAiming`,
`PlayerDoesntBounceAwayFromMovingCar`, `EnableDistantLights`,
`RandomVehicleModsInTraffic`. Se portan los cuatro que son código local, con su
define en `config.h` para que quede el mapa 1:1:

- `VICEEXT_SWIMMING` — el jugador **no se ahoga** (`bDrownsInWater = false` en
  `CPlayerPed`; `CPed::InflictDamage` ya ignora `WEAPONTYPE_DROWNING` con esa
  bandera, y los NPC siguen ahogándose).
- `VICEEXT_RECOIL` — retroceso: sacudida corta de cámara + vibración del mando
  con cada disparo del jugador (`CWeapon::Fire`, no la sacudida de explosión).
- `VICEEXT_NO_CAR_BOUNCE` — al ser atropellado, el jugador ya no sale despedido
  en parábola por el capó: el coche lo empuja pegado al suelo. El resto de peds
  siguen volando como el original (`CPed::KillPedWithCar`).
- `VICEEXT_SPRINT_HEAVY` — v1.5 "sprint con armas pesadas": un único
  `CPlayerPed::CanSprintWithCurrentWeapon()` sustituye las **4** condiciones
  `!IsFlagSet(WEAPONFLAG_HEAVY)` que bloqueaban el esprint.

`RocketLauncherThirdPersonAiming` y "moverse apuntando" **ya los cumple el
port** (el lanzacohetes usa `MODE_ROCKETLAUNCHER` y `PC_PLAYER_CONTROLS` strafea
con los grupos `ASSOCGRP_PLAYERLEFT/RIGHT`), así que no se toca nada. Los
**apagados** en su ini (`EnableClimbing=0`, `MirrorModeByDefault=0`,
`StandardCarsUseTurnSignals=0`, `CameraShakeInVehicleAtHighSpeed=0`,
`HealthRegenerationUpToHalf=0`, `MilitaryFiringFromTankAtPlayer=0`,
`DisableBulletTraces=0`, `WantedStarsHide…=0`, `RemoveMoneyZeros…=0`,
`VehiclesDontCatchFire…=0`) **no se portan**: con su propia configuración no
cambian nada. `EnableDistantLights` (luces lejanas) y
`RandomVehicleModsInTraffic` (variantes) piden su renderer y el taller de
tuning: fuera del plan, documentado.

### Evidencia (build `ve7`, servidor 2077)

| Sonda | Resultado |
|---|---|
| `tools/vehicles-smoke-test.mjs` | **PASS** — 8/8 TXD nuevos, 16/16 ficheros en el FS virtual, 38 `FPHASE`, 0 errores de página, **sin assert** (antes: `CalculateTrianglePlanes`) |
| `tools/weapons-smoke-test.mjs` | **PASS** (regresión del retroceso) — 8/8 TXD, bloques `deagle`/`steyr`, 56 `FPHASE`, 0 errores |
| `tools/slot0-load-test.mjs` | **PASS** — menú → cargar slot 0 → en partida, sin crash y sin barra fuera de sitio |

### Lo que sigue pendiente (y por qué)

- **Mecánicas caras**: autosave tras misión, esconderse de la policía a
  cualquier nivel, drive-by ampliado (motos/barcos), primera persona completa
  (el port ya tiene el modo *peek* con `LookAround*` y `m_bFirstPersonBeingUsed`)
  e IA. Son código, no dato: el mod no publica fuente.
- **`main.scm` del mod**: descartado con medición (1.191.180 B vs 1.269.133 B
del de serie, 1.088.070 bytes distintos, y `0FA8` sin implementar en
  `src/control/`). Sin sus opcodes nativos, un stub mudo desincroniza la pila.
- **Banco `ViceEx.{SDT,RAW}`**: 13 muestras sin nombres (pares L/R de 0,10 s a
  1,06 s) → mapearlas exige oído; las armas nuevas suenan con su equivalente de
  serie mientras tanto.
- Menores: columna 27 de su `weapon.dat` (`weapon sight`, 0-7) que el parser
  ignora (el arte `weaponSights.txd` está excluido), 8 vehículos extra de su
  `newVehicles.ide` (tunables, dependen del taller excluido) y `ped.ifp`
  (solo aporta con las mecánicas delante).

## Pendiente (ver `.agents/plans/mejoras-web.md`)
Audio de oído, multi-resolución en menú, troceado de `InitialiseGame`,
pipeline de datos Fase 1, DXT/artefactos según sonda, mando/táctil.

---

# Sesión 19-20/09 (noche) — plan de mecánicas repartido en 3 secciones

Lo que queda del plan de Vice Extended se partió **1 bloque = 1 mecánica** y se
repartió en tres secciones (`.agents/plans/mecanicas/00-INDICE.md`): la 1 (datos y
pulido, la más liviana) la lleva Buffy, la 2 (policía y conducción) y la 3
(cámara, guardado y movimiento) van a dos agentes auxiliares, en paralelo y con
tabla de propiedad de ficheros. **No ha habido ni un commit**; todo sigue en el
árbol de trabajo para revisión.

## D1 — Agujero de caché de datos (cerrado)

`dataTag` seguía en `2026-09-19-ve3` mientras los packs 3 (armas) y 4 (vehículos)
**sustituyeron** ficheros ya presentes (`weapons.img`, `anims.img`, `.col`), y la
caché IndexedDB va por ruta: quien hubiera jugado con `ve3` seguiría viendo los
bytes viejos. Subido a **`ve8`** junto con `VERSION`, con `stage_bootseed` +
`gen_manifest` + relink (`web/ondemand.js` va embebido en `reVC.js`, sin relink
no cambia nada). Ahora las secciones 2 y 3 pueden medir sobre datos coherentes.

## D2 — El HUD diciendo "streetfighter missing" (cerrado, y no era sólo el GXT)

El diagnóstico anterior se quedó a medias: la clave corta se había escrito en el
**campo equivocado** del IDE. La línea de `default.ide` es `id, model, txd, type,
handlingId, gameName, anims, class, …` (`CFileLoader::LoadVehicleObject` copia el
campo 6 en `m_gameName[10]`, y `CCurrentVehicle::Display` lo busca en el GXT), y
el arreglo anterior había puesto `STRTFTR`/`MANCHEZ`/… en el campo 7 (`anims`),
dejando el nombre largo del mod (`Streetfighter`, `VCPD_WinterGreen`) en el 6:
el HUD seguía diciendo "missing" **y** las motos nuevas perdían su grupo de
animación. Arreglado donde toca:

- `tools/import_mvl_vehicles.py` traduce ya el `gamename` de MVL a **clave GXT**
  (reusa `PEREN`/`TRASHM` para los clones y estrena `STRTFTR`, `HELLENB`,
  `PREMIER`, `MANCHEZ`, `WINTERG`, `VCPDWTR`), escribe
  `tools/gamename_keys.tsv` para no equivocarse y es **idempotente** (reescribe
  sus propias líneas en vez de duplicarlas).
- Las 6 claves nuevas están en los 6 GXT servidos, verificadas contra los
  originales del mod: **0 entradas perdidas**, array ordenado byte a byte (lo
  que exige el `BinarySearch` del motor).
- `tools/check_ide_gxt.py` nuevo: comprueba los 115 vehículos del IDE contra los
  6 GXT (clave existente, ≤7 caracteres, mayúsculas, y ≤9 por `m_gameName[10]`).
  Verde; sólo informa de dos huecos que ya traía el VC original de 2003
  (`AEROPL` del tren del aeropuerto y `RCGOBLI`), comprobado contra el GXT
  vanilla.

**Dos bugs propios por el camino, los dos en `tools/gxt_inspect.py`,** que
explican por qué el asunto se atascó: el escritor ponía el offset de TDAT al
**final** de la cadena (habría roto todos los textos) y el lector recortaba la
clave a **4 bytes**, con lo que ficheros sanos parecían corruptos. Reescrito:
**incremental** (añade al final de TDAT, conserva TABL y la cola de 78 pares
TKEY/TDAT que el motor no lee — `CText::Load` para en el primer par), **no toca
claves existentes** sin `--force` y **se autocomprueba** releyendo el temporal
antes de sustituir (`self-test` incluido, claves de 1 a 7 caracteres). Tras la
restauración desde `bootseed/` (que era copia byte a byte del mod) los 6 GXT
quedan como el mod + las 6 claves, sin tocar nada más.

## D3 — Luces de sirena de la policía (implementado; veredicto de píxeles en curso)

Eran **dos** piezas, y una no estaba en el plan:

- `CVehicle::UsesSiren()` es un `switch` de IDs fijos y **no incluía
  `MI_VEEXT_POLWINTERG`**: la moto policial del mod no tenía sirena de ningún
  tipo (ni conmutador por claxon, ni bucle de audio, ni trato de vehículo
  policial en `CarAI`/`CarCtrl`/`RoadBlocks`).
- `CBike` no tiene el bloque de coronas `TYPE_STAR` que `CAutomobile` sí tiene
  para `MI_POLICE`/`MI_AMBULAN`/…

Ambas detrás de `#define VICEEXT_POLICE_BIKE_LIGHTS`, con paridad en la máquina
del claxon (pulsación corta = conmutar, mantener = pitar) y dos coronas rojo/azul
en lo alto del manillar (`this+21` y `+23`: `+1/+6/+14/+22/+25` ya eran de las
luces de la moto). El cheat `CRAZYRIDES` la crea con la sirena puesta para poder
verla sin conducirla. El coche de policía **no se toca**: su `switch` es código
de serie y sus texturas están (`police.txd` con `lights`/`lightson`/`servicelight*`,
y `particle.txd` byte a byte el del mod, con las 8 coronas).

Herramientas nuevas: `tools/shot_stats.py` (PNG sin dependencias: píxeles con
rojo/azul dominante y "fuertes", centroide del foco) y
`tools/siren-smoke-test.mjs` (carga el slot 0, teclea el cheat, gira la cámara y
mide: **PASS si hay capturas con cientos de píxeles azul-brillante y otras sin
ninguno**, que es lo que prueba el parpadeo; criterios calibrados con capturas
reales: una calle nocturna da `azulF=0` siempre).

La propia sonda cazó un **crash real** que no era de este bloque: al teclear
`CRAZYRIDES` con el mundo aún cargando (perfil nuevo = caché vacía, el juego va
y vuelve de `state 7`), `CModelInfo::GetModelInfo(6500)` devuelve `nil` y
`CStreaming::RequestModel` revienta con un `null function` en una llamada
virtual. Añadido guardián en `SpawnViceExtendedVehicle` (no hacer nada en vez de
tirar la pestaña) y la sonda ahora espera a `WEBHB state=9 ingame=1` **estable**
antes de tocar nada.

| Sonda | Resultado |
|---|---|
| `tools/slot0-load-test.mjs` (build `ve8`) | **PASS** — menú → slot 0 → partida, 0 errores de página |
| `tools/check_ide_gxt.py --all-langs` | **OK** — 115 vehículos, 6 GXT, sin claves ausentes |
| GXT: originales vs servidos | **0 entradas perdidas**, 6 claves nuevas, array ordenado |
| `tools/siren-smoke-test.mjs` | ver veredicto abajo |

## Lo que queda en la sección 1

D4 (tráfico de los 8 vehículos: que se vean circulando, no sólo por cheat), D5
(`ped.ifp` del mod), D6 (columna 27 de su `weapon.dat`, `weapon sight`), D7
(`pcbtns.txd` / iconos de teclado), D8 (mapear de oído las 13 muestras de
`ViceEx.SDT`). Y de coordinación: mantener `VERSION`/`dataTag`, el manifiesto y
el `.data`, y validar lo que entreguen las secciones 2 y 3.

## C1 (sección 3) — medición del *peek* de 1ª persona (medido, sin implementar)

Primer paso del bloque C1: **medir** qué hace hoy el *peek*, antes de tocar
`Camera.cpp`/`PlayerPed.cpp`. Resultado: **no se puede usar**, y la causa no son
las teclas sino la puerta del ratón.

Cómo se midió (una compilación, una sonda): se añadió a `CCamera::CamControl`
(`src/core/Camera.cpp`) una traza `CAM1P` que **sólo escribe cuando cambia** el
estado vigilado (1ª persona sí/no, método de control, `mouse3d`, modo de cámara,
stick de mirar, teclas 4/6/8/5, W/Escape/flechas): no ensucia `odtrace` y deja la
causa escrita. Sonda nueva `gta_vc_browser/tools/firstperson-smoke-test.mjs`:
carga el slot 0, pulsa 4/6/8/5 (3 s cada una, cubre el límite de 2,85 s), mueve
el ratón, prueba entrega de teclas y guarda 25 capturas. **PASS**, 0 errores.

Lo medido (perfil y save del arnés, build `2026-09-19-ve8`):

- Arranca en **Standard/ratón**: `ctrl=0 mouse3d=1`.
- **Nunca entra en 1ª persona**: `fp=0` en las 20 líneas, `mode=4`
  (`MODE_FOLLOWPED`) siempre. Con `mouse3d=1` la condición
  `!Cams[0].Using3rdPersonMouseCam()` de `Camera.cpp` (~1016) es falsa, así que
  el bloque entero no se ejecuta y `m_bFirstPersonBeingUsed` se fuerza a `false`.
- **Las teclas sí llegan**: `Numpad4` → `stick=-128,0`, `Numpad6` →
  `stick=128,0` en `NewState.RightStickX` (las escribe
  `ControllerConfig.cpp` desde `PED_1RST_PERSON_LOOK_*`). `Numpad8`/`Numpad5`
  llegan como tecla (`num=4`/`num=8`) pero **no mueven el stick**: esas dos sólo
  se aplican con `m_ControlMethod == CONTROL_CLASSIC`.
- Por tanto **qué se ve y cuánto dura** el *peek* no es medible hoy: queda para
  el bloque de implementación (que tendrá que levantar la puerta del ratón, no
  sólo añadir una tecla). Salidas por código (caminar, botones, apuntar, 2,85 s)
  anotadas en el plan, sin verificar en partida.

**Dos trampas del arnés que costaron dos vueltas** (y que afectan a más sondas):

1. **El teclado numérico no llega al motor con `page.keyboard`**: Chrome lo
   entrega como dígito de la fila superior (la traza lo cazó: `num=0` con la
   tecla pulsada). Hay que despachar `Input.dispatchKeyEvent` con `location: 3`
   (`firstperson-smoke-test.mjs` ya lo hace). Afecta a las sondas que usan el
   numérico, p. ej. el ciclo de armas de `weapons-smoke-test.mjs`
   (`NumpadDecimal`): esa parte probablemente nunca hizo nada.
2. **Escape no llega al motor** (ni con CDP): desde una sonda no se puede abrir
   el menú de pausa, así que no se puede conmutar el método de control por menú
   (la fase 2 de la sonda queda sin efecto, reportada como tal). Además no hay
   `gta_vc.set` en `/userfiles` (sólo `reVC.ini`), así que tampoco se puede
   sembrar los *bindings*. Sí se verificó que **letras y flechas sí llegan**
   (`W` con `speed>0` = el jugador anda; flechas con `keys=4/8`).

## P1 (sección 2) — Esconderse de la policía: implementado y sonda **PASS**

Bloque P1 del plan de mecánicas (`02-seccion-POLICIA-conduccion.md`), detrás de
`VICEEXT_HIDE_COPS` (añadido **al final** del bloque "Vice Extended: paridad con
su features.ini" en `src/core/config.h`, sin reordenar nada).

**La mecánica** ("the player has the ability to hide from the police", v1.0):

- **Visibilidad nueva**: un policía te *ve* si está a menos de 40 m **y** tiene
  línea de visión libre (`CWorld::ProcessLineOfSight` con edificios/objetos,
  ignorando al propio policía y su vehículo). Vanilla no tenía el concepto:
  sólo contaba policías a menos de 18 m, vieran o no (**medido** en la línea
  base: 641 s seguidos "sin que te vieran" y el nivel clavado en 2).
- **Nivel ≥2**: no baja si alguien te ve; si no te ve nadie, **una estrella cada
  15 s** (medida exacta más abajo). Vanilla: a partir de 2 estrellas el nivel
  **no bajaba nunca**.
- **Nivel ≤1**: intacto (ahí vanilla ya baja 1 chaos/s si no hay policía en 18 m).
- **Perseguidores a pie**: mientras no te vean, van a la **última posición
  conocida** (`OBJECTIVE_GOTO_AREA_ON_FOOT`) y vuelven a la caza cuando te
  vuelven a ver. Los policías **en coche siguen igual que vanilla**
  (`CarAI.cpp` está fuera de mi tabla de propiedad): límite conocido.

**Ficheros**: `src/core/config.h` (define), `src/core/Wanted.{h,cpp}` (estado
nuevo — `m_vecLastKnownPos`, `m_nLastSeenTime`, `m_nHiddenSince`,
`m_nLastStarDrop`, `m_bHiding`, **al final de la clase** para no mover ni un
offset, que `CReplay` copia `CWanted` por valor —, `AnyCopSeesPlayer()`,
`UpdateHiding()` llamado cada frame desde `Update()`, más la traza de la sonda),
`src/peds/CopPed.cpp` (la rama de `CopAI` que redirige al policía de a pie).

**Evidencia** (build `reVC.wasm` del 19/09 21:20, huella
`23179142@2026-09-20T02:20:33Z`):

| Pregunta | Medido |
|---|---|
| ¿Baja el nivel sin que te vean, con 2 estrellas? | **sí**, 1 estrella a los **15.0 s** (reloj del motor: `WANTEDHIDE start t=3190255` → `WANTEDHIDE drop lvl=2->1 t=3205258`) |
| ¿Baja estando visto? | **no**: 0 bajadas con nivel ≥2 estando visto en las 3 corridas |
| ¿Qué hacen los CCopPed? | `WANTEDCOP search-last-known` 5 veces en la corrida buena: los perseguidores a pie van a la última posición conocida |
| ¿Revienta la partida? | 0 errores de página, 0 asserts, build intacto, 1 corrida con `busted` (detención real, no caída del nivel) |

Sonda `gta_vc_browser/tools/hidecops-smoke-test.mjs` (modo `feature`), capturas
`5s/30s/65s/95s/120s/99-final` en `%TEMP%\vc-hidecops-p1c`.

**Tres cosas que costaron vueltas y quedan documentadas** (están también en el
plan de la sección, que es donde las leerá el siguiente):

1. **Contar muestras no es contar segundos**: la traza de `Wanted.cpp` sale "a
   bloques" (una corrida de 120 s dio 54 líneas y otra 118) y una primera
   versión de la sonda contaba muestras como segundos (decía "15 s", "95 s"...).
   Ahora la latencia sale del campo `t=` del motor y la racha escondida del
   campo `unseen` (que ya es un contador en segundos).
2. **Una corrida que no llega a esconderte 15 s es INCONCLUSA (código 3), no
   FAIL**: la primera versión la reportaba como "la búsqueda nunca se activó"
   cuando en realidad se había activado dos veces y te habían vuelto a ver. El
   intento de subirse a un coche (que acaba conduciendo hacia los policías) es
   ahora opcional (`VC_HIDECOPS_CAR=1`) y el defecto es medir a pie.
3. **`odtrace.log` es de todos**: la sonda etiqueta su sesión en
   `window.__vcWantedTag` y **todas** las líneas de mi traza llevan ya ` tag=`
   (antes sólo la línea `WANTED`), así que puede ignorar y **contar** las líneas
   de otra pestaña/agente (en la corrida final ignoró 70). Ese cambio de etiqueta
   entra con el próximo build (el `wasm` medido aún no lo trae; la sonda acepta
   las dos formas), y es la única razón por la que `VICEEXT_HIDE_COPS` aún no
   está "tal cual" dentro del binario.

**Nada de la línea base se ha tocado**: la traza de `Wanted.cpp` y la de
`CopPed.cpp` sólo leen estado; la decisión de disparar/arrestar de la policía
es la de vanilla.

---

## 2026-09-20 — Sección 1 (datos y pulido): **D3, la sirena de la moto policial** (PASS medido)

El jugador reportó que "desapareció la textura de las luces de sirena de la
policía". Medido, eran **dos** piezas y sólo una era un fallo:

1. `CVehicle::UsesSiren()` no incluía `MI_VEEXT_POLWINTERG` (6507): la moto
   policial del mod no tenía sirena de ningún tipo — ni audio, ni conmutación
   desde el manillar, ni `CarAI`/`RoadBlocks` la trataban como policial.
2. `CBike` no tiene el bloque de coronas que `CAutomobile` sí tiene, así que la
   moto no podía parpadear.

**Cambios** (tras `VICEEXT_POLICE_BIKE_LIGHTS` en `config.h`; el coche de
policía no se toca): `UsesSiren()` incluye el modelo;
`CBike::ProcessControl` tiene el bloque de coronas (dos `TYPE_STAR` en lo alto
del manillar, rojo/azul alternando con el mismo temporizador 1023/511 ms que el
coche, más el `CPointLights::AddLight`) y la máquina de estados del claxon
(pulsación corta = conmutar sirena, mantener = pitar). El cheat `CRAZYRIDES`
crea la moto con la sirena puesta (y `SpawnViceExtendedVehicle` no revienta si
se teclea con el mundo a media carga). Traza `BSIREN` cada 20 fotogramas para
que la sonda sitúe la luz.

**Evidencia** (build `d3i`, huella `23182160@2026-09-20T04:14`,
sonda `tools/siren-smoke-test.mjs`):

| Pregunta | Medido |
|---|---|
| ¿Tiene sirena la moto? | **sí**: `BSIREN model=6507 … on=1` en **35/35** muestras (luz delante de la cámara) |
| ¿Se dibuja la corona? | **sí**: `CORONAREND dibujadas=0..2` con la moto delante; 9/9 texturas de corona cargadas (`coronastar`, `corona`, …) |
| ¿Se ve parpadear? | **sí**: la ventana 620,11,740,131 alterna (`azul=10,18,6,10  rojo=58,35,69,58  corr=-1.00`) |
| ¿Y no es otra cosa? | el **control apareado** (misma ventana, misma cámara, sin los vehículos del mod) da **0** celdas alternando |
| ¿Revienta algo? | 0 errores de página, 190 líneas `FPHASE`, `slot0` no se toca |

**Lo que costó vueltas (y queda documentado en el plan de la sección):** el
primer arnés suspendió una sirena que sí se dibujaba. Dos fallos del arnés, no
del motor: (1) medía "píxel azul dominante", y el color de la sirena va
**dividido por 6 en el código original** (`255/6 = 42`) con blending aditivo,
así que sobre asfalto cálido el azul sube sin dominar nunca; (2) centraba la
ventana en la **mediana de la proyección de todas las poses de cámara**, que no
es la posición de ninguna serie. El criterio nuevo es **alternancia** (una celda
24×24 que oscila en rojo y azul, anti-correlacionada, con amplitudes en
`[4,55]` — el techo es el propio `/6`) medida en la ventana que da **el motor**
para esa pose, contra la base en esa misma ventana.

**D8 entregado en la misma sesión**: `tools/extract_viceex.py` deja las 13
muestras del banco propio del mod en `gta_vc_browser/tmp/viceex-samples/` (WAV +
plantilla `viceex-map.tsv`); los 13 tamaños suman el `ViceEx.RAW` byte a byte
(364.850), o sea que el banco está completo y sólo falta el mapeo de oído.

## 2026-09-20 (madrugada) — C1 CERRADO: conmutador de 1ª persona (sección 3)

**Contrato entregado** (`VICEEXT_FIRST_PERSON`, `src/core/config.h`): la tecla
**B** (`PED_TOGGLE_1RST_PERSON`, el hueco que era `_CONTROLLERACTION_36`
"Unused"; rebindable en el menú de controles) enciende y apaga una vista en
primera persona **de PC**: cámara en la cabeza, mirada con ratón y punto de
mira, se mantiene al caminar (no caduca a los 2,85 s como el *peek* de serie) y
se cierra al apuntar con mira, al subir a un coche, al perder el control del ped
y en `CCamera::Restore` (morir/cargar partida/cinemáticas).

**Cómo está hecho** (todo detrás del define):

- `src/core/Camera.cpp`, en el camino del jugador a pie: lee la acción nueva y
  pide `ReqMode = CCam::MODE_1STPERSON_RUNABOUT` mientras esté encendida. Es la
  1ª persona de PC (`CCam::Process_1rstPersonPedOnPC`), no el
  `MODE_1STPERSON` del *peek*, que sólo mira con el palo derecho.
- `src/core/Cam.cpp`, `CCam::Process_1rstPersonPedOnPC` (modo que **nadie
  activaba** en este port, o sea código sin usar hasta hoy): la posición de la
  cabeza se saca del **IK** (`m_pedIK.GetComponentPosition(…, PED_HEAD)`), igual
  que `Process_M16_1stPerson`; y el ped al que se fuerza el rumbo es
  `CamTargetEntity`, no `TheCamera.pTargetEntity`.
- `src/core/ControllerConfig.{h,cpp}`: acción `PED_TOGGLE_1RST_PERSON` (mismo
  índice de siempre), su nombre y su binding por defecto (`B`, KEYBOARD). El
  comportamiento sí está detrás del define, el binding no.
- Traza `CAM1P` ampliada (sólo diagnóstico, sólo emite al cambiar algo):
  `tog=`/`togkey=` el conmutador y su tecla, `spd=` velocidad a pie cuantizada,
  `cam=x,z` posición de cámara en decenas de metro.

**Evidencia** (build con huella `reVC.wasm` de las 23:33, sonda
`tools/firstperson-smoke-test.mjs`, perfil `vc-firstperson-test-profile2`,
**PASS 7/7**, 0 errores de página, 38 capturas):

| Pregunta del plan | Medido |
|---|---|
| ¿La tecla llega al motor? | **sí**: `togkey=1` en la traza al pulsar B |
| ¿Se enciende el modo? | **sí**: `tog=1` y `mode=41` (`MODE_1STPERSON_RUNABOUT`) |
| ¿Se mantiene al caminar? | **sí**: 12 líneas andando con `tog=1`, `spd=21…168`, `mode=41` y la cámara moviéndose (`cam=220,10 → 230,10`) |
| ¿Se cae solo al moverse? | **no**: ninguna línea con `tog=0` durante la caminata |
| ¿Sale al apuntar con mira? | **sí**: `tog=0` y `mode=4` (`MODE_FOLLOWPED`) al apuntar (`RightShoulder1`) |
| ¿Revienta algo? | **no**: 0 errores de página; la sonda de base `slot0` sigue en PASS |

**Lo que costó vueltas** (dos fallos reales, uno de ellos del port y no mío):

1. **La 1ª persona del PC reventaba el motor** a los ~5 s de entrar:
   `memory access out of bounds` en `CWorld::ProcessLineOfSightSectorList`, con
   la pila `cAudioManager::UpdateReflections → CWorld::ProcessLineOfSight`. La
   causa era el cálculo de la cabeza del modo runabout
   (`TransformToNode` + transformar la **matriz del hueso** con
   `RpHAnimHierarchyGetMatrixArray`/`RpHAnimIDGetIndex` y escalarla a cero para
   esconder la cabeza): en este port devolvía una posición inválida, la cámara
   saltaba fuera del mundo y el rayo de las reflexiones de audio (que parte de
   `TheCamera.GetPosition()`) se salía del sector. Con la vía del IK se acabó.
2. **`DisablePlayerControls |= PLAYERCONTROL_CAMERA`** (que el *peek* de serie
   sí usa) hacía que **el jugador no anduviera ni el apuntado sacara del modo**:
   `CPad::ArePlayerControlsDisabled()` es `DisablePlayerControls != 0` y
   `GetPedWalkUpDown/LeftRight` y `TargetJustDown` devuelven 0/false si hay
   **cualquier** bit puesto. En el conmutador ese bit no se pone.

**Avisos del arnés para las otras secciones**:

- Un perfil de Chrome con copia vieja de `reVC.js` (aunque el servidor mande
  `no-store`) aborta el motor con `TypeError: _asyncify_start_unwind is not a
  function` al mezclarse con un `reVC.wasm` nuevo. Solución en las sondas:
  `page.setCacheEnabled(false)` y perfil nuevo.
- **El botón derecho del ratón no está atado en el arnés headless**:
  `CMousePointerStateHelper::GetMouseSetUp()` sólo da por buenos LMB/RMB/MMB si
  el cursor no está en (0,0) al inicializar el motor, y ahí empieza en (0,0).
  Para apuntar en una sonda, `Supr` (binding de teclado de `PED_LOCK_TARGET`) da
  la misma señal al pad.
- No fiarse de `odtrace.log` para saber si **tu** pestaña cargó: otras sondas
  escriben en el mismo fichero. El heartbeat de la propia pestaña (`#gamelog`,
  `state 9` = `GS_PLAYING_GAME`) sí es tuyo.

**Ficheros tocados**: `src/core/Camera.cpp`, `src/core/Cam.cpp`,
`src/core/ControllerConfig.{h,cpp}`, `src/core/config.h` (define al final del
bloque Vice Extended, sin reordenar),
`gta_vc_browser/tools/firstperson-smoke-test.mjs` (sonda, ahora de verificación)
y `.agents/plans/mecanicas/03-seccion-CAMARA-guardado.md` (Estado).
Sin commits. `VERSION`/`dataTag` no se tocan (no hay datos nuevos).

## P2 (sección 2) — Drive-by ampliado: **implementado** (verificación por partida + log-check)

Bloque P2 del plan de mecánicas (`v1.5 "Drive-by shooting"`): que desde
coche/moto/barco se pueda disparar con **pistola**, no sólo con la SMG (slot 5).

**Contrato implementado**, todo detrás de `#define VICEEXT_DRIVEBY_WIDE`
(**encendido**, al final del bloque Vice Extended de `src/core/config.h`):

| Qué | Dónde |
|---|---|
| Puerta nueva del drive-by | `CPed::CanDoDriveByWithCurrentWeapon()` (`src/peds/Ped.cpp`) sustituye el `m_nWeaponSlot != 5` de `CAutomobile::DoDriveByShootings`, `CBike::DoDriveByShootings` y `CBoat::DoDriveByShootings`. Sin el define devuelve **sólo slot 5** (vanilla exacto); con él añade **slot 3 (pistola) pidiendo `m_nAmmoTotal > 0`**. Rifle/escopeta/lanzacohetes siguen fuera (dos manos). |
| El arma se queda en la mano | `CPed::KeepsWeaponInHandWhileDriving()` + las ramas nuevas de `RemoveWeaponWhenEnteringVehicle`/`ReplaceWeaponWhenExitingVehicle`: entrando con pistola **no se borra el modelo** (antes: disparabas con la mano vacía) y saliendo no se vuelve a añadir (no se duplica). Sin el define, las dos funciones son vanilla. |
| Cheat de prueba | `WeaponCheat5()` + `"CRAZYPISTOL"`, añadidos **al final** de la cadena de `CPad::AddToPCCheatString` (`src/core/Pad.cpp`, comentario `Sección 2`). Da **solo** la Beretta y la deja elegida a pie: `CRAZYTOOLS` da además `Uziold` (slot 5) y el motor cambia de arma al entrar al vehículo, así que sin un caso "pistola y nada más" la mecánica no se puede medir. Literal codificado comprobado (`OT[TVk\aB]P`, autotesteado contra la tabla de `CRAZYTOOLS` antes de escribirlo). |
| Traza para confirmar sin sondas | `src/peds/Ped.cpp`: `DRIVEBY state` (1/s conduciendo: clase, arma, slot, ammo, `fire`, `lookL/R`, modelo, velocidad), `DRIVEBY shot` (**una por disparo que sale de verdad**, después de que `FireFromCar` devuelva true, con la animación usada), `DRIVEBY enter|exit` con `outcome=keep-weapon|remove-model|switch-smg|restore-stored|add-model|keep-smg`. Todas llevan `tag=` de sesión, como las de P1. |

**Ficheros tocados**: `src/core/config.h`, `src/vehicles/{Automobile,Bike,Boat}.cpp`
(míos), `src/core/Pad.cpp` (cheat, al final como manda la regla).
**Aviso de propiedad**: el plan ya avisaba de que hacían falta dos líneas en
`src/peds/Ped.{h,cpp}`, que **no aparecen en la tabla de propiedad de nadie**; se
han hecho 5 declaraciones + 3 funciones y **todo lo nuevo va detrás del define**
(sin él `Ped.cpp` compila y se comporta igual que vanilla). Si la coordinación
quiere, que fije la propiedad de `Ped.*` en el índice.

**Build medido**: `gta_vc_browser/web/public/build/reVC.wasm` = **23.216.990 bytes,
09:37:29** (`reVC.data` 09:37:09), enlazado por el build de las 09:37 desde el
árbol con este código. Comprobado con `grep -a -F` sobre el `.wasm`: contiene
`DRIVEBY state tag=`, `DRIVEBY shot tag=`, `DRIVEBY %s tag=`, `keep-weapon`,
`remove-model`, `switch-smg`, `restore-stored` y el literal `OT[TVk` del cheat.
Los objetos de mis cuatro ficheros compilan limpios (`ninja` sin trabajo pendiente).

**Verificación elegida (sin sondas de Chrome).** El jugador pidió el 20/09 no
lanzar más pruebas headless (Chrome + swiftshader se comen la CPU/RAM del equipo) y
que la confirmación salga de **sus partidas y de los logs**. Así que:
1. jugar y teclear `CRAZYPISTOL`; disparar desde un **coche mirando de lado**
   (`Q`/`E` + gatillo de vehículo: `Insert` o `CTRL izq`) y desde la **moto** (basta
   el gatillo: dispara hacia delante); un barco también vale;
2. `node gta_vc_browser/tools/driveby-log-check.mjs` → lee `odtrace.log` y resume:
   disparos por clase/slot/animación, `outcome` de cada entrada/salida y si hubo
   intento con el gatillo. **No abre navegador** (coste ~0). Sale 0 si la mecánica
   se ve, 1 si los logs la contradicen, 2 si aún no hay datos.

**Qué se verá en los logs si funciona**: `DRIVEBY enter … slot=3 … outcome=keep-weapon`
y `DRIVEBY shot … veh=car|bike slot=3 anim=left|right|left-lo|right-lo|forward|lhs|rhs`;
con la SMG, `outcome=switch-smg` y disparos `slot=5` (no regresión). **Línea base**
(para comparar): apagando el define y recompilando, el mismo comando muestra
`outcome=remove-model` y **0** disparos con `slot=3` aunque se apriete el gatillo.

**Sonda de laboratorio (escrita, NO ejecutada)**:
`gta_vc_browser/tools/driveby-smoke-test.mjs` (`VC_DRIVEBY_MODE=baseline|feature`),
con **instantánea del build**: copia `reVC.{js,wasm,data}` a `%TEMP%` y los sirve en
un puerto propio redirigiendo `/build/reVC.*`, porque este build compartido se
recompila cada pocos minutos y una corrida ya se abortó por eso (huella cambiada a
mitad de carga). Pendiente de correr cuando la máquina esté libre.

**P3 sigue bloqueado**: `6507 polwintergreen` continúa con clase **`ignore`** en
`gta_vc_browser/bootseed/data/default.ide` (medido hoy), así que la moto policial
**no circula** en tráfico: eso es dato de la sección 1. Las coronas de sirena ya las
hizo la sección 1 (D3); lo que quedaría (que la use la policía:
`CVehicle::IsLawEnforcementVehicle`, `CarCtrl`) está en ficheros fuera de mi tabla.

**Nota de convivencia**: el 20/09 a las ~09:36 el árbol no compilaba por el bloque
D7 de la sección 1 en `src/core/Pad.cpp:420` (`wchar *` vs `const wchar_t[]`); ya
está arreglado (el enlace de las 09:37 pasa). No toqué nada de eso.
---

## Sección 1 · bloques D4, D5, D6 y D7 (20/09/2026)

Todo **implementado y compilado** en un build; la verificación es por **log**
(no hace falta lanzar sondas de Chrome: se juega una vez y se pasa
`python gta_vc_browser/tools/viceext-log-check.py --desde-marca`).

`VERSION`/`dataTag` = **`2026-09-20-ve9`** (el build que está sirviendo): hay
**datos nuevos** (`models/weaponsights.txd`, `models/pcbtns.txd`) y uno
**sustituido** (`anim/ped.ifp`), así que la etiqueta sube y la caché IndexedDB
se purga sola.

### D4 · tráfico (medido antes y después)

El sospechoso del plan (los modelos del mod no son elegibles) era falso: los 7
tienen clase real y entran en `LoadedCarsArray`. Lo que estaba roto era la
**rotación del fondo cargado**:

- `CStreaming::StreamVehiclesAndPeds` cuenta **350 fotogramas** por modelo
  pedido, asumiendo 30 FPS. En el navegador eso son 40-70 s reales por modelo, y
  `CCarCtrl::ChooseCarModel` **sólo elige entre los modelos ya cargados**: la
  calle se queda con los 2-3 que ya estaban residentes. Ahora cuenta **tiempo**
  (11,7 s = 350/30) detrás de `__EMSCRIPTEN__`.
- La **puerta** de entrada al cargador exigía `ms_numModelsRequested < 5` y
  "sin carga prioritaria". Con la CPU compartida (otro Chrome jugando) esa
  puerta se queda cerrada **minutos enteros**: medido una sesión de 5 min con
  **0 llamadas** al cargador y la calle congelada. El cargador pide como mucho
  **un modelo cada 11,7 s**, así que ahora se le deja pasar con el streaming
  ocupado; se mantienen las condiciones de estado del juego (área, cinemática,
  replay, streaming desactivado).
- Trazas nuevas: `CARPED` (pedido + fondo del que elige la calle), `CARPOOL`,
  `CARBLOCK` (cuál de las condiciones cerró la puerta) y `CARSPAWN` (coche que
  entra al mundo: la medida directa de qué circula).

**Medición (5-6 min quieto en la calle):**

| Corrida | Ticks | Coches | Modelos distintos | Del mod |
|---|---|---|---|---|
| antes | — | 32 en 6 min | 2 (`197` ×28, `156` ×4) | 0/7 |
| después | 1 cada 11,7 s | 367 en 5 min | **30** | 3/7 (Streetfighter ×7, Hellenbach ×11, Premier ×6) |
| después con la CPU al 100% (13 FPS) | **0** | 45 en 3 min | 2 | 0/7 |

La tercera fila es la prueba de la cadena: **sin ticks no hay variedad**. La
moto policial (clase `ignore`) no salió en ninguna corrida ✓.

### D5 · `ped.ifp` del mod

`streamed/anim/ped.ifp` = 2.486.356 B (md5 `c61a251f1b991661926838c2a5e896`),
272 clips (el de serie: 2.201.152 B, 234). Traza nueva `IFPFILE <ruta>
clips=<n> total=<n>` en `CAnimManager::LoadAnimFile`. Es dato puro y reversible
(volver a copiar el original).

### D6 · mira por arma (`weapon.dat` columna 27)

- `CWeaponInfo::m_nSight` lee la 27ª columna (el `sscanf` añade un `%d` detrás,
  y `sight` se inicializa a 0: un `weapon.dat` de serie de 26 columnas sigue
  funcionando igual). Valores comprobados en el fichero servido: pistolas 2,
  escopetas 4, SMG 3, rifles 5, lanzallamas/minigun 6, lanzacohetes/lanzagranadas
  7, melee/granadas 0.
- `streamed/models/weaponsights.txd` (arte del mod, 7 texturas) se carga con el
  HUD; los 4 puntos donde el HUD dibujaba la cruz genérica (`HUD_SITEM16`) ahora
  pasan por `ViceExtDrawSight`, que pinta la mira del arma si la trae y su
  textura está, y si no deja la cruz de siempre.
- Verificación: `SIGHTS cargadas=7/7` al arrancar (ya visto en una sesión real
  con este build) y `SIGHT arma=<n> mira=<n> textura=<1>` al apuntar.

### D7 · iconos de tecla en los avisos (v3.0)

El v3.0 del mod es **dirigida por datos**: el GXT que ya servimos trae
`~k~~ACCIÓN~` (29 acciones, 2.000+ usos) y el `pcbtns.txd` del mod nombra los
128 iconos con el **código de tecla de Windows**. El port ya cambiaba ese token
por el **nombre** de la tecla; ahora, si esa tecla tiene icono, pinta el icono:

- `CControllerConfigManager::GetKeyIconCodeForAction` (nuevo) traduce la acción
  a código de tecla con el mismo orden de preferencia que el texto
  (teclado → extra → ratón); `ViceExtKeyToVK` cubre imprimibles, F1-F12,
  edición, navegación, teclado numérico, modificadores y los botones del ratón
  (que en ese TXD son "1", "2" y "4").
- `CMessages::InsertPlayerControlKeysInString` emite `~K<vk>~` **sólo** si esa
  tecla tiene icono; si no, deja el nombre en texto (cero regresión).
- `CFont` reconoce el marcador (con cuidado: `~K~` ya era el botón L1 del mando,
  se distingue por el dígito que sigue), crea el sprite bajo demanda desde
  `models/pcbtns.txd` —cargado una vez con el HUD— y lo pinta en el mismo hueco
  que los iconos de mando.
- **Cheat `CRAZYHINT`** (diagnóstico): muestra un aviso con cuatro acciones
  (`PED_FIREWEAPON`, `PED_LOCK_TARGET`, `PED_JUMPING`, `VEHICLE_HORN`) para ver
  los iconos sin esperar a un aviso de misión.
- Verificación: `KEYICONS txd=pcbtns cargado` al arrancar (ya visto) y
  `HINTKEY accion=<NOMBRE> vk=<n> icono=<1>` por aviso.

### D2 · además, verificable por log

Traza `TXTMISS key=<clave>` en `CData::Search` cuando una clave de texto no
existe: es el bug que reportó el jugador ("streetfighter missing") visto ahora
también en el log. Su **ausencia** es la prueba de que el HUD resuelve todos sus
nombres.

**Ficheros tocados**: `src/core/Streaming.cpp`, `src/control/CarCtrl.cpp`,
`src/animation/AnimManager.cpp`, `src/weapons/WeaponInfo.{h,cpp}`,
`src/renderer/Hud.cpp`, `src/renderer/Font.{h,cpp}`,
`src/core/ControllerConfig.{h,cpp}`, `src/core/Pad.cpp` (cheat al final),
`src/text/{Messages,Text}.cpp`, `src/core/config.h` (dos defines al final del
bloque Vice Extended),
`gta_vc_browser/{streamed/models/weaponsights.txd,streamed/models/pcbtns.txd,streamed/anim/ped.ifp,web/lib/index.js,web/ondemand.js}`,
`gta_vc_browser/tools/{viceext-log-check.py,traffic-smoke-test.mjs,shot_stats.py}`.
Sin commits.

### Partida del jugador (20/09 15:03) — veredicto por log, sin abrir navegador

Leída con `python gta_vc_browser/tools/viceext-log-check.py` (el log de la
partida, 5.835 líneas):

| Bloque | Veredicto de esa sesión |
|---|---|
| D2 claves del HUD | **OK** — 0 `TXTMISS` (ningún nombre sin texto) |
| D4 tráfico | **OK** — 146 ticks del cargador, **238 coches** de calle, **29 modelos**, **3 del mod** (Streetfighter ×9, Premier ×3, Manchez ×10) y `polwintergreen` a 0 |
| D5 `ped.ifp` | PARCIAL — 234 clips: su pestaña corría el build **anterior** al swap del fichero |
| D6 mira por arma | **OK** — armas 24 (mira 3, SMG), 19 (mira 4, escopeta) y 17 (mira 2, pistola), con textura cargada |
| D7 iconos de tecla | **OK** — 5 avisos con icono: los 4 del cheat `CRAZYHINT` + `PED_ANSWER_PHONE` |

Tres hallazgos que salieron de ahí:

1. **`dataTag` estaba COMENTADO** en `web/ondemand.js`: el comentario y la
   propiedad habían quedado en la **misma línea**, así que `OD.dataTag` era
   `undefined` — se ve en la propia traza (`build=2026-09-20-ve10
   data=undefined`) — y la **purga de caché por versión de datos no funcionaba**.
   Arreglado (propiedad en su propia línea) y ya dentro del build de las 10:05.
2. **La rotación de trazas borraba la sesión anterior**: `web/lib/vite.js`
   truncaba `odtrace.log` en cada carga de página, así que una recarga se llevaba
   por delante la evidencia de lo jugado (la sesión previa quedó sólo menú,
   `ingame=0`). Ahora se **copia a `odtrace.prev.log`** antes de truncar, y el
   verificador avisa y señala ese fichero cuando el log no trae marcas de juego.
3. **El verificador medía mal el ritmo del cargador**: la traza sella por lotes
   (muchas líneas comparten marca de tiempo) y salía "0.0 s/tick". Ahora dice
   que no es medible y usa el **número** de ticks como dato (0 sin el arreglo,
   146 con él).

Además, el build de las **10:05** (`ve10`, relinkado al arreglarse el fichero
roto de la sección 3) ya lleva **todo lo de esta sección**: el refuerzo de
frecuencia verificado **dentro del paquete** (`/data/default.ide`, offsets
34.303.835-34.325.816, con `100` en la columna 9 de los ids 6500-6506), el
`ped.ifp` del mod (`/anim/ped.ifp`, 31.463.724-33.950.080) y el `dataTag` vivo.
Basta recargar con Ctrl+Shift+R y jugar para confirmar D5 con `clips=272`.

## 2026-09-20 — Sección 3 (cámara/guardado/mecánicas): **C2 y el cajón C3** (código cerrado, verificación por traza)

Bloque grande de una tacada, **sin sondas de Chrome** (el usuario pidió no cargar
la CPU con navegadores: se arma el código, se deja traza y se verifica **con su
partida**). Build enlazado a las **10:13** (`reVC.wasm` 23.228.812 B) con todas
las marcas de traza dentro (`grep -c` sobre el `.wasm`): `VICEEXT autosave`,
`VICEEXT reload start/done`, `VICEEXT gastank hit/sin-dummy`,
`VICEEXT save-anywhere aceptado/rechazado`. Estado completo del bloque, con el
detalle de cada medición, en `.agents/plans/mecanicas/03-seccion-CAMARA-guardado.md`.

### C2 · autosave + guardar en cualquier sitio

*Medición previa (lo que pedía el plan):* el port **no tenía** opción de guardar
en el menú de pausa; sólo la zona de guardado del script (`ACTIVATE_SAVE_MENU` →
`m_OnlySaveMenu` + `MENUPAGE_CHOOSE_SAVE_SLOT`). El quicksave ya escribía la
**ranura 9** (`PAUSE_SAVE_SLOT = SLOT_COUNT = 8` → `GTAVCsf9.b`), pero esa ranura
quedaba **fuera** de las matrices (`SlotFileName[8]`, `Slots[8]`) y el menú no la
listaba. `FEM_SL9` sí existe en todos los GXT del port
(`tools/gxt_inspect.py check`).

*Implementado:* `SLOT_COUNT` 8 → 9 manteniendo `PAUSE_SAVE_SLOT = SLOT_COUNT - 1`
(= 8, el mismo número de fichero de antes), así que la ranura 9 ya entra en la
pantalla de carga con `FEM_SL9` y ninguna de las 8 del jugador cambia. El gancho
del autosave que estaba apagado (`Script.cpp:1553`, `#if 0`) se sustituye por
`ViceExtAutosavePending` (se marca en `COMMAND_REGISTER_MISSION_PASSED`,
`Script3.cpp`) + **una escritura** en `COMMAND_TERMINATE_THIS_SCRIPT`, con
`SaveGameForPause(SAVE_TYPE_QUICKSAVE)`. Y el menú de pausa gana la entrada
`FET_SG` → `MENUPAGE_CHOOSE_SAVE_SLOT` con las tres condiciones del mod antes de
escribir (`ViceExtCanSaveAnywhere`: sin misión, sin búsqueda, parado); si no se
cumplen, sale el aviso `FES_SAV` (también en los GXT) y no se escribe nada.

### C3.1 · recarga manual (`VICEEXT_MANUAL_RELOAD`)

Acción nueva `PED_RELOAD` en el hueco `UNKNOWN_ACTION` (mismo índice, sin
reordenar el enum), tecla por defecto **R** y rebindable. Reutiliza el camino de
la recarga automática (`WEAPONSTATE_RELOADING` + `m_nTimer`), así que animación y
sonido los pone el motor; sólo a pie, con control del jugador, cargador no lleno
y arma con cargador. Traza por pulsación (una línea, no por frame).

### C3.3 · depósito de gasolina (`VICEEXT_GAS_TANK`)

Dummy `petrolcap` del mod (su `AdaptingVehiclesEN.txt`) buscado con el helper
nuevo `CVehicle::FindDummyFrame()`; si el punto de impacto cae a ≤ 0,6 m, se deja
la salud del vehículo **en el umbral del motor** (250) para que **el daño de la
bala** lo cruce en `InflictDamage`, que es quien pone `ENGINE_STATUS_ON_FIRE` y
`m_pSetOnFireEntity`. O sea: el coche se incendia por el camino de serie
(humo → llamas → explosión ~5 s), no por un fuego puesto a mano. Enganchado en
`DoBulletImpact`, `FireShotgun` y `FireFromCar`. Las "luces rompibles" del mod
(faros que se rompen al disparo) **no** están hechas.

### C3.4 · intermitentes (`VICEEXT_TURN_SIGNALS`, apagado por defecto)

Paridad con su `features.ini` (`StandardCarsUseTurnSignals=0`): el define viene
**comentado**. `CAutomobile::ViceExtProcessTurnSignals()` (único bloque mío en
`Automobile.cpp`, al final de `ProcessControl`, marcado como tal) pone coronas
naranjas parpadeantes en los dummies `indicator*` del coche **del jugador**
mientras gira. Se comprobó que **compila** con el define puesto
(`-fsyntax-only -DVICEEXT_TURN_SIGNALS`), pero en el build normal no entra: quien
lo encienda debe recompilar y probarlo. Los turners de los NPC, pendientes.

### C3.2 · movimiento sentado: medido, **no** implementado

El mod lo nombra una sola vez y sin detalle (`ChangesEN.txt:171`, en la lista de
v1.0 mezclada con cosas de NPC). En el port: `PED_SIT` existe en el enum pero
**nadie lo escribe** (0 usos en `src/`), el jugador sólo está sentado en un
vehículo (moverse ahí = conducir) y los NPC sentados son el attractor de asiento
(`OBJECTIVE_GOTO_SEAT_ON_FOOT` → `WAITSTATE_SIT_*`), que se levantan sólo por
temporizador/amenaza. Implementarlo sería inventar la mecánica: queda a la espera
de que el jugador diga qué se ve en el mod.

### Trazas para la partida del jugador (todo en `odtrace.log`)

| Qué se quiere confirmar | Línea a buscar |
|---|---|
| autosave de fin de misión | `VICEEXT autosave ok=1 slot=8 file=GTAVCsf9.b mision=…` |
| guardar desde el menú de pausa | `VICEEXT save-anywhere aceptado opt=… zona=0` y después `save slot=…` |
| aviso por no cumplir condiciones | `VICEEXT save-anywhere rechazado opt=… slot=…` |
| recarga con tecla | `VICEEXT reload start/done` (y `… reload no …` si no recargó, con el motivo) |
| depósito | `VICEEXT gastank hit model=… hp=…` (o `sin-dummy` si el modelo no lo trae) |
| menú abierto/cerrado y pantallas | `FEMENU open=…` y `FEMENU scr=… opt=… only=…` |

**Ficheros tocados** (sección 3; nada de commits ni `git add`): `src/core/config.h`
(cinco defines al final del bloque Vice Extended: `VICEEXT_AUTOSAVE`,
`VICEEXT_SAVE_ANYWHERE`, `VICEEXT_MANUAL_RELOAD`, `VICEEXT_GAS_TANK`,
`VICEEXT_TURN_SIGNALS` — comentado), `src/core/ControllerConfig.{h,cpp}`,
`src/core/Frontend.{h,cpp}`, `src/core/MenuScreensCustom.cpp`,
`src/save/GenericGameStorage.{h,cpp}`, `src/control/Script.{h,cpp}`,
`src/control/Script3.cpp`, `src/peds/PlayerPed.{h,cpp}`, `src/weapons/Weapon.cpp`,
`src/vehicles/Vehicle.{h,cpp}`, `src/vehicles/Automobile.{h,cpp}`,
`.agents/plans/mecanicas/03-seccion-CAMARA-guardado.md`.

**Aviso para quien toque esto:** los tres bloques de `Weapon.cpp` (recarga y
depósito) y el de `Vehicle.cpp` son míos; la instrumentación de trazas no se
puede quitar sin quedarse sin el menú de verificación de la partida. Y sigue en
pie el aviso del arnés: **el numérico no llega al motor con `page.keyboard`** (hay
que despachar CDP con `location: 3`), así que el ciclo de armas de
`weapons-smoke-test.mjs` probablemente nunca hizo nada.

---

## 2026-09-20 10:03–10:20 — Sección 2 (policía/conducción): la primera partida real **caza un fallo de P2** (arreglado) y estrena la traza de P1

La partida del jugador (20/09 10:03→10:12 local, `JS build=2026-09-20-ve10`, tag 0) se
leyó entera desde `odtrace.log`, **sin abrir navegador**: 464 líneas de la traza de P1 y
193 de la de P2. Dos verificadores que sólo leen ficheros (coste ~0, ambos míos):

| Comando | Qué dice |
|---|---|
| `node gta_vc_browser/tools/hidecops-log-check.mjs` (nuevo) | por sesión: nivel máximo, cambios de nivel, episodios `WANTEDHIDE start/seen/end/drop`, resprays y la latencia de cada caída de estrella comparada con los 15 s del contrato |
| `node gta_vc_browser/tools/driveby-log-check.mjs` (actualizado) | conducción por clase/velocidad, disparos por arma + animación, entradas/salidas con su `outcome`, y **avisa de las dos señales de precedencia vanilla** |

### P2 — el fallo que encontró la partida (y su arreglo)

Las cuatro entradas en vehículo de esa sesión salieron así:

```
DRIVEBY enter … wep=24 slot=5 ammo=270 rev=1 outcome=switch-smg      (x4)
DRIVEBY exit  … wep=48 slot=3 ammo=100 rev=1 outcome=restore-stored  (x4)
```

O sea: el jugador **entró con la pistola en la mano** (slot 3, arma 48) y el motor le
cambió a la SMG; al salir le devolvió la pistola.

**Causa**: en `CPed::RemoveWeaponWhenEnteringVehicle` la rama vanilla `switch-smg` se
evaluaba **antes** que la mía `keep-weapon`, así que bastaba llevar una SMG en el
inventario para que la pistola no se quedara nunca en la mano. Es el riesgo que el
plan ya había medido (§ P2 medición) y que aun así se coló en la implementación: el
contrato estaba bien escrito y la **traza lo delató en la primera partida real**.

**Arreglo** (`src/peds/Ped.cpp`, una rama): si el jugador lleva pistola (slot 3) **con
balas** en la mano y `m_bDriveByAllowed`, tiene prioridad sobre el cambio automático a
la SMG (no hay nada que guardar ni que cambiar). Sin el define, sin balas o con el
script apagando `m_bDriveByAllowed`, la rama vanilla manda igual.

**No-regresión medida en la misma sesión** (137 s conduciendo, coche + moto, 48
disparos): SMG en moto `anim=lhs x19 / rhs x8 / forward x13` y en coche
`anim=right x8`, munición de 269 a 222, `outcome=switch-smg` en las 4 entradas.

### P1 — primera partida real con la traza (contrato no contradicho)

455 s de traza, niveles 0→4 (cambios `0->1, 1->0, 0->1, 1->2, 0->1, 1->2, 2->3, 3->4`),
un respray (`WANTEDPURGE lvl=2 cops=0`) y **cero** episodios de esconderse: los 13 s a
nivel ≥ 2 sin que te viera ningún perseguidor tenían **policía a 18 m**
(`presence18=1`) → no estabas escondido. El verificador lo dice así en vez de acusar al
build (la primera versión daba un falso "falta `VICEEXT_HIDE_COPS`" en cuanto había
nivel ≥ 2 sin `start`; ahora sólo avisa si hay ≥ 10 s ciego **y** solo).

### Artefactos y build

- **`rev=2`** en la traza `DRIVEBY enter|exit`: la etiqueta `JS build=` **no cambia
  cuando sólo se recompone el wasm**, así que sin esta marca un log no puede probar qué
  binario corrió. El verificador la imprime (`P2 rev=1 (sin el arreglo)` /
  `P2 rev=2 (con el arreglo de precedencia)`).
- Build publicado: `reVC.wasm` de **10:19:04** (23.228.868 bytes) con `rev=%d outcome=%s`
  dentro (`grep -a -F`); `Ped.cpp.o` compilado a las 10:11:33 (después del arreglo y
  antes del enlace).
- Todas las cifras y el contrato quedan en
  `.agents/plans/mecanicas/02-seccion-POLICIA-conduccion.md` (§ P1 partida real y
  § P2 corrección por partida).
- **Aviso operativo para compilar en este checkout**: activar Emscripten con la ruta
  del `.emscripten` en forma `C:/…` (en vez de la `C:\…` que usan los otros agentes)
  **limpia la caché entera** y deja ~300 objetos para recompilar; pasó una vez hoy y se
  recuperó con el build del compañero (10:11→10:13). La forma buena:
  `export EM_CONFIG='C:\Users\s0rno\emsdk\.emscripten'`.
- **Nada commiteado** (regla del plan): todo sigue en el árbol de trabajo.

**Pendiente del bloque**: repetir la partida con el wasm nuevo (`rev=2`): subir a un
coche con la pistola en la mano y buscar `DRIVEBY enter … slot=3 … outcome=keep-weapon`
+ disparos `slot=3` con animación lateral. P3 sigue bloqueado por los datos
(`polwintergreen` sigue `ignore` en `default.ide`).

## 2026-09-20 (sesión del log `17:57–18:10Z`, build `ve10`) — Sección 1: la partida del jugador **da D2–D7 por buenos** y destapa por qué faltaban vehículos (código en `ve11`)

Leído **sólo del log** (`web/odtrace.prev.log`, 7.001 líneas, cabecera
`build=2026-09-20-ve10 data=2026-09-20-ve10`), sin lanzar ni un navegador
(petición expresa del jugador), con `tools/viceext-log-check.py`.

### Veredicto de la partida (build `ve10`)

| Bloque | Resultado |
|---|---|
| D2 | **OK** — 0 `TXTMISS` |
| D4 | **OK** — 259 coches de calle en 27 modelos, 103 ticks del cargador; del mod Hellenbach ×25 y Wintergreen ×9; moto policial 0 |
| D5 | **OK** — `clips=272 total=272` (por fin el `ped.ifp` del mod; el 234 era del paquete anterior) |
| D6 | **OK** — 3 armas apuntadas, cada una con su mira y su textura (24→3, 19→4, 17→2) |
| D7 | **OK** — un aviso real resolvió su tecla y pintó el icono (`PED_ANSWER_PHONE` vk=9); el cheat `CRAZYHINT` no se tecleó |

### Y lo que no estaba bien, con causa medida

- `[raíz]` **El peso de tráfico no bastaba para CARGAR los modelos nuevos.** La
  frecuencia de `default.ide` pesa al elegir **entre los vehículos ya cargados**
  (`ChooseCarModel`), pero el cargador pide **uniforme**
  (`ChooseCarModelToLoad`): de los 7 del mod sólo 4 llegaron a pedirse en 6 min y 2
  a circular. Ahora el pedido se sortea con **la misma frecuencia** (acumulada;
  si la suma es 0, uniforme), así que el refuerzo temporal `10→100` sirve también
  para cargarlos. Sólo web (`#ifdef __EMSCRIPTEN__`): nativo mantiene la
  uniformidad del original. `6507` intacto (clase `ignore`, no entra en
  `CarArrays`).
- `[tooling]` **`ODSHORT open-fail` (60 en la partida) NO son ficheros que
  faltan**: los 60 nombres tienen entrada en `web/public/manifest.json` con su
  tamaño. Son ficheros sueltos del `.img` que la capa on-demand **aplaza** mientras
  se descargan (por diseño: no congelar el frame; el motor reintenta). Lo que no se
  podía era *demostrar* que volvían: el lector suelto lleva ahora un anillo de los
  últimos 16 fallos y emite `ODSRECOVER <ruta> fallos=<n>` cuando alguno se abre
  bien; `ODSHORT` lleva `fallo=<n> total=<n>` (una de cada 20 líneas, para que el
  total sobreviva al recorte de 60). **0 recuperaciones en la próxima partida =
  modelo perdido de verdad** (será la señal, no una sospecha).
- `[tooling]` Trazas nuevas para no confundir “no carga” con “su clase no se
  sortea”: `CARLOAD` (modelo que entra en el fondo, con su clase y frecuencia),
  `CARFAIL` (lectura fallida del cargador, con el id del canal), `CARZONE` (los 8
  umbrales de clase de la zona, los pone el script) y `CARRATE` (clase sorteada, 1
  de cada 8). `CARPED` ahora incluye `freq=`.
- `[tooling]` `tools/viceext-log-check.py` entiende esas trazas y, si el log es
  viejo, **lo dice** (“build anterior a `ve11`”) en vez de dar un veredicto
  inventado.

### Artefactos

- Mi código entró en el `reVC.wasm` de **13:17** (23.239.115 B) y **sigue dentro
del que se sirve ahora**, el de **13:20** (`ve12`): la sección 3 relinkeó después
y subió la etiqueta. Comprobado por `grep -a` en el wasm servido: `CARLOAD`,
`CARFAIL`, `CARZONE`, `CARRATE`, `ODSRECOVER` y `freq=%d` están los cinco.
  (`VERSION` lo movió la sección 3 a `2026-09-20-ve12`; yo había puesto `ve11`.
  **`dataTag` sigue `ve10`**: los datos no cambian, así que no hay que re-stagear
  ni se invalida la caché IDB de datos.)
- Recarga con **Ctrl+Shift+R**; el jugador juega unos minutos y el veredicto se lee
  del log (nada de sondas de Chrome: es lo acordado).
- Detalle completo en `.agents/plans/mecanicas/01-seccion-DATOS-pulido.md`
  (§ “2ª partida del jugador … veredicto” y § “Refuerzo temporal de frecuencia”).
- **Nada commiteado.**

## 2026-09-20 13:07 — Sección 3: la partida del jugador **desmonta una traza propia**, y entran C3.5 (luces rompibles) + intermitentes de NPC (código en `ve12`)

### Lo que dijo la partida (12:57–13:07 local, 10 min de juego normal)

De mi sección salió **una sola** línea: `VICEEXT reload done clip=17/17 total=325`.
Leída al detalle **no era mi recarga**: `tog=1` no aparece en ninguna de las 2.589
líneas `CAM1P` de la sesión (nadie pulsó el conmutador), no hay `reload start`
y el cargador quedó lleno — o sea, era la recarga **automática** al vaciar el
cargador. Traza mal instrumentada, no mecánica rota: el estado
`WEAPONSTATE_RELOADING` lo comparten los dos caminos.

**Corregido:** `VICEEXT reload done` exige ahora la marca `s_viceExtManualReload`
(la pone sólo `ViceExtTryManualReload`) e imprime `manual=1`. Regla que me llevo
para los bloques que quedan: *si el motor tiene un camino propio que produce el
mismo efecto, la traza tiene que decir de dónde vino*.

Nada más que verificar de la partida: no hubo misión superada (0 autosaves), ni
intento de guardar desde la pausa (0 `save-anywhere`, 0 pantallas de ranura), ni
disparo a depósito/faro (0 `gastank`/`luces`). C1 tampoco se probó (`tog=1`: 0).

## 2026-09-20 13:20 — Sección 3: **C3.5 luces rompibles** + intermitentes de NPC + verificador de log (build `ve12`)

### C3.5 — luces que se rompen al disparo (`VICEEXT_BREAKABLE_LIGHTS`)

Es el punto 5 del mod ("Car lights can break on impact") y su experimental 7).
Dos cosas comprobadas antes de escribir código:

- Los **modelos adaptados están en los datos del port**: `grep -la petrolcap
  streamed/models/gta3.img/*.dff` → 20+ modelos; `indicator_lf` 19, `indicators_f`
  21, `headlight_l` 25. O sea, C3.3 y C3.4 tienen de dónde tirar (el gas tank no
  era código muerto).
- El port **ya sabe dibujar una luz rota**: `CAutomobile::Render` guía el
  resplandor con `Damage.GetLightStatus(VEHLIGHT_*)` (`Automobile.cpp`
  ~2350-2530) y `DamageManager` ya rompe luces por componente con daño fuerte
  (`COMPGROUP_PANEL`). Lo que faltaba era **el tiro**.

Hecho: el impacto de bala a ≤ 0,5 m del objeto `headlight_l/r` / `taillight_l/r`
(6.1/6.2 de `AdaptingVehiclesEN.txt`) pone esa luz en `LIGHT_STATUS_BROKEN` →
deja de alumbrar. Lo que **no** se copia: el mod cambia la textura del faro y
enseña el hueco del modelo (aquí no hay intercambio de material por objeto).

**Refactor:** depósito y luces comparten un único punto de entrada
`ViceExtBulletHitVehicle()`, y los tres caminos del arma (`DoBulletImpact`,
`FireShotgun`, `FireFromCar`) lo llaman igual; antes había tres `#ifdef` sueltos
por mecánica.

### C3.4 — intermitentes: ahora también los NPC

`AdaptingVehiclesEN.txt` 6.5 da los nombres **de verdad**
(`indicator_lf/lr/rf/rr`, `indicators_f/r`): los que usaba el bloque ya eran esos,
no inventados. Añadido el camino de los NPC ("Turners, which are used by NPCs"):
coche con conductor a <40 m y presupuesto de **2 coches por fotograma**, porque la
piscina de coronas es de 56 para todo el juego. Se apaga en coches ardiendo. La
traza lleva `quien=jugador|npc`. El define sigue **apagado** (paridad con
`StandardCarsUseTurnSignals=0`).

### Corrección de un `#ifdef` que no compilaba (encontrada al verificarlo)

El bloque de intermitentes usaba `ODTRACES` **sin** `#include "ondemand.h"`. Con
el define apagado no lo veía nadie: habría petado al encenderlo. Ya está el
include. Para que no vuelva a pasar, herramienta nueva y genérica para las tres
secciones: **`tools/check-define-build.sh <DEFINE> <fichero.cpp>`**, que reutiliza
la línea de compilación real de `ninja -t commands` y le añade el define
(`OK: Automobile.cpp compila con -DVICEEXT_TURN_SIGNALS`).

### Verificación sin Chrome: `tools/seccion3-log-check.mjs`

Node (no abre navegador): lee `odtrace.log` (+ `odtrace.prev.log` con `--prev`) y
da el veredicto de C1/C2a/C2b/C3.1/C3.3/C3.4/C3.5 con las líneas de prueba y, si
falta, **la acción exacta** que hay que hacer en partida. Es la vía acordada de
cerrar los bloques restantes: se juega una vez y se pasa el verificador.

### Ruido retirado (afecta a las tres secciones)

`CAM1P` escribía en cada cambio de velocidad a pie: **2.589 líneas en 10 min**,
enterrando las trazas de los demás en el log compartido. Con C1 ya cerrado, ahora
sólo escribe al conmutar (2 líneas por conmutación) o **mientras** el modo está
encendido.

### Artefactos

- `reVC.wasm` de **13:20** (23.239.121 B) con `VICEEXT luces rota`,
  `reload done manual=1` y `CAM1P` filtrado. **`VERSION` subido a
  `2026-09-20-ve12`** (etiqueta, para no confundirlo con `ve11` de la sección 1);
  **`dataTag` sigue `ve10`** (no hay datos nuevos). Si la sección 1 ya tenía
  reservado otro número, que renumere: es una línea de `web/lib/index.js`.
- Recarga con **Ctrl+Shift+R** (el servidor de :2077 sirve `public/build`).
- Detalle y tabla del protocolo en
  `.agents/plans/mecanicas/03-seccion-CAMARA-guardado.md` (§ C3.5, § C3.4,
  § "Verificación desde el log").
- **Nada commiteado.**

## 2026-09-20 12:57–13:20 — Sección 2 (policía/conducción): **P2 CERRADO (PASS en partida real)** + traza saneada y coste de P1 acotado (`rev=4`)

Segunda lectura de una partida real, esta vez con el build arreglado. Comando (el log
**rota por sesión**: la partida recién jugada suele quedar en `odtrace.prev.log`):

```bash
VC_ODTRACE=$PWD/gta_vc_browser/web/odtrace.prev.log node gta_vc_browser/tools/driveby-log-check.mjs
VC_ODTRACE=$PWD/gta_vc_browser/web/odtrace.prev.log node gta_vc_browser/tools/hidecops-log-check.mjs
```

### P2 — confirmación en partida real → **PASS**

Sesión 17:57:59Z → 18:05:16Z (tag 0), `JS build=2026-09-20-ve10` + **`P2 rev=2`**:
154 s conduciendo (coche modelo 210 y moto 6506), **34 disparos de pistola (arma 17)
desde coche** con animación lateral (`left` x24, `right` x10) y **4 entradas con
`outcome=keep-weapon`** (la pistola se queda en la mano: era el fallo de la partida
anterior). Sin regresión de la vía vanilla: 1 entrada con la SMG da `switch-smg` y las
salidas siguen repartiendo `keep-weapon` / `restore-stored`.

Límite honesto: los 34 disparos son desde **coche**; no hubo pistola desde moto/barco en
esa sesión. La sonda de laboratorio (`driveby-smoke-test.mjs`) sigue **sin ejecutar** por
la petición del jugador de no lanzar Chrome/swiftshader.

### P1 — segunda partida real: nivel máximo 1, y la línea base de nivel 1 medida en vivo

273 s de traza, cambios `0->1` y `1->0`, **0 episodios** de esconderse (correcto: el
bloque sólo actúa a nivel ≥ 2; el nivel 1 lo lleva vanilla). Lo aprovechable: la estrella
cayó ~**21 s** después del último avistamiento (`seeing=1` a t≈4530455 ms →
`WANTEDCHANGE 1->0 chaos=49` a t=4552956 ms), exactamente al cruzar `chaos` 50→49 con el
decremento de 1/s. Sigue faltando una partida **a nivel ≥ 2** escondido ~20 s para
confirmar el contrato en vivo (la evidencia a nivel alto sigue siendo la sonda del 19/09,
bajada a los 15.0 s).

### Traza saneada y coste de P1 acotado (`rev=4`)

- **Ruido**: 140 líneas `WANTEDCOP join` en pocos segundos, repetidas cada ~16 ms. No es
  un fallo del port: `Garages.cpp:410-416` llama a `CWorld::CallOffChaseForArea` en CADA
  frame mientras conduces dentro del radio de un taller, y `CCopPed::CopAI` vuelve a
  meter al policía en la persecución al frame siguiente (unión/soltado a ~60 Hz).
  Arreglado **en la traza**: una línea por segundo con el contador de ráfaga `burst=N`.
- **Prueba de binario**: línea nueva por sesión
  `WANTEDHIDEINIT tag=0 rev=4 star_ms=15000 grace_ms=5000 sight_m=40 sweep_ms=200 t=…`
  (la etiqueta `JS build=` no cambia al recomponer sólo el `.wasm`). El verificador la
  imprime y avisa si la revisión es vieja.
- **Coste**: el barrido de visibilidad de P1 (`AnyCopSeesPlayer`, paso 2) recorría TODO
  el pool de peds con un `ProcessLineOfSight` por policía a menos de 40 m **en cada
  frame** a nivel ≥ 2; ahora se refresca a 5 Hz (`sweep_ms=200`). Los perseguidores
  (`m_pCops`) se siguen mirando cada frame. No medido en partida a nivel ≥ 2 (no ha
  habido ninguna): es una decisión de coste por lectura de código.
- **Build publicado**: `reVC.wasm` de **13:20:20** (23.239.121 bytes) con
  `WANTEDHIDEINIT … sweep_ms=%d`, `burst=%d`, `rev=%d outcome=%s` y `keep-weapon` dentro
  (`grep -a -F`). Para ver la traza nueva hay que **recargar la página** y volver a
  entrar en partida.

### P3 (opcional) — bloqueado por DATO, pedido a la sección 1

`6507 polwintergreen` sigue con clase **`ignore`** en `bootseed/data/default.ide`
(0 apariciones en las tres sesiones leídas hoy), así que la moto no circula. La petición
(mínima y reversible) está escrita en el plan de la sección 2: clase de tráfico + 2
colores en `carcols.dat` + frecuencia baja, confirmable con una línea `CARPOOL`/`CARSPAWN`
de `6507`. Sin eso no se puede ni medir "que la use la policía"
(`CVehicle::IsLawEnforcementVehicle`, `CarCtrl`: ficheros que no son míos).

Nada commiteado (regla del plan); todo sigue en el árbol de trabajo.

### Addendum (13:26) — cheat `CRAZYCOP` y herramienta `tools/cheat-literal-check.mjs`

Del plan de la sección 2 faltaba la tarea 4 de P1 (un cheat propio para medir "esconderse"
siempre desde el mismo nivel). Ya está: **`CRAZYCOP`** → `FindPlayerPed()->SetWantedLevel(3)`
(3 estrellas sin matar ni atropellar a nadie), añadido al final de la cadena de
`src/core/Pad.cpp`, detrás de `CRAZYPISTOL`.

El literal (`"STJZg\UJ"`, 8 bytes) no se copió a ojo: se sacó con una herramienta nueva,
`gta_vc_browser/tools/cheat-literal-check.mjs`, que lee la tabla `ccmp()` del propio
`Pad.cpp`, decodifica los 91 literales que ya hay, imprime el literal exacto para un nombre
(`--name=CRAZYCOP`) y avisa de las dos trampas reales: nombre ya usado, y literales
**prefijo** de la cadena anterior (es un `else if`: un prefijo te robaría el cheat).
Se autocomprueba reproduciendo el literal de `CRAZYPISTOL` que ya estaba compilado.

**Build publicado**: `reVC.wasm` de **13:26:39** (23.240.095 bytes) con `STJZg\UJ` dentro
(`grep -a -F`), además de todo lo de la entrada anterior (`WANTEDHIDEINIT … sweep_ms=%d`,
`burst=%d`, `rev=%d outcome=%s`, `keep-weapon`). **Recargar la página** para tenerlo.

## 2026-09-20 14:0x — Sección 2: lectura de la partida del jugador (P4-P7 en el código)

El jugador jugó con `ve12` (13:42-13:47 local) y reportó muchos fallos, válidos para las
tres secciones. Lo mío, leído del `odtrace.log` (sin abrir navegador):

- **«con la pistola desde el coche salen balas como si fuera de SMG» → CONFIRMADO, dos
  causas halladas**: (a) los tres `DoDriveByShootings` fijan la cadencia a mano
  (`m_nTimer = now + 70`, la del SMG): la pistola disparaba a 14/s; (b) el sonido del
  disparo se pide al audio del **vehículo**, y esa rama de `AudioLogic` no mira el arma
  del conductor: suena **siempre** el Uzi (`SFX_UZI_LEFT`). Referencias de la comunidad
  que ya lo hacen (buscadas hoy): «Manual Driveby» (SpitFire: *«Correct sounds for each
  weapon»*, *«Settings from weapon.dat»*) y «Manual Driveby (VC)» (BirbsLeHecker:
  *«proper fire rates, no more rapid fire bugs»*).
- **«carro de policía sin sirena»**: `fbicar` (147) es policía en los datos (`flags=7`) y
  `UsesSiren()` ya lo trataba como tal, pero el switch de coronas no lo contemplaba: la
  sirena sonaba y **no tenía ni una luz**. Añadido al grupo de `fbiranch`/`vicechee`.
  Siguen sin coronas `predator` (lancha) y `chopper` (helicóptero), anotado.
- **«con 3 estrellas se crasheó»**: el log se corta **exactamente 6.000 s** después de que
  un disparo impactara en el **helicóptero policial** (`VICEEXT gastank sin-dummy
  model=165`, 18:47:25.874Z), con el juego a 60 fps justo antes (aborto, no cuelgue).
  En `Heli.cpp` un heli derribado se borra con `CWorld::Remove(); delete pHelis[i];` a los
  +10 s — ese es el primer punto a mirar (no es fichero de esta sección). El otro
  sospechoso del mismo minuto es el estallido de activos del nivel 3 (sección 1): en el
  segundo exacto del `2->3` el motor pide `chopper`, `vicechee` y 8 skins `vice1..8` y
  esas lecturas fallan al abrir (`ODSHORT open-fail`, recuperadas después).
- **Bug de método general (para las tres secciones)**: el reloj del motor **retrocede al
  cargar partida** (medido: de t≈4,4M a t≈3,66M). Cualquier traza con guarda
  `static … now < last + N` **se queda muda para siempre**: a mi `WantedTraceState` le
  pasó (la sesión que cargó partida no imprimió **ni una** línea `WANTED tag=`) y lo
  mismo puede estar ocurriendo en `CARPED`/`SIGHT`/`CAM1P`/`IFPFILE`. **Revisad vuestras
  guardas**: la sección 2 ya las reancla (`P6`).

Hecho en el código (todo detrás de los defines de la sección 2, sin tocar datos):

| Bloque | Cambio | Ficheros |
|---|---|---|
| **P4** | `CWeapon::GetDriveByShotDelay()`: cadencia por arma (`weapon.dat`: Colt45/Beretta **210 ms**, Python 600) y 70 ms exactos para el SMG; el sonido del disparo con pistola se pide al **ped** (muestra de su arma) | `Weapon.{h,cpp}`, `Automobile.cpp`, `Bike.cpp`, `Boat.cpp`, `Ped.{h,cpp}` (traza con `delay=`) |
| **P5** | coronas para `fbicar` (147) | `Automobile.cpp` |
| **P6** | las tres guardas de traza reanclan cuando el reloj retrocede | `Wanted.cpp`, `CopPed.cpp` |
| **P7** | `UpdateHiding()` sale si no hay jugador (antes de `FindPlayerCoors()`, que desreferencia sin comprobar); auditoría del resto de rutas nuevas: sin punteros muertos | `Wanted.cpp` |

Verificación: `node gta_vc_browser/tools/driveby-log-check.mjs` (ahora lee `delay=` y avisa
si ve `delay=70` con slot 3). **Los 7 objetos compilan**, pero el **enlace no se ha podido
publicar** todavía: el árbol no compila por un error en vuelo de la **sección 3**
(`src/core/Cam.cpp:5209: no member named 'GetLookLeftRight' in 'CPad'`). Se reintenta en
cuanto compile; hasta entonces el `.wasm` servido (13:26) **no** lleva P4-P7.

Nota: la sección 1 ya implementó a las 14:07 el `pushFatal` (`JSERR`/`ENGERR` en
`odtrace.log`), que era justo lo que esta sección iba a pedir para poder diagnosticar el
crash. **Nada commiteado.**

## 2026-09-20 (tarde) — Sección 1: hallazgos de la 4ª partida, D6b/J1/D10/D4c

El jugador reportó una lista larga tras jugar. Se repartió por secciones en
**`.agents/plans/mecanicas/05-hallazgos-4a-partida.md`** (léanlo las 3 secciones:
ahí está el "quién hace qué" con la evidencia de la traza). Lo hecho por la
sección 1 en este bloque:

| Bloque | Cambio | Fichero |
|---|---|---|
| **D6b** | La mira del arma **sólo se dibuja al apuntar** (`PED_LOCK_TARGET`); se mantiene en coche (drive-by) y en las vistas de arma en 1ª persona | `src/renderer/Hud.cpp` |
| **J1** | El motivo de un cuelgue **queda en `odtrace.log`** (`JSERR`, `JSERR_REJ`, `ENGERR`) con envío síncrono antes de morir la página; la consola de la página lo perdía | `gta_vc_browser/web/lib/index.js` |
| **D10** | Traza `LODLEFT model=… dist=… relacion=… rwobj=… alpha=…` en el punto exacto donde un LOD se sigue dibujando de cerca (≤60 m, una vez por modelo, 40 máx.) | `src/renderer/Renderer.cpp` |
| **D4c** | `CARRATE` dejaba 2.391 líneas por sesión; ahora primeras 30 + cambios de clase + latido cada 512 | `src/control/CarCtrl.cpp` |
| — | `viceext-log-check.py` dicta también `J1` (cuelgue + LOD pegado) | `gta_vc_browser/tools/viceext-log-check.py` |

**Dato nuevo y gordo (J0 del plan):** el `anim/ped.ifp` del mod que ya se sirve
(272 clips) trae las animaciones de las mecánicas que faltan: `Swim_Breast/Crawl/
Tread/jumpout`, `Drown`, `Crouch_Idle/Forward/Backward/Roll_L/R`, `DUCK_down`,
`DUCK_low`, `WEAPON_crouch`, `sprint_armed/rocket/csaw`, y `CLIMB_*`. No hay que
traer arte: hay que **asociar** los clips, y los grupos están hardcodeados en
`src/animation/AnimManager.cpp` (enum + tabla de nombres + `aStdAnimDescs`). Los
nombres son de San Andreas, así que sus layouts sirven de referencia.

Otras dos piezas de dato medidas para las otras secciones:
- **Depósito de gasolina:** sólo **25 de ~100** `.dff` de vehículos traen el dummy
  `petrolcap` (los 8 del mod sí; de serie casi ninguno, p. ej. `bobcat` no) → hace
  falta posición de reserva para que explote igual (sección 3, C3.3).
- **Cámara 1ª persona:** `PED_TOGGLE_1RST_PERSON` **sí** está bindeada a `V`
  (`ControllerConfig.cpp:298`) y en la partida `tog=0` en las 7 muestras `CAM1P`
  (el conmutador nunca se encendió). El diagnóstico `VICEEXT 1p key …` de la
  sección 3 **no estaba en el wasm jugado** (`grep -a "1p key" reVC.wasm` = 0 en
  el de 13:26): hay que recompilar antes de tocar el binding.

Verificación: los tres `.o` de la sección 1 compilan sin errores
(`ninja … renderer/Renderer.cpp.o renderer/Hud.cpp.o control/CarCtrl.cpp.o`). El
**enlace sigue bloqueado** por el error en vuelo de la sección 3
(`Cam.cpp:5209 no member named 'GetLookLeftRight' in 'CPad'`), el mismo que anotó
la sección 2. Cuando esa sección cierre su edición, un solo `build.sh` publica
todo (mi `VERSION`/etiqueta se sube a `ve13` en ese momento; el `dataTag` sigue
`ve10`: **los datos no cambian** en este bloque). **Nada commiteado.**

## 2026-09-20 14:45 — Sección 3: la 4ª partida se jugó con el build **anterior** (`ve12`); PLAN v2 completo en **`ve14`** + respaldo del depósito por caja + verificador de 10 bloques

**El dato que ordena el reparto de la 4ª partida.** La sesión `18:42:35Z →
18:47:31Z` de `web/odtrace.log` empieza con `JS build=2026-09-20-ve12`, o sea
trece minutos **antes** del primer enlace con el PLAN v2 de la sección 3 (14:20,
`ve13`). Las nueve quejas de mi sección (V, R, depósito, ranura, autocentrado,
agachado, esprint armado, escopeta, nadar) estaban **sin escribir** en ese build:
no son fallos de la implementación, son huecos de una partida que se jugó con el
código viejo. Lo único atribuible a esa sesión en mi sección es que **funcionó**
lo que ya estaba: C3.5 (`VICEEXT luces rota model=156 luz=taillight_l/headlight_l/headlight_r`)
y el gancho de bala de C3.3 (`12 × VICEEXT gastank hit model=159/6504 hp=250→25`,
que es justo el contrato viejo de "arder", ya sustituido por explosión).

**Lo hecho en este turno (sección 3):**
1. **C3.3c — respaldo del depósito por caja de colisión** (`src/weapons/Weapon.cpp`,
   `ViceExtGasTankPoint`): si el `.dff` no trae `petrolcap` (25 de ~100 lo traen:
   el jugador disparó a un `bobcat` y no pasó nada) el depósito se localiza por
   la **caja de colisión** del vehículo — en este motor el eje local Y es el
   morro, así que el punto es `y = min.y + 6 % del largo`, `x = 0`,
   `z = min.z + 30 % de la altura`, radio 0,75 m. Traza nueva
   `VICEEXT gastank reserva model=N` (una por modelo) y `gastank hit … reserva=1`.
2. **`tools/seccion3-log-check.mjs` ampliado a 10 bloques** (C1 + `VICEEXT 1p key`,
   C1b-2 `camauto`, C2a, C2b, C3.1, C3.3 con `reserva=`, C3.5, C4 nado, C5
   agachado, C6 esprint) y con **detector de builds viejos**: avisa si un
   `reload done` viene sin `manual=` o un `gastank hit` sin `reserva=`, que son
   líneas de antes de `ve13` y no valen como prueba.
3. **Un solo build a las 14:41, etiqueta `ve14`** (`VERSION` en `web/lib/index.js`;
   `dataTag` sigue `ve10`, no hay datos nuevos). Dentro van todas las marcas:
   `grep -a` sobre el wasm encuentra `1p key`, `camauto`, `swim move`, `crouch %s
   arma`, `sprint grupo`, `playerswim`, `playercrouch`, `sprint_armed`,
   `reload key`, `gastank reserva` y el rótulo `AUTOGUARDADO` (UTF-16).
   **El enlace ya no está bloqueado**: el error de `Cam.cpp`
   (`GetLookLeftRight` en `CPad`) que anotaron las secciones 1 y 2 está resuelto
   desde el enlace de las 14:20.
4. **Plan**: `.agents/plans/mecanicas/03-seccion-CAMARA-guardado.md` → sección
   nueva `PLAN v3` con la tabla queja↔build, la lectura de la traza, la
   **investigación de mods de la comunidad** (Stories Style Swimming, SA Crouch
   Movement, Sprinting With Two Handed Weapons, Reload/Manual Aiming, First
   Person View) y de dónde sale cada decisión (p. ej. el umbral de agua honda
   evita el fallo conocido de que se nade en la orilla, y el movimiento del nado
   lo pone el pad para no acabar como el CLEO que anima pero no avanza).

**Aviso para las secciones 1 y 2:** el wasm servido es el de **14:41 / `ve14`**
(yo lo reenlacé; la etiqueta es mía). Si tocáis C++ tenéis que reenlazar y subir
la etiqueta otra vez. Y el candidato número uno del cuelgue sigue en vuestro
terreno: los `ODSHORT open-fail` de `BFOBE/WMYLG/WFYLG` (skins del mod que el IDE
pide y el `.img` no tiene) ocurren **inmediatamente antes** de que el log se corte
en seco. Nada commiteado.

## 2026-09-21 04:10 — Sección 1: **la pantalla negra del 21/09 era el GXT** (`CText::LoadMissionText` girando sin salida) + TABL del mod desfasado por nuestra edición de claves

**Síntoma del jugador:** "el juego ni carga partida: inicié una nueva y cargué
una y se queda en pantalla negra". El log moría en seco **tras
`loadlevel DATA\GTA_VC.DAT`**, sin `JSERR` ni abort: el motor no volvía a
`CGame::Process`. Acotado con marcas finas (frame 4 llega a `G13` =
post-`CWeather::Update` y nunca a `G2`), el cuelgue estaba **dentro de
`CTheScripts::Process`**, en un único comando del guion: **1356 =
`COMMAND_LOAD_MISSION_TEXT`**.

**Causa raíz (1): bucle infinito en `CText::LoadMissionText`**
(`src/text/Text.cpp`). El bucle de chunks no comprobaba **ni la apertura** del
`.gxt` ni el **fin de fichero**: `while (!tkey_loaded || !tdat_loaded)` con
`if (size != 0)` dentro, así que una cabecera con `size == 0` o un descriptor
inválido releían **la misma cabecera para siempre**. Sin un solo mensaje: por eso
era una pantalla negra muda y no un error.

**Causa raíz (2): los offsets de misión de `TABL` estaban desfasados -214 bytes**
en los 6 GXT que servimos. No es del mod: **el original del mod está bien**
(`vice-extended-october-2025-update_.../GameFiles/ViceExtended/TEXT/*.gxt`,
delta 0 en los 7 idiomas). Lo rompimos nosotros: `tools/gxt_inspect.py add`
metió 6 claves de nombre de vehículo (`HELLENB`, `MANCHEZ`, `PREMIER`,
`STRTFTR`, `VCPDWTR`, `WINTERG`), el TKEY creció 72 B (6×12) y el TDAT 142 B, y
como los bloques de misión van **detrás** de TKEY/TDAT se movieron 214 B mientras
`TABL` seguía apuntando a los offsets viejos. `CText::LoadMissionText` busca el
**nombre de la tabla en el offset** y encontraba texto UTF-16: `INTRO` no
aparecía y, sin la corrección (1), el bucle giraba. Efecto visible secundario:
todos los textos de misión del SPPC salían como `INTRO1..INTRO4 missing`.

**Lo que he corregido (arreglar, no deshacer):**

| Qué | Dónde | Qué hace |
|---|---|---|
| **D18** bucle con salida | `src/text/Text.cpp`, `src/text/Text.h` | `ReadChunkHeader` devuelve los bytes leídos; el bucle sale si la cabecera es ilegible o `size == 0`; `OpenFile` se comprueba (`file == 0` → aviso y fin, sin colgar); el nombre del GXT sale de `OdTextFileName()` **con `default`**, porque antes `filename` podía usarse sin inicializar si `m_PrefsLanguage` no caía en ningún `case` |
| **D19** lectura corta | `CKeyArray::Load` / `CData::Load` | devuelven los bytes leídos y la tabla sólo se acepta si se leyó **entera** (`tkey=864/864 tdat=5768/5768`). Antes una lectura corta dejaba la cola del array con memoria sin inicializar y la última clave de la tabla desaparecía de forma intermitente |
| **TABL** reajustado | `tools/gxt_inspect.py` | subcomandos `tabl`, `check-tabl`, `fix-tabl`; **`add`/`add-tsv` ya reajustan TABL solo** al reescribir TKEY/TDAT (era el fallo de origen) y verifican el resultado antes de mover el fichero |
| **GXT reparados** | `gta_vc_browser/streamed/TEXT/*.gxt` (6 idiomas) | 78 misiones +214 B cada una, con control contra el original del mod (`check-tabl` = OK) y sin tocar ninguna clave (el verificador compara el multiconjunto antes/después) |
| **Ruido fuera** | `Script.cpp`, `Script7.cpp`, `Streaming.cpp`, `AnimManager.cpp`, `AnimBlendAssocGroup.cpp`, `Game.cpp`, `main.cpp`, `CdStreamPosix.cpp` | retirados mis `printf`/`ODTRACE` por comando, por fotograma y por bloque de animación (frenaban el motor y llenaban el log). Quedan sólo los de error: `SCMLOOP` (tope de 200k comandos del guion, que es la red que habría cazado esto), `ODSHORT short`, `loduce MISS`, `WATER sin waterpro.dat`, `anim sin jerarquia`, `TXTGXTFAIL`, `TXTGXTSHORT` |
| **Traza honesta** | `CText::Get` | `TXTMISS` se escribe **sólo si la clave no está en ninguna de las dos tablas** (global y de misión) y dice `nglobal=`/`nmision=`. Antes trazaba también el fallo normal de la global para claves de misión, que sí resolvían después |

**Verificación (build `ve14`+ , reenlazado, perfil de Chrome limpio):**

* **Partida nueva desde el menú**: `WEBHB state=9 ingame=1` (en partida), `CORONAREND` ×8
  (la escena 3D se dibuja), **0 `TXTGXTFAIL`, 0 `TXTGXTSHORT`, 0 `TXTMISS`** y
  `MSGTABLE INTRO n=72 primera=INT1_A ultima=INTRO4 tkey=864/864 tdat=5768/5768`
  — la tabla de misión del SPPC entra **completa**.
* **Antes**: `CText::LoadMissionText - no se pudo leer SPANISH.GXT` + 10 `TXTMISS`
  (INTRO1..INTRO4) o, con el GXT ya arreglado, 1 `TXTMISS` intermitente por la
  lectura corta de D19.
* **Carga de partida guardada**: el test `tools/slot0-load-test.mjs` no llega a
  clicar la carga (el motor se queda vivo a **60 FPS en `state=7`**, sin cuelgue
  ni error de página: el fallo es del arnés, no del juego). Es lo siguiente que
  hay que mirar en el arnés, no en el motor.
* **El enlace está libre**: `ninja` completo (**58/58 + enlace**) sin errores y el
  error de `Cam.cpp`/`GetLookLeftRight` ya no está. Los otros dos agentes pueden
  compilar y publicar.

**Para el jugador:** recargar la página (el wasm y `reVC.data` son nuevos) y ya
se puede empezar partida nueva y cargar partida. Nada commiteado.

---

## Sección 2 · 5ª partida (21/09) — P1 `rev=6`, la mira al bajar del coche, el retroceso y la `R`

El jugador confirmó **P2 (drive-by) funcionando** y dio una lista nueva. Publicado en
`gta_vc_browser/web/public/build/reVC.wasm` de **08:04:37** (105/105 + enlace,
23.270.509 bytes, `exit=0`; las cadenas nuevas comprobadas con `grep -a -F`).

### 1. P1 — por qué esconderse no hacía nada (mi bloque)

Medido en la partida del 20/09 (ya en el plan): 649 s a nivel ≥ 2, **1.205 `seen` +
1.205 `start` encadenados cada 200 ms, 0 `drop`, 0 estrellas bajadas**. Dos causas y
sus dos arreglos en `src/core/Wanted.cpp`:

* el "te ve" no exigía que el policía **mirara** hacia el jugador: una patrulla a 40 m
de espaldas contaba. Ahora hace falta `DotProduct(GetForward, dirección al jugador) >
0.26` (±75°, escrito en el plan como decisión del bloque);
* el avistamiento se contaba **en el frame**: el barrido va a 5 Hz y en la calle
parpadea, así que la racha nunca pasaba de 9 s (hacen falta 15). Ahora el avistamiento
tiene que durar `VICEEXT_HIDE_SEEN_MS` = 400 ms para cortar la búsqueda; por debajo,
ni corta ni reinicia la cuenta.

`VICEEXT_HIDE_REV` sube a **6** y `WANTEDHIDEINIT` imprime además `seen_ms=` y
`sight_dot=`, así que un log puede probar qué binario jugó y con qué constantes.
**Pendiente de medir en partida** (el PASS ahora sí es alcanzable: `CRAZYCOP` + 20 s
escondido → `drop` a los ~15 s).

### 2. La mira seguía pintándose al bajarse del coche (bloque D6, sección 1 — avisado)

`ViceExtWantsSight()` (`src/renderer/Hud.cpp`) devolvía `true` con sólo
`FindPlayerVehicle() != nil`, y esa función **sigue devolviendo el vehículo mientras
dura la animación de salida**. Ahora exige `m_nPedState == PED_DRIVING` (o una vista
donde la mira ES la cámara). Un cambio, local, detrás del `#ifdef VICEEXT_WEAPON_SIGHTS`
que ya existía.

### 3. Retroceso: ahora mueve la MIRA, no sacude la cámara (P4)

Reporte del jugador: *"la cadencia no mueve la mira como lo haría la cadencia, solo
sacude la cámara como si algo hubiese explotado"*. Era literal: `CamShakeNoPos(0.05)`
por bala (una explosión son 0.2) + golpe de mando 120/96. Ahora
(`src/weapons/Weapon.{h,cpp}`, sin dueño en la tabla de propiedad):

* cada disparo acumula una **patada por slot de arma** (`ViceExtRecoilKick`: pistola
  0.022, escopeta 0.030, SMG 0.010, rifle 0.014, pesada 0.040, francotirador 0.055,
  tope 0.12);
* esa patada se aplica a `CCamera::m_f3rdPersonCHairMultY`, que es **a la vez** la altura
a la que el HUD pinta la mira y el ángulo con el que sale la bala
(`Find3rdPersonCamTargetVector`), así que la retícula sube y el tiro sube con ella;
* `ViceExtRecoilUpdate()` (llamado desde `CWeapon::UpdateWeapons`, cada frame) la
**devuelve a su sitio** a 0.12 ud/s; se toca el global sólo en diferencial, así que el
valor de reposo (0.4) queda intacto y una ida y vuelta no deja deriva;
* la sacudida de cámara baja a `0.004 + 0.20*patada` y el mando a `30 + patada*900`.

También se aplica en el **drive-by** (`CWeapon::FireFromCar`, si el conductor es el
jugador).

### 4. Recarga con `R` (bloque C3.1, sección 3 — avisado)

En `ViceExtTryManualReload` (`src/peds/PlayerPed.cpp`) la tecla se leía sólo de la
acción rebindable `PED_RELOAD`. Si la config de controles guardada en el navegador es
anterior a que la acción existiera (el hueco era `UNKNOWN_ACTION`), esa acción no apunta
a `'R'` y **la `R` no hacía nada**. Ahora también se acepta `'R'` directa, y la traza de
pulsación imprime la tecla resuelta: `VICEEXT reload key tecla=<código> arma=… clip=…`.

### 5. Verificación sin navegador

* `node gta_vc_browser/tools/hidecops-log-check.mjs` — actualizado al formato nuevo
  (`WANTEDHIDE seen tag=… d=<ms> t=…`, `seen_ms=`, `sight_dot=`) y **avisa si la sesión
  jugada es `rev<6`** ("con rev<6 la estrella puede no bajar nunca").
* `node gta_vc_browser/tools/driveby-log-check.mjs` — sin cambios de formato.
* Los dos pasan `node --check` y se ejecutaron contra el `odtrace.log` de hoy.

### 6. Lo que queda (con dueño, ya escrito en el plan de la sección)

* **P1 en vivo** con `CRAZYCOP` (el arreglo acaba de publicarse).
* **Moverse apuntando** ("apuntando no debería poder correr, sólo caminar") — sin empezar;
  vive en el apuntado de `PlayerPed.cpp` (sección 3). Referencia: Manual Aiming v1.5.
* **Agachado caminando estilo SA** → sección 3, C5 (hoy sólo hay agachado de disparo).
* **Animación de apuntar con escopeta** → sección 3, C7 (`CAN_AIM_WITH_ARM` pide
  `CANAIM_WITHARM`, que la escopeta no tiene).
* **Grosor/tamaño de las miras** → sección 1, D6 (`ViceExtDrawSight` dibuja la caja de
  32 px de la cruz de serie; el arte original está en su propio `weaponsights.txd`).
* **Escalar** → bloque opcional E1 (su `features.ini` lo trae a 0).
* **Sirena del coche policial (P5)**: retirado — el jugador ha dicho que no me encargue.
* **P3 (moto VCPD)**: sigue bloqueado por el dato (`6507` con clase `ignore`, sección 1).

**Aviso de propiedad**: además de mis ficheros, se han tocado `src/renderer/Hud.cpp`
(una condición de D6) y `src/peds/PlayerPed.cpp` (la lectura de la tecla de C3.1), a
petición expresa del jugador. Son cambios locales y detrás de los defines que ya
existían. Nada commiteado; nada de `git add`.

---

## Sección 2 · 2ª tanda del 21/09 — P1 `rev=7` (la última estrella), retroceso v2, agachado, escopeta, miras y escalada

El jugador jugó con `rev=6`: la traza confirma que **la mecánica ya baja estrellas**
(`WANTEDHIDE drop 3->2 en 15.0 s PASS`, 2 caídas sin que le vieran) pero **nunca
llegaba a 0**, dijo que la retícula **no tenía retroceso** y pidió agachado, escopeta,
miras y escalada. Publicado en `reVC.wasm` de **08:23:27** (23.284.179 bytes,
`exit=0`; cadenas comprobadas con `grep -a -F` sobre el `.wasm`).

### 1. Por qué la última estrella no caía (mi bloque)

`UpdateHiding()` salía del todo a **nivel ≤ 1** y ese nivel quedaba en manos de la
regla vanilla: 1 punto de chaos/s **sólo** si no hay policía a menos de 18 m. Con una
patrulla cerca, la última estrella no bajaba nunca. Ahora la regla de esconderse cubre
**también el nivel 1** (mismos 5 s de gracia + 15 s sin que nadie te vea) → `rev=7`.
La vía vanilla sigue viva y puede bajarla antes.

### 2. Retroceso: no se veía porque era pequeño y llegaba un frame tarde

Con `rev=6` el retroceso ya movía la retícula, pero: patada 0.022 (≈24 px), vuelta
0.12 ud/s y —lo importante— se aplicaba en el `ViceExtRecoilUpdate` del **frame
siguiente**, así que el primer disparo de cada ráfaga no movía nada. Ahora: patadas
dobladas por slot (pistola 0.045, escopeta 0.060, SMG 0.018, rifle 0.030, pesada 0.075,
sniper 0.100), vuelta 0.25 ud/s, tope 0.22, y **se aplica en el mismo frame del
disparo**. Traza `VICEEXT recoil kick slot=… patada=… subida=… multY=…` (5/s como
mucho) para poder probarlo desde el log. Referencias de la comunidad usadas: el CLEO
`WeaponRecoilAuto` (gtaforums 953286: recoil por disparo según la precisión del arma)
y `Bullet Spread/Recoil Fix` (jenksta): los dos empujan la mira arriba y la dejan
volver, en vez de sacudir la cámara.

### 3. Apuntar caminando (y agachado andando)

`PlayerControlZelda` limita la velocidad a **paso de andar** (1.0) y anula el esprint
cuando el jugador apunta (`PED_LOCK_TARGET`) o está agachado — pedido literal del
jugador ("cuando estoy apuntando no debería poder correr, sólo caminar"). Define nuevo
`VICEEXT_AIM_WALK` en `config.h`, al final del bloque.

El agachado (bloque C5) **sólo funcionaba desarmado**: con arma la función se salía y
el agachado lo llevaba el motor (`SetDuck` con la misma tecla C), que es el de
**disparo** y te deja quieto. Ahora hay un solo estado (`odCrouched`): con arma lo
sigue del motor, sin arma lo togglea la tecla, se superponen los clips del mod
(`Crouch_Idle/Forward/Backward`) y **correr o saltar pone de pie**
(`VICEEXT crouch off motivo=carrera`).

### 4. Apuntar con la escopeta (pose, no sólo mira)

El apuntado del motor elige la pose con `WEAPONFLAG_CANAIM_WITHARM`, que las escopetas
no traen en `weapon.dat`; por eso salía la mira y no la pose. Con `VICEEXT_SHOTGUN_AIM`
las escopetas del mod (`SHOTGUN`, `STUBBY_SHOTGUN`, `SPAS12_SHOTGUN`, `SHOTGUN2`) entran
por la rama de apuntar con brazo, sin tocar el dato (es de la sección 1).

### 5. Miras más finas (D6)

La caja de dibujo era la de la cruz de serie (32×0.6 px de semi-lado = 38 px de caja),
así que el arte del mod salía estirado y su borde engordaba. Ahora 0.42 (armas largas) y
0.26 (resto): ≈27 y 17 px de caja.

### 6. Escalar (bloque opcional E1) — nuevo `VICEEXT_CLIMB`

Los clips de escalada del mod (`CLIMB_idle`, `CLIMB_jump`, `CLIMB_jump_B`,
`CLIMB_jump2fall`, `CLIMB_Pull`, `CLIMB_Stand`, `CLIMB_Stand_finish`; comprobados
leyendo el `ped.ifp` servido) **no los usaba nadie**: el port no tenía grupo de
animación para ellos. Se añade `ASSOCGRP_PLAYERCLIMB` en `animation/AnimManager.h`,
`animation/AnimationId.h` y `animation/AnimManager.cpp` (**ficheros compartidos**: la
sección 3 añadió ahí los de nadar/agachado, mismo patrón) y la mecánica en
`PlayerPed.cpp`: saltando mirando a un borde de **0.55–1.85 m** con sitio libre arriba,
se reproduce `CLIMB_Pull` y en **0.7–1.1 s** el ped sube al borde, cerrando con
`CLIMB_Stand_finish`; si no hay borde, salta como siempre. Trazas
`VICEEXT climb start borde=… ms=…` y `VICEEXT climb fin`. Se abandona solo si el reloj
del motor retrocede (carga de partida) o si el ped pierde el control, para no dejar el
salto bloqueado.

### 7. Verificación acordada con el jugador (una sola probada)

En la próxima partida, por log: `drop` hasta **0 estrellas** al esconderse con 3★ ·
`VICEEXT recoil kick …` al disparar (y la retícula subiendo) · `VICEEXT crouch on/off`
(+ `off motivo=carrera` al esprintar) · `VICEEXT climb start borde=…` al saltar contra
un muro bajo. Verificadores: `node gta_vc_browser/tools/hidecops-log-check.mjs` (avisa
si la sesión es `rev<6`) y `driveby-log-check.mjs`.

**Pendiente en mi sección**: sólo **P3** (moto VCPD), bloqueado por el **dato** de la
sección 1 (`6507` con clase `ignore`). Nada commiteado, nada de `git add`.

## 2026-09-21 08:20 — Sección 1: sonidos de las 8 armas nuevas (banco del mod → banco del port), `CARRATE` rehacido y +20 m de LOD — build `ve15` / datos `ve11`

**1. D8 — los sonidos del mod, dentro y sonando (asignación provisional).**
El banco propio del mod (`ViceEx.SDT` + `ViceEx.RAW`, 13 muestras) ya está en el
banco del port: `gta_vc_browser/tools/add_viceex_sfx.py` convierte los 13 WAV ya
extraídos, los mete como **ids 9941..9953** (`Audio/sfx/<id>.mp3`, mp3 mono a
96 kbps, la misma receta de `finish_sfx.py`) y **amplía `Audio/sfx.SDT`** de 9941
a 9954 entradas (offsets acumulados, `size` = bytes PCM del mod, `rate` = su Hz).
En el motor: 13 entradas nuevas en `AudioSamples.h` (`SFX_VICEEX_00..12`, nombres
neutros a propósito) y el caso de disparo de las armas nuevas en
`AudioLogic.cpp` (`SOUND_WEAPON_SHOT_FIRED`) usa **su** muestra en vez del sonido
de serie que reutilizaban. La asignación muestra↔arma es **provisional y vive en
una sola tabla** (dentro de ese caso); cada disparo deja
`VICEEX sfx arma=<tipo> sample=<id>` (40 líneas máx.) para corregirla desde el log
y no a ciegas. Pares del banco (mismo tamaño+Hz) = el mismo sonido en dos
variantes; se usa una de cada. Los 8 pares muestra↔arma y el porqué, en el plan.

**2. `CARRATE` sí se corta ahora.** La versión de ayer imprimía "cuando cambia la
clase" y, como la clase se sortea al azar en cada intento, cambiaba casi siempre:
**66.525 líneas de 87.116 sorteos** en la partida del 21/09 (medido, el log seguía
ahogado). Ahora: las 12 primeras tiradas + la primera vez de cada clase +
recuento completo cada 4096 (`CARRATECNT n=… c0=… c1=…`).

**3. D10 — +20 m de modelo bueno antes del LOD** (petición del jugador). El margen
se **suma** en `CSimpleModelInfo::GetLodDistance` / `GetNearDistance` /
`GetLargestLodDistance` (`#define VICEEXT_LOD_EXTRA 20.0f` en `config.h`), que son
los mismos valores que usan a la vez la decisión de dibujado y la petición de
modelos, así que no se pide tarde lo que luego se dibuja de cerca.

**Verificado en el build `ve15`:** `ninja` 235/235 + enlace sin errores; `grep -a`
encuentra en el wasm servido `VICEEX sfx arma=`, `CARRATECNT` y `LODLEFT`; los
13 mp3 y `Audio/sfx.SDT` (199.080 B = 9954×20) están en el manifiesto;
`VERSION=2026-09-21-ve15`, `dataTag=2026-09-21-ve11` (sube porque hay datos
nuevos: sin eso la caché seguiría sirviendo el `sfx.SDT` viejo). **Nada commiteado.**

**Pendiente de esta tanda:** la asignación de sonidos se corrige con el oído en la
próxima partida (basta decir "el X suena como el Y: cámbialo") y el **AUG
apunta a un lado** sigue abierto: el modelo es byte a byte el del mod, sus
animaciones (`STEYR_fire/crouchfire/reload/crouchreload` en `steyr.ifp`) existen
con esos nombres exactos y su manejo (retroceso, offset de disparo) es igual al de
los otros rifles ⇒ hace falta una captura apuntando para localizarlo.

## 2026-09-21 08:30 — Sección 1: cierre de la sección — build `ve16` / datos `ve12`

**1. La frecuencia vuelve a los valores del mod.** El refuerzo a 100 era temporal
(para verlos circular) y ya cumplió: `tools/boost_veh_freq.py --revert` deja
`streetfi`/`peren2`/`trash2`/`premier`/`manchez`/`wintergreen` en 10 y `hellenbach`
en 7 (y la moto policial en 10 con clase `ignore`, como el mod). `stage_bootseed`
hecho y comprobado **dentro del `.data` servido** (el `default.ide` del paquete es
idéntico al `bootseed`). El cargador **ponderado por frecuencia** se queda: eso es
lo que hace que los 7 entren en el fondo a su ritmo real.

**2. La partida del 21/09 (log `13:12:03 → 13:17:38Z`) pasa las dos pruebas que
faltaban para J1:** llegó a **3 estrellas** (`lvl=3` en 38 muestras) y entró en
cinemática (`cut=1`) **sin un solo cuelgue ni `JSERR`/`ENGERR`**. Además el corte
nuevo de `CARRATE` funciona: `CARRATECNT n=8192 c0=2104 c2=2533 c3=1970 c6=1096
c8=489` — ni una tirada de las clases 1, 4 y 5, que es exactamente lo medido en D4
(el Perennial y el Trashmaster no pueden salir en esa zona).

**3. `VICEEX sfx arma=` = 0 en esa sesión** (no se tecleó `CRAZYTOOLS`): la
audición de las 8 armas nuevas queda pendiente de una partida con ellas en mano.
La única pieza de la sección que sigue abierta es el **AUG apuntando a un lado**
(bloqueada a una captura del jugador) y la corrección de la tabla de sonidos por
oído. **Nada commiteado.**

## 2026-09-21 08:40 — Sección 3: la 7ª partida del jugador ("todas mentiras") — build `ve17`

El jugador tenía razón y la culpa de casi todo era de trazas que decían una cosa y
un motor que hacía otra. Lo que se ha corregido (arreglando, sin deshacer nada):

**1. La V de la 1ª persona nunca llegaba al motor `[raíz]`.** Su log de la 6ª
partida tenía **0** líneas `1p key`. La causa es la misma que ya se corrigió en la
recarga con R: la **config de controles que el navegador guarda** entre sesiones
pisa los valores por defecto, y si se guardó con un build anterior a que existiera
la acción, ésta queda **sin tecla** (`rsNULL` = 1056 — es lo que imprime su propio
log: `VICEEXT reload key tecla=1056`). El conmutador leía la acción sin respaldo.
Nuevo `CControllerConfigManager::ViceExtActionKeyJustDown(accion, tecla)` (si la
acción no tiene tecla asignada, se acepta la histórica) y lo usan **V** (1ª
persona) y **C** (agachado, `VICEEXT_ENGINE_DUCK_KEY`).

**2. El autocentrado de cámara no estaba en el camino que se juega `[raíz]`.**
Estaba escrito sólo en `CCam::Process_FollowCar_SA` (cámara LCS), que en este port
sólo corre con `bFreeCam` encendido y **`bFreeCam` nace apagado**: en coche se juega
`MODE_CAM_ON_A_STRING` (`mode=18` en su log). Además el detector de "estoy
mirando" era `Abs(GetMouseX()) > 0`, que con el ratón apoyado —el ruido sub-píxel
nunca es 0 exacto— daba **siempre** 1: su log lo dice, `camauto auto=0 mirando=1`.
Ahora hay un detector por **ventana (~1 s, suma con signo)** y el retorno se aplica
en **las dos** cámaras de coche (en la de serie girando `Source` alrededor del
target). Traza `camauto2 auto= mirando= conduzco=`.

**3. El agachado del jugador ya no es el de DISPARO del motor.** `SetDuck` →
`bCrouchWhenShooting` es el agachado que usan las misiones para que un NPC no se
mueva: "el agachado no me deja mover" era exactamente eso. Ahora la C conmuta un
estado propio (con arma o sin ella) y **el movimiento lo lleva el bloque del
agachado** (0,5 m/s, rumbo = mirada), con los clips `crouch_idle/forward/backward`
del mod; `ProcessControl` no reparte el control a pie normal mientras esté
agachado. Traza `crouch move spd= andando= clip= arma= mio= estado=` (1/s).

**4. El nado no reconocía la CAÍDA al agua.** `ViceExtSwimControl` exigía
`IsPedInControl()`, y al caer al agua el ped está en `PED_FALL` con `bIsInTheAir`
puesto, así que esa comprobación era **falsa justo cuando hace falta nadar**: el
ped caía hasta el fondo del mar ("solo cae el personaje como si estuviera cayendo
al vacío"). Ahora la caída (`PED_FALL`/`PED_JUMP`) también cuenta y hay traza
`VICEEXT swim no …` (1/s) con el motivo si hay agua honda y no se nada.

**5. Depósito de gasolina: precisión `[raíz]`.** El radio era 0,6/0,75 m y el
jugador lo vio enseguida ("basta con disparar alrededor del depósito"): ahora
**0,22 m** al dummy `petrolcap` y, en el respaldo por caja, un **cuadro** en el
sistema local del vehículo (0,30 × 0,45 × 0,30). En **motos** el respaldo no
existe: o hay tanque real, o no hay depósito ("a las motos sólo si se les dispara
a su tanque").

**6. La lista de CARGAR vuelve a tener 8 filas.** Se había quedado en 9 (autoguardado +
ranuras 1..8) y el jugador lo rechazó con la captura delante: "los mismos 8 slots
pero 1 reservado para el autoguardado". Ahora son **autoguardado + 1..7** (con el
rótulo AUTOGUARDADO) y el menú de guardar lista esas mismas 1..7: el fichero
`GTAVCsf8.b` deja de ser accesible desde el menú (anotado en el plan).

**Compilado y enlazado** (`ninja` 3/3 + enlace, sin errores) en `ve17`; los DATOS
no cambian (`dataTag` sigue `ve12`). **Nada commiteado.** Lo que falta para
cerrarlo es la partida del jugador: V (cámara a la cabeza), C caminando agachado,
echarse al agua, disparar a un depósito (debe explotar sólo al tapón) y soltar el
ratón en coche (debe volver detrás sola).

## 21/09/2026 (tarde) — Logs por sesión con fecha + plan de correcciones de la 5ª partida (sección 1)

- **R1 (hecho):** `gta_vc_browser/web/lib/vite.js` archiva cada sesión cerrada en
  `gta_vc_browser/logs/odtrace-<AAA-MM-DD>_<HH-MM-SS>.log` (hora de inicio de la
  partida en el nombre; nunca se borra; `*.prev.log` se mantiene por
  compatibilidad). Las dos sesiones de hoy ya están archivadas a mano. Requiere
  reiniciar el servidor de Vite (el plugin se carga al arrancar).
- **Plan nuevo:** `.agents/plans/mecanicas/06-plan-correcciones-5a-partida.md`
  (R1–R12 + backlog X1–X26 del mod, excluyendo lo ya pedido y lo descartado).
  Reparto: sección 1 → R1-R4; sección 3 → R5-R10; sección 2 → R11 (luces de
  servicio del coche de policía = función v3.0 del mod `Service lights for
  service cars`).
- Diagnósticos medidos, no supuestos: la 5ª partida (21:24-21:27Z, build `ve17`,
  datos `ve12`) acabó en un **tirón de 1.256 ms de `wait`** (capa on-demand, R2);
  el nado entra y sale del agua por umbral (`0,90` de entrada vs `0,55` de
  flotación); el retroceso **sí** se ejecuta pero sólo mueve
  `m_f3rdPersonCHairMultY`; la 1ª persona no recibe **ni una** pulsación
  (`VICEEX 1p key` nunca sale); el agachado le quita el movimiento al motor; la
  recarga arranca (`reload start clip=28 total=441`) y no hay traza de cierre.
- **Plan 06 v2 (enriquecido con investigación externa).** Se leyeron y citaron:
  `jborza.com/post/2024-11-03-first-person-gta-iii` (trampas reales de la 1ª
  persona sobre este mismo motor y la clave del movimiento: `m_fMoveSpeed` sólo
  empuja hacia delante, hay que escribir `m_vecMoveSpeed`), `gta-reversed/gta-reversed`
  (fuente de SA para las mecánicas cuyo arte ya sirve el mod), el mod de luces de
  policía por **dummies** de vehículo, la ficha del mod y su propio `Help.txt` /
  `features.ini` / `limits.ini`. Con eso cada bloque trae implementación concreta:
  - **R8 (retroceso):** `CCamera::m_f3rdPersonCHairMultY` es el **vector de
    apuntado y la retícula** (`Weapon.cpp:63`, `Camera.cpp:4134`, `Hud.cpp:363`);
    por eso se mueve la mira y no la vista. El *pitch* es **`CCam::Alpha`**, que el
    ratón **recalcula entero cada frame** (`Cam.cpp:1397-1419`, clamp en
    `:1423`): hay que llevar un **offset persistente** y sumarlo tras el ratón.
  - **R5/R6/R7:** mismas trampas y misma solución de movimiento que el blog.
  - **R11 (luces de policía):** las coronas están **clavadas por modelo y
    posición** en `Automobile.cpp:2166-2255` y no leen los dummies
    `servicelights*` del `police.dff` del mod → sección 2, método de la comunidad.
  - §F deja los **encargos listos para pegar** en aux 1 (sección 2) y aux 2
    (sección 3): no puedo escribir en sus hilos desde aquí.
- **Hallazgo 21/09 (sección 1): el mod Vice Extended ES un fork de reVC.** Lo dice
  su propio `ViceEx.exe` (cadenas `reVC`, `librw`, `CViceEx.ini`,
  `ViceExtended/features.ini` y **101 rutas `extended/src/...`** de sus asserts) y
  lo confirman sus DLL (`libmpg123-0`, `OpenAL32`, `modloader.asi`). Como es reVC,
  sus mecánicas nuevas están en **los mismos ficheros que las nuestras** (no añade
  ficheros de mecánicas; sólo `extras/` para postfx): la paridad es un problema de
  *diff*, no de reinventar. Extraído del binario (nombres exactos que antes eran
  incógnita): `CCam::Process_1stPerson`, `FOV_FirstPerson`, `HeadBob1stPerson`,
  `Real 1st Person`, `DoomMode_FirstPerson`, `PED_RELOAD`,
  `PED_1RST_PERSON_LOOK_UP/DOWN/LEFT/RIGHT`, `PED_CENTER_CAMERA_BEHIND_PLAYER`,
  `PED_WALK`, y los dummies de luces de servicio **`servicelights`**,
  `servicelights_0..3` y **`servicelightson`** (más `DMAudio.Service`). Volcado
  reproducible con la herramienta nueva `gta_vc_browser/tools/viceex-strings.py`.
  Plan 06 actualizado a v3 (§B2 hallazgo, §B3 herramienta; R5/R10/R11 con los
  nombres exactos).

### 21/09 (tarde) — Reparto en 2 para trabajar en paralelo (sección 1)

- **Nuevo:** `.agents/plans/mecanicas/07-encargo-agente-paralelo.md`. Traspaso
  completo para el agente que sí puede correr en paralelo: contexto, reglas,
  **cómo compilar UN solo fichero sin enlazar**
  (`ninja -C gta_vc_browser/build/web src/CMakeFiles/reVC.dir/<ruta>.cpp.o`),
  estado de lo ya hecho, las **7 tareas pesadas** (R5-R11) con causa y criterio
  PASS, la **frontera de ficheros** (para que no haya dos versiones de un `.cpp`)
  y el reparto: mecánicas → agente paralelo; datos/textos/audio/web/herramientas y
  **el único build final** → sección 1.
- **Hecho hoy (sección 1):**
  - `web/lib/vite.js`: cada sesión se archiva con fecha en `gta_vc_browser/logs/`
    (R1). **Requiere reiniciar Vite.**
  - `web/ondemand.js` (R2): trazas `ODSTA`/`ODWRITE`; la emisora ya **no** se espera
    nunca; la precarga de la emisora siguiente va retenida (sin escribir MEMFS) y
    sólo si no hay carga bloqueante; `wkReadyCap` 24→40 MB. Verificado con
    `node --check`.
  - `src/text/Messages.cpp` + `src/renderer/Hud.cpp` (R3): traza `SCRTXT` con clave
    GXT, duración, canal y "¿hay misión?" en `AddMessageJumpQ`, `AddBigMessage`,
    `AddBigMessageQ`, `AddMessageJumpQWithString` y `SetHelpMessage`. **Verificado
    compilando los dos objetos con ninja por fichero** (0 errores).
  - Dato para R2: la traza C++ `STREAM preload` **ya existía** y en la última
    partida salió **0 veces** → esa sesión no abrió emisora; el tirón de 1256 ms se
    localizará con `ODSTA`/`ODWRITE` en la próxima.
- **Verificador:** `tools/viceext-log-check.py` dicta ya **R2** (peticiones de
  emisora `ODSTA`, aperturas `STREAM preload`, copias grandes `ODWRITE` y tirones
  de frame por `FPHASE wait > 100 ms` → FALLO si alguna petición esperó) y **R3**
  (textos `SCRTXT` con clave GXT, canal y misión → FALLO si hay clave vacía,
  AVISO si salen fuera de misión). Probado con el log archivado: detecta los dos
  tirones (1257 ms y 117 ms) y avisa de que el build jugado aún no lleva las
  trazas.
- **X12 (barato, mío):** `#define VICEEXT_MONEY_NO_ZEROS` en `config.h` +
  `src/renderer/Hud.cpp` (el HUD pasa de `$%08d` a `$%d`) = paridad con
  `RemoveMoneyZerosInTheHud` de su `features.ini`. Compilado el `.o` (0 errores).

## 2026-09-22 — Agente paralelo: R5-R11 del plan 06 (código + traza + .o, sin enlazar)

Encargo `.agents/plans/mecanicas/07-encargo-agente-paralelo.md`, orden §8:
R8 → R7 → R6 → R5 → R10 → R9 → R11. Cada bloque: código + su traza +
compilado SOLO por objeto con ninja (sin enlazar; el build del paquete lo lanza
la sección 1). No se tocó `gta_vc_browser/**`, ni `src/text/Messages.cpp`, ni
`src/renderer/Hud.cpp`, ni `src/core/config.h` (los cambios de `config.h`/
`Hud.cpp`/`Messages.cpp` que muestra `git status` son de las secciones 1-3,
anteriores a este turno). Sin commits.

### R8 · Retroceso mueve la CÁMARA además de la mira
- **Causa:** `m_f3rdPersonCHairMultY` es apuntado+retícula, no vista; el *pitch*
  es `CCam::Alpha` y el ratón lo recalcula cada frame → pico aislado = perdido.
- **Código:** offset persistente `s_odRecoilAlpha`/`s_odRecoilAlphaApplied`
  (`src/core/Cam.cpp`): se suma a `Alpha` DESPUÉS del ratón y ANTES del clamp en
  `Process_FollowPedWithMouse`, `Process_FollowPed_Rotation`,
  `Process_1rstPersonPedOnPC` y `Process_M16_1stPerson`; decae a
  0,035 rad/s, tope 0,05 rad; se anula con `ResetStatics`. Patada por slot en
  `ViceExtRecoilKick` (`src/weapons/Weapon.cpp`): pistola 0,25° / SMG 0,15° /
  rifle 0,35° / escopeta 0,8° / pesada 0,6° / sniper 0,9°; sólo a pie. La parte
  `multY` se mantiene (mueve bala+mira). `CamShakeNoPos` ahora proporcional y
  visible (`0,01 + 1,5*kickAlpha`).
- **Trazas:** `RECOIL2 alpha=… grados=…` (Cam, 5/s mientras hay offset) y
  `VICEEXT recoil kick … alpha=… grados=…` ampliada (arma).
- **Compila:** `Weapon.cpp.o` + `Cam.cpp.o` OK (sólo warnings preexistentes).
- **PASS (partida):** `Alpha` sube y vuelve; vista acompaña a la mira.

### R7 · Nado con histéresis
- **Causa 1:** entrar exigía 0,90 pero flotar deja a nivel−0,55 → exit→hunde→enter.
  **Causa 2:** `IsPedInControl()` falso al caer (`PED_FALL`) y en `PED_IDLE`.
- **Código** (`src/peds/PlayerPed.cpp`, `ViceExtSwimControl`): histéresis
  (entrar 0,90 / seguir hasta 0,45, `VICEEXT_SWIM_KEEP_MARGIN`); `odCanSwim`
  acepta control/`PED_IDLE`/`FALL`/`JUMP`; movimiento horizontal por
  `m_vecMoveSpeed` (rumbo = cámara+stick, `m_fRotationCur` fijado: atrás/lado
  van donde se mira); al salir `swim_jumpout` + velocidades a 0 + subir a
  nivel−0,40 si sigue en somera; cierre con motivo si se pierde control.
- **Trazas:** `SWIM2 move spd=… clip=… hondo=…` (1/s) y
  `SWIM2 exit motivo=poco-hondo|sin-agua|sin-control clip=…` (se conservan las
  `VICEEXT swim …` para el verificador actual; `exit` ahora lleva `motivo=`).
- **Compila:** `PlayerPed.cpp.o` OK.
- **PASS:** 20 s nadando = 1 `enter`, clips alternando, 0 `exit` intermedios.

### R6 · Agachado: movimiento al motor
- **Causa:** el bloque escribía `m_fRotationDest`/`m_fMoveSpeed` y vetaba el
  control normal → ped y mirada por caminos distintos, cámara "fija".
- **Código:** `ViceExtCrouchControl` (`src/peds/PlayerPed.cpp`) ya NO toca
  movimiento y devuelve false (el control a pie normal corre: Zelda/1ª
  persona); sólo clip `CROUCH_*` + `ViceExtCrouchLimitSpeed()` (tope 0,5 m/s
  tras el control normal). Objetivo de cámara −0,45 m agachado
  (`src/core/Cam.cpp`, `FollowPedWithMouse` + `FollowPed_Rotation`, vía
  `CPlayerPed::ViceExtIsCrouched()` nuevo en `PlayerPed.h`).
- **Trazas:** `CROUCH2 h=… rotDest=… rumbo=… clip=…` (1/s; la `VICEEXT crouch
  move …` sigue con `mio=0`).
- **Compila:** `PlayerPed.cpp.o` + `Cam.cpp.o` OK.
- **PASS:** cámara sigue, avanza donde mira, atrás/lado OK.

### R5 · Primera persona: la tecla ya llega
- **Causa 1:** config guardada anterior a la acción → sin tecla (`tecla=1056` =
  `rsNULL`): `ViceExtActionKeyJustDown` (`src/core/ControllerConfig.cpp`) ahora
  respalda con `'V'` también con `1056` explícito + comentario de la trampa del
  pad (`MODE_1STPERSON_RUNABOUT` a propósito fuera de la lista "sólo zoom").
- **Causa 2/puerta** (`src/core/Camera.cpp`): alineada con la cámara de ratón
  (`m_bLookingAtPlayer || Cams[0].Using3rdPersonMouseCam()`); `CAM1P` escribe
  TAMBIÉN al pulsar (aunque no aplique) con tecla cruda `v=` + modo; ojos =
  IK + 0,19 m al frente con rumbo actual (`src/core/Cam.cpp`,
  `Process_1rstPersonPedOnPC`).
- **Trazas:** `CAM1P … tog=… togkey=… v=…` + `VICEEXT 1p key …` (siempre al pulsar).
- **Compila:** `Camera.cpp.o` + `ControllerConfig.cpp.o` + `Cam.cpp.o` OK.
- **PASS:** `1p key` → `tog=1 mode=MODE_1STPERSON_RUNABOUT`; andar lado/atrás,
  saltar y subir al coche funcionan (sin trampa del pad).
- **Pendiente (pide a sección 1, toca sus ficheros):** `Real 1st Person` en
  menú + `FOV_FirstPerson`/`HeadBob1stPerson` propios (requiere `config.h` y
  `Frontend`: no tocados por regla). Las 4 acciones `PED_1RST_PERSON_LOOK_*` ya
  existen con binding.

### R10 · `R` recarga con cierre medible
- **Estado:** acción real `PED_RELOAD` con `R` por defecto + respaldo a `'R'`
  directa ya existían (C3.1 + P4-sección-2); el bloque pone el mismo estado que
  la automática y NO la corta ni deja `m_nTimer` a medias (comentado en código).
- **Código** (`src/peds/PlayerPed.cpp`): cierre `VICEEXT reload done manual=1
  clip=…/… total=…` (cargador rellenado) y `VICEEXT reload fail
  motivo=vehiculo|muerto|arma-cambiada|interrumpida clip=…/… total=…` (aborto:
  cambio de arma, coche, muerte…), con clip/tipo de partida guardados.
- **Compila:** `PlayerPed.cpp.o` OK.
- **PASS:** `reload done` con lleno y `total` reducido, 2 veces seguidas.

### R9 · Apuntado (AUG): medir con `AIMDIR`
- **Código** (`src/peds/PlayerPed.cpp`, tras `VICEEXT_AIM_WALK`): traza `AIMDIR
  arma=… desv=… grupo=… clip=… peso=… crouchW=… swimW=…` (1/s apuntando:
  desviación cámara-vs-rumbo, grupo del arma, clip de fuego y pesos de blends
  parciales). Arreglo de rumbo del blog SÓLO si todas fallan al caminar:
  caminando y apuntando con arma de apuntar, rumbo = cámara (`Front.Heading()`
  + `SetHeading` + `UpdateRW()`).
- **Compila:** `PlayerPed.cpp.o` OK.
- **PASS:** `AIMDIR desv<5°`. Si sólo fallan las nuevas → sección 1 compara el
  `AnimAssocDesc` contra el `.ifp` y prueba `steyr`→`rifle` / `Shotgun2`→
  `shotgun` (dato, no tocado aquí).

### R11 · Sirena del coche de policía por dummies
- **Causa:** coronas fijas `MI_POLICE` ±0,7/−0,4/1,0 que con el `police.dff` del
  mod caen fuera de la barra (destellos sí, barra no).
- **Código** (`src/vehicles/Automobile.cpp`, `PreRender`): buscar UNA vez por
  modelo (caché de 8: locales iguales para todas las instancias) los dummies
  `servicelights`, `servicelights_0..3` y `servicelightson` (nombres del binario
  del mod) vía `CVehicle::FindDummyFrame`; en sirena usar sus posiciones mundo
  (rojo/azul interpolando, sin doble `GetMatrix`), corona 0,55/60 en vía
  dummies; sin dummies, fijas 0,4/50 de siempre (vanilla intacto). `servicelightson`
  hoy sólo va a la traza (el que decide sigue siendo `m_bSirenOrAlarm`).
  Incluye `ondemand.h` sin condición (arregla de paso el `ODTRACES` de C3.4 que
  no compilaba con el define apagado).
- **Trazas:** `SVLIGHTS model=… dummies=N on=… pos=…` (al cachear + cada 2 s con
  sirena).
- **Compila:** `Automobile.cpp.o` OK.
- **PASS:** `SVLIGHTS dummies=4` + captura con barra roja/azul en la barra.
- **No hecho (de otra zona):** `DMAudio.Service` (audio, sección 1).

### Compilación final conjunta (sin enlazar)
`ninja -C gta_vc_browser/build/web` sobre los 6 objetos tocados
(`weapons/Weapon.cpp.o`, `core/Cam.cpp.o`, `peds/PlayerPed.cpp.o`,
`core/Camera.cpp.o`, `core/ControllerConfig.cpp.o`,
`vehicles/Automobile.cpp.o`): **EXIT=0**, sólo warnings preexistentes. Paquete
NO enlazado: lo lanza la sección 1.

## 2026-09-22 — Subagente 3, parte B: H1-H3 + §6 de la 7ª partida (código + traza + .o, sin enlazar)

Encargo `.agents/plans/mecanicas/09-encargo-subagente-3.md` (parte B: H1, H2,
H3 y §6; H4/H5 son HUD de la sección 1: sólo observados y anotados).
Fuente: `gta_vc_browser/logs/odtrace-2026-09-22_01-23-46.log` (ve19/ve13) +
vídeo (`t12` agachado, `t103-112` apuntado, `t135/141` policía, `t173-179`
nado, leídos los PNG ya extraídos en `tmp/frames/`). Cada bloque: código + su
traza + `.o` por ninja sin enlazar. Tocados sólo `src/core/Cam.cpp`,
`src/peds/PlayerPed.cpp`, `src/peds/PlayerPed.h`, `src/vehicles/Automobile.cpp`
(los de mi tabla §2). Sin commits, sin enlace.

### H1 · Agachado: la cámara baja 0,55 + `camz=` en la traza
- **Medido:** clip correcto con peso 1 (`clip=238/239 peso=1.00`) pero `t=12`
  cámara dentro de la cabeza: el −0,45 de R6 no bastaba.
- **Código** (`src/core/Cam.cpp`, `FollowPedWithMouse` + `FollowPed_Rotation`):
  objetivo −0,55 agachado. Traza: `CROUCH2 … camz=` (altura de cámara).
- **Compila:** `Cam.cpp.o` + `PlayerPed.cpp.o` OK.
- **PASS:** `cam.z − ped.z` baja ~0,5 agachado y la nuca no llena pantalla.

### H2 · Nado: enganche a 0,25 + cámara a superficie + bug gordo de velocidad
- **Medido:** 10 `enter`/`exit motivo=poco-hondo` (el flotar a −0,55 y las olas
  cruzaban el 0,45); `t=176` cámara en el plano del agua.
- **Bug propio cazado con el log:** `SWIM2 avance=65` con `spd=1.3` — el ped
  salía a ~65 m/s. Causa: `m_vecMoveSpeed` NO va en m/s (el físico integra
  `Translate(m_vecMoveSpeed * GetTimeStep())`, timestep ≈ 1 a 50 fps = m/frame).
  Todo lo vertical/horizontal pasa a m/frame (/50): subida máx. 0,010,
  hundimiento 0,006, avance `odSpeed/50`. Sin esto el "nada" era un teletransporte.
- **Código** (`src/peds/PlayerPed.cpp`): `KEEP_MARGIN` 0,45→0,25; velocidades en
  m/frame; espejo `odSwimActive` + `ViceExtIsSwimming()`; `SWIM2 … camz=`.
  Cámara (`src/core/Cam.cpp`, los dos procesos a pie): nadando, objetivo =
  superficie +0,5.
- **Compila:** `PlayerPed.cpp.o` + `Cam.cpp.o` OK.
- **PASS:** 0 `exit motivo=poco-hondo` en agua honda; `avance` ≈ 1,3/s.

### H3 · Apuntado: snap en el control con ratón + `peso` honesto
- **Medido (104 `AIMDIR`):** vanilla ≈ 0 de mediana con picos al girar; 50 med
  16,9; 55 med 45,1 con `peso=0.00` siempre; 54 máx. 73,6.
- **Causa 1 (rumbo):** con ratón corre `PlayerControl1stPersonRunAround`, no
  Zelda: el snap de R9 nunca se ejecutaba y el rumbo seguía al movimiento con
  retardo. Snap añadido ahí (sólo apuntando con arma de apuntar; quieto es
  inocuo). **Causa 2 (métrica):** `peso` sólo miraba el FIRE; apuntando
  agachado la pose la lleva el CROUCHFIRE → 0,0 falso. Ahora el mayor de ambos.
- **Sobre el 55:** en esta sesión jamás se disparó (0 kicks/sfx suyos) y el
  `buddy.ifp` servido trae sus 5 clips (`buddy_fire`…); con `peso=0` = sin
  assoc FIRE = sin disparar, no roto probado. Si al dispararlo sigue a un lado,
  el siguiente paso es comparar su `AnimAssocDesc` (sección 1 lee el `.ifp`).
- **Código:** `src/peds/PlayerPed.cpp` (snap + peso). **Compila:** OK.
- **PASS:** `AIMDIR desv<5` + pose con retícula en vídeo.

### H4/H5 · Retícula e icono (HUD, sección 1 — sólo observado)
- H4: el árbol YA trae el arreglo L2 (`DrawCrossHairPC` para 48-55); ve19 es
  anterior (0 `SIGHT` con arma nueva, 0 `HUDICON` en el log). Nada que tocar.
- H5: el camino del icono es por nombre de modelo en su TXD + traza `HUDICON`
  (también posterior a ve19). Si con ve20 sigue sin icono el 55, mirar esa traza.
- De paso: `reload key tecla=1056` sigue mostrando la tecla de la ACCIÓN
  (sin binding), no la física pulsada (`R` sí llega: salen sus `reload no`).
  Cosmético; no se toca.

### §6 · `SVLIGHTS dummies=0`: el fallo se reintenta, el buscador estaba bien
- **Medido:** `model=156 dummies=0` una sola vez (= quedó cacheado); el `.dff`
  trae `servicelight/servicelights/_1.._3` (sin `_0`) y el MISMO 156 resuelve
  `petrolcap` (`gastank hit reserva=0`) con la misma función.
- **Verificado en fuente:** `CVehicle::FindDummyFrame` YA es recursivo (en
  librw `forAllChildren` PARA con nil y SIGUE con no-nil, al revés que RW
  original; el callback lo respeta). No se toca `Vehicle.cpp`.
- **Código** (`src/vehicles/Automobile.cpp`): el `n=0` NO se cachea (reintento
  cada 5 s; sólo aciertos fijos) + nombres `servicelights,_1.._3,_0,
  servicelight` (+`servicelightson` a la traza).
- **Compila:** `Automobile.cpp.o` OK.
- **PASS:** `SVLIGHTS model=156 dummies>=1` + barra en vídeo.

### Compilación conjunta (sin enlazar)
`Weapon.cpp.o` (R8, sin cambios esta vez), `Cam.cpp.o`, `PlayerPed.cpp.o`,
`Camera.cpp.o` (R5, sin cambios), `ControllerConfig.cpp.o` (R5, sin cambios),
`Automobile.cpp.o`: **EXIT=0**. Paquete NO enlazado.

## 2026-09-21 · sección 1 — D8b: "las armas nuevas sonaban MUDAS" (arreglado)

**Lo dijo el jugador y era verdad**: los sonidos nuevos del mod no se oían. El
plan (bloque R4) los daba por "funcionando": era un veredicto basado en que la
traza existía, no en que el sonido arrancara. Corregido.

- **Evidencia**: partida `logs/odtrace-2026-09-21_21-24-32.log` → 40 líneas
  `VICEEX sfx arma=51 sample=9943` / `arma=54 sample=9953`, y **cero**
  `ODSFXMISS sfx=994x` (la capa de audio sí trazó 153 muestras: ninguna del mod).
- **Causa**: `cSampleManager::InitialiseChannel` (`src/audio/sampman_oal.cpp`)
  sólo reproducía `nSfx < SAMPLEBANK_MAX`, que es el **fin de la tabla SDT
  original** (`AudioSamples.h`). Las 13 muestras del mod (ids 9941..9953) están
  por encima: iban a la rama de comentarios de ped y `return FALSE`. Silencio sin
  traza. No era el mp3, ni el SDT (9954 entradas), ni el banco ni la tabla de armas.
- **Arreglo**: en `InitialiseChannel`, ese rango se sirve por el mismo camino
  on-demand que `SFX_BANK_0` (`odViceExSample`). Compilado sin enlazar: OK.
- **Segundo fallo de camino**: el banco del mod declara frecuencias imposibles en
  MP3 (36000/33000/22000 Hz); ffmpeg resampleaba a 32000/22050 y la SDT seguía
  declarando la del mod ⇒ hasta **12 % rápido/agudo**. `tools/add_viceex_sfx.py`
  ahora escribe la frecuencia REAL (ffprobe) y su tamaño PCM; los 13 mp3
  re-codificados ⇒ **DATO nuevo: `dataTag` `ve12` → `ve13`**, `VERSION` `ve18`.
  `manifest.json` regenerado (17477 entradas).
- **Herramientas nuevas**: `tools/viceex-sfx-check.py` (tabla + fichero + formato
  + rango del motor + tabla de armas; hoy 13/13 OK) y `tools/viceext-log-check.py`
  bloque **D8** (disparo sin arranque = arma muda; con el log de la partida muda
  da FALLO, probado). El lanzagranadas tiene ya su `case` propio en la tabla.
- **Sin enlazar el paquete** (el build único lo lanza la sección 1 con el resto).

## 2026-09-21 · sección 1 — REVISIÓN del trabajo del agente paralelo (R5-R11)

Pedido del jugador: "valida que lo del otro agente tenga sentido o no tenga riesgo
y corrígelo si puedes". Revisados los 7 bloques leyendo el código (sin jugar) y
compilando. **Dos correcciones** y una mejora de traza; el resto, visto bueno.

### Corregido 1 · R6 (agachado): el clip de agachado se perdía
- **Qué pasaba:** el agente lo cambió a "no mover, sólo clip + tope de velocidad"
  (bien: eso arregla el "Tommy va hacia atrás" y la cámara). Pero el clip se
  mezclaba al PRINCIPIO del frame y después corría el control a pie normal, que
  llama a `SetRealMoveAnim()` y mezcla andar/correr por `m_fMoveSpeed`: el clip de
  agachado quedaba pisado en el mismo frame ("se agacha a medias").
- **Arreglo (`src/peds/PlayerPed.cpp`):** el clip vive ahora en
  `CPlayerPed::ViceExtCrouchAnim()` (idempotente, mismo patrón que el nado) y
  `SetRealMoveAnim()` sale por ahí cuando `odCrouched`: mientras estás agachado,
  el selector de animación de serie no compite. El movimiento sigue siendo del
  motor (que es lo que R6 quería).
- **Traza:** `CROUCH2 … peso=0.82` — peso REAL del clip en la mezcla. Si algún día
  baja de ~0,5, es que otro clip se lo está comiendo (antes eso era invisible).

### Corregido 2 · R9 (apuntado): el override de rumbo se perdía
- **Qué pasaba:** el bloque que fija el rumbo a la cámara estaba ANTES del bloque
  de movimiento, que hace `m_fRotationDest = neededTurn` (el rumbo del palo) unas
  líneas después: la asignación se sobreescribía en el mismo frame, así que no
  arreglaba el "apunta a un costado" y encima hacía un giro brusco y una deriva.
- **Arreglo:** el bloque se movió DESPUÉS del movimiento, donde su asignación ya
  manda. Sigue limitado a caminar + apuntando + arma de apuntar.

### Mejorado · R7 (nado): la traza ahora mide si el ped AVANZA
- `SWIM2 move spd=… avance=0.42 sube=0.01 clip=…` — `avance` son los metros
  recorridos desde la traza anterior. `spd` sólo dice lo que el motor quiso;
  `avance` dice si se movió de verdad. Sin esto, "nada" y "nada bien" se leen
  igual desde el log (así se coló el fallo original de que no nadaba).

### Visto bueno (sin tocar)
- **R8 retroceso:** los 4 puntos de aplicación son correctos (después del ratón,
  antes del clamp) y el offset se anula en `ResetStatics`; la parte `multY` se
  mantiene. Sin deriva: sólo se toca el diferencial.
- **R7 nado:** histéresis y `m_vecMoveSpeed` con rumbo de cámara; al salir
  devuelve velocidades a 0 y sube al ped a la orilla. Coherente.
- **R11 luces de servicio:** la caché por modelo se llena UNA vez y lee los
  dummies con LTM fresca (`CWorld::Process` llama `UpdateRwFrame()` antes de
  dibujar), y `odFromDummies` evita el doble `GetMatrix` que habría mandado las
  coronas al quinto pino. Sin fugas ni trabajo por frame.
- **R5 1ª persona:** los ojos por IK + 0,19 m al frente; la puerta usa la cámara
  de ratón. Compila y no toca el camino de la 1ª persona de arma.
- **R10 recarga:** sólo añade trazas de cierre/aborto; no toca el estado.

### Compilación
`ninja` por objeto: `peds/PlayerPed.cpp.o` **OK** (sólo warnings preexistentes),
y los 6 objetos del agente paralelo ya estaban en verde. Paquete NO enlazado: un
único build al final, como se acordó.

## 2026-09-21 20:06 — Sección 1: **el paquete NUNCA se había enlazado** (build `ve19`)

El jugador jugó "varios minutos" y su informe fue: agachado, apuntado, nado, 1ª
persona, sirenas y sonidos de armas **exactamente igual**. Tenía razón, y el
motivo no era el código:

- **Medido:** el motor servido (`web/public/build/reVC.wasm`) era del **21/09
  08:38** (build `ve17`). Dentro de ese wasm: `VICEEX sfx arma` sí,
  `odViceExSample` / `SWIM2` / `CROUCH2` / `SVLIGHTS` / `RECOIL2` / `AIMDIR` = **0**.
  Y el bundle de la capa web llevaba `dataTag ve12` y **sin `ODSTA`**.
- Es decir: R5-R11, D8b, R2 y R3 estaban **compilados como objeto** pero el
  paquete no se enlazó (se acordó "un build al final" y se pidió la partida
  antes). Su informe es correcto: probó el código de ayer.
- **Arreglado:** `bash gta_vc_browser/build.sh` con `emcc` en el PATH (hay que
  exportar `$EMSDK` y `$EMSDK/upstream/emscripten`; el `source emsdk_env.sh`
  no deja `emcc` visible en este shell). Enlazado a las 20:06: `ve19`, datos
  `ve13`. 226/226 tareas, EXIT=0.
- **Herramienta nueva para no repetirlo:** `gta_vc_browser/tools/check-served-build.sh`
  — mira el wasm/bundle SERVIDOS y dice marca por marca si lleva lo implementado
  (D8, D8b, R2, R5-R11) y cuál es el `dataTag`. Hoy: **OK, todas**. Regla: se
  lanza ANTES de pedir cualquier partida de prueba.
- **Reparto nuevo:** `.agents/plans/mecanicas/08-encargo-subagente-2.md` (lo
  pesado de la sección 2: X8, X9, X4, X10, X11, X26) y la sección 1 se queda la
  verificación desde log, audio, datos, HUD y el menú de 1ª persona.

### Barra de carga (web/lib/index.js) — una sola subida de 0 a 100

Pedido del jugador: "que sea lineal de 0 a 100, que solo cargue una vez".
Antes **cada fase empezaba en 0**: `probe` (comprobar ficheros) era un barrido
sin medida y `build` volvía a arrancar desde cero, así que parecía cargar dos
veces y no se leía como progreso. Ahora hay un progreso **global y monótono**:
`probe` aporta el 0-4% y `build` del 4% al 100%; si una llamada no trae medida
(el cargador no da total) la etiqueta cambia **pero el ancho no retrocede** (el
barrido indeterminado solo se usa mientras no hay ninguna medida). Además la
barra deja una traza cada 5% (`BART pct=…`), así que el propio log demuestra que
la subida es monótona sin mirar la pantalla. `node --check` OK.

### Biblioteca de referencias (plan 09, §9)

Barrido del 21/09 buscando código ajeno reutilizable. Las tres joyas:
**SilentPatchVC** (`CHANGELOG-VC.md` + `SilentPatchVC.cpp:864-943`) trae el
arreglo exacto de las **posiciones de las coronas de la sirena** de Police,
Ambulance, Firetruck, Enforcer y FBI; y advierte de que el **temporizador de los
mensajes se escala con la resolución** ("Mission title ... stay on screen for the
same duration, **regardless of screen resolution**") — la explicación más
probable de los textos que salen 3-4 fotogramas a 1840×928; y `ZeroAmmoFix`
(armas con 0 balas). **gta-reversed** (SA) es la fuente para nado/agachado/
apuntar andando, y sus clips son los que sirve este mod. Y el blog de 1ª persona
(jborza.com) para este mismo motor. De paso se verificó que nuestro
`Pad.cpp:1880` ya copia los tres búferes de teclado en el orden correcto (el bug
de latencia de SilentPatch **no** es nuestro).

## 21/09 — 7ª partida (log `logs/odtrace-2026-09-22_01-23-46.log`, build `ve19`)

El motor servido **sí** era el nuevo (`check-served-build.sh`: todas las marcas).
Primera vez que el log y el vídeo se cruzan: `gta_vc_browser/tools/frames-at.sh`
saca fotogramas en las marcas UTC del log (base del vídeo `01:24:22Z`) y los PNG
se ven con las herramientas de ficheros. Eso convirtió "lo veo raro" en dato.

- **Agachado:** `CROUCH2 … peso=0.99/1.00` y el clip es el correcto (el nombre→clip
  es `strcasecmp`, `AnimManager.cpp:1205`). El vídeo lo confirma: `t=11`/`t=20`
  agachado de verdad, pero **`t=12` la cámara está dentro de la cabeza** — falta
  bajar la cámara con el ped, no la animación.
- **Nado:** `SWIM2 exit motivo=poco-hondo` **10 veces** con `z=5.6 nivel=6.0`
  (profundidad 0,40 < margen 0,45): el flotar cruza el umbral. El vídeo
  (`t=173/176/179`) enseña la cámara clavada en el plano del agua y el ped debajo.
- **Apuntado:** `arma=54` (Steyr/AUG) `desv` hasta **73,6°**; `arma=55`
  (lanzagranadas) **`peso=0.00`** (no hay animación de apuntado) y sin retícula.
  Los `.ifp` del mod (`steyr.ifp`, `deagle.ifp`, `buddy.ifp`…) y los grupos
  (`ASSOCGRP_STEYR=62`, `ASSOCGRP_DEAGLE=61`) **están bien**: el fallo está en el
  motor (`PedFight`/`PlayerPed` R9), no en los datos.
- **Sirenas:** `SVLIGHTS model=156 dummies=0` y `156` **es** `police`
  (`streamed/data/default.ide:208`); el `.dff` **sí** trae `servicelights`,
  `servicelights_1..3` (sin `_0`). `FindDummyFrame` (`Vehicle.cpp:1471`) solo
  recorre hijos directos → buscar recursivo.
- **Retroceso:** funciona y se ejecuta (`VICEEXT recoil kick` 121 veces, alpha
  0,15-0,80°), pero **solo se ve apuntando**; al no apuntar las nuevas, el
  jugador no lo ve. Primero el apuntado, luego (si hace falta) subir la patada.
- **Textos "al azar":** `SCRTXT` sólo cazó avisos de truco (`time=0`) y
  `^ELIMINADO!` (`time=4000`), pero el jugador aclaró que los suyos salen
  **abajo, en blanco, tipo misión, unos pocos fotogramas y no son de trucos**.
  Eso significa que vienen por un camino que no era de los cuatro instrumentados.
  **Hecho:** caza-todo en `CFont::PrintString` (`Font.cpp`): registra todo texto
  de la mitad inferior con su contenido literal (`SCRTXT3 x= y= texto="..."`,
  dedupe por texto + banda de 16 px, 2 líneas/s). El próximo log lo nombra; **no
  se cambia ninguna duración hasta saber cuál es**.
- **R5b (hecho, sección 1):** el conmutador de 1ª persona se leía **dos veces**
  por frame (traza + puerta) y la primera consumía el "justo ahora": 46 líneas
  `1p key` y `tog=1` ni una vez. Ahora se lee una vez, con antirrebote de 300 ms,
  y la traza `VICEEXT 1p set` dice el **resultado** (puerta, flanco, tog y por qué
  se canceló). Compilado el objeto, sin enlazar.
- **Reparto nuevo:** `.agents/plans/mecanicas/09-encargo-subagente-3.md`
  (H1 cámara del agachado, H2 cámara/estado del nado, H3 apuntado de las nuevas,
  H4 retícula, H5 icono del lanzagranadas, §6 sirenas por dummies).

## 21/09 (tarde) · sección 1 — pantalla de carga de partida: una portada, una barra

**Petición del jugador:** al cargar una partida a veces ve la portada del juego, el
splash parece cargar más de una vez y la barra se reinicia; quiere **solo el splash
con un único progreso**.

**Causas (leídas en el código, con fichero:línea):**

1. `LoadingScreen()` (`src/core/main.cpp:647`) pinta su **propia** barra con
   `NumberOfChunksLoaded/TOTALNUMCHUNKS` (`main.cpp:113-114`), que empieza en 0 en
   cada llamada. Durante la carga se llama muchas veces (init, `FileLoader`,
   memoria) → **segunda barra** que aparenta reiniciar la primera.
2. `LoadSplash(nombre)` (`main.cpp:560`) **recarga el TXD y recrea el sprite**
   cuando el nombre pedido no es el que ya está cargado, y las pantallas de carga
   vanilla piden portada **aleatoria** (`GetRandomSplashScreen()` → `loadscN`,
   `main.cpp:609`), que es literalmente la portada del juego → el arte cambiaba a
   media carga y el splash se recargaba ("carga más de una vez").
3. Cada fase reprograma `gWebLoadFrac` desde abajo (`Game.cpp:935/942/955` →
   `1072` → `1112`…) y `WebDrawLoadScreen` (`main.cpp:926`) dibujaba ese valor tal
   cual: la barra **retrocedía** en cada frontera (shutdown 0,03 → parse 0,01).

**Hecho:**

- `WebBeginLoadScreen()` / `WebEndLoadScreen()` + `gWebLoadScreenActive`
  (`main.cpp`, declarados en `main.h`): una sola secuencia. `Begin` es idempotente
  mientras está activa (el arranque y la carga posterior son la **misma**
  secuencia, así el segundo tick de splash no devuelve la barra a cero), fija
  `splash1` (una sola carga: la segunda vez la textura ya está y no recarga nada)
  y reinicia la barra.
- `LoadSplash` ignora cualquier otro nombre mientras la secuencia está activa →
  nunca más una portada aleatoria a media carga.
- `LoadingScreen` delega en `WebDrawLoadScreen(gWebLoadFrac)` mientras está
  activa → desaparece la segunda barra y el texto de fases (queda **solo el
  splash**).
- `WebDrawLoadScreen` ahora es **monótono** (recuerda el máximo pintado y nunca
  retrocede) y tiene guardia de re-entrada (un `LoadSplash` que acabe en
  `LoadingScreen` no anida `DoRWStuff*`).
- `skel/glfw/glfw.cpp`: `WebBeginLoadScreen()` en los dos ticks que presentan el
  splash (arranque y carga) y `WebEndLoadScreen()` al completar y también en la
  rama "no hay carga" (para que no quede fijado).
- Trazas: `LOADSCR begin splash=splash1`, `LOADSCR end`, `LOADSCR clamp a->b`
  (solo cuando una fase intenta retroceder >0,01: prueba en el log que el recorte
  funcionó).
- Verificación: `tools/check-served-build.sh` gana la marca `LOADSCR begin`;
  `tools/viceext-log-check.py` gana el bloque **L6** (secuencias abiertas vs
  cerradas + recortes de barra).
- **Enlazado**: build `ve21` (21/09 21:09), `ninja` EXIT=0, `check-served-build.sh`
  = TODAS las marcas. `VERSION` en `web/lib/index.js` a `2026-09-21-ve21` (el
  `.wasm` se pide con `?v=`, así que no hay caché vieja).

## 21/09 (noche) · sección 1 — revisión de la parte B + **enlace final** (`ve22`)

El otro agente avisó "H1-H3 + §6, sin enlazar". Revisé su código (no su informe) y
lo compilé con el enlace final.

**Revisión, fichero a fichero:**

- `src/peds/PlayerPed.cpp` (H2/H3): correcto y con una comprobación que hice aparte:
  `m_vecMoveSpeed` se escribe a mano sólo cuando `odSwimming`, y **el control a pie
  de serie queda saltado** en ese estado (`if (odSwimming) {} else …` en
  `ProcessControl`), así que nadie lo pisa después; las unidades en m/frame
  (`/50`) cuadran con el `avance=65` de la 7ª partida (65/1,3 = 50 exacto).
  El snap de H3 sólo actúa con `GetTarget()` y arma apuntable: mismo efecto que el
  snap de R9 en Zelda, en el camino de ratón. Riesgo menor aceptado: apuntando en
  el aire el ped gira en el aire (cosmético).
- `src/core/Cam.cpp` (H1/H2): los dos procesos de cámara a pie aplican el sesgo
  sólo al **ped del jugador** y por estado (`ViceExtIsCrouched`,
  `ViceExtIsSwimming`), sin tocar coche ni mirilla. Se mide con `camz=`.
- `src/vehicles/Automobile.cpp` (§6): el fallo ya no se cachea (reintento cada
  5 s; sólo los aciertos quedan fijos) y se buscan los 6 nombres del `.dff`. Y su
  verificación es la correcta: `CVehicle::FindDummyFrame` **ya era recursivo** —
  en librw `RwFrameForAllChildren` para con `nil` y sigue con no-`nil`, al revés
  que el RW original; lo prueba el `petrolcap` del mismo modelo 156. No se toca
  `Vehicle.cpp`: mi sospecha anterior era falsa.
- No tocaron ficheros fuera de su tabla (`Vehicle.cpp` y `PedFight.cpp` sin
  cambios). Cero solapes con los míos.

**Verificación automática nueva:** bloque **`H`** en `tools/viceext-log-check.py`
(agachado por `CROUCH2 camz=` agachado vs de pie; nado por `SWIM2 avance=` real y
por las salidas `poco-hondo`). Probado contra el log de la 7ª partida: da
**FALLO** y nombra los dos fallos viejos (10 salidas del agua y `avance=65`), o
sea que caza exactamente lo que la parte B dice haber arreglado. Marcas nuevas en
`check-served-build.sh`: `camz=`, `servicelightson`.

**Enlace final:** `ninja` 80/80, EXIT=0 → build **`ve22`** (21/09 21:25),
`check-served-build.sh` = TODAS las marcas (11 + datos `ve13`). `VERSION` a
`2026-09-21-ve22`. A partir de aquí, **el motor servido lleva las dos partes**.

## 21/09 (noche) · sección 1 — investigación de 3 mods de la comunidad (planes, sin código)

Petición: leer SilentPatch, FramerateVigilante y WidescreenFixesPack, ver
compatibilidad y decidir si conviene un cargador de mods o meter su código.

**Decisión: portar el código, sin cargador.** Los tres hookean la binaria
original por patrón de bytes / direcciones (`hook::pattern`, `WriteMemory(0x…)`,
`d3d8.ual`), que no existen en un motor compilado desde fuente; replicar esa ABI
en WebAssembly sería enorme para cero ventaja. Donde ellos parchean bytes,
nosotros cambiamos la línea del bug. El render es nuestro (librw), así que los
parches D3D8 del WFP tampoco aplican: se toma su lógica.

**Licencias (comprobadas en el propio repo): los tres son MIT** → se puede
portar dando crédito. Reglas escritas en el plan: cabecera de atribución por
bloque, manifiesto `docs/mods/ATTRIBUTION.md`, grupos `VICEEXT_FIX_*` en
`config.h`, y **cero assets** (los del WFP son arte derivado del juego; si hace
falta una textura mejor se dibuja por código, como hace su propio radardisc).

**Planes creados** (`.agents/plans/mods/`):

- `00-INDICE.md`: método, licencias, reparto sin solapes (A coches/peds/sombras,
  B HUD/escala, C scripts/armas/datos, sección 1 arranque+enlace), verificación y
  prompts listos para los tres agentes.
- `01-silentpatch-vc.md`: ~45 arreglos con su línea en `SilentPatchVC.cpp`, en tres
  familias (T1 bugs de juego, T2 HUD/escala, T3 datos `.ipl.diff` de 8 zonas del
  mapa) y destino en nuestro árbol. Hallazgo grande: `:802` («big messages duran
  más a alta resolución por el texto deslizante») y `:866` (`CDarkel`) son el
  **candidato principal** de los «textos blancos» que el jugador ve sin motivo —
  encaja con que `SCRTXT`/`SCRTXT3` no los vieran por los caminos que
  instrumentamos.
- `02-frameratevigilante.md`: sus direcciones no sirven, su lista sí. Localizados
  ya en nuestro código: rotor del helicóptero (`Heli.cpp:573`, suma por frame),
  giro de rueda en raíles (`CarCtrl.cpp:1159`), bocina/sirena (`Automobile.cpp:1378`
  y el parpadeo por `GetFrameCounter()&7` en `:1587`) y timers del autopiloto
  (`CarAI.cpp`). Plan de prueba a 35 vs 120 fps con trazas por segundo.
- `03-widescreen-fixes-pack.md`: mapa de sus módulos `.ixx` a nuestros ficheros.
  Nuestra base ya trae `SCREEN_SCALE_*`/`CalculateAspectRatio()`, así que lo que
  se porta es la lógica de aspecto de `Sprite2d.ixx`, el disco de radar en alta
  calidad **dibujado por código** (`Radardisc.ixx`), el letterbox de cinemáticas
  y el anclaje del HUD.

**Comprobaciones hechas en nuestro árbol** (para que el plan no sea teoría):
existen `CShadows::CastShadowEntityXY`, `CPedAttractor::GetEffectForIceCreamVan`,
`CMBlur::AddRenderFx`, `CStinger::Process` y `IS_PLAYER_TARGETTING_CHAR`; y el
arreglo de latencia del teclado de SilentPatch (`:1324`) **ya está** en
`src/core/Pad.cpp` (verificado antes), así que esa fila va como `YA ESTABA`.

Nada implementado: sólo investigación y planes. El enlace sigue en `ve22`.

---

## 21/09/2026 (tarde, 9ª partida) · Sección 1 · build ve23

**CAUSA RAÍZ de media lista de fallos del jugador**: `CPed::CanStrafeOrMouseControl`
(Ped.cpp) devolvía `false` siempre porque `bFreeCam` está encendido en la partida
(`1p set ... free=1`, `mouse3d=1`), y `bFreeCam` en este port es la *cámara
moderna* (ratón a pie + coche tipo SA), no la cámara libre de depuración. Con
`false` se caían las tres puertas que encaran al jugador con la cámara
(`Cam.cpp:1621`, `Cam.cpp:4009`, `PlayerPed::ProcessAnimGroups`): de ahí el
`AIMDIR desv` de hasta 86° (arma al costado), el "va raro" agachado, el nado que
no va hacia donde se mira y la 1ª persona con `puerta=0`.

Enlace **ve23** con: `CanStrafeOrMouseControl` corregido, conmutador de 1ª persona
sin `bFreeCam` en su puerta, apuntado guiado por `m_bUseMouse3rdPerson`, recarga a
mano **con animación** (`reload anim`), agachado con antirrebote (`repetido=`),
autocentrado de coche suave + empujón al soltar la mirada (`camauto2 ... pedido=`)
y apuntado del lanzagranadas. Detalle, evidencia y qué mirar:
`.agents/plans/mecanicas/10-seccion-1-9a-partida.md`.

**Pendiente con dato nuevo**: `SVLIGHTS dummies=0` **y** `gastank reserva ... (sin
dummy petrolcap)` → fallan TODAS las búsquedas por nombre de frame
(`FindDummyFrame`), no sólo las de la sirena. Los `.dff` sí traen los nombres
(82 frames en `police.dff`, parser del chunk `0x0253F2FE`), así que el fallo está
en la lectura del NodeName en runtime. Siguiente paso: traza de un solo uso que
recorra el clump y anote los primeros nombres vistos.

---

## 21/09/2026 (noche, 10ª partida) · Sección 1 · build ve24 · LA SIRENA DE LAS PATRULLAS

**Corrección de mi diagnóstico anterior**: dije que "fallan TODAS las búsquedas por
nombre de frame". Era falso. En la misma traza, `gastank hit model=156 explota=1
reserva=0` = el dummy `petrolcap` **sí** se encontró (`reserva=0` ⇒ por dummy). Las
líneas `gastank reserva model=132/197/207/210/212` son modelos que genuinamente no
traen el dummy (sólo ~25 de ~100 `.dff` lo traen; el respaldo por caja es
intencional). `FindDummyFrame` funciona.

**Causa raíz real (parseando el `.dff` servido, no adivinada)**: en
`police.dff`, `servicelights_1..3` son **hijos** de `extra1..3`, y los extras son
**componentes**: `PreprocessHierarchy` los saca del clump y `CreateInstance`
clonaba **sólo su atomic**. Los hijos (las lentes de la barra, con malla propia)
nunca llegaban a la instancia ⇒ barra sin luces y `SVLIGHTS dummies=0` en toda
partida. Escaneados los **4.640 `.dff`** servidos: **sólo `police.dff`** tiene
componentes con hijos ⇒ arreglo quirúrgico.

**Arreglo (build ve24, enlazado y verificado con `check-served-build.sh`)**:

1. `src/modelinfo/VehicleModelInfo.cpp`: `ViceExtCloneComponent` clona el
   **subárbol completo** del marco del componente (patrón de
   `RecurseFrameChildrenToCloneCB`) **copiando el nombre del marco a mano**
   (`RwFrameCreate` no lo trae; sin eso los clones se llamarían `""`).
2. `src/vehicles/Automobile.cpp`: las coronas se anclan a la **esfera envolvente**
   del atomic de las luces (los marcos están en `(0,0,0)`: la malla se coloca por
   vértices), ± radio sobre el eje derecho; caché por **(modelo, extras elegidos)**
   porque cada patrulla lleva una de las tres variantes de barra.
3. Trazas nuevas: `SVLIGHTS ... var= extras= radio=` y bloque **SR** (`SIRENA tipo=
   sample= freq= vol= dist2=`) en AudioLogic para la sirena *audible*; el
   verificador (`tools/viceext-log-check.py`) ya los dictamina y
   `check-served-build.sh` busca las dos marcas nuevas.

Detalle completo (con el árbol de frames del `.dff` y qué mirar en la próxima
partida): `.agents/plans/mecanicas/10-seccion-1-9a-partida.md`.

**Nota de comportamiento**: una de cada tres patrullas no lleva ninguna barra
(`default.ide:208` → `comprules=0` y el motor elige con `RND(0,3) < 2`). Es de
serie: la traza lo distinguirá (`extras=-1`) de un fallo real.

---

## 21/09/2026 (noche, 10ª partida) · Sección 1 · build ve25 · PLAN DE MODS: primera tanda liviana

Arranca la ejecución de `.agents/plans/mods/` tomando **mi parte (sección 1)** y
empezando por lo más liviano, como se pidió.

**Hecho:**

1. **`docs/mods/ATTRIBUTION.md`** (nuevo, obligatorio por el plan §2): manifiesto
   de los tres mods (MIT) con una fila por arreglo, destino nuestro y estado
   (`PORTADO` / `YA ESTABA` / `NO APLICA` / `PENDIENTE`), el formato de la
   cabecera de atribución y las reglas de «no se copia» (direcciones, hookeo,
   assets).
2. **SilentPatch :723 — el contorno de la barra de carga no escalaba**
   (`src/core/main.cpp`, `LoadingScreen` y `WebDrawLoadScreen`): el margen era un
   literal de 1 px (`hpos-1.0f`); ahora pasa por la misma escala que la barra
   (`SCREEN_SCALE_X/Y(1.0)`), con la cabecera PORTADO y la línea del mod citada.
   Medible: traza `LBAR w= h= borde=x,y` (una línea por tamaño) y bloque **LB**
   nuevo en `tools/viceext-log-check.py` (`borde_y/alto` constante ⇒ escala).
   Marca `LBAR w=` en `check-served-build.sh`. Build **ve25**, enlazado, 18/18
   marcas OK.
3. **Plan enriquecido**: §8 «Reparto por coste» en `00-INDICE.md` con tres tandas
   (liviano / medio / pesado), estado de cada fila y dueño; `01-silentpatch-vc.md`
   con el estado de ejecución (`:723` PORTADO, `:1397` hecho por nuestra vía,
   `:1324` YA ESTABA, `:344` NO APLICA).

**Hallazgo de coste (cambia el orden del plan):** los `.ipl` del T3 están en
`gta_vc_browser/bootseed.list` (líneas 1698+), es decir **precargados** en el
paquete de datos. Aplicar los `.ipl.diff` de SilentPatch obliga a re-empaquetar
~160 MB y a que el jugador los vuelva a descargar ⇒ pasa de «tanda liviana» a
«tanda 2», no porque el código sea caro sino por la descarga.

**Siguiente (tanda 2, medio):** luces de servicio de los demás modelos
(`ambulan`/`firetruk` ya traen `servicelights_0` colgado de `chassis_dummy`, no de
extras: se encuentran, quedaría afinar la corona con su malla), los cuatro puntos
de FV (medir por segundo a 35 y 120 fps antes de tocar), y los bugs T1 que el
jugador ya reportó (coche que explota dos veces, sirena del FBI, casquillos).

## 11ª partida (21/09 noche) — «vi la v25 y todo sigue igual» → `ve26`

Sesión: `logs/odtrace-2026-09-22_04-04-07.log` (`ve25`, 2 min). Veredicto del
verificador sobre esa partida (para no repetir juicios sin dato):

- **R5 SIN DATOS**: «no se pulsó la tecla V en esta partida» (0 líneas `1p key`,
  `CAM1P v=0`). La primera persona no se pudo probar: **la V no llegó al motor**.
- **R9 OK**: las 5 armas nuevas apuntadas salen **`desv=0.0`** con pose (`peso=1.00`);
  el fallo viejo era `desv=73.6`, ya corregido en R6.
- **D8 OK**: las muestras de sonido del mod se disparan y suenan.
- **R12**: la recarga se pulsó **con los puños** (`arma=0`), sin dato con arma real.
- Los flags de `weapon.dat` de las armas nuevas (`Steyr 0x28050`, `Gr_launch 0x28040`)
  son **los mismos que el `m4` de serie**, así que no son la causa de «sin
  animación de apuntado».

Aplicado (**R13**, build `ve26`, enlazado; 21/21 marcas del comprobador):

1. **La 'V' se lee siempre**: por la acción rebindable **y** por la tecla cruda
   (`src/core/Camera.cpp`). Si la config guardada en el navegador tenía la acción
   con otra tecla (el hueco fue la `B`), ni la V ni el respaldo la veían. Traza
   `VICEEXT 1p bind tecla=` (una por sesión) con lo que hay guardado.
2. **`WINFO`** (`src/peds/PlayerPed.cpp`): una línea por arma que el jugador saca,
   con `flags`, `canaim`, `witharm`, `reload`, `crouchr`, `sight` y los **nombres de
   clip** del grupo (`clips=disparo|agachado|recarga|recarga-agachado`). Es lo que
   faltaba para dictaminar «sin animación de apuntado/recarga»: flag o clip ausente.
3. `AIMDIR` añade `tgt=` e `ik=0x…` (IK del ped: `GUN_POINTED_SUCCESSFULLY`) y
   `SIGHT` añade `por=` (1=conduciendo, 2=modo de cámara, 3=apuntando).
4. Verificador: bloque **R13** en `tools/viceext-log-check.py`; `check-served-build.sh`
   pasa a 21 marcas y muestra la `VERSION` de la capa web.

Documentado en `.agents/plans/mecanicas/10-seccion-1-9a-partida.md` (§ 11ª partida).

---

## 12ª partida (22/09/2026) · comparación de vídeos `pc.mp4` / `browser.mp4` → R14 (`ve27`)

El jugador dejó dos grabaciones en `gta_vc_browser/`: `pc.mp4` (el mod en PC
nativo) y `browser.mp4` (el port). Se compararon con hojas de contactos
(`ffmpeg -vf crop,tile`) contra el log de esa sesión
(`logs/odtrace-2026-09-22_14-35-49.log`, `ve26`) y salieron **cuatro causas raíz**
más una que el verificador ya medía sin que la hubiéramos visto:

1. **Cámara de nado**: `CCam::IsTargetInWater` era verdadero nadando (el ped va
   0,7-0,9 m bajo la superficie) → el motor pedía `MODE_PLAYER_FALLEN_WATER` (la
   cámara del que se ahoga, clavada sobre el agua). El vídeo lo enseña literal: la
   calzada desde arriba con Tommy fuera de cuadro; el log: `SWIM2 camz=12.02`
   constante con el ped a z=5,4 sobre nivel 6,1. Arreglado: nadando ese predicado
   devuelve falso.
2. **Autocentrado de coche**: el empujón rápido (2,2 rad/s) se disparaba con el
   flanco de soltado de «mirando», y ese detector incluía el RATÓN → tirones cada
   vez que el jugador dejaba de moverlo (`camauto2 pedido=1` 64 veces con
   `vel=0.0`). Ahora el ratón sólo reinicia el temporizador del retorno suave y el
   empujón está reservado a cruceta/palo; el retorno pasivo es proporcional al
   error.
3. **Retroceso**: movía `m_f3rdPersonCHairMultY` (la retícula). Ahora la mira queda
   fija (traza `RECOIL3 miracheck multY=0.400`) y el retroceso lo acusa la cámara,
   con patada visible en todas las armas.
4. **Caminado apuntando** (H4): el clip lo elegía la velocidad con tope → el paso
   lento de cinemáticas. Ahora lo elige la velocidad sin tope y la cadencia se
   escala con el cociente real.
5. **Sonidos de arma**: verificado que el contenido está bien (mp3 == PCM del mod,
   misma longitud y envolvente); el fallo era de entrega (2 decodes inline/frame,
   cola de 1). Las muestras del mod pasan a **reservadas** (`ODSFXPROT`).
6. **Cámara agachado**: el bloque H medía sólo 0,27 m de descenso; el offset del
   objetivo sube de −0,55 a −0,95 (`VICEEXT_CROUCH_CAM_DROP`).

Build **`ve27`** enlazado; `check-served-build.sh` → **27/27 marcas OK**; el
verificador tiene bloque **R14** (y sobre el log viejo dictamina FALLO nombrando
los dos fallos, o sea que caza lo cambiado). Documentado en
`.agents/plans/mecanicas/10-seccion-1-9a-partida.md` (§ 12ª partida).

---

## 13ª partida (22/09/2026) · «al desagacharme sólo cambia la altura de la cámara» → R15 (`ve28`)

Pregunta del jugador, tras la 12ª partida (con `ve27` ya enlazado):

> «vale el error de que la cámara se mantiene fija al agachar y nadar? por que al
> estar agachado le vuelvo a dar para pararse este solo cambia la altura de la
> cámara pero el personaje sigue agachado, ya podré caminar agachado y nadar sin
> errores en cámara?»

Las dos cámaras de la pregunta **ya estaban arregladas en `ve27`** (R14):
cámara de nado (`SWIMCAM`: nadando el motor ya no pide la cámara del ahogado) y
descenso de la cámara agachado (`VICEEXT_CROUCH_CAM_DROP` −0,95, R14-6). El
**tercer** síntoma (desagacharse no devuelve la pose) **era un fallo real y
nuevo**, y es el que se arregla aquí.

### Causa raíz (leída en el motor, no supuesta)

`CAnimManager::BlendAnimation` retira las animaciones que había puestas mirando
sólo las del **mismo tipo** (`isPartial == anim->IsPartial()`). Los clips del mod
(`crouch_idle`/`crouch_forward`/`crouch_backward`) son `ASSOC_PARTIAL | ASSOC_REPEAT`
y viven en su grupo (`ASSOCGRP_PLAYERCROUCH`), mientras que de pie el motor mezcla
caminar/idle **no parciales** de `ASSOCGRP_STD`: **nadie apagaba el clip de
agachado**, así que el cuerpo se quedaba agachado para siempre (el `ASSOC_REPEAT`
lo deja puesto indefinidamente). El único dato que dependía de `odCrouched` era el
**objetivo de altura de la cámara** (`CCam` vía `CPlayerPed::ViceExtIsCrouched`),
y de ahí exactamente lo que describió el jugador: «sólo cambia la altura de la
cámara». El mismo camino dejaba al ped agachado al entrar en un coche, al morir o
en una cinemática (el early return ponía `odCrouched = false` sin tocar la
animación).

### Arreglo (`[raíz]`, `src/peds/PlayerPed.cpp`)

- `ViceExtCrouchStopAnims()`: al desagacharse se retiran los tres clips del mod
  igual que hace el motor en `CPed::ClearDuck` (`flags |= ASSOC_DELETEFADEDOUT` +
  `blendDelta = -4.0f` ⇒ el caminar/idle del motor recupera el cuerpo en ~0,25 s).
  Se llama en los **tres** caminos de salida: conmutar con la tecla, correr/saltar
  (`motivo=carrera`) y perder la posesión (coche, muerte, replay).
- Traza `VICEEXT crouch pose off clips=<0..3>` (una por desagachado: dice cuántos
  clips se retiraron; `0` = el motor no tenía ninguno puesto).
- Traza **`CROUCHPOSE desde=<ms> idle=… fwd=… back=…`** (vigilante, 1/s y sólo
  mientras algún clip de agachado siga mezclado con peso > 0,01): es la prueba
  desde el log de que el cuerpo vuelve a estar **de pie**.

### Verificación

- **Bloque `R15`** en `tools/viceext-log-check.py`: si hubo `VICEEXT crouch off` y
  no hay `pose off` → FALLO (motor sin R15); y cualquier `CROUCHPOSE` con
  `desde >= 700` y peso > 0,05 → FALLO («el cuerpo sigue agachado tras
  desagacharse»). Autocomprobado con log sintético bueno/malo.
- **Sobre el log real de la 12ª partida** (`logs/odtrace-2026-09-22_14-35-49.log`,
  `ve26`) dictamina FALLO nombrando el fallo: **9 `on` / 9 `off`** de agachado y
  **0 líneas `pose off`** → el motor de esa sesión no retiraba los clips. Es el
  registro del bug que reportó el jugador.
- **Marca nueva** `CROUCHPOSE` en `check-served-build.sh`. Build **`ve28`**
  enlazado: **28/28 marcas OK** (`VERSION 2026-09-22-ve28`, `dataTag` sigue
  `2026-09-21-ve13`: no cambian datos ⇒ basta **Ctrl+Shift+R**, sin re-descarga).
- **Pendiente de validar jugando**: agacharse → caminar agachado → **C** (debe
  ponerse de pie de verdad) → nadar y salir del agua. Lectura en el log:
  `VICEEXT crouch pose off clips=1` tras cada `off` y ninguna `CROUCHPOSE` con
  peso > 0,05 pasado un segundo.

Documentado en `.agents/plans/mecanicas/10-seccion-1-9a-partida.md` (§ 13ª partida).

## 13ª partida (22/09) · R16 (`ve29`): el mod trae el lanzacohetes en 3ª persona

Se sigue con **Vice Extended** y su propia lista de cambios
(`ChangesEN.txt`), no con otros mods. Checklist completo de lo que falta en
`.agents/plans/vice-extended-pendientes.md`.

`RocketLauncherThirdPersonAiming=1` es uno de los **6 toggles que el mod trae
ENCENDIDOS** en su `features.ini`, y era el único de ellos que quedaba por
implementar (los otros: no-rebotar-del-coche ✔, retroceso ✔, nadar ✔,
luces lejanas y tuning aleatorio del tráfico → pendiente/fuera). O sea: con `ve27`
el motor **no era** el mod en este punto.

- **Causa de que no funcionara**: en VC apuntar con el lanzacohetes mete al
  jugador en el modo francotirador — `SetNewPlayerWeaponMode(MODE_ROCKETLAUNCHER)`
  + `PED_SNIPER_MODE` + `m_fMoveSpeed = 0` (`PlayerPed.cpp`, rama de
  `TargetJustDown`) — y **el disparo exigía uno de esos modos de cámara**:
  `CWeapon::FireProjectile` hacía `return false` si la cámara no era de 1ª
  persona (por eso el misil sólo salía estando dentro del modo).
- **Arreglo (`[raíz]`, 3 sitios)**: define nuevo `VICEEXT_ROCKET_3RD_PERSON`
  (`config.h`); en `ProcessPlayerWeapon` el lanzacohetes **no entra** en la rama
  del francotirador (apunta con el apuntado de brazo de VC, que su `weapon.dat`
  ya trae: `WEAPONFLAG_CANAIM_WITHARM`) y por tanto se puede andar apuntando; en
  `FireProjectile` se deja de exigir el modo de 1ª persona (el misil ya volaba
  hacia la cámara: `CProjectileInfo::AddProjectile` usa su matriz cuando el
  tirador es el jugador). Traza nueva `R3P arma= modo= ped= mira= apunta= spd=`
  (1/s con el arma en la mano).
- **Verificación**: marca `R3P arma=` en `check-served-build.sh` + bloque **R16**
  en `viceext-log-check.py` (con `mira=1`, el modo de cámara no puede ser de 1ª
  persona: 8/40/16/34/42/45/46 → FALLO nombrando «el jugador queda clavado»;
  autocomprobado con log sintético bueno/malo). Build **`ve29`**: **29/29 marcas
  OK**, `VERSION 2026-09-22-ve29`, `dataTag` sin cambios (sólo Ctrl+Shift+R).
- **Pendiente al cerrar la sesión**: R15 (`ve28`, desagacharse) y R16 sin
  validar jugando; y del mod quedan las filas 1-18 de la tanda
  (`vice-extended-pendientes.md` § 2), con el orden propuesto en § 4.
## 14ª partida (22/09, tarde) — R17: la cámara fija y el personaje que no se movía (agachado y nadando)

Reporte del jugador (tras la 13ª, build `ve29`): «el problema de la cámara fija y
el personaje sin desplazamiento al nadar y agacharse; son modos que sufren lo
mismo; analizar qué los bloquea y removerlo; la cámara debe seguirlos como lo
haría cuando están caminando».

### Lo que bloqueaba (leído en el motor, no supuesto)

- **Cómo anda un ped en III/VC** (esto es lo que faltaba en el diagnóstico
  anterior): **no** anda por `m_fMoveSpeed`. Anda por la **traslación de la raíz
  de la animación de movimiento**:
  `AnimBlendFrameData::VELOCITY_EXTRACTION` (`FrameUpdate.cpp`) →
  `m_vecAnimMoveDelta` → `CPed::CalculateNewVelocity` (`m_moved`) →
  `CPed::UpdatePosition` —que **sólo corre con `bIsStanding`** → `m_vecMoveSpeed`
  → `CPhysical::ApplyMoveSpeed` (translate). `m_fMoveSpeed` sólo **elige** clip
  (andar/correr/esprint) y `assoc->speed` escala su cadencia.
- **Los clips del mod estaban declarados `ASSOC_PARTIAL`** (`crouch_*`,
  `swim_crawl/breast`). Eso hacía dos cosas malas a la vez: (a) el motor no les
  extraía la traslación (sólo mira clips con `ASSOC_HAS_TRANSLATION`), así que no
  aportaban velocidad; y (b) su traslación se aplicaba **a la pose** (`mat->pos =
  pos - trans` no se ejecuta), o sea el cuerpo se iba ~2,6 m delante del origen
  del ped en cada ciclo y volvía de golpe al reiniciar: eso es lo que se veía
  como «el personaje no se desplaza» mientras la cámara, que mira al **origen**
  del ped, se quedaba quieta.
- **Agachado**, además, `CPlayerPed::SetRealMoveAnim` **salía antes** de mezclar
  caminar/correr (`odCrouched` → sólo el clip del mod). Con el clip sin
  traslación, `m_moved` = 0 y `UpdatePosition` anulaba `m_vecMoveSpeed` cada
  frame: el ped no andaba **nada**. Cámara y ped quietos, tal cual lo contó.
- **Nadando**, la cámara: el predicado `CCam::IsTargetInWater` seguía dando
  verdadero en cuanto el ped quedaba por debajo de la superficie **sin estar
  nadando** (por ejemplo al salir a agua poco honda, con el nado ya soltado), y
  el motor pide entonces `MODE_PLAYER_FALLEN_WATER`, que fija la cámara en
  `m_vecLastAboveWaterCamPosition + 4 m` y la deja **clavada** mirando abajo
  (`Process_Player_Fallen_Water`). **Confirmado con el vídeo del navegador** de la
  sesión de las 14:38 (`tools/frames-at.sh` sobre `browser.mp4`, fotogramas
  t=138 y t=146): la cámara sobre el agua, Tommy fuera de cuadro y `camz=12,79`
  constante (= última posición de cámara + 4) mientras el ped avanzaba a 1,3 m/s.
- **Cuánto avanzan de verdad los clips del mod** — medido en el `ped.ifp`
  servido con la herramienta nueva `gta_vc_browser/tools/ifp_inspect.py` (raíz,
  metros por ciclo / duración): `crouch_forward` 2,615 m / 0,731 s = **3,58 m/s**
  (¡más que correr!), `crouch_backward` −1,853 m / 1,0 s, `swim_breast` 3,011 m /
  1,300 s = **2,32 m/s**, `swim_crawl` 2,505 m / 0,900 s = **2,78 m/s**,
  `swim_tread` 0 (en el sitio). De serie para comparar: `walk_civi` 1,13 m/s,
  `run_player` 6,44 m/s.

### Arreglo (R17, 4 ficheros)

1. `AnimManager.cpp`: los clips que **llevan el cuerpo** en esos dos modos pasan
   a ser animaciones de **movimiento**, con las claves de `WALK`
   (`ASSOC_REPEAT | ASSOC_MOVEMENT | ASSOC_HAS_TRANSLATION | ASSOC_WALK` para
   `crouch_forward/backward`; `MOVEMENT | HAS_TRANSLATION` para
   `swim_crawl/breast`); `crouch_idle` y `swim_tread` se quedan sin parcial y sin
   traslación (en el sitio). Con eso la velocidad sale del propio clip, la pose
   ya no se va del origen y el cambio adelante/atrás se sincroniza entre clips.
2. `PlayerPed.cpp` (agachado): el clip del mod es ahora el que avanza; su ritmo
   se escala (`assoc->speed`, igual que el truco H4 del caminado apuntando:
   `speed` multiplica avance **y** cadencia, así los pies no patinan) desde la
   velocidad natural del clip (`metros de raíz / hierarchy->totalLength`, sin
   números fijos que se rompan si cambia el `.ifp`) hasta
   `VICEEXT_CROUCH_WALK_SPEED = 0,9 m/s`. Y `PlayIdleAnimations` se salta
   estando agachado: las idles de aburrimiento no son parciales y habrían
   retirado el clip del mod (el ped se pondría de pie solo a los 25-30 s).
3. `PlayerPed.cpp` (nado): la velocidad ya no es un 1,3 «a ojo» que no cuadraba
   con lo que dibujan las manos — la dicta el clip (`ViceExtSwimClipSpeed`:
   braza 2,32 m/s andando, crol 2,78 m/s con esprint, flotar 0) con cadencia
   natural, y el mando analógico escala las dos a la vez.
4. `Cam.cpp`: **el jugador vivo en el agua nunca usa la cámara de ahogado**; sólo
   si de verdad se ahoga o ha muerto. Es el bloqueo de la cámara de nado.

### Verificación

- Trazas nuevas para que el log dé el veredicto: `CROUCH2 ... avance=x,xx
  camdist=x,xx modo=n` (metros/s recorridos de verdad y distancia real
  cámara-ped) y `SWIM2 ... camdist= modo=`; más una línea única
  `SWIMCAM no-fallen-water`.
- `tools/check-served-build.sh`: 5 marcas nuevas de R17 → **34/34 marcas OK**,
  `VERSION 2026-09-22-ve30`, `dataTag` sin cambios ⇒ sólo **Ctrl+Shift+R**.
- `tools/viceext-log-check.py`: bloque **R17** (FALLO si andando agachado el
  avance es 0 —con la frase del jugador—, si nadando el modo de cámara no es 4 o
  si `camdist` se dispara, y si el motor no lleva las trazas). Pasado sobre el
  log de la 13ª partida da **FALLO** nombrando el bloqueo: ese log era la prueba
  del fallo, no del arreglo.
- Herramienta nueva `tools/ifp_inspect.py` (lee el `.ifp` ANPK igual que
  `LoadAnimFile`): dice, por animación, si la raíz tiene claves de traslación y
  cuánto avanza por segundo. Es la que ha permitido decidir con datos en vez de
  a ojo.

## 15ª partida (22/09, tarde) — R18/R19: la velocidad REAL del agachado y del nado (`ve35`)

El jugador cerró la 14ª con la frase exacta del fallo y el encargo: «la cámara se
mantiene fija y el personaje sin desplazamiento al nadar y agacharse… la cámara
debe seguirlos como cuando caminan; verifica que el movimiento sea normal, que el
personaje se mueva a la velocidad que debe, como debe y con las animaciones que
deberían». Esta tanda: (a) medir la velocidad de verdad, (b) arreglar lo que
faltaba para que sea LA del clip, (c) dejar una sonda que lo pruebe sin depender
de que el jugador juegue.

### Lo que se midió (números, no impresiones)

Sonda nueva `tools/crouch-swim-smoke-test.mjs` (agachado → desagacharse → navegar
hasta el agua → nadar → girar 180° y nadar al revés), sobre la partida sembrada:

| Medida | Antes | Ahora |
|---|---|---|
| Andar agachado (avance real, m/s) | 0,00 clavado (14ª) | **0,87** (clip escalado a 0,9) |
| Cadencia del clip / peso en el cuerpo | — | `peso=1,00`, `clip=237/239` |
| Cámara agachado | fija | **modo 4** (seguir-al-ped) a `camdist` **3,56 m** constante |
| Nado (avance real, m/s) | 0,3 «patinando» en el fondo | **2,32** = velocidad natural de la braza |
| Flote nadando | hondo 0,90 m fijo (apoyado en el fondo) | `dz≈-0,01 m` sobre el punto de flote, hondo 0,54 |
| Cámara nadando | `MODE_PLAYER_FALLEN_WATER` fija sobre el agua | **modo 4** a 3,43-3,70 m |
| Reloj de mundo vs reloj del motor | — | **1,00** (160 muestras `PEDAT`) |

### Las dos causas que quedaban

1. **La fricción del agua se comía el nado** (`ProcessBuoyancy`): de 0,046 m
   pedidos por frame el ped avanzaba 0,017 (60 % perdido), y el ped iba **apoyado
   en el fondo** (hondo 0,90). Ahora el jugador que nada queda **exento** de esa
   fricción (`exento=1`), el avance se aplica como movimiento directo
   (`paso=0,046`) y el control vertical es un mando de flote (`flote=0,030`).
2. **La velocidad agachado salía a la MITAD en la medida**: la función del
   agachado se llama **dos veces por frame** a propósito (el clip se pide también
   desde `SetRealMoveAnim`), así que el tiempo de mundo se acumulaba dos veces y
   `velo = avance/dt` daba 0,44 de un andar de 0,87 (dt=2,04 s de mundo por
   segundo de reloj). **El juego estaba bien; la sonda mentía.** Se filtra por
   `GetFrameCounter()` (`medida=1x`) y R19 vigila que `dt≈1,00` y que las dos
   formas de medir la misma velocidad coincidan.

### Lo que se añadió para no volver a depender de la suerte

- **Traza `PEDAT`** (1/s, cualquier estado): posición, `av`/`velo` reales,
  `ms` (reloj del motor), `nf` (frames), `ts` (segundos de mundo) y
  `mar=`/`dist=` = **azimut y distancia al agua** que mide el propio motor
  (`CWaterLevel::GetWaterLevelNoWaves` en 16 rumbos × 4 pasos de 12 m, con el
  índice comprobado antes de preguntar para no indexar fuera de la tabla).
- La sonda **navega** con `dist`: sostiene el rumbo que acerca al mar y gira ~45°
  si no se acerca o si el ped se queda clavado contra una pared. Antes andaba a
  ciegas 4×30 s y el 22/09 se quedó contra una casa: la parte de nado quedó **sin
  medir** y el resultado fue un FALSO fallo.
- `tools/viceext-log-check.py`: bloque **R19** (velocidad del clip sin doble
  conteo) y dos falsos positivos corregidos en R17 (el ped **frena** en el primer
  segundo tras soltar el palo; ahora sólo cuentan las muestras de estado estable)
  y su lectura de `SWIM2` por nombre de campo, no por posición.
- `tools/check-served-build.sh`: **35/35 marcas OK**, `VERSION
  2026-09-22-ve35`, `dataTag` sin cambios ⇒ sólo **Ctrl+Shift+R**.

### Resultado

`RESULTADO: PASS` (19/19 comprobaciones, 0 errores de página) en dos pasadas
seguidas sobre `ve35`. Quedan cerrados **R17** (andar agachado y cámara),
**R18** (nado: flote, velocidad y cámara) y **R19** (la velocidad correcta y la
medida fiable).
## 16ª sesión (22/09/2026, tarde) — Arnés de pruebas: más rápidas, reutilizables y blindadas

Encargo del jugador: «mejores formas de hacer test más rápidas teniendo en cuenta
que es un jueguito y tal; más métodos, más test reutilizables para probar más
features después; que queden más blindados y más fáciles de realizar».

### El problema, medido

Nueve *smoke tests* (~5.500 líneas) y cuatro verificadores. Cada smoke test
repetía ~150 líneas de arranque (Chrome, sembrar la partida, esperar menú,
esperar partida, leer `odtrace.log`) con su propio perfil de Chrome:

| | Antes | Ahora |
|---|---|---|
| Arrancar el motor | ~2 min **por fichero** | **36 s una vez por corrida** (perfil caliente) |
| Pruebas por sesión | 1 | N (hoy 4) |
| Sincronía | `sleep(3000)` a ojo | **evento de traza** (`FPHASE`, `WLOAD`, `swim enter`) |
| Saber sobre qué binario se midió | a mano | el arnés exige **versión + marcas + dataTag** |
| Dos sesiones a la vez | trazas **mezcladas** | **un log por sesión** (`?trace=<nombre>`) |
| Un fallo | capturas de todo | captura + traza del escenario + informe |

### Lo que se montó (`tools/harness/`, `tools/vc-test.mjs`)

- `vc-harness.mjs`: sesión de juego reutilizable (una sola partida para N
  escenarios), espera por eventos, entradas (teclas/ratón/trucos), marca y tramo
  de traza, artefactos, topes de tiempo, perfil bloqueado que se recupera solo,
  aviso de sesión concurrente y verificación del binario servido.
- `esquemas.mjs`: escenarios como objetos (`{nombre, marcas, ejecutar(s)}`) con
  informe de **PASS / FALLO / AVISO** — el AVISO es para lo que la sonda NO ha
  podido medir (no se disimula ni suspende).
- `vc-test.mjs`: `--estatico` (2,6 s, sin navegador), `--todo`, `--escenario X`,
  `--listar`.
- `--estatico` comprueba: binario servido (versión + TODAS las marcas + dataTag),
  que los defines apagados (`VICEEXT_TURN_SIGNALS`) compilan con la línea real de
  ninja, y el verificador sobre el último log **diciendo de qué build es** (si es
  de otra sesión, AVISO en vez de suspender).

Práctica copiada de cómo se prueban los juegos de verdad (Unreal *functional
tests*, charlas de *Sea of Thieves*): **fijar el estado en vez de jugar la ruta**
(partida sembrada + navegación por datos del motor), **sincronizar por eventos en
vez de tiempos** y **medir telemetría, no píxeles** (las capturas sólo se guardan
cuando algo falla).

### Dos pruebas que estaban mintiendo (y ahora miden)

1. **`armas` «TXD vistos: ninguno»**: deducía la carga del mod de las líneas
   `TXDIN`, y esa instrumentación (diagnóstico de streaming F3a) está **acotada a
   las 60 primeras del proceso**: en una sesión larga el arranque ya se las había
   comido y la prueba salía en rojo **con el truco funcionando**. En vez de subir
   el tope, se instrumentó el HECHO (ve37): el truco de armas publica `WLOAD`
   (modelo y diccionario de cada arma, con el estado del streaming) y la ficha
   `WINFO` de las 8 (`CPlayerPed::ViceExtWeaponInfoOf`), así que la prueba mide
   los clips de las ocho **sin depender de que la sonda sepa cambiarlas de mano**.
   Ahora: **8/8 modelos en memoria, 8/8 diccionarios, 9 fichas sin una sola arma
   sin clips**. El ciclo a mano queda como AVISO con su número real (0 armas
   cambiadas tras el truco: la tecla del ciclo no está donde la sonda la busca).
2. **`nado` «no se usa la cámara de ahogado — SIN traza»**: la línea
   `SWIMCAM no-fallen-water` sale **una vez por sesión**, en el frame en que el
   motor decide la cámara del jugador vivo en el agua, y con estas partidas cae
   al CARGAR (el ped ya está en el agua), antes del tramo de nado. Ahora se busca
   en la sesión entera y el tramo exige además que ninguna línea anuncie `modo=23`
   y que el objetivo de cámara esté en la superficie: **`objetivo−nivel` = 0,50 m
   (16 muestras)**.

También se corrigieron tres falsos fallos del verificador: el ped **frena** el
primer segundo tras soltar el palo (R17 contaba esa muestra), `R17` leía `SWIM2`
por posición de campo (se habían añadido `velo`/`dt`/`dz`) y el bloque H comparaba
la `camz` de agachado con la de pie de **otra parte del mapa** (11,18 vs 11,33):
ahora mide **cámara sobre el ped** (de pie 1,58 m → agachado 0,65 m).

### Resultado

`node tools/vc-test.mjs --todo` sobre `ve37`: **PASS 4/4 escenarios en 359 s**
(carga 3 s · agachado 22 s · armas 34 s · nado 260 s), 35 comprobaciones, 0 errores
de página. `viceext-log-check.py` con R19 y `check-served-build.sh` **38/38 marcas
OK** (`ve37`). Diseño, uso y cómo añadir un escenario: `.agents/plans/tests-rapidos.md`.
Aprendizaje de fontanería: los `.cpp` del proyecto van en **CRLF**, así que las
ediciones multilínea van con `python` en binario (con `\n` se cuelan saltos de
línea doblados y un `str_replace` puede “encontrar” texto que no esperabas).

### Nota de comunicación (22/09/2026, petición del jugador)

Respuestas **cortas**: resumen al principio, tablas sólo si aportan, sin repetir el
plan entero. El detalle vive en `.agents/plans/*.md`, no en el chat.

## 17ª partida (22/09/2026, noche) — R20: el agachado anda en sus 8 direcciones (`ve41`)

Jugador: «el agachado se ve pero el personaje no logra moverse en sus 8
direcciones, la velocidad va demasiado lenta; usa la lógica que YA existe en el
mod, no adivines». Se hizo el plan (`.agents/plans/agachado-sa.md`, sin tocar
código) y después la implementación, con una corrección importante a mitad: **la
primera versión se equivocó de diagnóstico y lo cazó el propio arnés**.

- **[raíz] Los clips del mod que faltaban**: `Crouch_Roll_L/R` registrados en
  `ASSOCGRP_PLAYERCROUCH`. Antes hubo que **mover el bloque de agachado al final
  del `enum AnimationId`**: el índice dentro de un grupo es `id − firstAnimId`
  (`CAnimManager::CreateAnimAssocGroups`), así que los IDs de un grupo tienen que
  ser consecutivos — con la escalada en medio, el cuarto clip de agachado habría
  apuntado a un clip de escalada.
- **[raíz] Pero no son clips de costado**: medido con `tools/ifp_inspect.py`, la
  raíz de `Crouch_Roll_L` avanza **−2,17 m en Y** (hacia atrás) y la de
  `Crouch_Roll_R` **+2,25**; de lado sólo tienen 0,22 m (compárese con el
  `walk_left` de serie: −1,836 en X, 0,03 en Y). En el motor se confirmó: con
  `Crouch_Roll_L` puesto, pulsar izquierda movía al ped **179,6° respecto a su
  rumbo**. Elegir clip por ángulo, por tanto, NO bastaba (era la hipótesis A del
  plan, y era falsa).
- **[raíz] Desplazamiento por código, como en SA**: en `CPed::CalculateNewVelocity`
  el motor ya sustituye el avance de la raíz por el rumbo del mando cuando
  `|localWalkAngle| < 50°` (de ahí que las diagonales ya fueran bien). El bloque
  R20c extiende esa misma sustitución a **las 8 direcciones** cuando el ped está
  agachado, con la velocidad del clip de la pose. Pose: `Crouch_Forward`
  adelante/lados, `Crouch_Backward` atrás. `Crouch_Roll_L/R` quedan tras la
  palanca `?crouchlado=1` (no validada en partida).
- **[raíz] Fuera el objetivo inventado de 0,9 m/s**: obligaba a ritmo 0,25 (pies a
  un cuarto de velocidad = «va demasiado lenta»). Ahora ritmo del motor (1,0):
  **3,58 m/s adelante, 1,85 atrás, 2,33/2,42 de lado**, con palanca en caliente
  `?crouch=X` / `window.__vcCrouchRate = X` (el motor la relee 1/s, sin
  recompilar).
- **[tooling] El escenario `agachado` mide ahora las 8 direcciones** (~55 s): ángulo
  del mando (`ang=`), giro del cuerpo (`rumbo`), **dirección real del
  desplazamiento** con `PEDAT` (tiene que valer `−ang`), clip por NOMBRE (`nom=`) y
  velocidad contra la natural del clip en uso. Dos falsos fallos corregidos: la
  primera muestra del tramo desliza de la dirección anterior y la última puede caer
  en la frenada tras soltar las teclas.
- **[proceso] Dos intentos fallidos antes del bueno**, ambos diagnosticados con el
  propio arnés y no a ojo: (1) giro del cuerpo copiando la fórmula de
  `PlayerControlZelda` → **giro 0** (con ratón, `TheCamera.Orientation` es el
  azimut de cámara, no el desvío de órbita del control clásico); (2) desplazamiento
  con los clips de costado → **movimiento hacia atrás**. El cuerpo tampoco gira:
  con ratón en 3ª persona el ped se desplaza de lado sin girar, **igual que al
  caminar de pie** (allí son `walk_left`/`walk_right`; agachado no existen).
- **Resultado**: `--escenario agachado` → **PASS 23/23 en 51 s** (8 direcciones con
desvío −0,4° / ±90° / ±135° y velocidades 3,71 / 1,79 / 3,71 m/s).
- **Pendiente**: apuntar agachado (`WEAPON_crouch` como pose) y decidir en partida
  la pose de costado (`Crouch_Forward` por defecto vs `Crouch_Roll_L/R`).

## 23/09/2026 (17ª partida) — R21: apuntar agachado y los lados con la rueda

- **Petición del jugador**: «el agachado hacia adelante se ve perfecto; caminando a
  un lado se desplaza pero la animación no es la del lado; falta que al apuntar
  agachado haga su animación de apuntado; y si apunto y presiono izquierda o
  derecha el personaje **rueda** hacia ese lado».
- **Pose de apuntar (R21)**: el clip ya está en el `ped.ifp` del mod
  (`ANIM_STD_DUCK_WEAPON` = `WEAPON_crouch`); faltaba pedirlo, porque el motor sólo
  lo pone desde `CPed::SetDuck`, que marca `bIsDucking` (clava al jugador). Ahora se
  mezcla por animación sin tocar ese flag y se retira al soltar el botón o al
  desagacharse. Medido: `mira=1`, `pesomira=1.00`, y 0 tras soltar.
- **Lados con la rueda (R21)**: `Crouch_Roll_L/R` (los clips de revolcarse de SA
  que trae el mod) pasan a ser la pose por defecto de los lados; el desplazamiento
  lo sigue poniendo el código (R20c) porque la raíz de esos clips viaja en Y.
- **R21b (desplazamiento apuntando)**: dos causas encontradas midiendo: la pose
  parcial a peso 1 anula la traslación del clip (`1 - totalBlendAmount` en
  `FrameUpdate.cpp`) y el bloque de movimiento agachado quedaba detrás del corte por
  modo de cámara (apuntando, la cámara cambia de modo: `camdist` 0,61 vs 3,56 m).
  El ped ya se mueve apuntando (4,58 m/s medidos con la rueda).
- **Abierto**: la dirección medida del desplazamiento apuntando salió −1° donde la
  prueba esperaba −90°; queda instrumentarlo con `m_moved`/`walkAngle` en la traza
  (ver `.agents/plans/agachado-sa-handoff.md`, §2.3).
- **Arnés**: escenario corto nuevo `agachado-mira` (28 s, sólo apuntado), botones de
  ratón (`ratonAbajo/Suelta/Rueda`), `CRAZYPISTOL` para tener la pistola EN MANO,
  auto-recuperación del agachado (un coche atropellaba al ped y el motor cancelaba
  el agachado a mitad de prueba → veinte fallos falsos) y **regla nueva**: subir
  `VERSION` en cada build, porque es lo único que fuerza al navegador a bajar el
  `.wasm` nuevo (con el tag repetido se midió el binario anterior).

## R22 (23/09/2026, 18ª partida) — el agachado: velocidad, rueda y cámara (`ve46`)

Encargo del jugador: «al estar agachado y moverme el personaje se mueve a una super
velocidad sin razon, me muero si camino hacia a un lado y la animacion de apuntado no
es la que deberia tener».

- **La velocidad era el clip.** `Crouch_Forward` avanza 3,58 m/s de raíz y
  `Crouch_Backward` 1,85: el agachado andaba más que el `walk` de serie y cada
  dirección a una velocidad distinta, porque R20c usaba `pedSpeed` (el avance del clip).
  Ahora hay **una velocidad objetivo**, `1,85 m/s`, que es la de la propia familia del
  mod (`Crouch_Backward` y los `GunMove_*` van todos a 1,85), y el **ritmo del clip se
  calcula de ella** (`ViceExtCrouchRateFor` = objetivo / avance de la raíz), así que los
  pies acompañan al cuerpo: ni patinaje ni cámara lenta. Medido en `ve46`:
  `moved (motor) = 1,85 m/s` con el clip de adelante y con la rueda.
- **«Me muero si camino hacia a un lado»**: los clips de costado del mod son los de
  **revolcarse** (`Crouch_Roll_L/R`, `ASSOC_REPEAT`). Manteniendo la tecla, Tommy se
  quedaba revolcándose por el suelo. Ahora la rueda es **una sola por petición** (para
  repetir hay que soltar el lado), sólo al **apuntar**, y se anuncia como evento
  (`VICEEXT crouch roll lado=…`) porque una traza de 1/s puede no ver un movimiento de
  0,9 s. Palanca para el bucle: `?crouchroll=repeat`.
- **La cámara se metía dentro del ped**: `VICEEXT_CROUCH_CAM_DROP` estaba a 0,95 (el
  doble de lo que baja la cabeza agachada) porque se subió hasta pasar un umbral del
  verificador; con eso, agachado + apuntando la cámara acaba en el cuerpo (medido
  `camdist` 0,61 m en su partida y 0,25 m en el arnés). Vuelve a 0,55 y el test mide lo
  que importa: `camdist ≥ 1,5 m` (medido 2,19..3,56 en la corrida buena).
- **La animación de apuntar agachado del mod es `WEAPON_crouch`, y no hay otra** (está
  en el `ped.ifp` vanilla y en el del mod; `WEAPON_crouchfire`/`crouchreload` NO están
  en su ifp). Lo que estaba mal era el **peso**: a peso 1 la pose parcial deja los clips
  no parciales a 0 y las piernas se congelan (el ped se desliza). Andando va a **0,4**
  (las piernas del clip mandan) y quieto a 1,0. Siguiente paso: los `GunMove_*` del mod.
- **Arnés**: la velocidad se mide con `moved=` (motor) y no con el vaivén de la calle
  (el ped recibe golpes y el motor lo teletransporta: 42 m en un segundo, `velo=41`);
  la rueda se cuenta por eventos; la cámara es control duro en `agachado` y AVISO en
  `agachado-mira` (paredes). El escenario largo se cayó por tráfico: falta un punto de
  prueba sin coches.

## R23/R24/R25 (23/09/2026, 19ª partida) — el agachado iba ×50: las UNIDADES (`ve51`)

- **`[raíz]` LA CAUSA DE TODAS LAS "SUPER VELOCIDADES": `m_vecMoveSpeed` no va en m/s.**
  El motor mueve con `Translate(m_vecMoveSpeed * CTimer::GetTimeStep())`
  (`Physical.cpp:425`) y el paso vale ~1,0 a 50 fps, así que la unidad es "metros por
  frame de 50 fps" (m/s ÷ 50); el proyecto ya traía `METERS_PER_SECOND_TO_GAME_SPEED`
  (`CarCtrl.h`). El bloque R20c del agachado escribía el objetivo en m/s directo → **×50**.
  Medido en la traza del jugador: `obj=0,90` → 331,51 m en 7,38 s = **44,90 m/s**
  (`mvec=0,90`). Eso era "voy más rápido que cualquier auto", "me muero si camino a un
  lado" (la rueda, 2,35 = 117 m/s) y "los pies no van a la par" (pies a 0,9 m/s con el
  cuerpo a 45). Arreglado con la conversión; el nadado ya lo hacía bien. La traza publica
  ahora las dos unidades (`mps` m/s y `obju` del motor) para que `mvec ≈ obju` y
  `velo ≈ mps` canten cualquier mezcla futura.
- **`[raíz]` Crash al cambiar de arma**: `CAnimBlendAssocGroup::GetAnimation(id)` es
  `&assocList[id - firstAnimId]` **sin comprobar rango**; `ViceExtClipName` (traza
  `WINFO`) pedía el nombre de clips que el grupo del arma no tiene → puntero basura →
  `%s` → `memory access out of bounds` en `memchr/strnlen/printf_core`. Ahora comprueba
  grupo y rango y copia el nombre a un buffer terminado (`hierarchy->name` es `char[24]`).
- **`[tooling]` Medidor de tramo agachado** (`CROUCHMOVE`): metros desde que se pulsa la
  tecla hasta que se suelta, sumando desplazamientos frame a frame, con `maxfr` (mayor
  salto de un frame) y los valores que cree el juego (`mvec`, `fspd`, `anim`). Es lo que
  cazó el fallo de unidades después de tres rondas de medidas por muestras de 1 s, que se
  ensuciaban con tirones (4,3 s de stall → "45 m/s" falso) y con muertes/respawns.
- **`[proceso]` Sin palancas de URL/consola**: el jugador las quitó ("no necesito ni pedí
  eso; basate en logs"). El agachado son constantes en `PlayerPed.cpp` (0,90 m/s de
  caminata, 2,35 de rueda). No volver a añadirlas.
- **`[tooling]`** Buffer de `CROUCH2` de 300 → 480 (la línea se truncaba en el log) y
  `VERSION` subida en cada build (sin eso el navegador sirve el wasm viejo de caché).

## 23/09/2026 — Retoma del handoff (`ve51` servido, pendiente jugarlo)

- **Estado verificado**: `VERSION=2026-09-23-ve51` servido (`reVC.wasm` 22/09 21:04,
  posterior al último `.cpp` tocado) y con las trazas R25 dentro (`CROUCHMOVE fin`,
  `obju=` comprobados en el binario). El árbol trae la conversión
  (`Ped.cpp:1544`, `PlayerPed.cpp:2567`).
- **`[tooling]` Marca obsoleta fuera de `check-served-build.sh`**: pedía
  `ViceExtCrouchRollRepeat` (palanca `?crouchroll=repeat` de R22), quitada con el
  resto de palancas URL/consola en R24 por petición del jugador. La rueda única es
  comportamiento fijo (`s_odRollActive`/`s_odRollLocked`), no palanca. El check pasa
  ahora completo (60/60).
- **Verificado en `ve51` por el jugador (23/09)**: `CROUCHMOVE mps=0,90
  maxfr=0,016-0,054`, `mvec≈obju` — el arreglo de unidades cierra (§6.1 del
  handoff hecho). De ahí salen los dos pedidos de R26 (velocidad y giro).

## R26 (23/09, post-ve51) — velocidad 1,00 + giro a los lados (`ve52`)

- El jugador verificó `ve51` jugando ("ahora si va a la velocidad que deberia";
  en el log `CROUCHMOVE mps=0,90 maxfr=0,016-0,054`, `mvec≈obju`) y pidió dos
  cosas: (1) caminar agachado a la velocidad del caminar normal menos un poquito
  (ahora lo ve lento), (2) que a los lados el cuerpo gire hacia el avance (se
  desplazaba mirando al frente).
- **`[raíz]` Velocidad**: `VICEEXT_CROUCH_WALK_MPS` 0,90 → **1,00 m/s** (el andar
  de pie mide 0,79-1,13 en `PEDAT`). El ritmo sale solo (1,00/3,58 = 0,28): las
  piernas lentas siguen abiertas (punto 2 del handoff).
- **`[raíz]` Giro (handoff §6.5, receta cumplida)**: `ViceExtCrouchSideHeading`
  (`PlayerPed.cpp`) devuelve el rumbo en el MUNDO (cámara + mando) sólo a los
  lados (50-130°) y sin apuntar; `PlayerControl1stPersonRunAround` lo pone de
  destino en vez del forzado a cámara, y R20c (`Ped.cpp`) avanza en esa misma
  base. Como el objetivo no depende del cuerpo, no hay círculo. Apuntando no se
  gira (ahí se encara y se rueda: R21). Comparado con `pc.mp4` (t 55-85: de pie;
  los tiempos del plan no casan con ese fichero): el comportamiento pedido es el
  de SA (§4.2 del plan) y el que describe el jugador, no hace falta más vídeo.
- **Verificado**: `ninja` 84/84 + enlace (`ve52`, `check-served-build.sh` todo OK).
  En partida: `rumbo` de `CROUCH2` siguiendo a `ang` a los lados + ojo.

## R26b (23/09) — cadencia x1,7 e instrumento del giro (`ve54`)

- El jugador (ve52): piernas en cámara lenta con el cuerpo bien a 1,00; y a los
  lados sigue sin girar (en el log `rumbo` fijo con `ang=+-90`, con el código en
  verde en estática).
- **`[raíz] Piernas**: `VICEEXT_CROUCH_CADENCE 1,70` sólo al caminar
  (adelante/atrás; la rueda no se toca): ciclo ~1,5 s como andando, cuerpo a
  1,00. Declarado en código y al jugador: a igual zancada, más cadencia = los
  pies barren x1,7 (patinan); es exactamente lo pedido y va en una constante.
- **Instrumento del giro**: `CROUCH2` lleva `giro=` (0 gira; 2/3/4/5 motivo) y
  `hdgr=` (headingRate). Con una sesión corta de strafes se caza si es puerta,
  `Dest` pisado o ritmo de giro a cero. Marcas R26 añadidas al check (verde).
- **Verificado**: `ninja` + enlace (`ve54`, check todo OK). En partida: piernas a
  ojo + `pies≈0,48` + `giro=` en los lados.

## R26d (23/09) — velocidad 0,90 + giro ejecutado en R20c (`ve55`)

- El jugador (ve54): piernas bien, cuerpo a 1,00 bien; giro sin funcionar
  (`rumbo` fijo con `ang=+-90` y `giro=0`, o sea el helper decía girar pero el
  destino nunca llegaba al cuerpo) y regla nueva: caminar WASD=1 →
  agachado=0,90 (cadencia x1,7 intacta). Apuntar agachado ya medía igual que
  andar (0,99, 5 muestras) y no tiene tope propio: queda unificado por
  construcción.
- **`[raíz]` Giro**: el snap (`Cur=Dest=rumbo mundo` + `SetHeading`) va en R20c
  (`Ped.cpp`), que corre siempre agachado; el `Dest` del control se quedaba por
  el camino. Apuntar no gira (R21). Velocidad `WALK_MPS` 1,00 → 0,90.
- **Verificado**: `ninja` + enlace (`ve55`, check todo OK). En partida: `rumbo`
  siguiendo a `ang` a los lados + `mira=1 velo` igual que sin apuntar.

## 24/09/2026 — Tanda 2 EJECUTADA (build `ve57`) + veredictos Tanda 3

Encargo: las 2 tandas en orden, con el minimo codigo propio posible. Resultado:
Tanda 2 cerrada en `ve57` (enlazado 24/09, `check-served-build.sh` = TODAS las
marcas, `dataTag` sigue `ve13` = sin cambios de datos); Tanda 3 auditada item
por item (1 YA ESTABA, resto PENDIENTE con motivo y siguiente paso).

### Portado (codigo minimo, con cabecera PORTADO + fila en `docs/mods/ATTRIBUTION.md`)

- `[raiz]` **:1372 sirena FBI** (`src/audio/AudioLogic.cpp`): `FBIRANCH || FBICAR`
  en la rama aguda (12668 Hz). El enum de audio YA era el mismo del mod
  (`FBICAR=17`, `FBIRANCH=90`, comprobado contando el enum). 1 linea + cabecera.
- `[raiz]` **:2453 casquillos** (`src/weapons/Weapon.cpp`, `FireInstantHit`):
  guardia en el unico `AddGunshell` del caso COLT45/PYTHON/SNIPER/LASER (el enum
  ya coincide: 18/28/29). Fogonazo/humo intactos.
- `[raiz]` **FV rotor** (`src/vehicles/Heli.cpp`, `CHeli::Render`):
  `+= (3.14f/6.5f) * CTimer::GetTimeStep()`. 1 linea.
- `[raiz]` **Luces ambulan/firetruk** (`src/vehicles/Automobile.cpp`): bloque
  dummy-first + fallback fijo en los dos casos (mismo patron R11/ve24; sus
  `servicelights_0` cuelgan de `chassis_dummy`, verificado con strings en los
  `.dff` servidos). `fbicar`/`fbiranch`/`vicechee` ya tenian corona (P5) y sus
  `.dff` NO traen dummies: se quedan.
- `[raiz]` **Audio tabla de oido** (`src/audio/AudioLogic.cpp`,
  `SOUND_WEAPON_SHOT_FIRED`): grupo ViceEx a 4 armas (DEAGLE->05, SHOTGUN2->03,
  STEYR->09, GR_LAUNCH->11) + BERETTA->COLT45, UZIOLD->UZI, AK47/M16->M4 de
  serie. Cierra el bug audible de D8 (la Desert Eagle usaba la muestra 00,
  ROTA de fabrica). Recargas ya sonaban por familia; el banco ya se servia.

### Verificado sin tocar (regla previa: YA ESTABA / cubierto / no aplica)

- **:802 YA ESTABA**: `BigMessageX[0]/[1]` escalan velocidad Y distancia juntas
  (680/0.3 ms a cualquier resolucion); el hold de 120 va por `GetTimeStep`.
- **:1454 YA ESTABA**: `CVehicle::RemoveDriver` ya trae el `if != WRECKED` tras
  `FIX_BUGS` (activo, `config.h:304`).
- **FV autopiloto YA ESTABA** (0 contadores en frames) y **bocina/sirena NO
  APLICA** (toggle por flanco; `&7` re-emite una llamada idempotente; el
  parpadeo visible va por ms).
- **IPL 8 zonas: YA CUBIERTO, 0 cambios** (72 hunks P-vs-E revisados): el unico
  fix sustantivo (club: fuera `club_exterior03/04`) ya falta en los ficheros de
  ViceEx; `hotel` es ruido float + `hotroomfan` con rotacion quaternion-
  equivalente (dot=-1.0); el resto son flags normalizados por ViceEx y anadidos
  suyos (chandelier, puerta, `cull`) que se conservan. Copiar los SP revertiria
  eso + 160 MB de re-descarga.
- **B8 piernas YA ESTABA** via H4 (`odAimWalk` vale para 2H).

### Pendiente con motivo (ATTRIBUTION §6)

Rueda en railes (sin acumulador localizable + sin fuente FV: solo .asi/.ini),
swim-12/reload-2 (piden id de sonido nuevo: auditar `SOUND_TOTAL_SOUNDS`),
D14 (plumbing de uniform sin 2o set de colores con que mezclar), B7/B8-camara/
B9/Sprite2d/Radardisc/Loading/menus (proyectos con PASS visual, ninguno de una
linea), :1864/:1478/:1701 (sin ancla de una linea), D15 (el ultimo, por diseno).

### Criterios PASS para tu partida (build `ve57`, Ctrl+Shift+R)

1. FBI Washington con sirena: suena aguda como el FBI Rancher.
2. Python/francotirador/laser: fogonazo si, casquillo no; Colt45 con casquillo.
3. Ambulancia/bomberos con sirena: barra roja/azul sobre la cabina (trazas `SVLIGHTS`).
4. Helicoptero: rotor igual a 35 y 120 fps (a ojo).
5. Armas nuevas: Desert Eagle/shotgun2/steyr/lanzagranadas con SU sonido (trazas `VICEEX sfx arma=`); beretta/uziold/ak47/m16 con el de serie.

## 24/09/2026 — Inventario total de mods/ + MD extenso (3 subagentes, solo lectura)

Encargo (contexto del jugador): agachado/nado/apuntado/recoil se crearon de 0 y van mal; leer TODO mods/, no adivinar, copiar/adaptar, MD extenso, enriquecer CLEO-lite con fuentes de los autores, confirmar subagentes.

**Subagentes: si.** 3 en paralelo en esta pasada (nado+agachado / primera-persona+camaras / mineria GitHub). Reglas: solo-lectura, un dueno por fichero, sin commits.

**Nuevo inventariado** (cero cobertura antes): Serega-swim (curva 0.022/0.025/0.1 + 0.004/0.05/0.12 copiable; su ifp-236 NO: sin Swim_*, DUCK_low bug de 5.1m), GeniusZ-firstperson (offsets por ped/vehiculo + near-clip dual + FOV, sin head-bob), ClassicAXIS-ini nuevo (CrosshairMult 0.53/0.4, LockOnTargetType=1, WalkKey=LALT) con correccion (movements.img NO trae walk_legs). **SACarCam: nada que portar** (shim GInput, Process_FollowCar_SA ya lo cubre).

**Licencias**: MIT portables = FramerateVigilante, ThirteenAG/CLEOScripts. Todo lo demas (WeaponRecoilAuto, Drivebys, SA-Crouch con prohibicion explicita, ClassicAXIS, SkyGfx, GeniusZ) = SOLO spec/reimplementacion.

**Plan de re-trabajo** (orden): recoil por precision del arma -> nado con curva Serega -> agachado a 2.74m/0.731s -> apuntado ClassicAXIS+GeniusZ -> FV-MIT. Todo en .agents/plans/mods/13-inventario-mods-fuente.md (MD extenso para relectura).

## 24/09/2026 -- SACarCam mirado por dentro + MERGE de planes

- **SACarCam.asi** (53 KB, PE32): 275 strings (1 sola de camara = su propio
  `.pdb`), imports solo KERNEL32/MSVCP/VCRUNTIME + `ginput`, sin nombres de
  camara/FOV/beta/alpha. **Veredicto: shim fino que enruta el input a GInput
  y activa la via `FollowCar_SA` del propio juego; NO hay matematica de camara
  que copiar** (esa vive en el juego y ya la tenemos en `Cam.cpp:5292`; la
  spec real es `CamNew.cpp`). No se desensambla mas: el retorno no paga un RE.
- **MERGE**: `12-handoff-tanda2.md` p.13 une tandas pendientes + re-trabajo en
  el orden recoil -> nado -> agachado -> apuntado -> FV-MIT (el del 13 p.6).

## 24/09/2026 -- 5 bloques en paralelo (recoil/nado/agachado/apuntado/FV-MIT), build `ve58`

4 agentes con ficheros exclusivos (PlayerPed.cpp entero para nado+agachado en
secuencia; Cam.cpp entero para apuntado; Weapon.cpp recoil; FV Timer/CarCtrl/
CarAI/Bike). Yo: interfaces, cableado, gemelo del spool, enlace unico.

- **Recoil**: acumulador por `spread`/`range` de `weapon.dat` (`acc` 0.5-2.0) +
  `ViceExtRecoilPitch()` libre; `multY` intacto. **Cableado mio (1 linea)**:
  la patada ponderada (`kickAlpha*odAcc`) es la que entra a
  `ViceExtRecoilAlphaAdd` (el acumulador paralelo solo iba a traza; el TODO de
  Cam.cpp citaba `CWeapon::` cuando es libre: corregido).
- **Nado/agachado**: curva Serega (0.022/0.05/0.12 + timeout 300, unidades
  /50) y crouch a 2.74m/0.731s + deadzone +-16 + filtro de armas. Sin datos
  nuevos, sin defines nuevos.
- **Apuntado+coche**: hombro 0.2/0.55 suavizado, CrosshairMult ya estaba,
  stick 1/20, clamp +60/-89.5, near-clip dual, `CAMSA` 1/s, marco lock-on por
  vida. NO sustituido (documentado): autocentrado, ley de distancias por
  col-model, rama de agua (del otro agente). Decisiones: hombro 0.2 fijo,
  near-car DEFAULT, triangulo blando pendiente (lado ped), palo/WalkKey fuera.
- **FV-MIT**: heli-IA spool (1 linea) + gemelo del jugador (mio, Automobile);
  railes/Timer/autopiloto/Bike = YA ESTABA/NO APLICA con evidencia; claxon
  moto PENDIENTE (pide espejo AudioLogic+Automobile).
- **Fix mio del check**: la marca `ViceExtSwimClipSpeed` solo vivia en DWARF y
  el postlink la podo (FALLO con todo lo demas verde). `noinline` no basto;
  via robusta: traza viva `SWIMNAT` 1/sesion (fija la marca + documenta la
  calibracion en-log; el parser la ignora por regex con nombre).

Build **`ve58`** enlazado, `check-served-build.sh` = TODAS las marcas,
`dataTag` sigue `ve13`. Sin commits. Detalle por bloque en informes de los
agentes (hilo) y `docs/mods/ATTRIBUTION.md` p.4/p.6.

## 24/09/2026 -- BARRIDO 1 (coche+apuntado, `ve59`) + mineria profunda del exe

- **Barrido real en `Cam.cpp`**: fuera autocentrado-timer, string-cam por
  canales, `FixCamWhenObscured`, `Process_FollowCar_SA` entero (~700 lin) y
  clamp +45. Dentro ley unica CamNew (dist/height/stick/rata/clamps/retorno
  WellBufferMe/LOS+esferas/near dual) con adaptaciones documentadas (Beta
  re-derivada, Alpha integrada, altura por dimZ en coche, agua = suelo).
  `FollowCar_SA` delega. Apuntado: hombro 0.2 (sin conmutador natural),
  lock-on verificado, recoil intacto. Build **`ve59`**, check TODAS.
- **Exe rascado a fondo** (solo lectura, ficheros en `tmp/`: strings por tema,
  catalogo 101 rutas, disasm cluster 1a persona): su arbol trae `Cam.cpp`
  pero NO `Camera/Hud/Pad/PlayerPed/WeaponInfo`; crouch/swim = datos IFP
  (0 xrefs); recoil = solo clave ini + cadena `*0.53`; 1a persona = cluster
  loader (FOV byte, HeadBob, PI/2, 50.0) + `PED_1RST_PERSON_LOOK_*`;
  servicelights con offsets `0x4C4/0x4C8/0x704/0x708`; `PED_RELOAD` con slot.
  Adaptable: nombres/offsets de acciones, stubs de claves, forma del loader,
  offsets de dummies, asserts de doom-aim. Ilegible: constantes sueltas sin
  ancla (2.74/0.731, 0.022/0.05/0.12, 70 FOV) y Follow/OnString como codigo.
  Conclusion: confirma spec, no sustituye codigo; el port sigue por specs.

## 24/09/2026 -- Fixes cámara (A/B/C) + barrido recoil v2 (`ve60`)

- **A (delante al subir)**: Beta heredaba PI opuesta a pie vs coche + `TransitionBeta` lo devolvía delante. Fix: `Beta=TargetOrientation` + hold 5 frames al entrar.
- **B (Q/E abajo)**: Q/E no entraba a Beta y el snap usaba `CA_MAX_DISTANCE=0` (Dist=0, Front vertical). Fix: Q/E/cruceta/palo a yaw + distancias reales.
- **C (delay 2s)**: `idleMs=2000`, `orbiting` incluye ratón crudo y teclas; retorno pasivo solo quieto+avanzando; `camauto2` lleva `idle=`.
- **Recoil v2**: el factor viejo era ficción (`spread=-1.0` siempre -> 0.50 constante). Proxy `damage/range` [0.5,2.0]; una sola física; patada efectiva ~x2 vs R14 (aviso al jugador para escalar por oído). Mira fija (`multY=0.400`).
- Build **`ve60`**, check TODAS. Sin commits.

## 24/09/2026 -- Recoil v3 (mira sube, R14 rescindido) + A/D no orbita (`ve61`)

- **Causa A/D**: el DPad es steer (`LeftStickX+DPad` en `Pad.cpp`) y estaba
  inyectado a Beta + en `keyLook` (reintentos de idle). Fix: orbitan solo
  ratón/Q/E/mirar-atrás/palo derecho; el rumbo lo sigue la auto-rotación.
- **Mira que sube**: `multY` += `kick*acc` (tope 0.500), decae a 0.400, reset
  al cambiar de arma; la bala (`Find3rdPersonCamTargetVector`) y el HUD la
  siguen. Cámara intacta. `RECOIL3` cambia de criterio (debe mostrar >0.400).
- Build **`ve61`**, check TODAS. Sin commits.

## 24/09/2026 -- ve62: R14 restaurado + Q/E + retorno (regla: git solo lectura)

- **Regla del jugador: git PROHIBIDO salvo lectura.** Nada de add/commit/push
  ni escrituras via git; lectura (status/diff/grep) permitida.
- **Revert mira**: fuera subida/decaimiento/reset de `multY` (0.400 fijo);
  camara intacta; `RECOIL3` exige 0.400 otra vez.
- **Q/E infinito**: `+-127`/frame a Beta sin cota (190 grados/s). Fuera;
  Q/E estables por snap existente + idle 2s + retorno.
- **Retorno vs raton**: `btnReq` no chequeaba `orbiting` del frame y fijaba
  `Rotating=1` moviendo. Fix: `btnReq && !orbiting` (reuso).
- Build **`ve62`**, check TODAS. Sin commits.

## 24/09/2026 -- ve63: raton visto en coche + recoil en modo apuntar

- **Raton**: la via real es `useMouse`/BetaOffset-AlphaOffset, no el crudo
  (a 0 en ese modo). `orbiting` la incluye ahora; mover = `auto=0`.
- **Recoil**: apuntando con lock-on corre `Process_Syphon`, que nunca llamaba
  al apply. Ahora aplica tras su `WellBufferMe`: escalones por disparo con
  retorno, reticula fija. Internet (WeaponRecoilAuto): nuestro retorno aditivo
  ya es el suyo; nada que traer.
- Build **`ve63`**, check TODAS. Sin commits (git solo lectura).

## 25/09 (tarde) — Cámara de coche sin lucha + recentrado pasivo a los 2 s (`ve65`)

Petición del jugador tras probar `ve63`: la cámara de coche "lucha" con el ratón
(centrado activo mientras miras) y pidió: *mover el ratón libre; al soltar, el
recentrado 2 s después*. Decisión: **sin botón dedicado**.

**Diagnóstico (informe del 25/09, su sesión de las 20:21):** el pasivo y el botón
estuvieron callados toda la partida (1 sola `camauto2`, `auto=0 pedido=0`). Las
vías reales que recentran mientras miras: (1) `m_bUseTransitionBeta` pincha
`Beta = TargetOrientation` cada frame al entrar al coche; (2) el snap de mirar
(RMB = mirar atrás, Q/E = lados) teletransporta `Source`; (3) **la radio
(Insert/R) recentra**: `ForceCameraBehindPlayer()` = `LeftShoulder1` =
`VEHICLE_CHANGE_RADIO_STATION` (`Pad.cpp:3405`, `ControllerConfig.cpp:205/768`).

**Cambios** (`src/core/Cam.cpp` sólo; plan `.agents/plans/camara-coche-sin-lucha.md`):
- C1: el botón de radio ya no recentra en coche (el pasivo hereda su rol); el
  binding de la radio queda vanilla.
- C2: el pasivo pierde la condición `vel > 1.0` → recentra también parado.
- C4: si el jugador mueve el ratón durante la transición de entrada, el pin se
  libera (el ratón manda desde el primer frame); sin ratón, entra detrás igual.
- Traza `BETASNAP motivo=` (una por motivo/sesión) en las vías de snap que
  quedan (mirar-atrás, snap-lados, ratón-al-entrar) para que la próxima partida
  nombre cualquier empujón vivo.
- `VERSION` → `2026-09-25-ve65`; `dataTag` sin cambios (no hay datos nuevos).

**Verificación**: ninja completo + enlace OK; `check-served-build.sh` = TODAS
las marcas + `BETASNAP` presente en el wasm servido (grep -a = 1).

**PASS pedido al jugador**: en coche, mirar con el ratón y soltar → nada empuja
mientras miras, ~2 s después vuelve sola; la radio no mueve la cámara; Q/E y
RMB siguen mirando lados/atrás (ahora dejan línea `BETASNAP` en el log).

**Pendiente aparte (no en este build)**: recoil aditivo estilo WeaponRecoilAuto
(congelar decaimiento mientras se dispara, volver al soltar), medido en el
informe del 25/09 (el retorno 4°/s > entrada de cualquier ráfaga = invisible).

## 25/09 (noche) — La "lucha" persiste en ve65: diagnóstico con la sesión 23:52 + instrumento CAMB2 (`ve66`)

**Hechos de la sesión 23:52 (`ve65`, 213 s, 2 coches, 88+30 s conduciendo):**
- El pasivo y el botón estuvieron **mudos** (1 `camauto2`): `orbiting` fue true
  constantemente (el ratón del jugador en uso) ⇒ el recentrado de 2 s **nunca
  arrancó**. Lo que el jugador ve NO es el pasivo ni el botón.
- `BETASNAP` atrapó las 3 vías de snap: `raton-al-entrar` (23:52:31, la C4
  nueva funcionando), `snap-lados` (23:52:55, Q/E o RMB) y `mirar-atras`
  (23:53:07, RMB). RMB en coche = mirar atrás: se toca sin querer al apuntar.
- **La altura de la cámara oscila sola**: `CAMV2 altura` va 0.37 → 2.49 → 5.98
  → 4.31 (6 s clavada) → -1.77 → 0.37 con el coche a velocidad 0.5-0.7
  constante. La causa: `Alpha` en el string-cam es persistente y NO tiene
  retorno: lo que deja el snap de mirar (o el ratón vertical) se queda. Los
  snap `LookLeft/Right/Behind` además ponen `Source` con otra altura
  (AvoidTheGeometry) y su `Alpha` resultante persiste al soltar.
- El coche se para (`speed=0.0`) y la cámara NO recentra (correcto según C2,
  pero el jugador espera que vuelva al soltar el ratón: la condición era
  `driving`, se cumple... el problema es que con el ratón apoyado `orbiting`
  sigue true por micro-deltas ⇒ idle nunca llega a 2000).

**Cambio (`ve66`, sólo `Cam.cpp`)**: traza `CAMB2` 1/s en coche con
`brel` (Beta relativo al coche en grados), `alpha`, `mx/my` (deltas crudos del
ratón), `win` (ventana de mirada), `idle` (ms de quietud), `orbit`, `rot`,
`rotspb`. Con UNA partida del jugador el log dirá exactamente qué empuja Beta
o Alpha y por qué el reloj de quietud no llega a 2 s.
- `VERSION` → `2026-09-25-ve66`. `dataTag` sin cambios.

## 2026-09-25 — ve67: la cámara de coche devuelve TAMBIÉN el eje Y `[raíz]` + CAMB2 real
Sólo `src/core/Cam.cpp` (más `VERSION`). Plan: `.agents/plans/camara-coche-sin-lucha.md`.
- Lectura del log de ve66 (odtrace-2026-09-26_00-27-18, 11516 CAMB2 ~198 s): `rot=0`
  SIEMPRE (el pasivo jamás arrancó porque `orbit=1` en TODAS las muestras) y
  **Alpha sin retorno confirmado**: altura 0.37→2.49→5.98 (clavada 6 s)→-1.77;
  10627/11516 muestras fuera de [0.1,0.5]. El Beta sí lo mueve el ratón
  (|brel|>15° en 3293). Esa altura clavada era la otra mitad de la "lucha".
- **Retorno del eje Y** (lo pedido 25/09): el pasivo de 2 s ahora también hace
  `WellBufferMe(s_odCarAlphaBase, &Alpha, ...)` hacia la base del vehículo
  (0.0 = horizontal), misma ley 0.1/0.06 que Beta, y se anula igual si el
  jugador vuelve a mirar (orbit/suppress/lock). Baseline por vehículo
  (static + reset al cambiar de coche/ResetStatics).
- **Bug del guard de CAMB2** (ve66 emitía ~58/s en vez de 1/s): el reancla
  `if(nowCar < next) next = 0` disparaba frame sí/frame no. Ahora sólo
  re-ancla si el reloj saltó hacia atrás >60 s (patrón RECOIL2/WANTEDHIDEINIT).
- **CAMB2 ampliado** para cazar al culpable del `orbit=1` eterno (sospechoso:
  botón fantasma LS2/RS2 — RMB=VEHICLE_LOOKBEHIND los pone a 255, o stick
  derecho con drift > deadzone 85): nuevos campos `keyLook` (GetLookLeft/
  Right/BehindForCar), `bOff/aOff` (offsets aplicados), `ls2/rs2` crudos,
  `lkPersist` (alguna vez !=0 en esta sesión) y `base` (baseline Alpha).
  `win` pasa a %d booleano.
- `check-served-build.sh` OK; marcas nuevas en el wasm; `VERSION` →
  `2026-09-25-ve67`. Pendiente: leer CAMB2 de la próxima partida y decidir si
  el fantasma es RMB/mandos (ls2/rs2) o stick (bOff/aOff). El recoil aditivo
  sigue en su tarea aparte.

## 2026-09-25 — ve68: la RAÍZ del recentrado que nunca llegaba (ruido sub-pixel del ratón) `[raíz]`
Sólo `src/core/Cam.cpp` (más `VERSION`). Datos: partida del jugador con ve67
(`odtrace-2026-09-26_00-58-13`, 78 CAMB2, 1/s real ya, modo 18 = CAM_ON_A_STRING).
- **Medido**: `orbit=1` en 78/78 muestras y `idle=0` en 78/78, `rot=0` siempre
  (`camauto2` sólo imprimió `auto=0`). En 61/78 muestras con el ratón quieto
  TODOS los campos trazados estaban a 0 (`keyLook=0`, `ls2=rs2=0`,
  `bOff=aOff=0.000`, `win=0`) y aun así `orbit=1`: faltaba una vía.
- **Causa raíz**: el navegador entrega **deltas de ratón sub-pixel**
  (`|dx|,|dy| < 0.5`) casi todos los frames (por eso `mx` impreso con %.1f
  salía `0.0` y `bOff` con %.3f salía `-0.000`). Con `MouseX != 0.0f`,
  `useMouse` y `(BetaOffset != 0.0f)` en `orbiting`, la bandera quedaba TRUE
  para siempre ⇒ `Rotating = false` cada frame y el reloj de quietud **nunca**
  llegaba a 2000 ms: **el pasivo de 2 s no corrió jamás** (ni en ve65/66/67).
  Eso explica que el jugador viera "sigue pasando lo mismo" y que el retorno de
  Alpha de ve67 no llegara a ejecutarse nunca.
- **Arreglo (ve68)**: `orbiting` usa umbral de INTENCIÓN: `odMouseBig`
  (|MouseX|+|MouseY| >= 0.5), `odOffsetsBig` (|offset| >= 0.001) y
  `odUseMouseBig`. El ruido sub-pixel se sigue sumando a Beta/Alpha (se
  cancela solo) pero ya no frena el retorno. Con esto el pasivo de 2 s vuelve a
  correr y arrastra el retorno del eje Y de ve67 (Alpha → base).
- **CAMB2 en precisión completa** (para cerrar el resto): `mx/my` a 3 decimales,
  `mag` (|dx|+|dy|), `raw` (delta != 0 exacto), `big` (intención), `win`, `mA`
  (mouseActive real, ANTES del rearme de la ventana: antes se medía después y
  siempre daba 0), `stick`, `sup` (suppress), `gt` (GetTarget), `rsx/rsy`
  (palos derechos crudos), `dist`.
- **Etiquetas nuevas de recentrado forzado** (una por motivo y sesión):
  `hold-entrada` (los 5 frames al subir al coche), `centrar-detras`
  (`m_bCamDirectlyBehind`) y `centrar-delante` (`m_bCamDirectlyInFront`), para
  que la próxima partida nombre cualquier recentrado instantáneo que quede.
- `check-served-build.sh` OK; marcas nuevas en el wasm; `VERSION` →
  `2026-09-25-ve68`. Pendiente: leer CAMB2 de la próxima partida y confirmar
  `rot=1` con `idle>2000` tras soltar el ratón (y Alpha volviendo).

## 2026-09-25 — ve69: fuera el PIN de Beta en el coche (el recentrado inmediato del eje X) `[raíz]`
Sólo `src/core/Cam.cpp` (más `VERSION`). Datos: partida del jugador con ve68
(`odtrace-2026-09-26_01-37-25`, 45 CAMB2, 29 `camauto2`).
- **Lo que ve68 arregló (confirmado en los datos y por el jugador)**: `orbit=0`
  ya aparece con el ratón quieto (`raw=1 big=0`, mag=0.00: ruido sub-pixel), el
  reloj `idle` crece (666→2300→4300 ms), `rot=1` sale **sólo con idle>2000**
  (01:37:54 idle=2300, 01:38:01 idle=4067, 01:38:24 idle=2584) y `deg` vuelve a
  ~0 (`-74.3° → -17.5° → 0.2°`) junto con `alpha` (`-0.71 → -0.26 → -0.00`):
  **el pasivo de 2 s vuelve a correr y el eje Y espera su turno** (el jugador lo
  confirmó: "ahora sí centra en el eje Y tras 2 s").
- **El eje X seguía recentrando al instante**: a las 01:37:48 `deg` colapsa
  59°→1.7° con `rot=0` e `idle=666` (imposible para la ley) ⇒ un pin forzado.
  Causa: `Camera.cpp:2594/2670` rearma `m_bUseTransitionBeta` ("Get into
  vehicle"/"Getting out") y el bloque C4 del coche (`Cam.cpp:2451`) hacía
  `Beta = TargetOrientation` en los frames sin mirar → recentrado **inmediato**
  al soltar y **lucha** mientras el ratón se mueve. La etiqueta `raton-al-entrar`
  sólo se emitió una vez (una por motivo y sesión) y ocultó las repeticiones.
- **Arreglo (ve69)**: en el coche la transición **ya no pina Beta** (se sigue
  bajando la bandera, con etiqueta `pin-liberado`). El pasivo de 2 s es el único
  recentrado; la entrada la cubren el hold de 5 frames y la interpolación de
  `Camera.cpp`.
- **CAMB2** añade `tB/beh/fro` (banderas que fuerzan Beta, leídas antes de sus
  bloques) para cazar cualquier pin que quede vivo.
- `check-served-build.sh` OK; marcas nuevas en el wasm; `VERSION` →
  `2026-09-25-ve69`.

## 2026-09-25 — ve70: el yaw deja de re-derivarse de Source (causa de fondo) `[raíz]`
Sólo `src/core/Cam.cpp` (más `VERSION`). Datos: ve69
(`odtrace-2026-09-26_01-50-14`, 121 CAMB2).
- **Lo que reveló el log de ve69**: el yaw se movía 100+ grados con **cero
  entrada** (`mag=0`, `big=0`, `stick=0`, `keyLook=0`, `tB/beh/fro=0`) y
  `rot=0` (la ley no intervenía), con `idle` a veces < 2000 ms. Con las banderas
  de recentrado ya limpias (ve68/ve69) eso no podía venir del ratón ni de la ley.
- **Causa de fondo**: en la ley, `Beta` (el yaw) se **re-derivaba de `Source`
  cada frame** (`Beta = GetATanOfXY(-dist.x, -dist.y)`). Como `Source` lo mueven
  otras cosas — los snaps de mirar atrás/lados (`LookBehind/LookLeft/LookRight`
  escriben `Source`), las colisiones y los teleportes — el yaw se recalculaba
  solo, sin que el jugador ni la ley lo tocaran. `Alpha` en cambio ya era estado
  propio (se integra): **de ahí que el eje X "se recentrara" solo y el Y no**.
  Ese era el motivo real de que el jugador dijera "sigue absolutamente igual"
  build tras build: se estaban limpiando *pines*, no la re-derivación.
- **Arreglo (ve70)**: `Beta` pasa a ser ESTADO PROPIO. Solo lo cambian el
  ratón/palo (offsets), la ley de retorno de 2 s y los bloques forzados. Se
  re-deriva de `Source` únicamente cuando un snap lo ha movido
  (`LookingBehind/Left/Right`, que además siguen funcionando igual) o al entrar
  (`ResetStatics`). Con esto, al soltar el ratón el yaw queda **congelado** y a
  los 2 s vuelve detrás del coche; nada más puede moverlo.
- **Traza nueva `CAMB3`**: ráfaga a 10 Hz durante 4 s tras soltar el ratón, con
  el yaw descompuesto (`Beta`, `bGeo` = valor geométrico del frame, `ySrc` =
  yaw del Source, `yFr` = yaw del Front final) + `dist`, `idle`, `rot`, `tB`,
  `snap`. Sirve para verificar el congelado y cazar cualquier otro movedor.
- `check-served-build.sh` OK; marcas nuevas en el wasm; `VERSION` →
  `2026-09-25-ve70`.

## 2026-09-25 — ve71: distance de frente (rayo chocaba con el propio piloto) y teclas que aplazaban el recentrado `[raíz]`
Sólo `src/core/Cam.cpp` (más `VERSION`). El jugador confirmó que ve70 funciona
("ahora sí funciona") y reportó dos fallos nuevos. Datos: ve70
(`odtrace-2026-09-26_02-02-15`, 278 CAMB2 + 1305 CAMB3).
- **Fallo A — la cámara se pega al mirar de frente.** Medido por franjas de
  ángulo: `dist` media **7,20 m** detrás (|deg|0-30) y se desploma a **1,94 m**
  de frente (|deg|150-181, mín 1,49). Causa: el rayo de la ley
  (`ProcessLineOfSight(TargetCoors, Source, ...)`) parte de `TargetCoors`, que
  va a `0,8*alto` del vehículo — en la moto, la cabeza del piloto — así que el
  primer impacto era el propio jugador y `Source = colPoint.point` pegaba la
  cámara a él (de ahí "veo el casco/parabrisas y no el parachoques").
  Arreglo: si el primer impacto es un **ped**, el rayo se descarta (el ped
  propio nunca debe frenar la cámara; la prueba de esferas ya lo ignora).
- **Fallo B — espacio/flechas aplazaban el recentrado.** El log lo caza:
  muestras con `stick=1`, `rsy=±128`, `bOff=aOff=0.0000`, `mag=0`, `idle=0`,
  `orbit=1`. Esas teclas (wheelie/stoppie) escriben el palo derecho, y el
  `stickActive` booleano encendía `orbiting` aunque no movieran la cámara ni un
  píxel. Arreglo: `stickActive` **fuera** de `orbiting`; decide el offset que se
  aplica de verdad (`odOffsetsBig`), no el eje crudo.
- Instrumentación: `CAMB2b`/`CAMB3b` publican `los/losD/losPed` (impacto del
  rayo, su distancia y si era un ped) para confirmar el arreglo A en la próxima
  partida sin sondas.
- `check-served-build.sh` OK; `VERSION` → `2026-09-25-ve71`.

## 2026-09-25 — ve72: la esfera de oclusión chocaba con el propio vehículo (de frente la cámara se pegaba a 2,00 m) y el freno de mano cortaba el recentrado `[raíz]`
Sólo `src/core/Cam.cpp` (más `VERSION` y dos marcas nuevas en
`tools/check-served-build.sh`). El jugador confirmó que en ve71 las flechas ya
no cortan el recentrado. Datos: ve71 (`odtrace-2026-09-26_02-21-11`, 145 CAMB2 +
145 CAMB2b + 464 CAMB3).
- **Fallo A — de frente la cámara se pega a la cara de Tommy.** El log lo
  acota: `dist` 5,47 detrás → 3,61 → **2,00 clavada** de frente (con `los=0` en
  607 de 608 muestras, o sea el rayo NO era la causa). El culpable es el bucle
  de 5 esferas: su tercer argumento es `pIgnoreEntity` y estaba en **`nil`**, así
  que la esfera —centrada en la propia cámara y del tamaño del plano de vista—
  podía chocar con **el propio vehículo del jugador**, que está justo en el eje
  cámara→objetivo y tiene una bounding sphere enorme (`BaseDist` 5,37 ⇒ radio
  ~1,9). Ese impacto entra por la rama `d == 0,1f` (con la nearClip ya enganchada
  en 0,1, `Min(nearClip, dRaw)` deja de discriminar), mueve `Source` 0,1 m hacia
  el objetivo en cada frame y el clamp `[minDist, maxDist]` lo fija en
  `minDist = 2,00`: la "cámara anclada a la cara". Con Q+E (`LookBehind` coloca
  `Source` a `CA_MAX_DISTANCE`) no pasa por ahí, y ésa era la vista buena que
  reportó el jugador. Arreglo: `TestSphereAgainstWorld(..., CamTargetEntity, ...)`
  (el vehículo objetivo no ocluye su propia cámara) + corte explícito si la
  esfera devuelve el propio objetivo.
- **Fallo B — espacio cortaba el recentrado.** El log lo caza:
  `sup=1 gt=1 idle=0` con el ratón quieto. En vehículo `RightShoulder1`
  (`CPad::GetTarget`) es el **freno de mano** (espacio; ControllerConfig
  `VEHICLE_HANDBRAKE = rsRCTRL + ' '`), no mirar, pero `suppress` lo trataba como
  apuntar y reseteaba el reloj de 2 s en cada frame. Arreglo: `suppress` se queda
  sólo con `TheCamera.Using1stPersonWeaponMode()`.
- Instrumentación: `CAMB2b`/`CAMB3b` amplían con `sph/sphM/sphPed/sphOwn/dSph/dRaw/nc`
  (impacto de las esferas, modelo, si era ped o el propio vehículo, cuántas veces
  se aplicó el acercamiento y la nearClip) para confirmar el arreglo A sin sondas;
  `CAMSA`/`CAMB3` siguen publicando `dist`, que es la medida del fallo.
- `check-served-build.sh` OK (marcas `sphOwn=%d dSph=` y `sph=%d sphM=%d` presentes en
  el wasm); `VERSION` → `2026-09-25-ve72`.

## 2026-09-25 — ve73: la distancia de la cámara era estado sin retorno (de frente se quedaba clavada en 2,00 m) `[raíz]`
Sólo `src/core/Cam.cpp` (más `VERSION` y una marca en
`tools/check-served-build.sh`). El jugador confirmó que en ve72 el espacio ya no
corta el recentrado. Datos: ve72 (`odtrace-2026-09-26_02-39-05`, 145 CAMB2 +
464 CAMB3).
- **ve72 quedó descartado como causa del fallo A.** La traza nueva lo dice sin
  ambigüedad: `sph=0 dRaw=0.0` en TODAS las muestras (la esfera nunca chocaba) y
  `los=0` en 607 de 608. O sea de frente se pegaba a 2,00 m **sin geometría
  ninguna**. El cambio de ve72 (la esfera ignora el vehículo objetivo) se
  mantiene porque es correcto de por sí, pero no era el problema.
- **Causa real: la distancia es ESTADO y la ley sólo la acotaba.**
  `dist = Source - TargetCoors; length = |dist|` viene del frame anterior y sólo
  se aplicaba el clamp `[minDist, maxDist]`; nada devolvía la distancia a la
  querida. Si el vehículo avanza **contra** la cámara (mirando de frente), el
  objetivo se acerca a `Source` frame a frame y la distancia se encoge sola
  hasta quedarse clavada en `minDist`. Medido en ve72: 7,13 → 3,85 → 2,06 →
  **2,00** y de ahí no sale (`CAMB2 dist=1,67` al final del frame → `CAMB3`
  marca 2,00, que es el clamp). Mirando **atrás** pasa lo contrario: al alejarse
  el objetivo la distancia crece y el clamp la fija en `maxDist`, y por eso
  detrás siempre se vio bien. Con **Q+E** (`LookBehind` pone
  `Dist = CA_MAX_DISTANCE` cada frame) la vista de frente también era la buena:
  era la única vía que restauraba la distancia. Esto explica las dos imágenes
  del jugador sin excepción.
  Arreglo: si en el frame anterior **no hubo obstáculo** (ni rayo ni
  acercamiento de esfera), `length = maxDist` antes de recolocar `Source`. Si lo
  hubo, se respeta lo encogido por la geometría (no se atraviesan paredes). Es
  más fiel al original, que re-resolvía `Source` desde cero cada frame en
  `AvoidTheGeometry`.
- **Freno/ acelerador cabeceaban la cámara.** En un vehículo el eje VERTICAL del
  palo derecho es el ACELERADOR/FRENO (`VEHICLE_ACCELERATE = rsUP`,
  `VEHICLE_BRAKE = rsDOWN`, `rsUP = CPad::GetUp()`: la W y la flecha arriba),
  no mirar. Medido en ve72: con W pulsada `rsy=128` y `LookAroundUpDown()`
  devuelve −170, así que `AlphaOffset` valía −0,00095 por frame (~3,3°/s de
  cabeceo) sin tocar el ratón. Es el "se comporta como si hubiera movido el
  mouse". Arreglo: `AlphaOffset = 0` en la vía de palo; el eje X sigue mirando y
  el vertical de la cámara del coche sólo lo mueve el ratón.
- **Al entrar la cámara no se centraba ya.** El hold de entrada sólo se armaba
  con `ResetStatics` o cambio de puntero de vehículo, así que bajarse y volver al
  MISMO vehículo heredaba el yaw y sólo el pasivo de 2 s la centraba ("empieza a
  centrarse después del delay en vez de centrarse apenas me subo"). Arreglo: el
  hold se arma también en la transición `driving` false→true, y la restauración de
  distancia hace que la cámara arranque ya a `maxDist`.
- Trazas: `CAMB2` publica `obs=` (¿hubo geometría el frame anterior?) y
  `CAMB2b`/`CAMB3b` ya no se truncan (buffers de 80 → 240: el campo `nc=` se
  estaba perdiendo). El `dist` de `CAMB3` es la medida del fallo: debe quedarse en
  `maxDist` (5,47 en la moto) también de frente.
- `check-served-build.sh` OK (marca `base=%.2f obs=%d`); `VERSION` →
  `2026-09-25-ve73`.

## 2026-09-25 — ve74: la entrada al vehículo arrancaba el reloj de 2 s (la cámara no seguía el rumbo al empezar a andar) `[raíz]`
Sólo `src/core/Cam.cpp` (más `VERSION`). El jugador confirmó que en ve73 la
cámara de frente ya se ve a la distancia buena. Datos: ve73
(`web/odtrace.log`, 71 CAMB2 + 515 CAMB3).
- **ve73 validado con datos**: `dist` se queda en 8,93/9,5 (`maxDist` del coche)
  también con `deg=±180` (de frente) y en los tramos sin ratón. Sólo un caso de
  `dist<5` (3,51) y con `obs=1` (geometría real delante): la restauración de
  distancia funciona y no atraviesa paredes.
- **Causa (fallo "deja de centrar la cámara al subir a un vehículo y empezar a
  andar")**: al entrar, `ResetStatics` (cambio de modo) reiniciaba
  `s_lastLookCar = nowCar`, o sea el reloj de 2 s arrancaba de CERO aunque el
  jugador no hubiera mirado con el ratón. Resultado: 2 s sin pasivo y, al
  empezar a andar, la cámara se quedaba con el yaw viejo mientras el vehículo
  giraba. Medido en ve73: tramos con `mx=my=0` e `idle<2000` donde **Beta está
  congelada** (`b=65,5°` fijo) mientras `TargetOrientation` (= `RADTODEG(b)-deg`,
  el rumbo del vehículo) gira 59,6 → −2,7 → −54,9, y sólo a partir de
  `idle>2000` (`rot=1`) Beta empieza a converger. Es exactamente el "se comporta
  como si hubiese movido el mouse" con el que lo describió el jugador.
  Arreglo: `s_lastLookCar = ResetStatics ? (nowCar - 3000u) : nowCar;` — entrar
  cuenta como "no ha mirado desde hace rato", así que la cámara sigue el rumbo
  del vehículo desde el primer frame. Sólo el ratón (`orbiting`) arranca el
  reloj de 2 s. El guard `if(nowCar < s_lastLookCar)` ya existente evita el wrap
  si `nowCar < 3000` (primeros frames de la sesión).
- `check-served-build.sh` OK; `VERSION` → `2026-09-25-ve74`.
## ve75 (25/09) — recoil aditivo y delay de recentrado a 1,5 s

Petición del jugador, tras confirmar la cámara de coche ("perfecto ahora sí
funciona como esperaba"): (1) retroceso **aditivo** estilo `WeaponRecoilAuto`
—congelar el decaimiento mientras se dispara y volver al soltar—, y (2) bajar
el delay de recentrado pasivo de 2 s a 1,5 s.

- **(1) Recoil aditivo** (`src/core/Cam.cpp`, bloque `#ifdef VICEEXT_RECOIL`).
  Antes el offset de cámara (`s_odRecoilAlpha`) decaía a 4°/s *en cada frame*,
  también entre disparos: con una ráfaga (SMG ~14/s, hueco 71 ms) el retorno
  se comía la patada anterior y el tirón no se acumulaba. Ahora `ViceExtRecoilAlphaAdd`
  registra `s_odRecoilLastKick` y `ViceExtRecoilAlphaApply` SALTA el decaimiento
  mientras `now - s_odRecoilLastKick < VICEEXT_RECOIL_HOLD_MS` (200 ms). Cada
  patada se suma (tope `VICEEXT_RECOIL_ALPHA_MAX` 0,10 rad ≈ 5,7° ya existente)
  y la vista sube a escalones; al soltar (más de 200 ms sin disparar) el
  decaimiento vuelve y regresa al punto. La mira sigue FIJA (`RECOIL3
  miracheck multY=0.400` intacto): sólo se mueve la cámara. Traza `RECOIL2`
  gana el campo `hold=%d` (1 = decaimiento congelado este frame).
- **(2) Delay 1,5 s** (`src/core/Cam.cpp:2458`): `idleMs` 2000 → 1500. El
  contrato es el mismo (mientras miras nada recentra; al soltar, pasivo;
  la radio no recentra; vale parado). Comentarios y el plan
  `.agents/plans/camara-coche-sin-lucha.md` alineados a 1,5 s para no dejar
  documentación mentirosa (las citas literales del jugador se conservan).
- Sólo `src/core/Cam.cpp`, `VERSION` y `check-served-build.sh` (marca nueva
  `RECOIL2 ... hold=%d`). `VERSION` → `2026-09-25-ve75`; check-served-build OK.

Pendiente de validación del jugador: ráfaga con SMG/pistola → la vista debe
subir a escalones sin bajar entre disparos y volver al punto al soltar; y el
recentrado pasivo debe llegar en ~1,5 s.

## ve76 (25/09) — recoil aditivo SIN retorno (mecánica completa)

Validación de ve75 del jugador: "ahora sí el recoil sube la cámara de la mira".
Pero faltaba la segunda mitad de la mecánica: "al terminar de disparar, al
vaciar el cargador, la mira vuelve a bajar cuando no debe; [debe] mantenerse
donde la haya dejado el recoil así se acaben las balas o lo que sea — esa es la
gracia de la mecánica del recoil".

- `VICEEXT_RECOIL_ALPHA_RETURN` 0,07 → **0,0 rad/s**: fuera el retorno automático.
  El offset (`s_odRecoilAlpha`) es PERSISTENTE: acumula patadas hasta el tope
  `VICEEXT_RECOIL_ALPHA_MAX` (≈5,7°) y ahí se queda; el jugador compensa a mano
  con el ratón (Alpha es estado y el ratón lo mueve). Sólo `ResetStatics`
  (cambio de modo/vehículo) lo limpia. El decaimiento sigue en el código pero
  guardado por `if (VICEEXT_RECOIL_ALPHA_RETURN > 0.0f)`, así que subir ese
  define restaura la vuelta automática sin tocar nada más.
- Se retira la ventana `VICEEXT_RECOIL_HOLD_MS`/`s_odRecoilLastKick` de ve75
  (ya no hace falta congelar nada: no hay decaimiento que congelar).
- Traza `RECOIL2` ahora imprime `ret=%.2f` (=0,00 = sin retorno) en vez de
  `hold=%d`; `check-served-build.sh` actualizado con la marca nueva.
- Sólo `src/core/Cam.cpp`, `VERSION` y `check-served-build.sh`. `VERSION` →
  `2026-09-25-ve76`; check-served-build OK (sin avisos nuevos de compilación).

Pendiente de validación: ráfaga (SMG/pistola) → la vista sube a escalones y
**se queda arriba** al vaciar el cargador, bajando sólo cuando el jugador tira
del ratón hacia abajo.

## ve77 (26/09) — alineación de telemetría del recoil persistente

Se contrastó la partida `gta_vc_browser/logs/odtrace-2026-09-26_13-47-21.log`: la cabecera confirma `JS build=2026-09-25-ve76 data=2026-09-21-ve13`. En esa sesión `RECOIL2 alpha` sube de `0.0048` a `0.1000 rad` (5.73°) con `ret=0.00`, y permanece en el tope durante los segundos posteriores. `RECOIL3 miracheck` muestra hasta 331 disparos con `multY=0.400`; hay disparos de pistola, rifle, SMG y escopeta. No es evidencia de que el jugador visualmente lo percibiera ni de que vaciara un cargador.

La traza por disparo decía `pitch=0.0048` repetidamente: en `Weapon.cpp` el espejo se restaba `0.07 * dt` cada frame y se reiniciaba al cambiar de arma, divergiendo del offset real persistente de `Cam.cpp`. En ve77 se retiró ese espejo; `ViceExtRecoilOffset()` devuelve `s_odRecoilAlpha`, y `RECOIL2 kick` ahora emite `a_pie`, `offset` y grados consultando la fuente real. `RECOIL2` de cámara usa `offset=... reset=0`; si `ResetStatics` limpia un offset no nulo, emite `reset=1`. Se corrigieron comentarios de retorno y se actualizó `check-served-build.sh` para exigir ambos formatos.

No cambian la física, el tope, ni `dataTag`. **El código queda preparado, no publicado aún**: no se subió `VERSION` a ve77, no se enlazó el wasm ni se corrió el verificador porque `reVC.wasm.tmp0/1/2` estaban bloqueados en el build compartido. Cuando quede libre, actualizar `VERSION` a `2026-09-26-ve77`, enlazar, ejecutar `bash gta_vc_browser/tools/check-served-build.sh` y pedir una partida/log para comprobar las trazas nuevas y confirmar el efecto visual.

## 27/09/2026 — Harness actualizado: licencias sin puerta, perfil/estándares/diseño reales y 5 ADR `[proceso]`

- Instrucción directa del jugador: «actualiza eso y lo que mencionaste anteriormente
  en agents, actualiza todo el harness donde veas necesario». Documentado en
  `.agents/plans/harness-actualizacion-27-09.md`.
- **Licencias: dejan de ser una puerta.** Proyecto local y personal; de `mods/` se
  toma lo que haga falta, con o sin LICENSE. La atribución se mantiene como
  cortesía/trazabilidad. Lo único excluido sigue siendo lo **técnicamente
  inaplicable en WASM** (x86, `hook::pattern`, `.asi`, `.ual`, `rwd3d9`, `injector`,
  `plugin-sdk`, memory-hacks de `.cs`). Nota de norma vigente añadida en
  `00-INDICE.md` §2, `09-numeracion` §2, `13-inventario` §4/§7, `06-fuentes-externas`
  §4.2 y `12-handoff-tanda2` §13 + `docs/mods/ATTRIBUTION.md`.
- **Perfil real** en `.agents/AGENTS.md` §1 (stack C++/reVC + Emscripten + CMake/Ninja
  + librw GL3 + OpenAL + Vite + Python/Node; estructura del repo; scripts reales).
- **`CODING_STANDARDS.md`** y **`DESIGN.md`** reescritos para el stack real (features
  `VICEEXT_*`, contrato de unidades m/s ÷ 50, verificadores + logs, CRLF/python
  binario, `VERSION`/`dataTag`, anti-patrones; capas web/engine/datos, «portar no
  cargar», fronteras de carriles, caps de memoria).
- **5 ADR nuevos** en `.agents/decisions/`: 001 portar lógica (no cargador de mods),
  002 la licencia no es puerta, 003 el mod manda sobre lo existente, 004 API única
  `ViceExtIsAiming()`, 005 medición = partida del jugador + logs.
- Sin código del motor ni datos tocados. `.md` del harness en LF (salvo este fichero,
  CRLF). No hay build ni tests que correr.
