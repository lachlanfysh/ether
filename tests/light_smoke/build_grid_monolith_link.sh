#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$DIR/../.." && pwd)"

echo "Building grid_sequencer_advanced (link test with local stubs)..."
cxx=${CXX:-clang++}
flags="-std=c++17 -O2 -Wall -Wextra"
inc=("-I$ROOT" "-I$ROOT/src" "-I/opt/homebrew/include")
libs=()

tmp_o1=/tmp/grid_seq_main.o
tmp_o2=/tmp/dummy_link_stubs.o
tmp_o3=/tmp/serial_port.o
tmp_o4=/tmp/encoder_io.o
tmp_o5=/tmp/grid_io.o
out=/tmp/grid_seq_monolith

"$cxx" $flags "${inc[@]}" -c "$ROOT/grid_sequencer_advanced.cpp" -o "$tmp_o1"
"$cxx" $flags "${inc[@]}" -c "$ROOT/tests/light_smoke/dummy_link_stubs.cpp" -o "$tmp_o2"
"$cxx" $flags "${inc[@]}" -c "$ROOT/src/io/SerialPort.cpp" -o "$tmp_o3"
"$cxx" $flags "${inc[@]}" -c "$ROOT/src/io/EncoderIO.cpp" -o "$tmp_o4"
# no GridIO in link test

if [ ${#libs[@]:-0} -gt 0 ]; then
  linkcmd=("$cxx" $flags -o "$out" "$tmp_o1" "$tmp_o2" "$tmp_o3" "$tmp_o4" "${libs[@]}")
else
  linkcmd=("$cxx" $flags -o "$out" "$tmp_o1" "$tmp_o2" "$tmp_o3" "$tmp_o4")
fi

if "${linkcmd[@]}"; then
  echo "✅ Link succeeded: $out"
  echo "(This uses local stubs; audio/OSC are inert.)"
else
  echo "❌ Link failed. Please copy the error output here so we can adjust stubs."
  exit 1
fi
