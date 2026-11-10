---
name: 044-the-web-crash-is-corrupt-heap-metadata-from-unvalidated-dff-geometry-not-memory-exhaustion
status: accepted
date: 2026-11-04
domain: stability
---

# ADR-044: the web crash is corrupt heap metadata from unvalidated DFF geometry, not memory exhaustion

## Context

The player crashed in their first playtest of T112 (`2026-11-04T15:23:52.578Z`,
`gta_vc_browser/logs/odtrace-2026-11-04_15-20-03.log`). Seventeen identical
`JSERR_REJ RuntimeError: memory access out of bounds` rows, all with the same stack:

```
emscripten_builtin_malloc <- rw::malloc_h <- rw::mustmalloc_h <- rw::Geometry::create
  <- rw::Geometry::streamRead <- rw::Clump::streamRead <- RpClumpStreamRead
  <- CFileLoader::LoadAtomicFile <- CStreaming::ConvertBufferToObject
  <- CStreaming::ProcessLoadingChannel
```

then `WDTALIVE mainSilentMs=889 stall=0`, `5886 stall=1`, `10886 stall=1`: every Asyncify
resume re-faulted and the main loop never came back.

Two hypotheses were live: heap exhaustion, or a corrupt DFF slice reaching the parser.

Exhaustion is ruled out by measurement. At the fault the heap was `heap0=480MB` against
`-sINITIAL_MEMORY=402653184 -sMAXIMUM_MEMORY=2147483648`, `memfs 3726/402MB` flat, and no
growth-failure row anywhere in the session. The *next* session, which the player reloaded into
and played 26 minutes without a crash, reached `heap0=499MB` — higher than the crashed one.
`mustmalloc_h` also prints `Error: out of memory` and `exit(1)` on a genuine failure; neither
appears.

What remains is corrupt allocator metadata. Faulting *inside* `malloc` is what a smashed
dlmalloc free list looks like: the crash lands on whichever code next asks for memory, not on
whatever overran. The history confirms that reading. Across all archived sessions the same
`memory access out of bounds` appears 8234 times in `emscripten_builtin_malloc` and, at lower
counts, in `CRunningScript::Load` (126), `RpAtomicGetGeometry` (74), `CSprite` (17),
`rw::Frame::updateObjects` (11), `_emscripten_memcpy_bulkmem` (9), `CPed::LoadFightData` (7),
`CPickups::Load` (6), `memchr` (3) — an essentially random victim set, going back to
`2026-10-10`. It is chronic and predates T111 by weeks, so the deeper streaming queue is not
the cause; T111's own acceptance signal fired correctly in both sessions
(`LODHEAL` 33/40, `LODSTUCK` 0, `CARBLOCK` 4/3 against 153/153 before).

The 24/10 guard in `Geometry::streamRead` (`GEOSTRUCT rechazado`) was built for exactly this
failure and logged **0 rows** in the crashed session, as did `MATLIST ausente`, `MESHBAD` and
`FAIL model`. So the slice that killed the run was *not* one the guard could see: it passed the
header check and corrupted the heap afterwards. Reading `vendor/librw/src/geometry.cpp` for
writes that the guard does not bound found three:

1. `numTexCoordSets` is `(flags & 0xFF0000) >> 16`, range 0..255, but `Geometry::texCoords` is a
   fixed `TexCoords *texCoords[8]` (`src/rwobjects.h:526`). `Geometry::create` loops
   `for(i = 0; i < numTexCoordSets; i++) geo->texCoords[i] = data`, so indices 8..254 overwrite
   `morphTargets`, `matList`, `meshHeader`, `instData`, `refCount` and then the neighbouring
   heap block. `streamRead` afterwards does `read32(geo->texCoords[i], ...)` — a write to
   whatever those overwritten words now hold.
2. `Geometry::addMorphTargets` computed `int32 sz` and allocated `rwNew(n*sz)` with `n` =
   `numMorphTargets` and `sz` ≈ `24 + 24*numVertices`. The product overflows int32, the
   allocation comes back small, and the initialisation loop then writes `n` full morph targets
   through it. The guard bounded `numMorphTargets` only from below (`<= 0`).
3. The morph-target loop in `streamRead` trusts the stream's `hasVertices`/`hasNormals` bools,
   while `streamWrite` emits them as `vertices != nil` / `normals != nil` and `addMorphTargets`
   only allocates `normals` when `flags & NORMALS`. On disagreement the reader writes
   `12*numVertices` bytes to address 0 — the `_emscripten_memcpy_bulkmem` faults in the history.

## Decision

Validate the geometry against what the code will actually write, and reject the atomic instead
of corrupting the heap. All three fixes are in `vendor/librw/src/geometry.cpp`, so
`vendor-librw-local-changes.patch` is regenerated in the same change:

- the `GEOSTRUCT` guard also rejects `numMorphTargets > 8` and `texSets > 8`, and its row now
  prints `uvsets=`. The bound of 8 is the `texCoords[]` array size, and the game only ever
  touches `morphTargets[0]` (`src/vehicles/Automobile.cpp:2303`,
  `src/vehicles/SteeringWheel.cpp:225`), so no shipped asset is near either cap.
- `addMorphTargets` sizes in `size_t`, and its byte-copy loop no longer relies on `while(len--)`
  underflowing.
- the morph-target loop refuses a stream that claims vertices or normals the allocation does not
  have room for: `GEOMORPH rechazado` and `goto fail`. A zero-vertex geometry skips the reads
  instead, which keeps empty geometries working.

A rejected atomic already has a path: `Geometry::streamRead` returns nil, the clump read fails,
`LoadAtomicFile` returns false, and `ConvertBufferToObject` logs `FAIL model <nombre>` and
re-requests. The crash becomes a named, retried model.

Native builds are untouched — the caps are far above any real asset, and the nil-destination
check only fires on a file that disagrees with itself.

## Consequences

- This is not proven to be the trigger for the 15:23:52 crash; it is proven to be an unbounded
  write in the function that crashed, in a codebase whose crash history is a random victim set.
  The next session decides: `GEOSTRUCT`/`GEOMORPH` rows plus a matching `FAIL model <nombre>`
  would name the offending model, and the `emscripten_builtin_malloc` faults should stop.
- If the faults continue with all three rows at 0, the corruption is upstream of the geometry
  parser and the next instrument belongs at the slice boundary — `ConvertBufferToObject` does
  not currently name the model or check `offsets[i] + cdsize` against the streaming buffer, and
  `RequestModelStream` still carries its warning about non-contiguous chain slices.
- The same session answered the audio complaint. `LOOPLONG` attribution shows the long loops the
  player hears are legitimate (`SFX_CAR_IDLE_1..11`, `SFX_MOPED_IDLE`, `SFX_ROAD_NOISE`,
  `SFX_CAR_AFTER_ACCEL_18`, all `type=0` = `AUDIOTYPE_PHYSICAL`). The one genuinely stuck loop is
  `LOOPKILL sfx=37 ent=8 type=3` — `SFX_PALM_TREE_LO` on `AUDIOTYPE_WEATHER`, killed four times
  in 26 minutes at `silentsec=25`. `cAudioManager::ProcessWeather` (`src/audio/AudioLogic.cpp:9143`)
  re-queues it every frame while `wind > 0.0f`, and with wind barely above zero the float volume
  truncates to 0; a sound that is always requested never reaches the releasing path in
  `cAudioManager::ProcessActiveSamples`, so it holds a channel silently until the watchdog
  intervenes. It now queues only when the computed `Vol > 0`, which is the guard
  `ProcessEngineDamage` already uses two thousand lines above for the same sample.
- ADR-042's condition fired: `SEATGHOST` was 0 in both sessions while the player reports the
  seat as correct, so the inert seated-ped guard is removed from `src/peds/Ped.cpp`. The alpha
  hypothesis is now exonerated twice.
