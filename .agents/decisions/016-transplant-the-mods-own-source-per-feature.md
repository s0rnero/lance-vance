---
name: 016-transplant-the-mods-own-source-per-feature
status: accepted
date: 2026-10-15
domain: mods
---

# ADR-016: Transplant the mod's own source per feature, verified against its binary

## Context

Plan 13.0 reimplements Vice City Extended from scratch on top of this reVC fork.
The port and the mod are the same engine lineage: this tree is reVC `a16fcd8d`
(2021-09-02) with changes on top, and the mod is the public fork
`x87/gta-extended` (branch `miami`, delta `37921eb0..44f4ab3`, 103 commits),
so up to 3.0 every mechanism can be read in source and transplanted instead of
deduced. What 2506/2510 added only exists in the installed `ViceEx.exe`
(`Ver. 2510`), and comes out of it with REA. The served `main.scm` is the
mod's 2510 script bit for bit, so the engine must honour its numeric contracts
(see ADR-017).

## Decision

For each row of the mod inventory (`.agents/plans/13.0-anexos/inventario-mod.md`):

1. **Read** its hunks in the mod source (`diff a16fcd8d 44f4ab3` or
   `show 44f4ab3:<file>` from `gta_vc_browser/tmp/upstream-gta-extended`).
2. **Contrast with `ViceEx.exe` (REA)** whenever the row carries values
   (constants, ranges, tables) or the source is ambiguous. On mismatch the
   binary wins. The receipt goes to `gta_vc_browser/tmp/rea/`.
3. **Remove the old code of that feature** in the same files: its old
   `VICEEXT_*` blocks, its unguarded lines and its traces
   (`.agents/plans/13.0-anexos/inventario-port.md`). Where the old code
   deleted vanilla, restore from `gta_vc_browser/tmp/revc-a16fcd8d/src`.
   Never touch the NADO, RADIO or INFRA-WEB rows.
4. **Transplant** with these rules:
   - Flags: each mod define becomes `VICEEXT_<name>`, declared in `config.h`
     in a block mirroring the mod's own `config.h:57-163` (umbrellas
     included), at the end of the `VICEEXT` block, reordering nothing.
   - Not transplanted: `SWIMMING` and `EX_RADIO_ICONS` (this port's swim and
     radio already own `VICEEXT_SWIMMING` / `VICEEXT_RADIO_ICONS`);
     `MODLOADER`, `VEHICLE_MODS`, `EX_RANDOM_VEHICLE_MODS_IN_TRAFFIC`
     (tuning, excluded); `NEW_MENU`, `EX_CLUMP_WEAPON_MODELS` /
     `EX_IMPROVED_WEAPONS`; languages, borderless window, the audio
     regression.
   - Compound conditions mixing the mod's `SWIMMING` term drop that term and
     keep the rest; this port's swim branch stays as is.
   - No comments (project rule). The mod's comments are dropped.
   - Paths: `ViceExtended\...` becomes whatever path this port serves; data
     keeps its current layout.
   - Port equivalents, never the original platform: Gamepad API instead of
     XInput, GLSL instead of D3D9, the glfw/emscripten skeleton, OpenAL.
   - The mod's own bugs are not copied (missing comma in `re3.cpp:461`,
     `SetWheelMissing(FRONT_LEFT)` with `CAR_PIECE_WING_RF`, the `Heli.cpp`
     assignment, the shadowing local in `AutoPilot.h`, the missing `break`
     in `GetPadState`).
5. **Minimal measurement.** At most one trace per family (`VEX <family> ...`,
   one line per state change, under `#ifdef __EMSCRIPTEN__`) and one mark in
   `check-served-build.sh`. Marks and verifier blocks of removed code go away
   in the same step.

## Consequences

- Every ported behaviour is traceable to a source hunk or a binary receipt,
  never to a guess.
- The old implementation and the new one never coexist for the same feature.
- ADRs 004, 007, 008, 009, 010, 011, 012 and 013 are superseded as their
  tanda lands (each tanda marks them); ADRs 001, 002, 003, 005, 006 and 015
  stay in force.

## Alternatives Considered

- **Deduce the mechanism from the changelog and videos**: rejected, it is
  what produced the current half-working tree.
- **Remove all old code first, then port**: rejected, it would leave the game
  unplayable for several sessions; removal rides with each feature's tanda.

## State

`accepted` (2026-10-15, with plan `13.0-reimplementacion-limpia-extended`, T0).
