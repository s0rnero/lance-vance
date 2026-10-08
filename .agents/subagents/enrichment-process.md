---
name: enrichment-process
description: Enrichment guide to turn general requests into executable technical plans.
project: [PROJECT NAME] - generic agent harness
---

# Enrichment Process

Enrichment is the role/subagent in charge of the analysis phase. Its goal is to turn a plan in `CREATED` state into a sufficiently clear technical plan, updating its state to `ENRICHED` when done. It does not implement code nor run scripts.

## Input

An MD plan created by the orchestrator:

```markdown
---
name: 1.0-plan-name
status: CREATED
type: feature
domain: <project-domain>
parent: [optional: family plan]
owner_rules: .agents
created: YYYY-MM-DD HH:mm
---

# Plan: [Name]

## Objective

- [What the user wants to achieve]

## Scope

- [Affected files]

## Restrictions

- [What not to do]

## Assumptions

- [Explicit assumption]

## Success criteria

- [Verifiable goal]

## Steps

1. [General step] -> verify: [check]

## Verification

- [How to verify]
```

## Output

An enriched technical plan:

```markdown
---
name: 1.0-plan-name
status: ENRICHED
type: feature
domain: <project-domain>
parent: [optional: family plan]
owner_rules: .agents
created: YYYY-MM-DD HH:mm
---

# Technical Plan: [Name]

### Analysis

- Objective:
- Scope:
- Files (with lines/blocks):
- Assumptions:
- Risks:

### Changes

- [Concrete change per file, with lines]

### Restrictions

- [What to avoid]

### Success criteria

- [Verifiable goal per task]

### Steps

1. [Executable step with exact file, action and technical detail] -> verify: [check]

### Verification

- [Declared commands, run directly at execution, or manual review]

## Closure (persistent memory)

> Fill in at EXECUTED. Without this entry the plan is not finished (DoD).
```

## Process

### 1. Read the provided plan MD

Read the full plan including:

- Clear user objective.
- Defined scope (files, modules).
- Explicit restrictions.
- General steps.

### 2. Research Additional Context

Read at minimum:

| What to look for  | Where                                           |
| ----------------- | ----------------------------------------------- |
| Rules             | `.agents/RULES.md`                              |
| Standards         | `.agents/CODING_STANDARDS.md`                   |
| Architecture      | `.agents/DESIGN.md`                             |
| Repo profile      | `.agents/AGENTS.md` (stack, structure, scripts) |
| Existing plans    | `.agents/plans/`                                |
| Affected code     | Source files mentioned in the plan              |
| Relevant skills   | `.agents/skills/` (see README)                  |

### 3. Analyze And Detail

- Identify the user's exact objective.
- Determine affected files AND lines/blocks: map each spec point to the exact location that must change.
- Make the assumptions explicit (with what happens if wrong) and surface multiple interpretations with effort estimates.
- Review existing patterns before proposing changes.
- Flag risks or possible breaking changes.
- Strictly keep the original plan scope. If something new surfaces, question it first; once clear, add it to the plan and re-enrich what was added.
- Keep it minimal and surgical: challenge every step against RULES.md 0.16 (would a senior engineer call this overcomplicated?) and 0.17 (does every line trace to the request?).

Every step must have:

- Exact file.
- Action: `read`, `edit`, `create`.
- Concrete technical detail (lines/blocks when known).
- Success criterion: `-> verify: [check]`.
- Applicable restrictions.

### 4. Validate the plan against guidelines

Before emitting the plan as output, verify:

| Guideline        | What to validate                                                                                                                                                          | Reference                 |
| ---------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------ | ------------------------- |
| Architecture     | Changes respect the declared layers, modules and patterns                                                                                                                | `.agents/DESIGN.md`       |
| Coding standards | Structure, naming, contracts and code order per the stack                                                                                                                | `.agents/CODING_STANDARDS.md` |
| Rules            | No git, no blocked scripts, scope per requirement, secrets via environment, subordinate skills                                                                           | `.agents/RULES.md`        |
| Skills           | If the plan touches the repo stack, load the relevant skill from `.agents/skills/` and verify the plan does not contradict its recommendations, unless they contradict `.agents/CODING_STANDARDS.md` | `.agents/skills/` |
| Language         | Normative content in English; user plans in the language the user writes their prompts in (RULES.md 0.14)                                                                | `.agents/RULES.md` 0.14   |
| Behavior         | Assumptions explicit, nothing speculative, surgical scope, verifiable criteria per step                                                                                   | `.agents/RULES.md` 0.15-0.18 |

If the plan violates any guideline, correct it before emitting it. Do not pass plans that fail this validation.

Priority: `.agents/CODING_STANDARDS.md` prevails over any skill. Skills extend context but do not replace the local standard.

### 5. Mark As ENRICHED

Update the plan frontmatter with `status: ENRICHED` and return the result to the orchestrator.

## Rules

### Always

- Read the full plan MD before enriching.
- Consult `.agents/RULES.md`.
- Consult `.agents/CODING_STANDARDS.md`.
- Consult `.agents/DESIGN.md`.
- Read existing files before proposing edits.
- Use skills only when they add context.
- Be detailed without inflating the scope.
- Include the expected verification.
- Strictly keep the original plan scope (no unsolicited changes).

### Never

- Run code during enrichment.
- Invent files or commands.
- Reference removed automations.
- Propose nonexistent conversational shortcuts.
- Leave the user's scope.
- Add features or changes not requested in the original plan.

## Checklist

- [ ] I read the full plan MD.
- [ ] I read rules, standards and design (including RULES.md 0.15-0.20).
- [ ] I read the repo profile in `.agents/AGENTS.md`.
- [ ] I read affected files (with lines/blocks).
- [ ] I reviewed relevant skills.
- [ ] I kept the exact scope of the original plan.
- [ ] I listed executable steps with exact files and `-> verify` criteria.
- [ ] I included assumptions and restrictions.
- [ ] I included verification (declared commands).
- [ ] I validated against `.agents/DESIGN.md`.
- [ ] I validated against `.agents/CODING_STANDARDS.md`.
- [ ] I validated against applicable skills.
- [ ] I marked the state as `ENRICHED`.

<!-- USER-START -->

## Local additions (preserved on update)

_Write your local additions below this line. Everything between USER-START and USER-END is kept as-is on every install/update; the rest of this file is overwritten._

<!-- USER-END -->

**Last updated:** September 2026
**Version:** 1.4 (generic harness)
