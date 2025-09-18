#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
cd "$ROOT"

echo "🔧 Building advanced grid sequencer (real build)..."

# Prefer clang++ on macOS for system SDK compatibility; allow override
: "${CXX:=clang++}"

# Quick dependency hints (not enforced)
need_headers=("/opt/homebrew/include/portaudio.h" \
              "/opt/homebrew/include/lo/lo.h")
missing=0
for h in "${need_headers[@]}"; do
  if [ ! -f "$h" ]; then
    echo "⚠️  Missing header: $h"
    missing=1
  fi
done
if [ "$missing" -eq 1 ]; then
  echo "👉 Install dependencies if needed: brew install portaudio liblo"
fi

echo "➡️  Invoking make (Makefile.advanced grid_advanced)"
# Force rebuild if REBUILD=1 is set, otherwise incremental
if [ "${REBUILD:-0}" = "1" ]; then
  make -B -f Makefile.advanced grid_advanced CXX="$CXX"
else
  make -f Makefile.advanced grid_advanced CXX="$CXX"
fi

echo ""
BIN="$ROOT/grid_sequencer_advanced"
STAMP=$(stat -f "%Sm" -t "%Y-%m-%d %H:%M:%S" "$BIN" 2>/dev/null || echo "?")
echo "✅ Build complete: $BIN"
echo "   Modified: $STAMP"
command -v shasum >/dev/null 2>&1 && echo "   SHA256: $(shasum -a 256 "$BIN" | awk '{print $1}')" || true
echo "Run it from Terminal for real audio/OSC. Press Ctrl+C to quit."
