---
name: 005-measurement-player-sessions-and-logs
status: accepted
date: 2026-09-26
domain: process
---

# ADR-005: Gameplay measurement is the player's sessions plus logs

## Context

Headless smoke tests (`node gta_vc_browser/tools/*.mjs`, Puppeteer + SwiftShader)
were used earlier. They are slow, CPU-heavy, contaminate the measurement when another
browser is running, and the world stays alive during them (cars run the ped over and
cancel the crouch). They also convinced nobody: several "no improvement" reports were
actually the **old** engine being served. The player asked explicitly to stop them.

## Decision

Measure from the **player's own gameplay**: he plays, the engine writes traces to
`gta_vc_browser/logs/odtrace-*.log`, and the verifiers
(`tools/viceext-log-check.py`, `tools/check-served-build.sh`) dictaminate from that.
No URL/console levers. The **criterion PASS is defined before he plays**. Headless
smoke tests remain in the repo but are only run if the player asks.

## Consequences

- Measurement matches what the user actually sees; no CPU contamination.
- Everything measurable must emit a trace (numbers, states) or be judged by ear/eye.
- The ritual is mandatory: bump `VERSION` on each build and pass
  `check-served-build.sh` **before** asking for a session, so the served wasm really
  carries the change (the same tag is served from cache).

## Alternatives Considered

- **Keep headless harnesses as the primary method**: rejected — slow, contaminated,
  and repeatedly mis-measured a stale build.
- **Add URL/console levers**: rejected by the player.

## State

`accepted`.
