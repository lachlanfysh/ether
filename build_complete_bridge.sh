#!/bin/bash
echo "🔥 Building COMPLETE EtherSynth C++ Bridge..."

# Build the complete bridge
echo "Compiling complete bridge implementation..."
g++ -c complete_bridge.cpp -o complete_bridge.o -std=c++17 -O2

if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi

# Create the static library
echo "Creating static library..."
ar rcs libethersynth.a complete_bridge.o

# Copy to the Swift Package Manager location
echo "Installing library for Swift Package Manager..."
cp libethersynth.a Sources/CEtherSynth/libethersynth.a

# Clean up object files
rm -f complete_bridge.o

echo "✅ Complete C++ bridge built successfully!"
echo "📦 Library: libethersynth.a"
echo "📍 Location: Sources/CEtherSynth/libethersynth.a"
echo ""
echo "🚀 Ready for Xcode build!"

# Verify the library contains symbols
echo "🔍 Verifying symbols in library..."
nm libethersynth.a | grep "_ether_" | head -10
echo "... and more symbols available"