---
name: 057-a-node-without-a-measured-amber-row-never-gets-indicator-ambient
status: proposed
date: 2026-11-09
domain: reVC-WASM-intermitentes-vehiculos
supersedes: []
---

# ADR-057: a node without a measured amber row never gets indicator ambient

## Context

`ViceExtIndicatorAtomicCB` raised `surfaceProps.ambient` to 10,0 for every
non-body material of an `indicator_*` node whenever that side's indicator was on —
whether or not the table measured an amber lens in that node. The node path had no
gate at all; only the colour write and the atlas swap consulted the row.

The player's 62.0 pass showed what that costs on the models where the glass lives
somewhere else:

| case | what the node actually holds | amber row | what he saw |
|---|---|---|---|
| mrwhoop `indicator_lf`/`indicator_rf` | the **white headlight** | no (its front lens is a `chassis_hi` body row) | "it still paints its white headlights yellow" |
| rumpo/pony/topfun `indicator_lr`/`indicator_rr` | 1 glass face (56 % amber) + 1 housing face (0 %) | no — the 62.0 deleted the invented rear body rows | the 220-246 cm² housing lit whole: "opaque triangles at the rear", orange where he asked for the red stop |

This is the same rule the project already states for the corona — "sin fila no hay
destello" — applied to the material path.

## Decision

`ViceExtSetIndicatorMaterial` returns before touching any material unless the
resolved cell's kind is `ODIND_AMBER`. `ODIND_OWN` (the node carries the model's
own headlight or tail light) and `ODIND_NONE` are left completely alone: those
sides already blink through `ViceExtSetLightMaterial`, which is the mod's Banshee
route.

Measured before deciding: across the whole served fleet, **every** model that has
`indicator_*` frames also has lamp frames (`headlight_*` / `taillight_*` /
`brakelight_*`), and **0** bikes have indicator nodes, so no vehicle loses its
signal by this gate.

## Consequences

- Positive: one rule removes two of the three defects the player reported; the
  yellow headlight and the orange rear housing are the same bug.
- Positive: the doctrine becomes uniform — a light only appears where a measured
  row says it does.
- Negative: behaviour changes for every side of every model that has an indicator
  node without an amber row. The measurement says they keep signalling through
  their lamp, but only the player's session can confirm it. If any car stops
  signalling, the gate is reverted and the case is handled per model.
- Neutral: models whose node rows are amber (mrwhoop rear, washing, boxville rear,
  gangbur, pony/rumpo/topfun front) are untouched by this change.

## Alternatives Considered

- **Raise ambient only for the materials in the row mask**: does not help — the
  mrwhoop front node has no row at all, so `mats` is empty and the fallback
  (`lens = ~0u`) still lights the headlight.
- **Declare the rear nodes with `amber = 0`**: writes a row that contradicts the
  measurement (those faces are 56 % amber). Invention in the other direction.
- **Delete the rear nodes' geometry**: forbidden — zero geometry changes.

## State

`proposed` -> `accepted` once the player confirms on `2026-11-09-62.1-T1` that the
mrwhoop headlights stay white, rumpo/pony/topfun rear is red, and nothing stopped
signalling.
