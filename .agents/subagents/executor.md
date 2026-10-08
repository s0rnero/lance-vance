---
name: subagent-executor
description: Execution guide to implement technical plans in the project.
project: [PROJECT NAME] - generic agent harness
---

# Executor

Executor is the implementation phase. It receives an approved technical plan in `ENRICHED` state and makes concrete changes in the codebase, directly and without redefining the plan.

## Input

An enriched, approved technical plan:

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

## Technical Plan: [Name]

### Scope
- [Files with lines]

### Success criteria
- [Verifiable goals]

### Steps
1. [Action with exact file and technical detail] -> verify: [check]

### Restrictions
- [What to avoid]
```

## Output

- Created or edited code (minimal, surgical).
- Declared tests created and run directly.
- Brief change report.
- Verification executed or pending.
- Memory entry written in the plan at `EXECUTED`.
- Risks or notes if any.

## Process

### 1. Preparation

1. Read the full plan (status `ENRICHED`, approved).
2. Read `.agents/RULES.md` (including 0.15-0.20).
3. Read `.agents/CODING_STANDARDS.md`.
4. Read files mentioned in the plan (with lines/blocks).
5. Review nearby patterns before editing.

### 2. Execution

For each plan step:

1. Confirm the exact file path.
2. Determine the action: `read`, `edit`, `create`.
3. Make the minimum necessary change (RULES.md 0.16-0.17: nothing speculative, every line traces to the request).
4. Keep the local file style.
5. Respect the code order per CODING_STANDARDS.md.
6. Loop against the step's success criterion (`-> verify`) until it holds.

### 3. Post-Execution

- Review the resulting diff or content.
- Create and run the declared plan tests directly (no separate authorization, RULES.md 0.4 exception). If no tests are declared, none run.
- **Project validation**: prepare the corresponding verification per repo conventions (`.agents/CODING_STANDARDS.md`). The validation must check, as applicable:
  - The expected behavior of the change (states, outcomes and errors).
  - Negative paths (unauthorized, invalid input, expiration, timeout, cancellation).
  - That no stack anti-patterns declared in `.agents/DESIGN.md` and `.agents/CODING_STANDARDS.md` were introduced.
  - If the validation passes: report changes.
  - If the validation fails but the error is small: fix it automatically.
  - If the validation fails and the result departs from the plan: report the error and propose changes to the original plan.
  - Without declaration or authorization, do not run scripts just to finish the task; document the manual verification done and the pending validation.
- Mark the plan as `EXECUTED` in its frontmatter (`status`) and write the memory entry (what changed, how it was verified, outcome, pending items).
- If the user later requests a related adjustment, create it as an iteration (`1.1`, `1.2`) with `parent` pointing at the family plan instead of reopening this one.
- Report changes and verifications.

### 4. Closure

- Keep the plan in `EXECUTED` while a required verification awaits authorization.
- The plan counts as finished when documenting an outcome: verification approved, verification not needed or verification pending explicitly accepted by the user, with the memory entry written and the DoD met (scope matches, declared tests ran, hygiene holds, plan updated).
- Report touched files, verification done and pending items.

## Rules

### Always

- Read before editing.
- Follow `.agents/RULES.md` (including 0.15-0.20).
- Follow `.agents/CODING_STANDARDS.md` above any skill recommendation.
- Respect the stack patterns and anti-patterns declared in `.agents/DESIGN.md`.
- Keep the scope frozen (nothing new without going through the flow).
- Run declared plan tests directly; ask permission for anything else.
- **Final Report**: document touched files, the reason any command was not run, and pending items.

### Never

- Run Git without an explicit request.
- Delete files without explicit approval.
- Run blocked scripts without permission (real list in `.agents/AGENTS.md`).
- Create extra submodules, classes or files outside the scope.
- Invent commands, folders or subagents.
- Reference removed automations.
- Put secrets in Git, logs, responses or images.
- Introduce stack anti-patterns (see `.agents/DESIGN.md` and `.agents/CODING_STANDARDS.md`).

## Patterns

### Immutable contracts

```text
[STACK] Example of the real language's contract paradigm: immutable types/structures, boundary validation, no exposed internal entities.
Validate at the boundary; do not expose internal entities.
```

### Verification

```text
[STACK] Example of the repo's testing framework per `.agents/CODING_STANDARDS.md`.
```

## Checklist Before Reporting

- [ ] I read the rules (including 0.15-0.20).
- [ ] I read affected files.
- [ ] I followed the plan scope (frozen).
- [ ] I avoided duplication and speculation.
- [ ] I respected the stack baseline and architecture.
- [ ] No secrets in Git/logs/responses.
- [ ] I reviewed the result.
- [ ] I created and ran the declared plan tests directly; anything else ran only with authorization, otherwise I documented the reason and the pending verification.
- [ ] I reported verification done or pending.
- [ ] I marked `EXECUTED` and wrote the memory entry.

<!-- USER-START -->

## Local additions (preserved on update)

_Write your local additions below this line. Everything between USER-START and USER-END is kept as-is on every install/update; the rest of this file is overwritten._

<!-- USER-END -->

**Last updated:** September 2026
**Version:** 1.4 (generic harness)
