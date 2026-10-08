---
name: 010-family-laws-instead-of-per-weapon-values
status: accepted
date: 2026-10-06
domain: player-controls-aim-crouch
---

# ADR-010: family laws instead of per-weapon values

## Context

Two port values were being derived from data that made the same family of things
behave differently for no visible reason.

1. The crouch walking speed was taken from the pad magnitude:
   `ViceExtWalkerTargetMps()` scaled `|stick| / PAD_MOVE_TO_GAME_WORLD_MOVE` by
   `VICEEXT_WALK_MPS_PER_MFS`, so the crouch moved at **2,3918 m/s** with the key
   held (and 2,43 m/s on a diagonal) instead of the **1,13 m/s** the family
   declares and the player asked for. Pies and advance moved together because both
   came from that same number, which is why the movement looked consistent and
   still wrong. Writing the constant in `ViceExtCrouchWalkMps()` was not enough on
   its own: while crouched, the value the engine applies is `s_odMove.speedGame`,
   and `CPed::CalculateNewVelocity` overwrites the crouch `odSpeed` with it whenever
   `odCSpeed > 0.01` (`Ped.cpp:1608-1611`), so the pad kept winning.
2. The aiming FOV narrowing (`taken`) was decided per weapon by
   `m_fRange >= AIM_WEP_MIN_RANGE` (70). Measured in the `ctrl43` session: the
   STEYR (54, range 90) narrowed to `fov=50`, while its own class siblings
   AK47 (52) and M16 (53) did not, and the light pistols never did. The player
   states the baseline is **per weapon class** (a beretta is light, an AK/M16 is
   heavy, melee is its own class), not per weapon.

## Decision

The value of a family comes from a **law of the family**, never from the input
magnitude and never from the per-weapon data alone.

- **Speed by family**: crouch walking returns the constant
  `VICEEXT_CROUCH_WALK_MPS` (1,13 m/s) regardless of how hard the stick is
  pushed, and that number is what the engine applies: while crouched,
  `s_odMove.speedGame` comes from the family (`PlayerPed.cpp:1502`). The roll keeps
  its own `VICEEXT_CROUCH_ROLL_MPS` (2,35). The magnitude-driven value stays where it
  belongs (standing `speedGame`).
- **Zoom by class**: the FOV narrowing is true only for weapons of the heavy
  class. The class predicate lives in one place, `ViceExtAimHeavy()`
  (`src/peds/PlayerPed.h`, next to `ViceExtCanAim`), and all three sites that
  decided it now call it: `CCam::Process_AimWeapon` FOV lerp, the `AIMFOV`
  trace, and `ViceExtP4CamFovTarget`. The previous threshold is kept as an
  additional pass so no vanilla weapon changes behaviour; the class predicate
  adds the mod's heavy rifles that the threshold was dropping.

## Consequences

- The crouch advances at the family speed and its feet cadence follows from it
  (`ViceExtCrouchRateFor`), so feet and advance stay locked to each other. Measured on
  real sessions: `ctrl43` (before the crouched `speedGame` came from the family)
  displaced 2,39 m/s; `ctrl45`/`ctrl47` displace 1,10-1,12 m/s.
- All heavy rifles narrow the FOV identically, which is what can be compared
  against the reference video; a per-weapon threshold can no longer make two
  siblings of the same class disagree.
- The class predicate is a small explicit switch: adding a heavy weapon means
  adding its type there, in one file.
- The measured values of the weapons that already had samples (STEYR narrows;
  escopetas, deagle, colt45 and uziold do not) are unchanged; only the two
  unmeasured heavy rifles change.

## Alternatives Considered

- **Keep the magnitude-driven crouch speed**: rejected — the player asked for the
  same speed as normal walking, and the constant already existed with that name.
- **Keep the per-weapon `m_fRange >= 70` rule and treat it as a class**: rejected —
  it is the data that made AK47/M16 differ from the STEYR while being the same
  class, which is the symptom.
- **Make the class predicate purely range-based (lower the threshold)**: rejected —
  it would silently pull light and SMG weapons into the zoom, changing measured
  behaviour.
- **Put the class predicate in `Cam.cpp` next to the threshold**: rejected — the
  aiming law already lives in `PlayerPed.h` for exactly this reason (two copies
  were the bug fixed by the `apuntado-classicaxis-100` plan).

## State

`accepted` (06/10, with the plan `4.4-desagachado-velocidad-pies-poses`; the code
continued through `ctrl45` and `ctrl47`). The code is `ViceExtCrouchWalkMps()` and
`ViceExtAimHeavy()`, plus the crouched `speedGame` of `PlayerPed.cpp:1502`. What is
missing is the measurement, not the code (`ADR-005`): the player's session has to show
`P4 kind=crouchspeed mps=1.1300` with the key held and `AIMFOV taken=1` for the heavy
rifles that had no sample; the zoom-by-class comparison is criteria 1-13 of plan 4.4 and
is still unmeasured, and today only the swim feature is validated end-to-end.
