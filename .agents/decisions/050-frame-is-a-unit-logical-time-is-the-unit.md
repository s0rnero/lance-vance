---
name: 050-frame-is-a-unit-logical-time-is-the-unit
status: accepted
date: 2026-11-06
domain: gameplay-timing
supersedes: []
---

# ADR-050: "per presented frame" is a defect class, not a per-symptom bug — one canonical conversion for all of it

## Context

The engine was written for a fixed 30 Hz: one `Idle()` call per presented frame, one `CTimer::Update()`
per `Idle()` (`src/core/main.cpp:2005`), `ms_fTimeStep = frameTime/1000*50` (`src/core/Timer.cpp:150`).
On the WASM port the presented frame rate is whatever the browser composes — up to 120 Hz — so every
quantity that accumulates *once per frame without scaling* runs up to 4x faster in real time, and every
window measured *in frames* shrinks 4x. Plan 48.0 measured four of them, all reported by the player as
unrelated symptoms:

| site | frame-rate effect, measured |
|---|---|
| `CPad::UpdatePads` horn ring, 5 samples (`src/core/Pad.cpp:2512`), read by `CAutomobile::DoEngineHandling` (`src/vehicles/Automobile.cpp:1458`) and `CBike` (`:1147`) | the "short double tap" window is 5 frames = **166 ms at 30 FPS, 42 ms at 120 FPS**; a human tap is 120-200 ms, so at 120 FPS the tap always lands in the *long press* branch and the siren toggle never fires |
| parked-helicopter rotor decay `m_aWheelSpeed[1] -= 0.0005f` (`src/vehicles/Automobile.cpp:1490`) | rotor spins down **4x faster in real time** at 120 FPS |
| traffic-car alpha `clumpAlpha += 16` (`src/vehicles/Vehicle.cpp:2194`), spawn at alpha 0 (`src/control/CarCtrl.cpp:695`) | fade is 0.53 s at 30 FPS, 0.13 s at 120 FPS, and 4x more spawns/s — with the riders drawn at full alpha (`src/renderer/Renderer.cpp:207`) because the vehicle's alpha was never applied to them: "an NPC sitting in an invisible car" |
| `CClock::Update` one game-minute per frame inside an `if` (`src/core/Clock.cpp:64`), plus the 60 ms per-frame clamp (`src/core/Timer.cpp:164`) | every load hitch silently **deletes** wall time from the game clock: day/night and mission timers fall behind real time |

Plans 41.0-47.0 (SilentPatch-style and "frame-rate vigilante" work) had fixed these one symptom at a
time, which is why the player kept finding new ones: the *class* was never named, so nothing forced the
remaining sites to be looked for.

## Decision

1. **A per-frame accumulation is a bug by definition.** Conversion is canonical, and there are exactly
   three forms, chosen by what the quantity is:
   - *an increment tuned per frame at 30 Hz* → multiply by `CTimer::GetTimeStepFix()`
     (`GetTimeStep()/GetDefaultTimeStep()`: 1.0 at 30 Hz, 0.25 at 120 Hz). Chosen deliberately over
     `GetTimeStep()`: it reproduces the original 30 FPS value **bit-for-bit**, so no retuning of the
     constant is smuggled in with the fix.
   - *a window or ring sampled once per frame* → gate it on `CTimer::GetLogicalFrameCounter()`
     (`src/core/Timer.h:68`) and replay the missed samples with a bounded `while`/`for`
     (`while (ticks > HORNHISTORY_SIZE) ticks = HORNHISTORY_SIZE`), so the window keeps its
     milliseconds meaning at any FPS.
   - *a clock that must not lose real time* → accumulate against
     `CTimer::GetTimeInMillisecondsNonClipped()` (the timeline *before* the 60 ms clamp) with a carry,
     and catch up in a **bounded** loop (`OD_CLOCK_MAX_CATCHUP = 20` game-minutes) so a long pause
     changes the hour, it does not teleport it.
2. **The 60 ms clamp stays.** Without it one 2 s frame gives the physics a 2 s step. Losing a little
   clock time per hitch is the price of stable physics; the carry above is what pays it back.
3. **Each conversion gets its own `#define` gate and its tunable lives next to it** in
   `src/core/config.h` (`VICEEXT_HORN_LOGICAL_TICK`, `VICEEXT_HELI_SPIN_SCALED`,
   `VICEEXT_VEH_FADE_SCALED`, `VICEEXT_CLOCK_CATCHUP`, plus `OD_HELI_RAMP`, `OD_HELI_DECAY`,
   `OD_VEH_FADE_IN/OUT`, `OD_SEAT_GHOST_ALPHA`, `OD_CLOCK_MAX_CATCHUP`). Retuning a feel constant must
   not require recompiling half the engine, and disabling one conversion must not disable the others.
4. **Each conversion is observable.** `HORNHIST … ventana_ms=`, `RELOJ sim= real_ms= dif_ms=`, and for
   the fade the acceptance is visual (`SILLACAM`/`SEATGHOST` cover the seat side). A fix that cannot be
   measured in the served build's log is not considered done — that is how the 42 ms window survived
   unnoticed for four plans.

Forms 5–7 were added by the 49.0 codebase-wide census (06/11/2026, 650 candidates in 229 files), which
found that forms 1–3 do not cover three further shapes:

5. **A counter that indexes a curve or a choreography must not be scaled** — it must be *stepped*.
   `screendroplets.cpp` spends a drop budget read as `ndrops[ms_splashDuration]`, and the radio retune
   sequence fires on `gRetuneCounter == 19` / `== 1`. Multiplying those counters by
   `GetTimeStepFix()` would either skip samples of the curve or land on a label that has no step. The
   conversion is instead: run the body **at most once per logical tick** — gate it on
   `CTimer::GetLogicalFramesPassed() != 0`. Two sub-shapes, chosen on purpose:
   - *labelled choreography* (retune, blood decal, one-per-tick effects): one step per gate, so no
     label is ever skipped.
   - *countdown indexing a table* (splash duration): decrement by `GetLogicalFramesPassed()`, because
     exact wall-clock duration matters more than the two or three table samples skipped during a hitch
     — a hitch can make the effect lose drops, never invent them.
6. **A re-arm driven by object creation is a frame-rate symptom.** `RegisterSplash` re-armed the lens
   droplets every time a splash particle object was created, so repeated entry/exit kept the lens
   white. The gate is wall clock, not ticks: `OD_SPLASH_REARM_MS` against
   `CTimer::GetTimeInMillisecondsNonClipped()`, which also bypasses the 60 ms clamp for a stamp that
   must not stall. The same clock is the right limiter for traces (`ODLOAD`, `NEUMATICO`): a *total
   line cap* risks dropping the last line before a crash, which is exactly the line that matters.
7. **A per-frame *read* of a value whose owner already advances at 30 Hz is not a defect.** The census
   flagged `HornPattern[pattern][44 - m_nCarHornTimer]` as per-frame; the timer itself decays in
   `ReduceHornCounter` on logical ticks, so the pattern already advanced at 30 Hz. Before converting a
   flagged site, find who owns the increment — a large part of an automated census of this kind is
   this shape, plus dead `#else` branches of earlier conversions and angle wraps (`-= 6.28f`).

## Consequences

- Behaviour at 30 FPS is unchanged by construction; at 120 FPS the intended timings are restored rather
  than re-invented. Any future "it feels 4x too fast/slow" report is triaged against this class first.
- The class is open-ended: plan 48.0 fixed the four sites found by symptom. A codebase-wide audit for
  remaining per-frame logic touching physics is required as a separate plan (user instruction,
  06/11/2026), because the engine's PS2/30 Hz assumption is now a capability limit, not a contract.
- `OD_VEH_FADE_*`, `OD_HELI_RAMP` and `OD_CLOCK_MAX_CATCHUP` are now gameplay-visible knobs; a change to
  them needs no code review of the call sites, but does need a log line confirming the resulting timing.
- Occupants are hidden below alpha 32 instead of the vehicle's alpha being multiplied onto them. This
  trades a general fix (renderer would have to modulate skinned peds per-pass) for a narrower one; if
  motorcycles ever need partial fades, this is the place that will be wrong.
- The audit ran (plan 49.0) and the class is now closed **as a class**, not as a list: 12 conversions
  plus 2 instruments landed behind `VICEEXT_STICK_TIMESTEP_SCALED`, `VICEEXT_AIR_TURN_DAMPED`,
  `VICEEXT_STREAM_LOGICAL`, `VICEEXT_SUSPENSION_SCALED`, `VICEEXT_CAM_WINDOWS_LOGICAL`,
  `VICEEXT_SWIM_LOGICAL`, `VICEEXT_BLOOD_LOGICAL`, `VICEEXT_HUD_WINDOWS_LOGICAL`,
  `VICEEXT_AUDIO_WINDOWS_LOGICAL`, `VICEEXT_SPLASH_LOGICAL`, `VICEEXT_ALLOC_GUARD`,
  `VICEEXT_TYRE_TRACE`. Re-running the census afterwards still prints 38 ALTA lines and **none of them
  is a conversion target** — the tool counts dead `#else` branches, angle wraps, per-event
  coefficients, debug/editor keys and trace counters. A future plan should not treat that number as
  remaining work; the remaining work is one item, F17 (tank aiming, `Automobile.cpp:5508-5509`),
  deliberately left for the vehicles lane.
- The audit also closed two things it did not expect to: `VICEEXT_SEAT_BY_PED_HEIGHT` (a 48.0 fix in
  this same class) was **wrong in the player's eyes** — a seat that tracks ped height reads as
  discrimination, and the reference pose is the one normal-height peds already had, so the uniform
  value is `0.0f` (`OD_SEAT_LIFT_ALL`). Restoring a per-ped table would need a visual argument, not a
  measurement. And the AI horn was a census false positive (form 7).
