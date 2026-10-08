# Plan Ownership

`.agents/plans/` is the only plan location of the project. All plans belong to this repository and are governed by `.agents/WORKFLOW.md`.

## Plans Are The Project's Persistent Memory

- `.agents/plans/` is the project's only persistent memory: it records decisions, context, progress and the outcome of every change.
- The workflow must always be completed: a plan is never abandoned or discarded. If it stops being viable, it is finished (`EXECUTED`) documenting the reason.
- NEVER delete plans: do not remove the file, do not empty its content and do not archive it outside the repo. A finished plan remains as permanent history.
- Every change or modification of the project (code, configuration, structure, rules) must be documented in a plan BEFORE being executed: the plan is created/updated first and only then executed through the execution gate.
- If a change already has a plan, no modification is executed outside that plan and its gates.
- The human loop lives only in `CREATED`: iterate with the user before writing the plan; always ask at the start. Scope freezes after `CREATED`: `ENRICHED` and `EXECUTED` cover only what is defined; anything new goes through the flow and, once clear, is added to the plan per its state (re-enrich if `ENRICHED`).

## Format

- New plans must declare in their frontmatter the domain and the rules that govern them:

```yaml
domain: <project-domain>
owner_rules: .agents
```

- File naming: `YYYY-MM-DD-<fam>.<iter>-<slug>.md` (e.g. `2026-09-29-1.0-rate-limit.md`). `1` is the main plan; iterations bound to what it executed are created as `1.1`, `1.2`, referencing the family via the `parent` frontmatter field.
- When creating a plan, review `.agents/plans/` first to avoid duplicating existing plans.
- Historical copies are not deleted without inventorying references and explicit approval.
- Gates and states follow `.agents/WORKFLOW.md`:

`CREATED` -> `ENRICHED` -> (Orchestrator reviews, user approves) -> `EXECUTED`

- Transitions are authorized only by the explicit creation, enrichment and execution phrases or the **"advance the workflow"** wildcard (canonical, in English per RULES.md 0.13-0.14; a genuine equivalent in the user's language is detected and accepted; ambiguous variants never count).

<!-- USER-START -->

## Local additions (preserved on update)

- **Migración al harness 1.4 (07/10/2026).** Todos los planes heredados del harness viejo
  llevan ya frontmatter (`name`, `status`, `type`, `domain`, `owner_rules`, `created`).
  Regla del jugador: **todos `EXECUTED`** salvo la escalada
  (`escalada-climbing-completo.md` y `mecanicas/04-opcional-escalar.md`, que quedan
  `ENRICHED` esperando su gate de ejecución).
- Los planes heredados **conservan su nombre original** (sin `YYYY-MM-DD-<fam>.<iter>-<slug>`)
  y su campo `name` repite su propio slug: no se renumeran, para no romper las referencias
  cruzadas que otros planes y ADR hacen a esos nombres.
- `plans/mecanicas/` y `plans/mods/` son las colecciones de secciones previas al harness:
  se conservan como historia, con frontmatter válido como el resto.
- **`EXECUTED` en un plan heredado significa «ciclo cerrado y documentado», no «verificado»**:
  varios registran dentro un cierre sin completar (`carga-sin-doble-trabajo.md`,
  `diagnostico-crash-init.md`, `fluides-ram.md`, `mods/10-plan-pasada-siguiente.md`). A día
  de hoy lo único validado de punta a punta es el nado.

<!-- USER-END -->
