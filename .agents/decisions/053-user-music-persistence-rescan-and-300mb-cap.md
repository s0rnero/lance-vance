---
name: 053-user-music-persistence-rescan-and-300mb-cap
status: accepted
date: 2026-11-07
domain: web-data
supersedes: []
---

# ADR-053: user music is an IDBFS mount with an exported rescan and a 300 MB cap on both directions

## Context

`USERTRACK` is a real Vice City radio station (`src/audio/audio_enums.h:14`), not a
San Andreas addition, and its name `FEA_MP3` already exists in all six served GXTs.
On the web port it was dead end-to-end: `_FindMP3s()` (`src/audio/sampman_oal.cpp:862`)
runs only once from `cSampleManager::Initialise()`, is `static`, and scans
`getcwd() + "\\MP3\\"` — `/MP3` on the web, which was MEMFS. Uploaded `.mp3` files
vanished on reload, and `music.js` called a `__vcMusicRescan()` that existed nowhere.

The player decided a single hard limit of **300 MB accumulated**, both when uploading and
when downloading, and asked to be able to hear songs immediately after upload without a
reload. The limit is closed; it is not a parameter to re-discuss.

## Decision

- `/MP3` is mounted in IDBFS inside the same `preRun` as `/userfiles`
  (`web/lib/index.js:1270`), before `FS.syncfs(true, …)`, so the existing
  `addRunDependency('idbfs-populate')` covers both mounts. `music.js` keeps its `/MP3`
  path; `OD.syncUserfiles` already flushes every IDBFS mount.
- `extern "C" void EMSCRIPTEN_KEEPALIVE ViceExtRescanUserMusic(void)` is added in
  `sampman_oal.cpp`. It calls `_DeleteMP3Entries()`, zeroes `nNumMP3s`/`_pMP3List`, calls
  `_FindMP3s()` and rebuilds `nStreamLength[STREAMED_SOUND_RADIO_MP3_PLAYER]` and each
  entry's `nTrackStreamPos` from `nTrackLength`, then resets `_CurMP3Index`/`_CurMP3Pos`.
  JS calls it directly as `globalThis.Module._ViceExtRescanUserMusic()`; no `ccall` and no
  `EXPORTED_RUNTIME_METHODS` were needed.
- `build.sh` exports `_main,_ViceExtRescanUserMusic`. **`_main` must stay in that list**:
  Emscripten 6.0.9 treats an `EXPORTED_FUNCTIONS` list without `_main` as a reactor build,
  emits `unused-main`, drops `main` from the exports and never runs the game.
- `CAP = 300 * 1024 * 1024`. Upload sums the accepted files plus the bytes already in
  `/MP3` and rejects the **whole batch** before writing the first byte. Download refuses
  to build the ZIP if the selected MP3s exceed the cap. Only `.mp3` with a valid ID3 or
  MPEG frame signature is accepted.
- `deleteAllMusic()` unlinks `/MP3/*.mp3`, syncs and rescans so the station is skipped
  again; `downloadAllMusic()` builds `musica-vice-city.zip` with the shared `zip.js`.

## Consequences

- Songs survive an F5, `USERTRACK` appears in the radio cycle and plays without a reload,
  and deleting songs removes the station again.
- A batch over the cap writes nothing; a download over the cap produces no ZIP. Both
  limits are enforced in one module, not scattered across the three managers.
- The rescan frees `_pMP3List` while `USERTRACK` may be playing (risk R-12). This is left
  for the player's session to measure; no speculative guard was added in C.
- `EXPORTED_FUNCTIONS` is now a real list. Any future native function exposed to the web
  must be appended to it, and `_main` must remain.

## Alternatives Considered

- Storing music under `/userfiles/MP3` and rewriting `_FindMP3s()`'s path: rejected in D12
  because it costs more C++ for the same result and still needs the rescan.
- Lazy re-scan in C on `IsMP3RadioChannelAvailable()` (plan 4.8 variant b): rejected by the
  player in D11 because it leaves a stale list after a delete.
- `Module.ccall` with `EXPORTED_RUNTIME_METHODS=ccall`: unnecessary; the direct export is
  already assigned to `Module['_ViceExtRescanUserMusic']` by the generated glue.
- Per-file size caps: rejected; the player asked for an accumulated total.

## State

`accepted`
