---
name: 009-aim-movement-and-pose-come-from-the-mod
status: accepted
date: 2026-10-06
domain: player-controls-aim-crouch
refines: ADR-004
---

# ADR-009: While the player aims, the movement and the arm pose come from the petition, not from the engine's clip

## Context

Two behaviours of the on-foot player were owned by the base engine and are now owned
by the port, and in both cases the engine's answer contradicts the behaviour the
player asked for (plan `4.3`, session of 05/10 and 06/10 on build
`2026-10-05-ctrl42`).

**The movement.** `CPed::CalculateNewVelocity` (`src/peds/Ped.cpp`) has an early
return that hands the ped's displacement to the animation machinery:

```
	if (!odCrouchMove
	    && ((!TheCamera.Cams[TheCamera.ActiveCam].GetWeaponFirstPersonOn() && !TheCamera.Cams[0].Using3rdPersonMouseCam())
		|| FindPlayerPed() != this || !CanStrafeOrMouseControl()))
		return;
```

While the player aims, the camera is in its own aiming mode, so the return fires and
the block below it — the one that applies the *published* direction and magnitude
(`ViceExtGetMove`) — never runs. Measured on the real session (`web/odtrace.log`,
06/10): the 43 standing-aim spans of the log contain **zero** `move_apply` lines, and
the ped's actual displacement is `dirErrMax` 83°–180° away from the pressed
direction, because the displacement is then the root translation of whichever strafe
clip the engine picked (the player's report: *"cualquier tecla WASD mueve al
personaje siempre hacia la derecha"*). The same early return was already bypassed for
the port's crouch (comment `R21b`, `odCrouchMove`), for exactly the same reason: with
the mod's crouch the camera changes mode and the block did not run.

**The pose.** The 27/09 decision recorded in `src/peds/PedFight.cpp` (`R29`,
"the crouch aim pose is the mod's `WEAPON_crouch`") removed the weapon's own clip from
the aiming pose on purpose, because holding `*_crouchfire` in the *aim* frame had
been reported as odd in an earlier round. The player now asks for the opposite (plan
`4.3` §9): aiming without the trigger must show the weapon's aim pose —
`*_crouchfire` crouched, `*_fire` standing — and the trigger may only add the shot.
With a Deagle the crouched pose the player sees is `ANIM_STD_DUCK_WEAPON`, the generic
"crouch holding a weapon" partial, which he reads as a knife pose.

## Decision

**While the player is aiming, the on-foot displacement is decided by the petition
published by the port, and the arms pose is the weapon's own clip.**

1. The engine's early return of `CalculateNewVelocity` does not apply to the player
   ped while the port's aim arbiter (`CPlayerPed::ViceExtIsAiming`, ADR-004) says the
   player is aiming, nor while the port's crouch owns the stance, nor in the
   first-person weapon mode (where nobody moves). The block that applies the
   published direction and magnitude runs in those cases.
2. The magnitude published while aiming is the petition's (`ViceExtWalkerTargetMps`
   in engine units), not the clip's root translation: *the clip puts the pose, the
   code puts the speed and the direction*. This is the same model the crouch already
   used (`Ped.cpp`, `R22`).
3. The crouched aiming pose is the weapon's own clip, held in its aim frame:
   `*_crouchfire` when the weapon carries `WEAPONFLAG_CROUCHFIRE`, otherwise the
   weapon's fire clip (`*_fire`) when it carries `WEAPONFLAG_CANAIM_WITHARM`, and
   `ANIM_STD_DUCK_WEAPON` when it carries neither. That fallback is right for melee and
   **wrong for pistols**: the deagle (49) carries `witharm=0` and lands on the generic
   crouch partial, which the player reads as a knife pose (open as J8 in plan 4.4 §14).
   Closing it needs a class predicate for weapons (light / heavy / melee), the same shape
   as `ADR-010`'s `ViceExtAimHeavy()`. This **supersedes the 27/09 `R29` decision** of
   `PedFight.cpp` for the aim-without-trigger case; the trigger still adds the shot, and
   the reload keeps its own clips.

`ADR-004` is untouched: the arbiter is still the only owner of "am I aiming", and the
gate of `ADR-009` item 1 asks it, it does not re-derive it. The sprint and jump veto
that falls the aim (`PlayerPed.cpp`, arbiter) and the camera law (`Cam.cpp`) also stay
in the arbiter.

## Consequences

- The engine's clip stops deciding where the player walks while aiming: the axis the
  mod publishes (the port of `playerMovementType`) is the one that reaches
  `m_moved`, which is what makes strafing possible at all. The player's four
  directions stop depending on which strafe clip the locomotion selector chose.
- The same bypass fixes the crouched straight lines the player could not walk, and
  it is what lets the crouch roll advance instead of returning to its origin: during
  the roll the port publishes the roll's direction and speed, and those are the ones
  applied.
- Melee keeps the generic `ANIM_STD_DUCK_WEAPON` partial on purpose; weapons with their
  own crouch-fire clip aim with it, and weapons with `WEAPONFLAG_CANAIM_WITHARM` aim with
  their fire clip. Pistols with neither flag are the open defect above, not a deliberate
  exception.
- Two behaviours that the base engine owned while aiming are now the port's, so the
  next round of the player's log (`P4 kind=move_apply` with `aim=`/`crouch=`, `P4
  kind=armsight` with the applied clip name) is the only way to judge them, and both
  traces are part of this decision's evidence.

## Alternatives Considered

- **Let the engine's clip move the ped and only fix the magnitude** (publish a speed
  and keep the return): rejected — the direction is the clip's, so the ped would walk
  where the animation points, which is the reported defect, not the fix.
- **Force `CanStrafeOrMouseControl` to true / change `Using3rdPersonMouseCam`**:
  rejected — those two predicates guard seven other sites (camera orientation,
  anim groups, the HUD), and the aim case is not what they are asking about.
- **Keep the engine's `bIsDucking` for the crouch instead of the port's crouch**:
  rejected earlier (R6) and re-confirmed: `bIsDucking` pins the player in place.
- **Keep `WEAPON_crouch` for weapons without `WEAPONFLAG_CROUCHFIRE`** (the 27/09
  rule taken literally): rejected — the reported symptom *is* that pose with a
  firearm (a Deagle), so the fallback has to be a firearm pose, not the generic
  crouch partial.

## State

`accepted` (06/10, with the plan `4.3-apuntado-movimiento-agachado-ruedo`; the code
continued to `ctrl47`, where the `armsight`/`canon` instrumentation of this decision's
evidence was added). The bypass lives in `CPed::CalculateNewVelocity` (`odAimMove`, next
to `odCrouchMove`), the published magnitude in `CPlayerPed::ViceExtPrepareMove`, and the
pose in `ViceExtCrouchAimPose`/`ViceExtCrouchAimPoseClip` with its hold frame. What is
missing is the measurement, not the code: the player's session is the only judge
(`ADR-005`), the evidence being `P4 kind=aim`, `P4 kind=move_apply` and `P4 kind=armsight`
in his log, and today only the swim feature has passed that criterion.
