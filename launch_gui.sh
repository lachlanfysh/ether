#!/bin/bash

echo "🎹 Launching EtherSynth GUI..."

# Kill any existing processes
pkill -f EtherSynth 2>/dev/null

# Build and copy to app bundle
swift build
cp .build/debug/EtherSynth EtherSynth.app/Contents/MacOS/EtherSynth
chmod +x EtherSynth.app/Contents/MacOS/EtherSynth

# Run with proper GUI environment
echo "🖥️  Starting GUI..."
./EtherSynth.app/Contents/MacOS/EtherSynth &

# Wait a moment then try to activate
sleep 2
osascript -e 'tell application "EtherSynth" to activate' 2>/dev/null || echo "App may need manual activation"

echo "✅ EtherSynth should be visible now!"
echo "   - Look for the EtherSynth window on your desktop"
echo "   - If not visible, try Cmd+Tab to switch to it"
echo "   - Window is 960x320 pixels with a light gray interface"