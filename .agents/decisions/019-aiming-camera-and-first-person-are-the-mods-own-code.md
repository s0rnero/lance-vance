---
name: 019-aiming-camera-and-first-person-are-the-mods-own-code
status: accepted
date: 2026-10-15
domain: player-controls-aim-crouch
supersedes: [004, 009, 010, 012, 013]
---

# ADR-019: Aiming, aiming camera, first person and drive-by are the mod's own code

## Context

The old port did not port Vice Extended's aiming. It reimplemented the aiming law of another
mod (ClassicAXIS) on top of the engine, mostly without a guard: a `MODE_AIMING` camera with its
own collision solver (`Process_AimWeapon`, `Process_AvoidCollisions`), a per-frame player
control loop (`ViceExtProcessPlayerPedControl`), a movement law that owned the ped heading
(`ViceExtPrepareMove`/`ViceExtGetMove`), a single "am I aiming?" API (`ViceExtIsAiming`,
`ADR-004`), per-family laws (`ADR-010`), one angle convention for camera and movement
(`ADR-012`), a crosshair driven by ClassicAXIS settings (`ADR-013`) and the arm pose from the
"petition" (`ADR-009`). The car camera (`Process_Cam_On_A_String`) was rewritten and
`Process_FollowCar_SA` emptied. First person was a V-key toggle to `MODE_1STPERSON_RUNABOUT`
with its own arbiter lane (`PEDLANE_LEY`); recoil was a queue of camera kicks; drive-by,
rocket third person and weapon sights were separate inventions.

Plan 13.0 (`ADR-016`) transplants each feature from the mod's own source (`x87/gta-extended`,
44f4ab3). In the mod, aiming is one flag, `CPlayerPed::bIsPlayerAiming`, set in
`ProcessPlayerWeapon` while the target button is held; everything else (GunMove strafing,
smooth FOV and shoulder offset in `Process_FollowPedWithMouse`, `Process_FollowPed_Rotation` and
`Process_FollowCar_SA`, vertical limits, drive-by in the three `DoDriveByShootings`, the
`_DB` animations, recoil on `Alpha/Beta`, sights by `weapon.dat` column) reads that flag.
First person is a camera mode of its own (`MODE_REAL_1ST_PERSON`,
`Process_Real_1st_Person`) reached by cycling the camera, with a menu page of options.

## Decision

The port's aiming, aiming camera, first person, drive-by, recoil, weapon sights and rocket
third-person aiming are the mod's code, under `VICEEXT_FIRING_AND_AIMING`,
`VICEEXT_FIRST_PERSON`, `VICEEXT_WEAPON_SIGHT` and `VICEEXT_ROCKET_LAUNCHER_THIRD_PERSON_AIM`
(T4). The old flags (`AIM_CLASSICAXIS`, `AIM_WALK`, `AIM_BLADE`, `SHOTGUN_AIM`,
`ROCKET_3RD_PERSON`, `RECOIL`, `WEAPON_SIGHTS`, the old `FIRST_PERSON`, `DRIVEBY_WIDE`,
`SPRINT_HEAVY`), their unguarded laws, traces and verifier marks are removed in the same
batch; `Cam.cpp` and `Camera.cpp/.h` are rebuilt from vanilla plus the mod.

- **`ADR-004` is superseded.** The aiming state is `bIsPlayerAiming`; `ViceExtIsAiming()`
  no longer exists.
- **`ADR-009`, `ADR-010`, `ADR-012` and `ADR-013` are superseded.** Movement while aiming is
  `ProcessAiming` (GunMove clips) and the vanilla heading code; the camera is the vanilla
  follow cameras with the mod's FOV/offset/limits; the crosshair is the vanilla one with the
  mod's multipliers (0.51/0.49) and sights. No ClassicAXIS settings remain.
- `PEDLANE_LEY` is removed from the arbiter (it was the last lane, so no lane renumbers).
- The port's swimming keeps priority: aiming is dropped while the swim lane owns
  `PEDCAP_APUNTAR` (the port's equivalent of the mod's `ProcessSwimming` reset), and the
  mod's `bIsSwimming` branches in camera limits are not transplanted.

## Consequences

- One aiming mechanism, the mod's. Every T4 change sits behind its flag with the vanilla code
  in the `#else` branch.
- `Process_Cam_On_A_String` is vanilla again and, with `bFreeCam` on by default
  (`VICEEXT_IMPROVED_MENU_AND_INPUT`), the car camera is `Process_FollowCar_SA` as in the mod.
- `ped reaction to aiming` (row 7.14) depends on the wanted system and is left for T7.
