---
name: 011-one-clock-for-crouch-and-stand
status: accepted
date: 2026-10-06
domain: player-controls-aim-crouch
---

# ADR-011: one clock for crouch and stand

## Context

Standing up from the port's crouch was three separate events with three
different timings, measured in the `ctrl43` sessions:

1. **The body** faded out of the crouch clips with a hardcoded `blendDelta =
   -4.0f` over ~0.25 s (`ViceExtCrouchStopAnims`), while the engine brought the
   standing walk/idle in on its own clock. Two clips of the same kind crossed
   over with no shared budget, and the intermediate blend sums are what the
   player saw as two to four frames of a deformed model.
2. **The camera height** was binary: `TargetCoors.z -= VICEEXT_CROUCH_CAM_DROP`
   inside `if (ViceExtIsCrouched())`, in two places (`Cam.cpp`). The instant
   `odCrouched` flipped, the eye jumped to standing height while the body was
   still fully crouched: the player's "first he stays crouched in the air, the
   camera grows first, and only then does he stand up".
3. **The roll** cancelled by posture (`ViceExtCrouchRollFinish(nil, false,
   "postura")`) was cut off in mid-gesture with nothing returning the body to a
   standing pose, which is the lying-down screenshot from 05/10.

One event, three clocks, and the visible artefacts all come from that split.

## Decision

The crouch<->stand transition has **one clock**, and everything that depends on
it reads the same value.

- `CPlayerPed::ViceExtCrouchBlend()` returns 0..1 over
  `VICEEXT_CROUCH_BLEND_MS` (125 ms), from `s_odCrouchOnTime` when entering and
  from `s_odCrouchOffTime` when leaving. It is the only surface the rest of the
  engine interpolates with.
- The camera height subtracts `VICEEXT_CROUCH_CAM_DROP * ViceExtCrouchBlend()`
  instead of switching on a boolean, in both camera processes, so the eye
  descends and rises with the body.
- The crouch clips leave at the same rate the standing walk/idle enters
  (`-8.0f`, i.e. 125 ms, matching the engine's own `BlendAnimation(..., 8.0f)`),
  and their weight is additionally bounded by what is left free (`odLibre`, the
  weight the other moving animations do not already occupy, clamped to zero), so the
  crossing blend sums stay at or below one and no frame is deformed. The aiming pose
  leaves on the same clock.

## Consequences

- The camera and the pose can no longer disagree about when the transition
  happened: `CROUCH2 camz` must reach standing height within the `pose_exit`
  `tClear` window.
- The clock lives in `PlayerPed.cpp` next to `odCrouched`, not in `Cam.cpp`, so
  the camera keeps asking a question instead of owning the answer (the same
  shape as `ViceExtIsCrouched`).
- `VICEEXT_CROUCH_CAM_DROP` keeps its current magnitude (0.55, `config.h`); the
  stale comment in `Cam.cpp` that claims -0.95 is left alone, because the define
  is what the served build applies.
- Anything that later wants to move with the crouch (a HUD offset, a footstep
  filter) has an existing value to read instead of its own edge detector.

## Alternatives Considered

- **Interpolate the camera inside `Cam.cpp` from its own timer**: rejected —
  it would be a second clock for one event, which is the bug being fixed.
- **Speed up the crouch fade-out to hide the deformation**: rejected — it
  shortens the window without removing the two-clip crossover, and the frames
  would still be wrong at a different rate.
- **Raise `VICEEXT_CROUCH_CAM_DROP` to -0.95 (the stale comment)**: rejected —
  the comment is not what the served build applies; changing the magnitude
  would be a second change with no measurement behind it.
- **Return the body to a standing pose when a roll is cancelled by posture**: not
  needed — the roll clips (`ANIM_STD_CROUCH_LEFT/RIGHT`) are two of the nine
  associations `ViceExtCrouchStopAnims` already walks, so they leave on the same
  125 ms clock instead of staying pinned on their last frame.

## State

`accepted` (06/10, with the plan `4.4-desagachado-velocidad-pies-poses`; the code
reached `ctrl47`, where the bounded exit entered). The code is `ViceExtCrouchBlend()` in
`src/peds/PlayerPed.cpp`, its two call sites in `src/core/Cam.cpp` (`:1780`, `:6230`), and
the `-8.0f` plus the `odLibre` bound in `ViceExtCrouchStopAnims` (`PlayerPed.cpp:4821`).
What is missing is the measurement, not the code (`ADR-005`): `CROUCH2 camz` standing
still while crouched (±0.05 m; ±0.45 m when this decision was written), standing height
within the `pose_exit` `tClear` window, and the frames of the stand-up and of a roll
cancelled by posture are criteria 1-13 of plan 4.4, waiting for the player's session.
Today only the swim feature is validated end-to-end.
