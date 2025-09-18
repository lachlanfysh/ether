# Granular Engine Specification

## Engine Overview
**Type**: Granular synthesis engine for texture, time-stretching, and microsound manipulation  
**Base Class**: `PolyphonicEngine<GranularVoice>`  
**Voice Count**: 4-8 voices (CPU-dependent)  
**Sample Sources**: Loaded via `loadSample()` or real-time input capture

## Core Parameters Mapping

### Native Granular Parameters (Engine-Specific)
| Core Slot | Parameter | Range | Scale | Description |
|-----------|-----------|-------|-------|-------------|
| PARAM_HARMONICS | Grain Density | 1-200 grains/sec | Exponential | Grain trigger rate |
| PARAM_TIMBRE | Grain Size | 5-500 ms | Exponential | Individual grain duration |
| PARAM_MORPH | Position | 0.0-1.0 | Linear | Playback position in source |

### Additional Granular Controls
```cpp
enum GranularParameter {
    GRAN_JITTER = 16,     // Position randomization (0-1)
    GRAN_SPRAY = 17,      // Timing randomization (0-1) 
    GRAN_WINDOW = 18,     // Window shape (0=Hann, 1=Kaiser, 2=Blackman)
    GRAN_PITCH = 19,      // Pitch shift (-24 to +24 semitones)
    GRAN_STEREO_SPREAD = 20 // Stereo width (0-1)
};
```

### Fallback Post-Chain Behavior
When engine lacks native support, core parameters map to post-chain:
- **HARMONICS → HPF Cutoff**: Higher values = more high-pass filtering (grain texture emphasis)
- **TIMBRE → LPF Cutoff**: Controls grain brightness through low-pass filtering  
- **MORPH → LPF Q**: Adds resonant character to grain texture
- **Standard ADSR/Volume/Pan**: Applied to final grain cloud output

## DSP Architecture

### Grain Scheduler
```cpp
class GrainScheduler {
    struct Grain {
        float start_pos;      // Source sample position
        float duration;       // Grain length in samples
        float pitch_ratio;    // Playback speed multiplier
        float amplitude;      // Grain volume
        uint32_t age;         // Samples since trigger
        bool active;
        float pan_position;   // Stereo placement
    };
    
    Grain grain_pool[64];     // Pre-allocated grain pool
    float next_grain_time;    // Samples until next grain
    uint32_t grain_counter;   // For grain ID/age tracking
};
```

### Processing Flow
1. **Density Timer**: Calculate time to next grain based on density parameter
2. **Grain Spawn**: Create new grain with randomized position/duration/pan
3. **Window Application**: Apply selected window function (Hann/Kaiser/Blackman)
4. **Pitch Shift**: Resample grain at specified pitch ratio
5. **Stereo Spread**: Distribute grains across stereo field
6. **Overlap Sum**: Combine all active grains to output buffer

### Window Functions
```cpp
float applyWindow(float phase, WindowType type) {
    switch(type) {
        case HANN:     return 0.5f * (1.0f - cosf(2.0f * M_PI * phase));
        case KAISER:   return kaiserWindow(phase, 6.0f); // Beta = 6.0
        case BLACKMAN: return 0.42f - 0.5f*cosf(2.0f*M_PI*phase) + 0.08f*cosf(4.0f*M_PI*phase);
    }
}
```

## Parameter Ranges & Defaults

### Core Parameter Mapping
```cpp
struct GranularParams {
    // Mapped from core parameters
    float density = 50.0f;        // HARMONICS -> 1-200 grains/sec  
    float grain_size = 50.0f;     // TIMBRE -> 5-500ms
    float position = 0.5f;        // MORPH -> 0.0-1.0
    
    // Engine-specific extensions
    float jitter = 0.1f;          // 0-1 position randomization
    float spray = 0.05f;          // 0-1 timing randomization  
    uint8_t window_type = 0;      // 0-2 window shape
    float pitch_shift = 0.0f;     // -24 to +24 semitones
    float stereo_spread = 0.7f;   // 0-1 stereo width
};
```

### CPU Control Parameters
```cpp
float max_grain_overlap = 8.0f;  // Max simultaneous grains per voice
float cpu_limit_pct = 15.0f;     // Auto-reduce density if CPU > 15%
bool auto_reduce_quality = true; // Drop grain count under CPU pressure
```

## Implementation Architecture

### Voice vs Shared Processing
- **Per-Voice**: Each voice maintains independent grain scheduler and parameter set
- **Shared Resources**: Sample buffers, window lookup tables, pitch-shift buffers
- **CPU Scaling**: Voices automatically reduce grain density when global CPU > threshold

### Sample Buffer Management
```cpp
class GranularSampleBuffer {
    float* sample_data;
    uint32_t sample_length;
    uint32_t sample_rate;
    bool loop_enabled;
    
    // Circular buffer for real-time input
    float* input_buffer;
    uint32_t input_write_pos;
    uint32_t input_length;
};
```

### Tail Behavior
- **Note Off**: Existing grains fade out over their natural duration (no new grains spawned)
- **Voice Stealing**: Immediate fade-out over 10ms to avoid clicks
- **Tail Time**: Up to longest grain duration (500ms max) for natural decay

## Performance Budget

### CPU Allocation (per voice at 48kHz)
- **Grain Generation**: ~2% CPU (50 grains/sec baseline)
- **Window Processing**: ~1% CPU  
- **Pitch Shifting**: ~3% CPU (if enabled)
- **Stereo Processing**: ~0.5% CPU
- **Total per Voice**: ~6.5% CPU target

### Memory Usage
- **Grain Pool**: 64 grains × 32 bytes = 2KB per voice
- **Sample Buffer**: User-dependent (typically 1-10MB shared)
- **Window Tables**: 4KB shared lookup tables
- **Total**: ~10KB per voice + shared sample memory

### Quality vs Performance Modes
```cpp
enum GranularQuality {
    LOW_CPU,      // Max 4 grains overlap, simple windowing
    BALANCED,     // Max 8 grains overlap, Hann windowing  
    HIGH_QUALITY  // Max 16 grains overlap, Kaiser windowing
};
```

## Engine Integration Notes

### Loading Sample Sources
```cpp
bool loadSample(const char* filepath, int slot = 0);
void setInputSource(bool use_realtime_input);  // Use live audio input
void setLoopMode(bool loop_enabled);
```

### Real-time Parameter Response
- **Density/Size Changes**: Applied to next grain spawn (no discontinuity)  
- **Position Changes**: Smoothed over 10ms to avoid clicks
- **Pitch Changes**: Applied immediately with interpolation
- **Jitter/Spray**: Random seed updates every 100ms for variety

### MIDI/Note Behavior  
- **Note On**: Triggers grain generation at specified density
- **Velocity**: Scales overall grain amplitude (0.1-1.0 range)
- **Note Off**: Stops new grain generation, allows natural decay
- **Pitch Bend**: Modulates grain pitch shift parameter
- **Aftertouch**: Can be mapped to position or jitter parameters