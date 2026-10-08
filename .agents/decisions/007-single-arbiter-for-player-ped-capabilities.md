---
name: 007-single-arbiter-for-player-ped-capabilities
status: accepted
date: 2026-09-29
domain: architecture
---

# ADR-007: One arbiter owns the player ped's capabilities; the base engine is the default implementation

## Context

The ported features that live in this tree (swimming, crouch, the ClassicAxis aiming
law, the camera, recoil) were added one at a time, each owning a slice of the player
ped, and none of them knowing the others exist. The base engine keeps its own
mechanisms for the same slices, and the WASM build cannot load the mods at runtime
(`ADR-001`), so there is no "mod version" to fall back to: both implementations are
live at the same time.

The result is a recurring defect class, not a set of unrelated bugs:

| Symptom | Two authorities for | Where it was found |
| --- | --- | --- |
| Swim pose replaced by a fall pose | the fall/airborne state | `Ped.cpp` airborne check |
| Ped dragged down by the sea floor | the buoyancy impulse vs the swim pin | `Ped.cpp` `ProcessBuoyancy` |
| Crosshair drawn while not aiming | the HUD draw path vs the aim law | `Hud.cpp` crosshair block |
| Crouching while swimming, swim clip at blend 0 | the mod crouch vs the swim clip | `ViceExtCrouchControl` |
| Weapon reticle over a locked target | `m_pPointGunAt` vs the aim predicate | `ProcessPlayerWeapon` |

`ADR-003` already decided that the mod's behaviour is the specification and that the
mod must not be layered on top of an existing half-finished implementation. That
decision has not been violated by the swim feature, which is why its fixes
(`ADR-006`) converge; it **is** violated by the crouch, which runs the engine's duck
and the mod's `odCrouched` at the same time, and by the aiming reticle, which the HUD
draws from the weapon type alone.

Every one of these took a build and a play session to find, because the lanes are
discovered symptom by symptom rather than enumerated.

## Decision

**Each player-ped capability has exactly one owner at a time, and one small arbiter
decides who.**

A capability is a piece of the player ped's state that two features could both try to
drive: locomotion, stance, aim, camera, weapon. The arbiter is a static registry (no
allocation, no per-frame churn) behind `VICEEXT_PEDARBITER`, indexed by capability and
lane and deliberately **not** depending on `CPlayerPed`, so lanes outside `peds/` can use
it (`src/core/PedArbiter.h`, `src/core/PedArbiter.cpp`):

- `ViceExtPedClaim(lane, cap)` / `ViceExtPedRelease(lane, cap)` — take a capability and
  give it back. Release invokes the callbacks registered with
  `ViceExtPedOnRelease(lane, fn, ud)`, and only when the owner changes, never per frame.
- `ViceExtPedOwner(cap)` — who owns this capability right now.
- `ViceExtPedOwns(lane, cap)` — the only question a lane needs at its own decision point:
  does my lane own this (or does that other lane own it)? There is deliberately **no**
  negated "blocked" call: the `!` belongs at the call site, next to its reason
  (`ADR-008`, which also removed the one that could be misread).

The rules the arbiter encodes:

1. **One owner per capability.** Two features may not drive the same capability at the
   same time. A feature that wants an owned capability does not silently take it.
2. **The base engine is the default implementation, not a claimant.** Claiming a
   capability does not disable or delete engine code; it inserts a veto at the
   engine's own decision point. This is the `ADR-006` pattern, generalised: replacing
   is impossible because the base mechanisms are load-bearing (`bIsDucking` has 85
   uses across the AI, the fight system, the civilian and cop peds and the camera;
   `m_pPointGunAt` 80; `bIsInTheAir` 33 including audio and mission scripts), while
   removing the turn is one condition at one place.
3. **Release cleanly.** A feature that loses a capability must restore what it changed
   on exit (clips faded, state restored, flags cleared). The swim feature needed this
   ad hoc for its exit animation; it becomes the contract.
4. **Claiming is a precondition, not a substitute for cleanup.** A feature that claims
   a capability still has to leave the previous owner's state intact.

The arbiter enumerates the capabilities and lanes it knows in its own header (`ePedCap`,
`ePedLane`). The wider deliverable — one maintained table of capability → code location →
current owner → who must ask, with a verifier keeping it from rotting — was **not built**,
and the defects kept being found symptom by symptom (plans 4.1-4.4).

## Consequences

- A new mod feature that drives a shared capability has an obvious, documented place
  to declare itself instead of a bug report three sessions later.
- Ownership becomes a declared question instead of a guess: the lanes that need it
  (swim, crouch, weapon, HUD) can ask the registry at their own decision point.
- It does **not** close the crouch conflict: the arbiter gives the stance one place to be
  claimed, but reconciling the engine's duck with the port's crouch is its own work
  (plans 4.1-4.4; J5/J6/J7 in plan 4.4 §14).
- The HUD crosshair is still a second opinion: `Hud.cpp` reads the aim law and the pad
  directly (`:292`, `:723`) instead of the single "am I aiming" API (`ADR-004`).
  Converting it is open.
- The arbiter must be tiny and boring: one file, a fixed-size table, no dynamic
  dispatch, no cost worth measuring in the frame budget.
- Touching the crouch, aiming and HUD lanes crosses lane boundaries, which
  `DESIGN.md` requires to be coordinated in a plan rather than merged silently.

## Alternatives Considered

- **Delete the base mechanics and keep only the mods'**: rejected — the base
  mechanisms are shared with the AI, the fight system, the civilian and cop peds, the
  camera, the audio and the mission scripts. Removing them breaks the game, not just
  the player, and buys nothing for the goal.
- **A global feature priority order ("this mod always beats that one")**: rejected —
  the question is never which feature wins in the abstract, it is who owns *this piece
  of state right now*. The features must coexist: crouch on land, aim on land, swim
  in water.
- **Keep patching, one guard per symptom**: rejected — this is what produced six swim
  iterations, three "fixed" reports that came back, and a class of defect that is
  still open.
- **A generic event/signal system**: rejected — far more machinery than the four
  capabilities need, and it would hide the ownership question instead of answering it.

## State

`accepted` (29/09, with the plan `3.0-arbitro-estados`, build `2026-09-29-swim19`),
refined by `ADR-008`. The arbiter's invariants held in the swim logs (20 claims, 20
releases, no rejected claim, nothing held at the end of the session) and only the swim
lane uses it so far. What it buys is the single documented place to declare ownership,
not the end of the defect class; today only the swim feature is validated end-to-end by
the player's sessions.
