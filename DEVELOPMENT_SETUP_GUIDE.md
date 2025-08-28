# 🛠️ ether Development Setup Guide

## Overview

This guide covers setting up the complete development environment for the ether portable synthesizer, from basic C++ compilation to full SwiftUI interface development.

## System Requirements

### macOS Development
- **macOS**: 12.0 (Monterey) or later
- **Xcode**: 14.0 or later with Command Line Tools
- **RAM**: 8GB minimum, 16GB recommended
- **Storage**: 5GB free space for development tools
- **Audio**: Core Audio compatible interface (built-in audio works)

### Cross-Platform Components
- **C++ Compiler**: GCC 9+ or Clang 10+ with C++17 support
- **Build System**: Make, CMake (optional)
- **Git**: For version control and updates

## Quick Start

### 1. Clone and Test Audio Engine

```bash
# Clone repository
git clone [repository-url] ether
cd ether

# Test core C++ audio engine
make clean
make test_build
./test_build
```

**Expected Output:**
```
=== ether Architecture Test ===
Initializing...
✅ Created synthesizer instance
✅ Initialized synthesizer successfully
✅ Core Audio: 48kHz, 128 samples, 2.67ms latency
✅ All 8 instrument colors working
✅ Parameter control functional
✅ Audio engine test completed successfully!
```

### 2. Test Swift Bridge Integration

```bash
# Test C++ to Swift bridge
make bridge_test
./bridge_test
```

**Expected Output:**
```
🔗 Testing Swift Bridge Integration
✅ Bridge functions loaded successfully
✅ Synthesizer creation successful
✅ All 8 instruments accessible
✅ Parameter morphing functional
✅ Performance metrics reporting correctly
🎉 Bridge test completed successfully!
```

### 3. Install Xcode for SwiftUI Interface

```bash
# Install Xcode from Mac App Store, then:
xcode-select --install

# Run SwiftUI setup script
chmod +x run_swiftui.sh
./run_swiftui.sh
```

## Detailed Setup

### C++ Development Environment

#### 1. Compiler Setup

**Install Xcode Command Line Tools:**
```bash
xcode-select --install
```

**Verify C++17 Support:**
```bash
g++ --version
# Should show Apple clang version 14.0.0 or later

echo '#include <filesystem>' | g++ -std=c++17 -x c++ -c -
echo $?
# Should output 0 (success)
```

#### 2. Build System Configuration

**Makefile Structure:**
```makefile
CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra
FRAMEWORKS = -framework CoreAudio -framework AudioUnit -framework CoreMIDI -framework Foundation
INCLUDES = -I.

# Platform-specific definitions
DEFINES = -DPLATFORM_MAC
```

**Available Build Targets:**
```bash
make all          # Build all targets
make test_build   # Build audio engine test
make bridge_test  # Build Swift bridge test  
make enhanced_test # Build enhanced features test
make clean        # Clean all build artifacts
make info         # Show build configuration
```

#### 3. Audio Framework Setup

**Core Audio Requirements:**
- Framework automatically linked on macOS
- No additional installation required
- Built-in audio interface sufficient for development

**Test Audio Setup:**
```bash
# Verify Core Audio access
./test_build
# Look for: "Core Audio: 48kHz, 128 samples, X.Xms latency"
```

#### 4. MIDI Setup (Optional)

**Connect MIDI Controller:**
```bash
# List available MIDI devices
./test_build
# Check output for detected MIDI devices

# Test MIDI input
# Connect MIDI keyboard and play notes during test
```

### SwiftUI Development Environment

#### 1. Xcode Installation

**From Mac App Store:**
1. Open Mac App Store
2. Search for "Xcode"
3. Install (requires ~10GB download)
4. Open Xcode and accept license agreements

**Verify Installation:**
```bash
xcodebuild -version
# Should show: Xcode 14.0 or later

xcodebuild -showsdks
# Should list iOS, macOS, and watchOS SDKs
```

#### 2. SwiftUI Project Structure

```
ether/
├── Sources/EtherSynth/
│   ├── EtherSynth.swift       # Main SwiftUI interface
│   └── SynthController.swift   # Swift-to-C++ bridge controller
├── EtherSynthBridge.h          # C API header
├── EtherSynthBridge.cpp        # C++ bridge implementation
└── Package.swift               # Swift Package Manager config
```

#### 3. Building SwiftUI Interface

**Option 1: Automated Script**
```bash
./run_swiftui.sh
```

**Option 2: Manual Build**
```bash
# Using Swift Package Manager
swift build

# Using Xcode
xcodebuild -project EtherSynth.xcodeproj -scheme EtherSynth
```

**Option 3: Xcode IDE**
```bash
# Open in Xcode for development
open EtherSynth.xcodeproj
# Then build and run with Cmd+R
```

#### 4. SwiftUI Features Available

**Complete 960×320 Interface:**
- NSynth-inspired spatial touch pad
- 8-color instrument selection (ROYGBIV + Grey)
- Smart knob with visual feedback and 270° rotation
- Real-time waveform visualization
- 26-key polyphonic keyboard with pressure simulation
- Transport controls (play/stop/record) with state feedback
- Performance monitoring (CPU usage, voice count, latency)
- Context-sensitive parameter controls

## Development Workflow

### 1. C++ Engine Development

**Standard Workflow:**
```bash
# Make changes to C++ source files
vim src/synthesis/WavetableEngine.cpp

# Rebuild and test
make clean && make test_build
./test_build

# Test specific features
./enhanced_test
```

**Adding New Synthesis Engine:**
```bash
# 1. Create header and implementation
touch src/synthesis/NewEngine.h
touch src/synthesis/NewEngine.cpp

# 2. Add to Makefile SOURCES
vim Makefile
# Add: src/synthesis/NewEngine.cpp \

# 3. Update factory function
vim src/synthesis/SynthEngine.cpp
# Add creation case for new engine

# 4. Rebuild and test
make clean && make test_build
```

### 2. SwiftUI Interface Development

**Development Cycle:**
```bash
# 1. Modify SwiftUI interface
open Sources/EtherSynth/EtherSynth.swift

# 2. Update bridge if needed
vim EtherSynthBridge.cpp

# 3. Rebuild C++ bridge
make bridge_test

# 4. Test bridge functionality
./bridge_test

# 5. Build and run SwiftUI
./run_swiftui.sh
```

**Live Preview in Xcode:**
1. Open `EtherSynth.xcodeproj` in Xcode
2. Select `EtherSynth.swift` in navigator
3. Enable Canvas preview (Editor > Canvas)
4. Use live preview for rapid UI iteration

### 3. Integration Testing

**Complete System Test:**
```bash
# Test all components in sequence
make clean
make all

./test_build        # C++ audio engine
./bridge_test       # Swift bridge
./enhanced_test     # Advanced features
./run_swiftui.sh    # SwiftUI interface
```

## Hardware Abstraction

### Mac Development Platform

**Simulated Hardware:**
```cpp
class MacHardware : public HardwareInterface {
    // Simulates 960×320 display
    // Maps Core Audio to synthesis engine  
    // Provides MIDI input/output
    // Simulates smart knob and touch interface
};
```

**Touch Interface Simulation:**
- Mouse/trackpad maps to X/Y coordinates
- Keyboard shortcuts for smart knob rotation
- MIDI input for real hardware controllers

### STM32 Target Platform (Future)

**Hardware Abstraction Layer:**
```cpp
class NucleoHardware : public HardwareInterface {
    // Real 480×320 TFT display
    // TouchGFX for graphics acceleration
    // Real-time audio processing
    // Physical smart knob and touch interface
};
```

**Development Strategy:**
1. **Mac Prototype**: Complete feature development
2. **Cross-compilation**: STM32 toolchain setup
3. **TouchGFX Port**: UI adaptation for embedded display
4. **Hardware Integration**: Final assembly and calibration

## Testing and Validation

### Automated Tests

**Audio Engine Tests:**
```bash
./test_build
# Tests: Initialization, voice allocation, parameter control, audio processing

./enhanced_test  
# Tests: Wavetable engine, modulation matrix, effects chain, performance
```

**Integration Tests:**
```bash
./bridge_test
# Tests: C++ to Swift bridge, parameter mapping, real-time communication
```

### Performance Benchmarking

**CPU Usage Monitoring:**
```bash
# Monitor during intensive synthesis
./test_build
# Watch CPU usage output for performance bottlenecks

# Profile with Instruments (when using Xcode)
# Product > Profile > CPU Profiler
```

**Latency Measurement:**
```bash
# Check audio latency
./test_build
# Look for: "Latency: X.Xms" in output
# Target: <10ms total latency
```

### Manual Testing Checklist

**Audio Engine:**
- [ ] Initialization successful
- [ ] All 8 instrument colors working
- [ ] Parameter changes audible
- [ ] Polyphonic note handling
- [ ] Stable audio output (no dropouts)

**SwiftUI Interface:**
- [ ] UI renders correctly (960×320)
- [ ] Touch interface responsive
- [ ] Smart knob rotation functional
- [ ] Instrument switching works
- [ ] Real-time parameter updates
- [ ] Transport controls functional

**Integration:**
- [ ] Bridge communication working
- [ ] Parameter changes synchronized
- [ ] Performance metrics accurate
- [ ] No audio glitches during UI interaction

## Troubleshooting

### Common Build Issues

**"No such file or directory" Error:**
```bash
# Check source file paths
ls -la src/synthesis/
# Verify all files exist and are readable

# Check Makefile SOURCES list
grep -n "SOURCES" Makefile
```

**Core Audio Framework Error:**
```bash
# Verify Xcode Command Line Tools
xcode-select -p
# Should output: /Applications/Xcode.app/Contents/Developer

# Reinstall if needed
sudo xcode-select --reset
xcode-select --install
```

**SwiftUI Build Failure:**
```bash
# Check Xcode version
xcodebuild -version
# Update if older than 14.0

# Clear derived data
rm -rf ~/Library/Developer/Xcode/DerivedData
```

### Performance Issues

**High CPU Usage:**
```bash
# Check synthesis engine efficiency
./enhanced_test
# Look for CPU usage > 50%

# Profile in Xcode for optimization
# Enable Release build: -O3 optimization
```

**Audio Dropouts:**
```bash
# Check buffer size
# Increase buffer size in AudioEngine.cpp
# Test with ./test_build

# Verify no background processes interfering
# Close unnecessary applications
```

### Development Tips

**Debugging C++ Code:**
```bash
# Compile with debug symbols
make clean
CXXFLAGS="-std=c++17 -g -O0" make test_build

# Use lldb debugger
lldb ./test_build
(lldb) breakpoint set --name main
(lldb) run
```

**SwiftUI Debugging:**
```bash
# Use Xcode debugger and breakpoints
# View console output for Swift bridge logs
# Use Instruments for performance analysis
```

**Version Control Best Practices:**
```bash
# Ignore build artifacts
echo "*.o" >> .gitignore
echo "test_build" >> .gitignore
echo "bridge_test" >> .gitignore
echo "enhanced_test" >> .gitignore

# Commit frequently during development
git add .
git commit -m "Add wavetable synthesis engine"
```

## Next Steps

### Immediate Development
1. **Complete Features**: Implement remaining synthesis engines
2. **Preset System**: Save/load functionality
3. **MIDI Integration**: Enhanced MIDI support
4. **Effects**: Add reverb, delay, chorus

### Hardware Preparation
1. **STM32 Toolchain**: Cross-compilation setup
2. **TouchGFX**: UI framework for embedded display
3. **Hardware Simulation**: Exact hardware matching
4. **Performance Optimization**: Embedded system optimization

### Production Readiness
1. **Testing**: Comprehensive test suite
2. **Documentation**: User manual and technical docs
3. **Calibration**: Hardware-specific adjustments
4. **Manufacturing**: PCB design and assembly

---

This development setup provides everything needed to work on the ether synthesizer project, from basic audio engine development to the complete SwiftUI interface. The modular architecture allows development on multiple components simultaneously while maintaining system integration.