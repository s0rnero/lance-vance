---
name: 034-a-re-anchored-camera-seeds-its-angles-and-centering-is-two-axis
status: accepted
date: 2026-11-03
domain: camera
supersedes: []
---

# ADR-034: a re-anchored camera seeds its angles, and auto-centering covers pitch as well as yaw

## Context

The player reported two separate camera defects after session T101
(`gta_vc_browser/logs/odtrace-2026-11-03_04-37-26.log`):

1. "al iniciar el juego, cargar partida o iniciar partida nueva, o al finalizar
   una misión [...] el personaje o la cámara le entra un mareo, se empieza a
   mover de lado a lado".
2. "el centrado de la cámara en 3ª persona [...] solo centra en el eje X; si
   muevo la cámara en el eje Y no la vuelve a centrar, cuando sí debería
   hacerlo".

### Defect 1 — the on-foot camera inherits a memset state

`CCamera::Init()` runs `memset(this, 0, sizeof(CCamera))` and then `Cams[i].Init()`.
`CCam::Init()` deliberately leaves the angles neutral: `Beta = 0.0f`,
`Distance = 30.0f`, and the spring speeds at zero; `Front` and `Source` are the
zeros the memset left behind.

When the player is in a car, `Process_FollowCar_SA` rebuilds that state on the
frame it is needed (Cam.cpp, inside `if (ResetStatics || m_bCamDirectlyBehind ||
m_bCamDirectlyInFront)`): it assigns `Alpha`, `Beta = car->GetForward().Heading()
- HALFPI`, `Distance`, and recomputes `Front` and the two history points.

The on-foot path (`Process_FollowPedWithMouse`) had no equivalent. Its
`if (ResetStatics)` block only cleared `Rotating`, enabled collision checks and
cleared the mouse history. So the first frames on foot after a load, a new game
or a mission end ran with:

- `Beta = 0` while the ped faces any direction at all, so the camera sat on the
  world +X axis instead of behind the ped;
- `Alpha = 0` instead of the resting `-0.22 rad` of `fDefaultAlphaOrient`;
- `Source = (0,0,0)`, which the *same function* reads back the moment player
  control is disabled by `PLAYERCONTROL_PLAYERINFO`: it derives `BetaOffset` from
  `Atan2(Source - TargetCoors)`, i.e. from a world-origin camera position.
- `m_fAvoidTheGeometryProbsTimer = 0` (memset) and `Distance` too large, which
  feed the geometry-avoidance branch that inverts `DeltaBeta` when it disagrees
  with itself.

`log-check` output agrees: `PEDAT` samples at `velo=0.00` (ped not moving)
report the camera-to-ped planar distance alternating between ≈1.3 m and ≈3.6 m,
which is the camera swinging around a stationary ped, not the ped moving.

### Defect 2 — the centering spring only writes one angle

The auto-center block added under `VICEEXT_IMPROVED_MENU_AND_INPUT` in
`Process_FollowCar_SA` measures the yaw error against the car's heading and
drives a single `WellBufferMe` into `Beta`. Nothing ever writes `Alpha` back, so
pitch dragged upward with the mouse stayed where it was left.

The only existing pitch return in that function sits inside `if (correctAlpha)`,
and `correctAlpha` is true only when `car->GetModelIndex() == MI_VOODOO`
(the port kept the original guard). For every other vehicle that branch — and
therefore the `alphaCorrected`/`yMovement` return it contains — is dead code.
That is why yaw recenters and pitch never does.

## Decision

1. **A fresh camera is seeded, not interpolated.** `CCam` carries
   `m_bNeedsSeeding`, set in `CCam::Init()` and consumed once by
   `Process_FollowPedWithMouse`. On that frame it assigns
   `Beta = TargetOrientation + PI`, `Alpha = fDefaultAlphaOrient`, zeroes
   `BetaSpeed`/`AlphaSpeed`, and rebuilds `Distance`, `Front` and `Source` from
   them, so no code in the frame reads a zeroed `Source`. Ordinary re-anchors
   (mode change, look-behind) keep using `ResetStatics` alone and are not cut.
2. **Auto-centering is a two-axis spring.** The block now computes
   `odTargetAlpha = -zoomModeAlphaOffset` — the same resting pitch the game
   itself writes when you enter a vehicle (`Alpha = -zoomModeAlphaOffset`) and
   the same target the vanilla `correctAlpha` return uses — and applies a second
   `WellBufferMe` to `Alpha` with the same `0.15f`/`0.007f` constants as yaw.
   It is gated on `odAutoCentrar` only, **not** on `correctAlpha`, because that
   flag is the Voodoo exception and would make the feature dead for every other
   car.
3. **The engagement hysteresis considers both axes.** Centering starts when
   either |yaw error| or |pitch error| exceeds 3°, and stops when both fall
   below 2°, so a purely vertical drag engages the return instead of leaving the
   camera tilted forever.
4. **The `camauto2` trace carries the pitch axis** (`schema=2`:
   `erralpha=`, `alphagrado=`, `vely=`). A trace that reports only the axis it
   fixes cannot diagnose the axis it does not.
5. Free-look on foot remains untouched. Plan
   `camara-libre-autocentrado.md` was reverted on 19/09 with the note "no
   reintroducir sin preguntar"; this ADR does not reintroduce it — the player
   asked for the in-car pitch return in this session.

## Consequences

- Loads, new games and mission endings no longer start from a zeroed camera.
  The change is a one-frame cut instead of a damped approach; a cut is what the
  car path already does on entry, so the two paths now behave the same way.
- Pitch that the player drags up or down now returns to the zoom-mode resting
  angle after 1.5 s of idle while the car is moving forward, and the aiming
  camera (`bIsPlayerAiming`) still suppresses centering through `odMirando`.
- A spring that writes `Alpha` after the per-mode pitch clamps can in principle
  move pitch outside `[-CARCAM_SET[..][14], maxAlphaAllowed]`. The target is the
  resting pitch the game picks for that same zoom mode, and the clamps are
  re-evaluated the next frame, so an excursion is self-correcting; it is not
  bounded by construction.
- The Voodoo's `correctAlpha` path is unchanged and still runs alongside the new
  spring. Two returns for one vehicle were not merged because the second is
  driven by `yMovement` (it moves the input, not the angle) and the player's
  setup does not include the Voodoo mission set.
- `accepted` here means the decision is taken and it compiles. Neither the
  mareo nor the pitch return has been observed in a browser; the player is the
  one who tests camera feel.

## Alternatives considered

- **Seed on every `ResetStatics`.** Rejected: `ResetStatics` also fires on
  look-behind and mode changes, where the damped approach is the intended
  behaviour and a hard cut would be felt as a glitch.
- **Reuse `correctAlpha` as the gate for the new pitch spring.** Rejected: it is
  the MI_VOODOO exception, so the feature would never run for the cars the
  player actually drives.
- **Blend pitch back through `yMovement` instead of a spring.** Rejected: that
  writes the input channel, is scaled by `CARCAM_SET[..][12]`, and inherits the
  0.5 asymmetry applied to positive `yMovement` (look-up returns half as fast as
  look-down).
- **Raise `AlphaSpeed` limits.** Rejected: the complaint was "never returns", not
  "returns too slowly"; adding a spring keeps yaw and pitch under one clock and
  one set of constants.

## State

`accepted` (2026-11-03). Verification is the player's next session: `camauto2`
`schema=2` lines should show `erralpha=` converging to 0 while `vely=` decays,
and `PEDAT` should stop alternating camera distance at `velo=0.00` after a load.
