---
name: 026-one-simulation-update-per-presented-frame
status: accepted
date: 2026-11-02
domain: gameplay-timing
amends: [024-fps-is-not-a-gameplay-parameter]
---

# ADR-026: One simulation update per presented browser frame

## Context

ADR-024 clause 1 paces the web loop with a time-debt limiter against
`gWebFpsCap` (30 with the menu frame limiter, 0 with vsync, 120 otherwise) and
assumes one simulation update per *rendered* frame. Measured against the
player's session `gta_vc_browser/logs/odtrace-2026-11-02_17-55-33.log`
(build `2026-11-02-41.0-T97`), that assumption does not hold:

| Measure, dense 30 s gameplay window at 18:04:45 | Value |
| --- | --- |
| `EmscriptenTick` invocations (`WEBFT n=120 avg=6.2`) | ~161/s |
| Full simulation + render passes (`FPSLOG avg=`) | ~120/s |
| Presented browser frames (independent rAF probe, `FPSLOG frames=60 med=17 over17=0`) | **58.8/s** |
| `WEBLOOP` | `vsync=0 menu=0 cap=120 limit=0 wait=0` |

The display composites at 58.8 Hz while the engine renders ~120 times per
second, so roughly half of all simulation and GPU work is never presented, and
which simulated state lands on a presented frame depends on scheduling. That is
the mechanism behind the player's two reported symptoms: the on-screen counter
reads ~120 fps while motion looks laggy, and the main thread has no headroom
(`eng` mean 5.57 ms x 120 passes = 668 ms/s plus 41 non-rendering invocations),
so fps oscillate.

The extra invocations are not rAF-driven. With Asyncify, `emscripten_sleep`
pauses the Emscripten main loop and `MainLoop.resume()` re-enters the tick
directly from a `setTimeout`, so every on-demand file await produces additional
ticks that the display never asked for.

ADR-024's stated premise for rejecting ADR-023 — "a 30 Hz simulation is a
30 fps game with a 120 Hz picture" — is factually wrong on this machine: the
picture is 58.8 Hz.

## Decision

1. **Presented-frame gate** (`src/skel/glfw/glfw.cpp`, `case GS_PLAYING_GAME:`
   in `InnerFrame`). A rAF counter maintained by the pre-js
   (`window.__vcRaf.n`, `gta_vc_browser/web/ondemand.js`) is read once per tick
   through `EM_ASM_INT`. `WebRunTick()` runs only when the counter has advanced
   since the previous tick. Because each tick consumes one distinct counter
   value and the counter increments exactly once per browser frame, engine
   passes per second can never exceed the real refresh rate.
2. **Ceiling preserved.** The existing time-debt limiter against `gWebFpsCap`
   still applies on top of the gate, so the effective rate is
   `min(refresh, 120)`; the frame limiter (30) and vsync (0 = display decides)
   semantics of ADR-024 are unchanged.
3. **Blocking loads are exempt.** The gate reads `-1` when
   `window.__vcODBlock > 0`, so `LoadAllRequestedModels`, cutscene preparation
   and audio preload keep running at full invocation rate and loading speed is
   not halved. This addresses the per-tick streaming budget concern raised
   against ADR-023.
4. **Watchdog.** If no presented frame has been seen for more than 100 ms, the
   gate opens unconditionally. The engine can never freeze because of this
   change, even if the browser stops firing `requestAnimationFrame`.
5. **Fallback.** If `window.__vcRaf` is absent (page served without the
   current pre-js), the counter reads `-1` and the loop behaves exactly as it
   did before this ADR.
6. **Scope.** The gate applies only to `GS_PLAYING_GAME`. Menus, the front end
   and state transitions keep the previous behaviour.
7. **Verification field.** `WEBFT` gains `raf=` (presented frames consumed in
   the window) and `skip=` (invocations the gate rejected), so the ratio is
   readable from the player's own log without new trace lines.

ADR-024 remains accepted for everything else: fps is not a gameplay parameter,
the ~170 `CTimer::GetFrameCounter()` cadences stay on
`CTimer::GetLogicalFrameCounter()` (30 Hz logical clock), and the variable
timestep is unchanged. ADR-023 stays superseded: this is not a fixed-rate
simulation, it is a phase lock to whatever the display actually does.

## Consequences

- Engine passes drop from ~120/s to the refresh rate (~59/s on the measured
  machine): about half the CPU and GPU work per second disappears, and the
  on-screen fps counter stops over-reporting relative to what is presented.
- Motion becomes phase-locked, which is the intended fix for the "counter says
  120 but it feels laggy" symptom.
- On a 120 Hz or 165 Hz display the behaviour is unchanged in spirit: 120 Hz
  passes the gate every frame, 165 Hz is limited to 120 by the debt cap.
- Deferred on-demand requests (`OD.wkFastFail`) now retry once per presented
  frame instead of once per invocation, so retry latency roughly doubles at
  60 Hz. The freed ~10 ms of CPU per frame is expected to more than compensate;
  watch `ODWK aplazados=` and `FPSLOG strm=` in the next player session.
- `window.__vcRaf` becomes part of the contract between the pre-js and the
  engine. Removing or renaming it silently degrades to the pre-ADR-026 pace
  (safe, but the fix stops working); `WEBFT raf=0 skip=0` is the tell.
- Per the project's standing convention, `accepted` here means "decision taken
  and implemented", not "verified in the game". No build was produced for this
  change and only the player's session can confirm the felt result.
