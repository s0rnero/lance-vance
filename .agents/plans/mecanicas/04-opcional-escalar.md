---
name: 04-opcional-escalar
status: ENRICHED
type: feature
domain: gameplay-climb
owner_rules: .agents
created: 2026-09-19
---

# Bloque opcional — Escalar (v1.0: "Climbing")

**Ojo: en el `features.ini` del mod, `EnableClimbing = 0`.** Es decir, la
instalación que estamos replicando **no lo tiene encendido**: este bloque solo
tiene sentido si el jugador decide activarlo. Por eso no tiene dueño asignado y
va al final, cuando los demás bloques estén cerrados.

Changelog: "Climbing. Climbing over fences and climbing obstacles (including for
NPCs)" — SA-style.

**Lo que ya hay en el port (medido):**

- `#define WALLCLIMB_CHEAT` en `src/core/config.h` + cheat en `src/core/Pad.cpp`
  (escala peldaños "a lo Matrix": es un esqueleto, no una mecánica SA).
- Animaciones: el `ped.ifp` del mod trae los clips de escalada
  (superconjunto verificado), pero **no hay entradas en `AnimationId.h` ni
  grupo** para ellos → primer trabajo es mapearlos.
- Estados: `CPed::SetPedState`, `ANIM_STD_*`, y `PedFight` tiene el patrón de
  `SetFall`/`SetJump` para movimientos con física.
- Referencia de especificación (no código pegable): SA `gta-reversed`
  (`CPed::ProcessControl` → clímax de escalada, `TASK_SIMPLE_CLIMB`), o el mod
  original `Climbing [reVC]` del mismo autor.

**Contrato mínimo si se hace (`#define VICEEXT_CLIMB`):**

1. Detectar obstáculo al frente (caja baja delante, altura < ~1,2 m) con
   `CWorld::ProcessLineOfSight`/`ProcessVerticalLine`.
2. Clip de subida + desplazamiento controlado (sin física libre) y aterrizaje en
   el techo detectado (`CWorld::FindGroundZFor3DCoord`).
3. Ni peds ni jugador se quedan pegados: timeout y limpieza de estado.
4. Que no rompa el salto, el *duck* ni el apuntado (regresión con `slot0` +
   capturas).

**PASS:** subir una valla concreta del save de prueba (posición fija, medida) con
captura antes/después, y `slot0-load-test.mjs` en PASS.
