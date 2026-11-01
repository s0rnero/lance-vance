---
name: 022-no-attribution-or-authorship-in-the-repository
status: accepted
date: 2026-10-25
domain: process
supersedes: [002]
---

# ADR-022: The repository carries no attribution, credit or authorship notices

## Context

ADR-002 made licences a non-gate and kept attribution "as courtesy and
traceability": every ported block carried a header with the mod name, its licence,
its author and the upstream URL, and `docs/mods/ATTRIBUTION.md` required that header
above each block.

Measured on 2026-10-25, that convention had grown into 174 attribution lines across
61 files of `src/`, with `src/extras/` carrying one such header per global or
function, plus a 678-line attribution document. It also contradicted the project's
own rule that code carries no comments: the headers were the largest single class of
comment we still owned, and they carried no information the code needs to compile or
to be understood.

The player's decision (2026-10-25) is explicit: this repository is code, and it holds
no acknowledgements, credits or authorships of any kind. If he ever wants to credit
something, he does it himself, in a publication or in the README, when he says so.

## Decision

1. **No attribution, credit, acknowledgement or authorship notice** in any file of
   the repository: no headers, no author names, no licence blurbs, no upstream URLs
   in comments, no attribution document.
2. The 174 lines and `docs/mods/ATTRIBUTION.md` are removed. Provenance is not lost:
   it lives in ADRs 016-021 and in the plans, which record which mechanism came from
   which source, where it lives in the engine and why it was transplanted.
3. The licence question of ADR-002 does not change: licences are not a gate for this
   private project. What changes is only where the courtesy lived.
4. New code does not reintroduce any of it. A future credit is a decision of the
   player, recorded then, never a default of the agent.

## Consequences

- The repository reads as code: no per-block ceremony, and the comment rule has no
  exception left.
- Provenance is one hop away instead of zero: an ADR or a plan instead of the header
  sitting on top of the function.
- ADR-002 becomes `superseded` by this record; its licence reasoning stays valid and
  is not edited (rule 0.12).
- Anything the player later wants to credit belongs outside the tree (a release note,
  a README paragraph he writes), not in the sources.

## Alternatives Considered

- **Keep the headers and only consolidate them**: rejected — the player forbade
  attributions, and consolidating a forbidden element keeps it.
- **Move them to a single `CREDITS` file**: rejected — the prohibition covers the
  repository, not only the sources.
- **Keep them for the mods whose licence requires notice**: rejected as a licence
  question. ADR-002 already decided licences are not a gate here; nothing is
  redistributed.
- **Strip only the ones we wrote and leave upstream notices**: not applicable —
  measured, `src/` carries no upstream licence headers at all (0 files with a
  copyright header); every notice in the tree was ours.

## State

`accepted` (decided by the player on 2026-10-25, plan `20.0-limpieza-prepublicacion`).
Verified in that plan: `code-census.py --atribuciones` reports 0 lines and
`docs/mods/ATTRIBUTION.md` no longer exists.
