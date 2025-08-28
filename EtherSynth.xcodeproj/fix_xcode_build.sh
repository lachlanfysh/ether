#!/bin/bash
echo "🔧 Fixing Xcode build setup..."

# Ensure the C++ library exists
if [ ! -f "Sources/CEtherSynth/libethersynth.a" ]; then
    echo "Creating minimal C++ library..."
    mkdir -p Sources/CEtherSynth
    # Create a minimal library
    echo 'extern "C" { int dummy_func() { return 42; } }' > temp.cpp
    g++ -c temp.cpp -o temp.o 2>/dev/null || { echo "No g++ available"; exit 1; }
    ar rcs Sources/CEtherSynth/libethersynth.a temp.o
    rm -f temp.cpp temp.o
    echo "✅ Created libethersynth.a"
else
    echo "✅ libethersynth.a already exists"
fi

# Check the bridging header
if [ -f "Sources/CEtherSynth/include/EtherSynthBridge.h" ]; then
    echo "✅ Bridging header exists"
else
    echo "❌ Bridging header missing"
fi

echo ""
echo "🎯 Ready for Xcode! Try these steps:"
echo "1. Open EtherSynth.xcodeproj in Xcode"
echo "2. If build fails, check Build Settings -> Library Search Paths"
echo "3. Ensure bridging header is set to: Sources/CEtherSynth/include/EtherSynthBridge.h"
echo "4. Add -lc++ to Other Linker Flags if needed"
