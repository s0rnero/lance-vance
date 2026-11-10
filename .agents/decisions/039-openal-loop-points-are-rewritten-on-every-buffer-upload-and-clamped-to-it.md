---
name: 039-openal-loop-points-are-rewritten-on-every-buffer-upload-and-clamped-to-it
status: accepted
date: 2026-11-03
domain: audio
supersedes: []
---

# ADR-039: OpenAL loop points are rewritten on every buffer upload and clamped to it

## Context

After T107 the player still heard repeating sounds: "si note bucles en el audio aun al
disparar un arma o al chocar, como que sonidos nuevos o algo asi se quedan en bucle por que
cargan mal o no se que razon es molesto y debe arregarse". None of the T107 watchdogs
(`LOOPORPHAN`, `LOOPKILL`, `LOOPCOLLIDE`) fired during that session, so the manager was not
holding a stuck loop - the loop that was audible was one the manager still wanted. That
moves the fault from the logic layer to the buffer layer.

`CChannel::Start()` (`src/audio/oal/channel.cpp:116`) uploads the sample into
`alBuffers[id]` and plays it. Each channel owns exactly one buffer, and it is refilled with
a different sample every time the channel is reused. Before this change the loop points
were written to that buffer only when the caller had supplied a usable pair (start non-zero
and end greater than start), so:

- a sample uploaded without loop points kept whatever region the *previous* sample on the
  same channel had asked for, because `alBufferData` does not reset
  `AL_LOOP_POINTS_SOFT`;
- a sample with an unset pair (`(0, -1)`, the shape `cSampleManager::InitialiseChannel`
  writes at `src/audio/sampman_oal.cpp:2083`) kept the OpenAL default `(0, 0)`, which is a
  one-sample repeat region - the source then plays to the end, wraps to zero and stutters;
- no code checked that the requested pair was inside the new sample. `SetChannelLoopPoints`
  divides the byte offsets by `DIGITALBITS/8` (`src/audio/sampman_oal.cpp:2184-2190`) and
  the SDT table is authored for the CD audio, so a pair beyond the decoded length is
  accepted silently and the loop region becomes whatever OpenAL clamps it to.

A loop that repeats a region inherited from another sample is exactly "a sound that is
repeating but seems wrong / badly loaded".

## Decision

`CChannel::Start()` writes `AL_LOOP_POINTS_SOFT` on **every** upload, always derived from
the buffer it is filling, and clamps it to that buffer. `last = DataSize/2 - 1` is the frame
count for both formats (mono uploads `DataSize` bytes of 16-bit; the 2D upmix uploads
`DataSize*2` bytes of stereo 16-bit, which is the same number of frames). A missing or
out-of-range start becomes 0; a missing or out-of-range end becomes `last`; an inverted pair
becomes `0..last`, i.e. the whole sample loops instead of a bogus sliver. The write is
skipped only when the buffer holds fewer than two frames.

A capped `LOOPSET ch=%u nsamp=%d lp=%d,%d freq=%d cnt=%d` trace (ADR-029 idiom, emitted
only when `LoopCount != 1`, i.e. for real loops) reports the region actually handed to
OpenAL, so a future report of a bad loop can be read as data instead of argued.

## Consequences

- Loop regions can no longer depend on channel history, which removes the cross-sample
  contamination that made legitimate loops sound corrupted.
- A request for an unbounded or inverted loop region now loops the whole sample instead of
  stuttering on a zero-length region. The whole sample is louder and longer than a stutter,
  so if such a sample exists in the shipped data it becomes audible in a different way; the
  `LOOPSET` trace names the channel and the region so that case is identifiable.
- One `alBufferiv` call per upload. It is a state write on an already-touched buffer, so the
  cost is bounded and the frame budget does not depend on it.
- `cnt=1` rows never appear by construction: one-shots do not read the region, so the trace
  stays quiet on the paths that cannot loop.
- Accepted means the decision is taken, not verified in game.
