#!/bin/bash

echo "🔪 Killing all grid_sequencer processes..."

# Kill all grid_sequencer processes
pkill -f grid_sequencer

# Also kill any encoder_demo processes if they're running
pkill -f encoder_demo

# Clean up PID file if it exists
rm -f /tmp/grid_sequencer.pid

# Wait a moment for processes to clean up
sleep 0.5

# Check if any are still running
REMAINING=$(ps aux | grep -v grep | grep -c "grid_sequencer\|encoder_demo")

if [ $REMAINING -eq 0 ]; then
    echo "✅ All processes killed successfully"
else
    echo "⚠️  $REMAINING processes may still be running, trying force kill..."
    pkill -9 -f grid_sequencer
    pkill -9 -f encoder_demo
    sleep 0.5
    echo "✅ Force kill completed"
fi

echo "🧹 Ready for fresh start!"