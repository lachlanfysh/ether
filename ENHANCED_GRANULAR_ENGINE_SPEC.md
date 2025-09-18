# Enhanced GranularEngine Specification

## Overview
The Enhanced GranularEngine provides professional granular synthesis with texture control, density scheduling, and advanced windowing. The engine stays under 400 lines while delivering rich, controllable granular textures.

## Parameter Mapping (Normalized 0..1)

### Core 16 Parameter Integration
| Parameter | Control | Range | Mapping |
|-----------|---------|--------|---------|
| **HARMONICS** | Position | 0..1 | Buffer position (internal wavetable/noise) |
| **TIMBRE** | Size | 0..1 | 10..200 ms (logarithmic) |  
| **MORPH** | Density | 0..1 | 5..50 grains/sec (linear) |
| **OSC_MIX** | Jitter | 0..1 | Timing ±20%, pitch ±0.5 semitones |
| **DETUNE** | Texture | 0..1 | Hann↔Tukey window blend |
| **SUB_LEVEL** | Pitch | 0..1 | -12..+12 semitones |
| **PAN** | Spread | 0..1 | Equal-power pan modulation per grain |
| **VOLUME** | Volume | 0..1 | Master output level |

### Advanced Controls
- **Position**: Selects playback position in internal hybrid sine/noise buffer
- **Size**: Logarithmic grain duration mapping: `10ms * pow(20, size)`
- **Density**: Linear grain rate: `5 + density * 45` grains per second
- **Jitter**: Affects both timing (±20%) and pitch (±0.5 semitones)
- **Texture**: Crossfades between Hann (0) and Tukey (1) windows
- **Pitch**: Semitone offset with per-grain jitter
- **Spread**: Per-grain random panning within ±spread range

## DSP Implementation

### Internal Buffer
- **Size**: 2048 samples hybrid sine/noise source
- **Content**: 70% sine wave + 30% noise for rich texture
- **Access**: Linear interpolation for smooth grain playback

### Grain Scheduling
- **Algorithm**: Density-based timing with jitter
- **Capacity**: Up to 16 concurrent grains
- **Timing**: `baseInterval * (1 + jitter * randomness)`
- **Overlap**: Natural overlapping creates smooth textures

### Window Functions
```cpp
// Hann Window (texture = 0)
hannWindow = 0.5 - 0.5 * cos(2π * phase)

// Tukey Window (texture = 1) 
// Variable taper: 10% to 90% based on texture
if (phase <= α/2) tukeyWindow = 0.5 - 0.5 * cos(π * 2*phase/α)
else if (phase >= 1-α/2) tukeyWindow = 0.5 - 0.5 * cos(π * 2*(1-phase)/α)  
else tukeyWindow = 1.0

// Final blend
window = hann * (1-texture) + tukey * texture
```

### Jitter Implementation
- **Timing Jitter**: Random interval variation ±20%
- **Pitch Jitter**: Random semitone offset ±0.5 around base pitch
- **Pan Jitter**: Per-grain randomization within spread range
- **Amplitude**: Slight grain-to-grain variation (0.3-0.5 range)

## Performance Characteristics

### CPU Usage
- **Rating**: 0.05 (5% relative to reference engine)
- **Optimization**: Efficient grain scheduling and processing
- **Memory**: ~2KB for buffers + grain state arrays

### Audio Quality
- **Click-Free**: Advanced window crossfading eliminates artifacts
- **Smooth**: Linear interpolation in source buffer
- **Rich**: Hybrid source material provides musical content
- **Controllable**: Intuitive parameter response curves

## Code Structure

### Files Enhanced (~350 lines total)
1. **GranularEngine.h** - Enhanced header with grain structures
2. **GranularEngine.cpp** - Complete implementation with windowing
3. **Integration** - Already connected in harmonized bridge

### Key Methods
```cpp
void scheduleGrain()           // Create new grain with jitter
void processGrains()           // Render all active grains  
float getWindow()              // Hann↔Tukey blend calculation
float getSample()              // Linear interpolated buffer access
std::pair getPanGains()        // Equal-power panning with spread
```

### Grain Structure
```cpp
struct Grain {
    bool active;               // Grain state
    float phase, phaseInc;     // Playback position & speed
    float bufferPos;           // Source buffer position
    float duration, age;       // Grain envelope timing
    float panL, panR;          // Stereo positioning
    float amplitude;           // Grain volume
};
```

## Usage Examples

### Basic Granular Texture
```c
// Set moderate grain size and density
ether_set_parameter(synth, slot, PARAM_TIMBRE, 0.3f);    // ~50ms grains
ether_set_parameter(synth, slot, PARAM_MORPH, 0.4f);     // ~23 grains/sec
ether_set_parameter(synth, slot, PARAM_DETUNE, 0.0f);    // Pure Hann windows
```

### Rhythmic Granular
```c  
// Faster, jittered grains
ether_set_parameter(synth, slot, PARAM_TIMBRE, 0.1f);    // ~18ms grains
ether_set_parameter(synth, slot, PARAM_MORPH, 0.8f);     // ~41 grains/sec
ether_set_parameter(synth, slot, PARAM_OSC_MIX, 0.6f);   // High jitter
```

### Textural Clouds
```c
// Long, overlapping grains
ether_set_parameter(synth, slot, PARAM_TIMBRE, 0.7f);    // ~140ms grains
ether_set_parameter(synth, slot, PARAM_MORPH, 0.2f);     // ~14 grains/sec
ether_set_parameter(synth, slot, PARAM_DETUNE, 0.8f);    // Tukey windows
ether_set_parameter(synth, slot, PARAM_PAN, 0.6f);       // Wide stereo spread
```

## Integration Points

### EngineFactory
- Already integrated in `harmonized_13_engines_bridge.cpp`
- Factory function: `std::make_unique<GranularEngine>()`
- Engine type: `EngineType::GRANULAR`

### Parameter System  
- Uses standard 16-parameter layout
- Maps to musical controls (H/T/M + OSC_MIX/DETUNE/SUB_LEVEL)
- Post-chain processing handles ADSR, filters, effects

### Performance Integration
- CPU usage properly reported (0.05)
- Voice count tracking (0 or 1)
- Sample rate / buffer size aware

## Build Requirements

Apply patches in order:
```bash
patch -p1 < enhanced_granular_engine_patch.diff
patch -p1 < enhanced_granular_engine_impl_patch.diff  
patch -p1 < granular_engine_corrected_mapping.diff
```

No additional build system changes required - engine is already integrated in the harmonized bridge and Makefile.

The Enhanced GranularEngine provides professional-quality granular synthesis within the EtherSynth framework while maintaining the 400-line code limit and full integration with the core parameter system.