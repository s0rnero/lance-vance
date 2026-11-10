---
name: 033-vram-accounting-is-balanced-per-raster
status: accepted
date: 2026-11-02
domain: rendering
supersedes: []
---

# ADR-033: VRAM bytes are balanced per raster, and streaming starvation is measured by queue age

## Context

Build T100 shipped ADR-032: measure live texture bytes in librw, publish them in
`WEBHB` as `texb=`, and evict least-used models when that number crosses a
watermark. The player's T100 session ran clean — 59-60 fps, no crash, several
missions — and in the same session the mechanism turned out not to exist.

### The counter went negative

`gta_vc_browser/logs/odtrace-2026-11-03_01-19-44.log` (UTC; the evening of
2026-11-02 locally), 2790 one-second `WEBHB` samples:

| quantity | value |
| --- | --- |
| live GL textures at session end | `tex=4253` |
| created / destroyed over the session | `texgen=10106` / `texdel=5853` (difference is exactly `tex=`) |
| live TXD slots / cumulative dictionary VRAM accounted | `txd=361/660MB`; last `ODTXD` reads `vivos=339 acum=649MB` |
| `texb=` minimum over the session | **-70 MB** |
| `texb=` maximum | +250 MB |
| `texb=` last sample | **-15 MB** |
| samples below zero | 232, the first at `01:52:20.700Z` — 2 s after the level reload at `01:52:18` |
| `ODTEXREL` lines | **0** |

A session that accounted 649 MB of dictionary loads and destroyed 5,853 of the
10,106 textures it created does not have -15 MB resident. The eviction gate is
`gOdTexBytes > watermark`, so a counter that spends its final minutes below zero
is a gate that never opens — which is precisely what `ODTEXREL 0` says, and
what the player experienced as "los lod no se actualizan y nada se descarga".

The reason is structural. ADR-032 added bytes in exactly one place — the
`LOCKWRITE` branch of `rasterUnlock` at level 0
(`vendor/librw/src/gl/gl3raster.cpp:608`) — and removed them in
`odTexRelease`, whose only guard was "this raster owns a nonzero texture id".
Those are different sets. `rasterCreateCameraTexture` takes its id from
`odTexGen` (`:253`), and a render target that is never written through the lock
API never receives the level-0 addition — yet on destroy it subtracted
`odTexRasterBytes()` in full. SkyGFX and the render passes create and delete
those rasters continuously, and a level reload recreates the whole front-end
set, which is the moment the T100 counter crossed zero (`01:52:18` reload,
`01:52:20` first negative sample). The subtraction also re-evaluated the RGB565
and mip formula at destroy time instead of remembering what had been added.

ADR-032's Consequences stated the opposite risk: "the counter can drift upward
over a long session". It drifted downward. ADRs are permanent and are not
corrected in place, so this record carries the correction.

### The gameplay failures left no error trace either

Two symptoms came out of the same session, and neither of them was an I/O error.

- **Hammer mission.** `hammer.txd` (TXD slot 239, model 265) was absent from
  memory at `01:50:34` and `01:51:44` while the player held the weapon; the
  shop screenshot shows the hammer slot empty for the same reason. Over the
  whole session `FAIL txd` and `FAIL dff` count **0**, and `ODSHORT` is silent
  in that window. Heap stayed at 384 MB and no read failed, so "the memory
  filled up" is not what happened: the request never reached a loading channel.
  A level reload at `01:52:18` made the hammer appear; closing the tab and
  reopening did the same thing again.
- **LODs, minimap, water.** `LODLEFT ... relacion=1 rwobj=0` appeared for 40
  models (4 in the previous build): the detailed non-LOD has no `RwObject`, so
  the near-distance branch of `CRenderer::DoRenderList` keeps drawing the
  low-resolution LOD forever. `radar27.txd` (a minimap tile) and, in the second
  session, `starwaterfrnt01.txd` (water) are the same shape.

`CStreaming` has two paths that produce exactly that silence: the requested-list
walk that drops a model whose TXD is not loaded and re-requests it
(`src/core/Streaming.cpp:594-614`), and the `TxdNotLoaded` break inside
`GetNextFileOnCd` (`:2452`), which skips the file with no message at all. "No
error was logged" therefore did not mean "nothing was wrong" — it meant there
was no instrument pointed at the queue.

## Decision

1. **Balance the counter per raster instead of by formula.** `Gl3Raster` gains
   `long long odBytes` (`vendor/librw/src/gl/rwgl3.h:271`). `odTexAccount`
   (`gl3raster.cpp:104-113`) adds `odTexRasterBytes() - odBytes` and stores the
   new value; `odTexUnaccount` (`:115-123`) subtracts **the stored `odBytes`**,
   not a fresh estimate. `odBytes` is initialised to 0 in `createNativeRaster`
   (`:923`) and `copyNativeRaster` (`:996`) and cleared in `destroyNativeRaster`
   (`:936`, immediately after `evictRaster`). `odTexRelease` is deleted; its
   three former call sites in `destroyNativeRaster` are plain `odTexDel`. The
   invariant is `gOdTexBytes == Σ odBytes` over live rasters: a byte can only be
   subtracted by the object that added it, so the counter cannot go negative
   whatever path a raster took to get its texture, and re-uploading level 0
   replaces the value instead of double-counting it.
2. **The number remains an estimate, and its blind spot is now declared.**
   `odTexRasterBytes` is untouched. Only rasters whose pixels went through
   `rasterUnlock` LOCKWRITE level 0 are counted, so a render target that the
   driver allocates and librw never writes is invisible to `texb=`. That is the
   behaviour change from ADR-032: those rasters used to be *subtracted*; now
   they are ignored.
3. **The watermark moves from 256 MB to 512 MB** (`src/core/Streaming.cpp:411`),
   revising clause 6 of ADR-032. That clause said 256 MB was provisional and
   that the next log would decide. The next log is T100's, and while its
   `texb=` values are unusable, its `ODTXD` accumulator is not: a session that
   ran at a steady 60 fps without crashing carried hundreds of MB of streamed
   dictionaries. With the counter repaired the gate will actually fire, so
   keeping 256 MB would ship a behavioural change calibrated on a corrupted
   measurement. 512 MB also matches the figure the port already assumes for the
   machine (`_dwMemAvailPhys = 512 MB`, `src/skel/glfw/glfw.cpp:641`). It is
   still a guess; T101 produces the first trustworthy `texb=` series.
4. **Starvation is measured by queue age: `ODHAMBRE`.** `CStreaming::Update`
   (`src/core/Streaming.cpp:426-468`, web-only) walks `ms_startRequestedList`
   once every 300 calls (~5 s at 60 Hz) and reports every entry that was already
   in the list during the previous epoch and has not been reported for it: a
   request that has sat in the queue for at least 5 s. The line resolves
   `nombre` for model, TXD and collision ids (`CModelInfo::GetModelName`,
   `CTxdStore::GetTxdName`, `CColStore::GetColName`) and prints `estado`,
   `flags`, `prio`, the model's `txd` slot and that slot's `txdEstado`, `nreq`,
   `prioritarios`, `mem=<ms_memoryUsed>/<ms_memoryAvailable>MB`, the state of both
   loading channels and `buf` (`ms_bLoadingBigModel`). Those fields separate the
   three candidate causes in one line: the priority gate (`:2325`), a dictionary
   that never loads (`:2452`), or an exhausted disc budget (`MakeSpaceFor`).
   Volume follows ADR-029 — the first 60 starvation events, then every 200th —
   and the `odColaAvisado` epoch stamp means one line per episode rather than
   one per poll.
5. **`idb=` was reading the wrong counter.** `gta_vc_browser/web/ondemand.js:367`
   published the census field from `OD.idbHits`, the legacy main-thread counter
   that stays at 0 while the loader worker serves the cache; the hits land in
   `OD.wkStats.idb`. T100 therefore reported `idb=0` in `WEBHB` while IndexedDB
   had served 2,865 hits against 565 network fetches. The census now sums both.
   No cache behaviour changed — the reading was the only bug.

## Consequences

- `ODTEXREL` should appear in the next long session for the first time. The pass
  condition is `texdel=` rising above the organic 5,853 while `texb=` settles
  just under 512 MB. If `texb=` sawtooths against a visible re-streaming cost,
  the watermark is too low and ADR-032 clause 6 is revised again with a real
  number behind it.
- If the T100 symptoms recur they will be **named** — `ODHAMBRE ... nombre=
  hammer`, `radar27`, `starwaterfrnt01` — instead of inferred from what is
  missing from the log. The probe changes no streaming behaviour: choosing
  between the priority gate, the `TxdNotLoaded` break and the disc budget is
  left to the measurement, because blind is how ADR-032's watermark was set.
- `RemoveLeastUsedModel(STREAMFLAGS_20)` refuses anything with references, so
  eviction cannot pull a dictionary out from under a visible entity. Its
  last-resort branch (`RemoveLoadedVehicle`) can reach live traffic, and that is
  the main reason the watermark sits roughly twice as far from the observed
  working set as ADR-032 proposed.
- `Gl3Raster` grows by 8 bytes per raster, negligible against the texture it
  describes.
- Trace volume: `ODHAMBRE` is at most 60 lines plus one per 200 events per
  session. The `idb=` fix changes the value of an existing `WEBHB` token, not
  its position, so `viceext-log-check.py` is unaffected.
- `vendor/librw` changed, so `vendor-librw-local-changes.patch` was regenerated
  from `git diff HEAD` inside the submodule and verified to differ from the
  committed patch **only** in the `src/gl/gl3raster.cpp` and `src/gl/rwgl3.h`
  sections: same 29 file sections, same order, LF preserved in both.
- Verification: compiled as `2026-11-02-41.0-T101` with `bash
  gta_vc_browser/build.sh`, exit 0, `error:` count 0, no new warning on any
  added line (the `gl3raster.cpp` warnings in the log are the pre-existing
  `-Wvariadic-macro-arguments-omitted` family), `ODHAMBRE` and `ODTEXREL`
  present in the linked `reVC.wasm`, `OD.wkStats.idb` baked into `reVC.js`, and
  `check-served-build.sh` EXIT=0. `dataTag` stays `2026-11-02-41.0-T97`: no
  served data changed. **No gameplay was verified in a browser** — the project
  forbids the agent from doing that — so `accepted` here means "decision taken,
  implemented and compiled", not "confirmed in the game".

## Alternatives Considered

- **Count bytes where they are allocated** (`rasterCreateTexture`,
  `rasterCreateCameraTexture`, the compressed path) rather than at unlock. It
  would cover render targets too, but at creation time the final format, size
  and whether the web RGB565 downconvert applies are all still unknown, so a
  second estimate pass would be needed — and re-opening the gap between what is
  added and what is subtracted is exactly what this ADR is closing. Level-0
  `LOCKWRITE` is where the bytes really go up.
- **Ask the driver for exact GPU bytes.** WebGL2 here exposes no per-texture
  memory query and no memory-extension budget, and the engine would still need
  its own bookkeeping to attribute bytes to a TXD.
- **Leave 256 MB and measure before changing behaviour.** Rejected: with a
  correct counter the gate fires, so "measure first" and "ship the change" are
  the same action — only the calibration number differs.
- **Lower `ms_memoryAvailable` instead.** Still rejected for the reason recorded
  in ADR-032: `desiredNumVehiclesLoaded` (`src/core/Streaming.cpp:235`) is
  derived from it in `size_t` arithmetic, and any budget under 65 MB wraps and
  silently stops traffic.
- **Trace the two silent paths directly** (inside `:601-614` and at the `:2452`
  break). Set aside: both are entered from the requested-list walk, so per-poll
  traces are high-volume by construction (ADR-029), and `ODHAMBRE` already
  prints the discriminating fields — `txdEstado` separates the dictionary break
  from the priority gate. If T101 shows starvation this line does not explain,
  the targeted traces come next.
- **Fix starvation in the same build.** Rejected on scope: three different
  causes produce the same symptom and only one of them is provably wrong today.

## State

`accepted` (2026-11-02). Repairs clause 1 of ADR-032 (`odTexRelease`) and
revises its clause 6 (256 MB -> 512 MB); ADR-032's decision — evict on measured
VRAM — stands, and its Consequences bullet predicting upward drift was factually
wrong: the counter drifted downward, which is why a permanent record needs a
successor rather than an edit. Related to ADR-029 (trace volume) and ADR-030
(the deferred read path that makes re-streaming cheap). The open question is the
watermark, and T101 is the first build whose `texb=` can answer it.
