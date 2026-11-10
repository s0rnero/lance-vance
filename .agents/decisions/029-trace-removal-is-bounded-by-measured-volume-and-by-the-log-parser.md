---
name: 029-trace-removal-is-bounded-by-measured-volume-and-by-the-log-parser
status: accepted
date: 2026-11-02
domain: instrumentation
---

# ADR-029: Trace removal is bounded by measured volume and by the log parser

## Context

The player asked for traces that cost performance to be removed, as part of the
work on FPS stability. "Costs performance" was not defined, so the first pass
removed every `ODTRACES` call site that looked optional. That pass included two
traces the project's own log checker depends on, which would have silently
turned two verification blocks into permanent false alarms.

`gta_vc_browser/tools/viceext-log-check.py` is the only instrument the project
has for judging what happened in a played session, and the harness forbids
using a browser to verify gameplay. A trace that a check block reads is part of
the verification protocol, not decoration.

The 17:55 session log (`gta_vc_browser/logs/odtrace-2026-11-02_17-55-33.log`,
37,197 lines over 575 s of play, build T97) gives the real cost of each
candidate:

| Trace | Lines | Per second | Buffer | Consumer |
|---|---|---|---|---|
| `VEHATOM` | 8,924 | 15.5 | narrow | none |
| `VICEEXT push` / `pushshift` | 6,141 | 10.7 | 640 B, ~30 float conversions | none |
| `VEX scm` | 2,184 | 3.8 | 80 B | `viceext-log-check.py` block S13 |
| `VICEEX sfx` | 64 | 0.11 | 80 B | `viceext-log-check.py` block D8 |

The two traces with no consumer account for 15,065 lines, 40.5 % of everything
the session wrote. The two with a consumer account for 6.0 %.

## Decision

1. A trace may be removed for performance only when its measured volume
   justifies it. Volume is measured from a real session log, in lines per
   second of play, not estimated from how often the call site looks reachable.
2. A trace read by a `viceext-log-check.py` block is not removable on
   performance grounds. Removing one requires removing or rewriting the block
   that reads it, in the same change, and saying so to the player.
3. `VEHATOM` and the `VICEEXT push` / `VICEEXT pushshift` family are removed.
   The logic they observed stays: `odPushScale` still scales the pedestrian to
   vehicle impulse by `CTimer::GetTimeStepFix()` in
   `CPhysical::ApplyCollision`.
4. `VEX scm` (`src/control/Script8.cpp`) and `VICEEX sfx`
   (`src/audio/AudioLogic.cpp`) are kept as they were.
5. `check-served-build.sh` keeps its `VEX scm` marker. Because the marker
   survives, the served-build verifier needed no change.

## Consequences

- Trace output falls by 40.5 % in a comparable session, which is the whole
  performance gain this decision claims. It is a secondary gain: the measured
  frame cost was dominated by the on-demand await, not by formatting (see
  ADR-028).
- Blocks S13 and D8 keep working. S13 still reports unimplemented 4000-4099
  opcodes and still proves the mod's `main.scm` ran; D8 still cross-checks
  weapon samples that fired against samples that started, which is how the
  `SAMPLEBANK_MAX` mute was found.
- `VEHATOM` and the push traces can no longer be used to diagnose a collision
  or vehicle-atom regression. Restoring them is a revert of the two deletions,
  not a redesign.
- The 640-byte push trace was the widest format in the engine. Nothing that
  wide remains on a per-collision path.

## Alternatives Considered

- **Remove all four, then patch the parser so S13 and D8 report "trace removed
  by design"**: buys 5.9 % more volume reduction and permanently loses two
  verification capabilities. Discarded.
- **Keep all four**: leaves 40.5 % of trace volume in place, against an
  explicit instruction. Discarded.
- **Rate-limit the two hot traces instead of removing them**: `VICEEXT push`
  was already rate-limited to two per frame and 100 ms per path, and still
  produced 10.7 lines per second, so a further limit would only have hidden
  the events the trace exists to show. Discarded.

## State

`accepted`
