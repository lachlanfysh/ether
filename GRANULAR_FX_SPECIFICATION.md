# EtherSynth Granular FX Implementation Specification

## Overview
A comprehensive Clouds-like Granular FX node integrated as the third FX bus (Bus C) in EtherSynth's harmonized bridge architecture. Provides real-time granular processing with texture control, freeze capability, and advanced grain scheduling.

## Architecture

### Bus Configuration
- **Bus A**: Reverb (existing)
- **Bus B**: Delay (existing) 
- **Bus C**: Granular FX (NEW)

### Processing Order
```
Engine Outputs → Per-Slot Sends → Bus Processing → Master Mix
                 ↓               ↓
                 sendGranular[]  GranularFX.process()
                 ↓               ↓
                 Accumulate      Return to Master
```

## Core Features

### 1. Stereo Circular Capture Buffer
- **Size**: 3 seconds at 48kHz (144,000 samples per channel)
- **Implementation**: `std::vector<float> captureBufferL_/R_`
- **Write Mode**: Continuous circular buffer with configurable freeze
- **Thread Safety**: Lock-free single-writer design

### 2. Advanced Grain Generation
```cpp
struct Grain {
    bool active;                // Grain state
    float bufferPosL, bufferPosR; // Read positions (L/R channels)
    float phaseInc;            // Pitch shift multiplier
    float windowPhase;         // Window envelope position (0-1)
    float windowInc;           // Window phase increment
    float panL, panR;          // Stereo pan gains
    float amplitude;           // Grain amplitude
};
```

### 3. Block-Based Scheduling Algorithm
```cpp
// Calculate grains to launch per block
float density = mapDensity(params_[PARAM_DENSITY]);           // 2-50 grains/s
float blockTimeMs = blockSize / sampleRate * 1000.0f;
grainTimer_ += blockTimeMs;

while (grainTimer_ >= grainInterval && grainsLaunched < maxGrainsPerBlock) {
    scheduleNewGrain();
    grainTimer_ -= grainInterval;
}
```

## Parameter System

### 16 Parameters (Normalized 0-1)
| Index | Parameter | Range | Mapping Formula |
|-------|-----------|-------|-----------------|
| 0 | **Size** | 10-500ms | `10.0 * pow(50.0, norm)` |
| 1 | **Density** | 2-50 grains/s | `2.0 * pow(25.0, norm)` |  
| 2 | **Position** | 0-1 buffer | `norm * (bufferLen-1)` |
| 3 | **Jitter** | 0-100% | Position ±20%, timing ±20% |
| 4 | **Pitch** | -24 to +24st | `(norm*2 - 1) * 24` |
| 5 | **Spread** | 0-1 stereo | Equal-power per-grain panning |
| 6 | **Texture** | 0-1 blend | Hann (0) ↔ Tukey (1) morphing |
| 7 | **Feedback** | 0-30% max | Smeared reinjection with LPF |
| 8 | **Freeze** | 0/1 toggle | Stop capture, render from buffer |
| 9 | **Wet** | 0-1 level | Output gain (dry handled by sends) |
| 10 | **Return HPF** | 20-600Hz | `20.0 * pow(30.0, norm)` |
| 11 | **Return LPF** | 1k-18kHz | `1000.0 * pow(18.0, norm)` |
| 12-15 | *Reserved* | - | Future sync/quality extensions |

### Window Morphing Algorithm
```cpp
float generateWindow(float phase, float texture) {
    // Hann window (texture = 0)
    float hann = 0.5f - 0.5f * cos(2.0f * M_PI * phase);
    
    // Tukey window (texture = 1) with variable taper
    float alpha = 0.1f + texture * 0.8f;  // 10% to 90% taper
    float tukey = /* Tukey calculation based on alpha */;
    
    // Blend between windows
    return hann * (1.0f - texture) + tukey * texture;
}
```

## API Integration

### C API Functions
```c
// Extended FX send control
void ether_set_engine_fx_send(void* synth, int instrument, int which, float value);
//   which: 0=reverb, 1=delay, 2=granular

// Global granular parameter control  
void ether_set_granular_param(void* synth, int whichParam, float value);
//   whichParam: 0-11 mapped to parameter indices
```

### Bridge Integration Points
```cpp
// In harmonized_13_engines_bridge.cpp
struct Harmonized15EngineEtherSynthInstance {
    // NEW: Third FX bus
    std::array<float, SLOT_COUNT> sendGranular{};
    GranularFX granularFX;
    
    // Parameter cache
    struct GranularParams {
        float size, density, position, jitter, pitch, spread;
        float texture, feedback, freeze, wet, returnHPF, returnLPF;
    } granularParams;
};
```

## Performance Characteristics

### CPU Optimization
- **Grain Cap**: 16-128 grains per block (Quality parameter)
- **Block Processing**: Grain scheduling once per audio block
- **Linear Interpolation**: Fast buffer access with acceptable quality
- **Filter Simplification**: One-pole HPF/LPF for return path

### Memory Usage
- **Capture Buffers**: ~1.2MB (3s * 48kHz * 2ch * 4 bytes)
- **Grain Array**: ~4KB (128 grains * 32 bytes/grain)
- **Total Overhead**: ~1.25MB per instance

### Real-time Safety
- **Lock-free Design**: No mutexes in audio thread
- **Bounded Operations**: Fixed maximum grains per block
- **Safe Feedback**: Clamped to prevent runaway

## Terminal UI Integration

### Send Levels Section (Updated)
```
Instrument 0 (MacroVA):
  voices       : 4
  rev_send     : 0.20    # Bus A (Reverb)
  del_send     : 0.15    # Bus B (Delay)
  grn_send     : 0.40    # Bus C (Granular) ← NEW
  
Global Granular FX:
  grn_size     : 0.30    # ~80ms grain size
  grn_density  : 0.40    # ~10 grains/s
  grn_position : 0.50    # Center of buffer
  grn_freeze   : OFF     # Capture active
  grn_pitch    : +0.0    # Pitch in semitones
```

### Navigation
- **Period (.)**: Next parameter
- **Comma (,)**: Previous parameter  
- **+/-**: Adjust selected parameter
- **Range**: 15 total parameters (voices + sends + reverb + delay + granular)

## Test Coverage

### Automated Test Cases
```bash
make test-granular    # Run complete granular FX test suite
```

#### Test Categories
1. **Send Routing**: Verify Bus C routing and RMS response
2. **Freeze Functionality**: Capture persistence when input stops
3. **Pitch Shifting**: Spectral/zero-crossing change validation
4. **Parameter Ranges**: Boundary condition handling

#### Acceptance Criteria
- ✅ RMS increase ≥15% with granular send enabled
- ✅ Frozen content persists ≥5% RMS for 30 blocks after input stops
- ✅ Pitch shift detectable via RMS change or frequency ratio
- ✅ All parameters accept 0-1 range with proper clamping

## Files Created/Modified

### New Files
```
src/processing/effects/GranularFX.h         # Complete header (~120 lines)
src/processing/effects/GranularFX.cpp       # Implementation (~350 lines)
tests/test_granular_fx.cpp                  # Comprehensive tests (~300 lines)
```

### Modified Files (via patches)
```
harmonized_13_engines_bridge.cpp            # Bus C integration
EtherSynthBridge.h                          # API declarations  
grid_sequencer.cpp                          # Terminal UI updates
Makefile                                    # Build targets
```

### Patch Files
```
granular_fx_header_fix_patch.diff           # Method declaration fix
harmonized_bridge_granular_patch.diff       # Bridge integration
bridge_header_granular_patch.diff           # API headers
terminal_ui_granular_patch.diff              # UI enhancements  
granular_makefile_patch.diff                # Build system
```

## Build Integration

### Compilation Requirements
```makefile
# Add GranularFX.cpp to sources
GRANULAR_SOURCES = src/processing/effects/GranularFX.cpp

# Test target
$(TEST_GRANULAR_TARGET): tests/test_granular_fx.cpp harmonized_13_engines_bridge.cpp $(GRANULAR_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LIBS) -lm
```

### Dependencies
- **Standard Library**: `<vector>`, `<array>`, `<random>`, `<cmath>`
- **No External Deps**: Pure C++ implementation
- **Build Compatibility**: Works with existing `-std=c++17` requirement

## Usage Examples

### Basic Granular Texture
```c
// Set moderate granular send
ether_set_engine_fx_send(synth, 0, 2, 0.5f);  // 50% to Bus C

// Configure grain parameters  
ether_set_granular_param(synth, 0, 0.3f);     // Size: ~80ms
ether_set_granular_param(synth, 1, 0.4f);     // Density: ~10 grains/s
ether_set_granular_param(synth, 2, 0.5f);     // Position: center
ether_set_granular_param(synth, 6, 0.2f);     // Texture: slight Tukey blend
```

### Freeze and Pitch Effects
```c
// Enable freeze for texture hold
ether_set_granular_param(synth, 8, 1.0f);     // Freeze: ON

// Pitch shift up one octave
ether_set_granular_param(synth, 4, 0.75f);    // Pitch: +12 semitones

// Add return filtering
ether_set_granular_param(synth, 10, 0.3f);    // HPF: ~60Hz
ether_set_granular_param(synth, 11, 0.8f);    // LPF: ~12kHz
```

### Cloud Textures with Feedback
```c
// Large, overlapping grains
ether_set_granular_param(synth, 0, 0.7f);     // Size: ~300ms
ether_set_granular_param(synth, 1, 0.2f);     // Density: ~5 grains/s

// Wide stereo spread
ether_set_granular_param(synth, 5, 0.8f);     // Spread: 80%

// Subtle feedback smear
ether_set_granular_param(synth, 7, 0.2f);     // Feedback: 20%
```

## Future Extensions

### Commented Hooks for Enhancement
```cpp
// TODO: FFT-based pitch/time stretching for higher quality
// TODO: Multi-buffer support for longer capture times  
// TODO: MIDI sync divisions for rhythmic granular effects
// TODO: Cross-buffer modulation between slots
// TODO: Spectral filtering within grains
```

### Scalability Considerations
- **Multi-instance**: Each harmonized bridge instance has independent granular FX
- **Parameter Automation**: Compatible with Global LFO system for modulation
- **Preset Integration**: Parameters storable in existing preset system
- **Performance Scaling**: Grain cap allows CPU budget control

The Granular FX implementation provides a professional-grade granular processor seamlessly integrated into EtherSynth's existing FX bus architecture, delivering rich textural capabilities while maintaining real-time performance and system stability.