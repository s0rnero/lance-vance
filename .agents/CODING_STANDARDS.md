---
name: coding-standards
description: Code writing conventions for the project (reVC / GTA VC in the browser).
project: re3 - GTA Vice City in the browser (branch miami)
---

# Coding Standards

> Conventions for this repo. Adapted to the real stack on 2026-09-27.
> Whatever is not written here must not be invented by inference; ask or propose it.

## 1. File Structure

- The engine follows the upstream re3/reVC layout under `src/` (`peds/`, `control/`,
  `animation/`, `vehicles/`, `core/`, `renderer/`…). Keep files in their existing
  module; do not create new top-level modules without a reason the user agreed to.
- Ported mod features live **behind a `VICEEXT_*` define** in `src/core/config.h`
  (add the define **at the end of the corresponding block, without reordering**).
- **No comments in code** (project rule, the player's, 29/09). Code carries no
  comments at all: not lane headers (`// R29 ...`), not explanations, not TODO-ish
  notes. Everything a comment would say lives in the plan under `.agents/plans/`
  that ordered the change, and in the ADR if it is a design decision. This
  overrides the upstream re3 convention of documenting each block inline. Existing
  comments in untouched code stay (do not clean up other lanes' work: RULES 0.17);
  the rule applies to what we write or modify.
- Keep functions in a consistent order: helpers first, orchestrators last; do not
  shuffle others' blocks.
- One responsibility per file when the language allows it. Do not mix engine code with
  web/tooling code: those live in `gta_vc_browser/`.

## 2. Naming Conventions

| Context | Convention | Example |
| --- | --- | --- |
| Engine files | re3 style (PascalCase) | `PlayerPed.cpp`, `AnimManager.cpp` |
| Classes / structs | PascalCase, `C` prefix for engine classes | `CPlayerPed`, `CAnimManager` |
| Methods | PascalCase | `CalculateNewVelocity`, `BlendAnimation` |
| Member variables | `m_` + description | `m_vecMoveSpeed`, `m_fRotationCur` |
| Static file state | `s_` + description | `s_odRollActive` |
| Lane-local statics | `od` prefix (this project) | `odCrouched`, `odSideWhy` |
| Constants / macros | UPPER_SNAKE_CASE | `VICEEXT_CROUCH_WALK_MPS` |
| Tooling scripts | snake_case files | `ifp_scale_root.py`, `gen_manifest.py` |

New animation IDs go **at the end** of the relevant enum in `src/animation/AnimationId.h`,
with consecutive names/descs in `src/animation/AnimManager.cpp` (the engine indexes
`id - firstAnimId`).

## 3. Contracts And Data

- **Units are a contract.** `m_vecMoveSpeed` is in *metres per 50 fps frame* = m/s ÷ 50.
  Convert explicitly with `METERS_PER_SECOND_TO_GAME_SPEED` (`src/control/CarCtrl.h`).
  Never write a raw m/s number into engine speed fields.
- **Do not invent animation IDs, clip names or model IDs.** Verify clips with
  `python gta_vc_browser/tools/ifp_inspect.py <ifp>` and IDs against `vc.json` /
  the served data before using them.
- Engine data that crosses to disk follows the documented formats (IFP layouts,
  SCM bytecode, TKEY/TDAT GXT, `.ide/.ipl/.dat`). Decode with the existing tools;
  on-disk layout differences require a plan, not a guess.
- `VERSION` (`gta_vc_browser/web/lib/index.js`) rises on **every** build.
  `dataTag` (`gta_vc_browser/web/ondemand.js`) rises **only** when served data changes.

## 4. Errors

- Prefer the engine's own mechanisms (`REVC ASSERT`, boolean `Can...` guards, bounds
  checks before indexing). `CAnimBlendAssocGroup::GetAnimation(id)` has **no range
  check** — validate `firstAnimId`/`numAssociations` before asking for a name to `%s`.
- Diagnostics are traces, not prints scattered in logic: enable them behind
  `#ifdef __EMSCRIPTEN__` and emit with `ODTRACES(...)`, one line per state change
  (not per frame; dedupe with `CTimer::GetFrameCounter()` where the call can repeat).
- Keep trace buffers big enough for the full line (`snprintf` truncates silently);
  verify with the log checkers, never by assumption.

## 5. Testing

- There is **no unit-test framework**. Verification is:
  - `bash gta_vc_browser/tools/check-served-build.sh` — proves the **served** build
    carries every implemented marker. Run it before asking the player for a session.
  - `python gta_vc_browser/tools/viceext-log-check.py <log>` — dictaminates gameplay
    from the player's `gta_vc_browser/logs/odtrace-*.log` (D1…D7, R-blocks, AX…).
  - Headless smoke tests (`node gta_vc_browser/tools/*.mjs`) exist but **ask the player
    first**: the current measurement method is the player's own game + logs.
- The **criterion PASS is defined before** the player plays, and it must be measurable
  from the log (numbers) or by his ear/eye, never "looks fine".

## 6. Verification And Scripts

- Real scripts are in `.agents/AGENTS.md`. Do not run build/dev/test scripts without
  authorization (rule 0.4).
- Fast iteration: rebuild a single object with ninja
  (`cd gta_vc_browser/build/web && ninja src/CMakeFiles/reVC.dir/<path>.cpp.o`), and
  only link with `bash gta_vc_browser/build.sh` when the change is ready to test.
- House rules that bite:
  - **Line endings: detect, never assume.** The tree is **mixed and both are
    legitimate**: measured 07/10 on `src/**` = **366 files CRLF, 136 LF, 1 mixed**
    (the mixed one is `src/renderer/Sprite2d.h`); the 02/10 census (370/134, 0 mixed)
    is stale. There is no `.gitattributes` rule for them, **but `core.autocrlf=true` is
    set in the machine's global gitconfig**: git therefore normalises on the next
    checkout and warns (`LF will be replaced by CRLF`). Never let git be the one that
    rewrites a file you just touched. The old
    rule ("`.cpp`/`.h` are CRLF") was simply false and cost several edit scripts.
    So: read the target file, detect whether it is CRLF or LF, generate your patterns
    with that style, write it back in **the same** style, and assert you did not
    introduce the other one. `gta_vc_browser/web/lib/index.js` and
    `gta_vc_browser/tools/*.py` are **LF**; `tools/check-served-build.sh` is **LF**.
    Keep `.cpp`/`.h` edits with a binary python script and `assert count == 1` per
    replacement; do not use editors that rewrite line endings.
  - Windows console is cp1252: run python with `PYTHONIOENCODING=utf-8`.
  - **No commits / `git add` / `push`** unless the user asks.
  - **Never kill `node` processes** (other threads run dev servers in this checkout).
- Anti-patterns already paid for (do not reintroduce):
  - no new control flow inside `LoadAllRequestedModels` (Asyncify);
  - `BlendAnimation` restarts a finished clip — only (re)request poses when needed;
  - a partial pose eats the translation of non-partial clips by `1 - totalBlendAmount`;
  - `bIsDucking` nails the player in place and kills arm IK.
  Details in `.agents/plans/agachado-sa-handoff.md` §8.

## 7. Quick Reference

| Rule | Do | Avoid |
| --- | --- | --- |
| Feature flags | Port behind a `VICEEXT_*` define | Hard-coding gameplay changes unguardedly |
| New anims/models | Verify with the tools, add at enum tail | Inventing IDs or clip names |
| Units | `× METERS_PER_SECOND_TO_GAME_SPEED` | Raw m/s in engine fields |
| Editing .cpp | python binary + `assert count==1` + **detect CRLF/LF del fichero** | Asumir CRLF, o un editor que reescriba los finales |
| Validation | `check-served-build.sh` + `viceext-log-check.py` | Running smoke tests without asking |
| Tags | `VERSION` every build; `dataTag` only on data change | Same tag after a relink (cached wasm) |
| Git | Wait for an explicit request | Committing/staging on your own |
| Comments | Document in the plan, not in the code | Lane headers, explanations or notes in what you write or modify |

**Last updated:** September 2026
**Version:** 1.0 (adapted to this repo)
