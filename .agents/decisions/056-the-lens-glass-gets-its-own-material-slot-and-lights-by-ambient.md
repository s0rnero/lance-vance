---
name: 056-the-lens-glass-gets-its-own-material-slot-and-lights-by-ambient
status: proposed
date: 2026-11-09
domain: reVC-WASM-intermitentes-vehiculos
supersedes: []
---

# ADR-056: the indicator glass gets its own material slot and lights by ambient

## Context

The player asked twice for "the light must cover the whole face, not a line of
colour". Measured on the served DFF (plan 62.0, `lens_footprint.py`), a lens face
is not one triangle: in rumpo the amber rectangle is made of 4 faces of ~44 cm² —
one inside the `indicator_lf` node (56 % amber texels) and three in `chassis_hi`,
split between the body-paint material (mat0) and the material that also holds the
two rear faces of the same side (mat5).

Neither of the two routes that existed could handle that:

1. **The derived atlas** (`<tex>_lens`) paints texture texels. Measured: in
   mrwhoop the derived texture changed **1180 texels** over a 32-row band spanning
   the whole atlas width, while the real lens occupies 186 texels; in fbiranch
   `fbirancher86body8bit64_lens` changed **1998 texels = 49 % of the atlas** with
   no face of that texture measuring amber at all. Because the material is shared,
   the texture swap applied to the whole paintwork: that is the yellowing white
   headlight and red tail light the player reported.
2. **Tinting by material colour** (`RpMaterialSetColor`) multiplies: over a texel
   drawn dark it cannot brighten anything. And the 59.0 `tint` gate required
   *every* triangle of the material to fall within 0.35 m of *some* declared
   centre, so for the front to pass, the **rear had to be declared** (rumpo/pony/
   topfun `LR/RR` rows of 438-482 cm²) — an invention that blinked the red stop as
   a turn signal, outside the frozen 59.0 list.

## Decision

1. **The glass is split into its own material slot.**
   `split_lens_material.py --mode cristal` takes, for each declared row, the amber
   faces (`is_lens_face`, or ≥ 20 % amber texels) within 0,12 m of the row's `xyz`
   and gives them a new material — a byte-for-byte copy of the original. Triangles
   that are not glass stay in their slot. One slot per quadrant (LF/RF/LR/RR), so
   the front and the rear of a side stop blinking together. No vertex moves, no
   node is created, no shape changes: the gate is delta of vertices 0.0, identical
   world-triangle union and byte-identical OFF.
2. **The body route lights by ambient, exactly like the node route.** With an
   exclusive slot, `ViceExtIndicatorAtomicCB` raises `surfaceProps.ambient` to
   10,0 (on) / 1,0 (off) when the row carries `tint`, instead of only multiplying
   the colour. That is what lifts the half of the glass the modeller drew in dark
   amber (measured: 21-30 % amber texels).
3. **Anchor to the declared row, not to colour.** What gets split is the table the
   player approved, not a hue threshold over the whole fleet. This is what forbids
   inventing: mule has amber faces at the rear with no declared row, so it is not
   split.
4. **The derived atlas stays only for what cannot be split** (mule: its lens is a
   sub-region of one 462 cm² face; splitting its material would light the panel).

## Refinamiento medido en el 62.2 (mismo dia, antes de aceptar)

La primera version de `--mode cristal` decidia "es cristal" por la **fraccion de
texelas ambar** de la cara (>= 20 %). Eso depende del brillo con el que esta
pintada la lente, no de si es lente: perdio las caras oscuras del rectangulo de
pony/topfun (saturacion 0,85, color (95,41,14)) y metio una cara de bajos gris
(saturacion 0,22, 1034,5 cm2). La regla queda en matiz 15..55 **y** saturacion
>= 0,60, medida sobre el color renderizado de la cara, y vive en un unico sitio
(`is_any_lens_face` en `indicator_lens45.py`).

## Consequences

- Positive: mrwhoop, rumpo, pony, topfun, boxville, esperant, fbiranch and fbicar
  all resolve `EXCL n/0 tint=1` in the offline replay of the engine test; the red
  stop and the white headlight no longer share a material with the glass, so there
  is no path left that can yellow them.
- Positive: the same mechanism declares the FBI Washington (`fbicar`) at the
  `washing` sites, which is the player's chassis rule.
- Negative: the `mats` column is no longer reproducible by `gen_body_lens.py` (the
  generator does not know the split slots). Regenerating the table without the
  cristal mode erases the split rows. Declared as pending in the plan.
- Negative: two DFFs grow one sector each (esperant, pony) and the `.dir` is
  repacked.
- Risk: "glass face" is still a judgement over texels. If the player's eye rejects
  one, it is reverted row by row in the table, not in the engine.

## Alternatives Considered

- **Paint the atlas band (59.0)**: rejected by measurement — 1180 and 1998 texels
  outside the lens, with the paintwork and the underbody among them.
- **Lower the detector's amber threshold to 20 % and tint the shared material**:
  would light the bonnet and the underbody; the invention he rejected twice.
- **Raise the ambient without splitting**: illuminates the whole material,
  including the paint panel that shares it.
- **Cut the mule face into two triangles**: new geometry, out of scope.

## State

`proposed` -> `accepted` once the player signs the in-game pass (T6 of plan 62.0).
Shipped as `2026-11-09-62.0-T1`.
