---
name: 045-replay-recording-is-off-on-web-because-its-only-fault-is-a-crash
status: accepted
date: 2026-11-05
domain: stability
supersedes: []
---

# ADR-045: replay recording is off on web, because all it costs is a crash

## Context

The player reported a hard stop after an ambulance explosion. The crashed session's log ends
with three JS faults inside 33 ms (`odtrace.prev.log:13762-13764`): an `ODSHORT open-fail
errno=44`, then a caught `JSERR_REJ` whose frame is `RwStreamClose`, then a **fatal,
uncaught** `JSERR` whose stack is:

```
RpAnimBlendClumpGetMainAssociation
  <- CReplay::StorePedAnimation      src/control/Replay.cpp:463
  <- CReplay::RecordThisFrame
  <- CReplay::Update                 src/control/Replay.cpp:269
  <- CGame::Process
```

After that row only the browser-side watchdog keeps writing (`WDTALIVE ... mark=125`, the
logic tick frozen) until the page reloads (`BART pct=16`). So the fatal fault is the replay
recorder, not the explosion, not the ambulance, and not the streamer.

`CReplay::Update()` walks the whole vehicle pool and the whole ped pool every frame to build
one snapshot of the world, and `CReplay::RecordParticle()` writes a packet per particle.
`StorePedAnimation` dereferences `ped->m_rwObject` as an `RpClump`; a ped whose clump was
already released (exploded/wasted/reMOVED) gives garbage to `RpAnimBlendClumpGetMainAssociation`,
which indexes off the end of its own allocation - the out-of-bounds the browser reported.
Playback needs F1/F2/F3 and a `replay.h` file; none of that is reachable or useful in the
browser, so the recorder's whole output is discarded and its only observable effect is the
fault (plus per-frame pool walks on the hot path).

## Decision

`CReplay::Update()` and `CReplay::RecordParticle()` return immediately under
`#ifdef __EMSCRIPTEN__`, with a comment naming the reason. Nothing else in
the replay module is touched: the data structures, `Init()`, `Term()` and the menu hooks stay
as they are, so a native build is unchanged.

## Consequences

- The fatal path is gone and two full pool scans per frame plus one buffer write per particle
  stop being paid for - a small, honest FPS win folded into the same change.
- Replay is unrecoverable in the browser. If it is ever wanted, it must first be given a
  liveness test on `m_rwObject` and a way to reach playback; do not just delete the guard.
- The `RwStreamClose` fault two rows earlier stays **open**. `StreamMemory::close()` is empty
  and `StreamMemory::open()` cannot return nil in this port, so the symbol that the browser
  blamed is almost certainly an inlined neighbour, and a nil guard there would be dead code.
  ADR-044 (corrupt heap metadata from unvalidated DFF geometry) is the working hypothesis for
  that one and is *not* what this change fixes.
