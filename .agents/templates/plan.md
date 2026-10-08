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

> Copy to `.agents/plans/` as `YYYY-MM-DD-<fam>.<iter>-<slug>.md` and fill in. States: CREATED -> ENRICHED -> EXECUTED (see `.agents/WORKFLOW.md`). `1` = main plan; `1.1`, `1.2` = iterations bound to what family `1.0` executed (set `parent`).

## Objective

- [Expected, measurable outcome]

## Scope

- [Affected files or modules]
- [Out of scope: what is NOT touched]

## Restrictions

- [What not to do / rules to apply: RULES.md, CODING_STANDARDS.md, DESIGN.md]

## Assumptions

- [Explicit assumption + what happens if wrong]

## Success criteria

- [Verifiable goal per task]

## Steps

1. [Executable step] -> verify: [check]

## Verification

- [Declared commands, run directly at execution, or manual review]

## Closure (persistent memory)

> Fill in at EXECUTED. This is the project's memory entry (see `.agents/PLANS.md`). Without it, the plan is not finished (DoD).

- What changed: [real summary]
- Verification: [declared tests run directly + result; anything else: command executed and result, or pending with reason]
- Outcome: [done / pending explicitly accepted]
- Pending items: [if any]

<!-- USER-START -->

## Local additions (preserved on update)

_Write your local additions below this line. Everything between USER-START and USER-END is kept as-is on every install/update; the rest of this file is overwritten._

<!-- USER-END -->
