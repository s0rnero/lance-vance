---
name: 012-one-camera-angle-convention-for-aim-and-move
status: accepted
date: 2026-10-08
domain: player-controls-aim-move-crouch
supersedes: []
---

# ADR-012: One camera-angle convention and one move vector per frame

## Context

The follow law and the aim law of the ClassicAXIS camera describe the same
camera with opposite conventions for `Beta`:

- Follow: visible forward is `-(cos Beta, sin Beta)`.
- Aim: visible forward is `+(cos Beta, sin Beta)`.

Saving `cam.Beta` raw when entering aim therefore flipped the view by 180°.
Movement direction was also decided in several places at once: the camera
matrix forward for the move vector, `TheCamera.Orientation` for the crouch
fallback, and the body angle (`walkAngle`) in the crouch back-up path. Those
bases disagree, which explains the reported errors on S and diagonals.

## Decision

1. Angle conversion between the two laws goes through one helper,
   `CCamera::ViceExtAimBetaFromFollowBeta`, which converts via the visible
   forward vector. Entering aim stores the converted value in
   `CCamera::s_viceExtAimPrevHor`, which is already in aim convention.
2. Walking, crouching and the crouch side heading read the per-frame move
   vector built once in `ViceExtPrepareMove` (camera matrix forward basis).
   `CPlayerPed::ViceExtStickHeading` gives the heading of a raw pad input in
   that same basis, and the crouch fallback uses it instead of the camera
   `Orientation` or the body angle.
3. Jump (`SetJump`, `FinishLaunchCB`) and the crouch roll read the same move
   vector and add no direction of their own.

## Consequences

- Positive: one basis for every move consumer; entering aim has no 180° flip
  in the math (checked: the aim front equals the follow front, and the
  round trip is exact for 0°, 45°, 90°, 180°, -90° and -135°).
- Negative: the crouch fallback changes basis when the stick is below the
  move threshold, so its exact on-screen result is only measured in a real
  player session. Until then the criteria in plan 6.0 §8 stay pending.
- Positive: `walkAngle` (body + a) is no longer used for crouch movement.

## Alternatives Considered

- Keep the body angle for crouch and only fix the aim flip: rejected, it keeps
  two bases and the S/diagonal errors.
- Convert every consumer to `TheCamera.Orientation`: rejected, the move vector
  already uses the camera matrix forward and the mod's own controls are built
  on it.

## State

`accepted` as a design decision. Not verified in gameplay: `accepted` here
means the decision is taken, not that the game behaves as intended.
