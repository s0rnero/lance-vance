---
name: 014-revert-plan70-regressions
status: accepted
date: 2026-10-10
domain: player-controls-aim-crouch
supersedes: [013-aim-crosshair-centered-from-mod-data]
---

# ADR-014: Revert the 7.0 implementation back to the 6.0 state

## Context

Plan 7.0 (`7.0-agachado-apuntado-salto-luces`, build `2026-10-09-plan70`)
implemented `ADR-013` (centered aim crosshair) plus aim, crouch, roll, jump
and turn-signal changes. The player's session on that exact build showed
three results: a sniper scope overlay drawn while aiming with a shotgun in
third person, no improvement in aiming or crouching, and a hard crash on the
second session (`RuntimeError: memory access out of bounds` in
`CAnimBlendNode::FindKeyFrame` via `CAnimBlendAssociation::Start` via
`CAnimManager::BlendAnimation` via `CPed::Attack`). The log leading into the
crash shows the aim pose never resolved (`armsight pose=? posepeso=0.00`
with the shotgun standing) while the fire cycle ran into `estado=16/22`.

## Decision

Revert every 7.0 code hunk, keeping the 6.0 state (diagnostic traces,
angle helper, `6.0` movement fallback). `ADR-013` is `superseded` by this
record; the crosshair stays at the mod's offsets. No part of 7.0 is kept:
a crash is a hard blocker and partial keeps cannot be trusted without a
clean baseline. The re-approach (plan 7.1) is measure-first: the aim pose
must resolve to a real clip before any shot path runs, and the scope-mask
draw condition must be found and gated to scoped weapons before any HUD
gate is touched again.

## Consequences

- The game returns to its pre-7.0 behaviour: the scope overlay and the
  `Attack` crash are gone with the code that introduced them.
- The six original problem groups are still open and move to plan 7.1.
- `ADR-013` remains in the registry as history; this record is the one in
  force.

## Alternatives Considered

- **Keep the non-crashing parts of 7.0**: rejected — with a hard crash in
  the fire path and an unresolved aim pose, no 7.0 hunk in that area has a
  trustworthy baseline.
- **Fix forward on top of 7.0**: rejected — same reason; the player asked
  for the revert explicitly.

## State

`accepted` as a design decision. Verified: reverted build
`2026-10-10-plan70rev` links (exit 0), suite 39/39, served check exit 0.
Gameplay re-validation belongs to the player's session.
