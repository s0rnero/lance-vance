---
name: 024-fps-is-not-a-gameplay-parameter
status: accepted
date: 2026-10-25
domain: gameplay-timing
supersedes: [023-fixed-30hz-simulation-pace]
---

# ADR-024: Render FPS is not a gameplay parameter

## Context

The engine's clock is a variable timestep in 1/50 s units
(`ms_fTimeStep = frameTime/1000*50`, `Timer.cpp:150`), capped at 3.0 units
(60 ms) and documented against a 30 Hz baseline (`GetDefaultTimeStep() =
50/30`). The port ran one simulation update per rendered frame, and the web
loop capped that at 30 or 60 fps, so two things followed:

- Every one of the ~170 `CTimer::GetFrameCounter()` cadences (`% 60`, `& 7`,
  delta against a stored frame) was tied to the render rate.
- Every constant added or decayed once per update ran faster with more fps.

ADR-023 tried to solve this by freezing the simulation at 30 Hz. The player
rejected it: with the menu frame limiter and vsync both off the game must run
at the monitor rate up to 120 fps, and a 30 Hz simulation is a 30 fps game
with a 120 Hz picture.

## Decision

1. **Pace policy** (`src/skel/glfw/glfw.cpp`, `gWebFpsCap`): frame limiter on
   → 30 fps; vsync on → the display decides (no cap of our own); both off →
   **120 fps**.
2. **One update per rendered frame with the real timestep.** No fixed-step
   accumulator: the engine's own variable step already carries time, and an
   extra accumulator would add a second clock to reconcile.
3. **Cadence lives on the 30 Hz logical clock.** `CTimer::GetLogicalFrameCounter()`
   advances by elapsed time (1 per 33.3 ms) at any render rate, so a
   "1 in 8 frames" rule keeps its authored wall-clock rate. Frame stamps
   (`m_nFrameWhenHit`, lightning start, streaming cooldown) use the same clock
   on both the writer and the reader side.
4. **Per-update magnitudes become per-second.** Additions × `GetTimeStep()`,
   multiplicative decays `Pow(base, GetTimeStep())`, integer countdowns
   `- CTimer::GetLogicalFramesPassed()` clamped to the counter.
5. **Per-update probabilities become per-time.** `CGeneral::ChancePerTick(N)`
   is exactly 1-in-N per 30 Hz tick: at 120 fps it rolls a quarter as often per
   update, so the per-second rate is unchanged.
6. **What stays on the raw frame counter** is only the identity of an update:
   `odSfxPendFr` in the on-demand SFX queue, "already emitted this update"
   dedupe guards, and `frame=%u` fields in traces.

## Consequences

- Behaviour is now a function of elapsed time, not of how many frames the GPU
  managed. The engine's 50 Hz nominal values (`GetTimeStep() == 1.0`) and the
  30 Hz reference (`GetTimeStepFix() == 1.0`) are the two anchors every
  conversion uses, so a 30 fps session behaves as it did before.
- Physics is genuinely variable-step now, at four times smaller steps at
  120 fps: integration is more accurate, but any constant still written
  per-update somewhere in the engine will show up as a 4x rate, not as a
  crash. The sweep covered vehicles, peds, weapons, audio, script, camera,
  streaming and the renderer; integer counters hidden in rarely used paths
  may remain.
- `VICEEXT_FIXED_PACE` is gone, and with it the doubled streaming budget: the
  load loop is per-frame again, so a 120 fps session loads at the page's pace
  rather than half of it.

## Alternatives Considered

- **Fixed 30 Hz simulation + render decoupled** (ADR-023): rejected by the
  player — the picture would still update 30 times per second.
- **Fixed 60 Hz simulation with render interpolation**: rejected — it needs
  interpolated transforms for every entity and a second clock, and it is a
  larger change than the effect justifies.
- **Leaving the limiter at 60**: rejected — it is the mechanism the player
  asked to remove.

## State

`accepted` (player's instruction of 2026-10-25: no frame limiter and no vsync
must mean 120 fps with no framerate-dependent behaviour). Implementation:
plan `2026-10-25-21.0-independencia-fps`. Gameplay verdict pending the
player's session at 120 fps.
