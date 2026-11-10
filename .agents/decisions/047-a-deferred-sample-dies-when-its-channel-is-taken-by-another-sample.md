---
name: 047-a-deferred-sample-dies-when-its-channel-is-taken-by-another-sample
status: accepted
date: 2026-11-05
domain: audio
supersedes: []
---

# ADR-047: a deferred sample dies when its channel is taken by another sample

## Context

ADR-039 made `CChannel::Start()` write `AL_LOOP_POINTS_SOFT` on every upload, clamped to the
buffer being filled. The player's next report was that the loop bug came back - "me volvio a
pasar". That is the expected shadow of the change: an inherited region used to loop a bogus
sliver (a fast, quiet stutter); now a wrong loop wraps the **whole** sample, which is louder
and unmistakable. So the question moved from "which region" to "who is allowed to start a
sample that is no longer his".

On web, `cSampleManager::InitialiseChannel()` may not be able to decode the sample yet. It then
calls `OdSfxChannelDefer()` (`src/audio/sampman_oal.cpp:1972`), which **returns TRUE** - so the
requested-queue activation (`src/audio/AudioManager.cpp:1338`) runs the whole
block on a channel that holds no data: `SetChannelLoopPoints`, `SetChannelLoopCount(k,
m_nLoopCount)` (which writes `AL_LOOPING` on the source) and `StartChannel`, which only marks
`s_odChState = OD_CH_START` and skips the actual start. Later `cSampleManager::Service()` picks
the channel up and does `SetSampleData(...)` + `Start()` for the sample it remembered in
`s_odChSfx[]`.

Nothing invalidates that memory in between. `InitialiseChannel`'s normal path only clears it
through `StopChannel()`, and `StopChannel` is called only `if (GetChannelUsedFlag(nChannel))`,
which asks whether the *source is playing*. A deferred channel was never started, so it is not
playing, so a second sample can be handed the same channel and:

- the pending entry survives with the old sample id;
- the new sample is a loop, so `SetLoopCount` puts `AL_LOOPING = AL_TRUE` on the source;
- when the old sample finally decodes, `Service()` refills the buffer with it and starts it -
  **looping**, because the loop flag belongs to the new occupant, and the manager has no idea:
  its `m_asActiveSamples[]` says this channel is the other sound, so no duration watchdog,
  `LOOPORPHAN`, `LOOPKILL` or `LOOPCOLLIDE` can fire.

A one-shot (gunshot, collision) refilled into a loop-flagged channel is exactly "un sonido nuevo
se queda en bucle". Frequency in the two captured sessions is low (`ODSFXSTAT defer=1` and
`defer=0`), which matches an intermittent complaint rather than a constant one.

## Decision

`InitialiseChannel()` drops the pending defer as soon as it commits the channel to a *new*
sample - before `Reset()`, guarded by `s_odChState[nChannel] != OD_CH_NONE`, and traced as
`ODCHCLASH ch=%u pend=%u nuevo=%u estado=%u` (40 rows). The pending sample is lost, which is
correct: its slot is gone; keeping it would play the wrong sound in the wrong place.

The same change names the sample inside `LOOPSET` (`CChannel::SfxTag`, set by both
`InitialiseChannel` and `OdSfxChannelDefer`) and stops sampling the row (it used to log 1 of
every 200 uploads, which could hide the very case being hunted). `LOOPSET` is now rate-limited
to one row per 120 ms with a 600-row cap.

## Consequences

- One-shot-stuck-in-a-loop caused by channel reuse is closed, and if it ever happens through a
  different door the `ODCHCLASH` counter says how often and `LOOPSET ... sfx=` names the sample
  that OpenAL is really looping.
- A dropped deferred sample is silence, not a wrong sound - the honest trade for that slot.
- `CChannel::Reset()` still does not clear `AL_LOOPING`; it is now reached only through paths
  that set the loop count again right afterwards. If a new start path is added, it must call
  `SetLoopCount` or the stale flag returns.
- The other recurring-loop candidates are now measurable rather than argued: `SNDDIE` carries
  `ent=`/`vivo=` (does its owner still exist?) and `SNDTOP` reports the **window delta** of
  requests and births instead of the cumulative top-8, which is what a rapidly retriggered
  one-shot looks like.
