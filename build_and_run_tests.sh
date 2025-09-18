#!/bin/bash

echo "🔧 Building Parameter Routing Test Suite..."

# Build with the same flags as the main project
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -Wconversion -Wimplicit-fallthrough -Wswitch-enum"
INCLUDES="-I/opt/homebrew/include -Isrc -Isrc/engines -Isrc/audio -Isrc/hardware -Isrc/data"
LIBS="-L/opt/homebrew/lib -lportaudio -pthread -llo"

# Build the test suite
g++ $CXXFLAGS $INCLUDES -o test_parameter_routing \
    test_parameter_routing.cpp \
    src/engines/*.o \
    src/synthesis/SynthEngine_minimal.o \
    harmonized_13_engines_bridge.o \
    $LIBS

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo ""
    echo "🧪 Running Parameter Routing Tests..."
    echo "===================================="
    ./test_parameter_routing
else
    echo "❌ Build failed!"
    exit 1
fi