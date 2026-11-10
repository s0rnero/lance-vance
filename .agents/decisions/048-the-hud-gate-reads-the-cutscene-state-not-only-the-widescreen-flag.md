---
name: 048-the-hud-gate-reads-the-cutscene-state-not-only-the-widescreen-flag
status: accepted
date: 2026-11-05
domain: hud
supersedes: []
---

# ADR-048: the HUD gate reads the cutscene state, not only the widescreen flag

## Context

In the session played on `2026-11-05-45.0-T5` the player skipped the first cutscene and saw the
free-roam HUD - minimap, wanted stars, weapon - while the mission had not started yet
(`Grabación de pantalla 2026-11-05 170546.mp4`, 22:05:21Z onward). Vice City has no explicit
"during a cutscene" test in the HUD: the only thing that hides it is the widescreen flag, because
`CCutsceneMgr::SetupCutsceneToStart()` turns widescreen on and that is what keeps
`CHud::Draw()` out of the drawing branch (`src/renderer/Hud.cpp:340`).

ADR-046 made `FinishCutscene()` symmetric with `SetupCutsceneToStart()` and call
`SetWideScreenOff()` on the skip path (that is T117). It fixed the letterbox and the blur that
stayed on after a skip, and it is still correct. What it exposed is that the HUD was riding on
that flag: with widescreen now off, the cutscene tail - the player is still in
`MODE_FLYBY`/`ms_running` for a few frames while the data is cleaned up - lost its only HUD
blocker.

## Decision

`CHud::Draw()` gates on the cutscene state itself, next to the widescreen flag:

```c
if (m_Wants_To_Draw_Hud && !TheCamera.m_WideScreenOn && !CCutsceneMgr::IsRunning()) {
```

ADR-046's `SetWideScreenOff()` inside `FinishCutscene()` stays - it is what fixed the residual
letterbox/FOV, and the HUD no longer depends on it. `HUDGATE` traces one row per change of the
eight booleans that decide the gate (`wants/wide/cutrun/cutproc/cutskip/intro/replay/cam`, the
camera mode included), so a future "the HUD is where it should not be" is answered from the log
instead of by reasoning about which flag somebody else sets.

## Consequences

- The HUD is now hidden by state, not by a screen effect: any future change to the widescreen
  latch cannot re-expose it during a cutscene.
- Cost: a cutscene that ever wants the HUD visible would need an explicit exception. None exists
  in this engine - the original hides the HUD during cutscenes too, by the same widescreen trick.
- `HUDGATE` is `#ifdef __EMSCRIPTEN__`, capped at 60 rows and only on state change, so it does
  not add per-frame work.
- The in-game check is still open: T1 is verified by code path and by the recording's timing, but
  the "no minimap after the skip" confirmation needs the player's next session log.

## Alternatives Considered

- **Revert T117 (keep widescreen on after the skip).** Discarded: it re-introduces the letterbox,
  the reduced viewport and the cutscene FOV that ADR-046 measured as the blur half of the mareo,
  and the player asked to fix rather than undo plan 45.
- **Gate on `!CGame::playingIntro`.** Discarded: `playingIntro` covers the intro sequence only,
  not the between-mission cutscenes the player skips, so it would not have caught T1.
- **Gate on the camera mode (`Cams[0].Mode != MODE_FLYBY`).** Discarded: the mode is also FLYBY in
  the boat camera and in the chase camera, so it hides the HUD where it should stay.
