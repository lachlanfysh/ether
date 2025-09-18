#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$DIR/../.." && pwd)"

trap 'echo "Smoke tests failed."; exit 1' ERR

echo "Building light refactor smoke tests..."
cxx=${CXX:-clang++}
flags="-std=c++17 -O2 -Wall -Wextra"

"$cxx" $flags -o /tmp/test_grid_led_manager "$DIR/test_grid_led_manager.cpp"
/tmp/test_grid_led_manager

"$cxx" $flags -o /tmp/test_parameter_cache "$DIR/test_parameter_cache.cpp"
/tmp/test_parameter_cache

echo "All light smoke tests passed."
