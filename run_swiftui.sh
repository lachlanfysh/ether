#!/bin/bash
echo "🚀 Running SwiftUI EtherSynth with C++ Bridge"
echo ""
echo "Building C++ bridge library..."
./build_cpp_library.sh
echo ""
echo "Starting SwiftUI application..."
swift build && ./.build/arm64-apple-macosx/debug/EtherSynth
