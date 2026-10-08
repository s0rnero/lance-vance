# Plans

Project plans directory. Every broad request must have an MD plan persisted here per `.agents/WORKFLOW.md`.

`.agents/plans/` is the project's persistent memory: plans are never deleted, emptied or archived outside the repo. Every change or modification must be documented in a plan before being executed. A closed plan remains as permanent history.

Mandatory frontmatter format:

```markdown
---
name: 1.0-plan-name
status: CREATED
type: feature|bugfix|refactor|maintenance|research
domain: <project-domain>
parent: [optional: family plan, e.g. 1.0-plan-name]
created: YYYY-MM-DD HH:mm
---
```

Full template (with mandatory body and closure entry as persistent memory): `.agents/templates/plan.md`.

States: `CREATED` -> `ENRICHED` -> (Orchestrator reviews, user approves) -> `EXECUTED`.

Files are named `YYYY-MM-DD-<fam>.<iter>-<slug>.md` (`1` = main plan; `1.1`, `1.2` = iterations of family `1.0`).

No plan counts as finished without meeting the Definition of Done (`.agents/WORKFLOW.md`) and without its memory entry at `EXECUTED`. Architecture decisions are registered separately in `.agents/decisions/` (rule 0.12 of `.agents/RULES.md`).

User plans are written in the language the user writes their prompts in (RULES.md 0.14); structural fields and states remain in English.

See `.agents/PLANS.md` for ownership and `.agents/WORKFLOW.md` for the flow and gates.

<!-- USER-START -->

## Local additions (preserved on update)

_Write your local additions below this line. Everything between USER-START and USER-END is kept as-is on every install/update; the rest of this file is overwritten._

<!-- USER-END -->
