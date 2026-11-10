---
name: 055-swim-boundary-measured-at-the-feet-and-cutscene-fov-switches-while-black
status: proposed
date: 2026-11-08
domain: reVC-WASM-nado-camara
supersedes: []
---

# ADR-055: the water boundary is measured at the feet, and the cutscene FOV only changes on a black screen

## Context

Two player reports from 2026-11-07 share one root cause: a value was read from the
wrong place.

Entering the water compared the surface against the ped **centre**
(`nivel - z > 0,20` in `PlayerPed.cpp`, `z` = centre because `FEET_OFFSET` is
`1,04`), so the swim state started when the water was already **1,25 m above the
feet** and the fall pose survived more than a metre under water. Leaving the water
used the same single threshold with no notion of ground, so a 0,34 m tide swing made
the ped alternate `enter`/`exit` every 4,06 s (21 cycles in one session); on the way
out it played the spread-eagle fall pose the player calls *pose de bandera*.

For the cinematics, 49.2-T29 replaced the camera FOV with `ConvertFOV(DefaultFOV)`
(`VICEEXT_FOV_SINGLE`), publishing 82,4–86,1 where the original publishes the flyby
value as-is (54,4). The player wants the original FOV back but must never *see* the
change, because the value is published every frame and the transition is instant.

## Decision

- The wading depth is `nivel - (z - FEET_OFFSET)`, i.e. water over the **feet**, and
  it is the only measurement used by the swim gate.
- The ped only swims when it is not standing on ground: `CWorld::FindGroundZFor3DCoord`
  at the feet, support if the ground is within `OD_SWIM_FOOT_M` (0,30 m). Standing
  with less than `OD_SWIM_WADE_M` (1,60 m) of water over the feet means walking, and
  the swim lane is never claimed.
- Entry needs `odCalado > OD_SWIM_ENTER_M` (0,30); staying needs
  `odCalado > OD_SWIM_EXIT_M` (0,10). While already swimming the wading limit drops by
  the entry margin, so a 0,34 m wave cannot flip the state back and forth.
- On the entry frame the seven fall/salto clips are removed with `blendAmount = 0`
  and `blendDelta = -1000`, and the stroke association is forced to weight 1,0 with
  `blendDelta = 0`: no frame shows the fall pose inside the water.
- `VICEEXT_FOV_SINGLE` is retired. The cutscene branch publishes the camera FOV raw.
  The published value only changes between cutscene and gameplay when
  `CDraw::FadeValue >= 200` or after 24 `SetFOV` calls; entering a cutscene applies
  immediately, and `CCutsceneMgr::FinishCutscene` forces `FadeValue = 255` and sets
  `CDraw::SetCutsceneFOVDone(true)` so the return to the game FOV happens on black
  and publishes `ConvertFOV(DefaultFOV)` while the camera is still the flyby.

## Consequences

- Wading is a valid state: shallows, river beds and the canal no longer put the player
  in the swim lane, and the flag pose cannot be reached from a tide swing.
- The FOV changes exist but are hidden behind the fade; a cutscene that starts or ends
  without any fade falls back to 24 `SetFOV` calls, which at 60 fps is under half a
  second and is the only remaining path where a change could be perceived.
- `Tread` water and slope edges sit on a 0,30 m band: crossing it still switches state,
  but only once per crossing, not per wave.

## Alternatives Considered

- Keep the single 0,20 m threshold and add a timer/debounce: hides the oscillation
  without fixing that the ped swims while standing in 0,4 m of water.
- Ramp the FOV over 250–300 ms (49.2-T27): a ramp is a visible zoom, which is exactly
  what the player rejected.
- Publish a scaled cutscene FOV (`ConvertFOV(54,4)` ≈ 68,5): closer to the old engine
  look but not the original value the player asked for.

## State

`proposed` -> `accepted` (when confirmed) / `superseded` (only through a new ADR replacing it)
