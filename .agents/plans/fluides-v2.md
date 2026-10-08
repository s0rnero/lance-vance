---
name: fluides-v2
status: EXECUTED
type: maintenance
domain: performance
owner_rules: .agents
created: 2026-09-18
---

# Plan: fluidez v2 — re-implementación correcta + 120 fps

Fecha: 2026-09-18 (noche). Estado: **F5c construida (`2026-09-18-fluides-f5c`),
esperando sesión larga del supervisor** (conducir 5+ min con ?fps, cambiar de
radio varias veces; leer evict/ASYNC/strm para confirmar que el thrash murió).
Base: D1 carga OK (diagnóstico cerrado, ver `diagnostico-crash-init.md`).
Lección central de las ram1-4: **NADA dentro de `LoadAllRequestedModels`**
(es suspendible por Asyncify; el control flow nuevo ahí rompe el rewind).

## F5c — implementación (ondemand.js, sólo pre-js + relink)

- **Fuera la prefetch masiva de las 9 radios** (270 MB que ahogaban el cap —
  el thrash medido en lean1: 1683 ficheros/385 MB re-traídos; 90% de
  segundos con >100 ms de stalls asíncronos).
- **La emisora que suena se detecta** en el wrapper de `ensure` (regex sobre
  la ruta .adf contra el dial) → `noteStation`.
- **`noteStation`**: guarda la última usada (localStorage) y precarga LA
  SIGUIENTE del dial a los 3 s — cambiar de emisora sin freeze. Las
  prefetch internas usan `_ensure` directo (no re-disparan la cadena).
- **Al arrancar (15 s)**: precarga la última emisora usada (localStorage) —
  la primera play del coche suele ser sin freeze.
- LRU hace el resto: las emisoras inactivas envejecen y salen solas; el
  mundo tiene el cap para sí.
- **CAP 250 → 300** (mundo ~150 + emisora ~30 << 300 → sin thrash).
- Criterio de aceptación: sesión larga con `evict` bajísimo (<50 ficheros),
  ASYNC de >100 ms sólo en cambios de zona, cambios de radio sin freeze
  perceptible tras el primero, RAM ~1,3 GB.

## Higiene de log (petición supervisor, lean1)

Problema descubierto: el tope del log era un contador GLOBAL del servidor
(80k líneas, nunca se reiniciaba) → la sesión larga del usuario se cortó a
los 8 s ("ODCAP drop"). Arreglado:
- `vite.config.js`: el log SE ROTA al arrancar cada sesión nueva (primer
  lote con `JS build=`) + cap de seguridad 200k. **El Vite dev server se
  reinicia solo al detectar el cambio de config.**
- C++/wasm (lean1): fuera TXDAUD (de nuevo), GEAR/GEARP/ENGAP (AudioLogic),
  CHSTOP, LOOPSTART/LOOPALIVE/LOOPEND (queda la lógica funcional
  LOOPKILL/LOOPORPHAN), BIND2 completo (vendor gl3device).
- Caps reducidos a ~muestra: ALPHADRAW 512→20, TEXA 900/300→10/3 (+KEY 6),
  CONVDXT IMG/ALFA 400/900→6, ALFAMIP 250→4. `[texconv]` (errores) queda 30.
- QUEDAN: errores (FAIL model/txd/col, MISS/SHORT, hackRead NIL, SKIP tex),
  FPSLOG (avg/maxdelta/heap/strm), ASYNC (suspensiones), ODSFXSTAT/MISS/SFXLEN,
  CHINIT ms, RETUNE/RADIOTRACK, SFXDATA, addimage/loadlevel, initstep,
  webload/loadtick/loaded pos, WR, ODCAP.
- Nota: Vite rearranca al cambiar vite.config.js — recargar la página tras
  el reinicio (el título debe decir lean1).

## F1b-A2 — implementado así (todas las lecciones de ram1-4)

- Tick del SÍ (`GS_FRONTEND`, web): NO llama a InitialiseGame; pone
  `gWebBootInitPending=1` y cede (el menú sigue visible ese tick).
- Tick+1 (`AfterInner`, ANTES de cualquier teardown — lección ram2): pinta
  `LoadSplash(splash1)` + `WebDrawLoadScreen(0)` y cede → **el frame
  PRESENTA el splash**.
- Tick+2: `InitialiseGame()` MONOLÍTICO (una sola cadena de suspensiones,
  como el flujo validado — lección ram4) con guardia in-flight (lección
  ram3: ticks nuevos ceden hasta el rewind). El freeze de ~4 s ocurre CON
  el splash en pantalla (el canvas conserva el último frame presentado).
- Al terminar: limpia flag, cede → el teardown+restart troceado validado
  arranca al tick siguiente.
- Partida nueva: intacta (va por `GS_INIT_PLAYING_GAME`/`InitialiseGameStep`).

## F4a — trazas implementadas

- `ASYNC n= ms=` (1/s, ondemand.js): nº de `ensure()` atendidos y wall-ms
  totales — el coste de las suspensiones que pide el wasm. Vía prioritaria
  del flush (main.js ODPRI).
- `CHINIT ch= sfx= ms=`: coste real del arranque de canal (SetSampleData
  puede traer la muestra on-demand). Patrón t0/resta-tras-resume validado.
- Emparejar con `FPSLOG maxdelta` del mismo segundo para el veredicto F4b.

## Re-ordenación basada en evidencia (F1c → F2)

- **Hallazgo estructural**: la streaming de GAMEPLAY real va por
  `Update → LoadRequestedModels` (canales asíncronos), NO por
  `LoadAllRequestedModels` (sólo cargas/sync-points). El presupuesto F1
  (fijo 6) nunca tocó la vía que corre al conducir — por eso el jank
  persiste (mediana del peor frame = 26 ms en la sesión larga).
- **La correlación con las trazas actuales es inconclusa** (jank en el 82%
  de los segundos; ODSFXMISS/CONVDXT/CHINIT a tasa similar con y sin jank).
  → No hay throttle que justificar todavía: PRIMERO medir (F2), DESPUÉS
  apuntar el fix (F4) a lo que los datos señalen. F1c (adaptativo en
  LoadAll) queda APARCADO: mecanismo sobre la vía equivocada.
- El presupuesto fijo 6 de F1 se conserva (protegía sync-points de script;
  inofensivo y validado en la sesión F1).

## Lo aprendido (reglas de diseño que este plan respeta)

- R1: el bucle de streaming no se toca: ni tiempo, ni breaks, ni contadores.
  Toda política va en el LLAMADOR o vía mecanismos ya validados.
- R2: el mecanismo de presupuesto por CONTEO de ficheros
  (`gWebLoadBudget`) está validado por semanas (lo usa la carga).
- R3: cada fase = una build = una prueba del usuario. Nada se apila sin
  validar la anterior.

## F1 — RESULTADO (sesión 20:26, usuario)

- Carga OK ✓ (budget por conteo validado también en partida nueva implícito).
- Jank reducido: 5/11 s con maxdelta 22-50 ms (antes: cada segundo). Muestra
  corta — F2 dará la serie larga.
- Cap 60 fps = refresco del monitor (esperado; F3decide el límite interno).
- **Pendiente detectado por el usuario: freeze de ~4,2 s al dar SÍ**
  (maxdelta=4172ms = el init monolítico corre en el MISMO tick del clic,
  antes de que el splash exista; luego 2239/1090 ms = ticks del restart).
  → fase F1b.

## F1 — RESULTADO CORREGIDO (sesión larga 20:22, 165 muestras)

- Carga OK ✓ (dos cargas en la sesión). FPS p50=60 (cap del monitor ✓).
- **CORRECCIÓN: el jank NO quedó resuelto.** La primera lectura usó la
  sesión corta equivocada. En la larga: **mediana del peor frame = 26 ms**
  (más de la mitad de los segundos tuvieron un frame ≥26 ms; p95 de frames
  por debajo de 20 ms = casi ninguno). El presupuesto FIJO de 6 ficheros
  acota la cantidad, no el coste: un lote de 6 con un TXD pesado dentro =
  frame de 30-50 ms igualmente.
- Los 3-5 hitches grandes (2429/3025/1658/651 ms) = los inits monolíticos
  de las cargas + ticks pesados del restart → fase F1b.
- 149 ODSFXMISS = audio on-demand funcionando (benigno). ODCAP: eviction
  activa (352 ficheros/60 MB), live 242-299 estable → dieta RAM sana.

## F1c — Presupuesto ADAPTATIVO en el llamador (el fix real del micro-jank)

- Problema del fijo: acota ficheros, no milisegundos. Solución: el LLAMADOR
  (`CStreaming::Update`, web) mide el coste de cada llamada
  `LoadAllRequestedModels` (tiempo ALREDEDOR — R1: nada dentro) y ajusta
  `gWebLoadBudget` entre 1 y 12: si la llamada costó >12 ms y procesó
  ficheros → budget--; si costó <4 ms → budget++ (hasta 12).
- Efecto: los lotes con TXD pesados se encogen a 1-2 ficheros/frame
  automáticamente; los ligeros crecen. El peor frame se acota a ~12-15 ms
  sin tocar el bucle de streaming.
- Aceptación: sesión larga con FPSLOG → mediana y p95 del peor frame < 20 ms
  (idealmente ~17), sin regresión de throughput (ODCAP fmb/s no cae).

## F2 — RESULTADO (sesión 20:51, 176 muestras, instrumentación viva)

- **El streaming de gameplay está INOCULADO en el jank típico**: `strm` p50
  = **5 ms/s** (la mitad de los segundos casi no streamea); p95=116, max=308
  (sólo en ráfagas de cambio de zona). El jank típico (p50=24) NO es streaming.
- **decms real por ventana de 10 s**: incrementos de ~5-90 ms/10 s (~2-9
  ms/s) — los decodes de SFX existen pero son secundarios.
- **Correlación de los 116 segundos con jank vs 60 limpios**:
  - `CHINIT` ×4 (1.5/s con jank vs 0.4/s limpio) — arranques de canal de audio.
  - `ODSFXMISS` ×3 (1.5/s vs 0.5/s) — primer play de una muestra = fetch
    (suspensión Asyncify) + decode.
  - `RETUNE`/`RADIOTRACK` presentes sólo en segundos con jank.
  - Texturas (TEXA/CONVDXT/TXDAUD): tasa IGUAL con y sin jank → inoculadas.
- **maxdelta: p50=24, p75=39, p95=64, max=2109** (el 2109 = el init
  monolítico del SÍ — fase F1b). Heap wasm plano en 512 MB (sin fuga).
- Comparación F1→F2: no hay cambio de comportamiento esperable (F2 sólo
  mide); la "leve mejora" es varianza de sesión.

**Hipótesis líder (a confirmar con F4a): el coste del camino ASÍNCRONO del
audio** — cada primer-play de una muestra pasa por el fopen envuelto →
OD.ensure → **suspensión+rewind de Asyncify (copia de stack de 1 MB ida y
vuelta)** + decode + upload. ~1.5-2 de éstos por segundo ≈ el patrón de un
frame de 24-39 ms por segundo. Los CHINIT pueden añadir coste propio.

## F4a — Trazar las suspensiones y el camino de audio (próxima build)

- T-A: en el puente JS de `OD.ensure`, medir por llamada: ms de pared y
  cuenta; agregar por segundo al odtrace (`ASYNC n= ms=`). Dice CUÁNTAS
  suspensiones/s y cuánto cuestan.
- T-B: `CHINIT` con coste: ms del `InitialiseChannel` (medido en el
  llamador del sampman, no dentro del bucle).
- T-C: `RETUNE` con ms de la apertura del stream de radio.
- Todo es medición en límites llamador/JS — R1 intacto.

## F5 — DIAGNÓSTICO DE LA SESIÓN LEAN1 (7,7 min, 464 muestras) — THRASH DEL CAP

Datos:
- **420 de 464 segundos (90%) tuvieron ventanas ASYNC de >100 ms** — miles
  de mini-stalls repartidos por frame = el "laggy continua" del usuario.
- El coste por suspensión es de ~15-25 ms (p.ej. 18 llamadas = 367 ms en
  una ventana; 21 = 282 ms). Son unwinds/rewinds de Asyncify + IDB.
- **Causa raíz: el CAP 250 es menor que el working set real.** ODCAP:
  evict=**1683 ficheros / 385 MB** desalojados y re-traídos (fmb=540 MB) —
  thrash puro. El detonante: **prefetchRadios mete las 9 emisoras (~270 MB)
  en el mismo pool LRU que el mundo (~150 MB)** → caben mal en 250 → el LRU
  expulsa ficheros del mundo que se re-piden al conducir.
- **El freeze de la radio**: primera reproducción de una emisora = fetch de
  28-37 MB (una suspensión gorda) — y peor si la prefetch ya fue desalojada
  por el thrash.
- RAM estable en 1,2-1,3 GB — composición medida: memfs ~358 MB (bootseed
  126 + live 234 al tope del cap) + wasm 512 MB fijos + heap JS ~400 MB
  (V8 no devuelve el slack del churn al SO). La dieta recortó el bootseed,
  pero el working set runtime + el suelo del wasm dominan.
- strm p50=5 ms/s (streaming inoculado otra vez); maxdelta p50=21;
  avg FPS p50=60, p10=59 (la media esconde los stalls de ASYNC).

### Propuestas (elegir; sin código ejecutado aún)
- **F5a (1 línea, rápido)**: CAP 250 → **500**. Radios (270) + mundo (150)
  caben; thrash → 0; suspensiones sólo para ficheros nuevos. Coste: RAM
  ~1,4-1,5 GB (sube ~200 MB).
- **F5b (equilibrado)**: fuera el prefetch masivo de radios — sólo la
  emisora en reproducción vive en MEMFS; CAP 300. Thrash → 0 (150 mundo +
  30 radio << 300). RAM ~1,3 GB. Coste: cambiar a emisora fría = freeze de
  1-2 s (el de hoy).
- **F5c (el mejor, más trabajo)**: prefetch inteligente — la emisora actual
  + la siguiente en background (cambiar de emisora sin freeze), evicción
  prioritaria de estaciones inactivas, CAP 300. RAM ~1,3 GB.
- En todos los casos el resto de suspensiones (ficheros nuevos de zonas
  nuevas) queda en pocas por segundo → el laggy continuo desaparece.
- Pendientes fuera de F5: F1b-A1 (init troceado: cero freeze de carga +
  mundo 1 vez), F3 (límite 120 — en 60 Hz el contador seguirá en ~60).

## F5c — RESULTADO (sesión 21:59, 393 muestras, 6,5 min)

**Ganado (datos reales):**
- **Thrash muerto**: evict 1683 ficheros/385 MB (lean1) → **335/64 MB**.
  Ventanas ASYNC >100 ms: de 420/464 segundos → **11/291**. El "laggy
  continuo" (miles de mini-stalls) desapareció.
- La prefetch de "la siguiente" FUNCIONA: el tap a file=3 sonó limpio
  (maxdelta 24 ms) cuando la prefetch llegó a tiempo.
- RAM: live 289-290 (bajo el cap 300), memfs 415, js 450 — como previsto.

**Pendiente 1 — el freeze de la radio, medido y explicado:**
- Cada retune a emisora FRÍA = **UNA suspensión de 170-246 ms** = UN frame
  congelado (21:59:26=263ms n=41 ms=242; :29=241ms ms=232; :30=246ms;
  :32=190ms — todos sobre `RETUNE consume`/`RADIOTRACK`). Es la fetch del
  .adf de 28-37 MB (IDB + escritura a MEMFS) dentro del open del stream.
- La prefetch "siguiente" no alcanza cuando el usuario pasa emisoras a 1/s
  (cada prefetch tarda 3 s en arrancar).
- **F5d — radio muda hasta lista (propuesta del supervisor)**: en `ensure`,
  para rutas `/Audio/*.adf` que NO estén ya en MEMFS: lanzar la fetch en
  background (fire-and-forget) y devolver null AL INSTANTE → el open falla
  sin congelar → la radio queda muda 1-2 s y arranca cuando el fichero
  aterriza (el motor re-intenta el open en su servicio — VERIFICAR en
  MusicManager al implementar; si no re-intenta, reintentar desde JS no es
  posible, valorar alternativas). Cero freezes garantizados.

**Pendiente 2 — el baseline de ~21 ms sigue sin explicar:**
- Fuera de los eventos (radio/carga/zonas), el peor frame de cada segundo
  sigue en 19-30 ms SIN correlación con ASYNC (p.ej. 21:59:36: 26 ms con
  sólo 43 ms de async). Constante en TODAS las builds desde lean1 atrás.
- Sospechoso principal: **pausas de GC de V8** (js=450 MB con churn de
  buffers). **F4c (propuesta)**: medirlo desde main.js — delta de RAF por
  segundo + `performance.memory.usedJSHeapSize` (bytes liberados por GC por
  segundo) → una sesión dirá si el ~21 ms es GC o el propio motor.
- Si es GC: reducir churn (reusar buffers en ensure, espaciar ODCAP) o
  asumirlo (V8 no da más palancas).

## F1b — El freeze del SÍ: que se congele MIRANDO el splash

Estado actual: SÍ → el mismo tick llama a `InitialiseGame()` (bloquea ~4 s
con el diálogo de confirmación en pantalla) → después el restart pinta el
splash. El canvas sí conserva el último frame presentado durante un freeze:
basta que el splash se pinte y SE PRESENTE antes de bloquear.

### Opción A2 (recomendada primero): splash primero, init diferido 1 tick
- Tick del SÍ: pintar splash + `WebDrawLoadScreen(0)` y DIFERIR el init
  (flag, sin llamar a InitialiseGame); el frame termina y PRESENTA el splash.
- Tick siguiente: el init corre monolítico (igual que hoy, mismas
  suspensiones/rewinds ya validadas) desde `AfterInner` con guardia in-flight
  (patrón ram3, cuyo hang era con C6 encima — nunca probado limpio).
- Qué resuelve: el freeze pasa a ocurrir CON el splash en pantalla. No
  elimina el freeze (~4 s) ni el doble build del mundo.
- Riesgo: medio — mueve la llamada de InnerFrame a AfterInner (estructura
  que vio problemas, pero SIEMPRE con C6 presente y con init troceado; el
  monolítico es UNA cadena de suspensión, no varias). Fallback: revertir a
  F1 tal cual (1 revert).

### Opción A1 (después de A2 validada): init troceado de verdad
- Sobre el shell de A2 ya validado: sustituir la llamada monolítica por
  `InitialiseStep` por tick con guardia in-flight (ram3, sin C6).
- Qué resuelve además: cero freeze (progreso en la barra) y **el mundo deja
  de construirse 2 veces** (el objetivo original de
  `carga-sin-doble-trabajo`).
- Riesgo: el que fue — si rebrota algo, el método D lo acota y el fallback
  es A2 ya validada.

### Prueba de aceptación F1b
- A2: al dar SÍ el splash aparece ANTES del freeze (criterio del usuario:
  "que se freeze ya mirando el splash"), carga completa OK, sin regresión.
- A1: además, barra avanzando y sin freeze perceptible; partida nueva OK.

## F3 — Límite 120 fijo (decidido con el supervisor)

- El limitador interno del juego fija 120 SIEMPRE (independiente del
  refresco del display, a modo de prueba). En un monitor de 60 Hz el
  contador seguirá en ~60 (vsync del navegador) — no es un fallo del límite;
  en 120 Hz debe verse ~120 real.
- Pasos: localizar el limitador (menú/frame limiter de reVC) y en web fijar
  target 120; verificar timestep (física idéntica a 60); FPSLOG como juez.

## F2 — Instrumentación segura (lote de medición, sin bucles suspendibles)

- C2 `RsTimer` en `webload: total=` (Game.cpp; 2 llamadas totales, fuera de
  bucles) — timer honesto de carga.
- C4 `decms` con reloj de pared (sampman_oal.cpp; llamadas llanas, sin
  control flow nuevo) — saber el coste real de decodificar SFX.
- C5 `LOOPALIVE` 2 s → 10 s (AudioManager.cpp; cadencia de traza).
- C3v2 FPSLOG: `heap=MB` (`emscripten_get_heap_size`, llamada llana en
  WebDrawFps) + `sf=` ficheros servidos/segundo contados en el LLAMADOR
  (delta de `CountPendingRequests` en `CStreaming::Update`, aritmética
  pura). Nada dentro de LoadAll.
- C1 (TXDAUD) NO vuelve: D1 probó que sobra (era ruido de diagnóstico).

## F4 — Si queda jank residual tras F1-F3 (solo con datos)

PERF temporal 1/s (todo a nivel llamador/JS): streaming ms (delta de tiempo
ALREDEDOR de la llamada en Update — nunca dentro), decodes ms (de C4),
suspensions/s, GC (`performance.memory`). Una sesión de 3 min discrimina.
Sospechosos a medida: batch de conversión DXT (software si no hay S3TC),
subida GL de texturas, pausas de GC de V8.

## F5d + F4c — EJECUTADO (2026-09-18 noche, build `2026-09-18-f5d-f4c`)

- **F5d entró en la build** al reenlazar (el pre-js se incrusta: el cambio a
  las 17:14 no estaba activo). Camino del motor verificado en
  `src/skel/ondemand.cpp` + `MusicManager` (reintenta mientras el stream no
  suena) → la radio se recupera sola tras el silencio. Pendiente: prueba del
  usuario cambiando de emisora fría (radio muda 1-2 s, sin freeze).
- **F4c implementado** (`web/src/main.js`, `jankProbe()`): línea
  `PERF frames= maxd= heap= freed=` por segundo (peor intervalo entre frames +
  bytes que libera el recolector). Activo al recargar (no requiere build).
- **Primera lectura (headless, menú, 92 s)**: mediana del peor frame **18 ms con
  `freed=0`** (sin recolección en ese segundo); una recolección de 127 MB cayó
  en un segundo de `maxd=23 ms`. → **El recolector de basura NO explica el
  baseline de ~20 ms.** Falta la lectura en gameplay (conduciendo), que es
  donde aparecía el tirón.

## F4c — PRIMER ANÁLISIS EN GAMEPLAY (sesión 22:34, 341 s, build `f5d-f4c`)

- F5d confirmado por el jugador: 60 cambios de emisora, **cero freezes**.
- **Síntoma caracterizado**: 60 fps (325/341 segundos a 60-61 frames/s) pero el
  **100 % de los segundos tiene un frame >16,7 ms**: p50 21, 78 % >20, 16 % >25,
  6 % >33 (frame perdido real), 1 % >50. Ese es el "un poco laggy".
- **Descartados**: recolector (-0.01), streaming (-0.04), audio (mismo p50 con
  y sin ODSFXMISS/CHINIT), cambio de emisora, suspensiones Asyncify como causa
  del baseline (13,9/s, ~1 ms; explican los 26 picos: media 50,9 ms con 21+).
  El motor y el navegador ven el mismo frame tardío (p50 23 vs 21 ms) → es real
  y está dentro del frame.
- **Culpable candidato eliminado**: `OD.trace` usaba **XHR síncrono** 1/s
  (bloquea el hilo). Ahora encola en `__odq` (no bloqueante). Build `lag1`.
- **Instrumento v2**: `PERF` con `at=` (posición del frame tardío), `over17=`,
  `p90=`, `xhr=N/ms@offset` y `dom=N/ms` (trabajo de la página por segundo).
- Próximo veredicto: si el frame tardío persiste con `xhr=0`, no es la
  instrumentación; toca medir el frame del motor por fases (build).

## Orden de ejecución

1. **F1b-A2 + F4a en la misma build** (independientes: una arregla el UX del
   freeze, la otra traza suspensiones/audio) → prueba del usuario: splash
   antes del freeze + sesión larga para leer `ASYNC`/`CHINIT` ms.
2. **F4b** dirigido por esos datos → sesión larga → mediana/p95 del peor
   frame < 20 ms.
3. **F1b-A1** (init troceado, cero freeze, mundo 1 vez) sobre el shell ya
   validado → prueba.
4. F3 (límite 120) → prueba en 60 Hz (~60 esperado por vsync) y 120 Hz.
5. F2 restante ya aplicado; F4 extra sólo si los datos lo piden.

## Nota vsync (pregunta del supervisor, respuesta honesta)

En el navegador NO se puede desactivar vsync: el bucle principal es RAF y el
compositor sólo entrega frames al ritmo del display. Con el límite interno a
120, en un monitor de 60 Hz el contador seguirá marcando ~60 (no es fallo);
sólo en displays de 120 Hz+ se verá ~120 real. WebGL no expone "present sin
vsync" desde una página. El límite 120 sirve para que el MOTOR nunca sea el
techo; el display siempre lo será.

## F6 — fases del frame + reparto de contenido (build `2026-09-18-lag2`)

Petición del jugador: (1) instrumentar el frame por fases (lógica/dibujado/
entrega) para localizar los ~4 ms que se pierden una vez por segundo y corregir
esa fase; (2) que texturas y sonidos nuevos lleguen repartidos entre frames en
vez de en ráfaga (picos de 30-50 ms al entrar en zona nueva).

**Instrumento (F6a).** Una línea `FPHASE` por segundo, con el frame de peor
intervalo del segundo y sus seis tramos (`L` lógica, `A` audio, `R` lista de
render, `S` escena 3D, `T` 2D, `P` entrega) más los del frame anterior, `sum`
y `wait = gap - sum`. Lectura: `sum ≈ gap` → el coste es CPU nuestra y el tramo
señala dónde; `wait` grande → el trabajo ya está lanzado y el retraso es del
navegador (GPU/compositor/vsync). Nota de plataforma: en WebGL
`glfwSwapBuffers` es no-op y buena parte del dibujado es asíncrono, así que
parte del coste de dibujar aparece como `wait`, no en `S`.

**Reparto (F6b).** El presupuesto por TIEMPO (3/10 ms) dentro de
`LoadAllRequestedModels` se probó en `lag2` y **se REVIRTIÓ en `fixload1`**:
esa función es un punto de sincronía para el init y los scripts, y el corte
deja a `CPed::SetAnimOffsetForEnterOrExitVehicle` leyendo memoria que no está
(OOB; crash ya bisecado antes, ver `diagnostico-crash-init.md`). R1 queda
reforzada: NADA de tiempo dentro del bucle de streaming. Se conserva el
tope por CONTEO (6/10, R2) y el instrumento `ODLOAD big`, que mide qué
ficheros se pasan solos del presupuesto. Si se retoma el reparto de texturas,
va en el LLAMADOR y con esos datos delante.
Presupuesto por frame en los decodes de SFX (2 / 5 ms inline) con cola
diferida drenada a 1 sample/frame y promoción de peticiones repetidas (motor,
sirenas): eso SÍ se queda (va por otra vía, no toca el streaming).

**Regresión automatizada (nueva).** `gta_vc_browser/tools/slot0-load-test.mjs`
carga la partida del slot 0 en headless y falla si el init no completa
(`WR load-req` + `FPHASE`, cero errores de página, cero `ODANIMFAIL`). El save
lo aporta `tools/extract-save-from-profile.mjs`. Antes: FAIL con `lag2`, PASS
con `fixload1`.

**Hedge de entrega (F6c).** `showRaster` llamaba `glfwSwapInterval` en CADA
frame; en Emscripten eso reasigna el reloj del bucle principal
(`set_main_loop_timing`). Ahora solo cuando cambia.

**Pendiente de datos** (sesión del jugador con `?fps`): veredicto del tick
tardío de ~4 ms a 60 fps (tramo o `wait`), lista `ODLOAD` de ficheros que se
pasan solos del presupuesto, y `ODSFXDEFER`/`ODSFXSTAT` para confirmar que el
reparto de audio actúa sin perder sonidos.

## Cierre por build

- F1b-A2: splash visible durante el freeze del init; carga OK.
- F1b-A1: sin freeze perceptible, barra avanza, mundo construido 1 vez
  (verifiable por initstep + ausencia del doble pase), partida nueva OK.
- F2: logs con timer honesto + heap + decms reales, sin regresión.
- F3: límite 120 activo; en display 120 Hz avg ≈ 120, hitch=0, física igual
  que a 60.
