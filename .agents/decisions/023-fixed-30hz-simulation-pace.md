---
name: 023-fixed-30hz-simulation-pace
status: accepted
date: 2026-10-25
domain: gameplay-timing
supersedes: []
---

# ADR-023: The simulation runs at a fixed 30 Hz pace; render FPS decides nothing

## Context

The engine's clock is a variable timestep expressed in 1/50 s units
(`ms_fTimeStep = frameTime/1000*50`, `Timer.cpp:150`), capped at 3.0 units (60 ms,
`Timer.cpp:162`), with a documented baseline of 30 logical frames per second
(`GetDefaultTimeStep() = 50/30`, `Timer.h:66`). On top of that, 172 call sites read
`CTimer::GetFrameCounter()`, an *update* counter, and the original frame limiter (30
fps) hid every per-frame assumption behind a constant cadence.

Two measured consequences in this port:

- At 60 fps the step is 0.833 and every frame-counter cadence runs twice as fast as
  the 30 Hz the engine was written for.
- When a frame takes longer than 60 ms, the cap discards time: the game advances 60 ms
  per real 100 ms, i.e. it goes into slow motion. The player perceives the game as
  "dependent on FPS" because it literally loses time at low FPS and speeds up
  frame-based logic at high FPS.

The engine already carries the 30 Hz machinery: `GetTimeStepFix()` normalises by
50/30 (the audio uses it) and `GetLogicalFrameCounter()` accumulates elapsed time in
1/30 s intervals. The mod the port follows (Vice Extended, a reVC fork) and its
FramerateVigilante-derived fixes (`VICEEXT_FIX_FV`, ~24 sites) also assume 30 Hz.

## Decision

1. **The logical simulation advances at a fixed 30 ticks per second** — 1000/30 ms per
   tick, driven by an accumulator in the web loop, with a ceiling of **3 catch-up
   steps per rendered frame** so a stall does not spiral.
2. Render frames decide nothing: at 120 Hz the same state is drawn twice, at 30 Hz one
   tick runs per frame, at 24 Hz the accumulator runs catch-up. The monitor refresh
   rate stops being a gameplay parameter.
3. The web streaming budget is doubled while the fixed pace is active, because a
   per-tick budget at 30 ticks/s would otherwise load at half the speed of the previous
   60 Hz build.
4. Frame-counter cadences are **not** converted one by one: with a constant 30 Hz tick
   they are deterministic and match the cadence the engine was written for. Converting
   them would change the game on purpose, not remove FPS dependence.
5. The old 30/60 limiter stays reachable in the `#else` branch of
   `VICEEXT_FIXED_PACE`; disabling the define restores it.

## Consequences

- The same save behaves the same at 30, 60 and 120 Hz of render. That is the property
  the player asked for, and it is measurable from the `WEBFT` trace (`n` ≈ 30 per
  window, `avg` ≈ 33.3 ms).
- The timestep is still measured from the real clock, so jitter of ±1 frame remains and
  the 60 ms cap can still eat time in a bad spike. Closing that requires the second
  phase: `CTimer::AdvanceFixedStep()` with an exact 50/30 step and the 13 manual
  `CTimer::Update()` pump calls (cutscenes, menus, blocking scripts) made clock-neutral.
- Loading a level, restarting and the loading-screen slices advance 30 times per second
  instead of 60: the budget is doubled, but a slower machine may still take longer.
- Anything tuned by ear or eye at 60 fps in this port (wheel damping, splash rates, HUD
  blink) returns to its 30 Hz cadence, which is the original design but not what the
  last builds did.
- Physics keeps its variable-step scaling; no handling constant changes in this phase.

## Alternatives Considered

- **Keep the 30/60 limiter from the menu**: rejected — it is exactly the mechanism that
  makes behaviour depend on FPS, and it cannot hold 30 under load.
- **Convert all 172 frame counters to elapsed time**: rejected for this phase — it is a
  behaviour change per site, needs a verdict per feature (ADR-005), and with a constant
  tick the cadence is already deterministic. It stays as a documented list for the cases
  that genuinely meant seconds (traffic timer, streaming cooldown, lightning duration).
- **Fixed step at 60 Hz**: rejected — every per-frame constant in the engine and in the
  transplanted mod code is tuned for 30; 60 would double animation blend speed and
  halve every frame counter's period.
- **A fixed step without a catch-up ceiling**: rejected — a single 300 ms stall would
  queue ten ticks, causing a second stall (spiral of death).

## State

`accepted` (decided by the player on 2026-10-25, plan `21.0-paso-fijo-30hz`).
`VICEEXT_FIXED_PACE` is on in `src/core/config.h`; the loop accumulator lives in
`src/skel/glfw/glfw.cpp`. Gameplay verdict pending the player's session.
