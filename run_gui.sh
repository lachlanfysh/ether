#!/bin/bash

echo "🎹 Starting EtherSynth GUI..."

# Build the app
swift build

if [ $? -eq 0 ]; then
    echo "✅ Build successful, starting GUI..."
    
    # Run with proper environment for GUI
    DISPLAY=:0 .build/debug/EtherSynth
else
    echo "❌ Build failed"
    exit 1
fi