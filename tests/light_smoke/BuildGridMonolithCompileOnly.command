#!/bin/bash
# Double-clickable compile-only build for the monolith
DIR="$(cd "$(dirname "$0")" && pwd)"
exec /bin/bash "$DIR/build_grid_monolith_compile_only.sh"

