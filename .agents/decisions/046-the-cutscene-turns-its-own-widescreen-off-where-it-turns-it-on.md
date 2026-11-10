---
name: 046-the-cutscene-turns-its-own-widescreen-off-where-it-turns-it-on
status: accepted
date: 2026-11-05
domain: camera
supersedes: []
---

# ADR-046: the cutscene turns its own widescreen off, where it turns it on

## Context

The player reported the sickness effect again after T115's drunkenness fix: the camera drifts
slowly side to side and the picture is soft. Two sessions of `MAREO` rows decide what it is.
Every row in both sessions reads:

```
type=2 blur=5 add=150 drunk=0.000 trails=0 just=1 sw=0 fb=1
```

`drunk=0.000` rules out `CMBlur::Drunkness` (the sway at `Camera.cpp:549-572` needs it > 0) and
`sw=0` rules out the post-FX pipeline (`EffectSwitch = POSTFX_OFF`, so `CPostFX::Render` paints
nothing). What is left is `add=150`: `main.cpp:2168-2170` writes that alpha only while
`m_ScreenReductionPercentage > 0`, and the only writer of 30.0 is
`CCamera::ProcessWideScreenOn()` - which only runs while `m_WideScreenOn`. So the widescreen
letterbox, its reduced viewport (soft image) and its cutscene FOV stayed on **after** the
cutscene: the `ms=` column of the MAREO window counts up from 0 to the row cap (150 rows at
500 ms = 75 s) without ever resetting, and the skip is at `ms=770`. The condition was continuous
for at least 77 s of gameplay, in both sessions.

The asymmetry is in `CCutsceneMgr`: `SetupCutsceneToStart()` calls `TheCamera.SetWideScreenOn()`
and only `DeleteCutsceneData()` calls `SetWideScreenOff()`. The script may reach the end of the
cutscene, or the player may skip, long before `DeleteCutsceneData()` is called - and on the skip
path `FinishCutscene()` already restores the camera and the player, so everything else is back to
normal except the screen mode.

## Decision

`CCutsceneMgr::FinishCutscene()` calls `TheCamera.SetWideScreenOff()` right after
`TheCamera.FinishCutscene()`, inside the `if (bCamLoaded)` block - symmetric with the
`SetWideScreenOn()` of `SetupCutsceneToStart()` and not dependent on the data cleanup.
`DeleteCutsceneData()` keeps its own call (idempotent: it only sets
`m_bWantsToSwitchWidescreenOff`).

The MAREO row additionally prints `wide`, `red` and the cutscene zoom, and its row cap went from
150 to 400 so a stuck state is measured to its real end instead of to the cap.

## Consequences

- The blur/letterbox half of the reported mareo is fixed at the cause. The residual slow sway is
  **not** attributed yet: the old `PEDCAM` gate only watched `MODE_FOLLOWPED` under 0.5 and
  produced zero rows in two driving sessions, so T117 replaced it with a 1 s window that reports
  the *travel* of Beta/Alpha/Distance in any mode, without stick input, and carries
  `wide`/`red`/`zoom`/`avoid`/`col` to show who holds it.
- The shipped `CDraw::SetCutsceneCameraZoom(1.0f)` inside the widescreen **off** branch
  (`Camera.cpp:2891`) stays: it is correct but was unreachable while the latch held. With this
  fix the branch is reached again.
- If a future report shows `wide=1` outside a cutscene with no `CUTSKIP` row nearby, look for
  another `SetWideScreenOn()` caller rather than assuming the cutscene.
