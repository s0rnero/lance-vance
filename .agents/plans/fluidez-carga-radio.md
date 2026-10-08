---
name: fluidez-carga-radio
status: EXECUTED
type: maintenance
domain: performance
owner_rules: .agents
created: 2026-09-18
---

# Plan de cambios: fluidez de carga, lag post-carga y radio

Fecha: 2026-09-18. Origen: super-investigación 2026-09-18 (ver conversación).
Objetivo: que cargar partida no congele la pestaña, mostrar portada + progreso real,
eliminar el lag de los primeros minutos y que un tap de radio = una emisora.

## A. Congelamiento al cargar partida + portada con progreso

### Causa
Al confirmar carga, `GenericLoad()` + recarga de escena corren síncronos
(`src/core/Game.cpp:928-937`), y en web el freeze grueso está en el **case 2 del
restart troceado** (`InitialiseRestartStep`, `Game.cpp:968+`). La portada no sale
"tarde": **no sale nunca** (sin present en los ticks `AfterInner`) → el splash GL no
es viable sin más; va overlay DOM primero.

### Cambios
1. **Pantalla de carga in-game (decisión usuario: nada de overlay DOM)**:
   `WebDrawLoadScreen(frac)` en `src/core/main.cpp` — splash vanilla (`loadsc`,
   vía `LoadSplash` cacheado) + barra con la geometría/colores de `LoadingScreen`,
   sin texto. Se dibuja una vez por tick en la rama `wantToLoad` de `AfterInner`
   (`glfw.cpp`); `InnerFrame` no corre con `WantToRestart`, así que es el único
   escritor del canvas (sin parpadeos) y el navegador presenta al volver cada tick.
   La fracción vive en `gWebLoadFrac` (`main.h`): 0.01 parse, 0.05–0.55 edificios,
   0.57–0.95 zona, 1.0 fin. Si el splash aún no está (`m_pTexture==nil`) no dibuja
   (fallback seguro: queda el menú).
2. **Barra con progreso real**: la fracción sale de peticiones pendientes
   (`CountPendingRequests`), no de fases fijas.
3. **Troceado por ticks, no sleeps en el loop**: `CStreaming::LoadSceneStep()`
   (mismo orden que `LoadScene`: purga+request big → oleada 1 → instanciar big+
   request zona → oleada 2 → instanciar), con `gWebLoadBudget = 10` ficheros por
   `LoadAll`; `GenericLoad` difiere la cola (`gWebDeferSceneLoad`) y el paso final
   corre colisión + tidy + scripts en orden vanilla. El overlay DOM queda dormido
   (no se usa).

### Verificación
- Cargar partida de la playa: overlay al instante, barra avanza, cero
  "esperar/salir". Arnés `loadsave.mjs` en verde.
- **Regresión F2 obligatoria** (se toca el restart de los 4 días): slots listan,
  save/load OK con `GenericLoad OK` + `loaded pos` en log.

## B. Lag post-carga (minutos, luego fluido)

### Causas candidatas (ordenadas por evidencia)
1. **Asentamiento del streaming**: cientos de TXD/modelos bajo demanda al spawnear
   (fetch + decode DXT software + subida GL). Caro por definición los primeros minutos.
2. **Traces propios con XHR síncrono**: `TXDAUD/BIND2/GEAR/ALFA/ALFAMIP` hacen
   `x.open('POST', '/odtrace', false)` por línea. Cientos de roundtrips bloqueantes
   en fase de carga = jank nuestro y medible.
3. **Shaders**: WebGL compila cada programa en su primer uso; zona nueva = tirones.
4. **Audio**: el motor re-decodifica el MP3 en cada restart (~1.4 s, `ENGAP=0ms`
   medido: sin stall pero con churn de malloc); los seeks de radio decodifican de cero.

### Cambios
1. **Primero el remedio, después la medida**: `TXDAUD` (y resto de trazas) pasan a
   envío async/batch con **cap global** ANTES de medir nada — sin cap es
   probablemente medio lag él solo (miles de POST síncronos en la tormenta de carga).
2. **Instrumentación mínima y temporal**: una sola línea `PERF` por segundo a odtrace
   (frame avg/max, streaming pendiente, texturas convertidas, shaders compilados,
   decodes audio, XHRs). Una sesión de 3 min conduciendo delata al dominante.
3. **Wins inmediatos**:
   - Traces async (punto 1, bloqueante para todo lo demás).
   - **Presupuesto ms/frame en el streaming**: tope de trabajo de streaming por
     frame (corte por tiempo, resto al siguiente tick).
   - **Census menos frecuente**: espaciar los censos/conteos periódicos del loop.
   - Caché PCM de los bancos de motor (no re-decodificar cada ~1.4 s).
   - Prewarm de shaders: compilar al inicio los programas GL habituales con una
     escena dummy (1 draw por programa) — si no hay método concreto barato,
     se quita en vez de prometerlo.

### Verificación
- `PERF` muestra qué cae tras los primeros minutos; frame p95 estable desde el
  minuto 0 tras los wins. Sin regresión en `TXDAUD/ALFA` (siguen llegando al log).

## C. Radio que retrocede al cambiar rápido

### Causa
Cada pulsación suma `gNumRetunePresses` y **reinicia** la espera de 20 frames
(`src/audio/MusicManager.cpp:564-571`, consumo en `660-667` y `717-718`,
`GetNextCarTuning()` en `1151-1173`). Con lag, 20 frames son segundos: el usuario
cambia, no pasa nada, repite taps (cada uno resetea la espera), los taps se acumulan
invisibles y al expirar salta N emisoras de golpe; con ~N = vuelta completa al dial
cae de nuevo en la emisora inicial ("retrocede"). Agravante: `ServiceTrack` corre
cada 4 frames (`:455`) y la apertura del stream es lenta, así que cada salto tarda
más y el usuario tappea más.

### Cambios (solo UX de retune, sin tocar streams, todo bajo `__EMSCRIPTEN__`)
1. Ventana de retune por **tiempo real** (~300 ms) en vez de 20 frames
   (usar `CTimer`, como ya hace `RadioStaticTimer` en `:711-715`).
2. No reiniciar la cuenta en cada tap: arrancar solo si está en 0; consumir al expirar.
   Resultado: un tap = una emisora, con o sin lag.
3. **Feedback de estático al tap**: sonar el clic/static nada más pulsar (no al
   expirar la ventana), para que el input se sienta instantáneo aunque el stream tarde.

### Verificación
- Con lag inducido (throttleniosk), 1 tap = 1 emisora; 3 taps rápidos = 3 emisoras,
  sin vueltas al dial. Sin regresión en scroll rueda (`RADIO_SCROLL_TO_PREV_STATION`)
  ni en radio por script/misión (`m_bRadioSetByScript`).

## Orden de ejecución
1. B.1 (traces async + cap global — bloquea medir el resto).
2. A (overlay DOM + barra + troceado por ticks del case 2, con regresión F2).
3. C (retune por tiempo + static al tap).
4. B resto (build `PERF`, presupuesto ms/frame, census, caché PCM motor, prewarm).

## Cierre por build
`loadsave.mjs` verde + cero `FAIL txd` + oído en radio (cambios rápidos 1→2→3 sin
vueltas al dial, static inmediato al tap).
