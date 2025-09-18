#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$DIR/../.." && pwd)"

echo "Compiling grid_sequencer_advanced.cpp (compile-only)..."
cxx=${CXX:-clang++}
flags="-std=c++17 -O2 -Wall -Wextra"
inc=("-I$ROOT" "-I$ROOT/src" "-I/opt/homebrew/include")

if ! "$cxx" $flags "${inc[@]}" -c "$ROOT/grid_sequencer_advanced.cpp" -o /tmp/grid_seq_monolith.o; then
  echo ""
  echo "Compile failed. If errors mention missing headers like <portaudio.h> or <lo/lo.h>, install:"
  echo "  brew install portaudio liblo"
  echo "Or set CPATH to your include directories."
  exit 1
fi

echo "✅ Compile-only succeeded: /tmp/grid_seq_monolith.o"

