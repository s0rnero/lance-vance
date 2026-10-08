# Installing The Harness In A Repository

This harness boots the same agent workflow in any repository without copy/paste or cloning. The core (workflow, roles, gates, plan cycle, skills, DoD) is identical in all repos; what gets adapted per repo is the profile, the standards and the design.

## Requirements

- **Node >= 18** for `scripts/setup.mjs` (autoskills requires >= 22).
- Git to install from a remote URL.

## Install with a short command (npx/bunx)

The harness is also an npm package with a binary. Once published:

```bash
npx agents-setup-sorno ../repo-name
bunx agents-setup-sorno ../repo-name
```

Before publishing, you can activate the command locally (once, from this repo):

```bash
npm link     # then 'agents-setup-sorno <repo>' works from any folder
# or with bun:
bun link
```

Without publishing or linking, use the repo's direct script:

```bash
node scripts/setup.mjs ../repo-name
```

All options are the same through any execution path (see below).

## Install

From this repository (the harness source):

```bash
node scripts/setup.mjs ../repo-name
```

From another origin (a git URL of the harness, or a local copy):

```bash
node scripts/setup.mjs ../repo-name --from https://github.com/<user>/<harness-repo>.git
node scripts/setup.mjs ../repo-name --from /path/to/harness
```

Useful options:

```bash
node scripts/setup.mjs ../repo --dry-run     # shows what it would do, writes nothing
node scripts/setup.mjs ../repo --skip-skills # does not run autoskills
node scripts/setup.mjs ../repo --skills      # installs skills (preview only by default)
node scripts/setup.mjs ../repo --no-fill     # does not touch .agents/AGENTS.md
node scripts/setup.mjs ../repo --no-compat   # does not generate the root compatibility layer
```

What `install` does (with the `s0rno` DOS Rebel banner and a 5-stage progress bar, no dependencies):

1. Copies the harness **core** to `.agents/` (workflow, rules, roles, subagents, templates, manifest, scripts). Everything between `<!-- USER-START -->` and `<!-- USER-END -->` in the target is preserved; the rest is replaced.
2. Creates the **per-repo** files only if missing: `AGENTS.md`, `CODING_STANDARDS.md`, `DESIGN.md`.
3. **Fills the profile** in `AGENTS.md`: detected stack (`package.json`, Gradle/Maven, Go, Rust, Python) and the real scripts table.
4. Generates the **root compatibility layer**: `AGENTS.md`, `CLAUDE.md`, `.cursorrules` and `.github/copilot-instructions.md` (thin files pointing to `.agents/`), so the harness works with Codex, Copilot, Cursor, Claude Code, etc.
5. Runs `npx autoskills` as the last stage (preview by default; `--skills` to install) and registers the **origin** plus core file hashes in `.agents/manifest.json`.

## Update (bring harness changes)

Since the install registers the origin, bringing harness updates is one command:

```bash
node scripts/setup.mjs ../repo-name update              # uses the registered origin
node scripts/setup.mjs ../repo-name update --from <url> # update from another source
```

What `update` does:

- Compares versions (`.agents/manifest.json`) and reports the delta.
- Overwrites only the **core** (workflow, rules, roles, subagents, templates, scripts), preserving every `<!-- USER-START --> … <!-- USER-END -->` block found in the target.
- Before overwriting, lists any **USER-MODIFIED** originals (core files you edited outside USER blocks, detected via the hashes registered at install); they are still overwritten, but you see the list first. Your USER blocks are always kept.
- Creates per-repo files only if missing; **never overwrites** already-adapted `AGENTS.md`, `CODING_STANDARDS.md` or `DESIGN.md`.
- Regenerates the root compatibility layer.
- **Never touches**: `.agents/plans/`, `.agents/decisions/`, `.agents/skills/` (including `skills-lock.json`) nor any file not listed in the manifest.
- Does not reinstall skills unless `--skills`.

### What gets overwritten and what gets preserved

| Type | Files | Behavior |
| --- | --- | --- |
| Core | `.agents/WORKFLOW.md`, `RULES.md`, `PLANS.md`, `plans/README.md`, `orchestrator/`, `subagents/`, `skills/README.md`, `bootstrap/INSTALL.md`, `decisions/README.md`, `templates/`, `manifest.json`, `scripts/setup.mjs`, `scripts/verify.mjs`, `scripts/install.sh` | Overwritten on every install/update, except `<!-- USER-START --> … <!-- USER-END -->` blocks (always kept; edited originals listed first as USER-MODIFIED) |
| Per repo | `.agents/AGENTS.md`, `CODING_STANDARDS.md`, `DESIGN.md` | Created only if missing; never overwritten |
| Generated | root: `AGENTS.md`, `CLAUDE.md`, `.cursorrules`, `.github/copilot-instructions.md` | Regenerated on install/update |
| Preserved | `.agents/plans/`, `.agents/decisions/`, `.agents/skills/`, and every file not listed in the manifest | Never touched |

## Verify the installation

```bash
node scripts/verify.mjs ../repo-name   # harness health check
node scripts/verify.mjs                # current directory
```

Validates manifest and version, core presence, plans with valid frontmatter/status, ADRs, compatibility layer and skills. Exit 0 when there are no errors.

## Lightweight installation

`scripts/install.sh` is a wrapper that uses `setup.mjs` when Node is available; without Node it makes a basic bash copy of the core (no profile and no root compatibility layer).

```bash
bash scripts/install.sh ../repo-name          # copies core + autoskills preview
bash scripts/install.sh ../repo-name --skills # also installs detected skills
```

## Per-Repo Adaptation Checklist

1. **`.agents/AGENTS.md`** — review the "Project Profile" section (already filled by setup.mjs): real stack, repo structure and real build/dev/test/preview scripts.
2. **`.agents/CODING_STANDARDS.md`** — adapt to the stack: file structure, naming, contracts, testing.
3. **`.agents/DESIGN.md`** — adapt to the stack: layered architecture, contracts, performance, security.
4. **Skills** — review the autoskills preview, run `npx autoskills -y` and register the skills in `.agents/skills/README.md`.
5. `node scripts/verify.mjs <repo>` to confirm the harness is healthy.
6. Initial harness commit in the target repo.

## Harness Rules

- The flow, gates, DoD and plan cycle are the same in every repo (see `.agents/WORKFLOW.md`).
- Plans are the project's persistent memory: they are never deleted and every change is documented in the plan before being executed (see `.agents/PLANS.md`).
- Architecture decisions are recorded as ADRs in `.agents/decisions/` (see `.agents/RULES.md` 0.12).
- No skill prevails over `.agents/RULES.md`, `.agents/CODING_STANDARDS.md` or `.agents/DESIGN.md`.

**Language note (RULES.md 0.14):** normative content is written in English; user plans in `.agents/plans/` follow the language the user writes their prompts in.

<!-- USER-START -->

## Local additions (preserved on update)

_Write your local additions below this line. Everything between USER-START and USER-END is kept as-is on every install/update; the rest of this file is overwritten._

<!-- USER-END -->
