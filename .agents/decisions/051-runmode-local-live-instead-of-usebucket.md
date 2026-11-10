---
name: 051-runmode-local-live-instead-of-usebucket
status: accepted
date: 2026-11-07
domain: web-data
supersedes: []
---

# ADR-051: the shell's data origin is a run mode, not a bucket on/off switch

## Context

Plan 43.0 (October) introduced `useBucket: false` in `gta_vc_browser/web/lib/index.js`
as a flag that was supposed to switch the whole game's data origin to the R2 worker.
Nothing was ever downloaded: the flag only gated the intro video fallback. The player
rejected the name on 07/11 because it describes a yes/no bucket feature rather than
how the game is being run, and because the game is still being modified on the local
disk. Starting to pull the engine, data and trailer from the bucket while local
development is in progress would obstruct that work.

The player also stated that the execution gate does **not** turn the bucket on
(`el gate enciende el bucket no`). Bucket/trailer/assets loading belongs at the very
end, "cuando no haya mas procesos locales corriendo", and is deferred until the player
explicitly resumes it.

## Decision

`useBucket` is replaced by `runMode?: 'local' | 'live'`, default `local`.

- `local`: `bucketUrl` is `null`; `buildUrl`, `streamedUrl`, `manifestUrl` and
  `assetsUrl` keep their relative development-disk origins; `trailerUrl` is `null`;
  the intro's fallback-to-bucket branch is gated by `live && trailerUrl`. The result is
  **zero requests to the bucket host**, including when the local intro video is missing.
- `live`: reserved for the published build. It derives the absolute origins from
  `bucketUrl` and enables the trailer fallback. No new download capability was added in
  this execution; the flag is a name and a precision, not a new transport.

The option is declared in `web/lib/index.d.ts` and consumed at `web/lib/index.js:464`.

## Consequences

- Local development keeps working exactly as before, with no network dependency on the
  Worker. A missing `intro/vc.mp4` ends the intro instead of silently reaching out to
  the bucket.
- Publishing requires setting `runMode: 'live'` plus the deferred blocks 1 and 2 of
  plan 43.0 (`bulkDownload`, `__bulk__`, `OD.dataReady`, `storage.persist()`,
  `idbCapMB 0`, the trailer asset). Those remain pending in the plan.
- The old `useBucket` name is gone from `web/lib/`; no compatibility alias was kept,
  because the option was never consumed by any host.

## Alternatives Considered

- `deployMode: 'dev' | 'release'`: rejected by the player, who chose `runMode` with
  `local`/`live`.
- Keeping `useBucket` and merely defaulting it to false: rejected, because a boolean
  cannot express "published mode" and invites the mistaken belief that the gate enabled
  bucket downloads.

## State

`accepted`
