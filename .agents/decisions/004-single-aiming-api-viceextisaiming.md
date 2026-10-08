---
name: 004-single-aiming-api-viceextisaiming
status: accepted
date: 2026-09-27
domain: gameplay
---

# ADR-004: A single "am I aiming?" API (`ViceExtIsAiming()`)

## Context

The crouch lane and the ClassicAxis aiming lane both needed to know "is the player
aiming?", and each read it its own way: the crouch roll gates used
`GetTarget() + ViceExtCanAim`, the aim pose and the body-turn used other checks, and
the ClassicAxis port introduced its own law (`s_viceExtAimLawActive`). The result was
symptom G — cross-lane disagreement: parts of the crouch system behaved as "not
aiming" while the axis was actually aiming.

## Decision

Define **one** predicate, `CPlayerPed::ViceExtIsAiming()` (`src/peds/PlayerPed.h/.cpp`),
as the only source of truth for "am I aiming", built from `s_viceExtAimLawActive` with
`GetTarget() + ViceExtCanAim` as fallback, and false in a vehicle or dead. Every
consumer reads **only** this helper: the roll gates, the aim pose, the body-turn
heading, and the `CROUCH2 mira=` trace.

## Consequences

- The crouch and aiming lanes answer the *aiming* question the same way, which is what
  symptom G was: parts of the crouch behaving as "not aiming" while the axis was aiming.
- The axis lane can extend the aim semantics in one place (the helper) without touching
  the crouch consumers.
- Changing the meaning of "aiming" is a single-file, reviewable change.
- It settles only the *aiming* question. The *stance* side of the same defect class kept
  its own conflict and is separate work (plans 4.1-4.4), and the HUD crosshair still
  reads the aim law and the pad directly (`Hud.cpp:292`, `Hud.cpp:723`) instead of
  asking this API.

## Alternatives Considered

- **Keep per-consumer checks**: rejected — it is exactly what produced the disagreement.
- **Read the axis law directly everywhere**: rejected — couples the crouch lane to the
  axis lane's internals and its in-progress state.

## State

`accepted` (27/09). `CPlayerPed::ViceExtIsAiming()` is in `src/peds/PlayerPed.h/.cpp` and
the consumers listed above read it. Status (07/10/2026): the interface holds; the lane
that uses it has not been validated end-to-end by a player session yet — today only the
swim feature has passed that criterion (`ADR-005`).
