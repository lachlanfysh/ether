#!/usr/bin/env bash
set -euo pipefail

# Simple runner to generate docs/tests using the local `claude` CLI.
# Adjust CLAUDE_CMD/CLAUDE_ARGS if your CLI syntax differs.

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
PROMPTS_DIR="$ROOT_DIR/tools/claude_prompts"

# Command and flags
CLAUDE_CMD=${CLAUDE_CMD:-claude}
CLAUDE_ARGS=${CLAUDE_ARGS:-}

echo "Using CLAUDE_CMD=$CLAUDE_CMD"

generate() {
  local prompt_file="$1"; shift
  local out_file="$1"; shift
  echo "\n=== Generating $out_file from $prompt_file ===" >&2
  if ! command -v "$CLAUDE_CMD" >/dev/null 2>&1; then
    echo "Error: '$CLAUDE_CMD' not found in PATH. Set CLAUDE_CMD or install the CLI." >&2
    exit 1
  fi
  # Read prompt and pass to CLI. Many CLIs support `-p`; adjust CLAUDE_ARGS if needed.
  "$CLAUDE_CMD" $CLAUDE_ARGS -p "$(cat "$prompt_file")" > "$out_file"
  echo "Wrote $out_file" >&2
}

mkdir -p "$PROMPTS_DIR"

generate "$PROMPTS_DIR/test_suite_plan.prompt"         "$ROOT_DIR/TEST_SUITE_PLAN.md"
generate "$PROMPTS_DIR/test_cases_global_lfo.prompt"   "$ROOT_DIR/TEST_CASES_GLOBAL_LFO.md"
generate "$PROMPTS_DIR/engine_naming_taxonomy.prompt"  "$ROOT_DIR/ENGINE_NAMING_TAXONOMY.md"
generate "$PROMPTS_DIR/param_layout_checklist.prompt"  "$ROOT_DIR/PARAM_LAYOUT_CHECKLIST.md"
generate "$PROMPTS_DIR/test_snippets.prompt"           "$ROOT_DIR/TEST_SNIPPETS.md"

echo "\nAll documents generated. Review the .md files at repo root."

