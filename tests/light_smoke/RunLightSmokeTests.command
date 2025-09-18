#!/bin/bash
# Double-clickable wrapper for light smoke tests on macOS
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR/../.." || exit 1
exec /bin/bash "$DIR/build_light_smoke.sh"
