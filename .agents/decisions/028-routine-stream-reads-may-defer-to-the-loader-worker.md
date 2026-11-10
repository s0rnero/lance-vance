---
name: 028-routine-stream-reads-may-defer-to-the-loader-worker
status: accepted
date: 2026-11-02
domain: on-demand-data
amends: [026-one-simulation-update-per-presented-frame]
---

# ADR-028: Routine in-game stream reads may defer to the loader worker

## Context

`CStreaming::RequestModelStream` wrapped its `CdStreamRead` in
`odBlockingPush()` / `odBlockingPop()`. Those raise `window.__vcODBlock`
(`src/skel/ondemand.cpp:30`), and `OD.wkFastFail` returns `false` while that
counter is above zero (`gta_vc_browser/web/ondemand.js:848`). The on-demand
layer therefore could not defer anything the streamer asked for: every routine
stream read became an in-frame Asyncify suspension.

`RequestModelStream` is reached from `LoadRequestedModels`, which `CStreaming::Update`
calls every frame, so this was the per-frame path, not an occasional one.

Measured in `gta_vc_browser/logs/odtrace-2026-11-02_17-55-33.log`
(build `2026-11-02-41.0-T97`). `FPHASE` is emitted once per second and reports
the **worst** frame of that second:

| Measure | Value |
| --- | --- |
| `FPHASE wait` (worst frame per second) | median **10.20 ms**, mean 13.35, p95 19.40, max 1019.30 |
| Seconds whose worst frame waited > 8 ms | 441 of 575 |
| Seconds whose worst frame waited > 16 ms | 168 of 575 |
| Presented browser frame (`FPSLOG med` / `over17`) | 17.0 ms, `over17` median 0.5 |
| `FPSLOG strm` (time inside `LoadRequestedModels`) | mean 30 ms/s, p95 121, max 876 |
| `CARFAIL` / `CDSKIP` / `loosemiss` | **0** / **0** / 0 |

The typical second has one frame that spends ~60% of its entire 17 ms budget
suspended in an on-demand await. `strm` averaging 30 ms/s rules out model
conversion as the cause of that wait: it is the await itself. And with the
marker in place the channel-error path never ran once in a whole session.

## Decision

1. **Remove the marker from the routine path.** The `odBlockingPush()` /
   `odBlockingPop()` pair around the `CdStreamRead` in
   `CStreaming::RequestModelStream` (`src/core/Streaming.cpp`) is deleted. The
   read may now defer: `od_fetch` -> `OD.ensure` -> `wkFastFail` hands the file
   to the worker and returns `null`, `__wrap_open` fails, `OdDoRead` returns
   `STREAM_ERROR` (`src/core/CdStreamPosix.cpp:227`).
2. **Every other marker stays.** `RetryLoadFile` (`Streaming.cpp:2590`),
   `LoadAllRequestedModels` (`:2777`) and the mission-audio preload
   (`src/audio/sampman_oal.cpp:2240`) keep blocking. Keeping `RetryLoadFile`
   blocking is what bounds this change: it is the recovery path, and it
   guarantees a channel error resolves in one attempt instead of spinning.
3. **No new guards are added; the two existing ones are what make this safe.**
   - `wkFastFail` returns `false` when `window.__vcInGame !== true`, so loading
     screens, the front end and the TXD conversion screen
     (`src/rw/TexRead.cpp:473`, the only other caller of `RequestModelStream`)
     behave exactly as before.
   - `wkFastFail` returns `false` after three consecutive deferrals of the same
     file (`wkFails[canon] >= 3`, reset when the worker delivers), so a
     deferral cannot loop indefinitely: the fourth attempt waits.
4. **The error handling is unchanged.** On web, `RetryLoadFile` with
   `numTries >= 3` only emits `CDSKIP web: aplazamiento on-demand, no hay disco`.
   The `CHud::SetMessage(TheText.Get("CDERROR"))` plus
   `CTimer::SetCodePause(true)` branch sits in the `#else` and does not exist in
   the WASM build. An earlier reading of this code assumed the web build could
   show a CD error and pause on a deferral; it cannot.

Expected flow per streamed batch, on a 17 ms presented frame:

| Frame | What happens |
| --- | --- |
| N | `RequestModelStream` -> `CdStreamRead` defers; the worker starts fetching; `nStatus = STREAM_ERROR` |
| N+1 | `ProcessLoadingChannel` reads the error -> `CHANNELSTATE_ERROR`, `ms_channelError = ch`, one `CARFAIL` line (capped at 40 per session) |
| N+2 | `Update` takes the `ms_channelError` early return -> `RetryLoadFile` -> blocking read. The worker has had ~2 presented frames, so the file is usually already in `wkReady` and the wait is zero |

## Consequences

- The network round trip moves off the game frame and onto the worker. The
  intended effect is a lower `FPHASE wait=` median and fewer seconds with a
  worst frame over 17 ms. ADR-026's `WEBFT raf=` / `skip=` should also improve,
  because fewer ticks are spent suspended and more presented frames get a
  simulation update.
- `CStreaming::Update` returns early while `ms_channelError != -1`
  (`Streaming.cpp:332`), so `AddModelsToRequestList`, `StreamVehiclesAndPeds`,
  `StreamZoneModels`, `CColStore::LoadCollision` and `LoadRequestedModels` all
  stop for the ~2 frames a deferral takes. Today that same interval is a hard
  Asyncify suspension, so this is not new dead time; it is dead time that now
  lets the browser keep presenting frames.
- The channel-error path becomes routine instead of never-used. `CARFAIL`
  (capped at 40) and `ODSHORT open-fail` (first 60, then every 20th) will appear
  in normal play. On web `CARFAIL` means "deferred", not "failed"; reading a
  future log requires knowing that, and the caps keep it out of the frame budget.
- If the worker is saturated the change degrades to today's behaviour: the third
  deferral falls through to a blocking wait. In the measured session `esperando`
  peaked at 36 MB against `wkReadyCap` of 40 MB, so saturation is plausible and
  the worst case is unchanged rather than worse.
- Watch in the next player session: `FPHASE wait=` (should fall from ~10 ms
  median), `ODWK aplazados=` (should rise well above the measured 323),
  `CARFAIL` / `CDSKIP` counts, and whether vehicles and peds still appear. A
  `CDSKIP web` line repeating every frame means `numTries` is climbing without
  converging, and this decision has to be reverted.
- Verification: compiled as build `2026-11-02-41.0-T98` via
  `bash gta_vc_browser/build.sh`. No gameplay was verified in a browser — the
  project forbids the agent from doing that — so `accepted` here means "decision
  taken, implemented and compiled", not "confirmed in the game".

## Alternatives Considered

- **Also remove the marker in `RetryLoadFile`**: rejected. It would extend the
  streaming halt from ~2 frames to the three deferrals `wkFails` allows (~6
  frames), and once `numTries >= 3` the `CDSKIP web` trace would fire every
  frame. The extra head start does not pay for a per-frame trace plus a longer
  halt in `CStreaming::Update`.
- **Make a deferred read report `STREAM_WAITING` instead of `STREAM_ERROR`**, so
  the channel never enters the error state: rejected. The emscripten branch of
  `CdStreamRead` forces `nSectorsToRead = 0` and `bReading = false`
  (`CdStreamPosix.cpp:497`), and nothing re-issues the read, so a faked
  `STREAM_WAITING` would be reported forever and the models would stay stuck in
  `STREAMSTATE_READING` — the "vehicles that never appear" failure.
- **Prefetch the next zone** so the engine never asks for a file that is not
  already in MEMFS: rejected for this change. It is the better long-term answer,
  but it is a design of its own (zone prediction, budget, eviction) and not a
  fix to a blocking marker.

## State

`accepted` (2026-11-02). Amends ADR-026 clause 3: routine in-game stream reads
no longer raise `window.__vcODBlock`, so the presented-frame gate is no longer
bypassed by them. Not superseded.
