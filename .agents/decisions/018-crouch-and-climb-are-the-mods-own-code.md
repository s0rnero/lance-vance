---
name: 018-crouch-and-climb-are-the-mods-own-code
status: accepted
date: 2026-10-15
domain: player-controls-aim-crouch
supersedes: [011]
---

# ADR-018: Crouch, roll and climbing are the mod's own code

## Context

The old port built its own crouch (`VICEEXT_CROUCH`, ~1,300 lines in `PlayerPed.cpp`): a
private `odCrouched` flag that deliberately kept the engine's `bIsDucking` off, its own
animation group (`ASSOCGRP_PLAYERCROUCH`, with two clips grafted into the served `ped.ifp`
from a San Andreas mod), its own speeds, roll state machine, aiming pose, camera drop and a
single blend clock for crouching and standing up (`ADR-011`). Climbing (`VICEEXT_CLIMB`) was
likewise an invention: a ledge probe and a timed slide with its own group.

Plan 13.0 (`ADR-016`) transplants each feature from the mod's own source
(`x87/gta-extended`, 44f4ab3). The mod's crouch is simpler and lives inside the engine's
vanilla ducking path:

- crouching is `bIsDucking` + `bCrouchWhenShooting` (`SetDuck`/`Duck`/`ClearDuck`), with the
  mod's clips appended to `ASSOCGRP_STD` (`Crouch_Idle/Forward/Backward/Roll_L/Roll_R`);
- moving crouched is `CPlayerPed::ProcessCrouch`, called from `SetRealMoveAnim`; rolling is a
  ped state (`PED_ROLL`) with `FinishRollCB`;
- the camera height is two fields of `CCam` (`m_fCurrentCameraOffsetZ` chasing
  `m_fTargetCameraPosZ` with `InterpFloat`), written by the crouch code (0.6 still, 0.3
  moving, 0.4 aiming, 0 standing);
- climbing is `CPed::CanPedClimbingThis`/`StartClimbing`/`EndClimbing` plus the
  `PED_CLIMBING` state, the `CLIMB_*` clips in `ASSOCGRP_STD`, and it is gated by
  `features.ini` `EnableClimbing`.

## Decision

The port's crouch, roll and climbing are the mod's code, transplanted under
`VICEEXT_CROUCH` and `VICEEXT_CLIMBING` (`config.h`, Vice Extended block). The old crouch and
climb, their animation groups, IDs, constants, traces, verifier marks and harness scenarios
are removed in the same batch (T3). In particular:

- **`ADR-011` is superseded.** There is no separate crouch/stand clock any more: the body
  follows the engine's own blend deltas and the camera follows `m_fTargetCameraPosZ`,
  exactly as the mod does. `VICEEXT_CROUCH_CAM_DROP` and `ViceExtCrouchBlend()` no longer
  exist.
- The port's swimming (`PEDLANE_NADO`) keeps priority: `SetDuck` refuses while the swim lane
  owns the posture and `CanPedClimbingThis` refuses while it owns the movement. This is the
  port's equivalent of the mod's `bIsSwimming` checks, which are not transplanted because
  the port's swimming is its own.
- Until the mod's aiming arrives (T4, `VICEEXT_FIRING_AND_AIMING`), "aiming" inside
  `ProcessCrouch` (what enables the roll) is the target button held with a gun slot
  (handgun to sniper); T4 replaces it with `bIsPlayerAiming`.
- The served `ped.ifp` must be the mod's file byte for byte (the old port retouched
  `Crouch_Roll_L/R` and added two clips).

## Consequences

- One mechanism, the mod's, with the vanilla ducking flags: NPC ducking (`SetDuck` with
  `bCrouchWhenShooting`) also uses `Crouch_Idle`, as in the mod.
- New ped states shift every state from `PED_STATES_NO_ST` by +2. Saves are unaffected
  (`COMPATIBLE_SAVES` does not store ped states); numeric `estado=` fields in old traces
  change meaning.
- Climbing is off unless the served `features.ini` says `EnableClimbing = 1` (the mod's
  file ships 0).
- Parts that depend on later batches stay out until then: the aiming term and the
  first-person branches (T4), the AI crouch of cops (`DuckAndCover`, `SetMoveAnim`, row
  7.07, T7) and photo mode's checks (T9).

## Alternatives Considered

- **Keep the old crouch and graft the mod's roll and climbing onto it**: rejected — two
  mechanisms for one feature is what plan 13.0 forbids (`ADR-016`), and the old crouch
  depended on keeping `bIsDucking` off, which the mod's code needs on.
- **Keep `ADR-011`'s single clock on top of the mod's code**: rejected — the mod has no such
  clock; adding one would be port invention, not transplant.

## State

`accepted` (15/10/2026, plan `13.0-reimplementacion-limpia-extended`, batch T3). Like every
ADR here, `accepted` means "decision taken", not "verified in game": the T3 PASS is the
player's session.
