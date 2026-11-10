---
name: 043-the-web-streamer-throttles-on-queue-depth-only-never-on-loading-priority
status: accepted
date: 2026-11-03
domain: streaming
supersedes: []
---

# ADR-043: the web streamer throttles on queue depth only, never on loading priority

## Context

The player reports that only the lowest-quality LOD is drawn and that walking over the building
does not load it. The T109 session (82 min) shows 40 `LODLEFT` rows (the diag's own print cap),
all on `LOD*`/`lod*` big buildings, each with `relacion=1 rwobj=0 alpha=0`: the hi-res twin model
exists in the IDE and in the served data, and never received an atomic.

The data lane is exonerated - every twin and its TXD exist in `gta3.dir` and on disk with valid
DFF headers, `FAIL model` = 0, `REINTENTO` = 0, no MEMFS eviction, and the `ODSHORT open-fail`
rows are the by-design fast-fail that always recovered (`CDSKIP` = 0, `ODSRECOVER` present).

The mechanism is the vanilla disc-seek throttle, which never disengages on the web:

- `CStreaming::AddModelsToRequestList` returns out of its whole sector loop when
  `CRenderer::m_loadingPriority && ms_numModelsRequested > 5` (`src/core/Streaming.cpp:3056`).
- `CRenderer::ScanSectorList` gates its `VIS_STREAMME` request on
  `!m_loadingPriority || ms_numModelsRequested < 10` (`src/renderer/Renderer.cpp:1550`).

`CARBLOCK` sampled `prio=1` in **153/153** probes across >=45 900 `CStreaming::Update` calls
(13-26 min of an 82-min session), while `ODHAMBRE` shows the queue sitting chronically at
`nreq=9..26`. On disc a seek is the cost being protected; on the web the cost is the worker's
backlog, which keeps `m_loadingPriority` set forever because the flag is raised whenever anything
nearby is still in flight (`src/renderer/Renderer.cpp:1596`) and cleared only once a frame's
requests are all `LOADED`.

That is also why it looks like a regression that "comes back": the throttle only binds when the
cache is cold. T109 booted with a `dataTag` change that purged IndexedDB, so every request took
seconds and the queue never drained. The warm T107 session logged 1 `LODLEFT` row; the cold
sessions logged 40 (their cap). Vehicles and peds are unaffected because the Emscripten traffic
gate deliberately ignores `m_loadingPriority` (`src/core/Streaming.cpp:353`), which is why only
buildings look broken.

## Decision

Under `__EMSCRIPTEN__` the queue is throttled by depth alone; `m_loadingPriority` no longer
participates, in both sites above. The depth bound is 32 - above the measured operating range of
9..26 and 6x the vanilla 5/10, so a cold cache cannot silently re-starve the neighbourhood path.
The native code paths are unchanged, because on disc the seek saving is real.

`LODLEFT` is replaced by a bounded watcher over the same hook (`OdLodLeftDiag`,
`src/renderer/Renderer.cpp:838`): `LODSTUCK` reports how many frames a big building has been
drawn as a LOD while its twin is unloaded (repeating every 600 frames to prove permanence), and
`LODHEAL` reports the streak length at the moment the twin gains geometry. The old diag deduped
per model, so it could only prove a first sighting, never whether the condition was permanent.

## Consequences

- More files are in flight per frame while the queue is short; the per-frame work itself is
  unchanged, since `LoadRequestedModels` still processes at most one file per channel per frame
  (`src/core/Streaming.cpp:2682`).
- If `LODSTUCK` still repeats with `nreq=32`, the bound is still the limiter and the next step is
  to raise it or to serve the twin from the LOD's own request path, not to restore the priority
  term.
- `LODHEAL` rows are the acceptance signal for this change in the next playtest.
