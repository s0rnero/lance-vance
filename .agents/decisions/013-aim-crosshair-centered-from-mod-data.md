---
name: 013-aim-crosshair-centered-from-mod-data
status: accepted
date: 2026-10-10
domain: player-controls-aim-crouch
supersedes: []
---

# ADR-013: Aim crosshair centered from mod-owned data

## Context

While aiming, the on-screen crosshair is drawn at `SCREEN_WIDTH * m_f3rdPersonCHairMultX` by `SCREEN_HEIGHT * m_f3rdPersonCHairMultY`, with both multipliers owned by the ClassicAXIS INI section (`CameraCrosshairMultX/Y = 0.53/0.40`). In the browser session the reticle renders small, offset to one side and above center, while the reference session shows it large and centered. The same two multipliers also feed the aim-ray math (`Find3rdPersonCamTargetVector`, `ViceExtP4AimPointDir`, lock-on shifts), so the ray and the reticle agree with each other but both disagree with the screen center.

A second defect rides on the same HUD gate: the branch that draws the third-person reticle from the view alone (`DrawCrossHairPC && Using3rdPersonMouseCam()`) stays true when sprint kills the aim, so the reticle remains on screen while the aim itself is gone.

## Decision

1. The centered aim position is owned by the mod data, not hardcoded at the call site. Two new scalars join `CAimClassicAxisSettings`, next to `crosshairMultX/Y`: `aimCrosshairX` and `aimCrosshairY`, default `0.5/0.5`, read and stored as `ClassicAxis` INI keys `AimCrosshairX` / `AimCrosshairY`.
2. The third-person reticle (`Hud.cpp`) draws at the centered adjustment while the aim arbiter (`CPlayerPed::ViceExtIsAiming()`) holds, and at the legacy multipliers otherwise. The barrel-ray math keeps using the legacy multipliers, so ray and reticle stay consistent through the shared aim direction; only the HUD anchor moves to center.
3. The view-only HUD gate also asks the arbiter: the `DrawCrossHairPC && Using3rdPersonMouseCam()` branch now requires `ViceExtIsAiming()`, so sprint (which the arbiter vetoes) hides the reticle together with the aim.

## Consequences

- Positive: aiming shows a centered reticle by default while every other camera mode keeps the mod's `0.53/0.40` values untouched.
- Positive: sprint with the aim button held hides the reticle with the aim (zero reticles while sprinting); no second aim predicate is introduced.
- Negative: players who preferred the offset mod reticle must set `AimCrosshairX/Y` back to `0.53/0.40` in their INI.
- Pending measurement: reticle within 2 px of center with a 10 px margin against the reference, and zero reticles while sprinting, judged in the player's session (`P4 kind=hud_cross`, `AIMHUD`).

## Alternatives Considered

- Hardcode `0.5/0.5` at the HUD call site: rejected, it hides a tunable inside code instead of the mod-owned data the port reads.
- Change the `CameraCrosshairMultX/Y` defaults to `0.5/0.5`: rejected, those two values also drive the aim-ray and lock-on math, so every weapon's ray would move without a measurement behind it.
- Keep the view-only branch as-is and hide the reticle from the sprint state directly: rejected, sprint is one of several arbiter vetoes (vehicle, dead, swim, jump); asking the arbiter covers all of them through the single law.

## State

`accepted` as a design decision. Not verified in gameplay: `accepted` here means the decision is taken, not that the game behaves as intended.
