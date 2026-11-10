---
name: 031-the-collision-impact-one-shot-is-rate-latched-on-web
status: accepted
date: 2026-11-02
domain: audio
supersedes: []
---

# ADR-031: The collision impact one-shot is rate-latched on web

## Context

The player drove into a lamppost and reported that the impact sound "stayed in a
loop", repeating continuously, and associated it with the sample being loaded for
the first time. Build `2026-11-02-41.0-T99`, log
`gta_vc_browser/web/odtrace.log`.

The sound is not looping. `SetUpOneShotCollisionSound` sets
`m_nLoopCount = 1` (`src/audio/AudioCollision.cpp:325`), and
`CChannel::SetLoopCount` maps a count of 1 to `AL_LOOPING = AL_FALSE`
(`src/audio/oal/channel.cpp:213`). An impact one-shot cannot loop at the OpenAL
level. It can only be **re-fired**, and it is.

`DMAudio.ReportCollision` is called once per collision point per frame from the
collision resolver — `src/entities/Physical.cpp:1523, 1545, 1578, 1605, 1804,
1806, 1815, 1842, 1844, 1869, 1924, 1949, 1985, 2015` — with no latch and no
cooldown. The only deduplication is `cAudioManager::ServiceCollisions`, which
compares this frame's collision against the previous frame's on a strict
four-field tuple (`AudioCollision.cpp:145-149`):

```
(m_pEntity1, m_pEntity2, m_bSurface1, m_bSurface2)
```

When the tuple matches, only the looping scrape is refreshed (`:153`). When it
does not, `SetUpOneShotCollisionSound` runs (`:187`) and mints a brand-new
`m_nCounter = counter++` cycling 28 to 254 (`:319`). Because the counter is new
every time, the match key in `ProcessActiveQueues` — entity index, counter and
sample index together (`src/audio/AudioManager.cpp:1142-1143`) — can never match
an already-active sample, so a fresh channel is allocated and the sample is
restarted from sample zero. A vehicle resting against or grinding on a lamppost,
where the winning collision point alternates or the surface flips between
`SURFACE_LAMP_POST`, `SURFACE_CAR` and `SURFACE_TARMAC`, produces a new impact
one-shot every frame.

The session log records exactly that. Between 22:44:38.743 and 22:44:41.543 —
2.8 seconds — the engine requested eight distinct collision one-shot samples,
all of them cold:

| Sample id | Name | Surface family |
| --- | --- | --- |
| 93, 95, 96 | `SFX_COL_TARMAC_2/4/5` | road |
| 136, 137, 138, 139 | `SFX_COL_CAR_1..4` | hard car impact, not the panel variant |
| **110** | **`SFX_COL_HYDRANT_1`** | lamppost |

Sample 110 is the lamppost. `gOneShotCol[SURFACE_LAMP_POST]` is
`SFX_COL_LAMP_POST_1`, id 109, and the selector adds
`m_anRandomTable[1] % 2` (`AudioCollision.cpp:275-277`). The enum has only one
lamppost entry, so that modulo spills half of all lamppost hits into id 110,
`SFX_COL_HYDRANT_1`. Requesting tarmac, car and lamppost samples together inside
2.8 seconds is the signature of one sustained vehicle contact being re-resolved
as several different surface pairs, not of eight separate collisions.

The player's intuition about first load was also correct, and it is a second,
independent defect. `OD.prefetchLoops` pre-fetches only samples that carry loop
points (`gta_vc_browser/web/ondemand.js:577`). Reading the SDT descriptors in
`gta_vc_browser/streamed/Audio/sfx.SDT`:

| id | sample | size | freq | loopStart | loopEnd | prefetched |
| --- | --- | --- | --- | --- | --- | --- |
| 109 | `SFX_COL_LAMP_POST_1` | 8824 | 11025 | 0 | -1 | **no** |
| 110 | `SFX_COL_HYDRANT_1` | 8982 | 16000 | 0 | -1 | **no** |
| 136 | `SFX_COL_CAR_1` | 41566 | 22050 | 0 | -1 | **no** |
| 144 | `SFX_SCRAPE_CAR_1` | 28092 | 22050 | 0 | 28089 | yes |

The impact samples have no loop points, so they are never prefetched and are
guaranteed cold on first use. The scrape does have them, so it is warm — which is
why the log shows `ODSFXMISS sfx=144 ... loop=1` at 22:44:39.526 succeeding
while `ODSFXNO sfx=110` failed at 22:44:40.293. A cold sample goes through
`OdSfxDecodeRaw`'s failed `fopen` (`src/audio/sampman_oal.cpp:356-368`) into
`OdSfxChannelDefer`, which returns `TRUE` despite loading nothing
(`:1983`). `ProcessActiveQueues` takes that as a live channel
(`AudioManager.cpp:1282`), and `Service` starts it up to `OD_CH_MAX_WAIT`
(700 ms) later without re-checking that the sound is still wanted
(`sampman_oal.cpp:2615-2618`).

Neither of the two web watchdogs that should catch a stuck collision sound can
fire. `LOOPKILL` (`AudioManager.cpp:1392-1404`) needs
`now - wl[i].silent > 25000`, but the toggle at `:1405-1410` has an
`else if (wl[i].on)` branch that is not guarded by `!playingLoop`, so it zeroes
`wl[i].silent` on alternating frames *while the loop is still playing*; the
elapsed check always evaluates to about zero. `LOOPORPHAN` (`:1373-1385`) needs
`!entUsed`, and `m_nCollisionEntity` is created once and marked used permanently.
The T99 log records zero `LOOPKILL` and zero `LOOPORPHAN` in 8 minutes 55
seconds of play, which is consistent with both being dead rather than with both
being unnecessary.

## Decision

1. **Rate-latch the impact one-shot on web.** `ServiceCollisions` consults
   `OdColImpactLatched` (`AudioCollision.cpp:80-127`) before calling
   `SetUpOneShotCollisionSound` (`:186-187`). The latch keys on the collision
   identity **normalised for order** — the two entity pointers sorted, the two
   surface bytes sorted — so that `(car, post, tarmac, lamp)` and
   `(post, car, lamp, tarmac)` are recognised as the same contact, which is the
   case the strict tuple at `:145-149` misses.
2. **The window is a cooldown, not a suppress-while-held.** A key that matches
   inside `LATCH_MS = 250` is rejected and its stored timestamp is *not*
   refreshed. The result is at most four impact one-shots per second per contact
   pair, rather than sixty, and a genuinely new hard impact after the cooldown
   still plays. Refreshing the timestamp was considered and rejected: it would
   silence the impact entirely for as long as the vehicle stays in contact.
3. **The state is function-local statics, not class members.**
   `cAudioCollisionManager` is size-asserted at `AudioCollision.h:57`
   (`VALIDATE_SIZE(cAudioCollisionManager, 0x354)`) and `cAudioCollision` at
   `:34`, so adding a field would break the retail layout check. The latch keeps
   `NUMAUDIOCOLLISIONS` (10) entries in a ring, matching the manager's own
   capacity, so it cannot evict a key that is still in play.
4. **The looping scrape is untouched.** `SetUpLoopingCollisionSound` is still
   called on both paths (`:153` and `:191`) with its `m_nLoopCount = 0`, i.e. a
   genuine infinite `AL_LOOPING`. Sustained contact is meant to be carried by
   that scrape, and the latch on the one-shot is what lets it do that job.
5. **Web build only.** The helper and its call site are inside
   `#ifdef __EMSCRIPTEN__`; the `#else` arm keeps the original single call. The
   native reconstruction is unchanged.
6. **The dead watchdogs and the deferral amplifier are recorded but not fixed
   here.** Clause 1 removes the reported symptom at its source. Fixing
   `LOOPKILL` (one guard at `AudioManager.cpp:1407`) and making
   `OdSfxChannelDefer` stop returning `TRUE` on failure are separate decisions
   with separate blast radius, and the player scoped this change to the impact
   latch.

## Consequences

- The reported symptom is addressed at its source: a sustained vehicle contact
  can no longer restart the impact sample every presented frame.
- A cost in fidelity: two genuinely distinct hard impacts on the same object
  with the same surface pair inside 250 ms will sound as one. At 60 fps that
  window is 15 frames, shorter than the 0.4 s length of sample 109
  (8824 bytes, 11025 Hz mono), so the second impact would in any case have
  landed inside the tail of the first.
- The latch is keyed on entity pointers. A pointer can be reused after an entity
  is destroyed, so a destroyed-and-reallocated pair could be wrongly latched for
  up to 250 ms. The consequence is one suppressed impact sound; there is no
  memory-safety implication because the pointers are only compared, never
  dereferenced.
- The surface-pair normalisation deliberately treats `(A,B,tarmac,lamp)` and
  `(B,A,lamp,tarmac)` as one contact. That is a deviation from the strict tuple
  at `:145-149`, and it only affects the one-shot, never the scrape.
- One new trace, `ODCOLLATCH sup=%u s1=%u s2=%u edadms=%u`, capped at the first
  20 suppressions and then every 500th. Within ADR-029's bound: at most about 85
  lines in a long session, and it reports the cumulative suppressed count so a
  single line carries the whole magnitude.
- Not fixed by this decision, and still open: `LOOPKILL` cannot fire
  (`AudioManager.cpp:1405-1410`); `LOOPORPHAN` cannot fire for collision audio;
  `OdSfxChannelDefer` returns `TRUE` on failure (`sampman_oal.cpp:1983`);
  `prefetchLoops` skips every impact sample (`ondemand.js:577`); and
  `SFX_COL_LAMP_POST_1 + rand % 2` spills half of all lamppost hits into
  `SFX_COL_HYDRANT_1`.
- Verification: compiled via `bash gta_vc_browser/build.sh`. No gameplay was
  verified in a browser — the project forbids the agent from doing that — so
  `accepted` here means "decision taken, implemented and compiled", not
  "confirmed in the game". The measure that decides it in the next session is
  `ODCOLLATCH`: a `sup=` value in the hundreds after a lamppost hit means the
  latch caught real re-triggering, and `sup=0` for a whole session means the
  diagnosis was wrong.

## Alternatives Considered

- **Normalise the four-field tuple at `AudioCollision.cpp:145-149` instead**:
  rejected. That comparison also decides whether the *looping scrape* is
  refreshed and whether the collision slot is reused, so normalising it changes
  the scrape's behaviour and the slot bookkeeping at `:158-170`. Wider blast
  radius for the same audible result.
- **Fix `LOOPKILL` and rely on it**: rejected as the primary fix. It only acts on
  samples with `m_nLoopCount == 0`, and the impact one-shot is hard-coded to 1,
  so the watchdog is structurally blind to the reported symptom no matter how
  correct its bookkeeping becomes. It is still worth fixing, separately.
- **Prefetch the collision samples in `ondemand.js`**: rejected for this change.
  It removes the cold-load amplifier but not the re-trigger, which is what makes
  the sound audible as a loop. It also needs a rebuilt `reVC.js` and a data-tag
  decision of its own.
- **Deduplicate by sample index rather than by contact**: rejected. Two different
  vehicles hitting two different lampposts in the same 250 ms would share the
  `SFX_COL_HYDRANT_1` index and one of them would be silenced.

## State

`accepted` (2026-11-02, player's instruction after the T99 session, scoped to
the impact latch). Implementation: `src/audio/AudioCollision.cpp:80-127` and
`:186-189`. Gameplay verdict pending the player's next session.
