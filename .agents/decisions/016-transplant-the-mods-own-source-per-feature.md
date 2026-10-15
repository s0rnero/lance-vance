---
name: 016-transplant-the-mods-own-source-per-feature
status: accepted
date: 2026-10-15
domain: mods
supersedes: []
---

# ADR-016: Port Vice City Extended by transplanting the mod's own source, feature by feature

## Context

Plan 12.0 ported Vice City Extended by reading its changelog and reconstructing each
mechanism by hand. The player's verdict on 14/10/2026 is that, apart from swimming and the
radio icons, none of that work holds up in play. Plan 13.0 replaces it.

Two measured facts change the method:

1. The mod is itself a reVC fork with public source (`x87/gta-extended`, branch `miami`).
   Its net delta is `37921eb0..44f4ab3` (v3.0, 2024-07-20). This port's `src/` is reVC
   `a16fcd8d` plus local changes. Both sit on the same engine, so up to v3.0 every
   mechanism can be read and transplanted instead of guessed.
2. The installed binary is `Ver. 2510`. What 2506 and 2510 added exists only in
   `ViceEx.exe` and has to be recovered with REA (Ghidra).

## Decision

For each row of the mod inventory (`plans/13.0-anexos/inventario-mod.md`):

1. **Read** the row's hunks in the mod source (`git diff a16fcd8d 44f4ab3 -- <files>`).
2. **Check against `ViceEx.exe` with REA** whenever the row carries values (constants,
   ranges, tables) or the source is ambiguous. **The binary wins** on mismatch. The receipt
   goes to `gta_vc_browser/tmp/rea/`.
3. **Remove the old code of that feature** in the same step: its old `VICEEXT_*` blocks,
   its unguarded lines and its traces. Where old code deleted vanilla, vanilla is restored
   from `gta_vc_browser/tmp/revc-a16fcd8d/src`. The swimming, radio and web-infrastructure
   rows are never touched.
4. **Transplant** behind `VICEEXT_<mod define>` flags declared in `config.h` (same tree as
   the mod's `config.h:57-163`, appended at the end of the `VICEEXT` block). The flags for
   modloader, vehicle tuning, languages, borderless window and the mod's audio regression
   are not transplanted. Mod comments are dropped. Windows, D3D9, XInput and Miles pieces
   use the port's equivalents (Gamepad API, GLSL, glfw/emscripten skeleton, OpenAL). Known
   bugs of the mod (`inventario-mod.md` §7) are not copied.
5. **Minimal measurement:** at most one trace per family (`VEX <family> ...`, under
   `__EMSCRIPTEN__`) and one marker in `check-served-build.sh`.

With every flag off the game is vanilla.

ADRs 004, 007, 008, 009, 010, 011, 012 and 013 describe mechanisms of the old port. Each one
is marked `superseded` by this ADR in the batch (tanda) that replaces its feature.
ADRs 001, 002, 003, 005, 006 and 015 stay in force.

## Consequences

- No mechanism is invented: what is neither in the source nor in the binary is asked.
- Old and new code of a feature never coexist; the swap is per feature, not big-bang.
- Large volume (≈26,800 mod lines in, ≈16,000 old lines out): several sessions, one
  playable build per batch, and nothing counts as done without the player's verdict.
- Old saves made with the port may stop loading as enums move to the mod's layout.

## Alternatives Considered

- Keep fixing the plan 12.0 code: rejected by the player, the code is not trusted.
- Rebuild from the changelog plus REA only, ignoring the public source: slower and more
  error-prone, since up to v3.0 the source answers directly.
- Load the original binary or ASI plugins: impossible in WASM (ADR-001).

## State

`accepted` (decided by the player on 14/10/2026, plan 13.0).

<!-- USER-START -->

## Local additions (preserved on update)

<!-- USER-END -->
