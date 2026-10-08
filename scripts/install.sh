#!/usr/bin/env bash
#
# Installs or syncs the .agents harness into a target repository.
#
# Prefers scripts/setup.mjs (Node >= 18), which does everything: core + profile +
# root compatibility layer + autoskills. Without Node, uses a basic bash copy.
#
# Usage:
#   bash scripts/install.sh <target-repo> [--skills] [--dry-run]
#
# Note: to update an already-installed repo (bring harness changes) use:
#   node scripts/setup.mjs <repo> update
set -euo pipefail

HARNESS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if command -v node >/dev/null 2>&1; then
  NODE_MAJOR="$(node -p 'process.versions.node.split(".")[0]' 2>/dev/null || echo 0)"
  if [[ "$NODE_MAJOR" -ge 18 ]]; then
    exec node "$HARNESS_DIR/scripts/setup.mjs" install "$@"
  fi
fi

TARGET=""
RUN_SKILLS=0
AUTOSKILLS_FLAGS="--dry-run"

for arg in "$@"; do
  case "$arg" in
    --skills) RUN_SKILLS=1 ;;
    --dry-run) AUTOSKILLS_FLAGS="--dry-run" ;;
    *) TARGET="$arg" ;;
  esac
done

if [[ -z "$TARGET" ]]; then
  echo "Usage: bash scripts/install.sh <target-repo> [--skills] [--dry-run]"
  exit 1
fi
if [[ ! -d "$TARGET" ]]; then
  echo "Error: target does not exist: $TARGET"
  exit 1
fi

CORE="$HARNESS_DIR/.agents"
TARGET_AGENTS="$TARGET/.agents"
mkdir -p "$TARGET_AGENTS/orchestrator" \
         "$TARGET_AGENTS/subagents" \
         "$TARGET_AGENTS/skills" \
         "$TARGET_AGENTS/plans" \
         "$TARGET_AGENTS/decisions" \
         "$TARGET_AGENTS/templates" \
         "$TARGET_AGENTS/bootstrap"

echo ""
echo "==> Installing harness (bash fallback, no Node) at: $TARGET"
echo ""

for f in AGENTS.md WORKFLOW.md RULES.md PLANS.md; do
  cp "$CORE/$f" "$TARGET_AGENTS/$f"
  echo "Copied: .agents/$f"
done
cp "$CORE/orchestrator/instructions.md" "$TARGET_AGENTS/orchestrator/instructions.md"
echo "Copied: .agents/orchestrator/instructions.md"
for f in enrichment-process.md executor.md; do
  cp "$CORE/subagents/$f" "$TARGET_AGENTS/subagents/$f"
  echo "Copied: .agents/subagents/$f"
done
cp "$CORE/skills/README.md" "$TARGET_AGENTS/skills/README.md"
echo "Copied: .agents/skills/README.md"
cp "$CORE/plans/README.md" "$TARGET_AGENTS/plans/README.md"
echo "Copied: .agents/plans/README.md"
cp "$CORE/decisions/README.md" "$TARGET_AGENTS/decisions/README.md"
echo "Copied: .agents/decisions/README.md"
cp "$CORE/bootstrap/INSTALL.md" "$TARGET_AGENTS/bootstrap/INSTALL.md"
echo "Copied: .agents/bootstrap/INSTALL.md"
for f in plan.md adr.md; do
  cp "$CORE/templates/$f" "$TARGET_AGENTS/templates/$f"
  echo "Copied: .agents/templates/$f"
done
cp "$CORE/manifest.json" "$TARGET_AGENTS/manifest.json"
echo "Copied: .agents/manifest.json"
mkdir -p "$TARGET/scripts"
cp "$HARNESS_DIR/scripts/setup.mjs" "$TARGET/scripts/setup.mjs"
cp "$HARNESS_DIR/scripts/verify.mjs" "$TARGET/scripts/verify.mjs"
echo "Copied: scripts/setup.mjs, scripts/verify.mjs"

for f in CODING_STANDARDS.md DESIGN.md; do
  if [[ -f "$TARGET_AGENTS/$f" ]]; then
    echo "Kept (already exists, not overwritten): .agents/$f"
  else
    cp "$CORE/$f" "$TARGET_AGENTS/$f"
    echo "Created (adapt to the stack): .agents/$f"
  fi
done

echo ""
echo "Harness installed at: $TARGET_AGENTS"
echo "Pending on the user:"
echo "  - Fill in .agents/AGENTS.md (Project profile: stack, structure, real scripts)"
echo "  - Adapt .agents/CODING_STANDARDS.md and .agents/DESIGN.md to the stack"
echo "  - Note: the root compatibility layer (AGENTS.md, CLAUDE.md, .cursorrules) and"
echo "    automatic profile fill require Node >= 18: run 'node scripts/setup.mjs'"

if command -v node >/dev/null 2>&1; then
  NODE_MAJOR="$(node -p 'process.versions.node.split(".")[0]' 2>/dev/null || echo 0)"
  if [[ "$NODE_MAJOR" -lt 22 ]]; then
    echo ""
    echo "Warning: autoskills requires Node >= 22 (current: $(node -v))."
    exit 0
  fi
  echo ""
  if [[ "$RUN_SKILLS" -eq 1 ]]; then
    echo "Running autoskills at $TARGET ..."
    (cd "$TARGET" && npx --yes autoskills -y)
  else
    echo "autoskills preview (installs nothing):"
    (cd "$TARGET" && npx --yes autoskills "$AUTOSKILLS_FLAGS")
    echo ""
    echo "To install the detected skills:"
    echo "  cd $TARGET && npx autoskills -y"
  fi
else
  echo ""
  echo "Node not found: skipping autoskills."
fi
