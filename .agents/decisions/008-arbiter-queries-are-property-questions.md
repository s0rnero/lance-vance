---
name: 008-arbiter-queries-are-property-questions
status: accepted
date: 2026-09-29
domain: architecture
refines: ADR-007
---

# ADR-008: The arbiter's queries are property questions; a lane-relative negation is not part of the API

## Context

`ADR-007` decided that one arbiter owns the player ped's capabilities and exposed
three calls: claim, release, and "blocks" — the last one described there as *"the only
call a feature needs: am I allowed to drive this right now?"*. That is a **self**
question, and for a self question the lane-relative predicate is correct: a lane is
blocked when somebody else owns the piece.

The `3.0` plan implemented that API with three queries, one of them
`ViceExtPedBlocked(lane, cap)`, which reads as a global statement ("this is blocked")
while meaning a lane-relative one ("this lane is not the owner"). It is the exact
negation of `ViceExtPedOwns(lane, cap)`, so the name inverts the meaning of the
sibling in the same header.

That distinction is invisible until somebody asks a **third-party** question. The nine
guards the 3.0 plan converted were not self-permission questions: they were the
engine asking *"does the swim own this piece?"*. Using `Blocked` for that question
produced its opposite in every one of the nine sites, so every guard fired when it
should not and stayed silent when it should.

The incident is `2026-09-29-swim20`, the build the player tested after the 1.7. The
arbiter itself was correct throughout: the session log shows 20 claims and 20
releases, no rejected claim and the invariant back to zero pieces held. What was
broken was only how the answers were read:

| Guard | Read as | Actual truth | Symptom reported by the player |
| --- | --- | --- | --- |
| crouch guard | swim owns the stance | never (the crouch lane never claims) | crouching impossible everywhere, `crouch on` = 0 in the whole session |
| weapon switch, twice | swim owns the weapon | true on land, false in water | cannot change weapon on land, can change it while swimming |
| aim predicate, twice | swim owns the aim | true on land, false in water | aiming possible while swimming |
| aim law | swim owns the aim | true on land, false in water | camera also switches to aiming |
| ground check | swim does not own movement | false in water | the swim pose is replaced by the fall pose again, `glide>0` in 71 of 84 samples |
| HUD gate, and its own trace | swim does not own the aim | true in water | crosshair drawn while swimming, and the trace that should have revealed it never fired |

The diagnostic trace for the HUD gate was inverted by the same mistake, so the log
carried no evidence at all: zero `SWIMHUD` lines in 4610. The generic checker had
already failed the session on the pose criterion, and the 1.7 was still reported as
ready to play: a second, smaller mistake, recorded here because the fix below is
worthless without it.

## Decision

**The arbiter answers questions of ownership, never about permission, and the
negation of a lane-relative query is not part of the API.**

The public surface is:

- `ViceExtPedOwner(cap)` — who owns this capability right now.
- `ViceExtPedOwns(lane, cap)` — does this lane own it.
- `ViceExtPedClaim(lane, cap)`, `ViceExtPedRelease(lane, cap)`.
- `ViceExtPedOnRelease(lane, fn, ud)`, `ViceExtPedResetAll()`.

`ViceExtPedBlocked` is **removed**. It had zero users once the nine sites were
corrected, and it was the only expression in the header that could be read as the
opposite of its neighbour.

The rules:

1. A caller that wants to know whether **it** may drive a capability asks
   `ViceExtPedOwns(myLane, cap)`. There is no separate "may I" call, because it
   would be the same call.
2. A caller that wants to know whether **another** lane owns a capability asks
   `ViceExtPedOwns(thatLane, cap)`. Asking about another lane through a negated
   predicate is what inverted the nine sites; the negation must be written by the
   caller, visibly, in the guard.
3. A guard that vetoes the base engine asks about the lane that wants the engine to
   stop, i.e. the same lane it is vetoing for. "Stop the engine because the swim
   owns it" is `!Owns(PEDLANE_NADO, cap)`, and the `!` belongs to the caller, next
   to the reason.

`ADR-007` is not edited (rules 0.12): this refines the description of the third
query, and the arbitro, its capabilities, lanes and invariants are unchanged.

## Consequences

- The class of defect is uncompilable rather than invisible: there is no expression
  in the header whose name reads as the opposite of its meaning.
- A guard now states its reason at the call site: `!Owns(PEDLANE_NADO, ...)` says
  "the swim does not own it, so the engine may act", which is checkable by reading
  the line.
- The three lanes that ask about a lane other than their own (the crouch asking
  whether the swim holds the stance, the HUD asking whether the swim holds the aim)
  are now explicit about it, and the arbiter's registry table (`ADR-007`) is the
  place where those questions are enumerable.
- One expression is lost. Nothing needed it: the count of `ViceExtPedBlocked` in
  the tree was 0 before its removal, and the removed function is recoverable from
  this ADR if a future need turns a lane-relative negation into the honest form.
- A guard that was wrong in the same way twice is not detectable by reading the
  arbiter, only by running the player's log through the checker. Hence the DoD
  change in the 1.7/1.8 plans: a build is not ready until the checker has run on a
  real session of that build.

## Alternatives Considered

- **Keep `ViceExtPedBlocked` and rename it** (`ViceExtPedNotOwnedBy`, or
  `IsOwnedByOther`): rejected — any name can be misread under time pressure, and
  this one had already been misread nine times in a row. Removing the expression
  turns a reading error into a compile error.
- **Keep both and document the difference in a comment in the header**: rejected
  twice over. The project forbids comments in code, and a comment is weaker than a
  signature that cannot be misused.
- **Invert at the call sites, keeping `Blocked` and negating nine more times**:
  rejected — it leaves nine places where the sign of the question is decided by
  hand, which is precisely the mistake being removed.
- **Return an enum with the three states (free / mine / theirs)** from one call:
  rejected as speculative. Two predicates cover the two questions actually asked;
  a three-state result would be a third form to learn, and nothing needs the
  distinction between "free" and "theirs" at a decision point.

## State

`accepted` (29/09, with the plan `1.8-polaridad-arbitro`, build
`2026-09-29-swim21`). The API lives in `src/core/PedArbiter.h` and
`src/core/PedArbiter.cpp` behind `VICEEXT_PEDARBITER`; after this decision the
header declares six functions and the tree contains zero occurrences of
`ViceExtPedBlocked`. The pose, the buoyancy, the entry/exit gate and the arbiter
invariants are untouched by this decision: the player's log for the failing build
showed them correct (`hondo` median 0.41, `pie=0`, `odcrouch=0`, invariant 0 = 0),
and the plan records that as out of scope on purpose.
