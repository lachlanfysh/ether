#!/bin/bash
# Double-clickable runner for the advanced grid sequencer
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR/.." || exit 1
exec ./grid_sequencer_advanced "$@"

