---
name: workflow-automation
description: Current agent system workflow. It does not rely on removed automations.
project: [PROJECT NAME] - generic agent harness
---

# Agent System Workflow

This document describes the real flow the orchestrator must follow within the current session. The system uses no conversational shortcuts, automatic watchers or automatic history phases. This flow is identical in every repository using this harness.

## Base Principle

The orchestrator must not simulate automations that do not exist. If a task requires code, the agent reads context, prepares the scope, implements and reports. If a task requires a verification the rules block, ask for permission or leave it explicitly reported.

Without explicit gates nothing in the workflow advances; the single exceptions are the exact phrase "skip the workflow" and the wildcard "advance the workflow" (see Explicit Transition Gates and RULES.md 0.13). Without gates the agent works as a normal chat: questions and simple tasks, following the conversation.

## Main Flow

```text
1. Receive user request
2. Classify: question, bug, feature, refactor, maintenance or research
3. Read minimal context:
   - .agents/AGENTS.md
   - .agents/RULES.md
   - .agents/CODING_STANDARDS.md
   - affected source files
   - relevant skill, if any
4. Classify scope:
   - Question -> answer without changing states or files
   - Simple (1-2 changes) -> execute only if the user explicitly asks to implement
   - Broad (a list of things: multiple changes/files) -> iterate with the user (human loop),
     then create a CREATED plan directly (creation is inferred) and stop
5. Enrich only through the enrichment gate or the wildcard
6. Execute only through the execution gate (or wildcard / "skip the workflow") with an approved ENRICHED plan
7. Verify: declared plan tests run directly; anything else needs authorization
8. Write the memory entry at EXECUTED and meet the DoD before finishing
9. Report the final result (markdown by default, concise, visual per RULES.md 0.20)
```

## Explicit Transition Gates

Lifecycle: `CREATED` -> `ENRICHED` -> `EXECUTED`, one gate per transition:

- **Creation** — inferred, not gated: a list of things means a plan is the only option. The human loop happens BEFORE the plan exists: iterate (what they want, what they need, how yes / how no) until the spec is clear; then write it point by point and stop. **"create the plan"** / **"add it to a plan"** also trigger it explicitly.
- **Enrichment** — **"enrich the plan"** / **"enrich the plans"**: `CREATED` -> `ENRICHED` (technical, granular, no implementation).
- **Execution** — **"execute the plan"**: approved `ENRICHED` -> `EXECUTED` (direct implementation).
- **Wildcard** — **"advance the workflow"** advances from the current phase without naming it; valid in all 3 phases, never inferred from other phrases.
- **Bypass** — **"skip the workflow"**, spoken exactly by the user, runs the full cycle (create -> enrich -> review -> execute -> closure + DoD) in a single pass, keeping review, DoD and per-phase reporting.

Rules:

- Creating a plan is inferred from a list of things; updating an initial plan leaves it `CREATED`; neither starts enrichment or execution. For broad requests the persisted plan is mandatory.
- Researching, reading files, consulting documentation or reproducing a problem does not change the plan state.
- Ambiguous variants ("investigate", "contextualize", "continue", "proceed", "do it", "make a plan") never authorize transitions.
- Scope freezes after `CREATED`: `ENRICHED` and `EXECUTED` cover only what is defined. Anything new goes through the flow (question what is unclear; once clear, add it to the plan per its state; if `ENRICHED`, re-enrich what was added).
- Executor cannot be invoked from a `CREATED` plan.
- Non-declared scripts still require independent authorization even when execution is authorized (RULES.md 0.4 exception covers only declared plan tests).
- Finishing is not authorized by a phrase: it is reached only when the DoD is met and the memory entry is written at `EXECUTED`.
- After a plan is executed, if no adjustments to what was done are requested, the agent returns to the start: it interprets the conversation and awaits either a new plan or a simple instruction.

Canonical gate phrases are in English (RULES.md 0.14). If the user expresses the same gate phrase in their native language, the orchestrator detects it and takes it as valid for the same transition; ambiguous variants never count.

The orchestrator must confirm in its reply which gate it is attending and stop when the required phrase is absent.

If a skill contradicts `.agents/CODING_STANDARDS.md`, the flow must follow `.agents/CODING_STANDARDS.md`. Skills only complement local judgment.

## Simple Flow (1-2 changes)

For small, clear requests:

```text
1. Read minimal context (rules, standards, files)
2. Implement the change directly
3. Verify inline: RULES.md, CODING_STANDARDS.md, DESIGN.md
4. Report the result
```

No MD plan is created. No subagents are invoked. The orchestrator assumes all verifications.

## Full Plan Flow (Broad Request)

For requests with multiple steps or touching multiple files:

```text
1. Create MD plan in .agents/plans/
   - Frontmatter: name (`<fam>.<iter>-<slug>`), status (CREATED), type, domain, parent (iterations only), owner_rules, created
   - Filename: `YYYY-MM-DD-<fam>.<iter>-<slug>.md` (`1` = main plan; `1.1`, `1.2` = iterations of family `1.0`)
   - Content: objective, scope, files, restrictions, assumptions, success criteria, steps (`step -> verify`), verification
2. Wait for the enrichment gate or the wildcard, then invoke Enrichment
   - Pass plan MD + context (rules, standards, source files)
   - Enrichment turns the spec into atomic technical steps (files, lines, actions); it does not implement
3. Orchestrator critical review (MANDATORY)
   - Verify it fulfills exactly what the user asked
   - Verify it invents no unsolicited changes
   - Verify it respects RULES.md, CODING_STANDARDS.md, DESIGN.md
   - Verify it introduces no stack anti-patterns (see CODING_STANDARDS.md)
   - Verify every step has success criteria (`step -> verify`)
   - If it fails: reject, correct or re-enrich
4. If approved: wait for the execution gate, the wildcard or "skip the workflow"
5. Then invoke Executor to implement the changes directly
6. Declared plan tests are created and run directly, no separate authorization; if no tests are declared, none run
7. Record `EXECUTED` when the implementation finished, write the memory entry, and meet the **Definition of Done**
```

## Using Plans

`.agents/plans/` is mandatory for broad requests.

### When to create an MD plan

- The request has 3+ steps or touches multiple files/areas.
- The user explicitly asks for a plan.
- The change is complex enough to need technical enrichment.
- The task may be resumed later.

### When NOT to create an MD plan

- A simple 1-2 step change (1-2 files, point fix).
- An informational question.
- When creating the plan would add noise without value.

### Plan Format

Use `.agents/templates/plan.md` as the base (it includes the closure entry as persistent memory).

```markdown
---
name: 1.0-plan-name
status: CREATED
type: feature|bugfix|refactor|maintenance|research
domain: <project-domain>
parent: [optional: family plan this iteration belongs to, e.g. 1.0-plan-name]
owner_rules: .agents
created: YYYY-MM-DD HH:mm
---

# Plan: [Name]

## Objective
- [Expected outcome]

## Scope
- [Affected files or modules]
- [Out of scope: what is NOT touched]

## Restrictions
- [What not to do]

## Assumptions
- [Explicit assumption + what happens if wrong]

## Success criteria
- [Verifiable goal per task]

## Steps
1. [Executable step] -> verify: [check]

## Verification
- [Declared commands, run directly at execution, or manual review]
```

User plans are written in the language the user writes their prompts in (RULES.md 0.14). Structural fields (`name`, `status`, `type`, `domain`, `parent`, `owner_rules`, `created`) and lifecycle states remain in English.

### Plan States (Lifecycle)

- **CREATED:** Specs written point by point from the human loop, awaiting enrichment.
- **ENRICHED:** Enriched technically (files, lines, atomic steps), awaiting orchestrator review and user approval.
- **EXECUTED:** Implemented and finished, with the memory entry written and the DoD met (or the reason documented).

**Cycle:** `CREATED` -> `ENRICHED` -> (Orchestrator reviews, user approves) -> `EXECUTED`

**Naming:** file `YYYY-MM-DD-<fam>.<iter>-<slug>.md`; `1` = main plan; `1.1`/`1.2` = iterations bound to what family `1.0` executed, referencing it via `parent`.

## Definition Of Done (DoD)

No plan counts as finished without meeting the DoD. Mandatory checklist:

- [ ] The implemented code matches the approved plan: no unsolicited changes or extra scope.
- [ ] Declared plan tests were created and run directly; anything else ran with authorization or was reported as pending per rule 0.4 with the exact reason.
- [ ] No secrets, debug logs, comments, dead code or duplication were introduced.
- [ ] Touched files respect `.agents/CODING_STANDARDS.md` and `.agents/DESIGN.md`.
- [ ] The plan was updated with the real changes and the verification outcome.
- [ ] The plan closure entry was written as persistent memory at `EXECUTED` (see `.agents/PLANS.md`): what changed, how it was verified, outcome and pending items.

Finishing without DoD is a rule violation. The orchestrator verifies scope, declared tests, hygiene, updated plan and memory entry before finishing.

## Enrichment

Use `.agents/subagents/enrichment-process.md` as the guide when the request is broad. This phase starts only through the enrichment gate or the wildcard (RULES.md 0.13). Enrichment analyzes, researches and details at file/line granularity; it does not implement code nor run scripts.

Input:

- Plan MD + additional context.

Output:

- Enriched technical plan with files, actions, restrictions and verification.

The orchestrator MUST critically review the output before approving.

## Executor

Use `.agents/subagents/executor.md` as the guide for implementation. This phase starts only with an approved `ENRICHED` plan through the execution gate, the wildcard or "skip the workflow" (RULES.md 0.13). Executor implements directly within the approved scope; it does not redefine the plan. Declared plan tests are created and run directly without separate authorization; if no tests are declared, none run.

Responsibilities:

- Read files before editing them.
- Touch only the necessary scope.
- Follow `.agents/RULES.md` and `.agents/CODING_STANDARDS.md`.
- Pass the stack rules quality gate before reporting.
- Create and run declared plan tests directly (no separate authorization); anything else only with authorization.
- Write the memory entry in the plan at `EXECUTED`.
- Report changes.

## Verification

The real project scripts are listed in `.agents/AGENTS.md` (section "Project Real Scripts"). Current rule: checks and tests declared in an `ENRICHED` plan run directly at execution (RULES.md 0.4 exception). Anything else: ask the user for permission or report it as pending by restriction.

Safe read-only inspection commands such as file reading, search, listings and spot checks may be used to contextualize the work.

## Final Report

The final report follows `.agents/RULES.md` 0.20 (short -> text; default -> markdown; heavy visual -> HTML file in `.agents/tmp/`) and must include:

- What changed.
- Modified files.
- Verification executed or the reason it was not.
- Risks or pending items, if any.

Do not reference automations, files or shortcuts that do not exist in the current `.agents` tree.

## Summarized Flow

```text
User -> Orchestrator -> Context
  -> [Simple?] -> Execute directly -> Inline verification -> Report
  -> [Broad?] -> Human loop -> CREATED plan -> Enrichment -> Orchestrator review + approval
                                   -> Executor (+ declared tests) -> EXECUTED + memory + DoD -> Report
```

<!-- USER-START -->

## Local additions (preserved on update)

_Write your local additions below this line. Everything between USER-START and USER-END is kept as-is on every install/update; the rest of this file is overwritten._

<!-- USER-END -->

**Last updated:** September 2026
**Version:** 1.4 (generic harness)
