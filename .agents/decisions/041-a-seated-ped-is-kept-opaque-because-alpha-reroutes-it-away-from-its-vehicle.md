---
name: 041-a-seated-ped-is-kept-opaque-because-alpha-reroutes-it-away-from-its-vehicle
status: superseded
date: 2026-11-03
domain: render
supersedes: []
---

# ADR-041: a seated ped is kept opaque, because alpha reroutes it away from its vehicle

## Context

ADR-036 deferred the seat-sink fix until `SILLACAM` measured the anchor, and promised that if
player and NPC received the same hip Z the investigation would move downstream. The T107
session (43 min, 12 636 `SILLACAM` rows) does exactly that:

- `mov=1` in all 12 636 rows (12 111 NPC, 525 player). Every seated ped is in
  `ms_listMovingEntityPtrs`, so `CWorld::Process` already runs
  `SetPedPositionInCar(); GetMatrix().UpdateRW(); UpdateRwFrame();` for NPC and player alike
  (`src/core/World.cpp:2281-2310`). The anchor is applied to the render frame every tick, for
  both roles. This also kills the alternative that was open at T107 - adding the missing
  `UpdateRwFrame()` to the re-anchor block in `src/vehicles/Automobile.cpp:1869-1876`: that
  block is redundant, and `src/vehicles/` is the shared vehicles lane.
- The per-call delta averages -0.001 (NPC driver), -0.004 (NPC front seat) and +0.001 (player).
  The NPC outliers (-0.44 to -0.58) occur only in states 59 `PED_DRAG_FROM_CAR` and 62
  `PED_EXIT_CAR`, i.e. while the ped is leaving. Position data is not the bug.

The render path is. `PEDALPHA` reported 417 rows in that session, 231 of them (55%) in
alpha 0..15 with the mode at 12-14, exactly one row at 0 and none below 6. Neither ramp
available to a ped - `+Max(1, 16*ts)` in, `-Max(1, 8*ts)` out (`src/peds/Ped.cpp:2085-2095`) -
holds a value, it sweeps through the range, so a distribution with a spike at 12 and a hole
under 6 is a *hold*, not a fade caught mid-flight. The sampling is one ped per 2 s
(`src/rw/VisibilityPlugins.cpp:941-953`), so the sample only grows if many peds sit there.

What low alpha does to an occupant is not cosmetic:

- `RenderPedCB` reads the clump alpha and, below 255, sends the atomic through
  `RenderAlphaAtomic`; `RenderPlayerCB` never reads alpha at all
  (`src/rw/VisibilityPlugins.cpp:921-961`). Tommy cannot enter this path by construction,
  which is the "yo me veo bien, solo pasa con los npcs" half-sentence.
- `CRenderer::RenderEverythingBarRoads` pulls any ped whose alpha is not 255 out of the inline
  pass and into the distance-sorted list (`src/renderer/Renderer.cpp:306-319`). Peds that stay
  inline are drawn *before* their vehicle body, deliberately, inside the vehicle's own render
  group so the torso survives the depth test against its own door
  (`src/renderer/Renderer.cpp:207-224`, "Render Peds in vehicle before vehicle itself"). An
  occupant rerouted by alpha loses that ordering, and the body it should have been drawn
  under is already on screen. The head, above the beltline with nothing in front of it, is
  the part that still reads - the reported symptom: "el npc solo se ve asomada la cabeza por
  que esta hundido, no esta bien posicionado".

## Decision

In `CPed::ProcessControl`, between the fade arithmetic and `SetClumpAlpha`
(`src/peds/Ped.cpp:2097-2114`), a ped that is seated (`bInVehicle && m_pMyVehicle`), not
fading out, and below full opacity is snapped to 255. The block emits a capped
`SEATGHOST nom=%s alfa=%d est=%d jug=%d coche=%d` trace (ADR-029 idiom) on the snap, so the
fix reports whether its own premise was true. Nothing else changes: no matrix, no clip, no
seat dummy, no DFF - the constraint the player set ("se arregla en codigo sin tocar DFFs").

## Consequences

- A seated NPC is rendered by the same callback and the same pass as the player, so the
  ordering artifact cannot be reached while a ped occupies a seat.
- The fade *out* is untouched, so `CPopulation`'s pruning of peds that reached alpha 0
  (`src/peds/Population.cpp:1114`) keeps working; the snap deliberately excludes `bFadeOut`
  because forcing a fading ped to 255 would strand it in the ped pool.
- A ped scripted to fade in while already seated pops to opaque on its first tick. Fades out
  still animate. No shipped script does the former, and the alternative is the bug.
- The trace makes the change falsifiable. If `SEATGHOST` fires and the occupants look right,
  the alpha route was the cause. If it stays silent and the sink persists, alpha is exonerated
  too, and the remaining suspect is the sit clip's blend (`AddInCarAnims` picks it only by
  `isDriver` and `bLowVehicle`, `src/peds/PedAI.cpp:6309-6340`); that is measurable by adding
  the association id, `currentTime` and blend weight to `SILLACAM`, which stays in place until
  the sink is closed.
- The corpse observation (the same NPC shows its torso once dead) is not explained by this
  mechanism and is deliberately not used to justify it: a dead ped is excluded from the
  vehicle re-anchor and keeps the position from the moment it died, which changes the pose but
  not the alpha path. It is the reason the anchor was never blamed in the first place.
- Accepted means the decision is taken, not verified in game.
