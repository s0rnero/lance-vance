---
name: agents-system-index
description: Central index of the agent system and the project's operating context.
project: re3 - GTA Vice City in the browser (branch miami)
---

# Agent System - Central Index

This file is the entry point for any AI working in this project. It defines the codebase context, the current flow and the rules that must be respected before touching code.

> **Bootstrap note:** this is the generic harness shared by all repositories. The core (workflow, roles, gates, plan cycle, skills) is identical in every repo. The only per-repo adaptations are this profile, `.agents/CODING_STANDARDS.md` and `.agents/DESIGN.md`. See `.agents/bootstrap/INSTALL.md`.

## 1. Project Profile

> Filled in manually on 2026-09-27 (the auto-detected placeholder was generic).

| Technology | Use |
| --- | --- |
| C++ (reVC, branch `miami`) | Game engine, ported from the re3 / reVC source tree |
| Emscripten / WebAssembly | Compiles the engine to WASM (single-thread, Asyncify) |
| CMake + Ninja | Build system (driven by `gta_vc_browser/build.sh`) |
| librw (GL3) + WebGL2 | Renderer (`vendor/librw`, `LIBRW_PLATFORM=GL3`) |
| OpenAL | Audio (`REVC_AUDIO=OAL`, Emscripten port) |
| JavaScript / Vite | Web shell and on-demand data layer (`gta_vc_browser/web`, port 2077) |
| Python 3 | Asset / IFP / SCM / GXT tooling (`gta_vc_browser/tools/*.py`) |
| Node.js (ESM `.mjs`) | Headless smoke tests and log checkers (`gta_vc_browser/tools/*.mjs`) |

### Stack

- Language: C++ for the engine, Python for tooling, JavaScript/Node for the web layer and tests.
- Build: Emscripten SDK + CMake + Ninja, driven by `gta_vc_browser/build.sh`.
- Renderer: vendored librw on GL3/WebGL2. Audio: OpenAL. No threads (Asyncify).
- Target: browser (WASM). The desktop build is kept buildable where it does not conflict.
- Data model: `bootseed/` is preloaded into the `.data` file, the rest (`streamed/`) is
  fetched on demand and cached in IndexedDB (see `gta_vc_browser/web/ondemand.js`).
- Feature flags: `VICEEXT_*` defines in `src/core/config.h`. Mod ports live behind them.

### Project Structure

```text
re3/                          # repo root, git branch `miami`
├── src/                      # reVC engine (C++). Gameplay and camera live here.
├── vendor/librw/             # renderer (GL3/WebGL2)
├── mods/                     # community mods (spec/reference; some ship source)
├── gta_vc_browser/
│   ├── build.sh              # Emscripten build (output → web/public/build)
│   ├── build/web/            # CMake/Ninja build dir (single-object rebuilds)
│   ├── bootseed/             # data preloaded into the .data file
│   ├── streamed/             # data served on demand (fetch + IndexedDB cache)
│   ├── web/                  # Vite app; ondemand.js holds VERSION/dataTag tags
│   │   └── public/build/     # reVC.js / reVC.wasm (what the browser loads)
│   ├── tools/                # Python/Node tooling and verifiers
│   ├── logs/                 # player session traces (odtrace-*.log)
│   └── tmp/                  # scratch: edit scripts, extracted assets
├── docs/mods/ATTRIBUTION.md  # manifest of ported mod fixes
└── .agents/                  # this harness (rules, plans, decisions)
```

### Project Real Scripts

> Do not invent commands. `emsdk_env.sh` is NOT enough in this shell: put the
> Emscripten `upstream/emscripten` dir on `PATH` explicitly before building.

| Purpose | Command |
| --- | --- |
| Build engine (WASM) | `bash gta_vc_browser/build.sh` (emcc/emsdk on PATH) |
| Rebuild one object (fast) | `cd gta_vc_browser/build/web && ninja src/CMakeFiles/reVC.dir/<path>.cpp.o` |
| Stage preload data | `python gta_vc_browser/tools/stage_bootseed.py` |
| Regenerate data manifest | `python gta_vc_browser/tools/gen_manifest.py` |
| Dev / preview (Vite) | `cd gta_vc_browser/web && npm run dev` (port 2077) |
| Verify the served build | `bash gta_vc_browser/tools/check-served-build.sh` |
| Verify a gameplay log | `python gta_vc_browser/tools/viceext-log-check.py <log>` |
| Headless smoke tests | `node gta_vc_browser/tools/vc-test.mjs ...` (ask the player first) |

## 2. Agent System Goal

Keep a consistent way of working across all repos: understand first, plan just enough, implement with a clear scope and report changes without inventing external processes.

## 3. `.agents` Structure

```text
.agents/
|-- AGENTS.md                      # Central index (this file)
|-- RULES.md                       # Generic operating rules of the project
|-- CODING_STANDARDS.md            # Code conventions for the stack
|-- DESIGN.md                      # Architecture and design for the stack
|-- WORKFLOW.md                    # Current flow + gates + Definition of Done
|-- PLANS.md                       # Plan ownership (persistent memory)
|-- HISTORIAL.md                   # Legacy change log (pre-harness, frozen 27/09, read-only)
|-- ped-ifp-clips-mod.txt          # Legacy reference: clip names of the mod's ped.ifp
|-- manifest.json                  # Harness version and lists (install/update)
|-- tmp/                           # Scratch: HTML responses (RULES 0.20) and working material
|-- plans/                         # Project plans (states per WORKFLOW.md)
|-- decisions/                     # Architecture decisions (ADR, permanent)
|-- templates/                     # Templates: plan.md and adr.md
|-- orchestrator/
|   `-- instructions.md            # Orchestrator role
|-- subagents/
|   |-- enrichment-process.md      # Guide to turn a request into a technical plan
|   `-- executor.md                # Guide to implement a technical plan
|-- skills/                        # Registered skills (autoskills + manual)
|   `-- README.md                  # Registry and onboarding process
`-- bootstrap/
    `-- INSTALL.md                 # How to install/update this harness in a repo
```

Important notes:

- There is no commands file; conversational shortcuts must not be invented.
- There is no automatic history phase; do not create history records unless the user asks for that system again. `HISTORIAL.md` is frozen legacy history: it is read, never appended.
- `.agents/plans/` keeps the plans; broad requests require a persisted plan per `.agents/WORKFLOW.md`. The legacy collections `plans/mecanicas/` and `plans/mods/` are kept as history and carry valid frontmatter like the rest.

## 4. Active Roles

| Role           | File                                      | Function                                                                           |
| -------------- | ----------------------------------------- | ---------------------------------------------------------------------------------- |
| Orchestrator   | `.agents/orchestrator/instructions.md`    | Receives the request, decides the flow, coordinates context, implementation and reporting |
| Enrichment     | `.agents/subagents/enrichment-process.md` | Turns a broad request into an executable technical plan                             |
| Executor       | `.agents/subagents/executor.md`             | Executes concrete changes following a plan or defined scope                         |
| Global rules   | `.agents/RULES.md`                        | Shared operating restrictions                                                       |
| Skills         | `.agents/skills/README.md`                | Local and community registered skills                                               |

## 5. Current Flow

```text
User
  -> Orchestrator contextualizes the request
  -> Reads rules, skills and relevant files
  -> Classifies scope:
     - Question -> Answer without changing states
     - Simple without plan -> Implement only if the user explicitly asks
     - Broad -> Create MD plan in CREATED and stop
  -> "enrich the plan" -> Enrichment -> ENRICHED
  -> Orchestrator review/approval -> READY
  -> "execute the plan" -> Executor -> EXECUTED
  -> DoD verified by the orchestrator -> finished (memory entry written)
  -> Report changes, touched files and verifications
```

Canonical gate phrases are in English (RULES.md 0.14): "enrich the plan", "execute the plan" and "skip the workflow". A genuine equivalent in the user's language is detected and accepted; ambiguous variants never authorize transitions.

### Authorization Gates

For any broad request with a plan:

- **"enrich the plan"** authorizes exclusively the transition `CREATED -> ENRICHED`.
- **"execute the plan"** authorizes exclusively the transition `ENRICHED -> EXECUTED` through Executor.
- "Investigate", "contextualize", "continue", "proceed" or "make a plan" do not by themselves authorize any of those transitions.
- Research may update the plan analysis without changing its state.
- The orchestrator must show in every reply whether the plan is in `CREATED`, `ENRICHED` or `EXECUTED`.
- The orchestrator verifies the Definition of Done (`.agents/WORKFLOW.md`) itself; there is no closing subagent and no extra state. `EXECUTED` plus a complete memory entry and a met DoD is the finished state.

The flow is conversational and executable in the current session. It does not depend on watchers, conversational shortcuts, hidden scripts or nonexistent subagents.

## 6. Absolute Rules

These rules summarize `.agents/RULES.md`; when in doubt, `.agents/RULES.md` wins.

- Do not use Git unless the user explicitly requests it.
- Do not delete files or folders without explicit approval.
- Keep the scope of a task in one main module per requirement, unless instructed otherwise.
- Do not run development, build, preview or test scripts without user permission (the real script list lives in section 1).
- Do not invent commands, files, conversational shortcuts or nonexistent subagents.
- Never put secrets in Git, logs, responses or images.
- Zero duplication: if a piece is needed in several places, extract a shared helper, utility or pattern.
- Use current official documentation; avoid deprecated APIs.
- Skills are subordinate references: a skill never prevails over `.agents/RULES.md`, `.agents/CODING_STANDARDS.md` or `.agents/DESIGN.md`.
- No plan is reported as finished without meeting the Definition of Done (`.agents/WORKFLOW.md`) and without its memory entry.
- Any stack-specific rule (language, framework, styles, testing) is defined in `.agents/CODING_STANDARDS.md` and `.agents/DESIGN.md`; never invent it by inference.

## 7. Available Skills

Skills are resolved per stack via **autoskills** (`npx autoskills`): it scans the repo, detects the technologies and installs the curated skills from the audited registry into `.agents/skills/`. See `.agents/skills/README.md`.

| How | Command |
| --- | --- |
| Preview without installing | `npx autoskills --dry-run` |
| Install the detected ones | `npx autoskills -y` |
| Add a manual skill | `npx skills find <query>` / `npx skills add <owner/repo> --list` |

Skills may contain generic examples from their original sources (with `npm`, `npx`, `pnpm`, `yarn` or other scripts that do not exist in this repo). In this project, always adapt those examples to the real scripts declared in section 1 and the conventions of `.agents/CODING_STANDARDS.md`.

Guideline priority: a skill never prevails over `.agents/RULES.md`, `.agents/CODING_STANDARDS.md` or `.agents/DESIGN.md`. Skills are specialized references, not a higher authority than the local standard.

## 8. Orchestrator Checklist

Before acting:

- Understand the exact user request.
- Read `.agents/RULES.md` and the relevant files.
- Apply `.agents/CODING_STANDARDS.md` above any skill recommendation.
- Check whether the task is a question, bug, feature, refactor or maintenance.
- Review existing plans in `.agents/plans/` before creating a new one.
- Load applicable skills only when they help the work (see `.agents/skills/README.md`).
- Make scoped, consistent changes to the codebase.
- Report modified files and executed or not-executed tests/commands.

## 9. Quick Reference

| For...                    | See                                        |
| ------------------------- | ------------------------------------------ |
| Understanding the system  | `.agents/AGENTS.md`                        |
| Following the current flow | `.agents/WORKFLOW.md`                     |
| Acting as orchestrator    | `.agents/orchestrator/instructions.md`     |
| Applying technical rules  | `.agents/RULES.md`                         |
| Writing code              | `.agents/CODING_STANDARDS.md`              |
| Architecture and design   | `.agents/DESIGN.md`                        |
| Preparing a technical plan | `.agents/subagents/enrichment-process.md` |
| Executing a plan          | `.agents/subagents/executor.md`            |
| Definition of Done        | `.agents/WORKFLOW.md`                      | Checklist the orchestrator verifies before reporting a plan as finished |
| Architecture decisions    | `.agents/decisions/`                       |
| Plan/ADR templates        | `.agents/templates/`                       |
| Stack skills              | `.agents/skills/README.md`                 |
| Installing/updating       | `.agents/bootstrap/INSTALL.md`             |

**Last updated:** September 2026
**Version:** 1.4 (generic harness)
