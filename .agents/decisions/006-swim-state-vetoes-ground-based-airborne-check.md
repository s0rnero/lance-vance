---
name: 006-swim-state-vetoes-ground-based-airborne-check
status: accepted
date: 2026-09-29
domain: gameplay-swim
---

# ADR-006: The swim state vetoes the engine's ground-based airborne check

## Context

`CPed::ProcessControl` decides the fall state of any ped with one question, and that
question only looks at the ground: `CheckIfInTheAir()` (`src/peds/Ped.cpp`) casts a
vertical ray about 4 m down and, finding nothing, reports "in the air". The engine has
no notion of a swim state, so a player floating in deep water — where there is no floor
within that ray — was re-classified as **falling on every frame**: `SetInTheAir()`
blended `ANIM_STD_FALL_GLIDE` and `InTheAir()` blended `ANIM_STD_FALL`.

The ported swim feature (`CPlayerPed::ViceExtSwimControl`, `src/peds/PlayerPed.cpp`)
runs later in the same frame from `CPlayerPed::ProcessControl`, blended its own swim
clip and cleared `bIsInTheAir`, but that is *after* the fall animations were already
blended. The result was a ped that moved correctly at chest depth while wearing a fall
pose, and the swim clip only appeared where the sea floor was close enough for
`CheckIfInTheAir()` to answer "there is ground". Three iterations (1.1-1.3) tried to
solve it with vertical pins, buoyancy tuning and per-frame animation fading — all of
which moved the *body* and could never touch the *pose*.

Two instrumentation blind spots hid it: the `fall=` trace field measured only
`ANIM_STD_FALL` and never `ANIM_STD_FALL_GLIDE` (the one actually responsible), and the
`aire=` field was sampled after the swim code had already cleared the flag, so it
reported `0` by construction.

## Decision

A ped that is swimming is **not** in the air, and that veto is applied at the engine's
single airborne decision point:

```cpp
} else if (!CPlayerPed::ViceExtIsSwimming() && CheckIfInTheAir()) {
```

(`src/peds/Ped.cpp`, inside `CPed::ProcessControl`). `SetInTheAir()`, `InTheAir()` and
`CheckIfInTheAir()` themselves are **not** modified.

Two consequences are treated as part of the same rule, not as separate features:

- **A state, not a patch.** A mod feature that owns a ped state must veto the engine's
  conflicting state classification where the engine *decides* it, never by compensating
  downstream (velocity pins, per-frame animation fading, blend wars).
- **A trace must measure the state it claims to measure.** When a feature asserts
  something about a system state, the trace field has to sample it *before* the feature's
  own code resets it, and has to cover every animation of the family it blames.

## Consequences

- `FALL_GLIDE` and `FALL` are never blended while swimming, so the swim clip is the only
  pose; `InTheAir()` becomes a no-op because `bIsInTheAir` is never set.
- The fix is one condition on one line, with no tuning and no per-frame cost.
- The ported swim feature now reaches into a core engine decision point
  (`CPed::ProcessControl`). That is the accepted price of `ADR-001` (port, do not load):
  a WASM build cannot hook, so state arbitration happens in the engine's own code.
- Any future mod feature that owns a ped state (climb, ragdoll, vehicle entry) must
  apply the same veto at the same decision point, or it will show the same class of bug.

## Alternatives Considered

- **Keep compensating downstream** (per-frame blend fading, `bIsInTheAir = false` after
  the fact, vertical velocity pins): rejected — this is what iterations 1.1-1.3 did. It
  fights a state that is re-entered every frame, and it cannot change the pose at all.
- **Modify `CheckIfInTheAir()` to know about swimming**: rejected — it is a pure
  ground-probe used by every ped; teaching one player's mod state inside it spreads the
  coupling instead of concentrating it.
- **Force the swim clip's animation group to win the hierarchy fight**: rejected — it
  depends on `ASSOCGRP_PLAYERSWIM`'s position in the blend hierarchy, which is fragile
  and does not stop the engine from believing the ped is falling.
- **Add the veto in the swim lane** (as iteration 1.3 did, by clearing the flag late):
  rejected — order-dependent and structurally incapable of preventing the blend.

## State

`accepted`.
