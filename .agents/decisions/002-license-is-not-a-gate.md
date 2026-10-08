---
name: 002-license-is-not-a-gate
status: accepted
date: 2026-09-27
domain: process
---

# ADR-002: Mod licenses are not a gate

## Context

The mods planning docs (`00-INDICE` §2, `09-numeracion`, `ATTRIBUTION.md`) treated
licenses as a hard rule: MIT mods could be copied with attribution, mods without a
LICENSE (skygfx, classic-axis) were to be "reimplemented" rather than copied, and
`sa-crouch-movement` (explicitly prohibitive) was "spec only". This cost effort and
hesitation for no benefit in this project.

## Decision

This is a **local, personal project; nothing is redistributed**. Licenses **do not
block** taking code, logic or data from `mods/`: take whatever the game needs, with or
without a LICENSE. The attribution header and `docs/mods/ATTRIBUTION.md` are kept as
**courtesy and traceability**, not as an obligation.

The only things excluded are those **technically inapplicable in WASM** (x86
addresses, `hook::pattern`, `.asi`, `.ual`, `rwd3d9`, `injector`, `plugin-sdk`,
memory-hackopcodes). Art assets are not copied not for legal reasons but because the
game serves them locally and anything missing is drawn by code.

## Consequences

- No more "reimplement-with-attribution" ceremony for no-LICENSE mods: direct ports allowed.
- Attribution remains, so provenance of each ported fix is still traceable.
- The technical exclusions (ADR-001) still stand and are unrelated to licensing.

## Alternatives Considered

- **Keep the license gate**: rejected — it slowed work with no real risk here.
- **Drop attribution too**: rejected — the manifest is useful traceability regardless.

## State

`accepted`.
