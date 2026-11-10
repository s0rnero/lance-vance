---
name: 038-a-collision-loop-is-bound-to-a-liveness-signal-not-to-the-global-collision-entity
status: accepted
date: 2026-11-03
domain: audio
supersedes: []
---

# ADR-038: a collision loop is bound to a liveness signal, not to the global collision entity

## Context

The player reported a sound stuck in a loop mid-session: a car scraped a lamp post, the loop
kept going after the car exploded and was gone, and the game then crashed.

The loop is the car scrape sample. `SetUpLoopingCollisionSound` requests it with
`m_nLoopCount = 0`, meaning loop forever (`src/audio/AudioCollision.cpp:340-367`), and
`SetLoopingCollisionRequestedSfxFreqAndGetVol` maps any surface pair that is not grass,
water, gravel or a ped - a lamp post among them - to `SFX_SCRAPE_CAR_1`
(`src/audio/AudioCollision.cpp:369-404`).

The attribution is the problem. `ServiceCollisions` tags every collision sample with
`m_sQueueSample.m_nEntityIndex = m_nCollisionEntity` (`src/audio/AudioCollision.cpp:137`),
and `m_nCollisionEntity` is a single global audio entity of type `AUDIOTYPE_COLLISION`,
created once at init with `(void *) 1` as its entity pointer
(`src/audio/AudioLogic.cpp:130-132`) and destroyed only at terminate
(`src/audio/AudioLogic.cpp:211-213`). Consequences:

- The `DestroyEntity` sweep added in T99 (`src/audio/AudioManager.cpp:206-216`) stops loops
  owned by a dying entity, but a scrape loop is not owned by the car, so destroying the car
  does not reach it.
- The `LOOPORPHAN` rule (`src/audio/AudioManager.cpp:1373-1385`) asks whether the entity slot
  is still in use. The global collision slot always is, so an orphaned scrape can never be
  classified as orphaned.
- The vanilla fade lives in `AddReleasingSounds` (`src/audio/AudioManager.cpp:1040-1069`) and
  decays the volume of the *requested-queue* copy. Once that copy reaches zero volume it stops
  being re-requested and nothing writes to the active channel again: the source keeps looping
  at the last volume it was given. `LOOPKILL`, which waits for 25 s of
  `m_asActiveSamples[i].m_nVolume == 0`, never sees a zero either, because the active copy
  kept the old value.

A scrape loop that loses its collision therefore has no owner, no orphan signal and no fade.
The one thing that reliably says "this loop is still wanted" is the collision manager asking
for it again.

## Decision

The watchdog demands positive evidence of life from collision loops. In the same
`__EMSCRIPTEN__` block of `ProcessActiveQueues` (`src/audio/AudioManager.cpp:1387-1405`), for
a playing loop whose entity is `m_nCollisionEntity`, the per-channel `live` stamp is
refreshed while `m_bIsBeingPlayed` is set - which `ProcessActiveQueues` does each frame only
when the collision manager re-requested that exact sample - and the channel is stopped, with
a `LOOPCOLLIDE` trace, after 400 ms without one. The rule is skipped while `m_bIsPaused`, so
a pause cannot starve the stamp, and it is scoped to `m_nCollisionEntity`, so no other loop
family is affected. `live` is initialised when a loop starts, cleared when it stops, and
cleared by every kill path.

## Consequences

- A stranded scrape is silenced in about 0.4 s instead of 25 s, and it no longer depends on a
  volume field the orphan never updates.
- A genuine continuous scrape is untouched: while the surfaces keep colliding the sample is
  re-requested every frame, so the stamp refreshes every frame.
- If `LOOPCOLLIDE` appears while the player still hears a scrape that should be audible, the
  fault is upstream - `ServiceCollisions` re-reporting a dead pair - and the fix belongs in
  the collision manager, not in the watchdog.
- The global collision entity stays ownerless by design. Binding loops to the actual vehicle
  would change sample attribution for every collision sound, a larger edit than this lane
  justifies for a symptom the latch covers.
- Accepted means the decision is taken, not verified in game.
