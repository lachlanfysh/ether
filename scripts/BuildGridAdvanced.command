#!/bin/bash
# Double-clickable wrapper for building the advanced grid sequencer
DIR="$(cd "$(dirname "$0")" && pwd)"
exec /bin/bash "$DIR/build_grid_advanced.sh"

