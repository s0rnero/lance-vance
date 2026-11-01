---
name: 021-transplant-equivalence-check
status: proposed
date: 2026-10-23
domain: mods
supersedes: []
---

# ADR-021: A transplant is validated block by block before it counts as done

## Context

ADR-016 orders each Vice Extended feature to be transplanted from the mod's own source
(or measured with REA when 2506/2510 differs), removing the old code of that feature in
the same step. Its evidence is one trace per family and one marker in
`check-served-build.sh`, and nothing else.

That evidence cannot tell a transplant from a look-alike, in either direction:

1. The vehicle siren block is a **hybrid** no check flags: vanilla positions kept, two
   coronas when the `.dff` carries `servicelights_*`, a port-invented radius/range of
   `0.55/60`, no rear pair on the ambulance and no camera-angle gating, while the mod's
   block has all three.
2. The save menu **invented** `SAVESLOT_9`, moved the autosave row to the top, deleted the
   `FEM_SL8` row, hardcoded its label and left `Slots[]` indexed by row position, so every
   row operates on the wrong slot. No trace and no marker caught it.
3. Conversely, on 23/10 `CPlayerPed::ProcessCrouch` was accused of being a reduced copy of
   the mod. A line-by-line comparison proved it is faithful (every guard, `PED_ROLL`,
   `CROUCH_BACKWARD`, the `padUpDown` condition and the aim branches). The gate also
   protects good code from being rewritten for no reason.

Traces measure what their author remembered to trace (ADR-006) and markers prove the wasm
carries code, not that the code is the mod's. Neither answers the question the method
actually needs answered: does this function match its source block.

## Decision

Before a transplanted function counts as done, it is compared **block by block** against
the mod's source block or the REA receipt:

1. Every row of the comparison is one construct of the mod's block: a condition, an early
   return, a constant, a clip or animation id, an enum id, a call, a write to a field.
2. Each row is classified as exactly one of:
   - `present`: the same construct exists in the port's block;
   - `translated`: it exists in the port's equivalent form (Gamepad API, GLSL, OpenAL,
     glfw/emscripten), with the equivalent named;
   - `excluded`: it is deliberately left out, only when an ADR, the plan or the player
     names the exclusion and its reason;
   - `gap`: it is missing and closes in a later batch.
3. A `gap` row is written in the plan with its closing batch and re-checked at every batch
   closure. A gap whose batch landed is either closed or re-declared with a new reason.
4. A port-invented value that the mod does not have is a row too, classified `excluded`
   with its reason: it must be a decision, never a left-over.
5. The table lives in the plan's Closure, or in `gta_vc_browser/tmp/rea/` when it comes
   from a receipt, and the closure cites it.
6. A feature with open `gap` rows is not "transplanted". Its DoD checkbox stays unchecked
   and the plan reports it as an accepted pending item.

## Consequences

- A half-transplant is caught the day it enters, not by a player session weeks later, and
  a faithful one is not "fixed" twice.
- Cost: minutes of bookkeeping per function, plus an honest `gap` list that grows before it
  shrinks. That list is the project's real transplant backlog.
- The mod's source stays the specification and the binary keeps winning ties (ADR-016).

## Alternatives Considered

- **Trust the traces and the served-build markers (the ADR-016 status quo)**: rejected —
  it is exactly what missed both the hybrid siren block and the invented save slot.
- **Compare by line count or textual diff**: rejected — the port legitimately rewrites
  Windows/D3D9/XInput/Miles pieces, so counts differ without anything being missing.
- **Enforce it only in review, without a written table**: rejected — the previous reviews
  approved both the hybrid block and the invented slot; the table is what makes the
  divergence visible.

## State

`proposed` (raised by the player on 23/10/2026 during plan 17.0; becomes `accepted` when
the player confirms it).

<!-- USER-START -->

## Local additions (preserved on update)

<!-- USER-END -->
