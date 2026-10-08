---
name: diagnostico-crash-init
status: EXECUTED
type: bugfix
domain: engine
owner_rules: .agents
created: 2026-09-18
---

# Plan: diagnóstico con causa probada del crash de init (OOB en anims)

Fecha: 2026-09-18 (noche). Estado: **EN CURSO — D1 construida, esperando prueba de carga del supervisor.**
Regla acordada: nada de revertir X o Y a ciegas; cada build del diagnóstico
responde UNA pregunta y su resultado queda escrito aquí.

## Resultados

- **D1** (build `2026-09-18-diag-d1`): C++ idéntico a loaddrain (C1-C6
  revertidos: presupuesto 7 ms, RsTimer, FPSLOG heap/sm, decms, LOOPALIVE,
  TXDAUD fuera). Conservado: cap 250, ODCAP, bootseed sin radios.
  **Pregunta: ¿el culpable es C++ o el lado JS/datos?**
  → **RESULTADO: CARGA OK (×2, usuario). El culpable está en C1-C6.**
  ODCAP en la sesión D1: evict funcionando (388 ficheros/125 MB), live
  222-247 MB estable bajo el cap, idb 860 hits — la dieta RAM es sana.
- **Cierre por eliminación:** D1 == loaddrain en C++ y carga bien; ram4 ==
  loaddrain + C1-C6 y crashea. Culpable ∈ {C1..C6}; sospechoso principal C6
  (único con control flow nuevo dentro del bucle de streaming, que el init
  usa y que convive con suspensiones Asyncify). C1 queda fuera PERMANENTEMENTE
  (D1 lo valida: era ruido de diagnóstico).
- El plan continúa en `fluides-v2.md` (re-implementación correcta).

## 1. Hechos probados (evidencia, no teoría)

| # | Hecho | Fuente |
|---|---|---|
| H1 | El crash es OOB en la región `CPed::Initialise → SetAnimOffsetForEnterOrExitVehicle`, durante `InitialiseStep`, SIEMPRE en un **rewind de Asyncify** (doRewind en la pila) | stacks del usuario ram1/ram3/ram4 |
| H2 | Pasa en el flujo A1 troceado (ram1-3) Y en el flujo monolítico revertido (ram4) → **la causa NO es el A1** | stacks |
| H3 | ram3 se colgó en el case 8 ("Load animations") sin fetch de red alguno (access log limpio); ram4 murió en el mismo case | odtrace ram3/ram4 |
| H4 | El último build bueno es `loaddrain` (10:39): 13 min jugando, cargas OK | sesión 17:39 |
| H5 | Los datos están limpios: bootseed == streamed == vanilla (hashes, 1924 ficheros; ped.ifp/main.scm/gta_vc.dat/default.dat vanilla) | verificación por hash |
| H6 | ram2 (mismo código C++ que ram4, init en AfterInner) NO crasheó en el init pero dejó el streaming muerto (`sm=0`, `bInsideLoadAll` pegado) | odtrace ram2 |
| H7 | En ram2 el case 8 se ejecutó 3 veces (re-entrada durante suspensiones) — en ram4 no hay re-entrada visible | odtrace |

## 2. Diff real entre loaddrain (bueno) y ram4 (roto)

Cambios de C++ aún presentes en ram4:
- **C6** Presupuesto 7 ms + `emscripten_get_now()` dentro del bucle de
  `LoadAllRequestedModels` + acumulador `gWebStreamMs` (Streaming.cpp) —
  **único cambio que altera control flow por donde el init pasa**.
- C1 TXDAUD fuera (TxdStore.cpp) — solo lectura eliminada.
- C2 `RsTimer` en `webload: total=` (Game.cpp) — medición.
- C3 FPSLOG `heap=`/`sm=` (main.cpp) — medición.
- C4 `decms` con reloj de pared (sampman_oal.cpp) — medición.
- C5 LOOPALIVE 10 s (AudioManager.cpp) — cadencia de traza.

Cambios de JS/datos:
- J8 CAP MEMFS 400→250 (ondemand.js). J9 interval ODCAP (ondemand.js).
- D10 bootseed regenerado SIN radios (data — verificado limpio, H5).

## 3. Estrategia: bisect por construcción, cada build responde una pregunta

### Build D1 — "¿Es el C++ o el lado JS/datos?"
ram4 **menos C1-C6** (C++ idéntico a loaddrain) **conservando** J8+J9+D10.
- Si carga OK → el culpable está entre C1-C6 → ir a D2.
- Si crashea → el culpable es J8/J9/D10 → ir a D3.
Coste: 1 build (solo recompilar C++; bootseed/.data no cambian).

### Build D2 (condicional) — "¿Cuál de C1-C6?"
Reaplicar de una en una las que NO tocan streaming primero (C1, C2+C3, C4,
C5 — medición pura) en un solo build; si carga OK, el culpable es C6 por
eliminación. Si crashea, bisect binario de las de medición (improbable).
Coste: 1-3 builds.

### Build D3 (condicional) — "¿Es el bootseed sin radios o el JS?"
loaddrain bootseed CON radios (solo re-staging, sin tocar C++) + J8/J9.
- OK → D10 (la dieta del bootseed) es el detonante (¿qué file falta?) →
  trazar los fopens del init para ver cuál.
- Crashea → J8/J9 (cap/ODCAP) → probar CAP 400 (J8 revertido).
Coste: 1-2 builds.

### Trazas de certezza (solo si el bisect deja ambigüedad, en UNA build)
- T1: en el `__wrap_fopen`/ensure, trazar `ENSURE <ruta>` ANTES de suspender
  durante el init (saber QUÉ fichero suspende en case 8).
- T2: en `SetAnimOffsetForEnterOrExitVehicle`, trazar una vez el índice de
  grupo de anims + puntero de bloque + límites al entrar (saber QUÉ puntero
  es basura y de dónde viene).
- T3: contador de rewinds (`Asyncify` handleSleep hook) para saber cuántas
  suspensiones hubo antes del crash.

## 4. Lo que NO se toca hasta tener causa probada
- Nada de reverts adicionales, ni del presupuesto, ni del A2, ni del A1.
- El plan fluides-ram queda congelado tal cual está en ram4.

## 5. Cierre del diagnóstico
- Culpable identificado con build que lo aísla (carga OK sin él, crashea con
  él) + explicación mecánica del OOB.
- ENTONCES: fix dirigido del culpable, validación del usuario, y recién
  ahí retomar lo pendiente (A1 si procede, presupuesto si resultó inocente).

## 6. Qué necesito del supervisor
- Visto bueno al orden D1 → D2/D3 (o reordenar).
- Cada build D*: una carga de partida del usuario (1 min) — o decir si
  prefieres que intente automatizarla headless fabricando un save en el
  perfil de pruebas.

## 7. CIERRE (2026-09-18, noche) — el culpable volvió y quedó cerrado del todo

La build `lag2` reintrodujo el MISMO mecanismo (C6: presupuesto por TIEMPO en
`LoadAllRequestedModels`, 3 ms en juego / 10 ms en carga) al intentar repartir
las texturas, y volvió a crashear al cargar partida con la pila idéntica
(0 líneas `FPHASE`; log del jugador guardado en
/tmp/odtrace-sesion-crash-slot0.log).

- Mecanismo confirmado leyendo la pila: `CPed::SetAnimOffsetForEnterOrExitVehicle`
  (`src/peds/PedAI.cpp`) es un **punto de sincronía**: pide los 5 bloques de
  anim de vehículos, llama `LoadAllRequestedModels(false)` y USA las jerarquías
  en la misma función. El corte deja las tablas sin inicializar → OOB.
- Arreglado en `2026-09-18-fixload1`: tope por tiempo ELIMINADO (R1 reforzada en
  `Streaming.h`) + punto de sincronía blindado (carga sin tope y traza
  `ODANIMFAIL` si algo faltara, con offsets por defecto en vez de crashear).
- **D2/D3 y T1-T3 quedan CERRADOS**: el culpable era C6 y ya no existe.
- **Regresión automatizada** (ya no hace falta el jugador como conejillo):
  `gta_vc_browser/tools/slot0-load-test.mjs` + `tools/extract-save-from-profile.mjs`
  (saca el save del IndexedDB del navegador). Validado: FAIL con `lag2`
  (misma pila) y PASS con `fixload1`.
- Regla derivada, escrita y aceptada: **en un punto de sincronía no puede haber
  ningún tope**. La lista de sitios que "asumen cargado" tras
  `LoadAllRequestedModels` (init casos 7-8, `Script*.cpp`, `ColStore`,
  `CutsceneMgr`) queda pendiente de auditoría en otra ronda; el canario
  (anims de vehículo) ya está blindado y cubierto por el test.
