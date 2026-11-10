---
name: 036-the-npc-seat-sink-is-fixed-from-measured-hip-z-not-from-a-speculative-gate
status: accepted
date: 2026-11-03
domain: vehicles
supersedes: []
---

# ADR-036: the NPC seat sink gets measured before it gets fixed

## Context

The player reported (session after T101): "la posicion de los npcs dentro de un
vehiculo esta mal, es como que la silla del vehiculo se los traga", while the
death pose in the same car looks correctly placed. Asked to compare himself, the
answer was decisive: "yo me veo bien, solo pasa con los npcs" — the player and
an NPC in the same seat of the same model disagree, so the bug is a behaviour
difference, not a data difference.

`CPed::SetPedPositionInCar()` (`src/peds/Ped.cpp:8544`) is the single anchor that
places a seated ped: it takes the vehicle matrix, adds the rotated seat dummy
position (`vehModel->GetFrontSeatPosn()` / `m_positions[CAR_POS_BACKSEAT]`,
applied at `src/peds/Ped.cpp:8613`) and writes the result with `SetMatrix(newMat)`.
The ped matrix origin is the hip (`FEET_OFFSET = 1.04f`, `src/peds/Ped.h:15`).
Nothing in that function branches on player-vs-NPC, and the sit clip is chosen
per car (`car->bLowVehicle` in `CPed::AddInCarAnims`, `src/peds/PedAI.cpp:6308`),
also not per role. Static reading cannot produce the asymmetry the player sees,
so any code change made now would be a guess.

Two guesses were considered and rejected:

- Gate the T28/T64 per-frame re-anchor (`src/vehicles/Automobile.cpp:1856-1877`)
  on the ped being absent from the moving-entity list. `SetPedPositionInCar` is
  absolute — it recomputes the matrix from the vehicle each call, so calling it
  twice in one frame is idempotent. The gate would most likely be a no-op for the
  sink while risking the exact parked-car bug the block was added for.
- Subtract a constant hip-Z offset for NPCs, or edit the seat dummies in the
  DFFs. The measured per-model `CAR_POS_FRONTSEAT.z` spread is wide (cheetah
  -0.388, stallion -0.280, premier -0.138, boxville +0.254), so no single
  constant can be right, and the DFF lane belongs to another agent.

## Decision

T104 ships measurement only. A throttled `SILLACAM` trace was added immediately
before `SetMatrix(newMat)` (`src/peds/Ped.cpp:8637-8664`) that logs, per anchor
call: model index, `IsPlayer()`, seat slot, the hip Z before and after the
anchor plus the delta, the seat dummy Z, the vehicle Z, ped state, whether the
ped is in the moving-entity list, `bLowVehicle` and `bIsVan`. No control flow,
no matrix and no animation changed; the block is inside `#ifdef __EMSCRIPTEN__`
and follows the ADR-029 cap (first 60 calls, then every 200th).

The fix for the sink lands in the build *after* the numbers come back, and is
constrained to code: the player's own words are "se arregla en codigo sin tocar
DFFs".

## Consequences

- One extra playtest session before issue 3 is closed; in exchange the correction
  is arithmetic instead of a hunch, and it cannot silently break the parked-car
  re-anchor or the corpse pose (the only peds excluded from the re-anchor are the
  player and `DyingOrDead()`, and corpses are exactly the ones that look right).
- If the numbers show player and NPC receiving the *same* hip Z in the same seat,
  the asymmetry is downstream of the anchor (clip, bone offsets, or the render
  path) and the next investigation is there, not in the seat data.
- Trace volume is bounded the same way as the other `VICEEXT` traces;
  `gta_vc_browser/tools/viceext-log-check.py` does not know the `SILLACAM` tag,
  so the lines are read with grep until the fix ships and the trace is removed.
