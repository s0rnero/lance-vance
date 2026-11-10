---
name: 040-the-audio-reset-stops-the-live-channels-not-only-the-managers-slots
status: accepted
date: 2026-11-03
domain: audio
supersedes: []
---

# ADR-040: the audio reset stops the live channels, not only the manager's slots

## Context

`cAudioManager::ResetTimers()` runs at every scene boundary: `Game.cpp:1031`, `1180` and
`1210` (boot, menu to game, game to menu), `Frontend.cpp:1177`, `FrontendTriggers.h:870`
and `1012`, and once from `CTimer` startup (`src/core/Timer.cpp:79`). It is the handler that
clears the audio state when the world is thrown away and rebuilt, which is also when the
player notices sounds that outlive their source.

The old body swapped the requested queue, then called `ClearActiveSamples()`
(`src/audio/AudioManager.cpp:1447-1486`) and finally stopped a single channel explicitly,
`CHANNEL_POLICE_RADIO` (`src/audio/AudioManager.cpp:360-363`). `ClearActiveSamples` only
forgets the manager's bookkeeping - `m_nActiveSamples` and the `m_asActiveSamples` slots -
it never reaches OpenAL. A looping source that was playing at that moment therefore keeps
playing with no slot describing it, and everything that could later silence it is blind:

- `ProcessActiveQueues` sweeps `i < m_nActiveSamples` and stops channels whose slot was not
  re-requested this frame (`src/audio/AudioManager.cpp:1243-1249`); the forgotten channel is
  past the new, smaller `m_nActiveSamples`.
- The T107 watchdogs (`LOOPORPHAN`, `LOOPKILL`, `LOOPCOLLIDE`, `src/audio/AudioManager.cpp:1360-1434`)
  iterate the same array, so an orphan outside it is never examined.
- `cSampleManager::Service()` (`src/audio/sampman_oal.cpp:2584-2635`) only pumps deferred
  channels; it never kills a playing one.

The result is a loop that survives its own scene - a scrape, a siren or an engine that keeps
repeating after the car, the mission or the save is gone. The manager-side machinery cannot
see it because the manager has already forgotten it.

## Decision

`ResetTimers` stops the hardware before it forgets the bookkeeping: the loop
`for (uint8 i = 0; i < m_nActiveSamples; i++) SampleManager.StopChannel(i);` is inserted
immediately above `ClearActiveSamples()` (`src/audio/AudioManager.cpp:356-360`). This is the
same idiom `SetCurrent3DProvider` already uses for its own teardown
(`src/audio/AudioManager.cpp:469-477`), and together with the existing
`StopChannel(CHANNEL_POLICE_RADIO)` on the next line it covers every generic channel
(`NUM_CHANNELS_GENERIC = 42`, `src/audio/sampman.h`). It runs only on the transitions listed
above, so it is not a per-frame cost.

## Consequences

- A loop that is alive when a scene changes is cut at the source instead of being orphaned
  in OpenAL, where no watchdog can reach it.
- The reset becomes destructive by design: any sound wanted across the boundary must be
  re-requested after it, which is already how the music, police-radio and mission-audio
  resets behave in the same function.
- The sweep is bounded by `m_nActiveSamples`, so if a channel above that index were somehow
  live without a slot it would still be missed; no path in the manager leaves one behind,
  and `LOOPSET` (ADR-039) would name it if it ever happened.
- Accepted means the decision is taken, not verified in game.
