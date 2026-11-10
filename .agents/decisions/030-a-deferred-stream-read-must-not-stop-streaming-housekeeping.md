---
name: 030-a-deferred-stream-read-must-not-stop-streaming-housekeeping
status: accepted
date: 2026-11-02
domain: on-demand-data
supersedes: [028-routine-stream-reads-may-defer-to-the-loader-worker]
---

# ADR-030: A deferred stream read must not stop streaming housekeeping

## Context

ADR-028 removed the `odBlockingPush()` / `odBlockingPop()` marker from
`CStreaming::RequestModelStream`, so a routine in-game stream read may now defer
to the loader worker. The player confirmed the result in a full session: stable
60 fps and visibly more fluid. The measurements agree. Build
`2026-11-02-41.0-T99`, log `gta_vc_browser/web/odtrace.log`, 22:43:37 to
22:52:32, 31270 frames, compared against build `2026-11-02-41.0-T97` in
`gta_vc_browser/logs/odtrace-2026-11-02_17-55-33.log`:

| Measure | T97 (blocking) | T99 (defers) |
| --- | --- | --- |
| `FPSLOG strm` ms/s | mean 30.45, p95 121, max 876 | mean **3.88**, p95 **15**, max **90** |
| `FPHASE wait` (worst frame per second) | max **1019.30 ms** | max **34.00 ms** |
| `WEBFT avg` | mean 9.72 | mean 7.26 |
| `ODWK aplazados` | 323 | 1645 |

ADR-028 also predicted, correctly, that the channel-error path would become
routine. What it did not predict is how often, nor what the early return costs.
Measured in T99:

| Measure | T97 | T99 |
| --- | --- | --- |
| `ODSHORT open-fail` | **0** | **1280** (all `errno=44`, all `fallo=1`) |
| `ODSRECOVER` | 0 | 12 |
| `CARFAIL` | 0 | 40 — this is the **cap** at `Streaming.cpp:2495`, not a count |
| `CDSKIP web` | 0 | **0** |

`CDSKIP web` staying at zero means every deferred read converged: `numTries`
never reached 3 and no model was lost. The 1280 failures are churn, not data
loss. But each one sets `ms_channelError` in `ProcessLoadingChannel`
(`Streaming.cpp:2489`), and `CStreaming::Update` answered that with an
unconditional `return`, so it skipped the rest of the frame's streaming work.
A retry needs at least two frames — one to issue the blocking read, one to
process it — so roughly 2560 of 31270 frames, **8.2%**, ran no housekeeping.

ADR-028 clause "Consequences" listed what the early return stops
(`AddModelsToRequestList`, `StreamVehiclesAndPeds`, `StreamZoneModels`,
`CColStore::LoadCollision`, `LoadRequestedModels`) and argued that this is not
new dead time because the same interval used to be an Asyncify suspension. That
argument covers the *requesting* calls but it omits two that are not on the
read path at all and that only run at the end of `Update`:

- `DeleteFarAwayRwObjects(TheCamera.GetPosition())` — `Streaming.cpp:345`
- the `RemoveModel` LRU release loop — `Streaming.cpp:400-404`

Skipping those 2560 times does not delay a load, it delays *unloading*. The same
session shows `WEBHB tex=` climbing 223 to 7277 with only 2 decreases across 608
samples, so release was already the weak side of the budget.

## Decision

1. **The deferral stays exactly as ADR-028 clause 1 defined it.** The marker is
   still absent from `RequestModelStream` and still present in `RetryLoadFile`
   (`Streaming.cpp:2591`), `LoadAllRequestedModels` (`:2784`) and the
   mission-audio preload. ADR-028 clauses 2 and 3 are carried over unchanged.
2. **A deferred read resolves inside the same `RetryLoadFile` call.** On web,
   `CdStreamRead` is synchronous — the emscripten branch of
   `CdStreamPosix.cpp:496` runs `OdDoRead` inline and clears `nSectorsToRead`
   and `bReading`. There is therefore nothing to wait for after the blocking
   re-read, so `RetryLoadFile`'s `CHANNELSTATE_IDLE` case now calls
   `ProcessLoadingChannel(ch)` immediately and clears `ms_channelError` on
   success (`Streaming.cpp:2599-2604`). This collapses the retry from two
   frames to one.
3. **`CStreaming::Update` only bails out if the error genuinely persists**
   (`Streaming.cpp:332-336`). The `return` is now guarded by a second
   `ms_channelError != -1` test after `RetryLoadFile`. When the retry succeeds —
   which is every time the measurement covers — the frame continues into
   `DeleteFarAwayRwObjects`, `LoadRequestedModels` and the `RemoveModel` loop.
4. **The bail-out is kept for real failures.** If `ProcessLoadingChannel` returns
   false, `ms_channelError` stays set and `Update` returns early as before. The
   safety property ADR-028 relied on is preserved; only the routine case changes.
5. **`LoadRequestedModels` needs no change.** It already guards its own request
   path with `if(ms_channelError == -1)` (`Streaming.cpp:2629`) and only touches
   a channel in `CHANNELSTATE_READING` or `CHANNELSTATE_STARTED` (`:2625-2627`),
   so running it after a resolved retry cannot clobber a live read. This is what
   makes clause 3 safe rather than merely optimistic.

## Consequences

- Housekeeping runs on every presented frame again, so `DeleteFarAwayRwObjects`
  and the LRU release loop are no longer skipped ~2560 times per session. This
  removes the one measured way in which the ADR-028 deferral could have
  aggravated texture and model retention.
- A deferred batch now costs one frame of `Update` instead of two, so streamed
  models appear one presented frame sooner than under ADR-028.
- The 1280 `ODSHORT open-fail` lines and their `CARFAIL` companions remain. They
  are the observable trace of the deferral working, and ADR-029 bounds what may
  be done about them: `CARFAIL` is already capped at 40 per session.
- Reading a future log changes slightly. Under ADR-028 a `CARFAIL` line implied
  two frames without housekeeping; it now implies none. `CARFAIL` still means
  "deferred", not "failed", on web.
- Verification: compiled via `bash gta_vc_browser/build.sh`. No gameplay was
  verified in a browser — the project forbids the agent from doing that — so
  `accepted` here means "decision taken, implemented and compiled", not
  "confirmed in the game". The measures to watch in the next player session are
  `ODSHORT open-fail` (should stay near 1280, proving the deferral survived),
  `CDSKIP web` (must stay at 0) and `WEBHB tex=` descent count (should rise
  above 2 per session if clause 3 is doing its job).

## Alternatives Considered

- **Report a deferred read as `STREAM_WAITING` instead of `STREAM_ERROR`**, so
  `ms_channelError` is never set: rejected, for the reason ADR-028 already
  recorded. `CdStreamPosix.cpp:497` forces `nSectorsToRead = 0` and
  `bReading = false`, and nothing re-issues the read, so a faked
  `STREAM_WAITING` is reported forever and the models stay stuck in
  `STREAMSTATE_READING`.
- **Move the `ms_channelError` block to the end of `Update`**: rejected. It
  changes the order in which `RequestModelStream` and the retry see the channel
  for every error, including genuine ones, and buys nothing over clause 3.
- **Re-queue the deferred models with `ReRequestModel` and reset the channel to
  `CHANNELSTATE_IDLE`**: rejected. It discards the sector position and size that
  `RetryLoadFile` needs, so the batch would be re-selected from the requested
  list from scratch. More invasive and slower than re-reading what was already
  chosen.
- **Revert ADR-028 entirely**: rejected by the player. The measured fluidity gain
  is large and was confirmed by feel; only its side effect needed fixing.

## State

`accepted` (2026-11-02, player's instruction after the T99 session). Supersedes
ADR-028: its clause 1, 2 and 3 are carried over verbatim by clause 1 above, and
its clause 4 ("the error handling is unchanged") plus the "Consequences"
paragraph about `CStreaming::Update` returning early are the parts this ADR
replaces. Implementation: `src/core/Streaming.cpp:332-336` and `:2599-2604`.
Gameplay verdict pending the player's next session.
