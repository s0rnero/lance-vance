---
name: 027-on-demand-cache-is-keyed-by-data-content
status: accepted
date: 2026-11-02
domain: on-demand-data
amends: [020-mod-weapons-and-second-sound-bank-are-the-mods-own-code]
---

# ADR-027: The on-demand cache is keyed by data content, not by build tag

## Context

`streamed/` (17,490 files, 1.40 GB) is fetched on demand and cached in IndexedDB
`vcod2` with a 900 MB ceiling. Cache validity was tied to `OD.dataTag` in
`gta_vc_browser/web/ondemand.js`: `idbPurgeIfStale()` compared `dataTag` against
a `__datatag__` marker in the `files` store and called `idbClear()` on any
mismatch.

`CODING_STANDARDS.md` §3 says `dataTag` changes only when the served data
changes, but in practice it tracks the build number. The player's logs show the
cache purged in twelve consecutive sessions:

```
T57 -> T63 -> T65 -> T66 -> T67 -> T68 -> T69 -> T70 -> T71 -> T96 -> T97
```

Measured effect in `gta_vc_browser/logs/odtrace-2026-11-02_17-55-33.log`
(build `2026-11-02-41.0-T97`):

```
ODWK peticiones=1947 aplazados=323 entregados=1947 ... idb=74 red=1873
```

96% of deliveries came from the network. The cache was not broken; it was empty
at every start, so the whole 1.40 GB was re-downloaded session after session.

The same defect hit the warm-start list (`vc_warm1` in `localStorage`), which
was also tagged with `dataTag`: session 17:38 preloaded
`ODWARM n=133 mb=2 ms=308 lista=138`, and the next session, after the `T97`
bump, logged `ODWARM sin lista previa`.

There was no client-visible content identity to use instead: `manifest.json`
carried only `{p, s}` per file, and size is not identity in this project.
`Audio/sfx.SDT` is 198820 bytes both in vanilla and in the regenerated bank
(ADR-020), and ADR-025's mesh reorder changed geometry at, in its own words,
"mismo tamano".

## Decision

1. **Content stamp in the manifest.** `gta_vc_browser/tools/gen_manifest.py`
   emits `man["__vc__"] = {"v": 1, "h": <blake2b-128 hex>}`. The digest covers
   exactly what the manifest serves: entries are collected with the existing
   `SKIP` and `.hide` exclusions, sorted by lowercase relative path, and the
   stamp is fed `path + 0x00 + blake2b-128(file bytes) + 0x00` per file. Files
   are read in 1 MiB chunks, so `anim/cuts.img` (115 MB) is never loaded whole.
2. **Deterministic output.** Entries are now emitted in sorted order, so
   identical content produces a byte-identical `manifest.json`.
3. **Cache key derived from the served data.** `OD.cacheKey()` returns
   `'m' + manifest['__vc__'].h` and falls back to `OD.dataTag` when the stamp is
   absent. The `'m'` prefix keeps the two namespaces from colliding.
4. **Marker renamed.** `__datatag__` becomes `__cachekey__` in the same `files`
   store. Legacy caches hold no `__cachekey__`, so the first session after this
   change purges once and `idbClear()` removes the old marker with everything
   else. No migration code is needed.
5. **The marker is exempt from the LRU.** `idbTrim` filters it out of the key
   list before sizing and evicting. This is a correctness requirement, not
   tidiness: the marker has no `meta` entry, so it sorted as `t = 0` and was the
   first key dropped whenever the cache exceeded `idbCapMB`, which would have
   caused a silent full purge on the following session.
6. **Warm list uses the same key.** `warmLoad()` and `warmPersist()` tag
   `vc_warm1` with `OD.cacheKey()`.
7. **The manifest is revalidated on every boot.** `init()` fetches it with
   `cache: 'no-cache'`, matching what `probeData()` in `web/lib/index.js`
   already did. The manifest is now an identity source: a stale HTTP-cached copy
   would keep an old stamp while `streamed/` had changed, and the browser would
   serve stale bytes with no purge at all.
8. **`dataTag` keeps its reporting roles** (boot trace, RECOIL lines) and stops
   being a cache-invalidation input. ADR-020's consequence is amended: rebuilding
   the sound bank requires regenerating the manifest, and the stamp — not a
   manually bumped tag — is what invalidates the cache.

## Consequences

- Builds that do not touch `streamed/` keep the entire cache. From the second
  session on the same data, `idb=` should dominate `red=` in `ODWK`; that ratio
  is the field that confirms or refutes this ADR.
- Correctness no longer depends on a human remembering to bump a tag. The stamp
  is computed from the bytes, so a same-size content change invalidates the cache
  instead of silently serving the old one.
- `gen_manifest.py` now reads all 1.40 GB. Measured on this machine, a full
  blake2b pass over `streamed/` takes ~18 s, so the generator goes from seconds
  to tens of seconds.
- The stamp is all-or-nothing and the purge is still `idbClear()`, so editing one
  DFF invalidates the whole cache. Per-file reconciliation would fix that, and
  the generator already hashes every file, so it needs no new data — only a
  per-entry digest field and a reconciliation pass in place of the wholesale
  clear. Deliberately not done here: it cannot be validated without a build, and
  a mistake there serves stale bytes instead of merely re-downloading them.
- `manifest.json` gains one reserved top-level key that is not a file path. Both
  consumers (`probeData()` and the `OD.manifest[OD.norm(canon)]` lookups) do
  keyed access only and nothing iterates the map, so it is inert; a file named
  literally `__vc__` at the root of `streamed/` would collide.
- The change is inert until the manifest is regenerated. With the currently
  served `manifest.json` (no `__vc__`) `cacheKey()` returns `dataTag` and the
  behaviour is exactly the previous one. `python gta_vc_browser/tools/gen_manifest.py`
  is a data script and was not run as part of this decision.
- Verification performed: the generator was exercised against a synthetic tree
  (determinism across runs, mtime independence, same-size content change, added
  file, removed file); the client logic was exercised in node with the real
  `cacheKey`, `idbPurgeIfStale`, `idbTrim` and `idbSizeOf` extracted from
  `ondemand.js` and run against a fake IndexedDB — 15 checks, including the
  marker surviving an over-cap eviction. No build was produced and no gameplay
  was verified in a browser; per the project's standing convention `accepted`
  means "decision taken and implemented".

## Alternatives Considered

- **Keep `dataTag` and enforce §3 by discipline**: rejected. It is a manual step
  in a lane that bumps the tag on every build, and twelve consecutive purges show
  the discipline does not hold.
- **Hash the manifest text in the browser with `crypto.subtle.digest`**:
  rejected. It needs a secure context and an extra async step at boot, and it is
  only as accurate as the manifest content — which, without per-file digests,
  still misses same-size edits. Reading a stamp emitted by the generator is exact
  and works on any origin.
- **Per-file digest on all 17,490 entries**: rejected for now. It would add
  ~330 KB to a manifest fetched on every boot for a client that only ever purges
  wholesale. It becomes worth its cost together with per-file reconciliation.
- **Fold the digest into the IndexedDB key** (`canon + '#' + h`), so stale
  entries are never read and the LRU evicts them on its own: rejected. It is the
  most elegant option but touches every key-construction site, including the
  worker's, and none of it can be validated without a build.

## State

`accepted` (2026-11-02). Amends ADR-020's data contract consequence. Not
superseded.
