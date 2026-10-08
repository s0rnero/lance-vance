---
name: 003-mod-spec-wins-over-existing-code
status: accepted
date: 2026-09-26
domain: architecture
---

# ADR-003: The mod's behaviour is the specification; existing code is rewritten, not patched

## Context

Much of the ported feature area (swimming, crouch, aiming, recoil…) already had code
in the tree from earlier rounds. That code was frequently half-finished, broken or
unpolished, and patching on top of it produced regressions and conflicting behaviour
(e.g. the crouch "fighting between two animations", the aiming law that was not the
mod's). The player's permanent instruction (26/09/2026) was explicit.

## Decision

For every mod's mechanic: **ignore the existing implementation and prioritise the
mod's code, logic and behaviour.** Rewrite from the mod's spec; do not patch over what
is already there. The mod is the source of truth.

**Exceptions (intocables):** the recent, validated features — camera auto-centering
and recoil — are not rewritten.

## Consequences

- Cleaner results: the mechanic behaves like the mod, not like an accumulation of patches.
- More upfront work per mechanic (rewrite), but fewer regressions.
- When there is no mod source (binary-only), the spec is mined from the mod's INIs,
  readmes and the `ViceEx.exe`, and documented before implementing.

## Alternatives Considered

- **Patch the existing code**: rejected — it is what produced the current defects.
- **Keep both and negotiate**: rejected — the crouch round proved the two anim systems
  fight and deform the body.

## State

`accepted`.
