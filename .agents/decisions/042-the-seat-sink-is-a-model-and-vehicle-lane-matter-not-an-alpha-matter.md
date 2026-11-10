---
name: 042-the-seat-sink-is-a-model-and-vehicle-lane-matter-not-an-alpha-matter
status: accepted
date: 2026-11-03
domain: render
supersedes: [041]
---

# ADR-042: the seat sink is a model/vehicle-lane matter, not an alpha matter

## Context

ADR-041 claimed a seated ped was swallowed by its vehicle because a fading-in alpha routed it
through `RenderPedCB` (the alpha pass) instead of the vehicle's own draw order, and T108 shipped
a guard that pins a seated ped to alpha 255 (`src/peds/Ped.cpp:2097-2114`), instrumented by
`SEATGHOST`.

The player's T109 session (82 min, `2026-11-04T00:30:05Z -> 01:52:22Z`) reports the seat as
fixed. That session contains **0 `SEATGHOST` rows**, and the guard increments its counter inside
the same condition it guards (`bInVehicle && m_pMyVehicle != nil && !bFadeOut && alpha < 255`),
before the assignment. Zero rows therefore proves the guard never ran, so it cannot be what
fixed the seat.

Why it never runs: `ProcessControl` reads the live value back with
`int alpha = CVisibilityPlugins::GetClumpAlpha(GetClump())` (`src/peds/Ped.cpp:2083`) instead of
keeping a per-ped field, so a ped that is already seated has normally been driven to 255 by the
ramp (or by the distance-fade plugins) before this block can ever see it below 255.

What plausibly did fix it is the vehicles lane in the same period: T109/T110 rebuilt 18 served
vehicle DFFs from the clean base, and `SILLACAM` had already shown at T107 that the anchor, not
the render path, was where NPC and player differ per state. That is consistent with the standing
doctrine that the mod's own model wins wherever the mod designed it.

## Decision

The alpha hypothesis is retired. The guard stays for one more session as a cheap safety net (one
comparison per seated ped per frame): if the sink returns and `SEATGHOST` stays at 0, the alpha
path is exonerated twice; if it fires, we finally have the state that trips it. Either way the
next round removes it from `src/peds/Ped.cpp:2097-2114` if it is again inert while seats look
right.

Any further seat work belongs to `src/vehicles/` and the served DFFs - the shared vehicles lane,
to be integrated rather than overwritten - not to the alpha path.

## Consequences

- ADR-041's causal claim must not be reused as a premise for new render work.
- `SEATGHOST` is the instrument that decides the guard's fate; it stays until then.
- A falsified hypothesis is recorded as falsified: the shipped guard is not credited with the
  fix.
