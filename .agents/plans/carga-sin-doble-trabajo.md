---
name: carga-sin-doble-trabajo
status: EXECUTED
type: maintenance
domain: performance
owner_rules: .agents
created: 2026-09-18
---

# Plan: carga sin doble trabajo (ejecución aprobada)

Fecha: 2026-09-18. Estado: **CERRADO SIN COMPLETAR — A1 revertida en ram4.**
La vía A1 (init troceado en AfterInner) rompió Asyncify en 3 builds seguidas
(mundo vacío, streaming muerto, carga colgada + OOB en CPed::Initialise). El
flujo de carga volvió al monolítico validado (SÍ → InitialiseGame con saltos
A2 → restart troceado). Los saltos A2 SÍ quedan (reducen el doble trabajo).
Si se reabre: pre-ensure de todos los ficheros del init antes de arrancarlo
(cero suspensiones) o mover el fetch fuera del hilo del juego.
Diagnóstico original: CONFIRMADO con el log del usuario (ya no es teoría).

## Diagnóstico confirmado (citas del log)

Al dar SÍ en el confirm:
1. `GS_FRONTEND` → `GS_INIT_PLAYING_GAME` (glfw.cpp:2227) + `InitialiseGame()`
   (glfw.cpp:2237): **inicialización de partida nueva entera** — casos 0-18
   (todos los IDE/IPL), tormenta TXDIN + `Start load scene`/`End load scene`,
   `Creating player`, `case 18`, radio lens → estado 7→9.
2. Después, el restart AfterInner (wantToLoad seguía activo): shutdown de todo
   lo anterior (`pools have been cleared`, `Shutting down CPedType`...) +
   re-init + `GenericLoad OK` + `loaded pos` en la playa.
3. La escena se construye 2 veces; la primera se tira. El hilo nunca muere
   (FPS 50 en negro, ticks avanzan): es trabajo duplicado + tramos monolíticos,
   no deadlock. El ratón muerto es el menú inactivo durante el restart (normal
   con pantalla visible; anormal con el confirm clavado).

## A. No construir el mundo dos veces (el win mayor, ~50%)
- Spike primero (medir, no adivinar): añadir `initstep` a la vía prioritaria
  del flush (una línea en main.js, sin rebuild) y con UNA carga leer el reparto
  por caso (0-18 vs restart). Cita a corregir: el monstruo no es "el caso 17"
  (hoy = StartTestScript); la tormenta TXDIN sale con `Start load scene`
  pegado al caso 17 — el spike dice quién la dispara.
- Lista de dependencias del restart (lo que el restart asume ya iniciado y NO
  se puede saltar): pools, modelinfo/IDE (registro), streaming/imagen CD,
  slots TXD, `CTheScripts::Init`, cámara, paths, radar, clock.
  Solo sobran: escena en área por defecto + crear player + `StartTestScript`.
- Implementación recomendada A2 (blast radius menor): en `InitialiseStep`,
  con `m_bWantToLoad`, saltar lo sobrante manteniendo el registro.
  Alternativa A1: en `GS_FRONTEND` (glfw.cpp:2234) no llamar a
  `InitialiseGame()` con wantToLoad y dejar todo al restart.
- Regresión: partida nueva idéntica (wantToLoad=false, sin tocar), F2 (slots +
  `GenericLoad OK` + `loaded pos`), primer load tras boot sin new game previo.

## B. Trocear lo pesado DE LA RUTA DE CARGA (redirigido, no el 17)
- Verificar por código que el drenado 0b y `LoadSceneStep` respetan
  `gWebLoadBudget=10` (ambos llaman a `LoadAllRequestedModels`, confirmar que
  es la rama con tope).
- Con el spike (A), trocear los monstruos que queden en la ruta (casos 7/8 si
  A los conserva, cola de colisión, escena) con el patrón ya probado
  (presupuesto/tick + `%` en `gWebLoadFrac` + dibujado por tick).
- Nota: pantalla in-game (decisión de usuario, se mantiene) en vez de overlay
  DOM; el criterio "visible en el tick del clic + barra avanzando" se cumple igual.

## C. Multihilo: NO (aceptado, con diseño fase-2 documentado)
Asyncify (todo el I/O: `OD.ensure`, IDBFS, fetch) ⊥ pthreads en la práctica, y
WebGL sube texturas solo en el principal. Fase 2 SI hiciera falta: worker
solo-decode (segunda instancia wasm para DXT/MP3, anillo SAB para PCM/píxeles;
el principal conserva juego+GL+subida). Semanas + regresión; innecesario si
A+B cumple. Reabrir solo con FPSLOG de carga en mano.

## D. Ya codificado, pendiente de compilar (entra en la próxima build)
Cap global TXDAUD 4000, traza rueda/F9 del retune, vía prioritaria del flush
(+`initstep` cuando se añada). Instrumentación, no cambia la carga.

## Aceptación por build
Ratón vivo durante TODA la carga, pantalla visible desde el tick del SÍ con
barra avanzando, cero "esperar/salir", `GenericLoad OK` + `loaded pos`,
`loadsave.mjs` verde (con save fabricado v2 con nombre/fecha si el actual no
lista), oído en radio sin cambios.
