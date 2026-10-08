---
name: agent-rules
description: Strict rules and operating protocols for project agents.
project: [PROJECT NAME] - generic agent harness
---

# Rules & Standards - Project Agents

These rules apply to any agent working in the codebase. They are the generic, shared part of the harness: they do not depend on the stack. Stack-specific rules (language, framework, styles, testing, architecture) live in `.agents/CODING_STANDARDS.md` and `.agents/DESIGN.md`, which each repo adapts.

## 0. Absolute Rules

### 0.1 Git

- Do not interact with Git unless the user explicitly requests it.
- If the user asks for a destructive Git operation, confirm before proceeding.

### 0.2 File Deletion

- Do not delete files or folders without explicit approval.
- If a file looks obsolete, report it and wait for instructions.

### 0.3 Scope Per Requirement

- Keep one main module (or unit) per requirement.
- Do not create extra submodules, classes or files unless explicitly requested or a clear technical need is explained to the user.
- Do not expand the scope with unsolicited changes.

### 0.4 Verification Scripts

- Do not automatically run project development, build, preview or test scripts.
- The real scripts are listed in `.agents/AGENTS.md` (section "Project Real Scripts"); do not invent commands that do not exist.
- If one needs to run, ask for permission or report that verification was left pending by project rule.
- Exception: checks and tests explicitly declared in an `ENRICHED` plan's Verification section run directly at execution without separate authorization. Anything not declared still requires permission.

### 0.5 Subordinate Skills

- Skills are specialized references, not a higher authority than the local standard.
- If a skill contradicts `.agents/RULES.md`, `.agents/CODING_STANDARDS.md` or `.agents/DESIGN.md`, the local standard prevails.
- Always adapt generic skill examples (npm/npx/pnpm/yarn) to the real scripts and conventions of the repo.

### 0.6 Zero Duplication

- Do not duplicate code blocks.
- If a piece is needed in several places, extract a shared helper, utility or pattern when the scope allows it.

### 0.7 Current Code

- Use current official documentation when in doubt.
- Avoid deprecated APIs.

### 0.8 Secrets

- Secrets only via environment/secret manager; never in Git, builds, logs, responses or published code.

### 0.9 Stack Verification

- Before suggesting APIs or patterns, verify the repo baseline (language, framework, package manager versions) declared in `.agents/AGENTS.md`.
- Do not introduce unplanned major versions by inference; they require a migration plan and approval.
- Do not reintroduce standards from stacks that do not belong to the repo.

### 0.10 Plans As Persistent Memory

- The plans in `.agents/plans/` are the project's persistent memory: they record decisions, context, progress and the outcome of every change.
- Always complete the workflow: a plan is never abandoned or discarded; if it stops being viable, it is finished (`EXECUTED`) documenting the reason.
- Never delete plans: do not remove the file, do not empty its content and do not archive it outside the repo. A finished plan remains as permanent history.
- Every change or modification of the project must be documented in the corresponding plan BEFORE being executed. Without a prior plan, nothing that requires a plan gets modified.
- The human loop lives only in `CREATED`: iterate with the user (what they want, what they need, how yes / how no) before the plan is even written; always ask at the start because the needed context exceeds what the user estimates.
- Scope freezes after `CREATED`: `ENRICHED` and `EXECUTED` cover only what is defined; anything new goes through the flow (question what is unclear, and once clear add it to the plan per its state; if `ENRICHED`, re-enrich what was added).

### 0.11 Definition Of Done

- No plan counts as finished without meeting the DoD in `.agents/WORKFLOW.md` (section "Definition Of Done").
- The DoD is verified with the tests and checks declared in the plan before finishing.
- If the DoD is not met, the plan stays `EXECUTED` with the reason documented; the memory entry is still written. A plan is never marked finished under pressure or by inference.

### 0.12 Architecture Decisions (ADR)

- Every decision that affects the project's architecture or design is recorded as an ADR in `.agents/decisions/` (template in `.agents/templates/adr.md`).
- Plans record the execution of a task; ADRs record the why of a decision and are permanent: they are never deleted nor edited retroactively; a change of decision is recorded as a new ADR that marks the previous one as `superseded`.
- An architecture decision made during a plan generates an ADR in addition to the plan closure.

### 0.13 Gates And Bypass

- Creation is inferred, not gated: if the request is a list of things (multiple steps, files or areas), creating a plan is the only option — run the human loop and create the `CREATED` plan directly. If it is a single change (one file, name or line of code), no plan is needed. Saying **"create the plan"**, **"add it to a plan"** also works explicitly.
- Enrichment and execution always require their gates. Without them the agent works as a normal chat (questions, simple tasks), following the conversation.
- Lifecycle: `CREATED` -> `ENRICHED` -> `EXECUTED`, one gate per transition from enrichment on:
  - Creation: inferred (list -> plan; single change -> direct), or explicit **"create the plan"**, **"add it to a plan"** (referencing the plan).
  - Enrichment: **"enrich the plan"**, **"enrich the plans"**.
  - Execution: **"execute the plan"**.
  - Wildcard: **"advance the workflow"** advances from the current phase without naming it; valid in all 3 phases, never inferred from other phrases.
- Single exception: if the user says exactly **"skip the workflow"**, the orchestrator may run the full cycle of that broad request (create -> enrich -> review -> execute -> closure + DoD) in a single pass, without waiting for the intermediate phrases.
- Enrichment and execution gate phrases are never inferred from "continue", "go on", "do it" or similar. The bypass does not remove the gates: it subsumes them into a single authorization. Phase reporting and the DoD remain mandatory.
- After a plan is executed, if no adjustments to what was done are requested, the agent returns to the start: it interprets the conversation and awaits either a new plan (a list of things) or a simple instruction.

### 0.14 Language Policy

- All normative and documentation content of the harness (rules, workflow, roles, subagents, templates, decisions, scripts, README) is written in **English** for general compatibility.
- **User plans** in `.agents/plans/` are written in **the language the user writes their prompts in**.
- Canonical gate phrases are in English: **"create the plan"**, **"enrich the plan"**, **"execute the plan"**, **"advance the workflow"**, **"skip the workflow"**. When the user expresses the same gate phrase in their native language (e.g. "crea un plan", "enriquece el plan", "ejecuta el plan", "avanza en el flujo", "omitir el workflow"), the orchestrator detects it and takes it as a valid instruction for the same transition. Ambiguous variants ("continue", "procede", "hazlo", "go on") never authorize anything; only the genuine gate phrase, in English or in the user's language, does.

### 0.15 Think Before Coding

- Don't assume. Don't hide confusion. Surface tradeoffs.
- State assumptions explicitly. If uncertain, ask instead of guessing.
- If multiple interpretations exist, present them with effort estimates; never pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what is confusing and ask.

### 0.16 Simplicity First

- Minimum code that solves the problem. Nothing speculative.
- No features beyond what was asked. No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If 200 lines could be 50, rewrite it. The test: would a senior engineer call this overcomplicated? If yes, simplify.

### 0.17 Surgical Changes

- Touch only what you must. Clean up only your own mess.
- Don't "improve" adjacent code, comments or formatting. Don't refactor what isn't broken.
- Match the existing style, even if you would do it differently.
- If you notice unrelated dead code, mention it; don't delete it.
- Remove imports, variables or functions that YOUR changes made unused. Never remove pre-existing dead code unless asked.
- The test: every changed line must trace directly to the user's request.

### 0.18 Goal-Driven Execution

- Define success criteria. Loop until verified.
- Transform imperative tasks into verifiable goals ("fix the bug" -> "write a test that reproduces it, then make it pass"; "refactor X" -> "tests pass before and after").
- Reproduce first: write the failing test before fixing.
- Multi-step tasks state a brief plan with verification per step (`1. [Step] -> verify: [check]`). Each step is independently verifiable.
- Strong criteria let the agent loop independently; weak criteria ("make it work") require clarification.

### 0.19 Tradeoff And Signals

- These rules bias toward caution over speed. For trivial tasks (typo fixes, obvious one-liners), use judgment; not every change needs the full rigor.
- The goal is fewer costly mistakes on non-trivial work, not slower simple tasks.
- The system is working if: fewer unnecessary changes in diffs, fewer rewrites from overcomplication, clarifying questions before implementation, clean minimal change sets.

### 0.20 Response Communication

- Short answer -> plain text, 1-3 lines, concise.
- Default -> markdown (always rendered; far superior to the LLM's default plain text).
- Heavy visual -> HTML file with whatever CDN/imports it needs (e.g. mermaid). Never dump HTML code in chat: save it as `.agents/tmp/YYYY-MM-DD-<plan>-<title>.html` (date + plan name when part of a plan + response title) so the user can open and render it. Only HTML files are stored there; markdown goes directly in the response. If the agent supports HTML rendering, render the physical file without dumping the code.
- Decision capability (this is what measures response quality): pick the component from the information at hand, case by case per the matrix written in `.agents/orchestrator/instructions.md` (short -> text; list -> numbered list; comparable data -> table; flow/process -> mermaid diagram; long explanation -> headings + short paragraphs; progress -> checklist or `step -> verify` table; code detail only when the user talks code, otherwise mechanics level; unsure user -> visual first).
- Request lists are enumerated `1, 2, ...` with sub-steps `1.1, 1.2`; never letters (`A1`, `B2` are forbidden).
- Mission: make it easy for the human to understand what the agent does, what they are really asking, and that what they asked is what the agent does.

## 1. Agent Workflow

The current flow is:

1. Analyze request and context.
2. Read rules, `.agents/CODING_STANDARDS.md`, relevant skills and affected files.
3. For broad work (a list of things: multiple steps, files or areas), iterate with the user (human loop) and create a `CREATED` plan directly — creation is inferred, no gate needed; then stop. For a single change, no plan.
4. Move to Enrichment only through the enrichment gate or the wildcard.
5. Critically review the enriched plan; never mark it ready automatically.
6. Move to Executor only through the execution gate (or the wildcard / "skip the workflow") with an approved `ENRICHED` plan.
7. Verify only with declared or authorized commands (declared plan tests run directly).
8. Write the memory entry at `EXECUTED` and meet the Definition of Done before finishing.
9. Report the final result (RULES 0.20: markdown by default, concise, visual).

**CRITICAL FLOW RULE:**

- NEVER run creation, Enrichment or Executor automatically.
- The enrichment and execution gate phrases (**"enrich the plan"** / **"enrich the plans"**, **"execute the plan"**, wildcard **"advance the workflow"**) are explicit gates and must not be inferred from "continue", "proceed", "investigate" or similar phrases. Creation is inferred from a list of things (**"create the plan"** / **"add it to a plan"** also trigger it explicitly).
- Investigating, contextualizing, answering questions or adjusting a plan does not change its state nor authorize the next phase.
- The user must review/approve the Enrichment result before execution.
- Executor cannot receive a `CREATED` or unapproved plan.
- Declared plan tests run directly at execution (rule 0.4 exception); other build, dev, preview and test commands still require independent authorization.
- A simple 1-2 change task that does not use a plan can be executed directly only when the user explicitly asked to implement that specific change; it must not silently become a plan flow.
- A plan counts as finished only with the DoD met (rule 0.11) and the memory entry written; plans are never closed without them.

There is no automatic history phase. Do not create history records unless explicitly requested by the user.

## 2. Stack & Structure

- The stack, repo structure and real scripts are declared in `.agents/AGENTS.md` (section "Project Profile").
- Code conventions are declared in `.agents/CODING_STANDARDS.md`.
- Architecture and design are declared in `.agents/DESIGN.md`.
- Do not invent stack rules by inference: if they are not written, ask or propose writing them.

## 3. Verification

- Use only safe read-only inspection commands without permission: file reading, search, listings and spot checks.
- Blocked scripts require explicit user authorization.
- If a verification is not run because of a rule, report it explicitly at closing.

## 4. Quick Reference

| Rule         | Do                                           | Avoid                                           |
| ------------ | -------------------------------------------- | ----------------------------------------------- |
| Git          | Wait for explicit request                    | Interacting without permission                  |
| Files        | Report before deleting                       | Deleting without approval                       |
| Scope        | Changes scoped to the requirement            | Extra submodules/files                          |
| Scripts      | Ask permission for verification              | Running blocked scripts automatically           |
| Skills       | Adapt to the stack and the local standard    | Letting them prevail over RULES/STANDARDS       |
| Stack        | Follow CODING_STANDARDS.md and DESIGN.md     | Inventing rules by inference                    |
| Secrets      | Environment/secret manager, redact in logs   | In Git, builds, logs, responses                 |
| Plans        | Document the change in the plan before executing; human loop in CREATED; frozen scope after; complete the workflow and finish in EXECUTED | Deleting, emptying or abandoning plans without a record |
| DoD          | Meet the checklist before finishing in EXECUTED | Finishing a plan without DoD or memory entry      |
| Decisions    | ADR in `.agents/decisions/` (permanent)      | Leaving the decision only in the plan           |
| Gates        | Wait for the creation/enrichment/execution gates or the "advance the workflow" wildcard; only "skip the workflow" allows the full cycle | Advancing by interpreting "continue"/"proceed" |
| Responses    | Short -> text; default -> markdown; heavy visual -> HTML file in `.agents/tmp/`; numbered lists, never letters | Dumping HTML code in chat; plain-text walls |
| Language     | Normative content in English; user plans in the user's language | Mixed languages in normative files |

<!-- USER-START -->

## Local additions (preserved on update)

- **No comments in code.** Nothing we write or modify carries comments. The rule,
  its scope and where the documentation lives are in
  `.agents/CODING_STANDARDS.md` §1; it is checked at closure by the DoD in
  `.agents/WORKFLOW.md`.

<!-- USER-END -->

**Last updated:** September 2026
**Version:** 1.4 (generic harness)
