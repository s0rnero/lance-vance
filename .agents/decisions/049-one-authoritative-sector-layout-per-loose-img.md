---
name: 049-one-authoritative-sector-layout-per-loose-img
status: accepted
date: 2026-11-05
domain: web-data
supersedes: []
---

# ADR-049: one authoritative sector layout per loose IMG, and nothing builds on top of a desynced copy

## Context

The web port serves `models/gta3.img` as a **directory of loose files** plus a synthetic sector
index `models/gta3.dir` (32-byte records: `start:u32`, `sectors:u32`, `name:24`). Two copies of
that index exist: `gta_vc_browser/streamed/models/gta3.dir` (served on demand) and
`gta_vc_browser/bootseed/models/gta3.dir`, which is **baked into `reVC.data`** by
`--preload-file`. The engine reads the baked one at boot, and `CdStreamPosix` resolves every
sector request against it in memory. So the copy that decides what the game sees is the bootseed
one, not the served one.

The crash in the 22:01 session (`2026-11-05-45.0-T5`, plan 47.0) came from those two copies
describing different layouts. Measured on the bootseed copy after plan 45:

| index | entries | sum of sectors | end of last entry | holes | overlaps |
|---|---|---|---|---|---|
| before | 6096 | 178489 | 178476 | 367 | 368 |
| after | 6096 | 178498 | 178498 | 0 | 0 |

The cause is in `repack_dir.py`: it recomputed each copy by walking **its own** tree, so an entry
whose file was not present in that tree kept its old `start` while the running cursor moved on.
367 holes and 368 overlaps later, two models claimed the same sector, `OdDoRead` returned a slice
of one of them for the other, and the RW header of the foreign model declared more bytes than the
buffer - the `rw::malloc_h` out-of-bounds of ADR-044, whose "why the slice was foreign" was
unknown until this measurement.

## Decision

- `repack_dir.py` builds **one** layout: entries keep their original order, `start` is a running
  cursor over `ceil(real_size / 2048)` sectors, and an entry whose file is absent in this tree
  still occupies its slot (cursor advances, `start` is written). It refuses to write a layout that
  is not contiguous, and `--mirror DIR` writes the other copies with the same bytes.
- `tools/check_img_index.py` is a **hard gate in `build.sh`**: contiguity, per-entry sector count
  against the served file, byte-for-byte equality of every mirror, manifest presence and size,
  bootseed.list inclusion. A desynced index cannot produce a build. Verified by shifting one
  entry 5 sectors on purpose: `build.sh` exits 1 before `emcmake`.
- The engine defends itself anyway, because the index can only be as good as the last run:
  `OdDoRead` returns `STREAM_ERROR` when a read is short by a whole sector (zeros were previously
  handed to the parser as if they were data), `ConvertBufferToObject` rejects a DFF whose BIN
  header declares more bytes than the slice (`DFFGARBAGE`) before librw touches it, and a
  streamId that fails four times stops being re-requested for a while (`RETRYCAP`) so one bad
  entry cannot keep the loading queue busy.
- `DIRTAG` prints the index the engine actually loaded (`n`, `total`, FNV hash) so the claim in a
  bug report can be checked against the file on disk. Expected for this data:
  `n=6096 total=178498 h=6742a6fe`.

## Consequences

- Editing a served file now requires one command (`repack_dir.py <box> --mirror <bootseed copy>`),
  and the build proves it was done. The failure mode this removes was silent: the log showed a
  crash, never an index mismatch.
- The mirror is byte equality, so a deliberate difference between the two copies is no longer
  possible. None is wanted: they describe the same sector space by definition.
- 11 of the 6096 entry names differ only in case (`chef.dff`/`CHEF.DFF`). `CdStreamPosix`
  lowercases the base path and this machine's filesystem is case-insensitive, so those pairs
  resolve to the same file: each entry gets its own sector slot and a valid RW slice (no crash),
  but one of the two models is the other one's geometry. Pre-existing, unrelated to this change,
  and now visible as a `AVISO` in the validator instead of being invisible.
- The extra defences cost a handful of comparisons per read and per conversion; nothing in the
  frame path.
- `dataTag` and the manifest seal both moved; the seal (`__vc__`) is what actually purges the
  IndexedDB cache (`cacheKey()` prefers it), the tag is for reading the trace back per plan.

## Alternatives Considered

- **Revert plan 45's DFF resizes.** Rejected by the player: those changes are wanted, and the
  DFFs were never the problem - they are byte-valid; the index describing them was wrong.
- **Have `build.sh` re-run `repack_dir.py` on every build.** Rejected: the repack is a data
  authoring step with a report of what changed; silently rewriting an index on each build hides
  the very thing that needs to be noticed.
- **Rebuild the index from the `.img` archive instead of the loose tree.** Rejected: the loose
  tree is what is served, so it is the only thing a validator can check against the bytes the
  browser will get.
- **Fix only the bootseed copy and leave `repack_dir.py` as it was.** Rejected: the tool would
  produce the same desync at the next resize.
