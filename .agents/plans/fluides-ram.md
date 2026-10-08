---
name: fluides-ram
status: EXECUTED
type: maintenance
domain: performance
owner_rules: .agents
created: 2026-09-18
---

# Plan: tirones en gameplay + dieta de RAM

Fecha: 2026-09-18 (noche). Estado: APROBADO y APLICADO (build `2026-09-18-ram1`,
verde). Pendiente: sesión de validación del usuario (criterios al final).

## Diagnóstico tirones (con evidencia del log)

FPS estable 58-60 (770 FPSLOG). El problema NO es la media: es el peor frame
de cada segundo.

1. **Micro-jank constante (CONFIRMADO en log): maxdelta 22-54 ms CADA segundo**
   mientras conduce. Presupuesto de frame a 60 fps = 16,6 ms. Causa
   arquitectural: `CStreaming::LoadAllRequestedModels` corre SIN tope en
   gameplay (`gWebLoadBudget=0`; el tope de ficheros solo se activa en la
   carga). Cada frame procesa TODO lo pendiente: leer de MEMFS + parse DFF +
   conversión/subida TXD + instanciación + colisión. Es el punto B del plan
   `fluidez-carga-radio` ("presupuesto ms/frame en el streaming"): nunca se
   implementó.
2. **Picos grandes puntuales (CONFIRMADO): mp3 grandes on-demand.**
   - 17:40:01: hitch 352 ms — `City.mp` ambiente 9,6 MB traído en 17:40:00
     (writeFile + IDB put + apertura stream) + primer burst del área spawn.
   - 17:50:36: hitch 133 ms — `police.mp3` 14 MB traído en 17:50:35.
   - 17:44:55: hitch 483 ms — carga F2 (normal, ya troceada).
   - 17:52:39: hitch 1128 ms — cierre de pestaña (última línea del log).
   Son una vez por sesión (IDB los cachea). Con PERF se decide si acotar el
   prefill/decode de streams grandes.
3. **Ruido de medición**: `decms=0` SIEMPRE en ODSFXSTAT (métrica de decode
   rota) y `ENGAP restart=0ms` siempre (dud). Los tirones de 5-20 ms de
   decodes SFX (200 misses/sesión) son hoy invisibles. `LOOPALIVE` cada 2 s
   por loop = 14k líneas/sesión de ruido en el log.
4. La primera sesión del día 17:39:39 arranca con el área ya cacheada en IDB
   (199 fetches/sesión, casi todos sfx pequeños): los hit de IDB no se
   trazan, por eso el coste fetch/write no es visible salvo en los mp3
   grandes.

## Diagnóstico RAM (desglose del ~1,2 GB)

- **bootseed (MEMFS fijo, no desalojable): 376 MB reales** (el comentario
  "~130 MB" del script está viejo). Dentro: **radios 274 MB** (9 .adf
  enteras — decisión antigua "cambio de emisora instantáneo", HOY redundante:
  `prefetchRadios()` ya las trae por IDB 25 s después del arranque),
  models 81 MB (gta3.img de spawn 68 + generic/particle/frontend), data 11,
  txd 6, anim 3, TEXT 3.
- **OD live (MEMFS desalojable): hasta 400 MB** (`OD.CAP`; LRU 60 s grace,
  re-fetch sin red desde IDB). En la sesión no llegó a desalojar (evict=0).
- **Wasm heap tocado**: ~250-350 MB (mundo, PCM cache 7 MB al final de
  sesión, buffers de parse). INITIAL_MEMORY=512 MB es dirección, no RAM.
- Slack V8/GLue: ~100 MB.
- Total ≈ 1,1-1,2 GB. Encaja con lo que ve el usuario.

## Cambios propuestos

### R1. Radios fuera del bootseed (decisión de usuario a rever: −274 MB RAM, −274 MB descarga)
- `tools/stage_bootseed.py`: dejar solo FLASH en el paquete (música/primera
  emisora) y quitar las otras 8 de `RADIOS` (quedan en `streamed/` y las trae
  `prefetchRadios` a los 25 s; IDB las cachea entre sesiones).
- Efecto: `reVC.data` 390 → ~120 MB (arranque 3× más rápido) y −274 MB RAM.
- Trade-off: cambiar de emisora antes de la prefetch (primeros 25 s) o con
  IDB vacía = espera de 1-2 s la primera vez por emisora.
- Alternativa conservadora: mantener radios en bootseed (RAM alta para
  siempre). Decisión del supervisor.

### R2. Cap de MEMFS 400 → 250 MB (−150 MB)
- `ondemand.js OD.CAP`. El desalojo ya funciona y re-lee de IDB sin red.
- Trazar el census (live/evict/fetched/idbHits) a odtrace cada 30 s (tag
  `ODCAP`) para ver el working set REAL antes de bajar más.

### F1. Presupuesto de TIEMPO por frame en el streaming (el fix del micro-jank)
- En `LoadAllRequestedModels` (web): corte por tiempo ~7 ms de trabajo por
  llamada durante gameplay (budget de ficheros ya existe para la carga; este
  es un reloj). Continúa el siguiente frame. Sin tocar nativo.
- Riesgo controlado: prioridad de script (`priority=true`) sin corte o corte
  generoso (las precargas de misión no deben eternizarse).

### F2. Instrumentación de verdad (una build, luego se mide)
- Arreglar `decms` (medir decode SFX real) y quitar/retirar ENGAP si sigue
  en 0.
- `PERF` 1/s a odtrace: streaming ms/frame, decodes ms, heap wasm
  (`emscripten_get_heap_size`), JS heap si `performance.memory` existe,
  suspensions Asyncify. Una sesión de 3 min conduciendo discrimina el jank
  residual que quede tras F1.
- `LOOPALIVE` cada 10 s (o solo en cambio de estado) — el log respira.
- Heap en FPSLOG (`heap=MB`): serie temporal gratis de RAM del motor.

### F3 (opcional, solo si PERF lo delata): prefill/decode acotado en
`CStream::Open` para ficheros >8 MB y prefetch de ambientes (City/Water) en
segundo plano tras el arranque (como prefetchRadios/Loops).

## Orden
1. R1 (si se aprueba) + R2 + F2 (instrumentación) — misma build.
2. F1 (presupuesto por frame).
3. Sesión de 3 min del usuario → leer PERF/ODCAP → decidir F3 o ajustar CAP.

## Cierre por build
- FPSLOG: maxdelta p95 < 20 ms conduciendo (antes 22-54 cada segundo), 0
  hitches fuera de carga/cierre.
- ODCAP muestra live estable < 250 MB; heap serie plana; RAM de pestaña
  ~700-800 MB (estimado post-dieta).
- Sin regresión: radio cambia (primera vez con espera aceptable), carga F2,
  partida nueva, sonidos con loop sin cortes (LOOPSTART/END sanos).
