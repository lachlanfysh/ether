# Global LFO Implementation Specification

## Data Model

### Assignment Structure
```cpp
struct LFOAssignment {
    uint8_t instrument_id;     // 0-15 instrument slot
    uint8_t parameter_id;      // CoreParameter enum value
    float depth;               // -1.0 to 1.0 (negative = inverted)
    float base_value;          // Parameter value before modulation
};

struct GlobalLFO {
    float rate;                // 0.01 - 20.0 Hz
    float phase;               // 0.0 - 1.0 current phase
    uint8_t waveform;          // 0=sine, 1=saw, 2=square, 3=random
    bool key_sync;             // Reset phase on note trigger
    bool envelope_mode;        // LFO amplitude follows ADSR
    std::vector<LFOAssignment> assignments; // Multiple parameter assignments
    std::atomic<bool> dirty;   // Thread-safe update flag
};

// Global state: 8 LFOs shared across all instruments
extern GlobalLFO g_global_lfos[8];
extern std::mutex g_lfo_mutex; // Protects assignment changes only
```

### Thread Safety
- **Audio Thread**: Read-only access to LFO state, lockless processing
- **UI Thread**: Modifies assignments via mutex-protected functions
- **Smoothing**: 10ms parameter interpolation to avoid clicks during live changes

## Bridge Functions

### Core Assignment Functions
```cpp
// Assign LFO to parameter with depth control
int ether_assign_lfo_to_param_id(void* engine, uint8_t lfo_id, uint8_t instrument_id, 
                                 uint8_t parameter_id, float depth);
// Returns: 1=success, 0=invalid indices, -1=assignment limit reached

// Remove all LFO assignments from a parameter
int ether_remove_lfo_assignment_by_param(void* engine, uint8_t instrument_id, uint8_t parameter_id);
// Returns: number of assignments removed

// Remove specific LFO assignment
int ether_remove_lfo_assignment(void* engine, uint8_t lfo_id, uint8_t instrument_id, uint8_t parameter_id);
// Returns: 1=success, 0=not found
```

### LFO Configuration Functions
```cpp
// Set LFO parameters
void ether_set_lfo_rate(void* engine, uint8_t lfo_id, float rate_hz);        // 0.01-20.0 Hz
void ether_set_lfo_depth(void* engine, uint8_t lfo_id, float depth);         // 0.0-1.0 global multiplier
void ether_set_lfo_waveform(void* engine, uint8_t lfo_id, uint8_t waveform); // 0-3
void ether_set_lfo_key_sync(void* engine, uint8_t lfo_id, bool enabled);     // Reset on note trigger
void ether_set_lfo_envelope_mode(void* engine, uint8_t lfo_id, bool enabled); // Follow ADSR envelope

// Get LFO parameters
float ether_get_lfo_rate(void* engine, uint8_t lfo_id);
uint8_t ether_get_lfo_waveform(void* engine, uint8_t lfo_id);
bool ether_get_lfo_key_sync(void* engine, uint8_t lfo_id);
```

### Query Functions
```cpp
// Get parameter LFO info: bitmask of active LFOs + current combined modulation value
struct ParameterLFOInfo {
    uint8_t active_lfo_mask;   // Bit 0-7 for LFO 0-7 assignments
    float current_value;       // Combined modulated parameter value
    float base_value;          // Unmodulated parameter value
    int assignment_count;      // Number of LFO assignments to this parameter
};

ParameterLFOInfo ether_get_parameter_lfo_info(void* engine, uint8_t instrument_id, uint8_t parameter_id);

// Get global LFO activity mask (which LFOs have any assignments)
uint8_t ether_get_global_lfo_mask(void* engine);
```

## Processing Integration

### Modulation Application Order
1. **Engine-Internal Modulation** (if engine supports native LFO routing)
2. **Post-Chain Modulation** (always applied):
   - HPF cutoff (PARAM_HPF)
   - LPF cutoff/resonance (PARAM_FILTER_CUTOFF, PARAM_FILTER_RESONANCE)
   - Pre-gain (PARAM_AMPLITUDE)
   - Soft clipping (PARAM_CLIP)
   - Pan position (PARAM_PAN)
   - Master volume (PARAM_VOLUME)

### Processing Algorithm (per audio block)
```cpp
void processGlobalLFOs(float sampleRate, int blockSize) {
    float dt = blockSize / sampleRate;
    
    for (int lfo = 0; lfo < 8; lfo++) {
        GlobalLFO& lfo_state = g_global_lfos[lfo];
        if (lfo_state.assignments.empty()) continue;
        
        // Update LFO phase
        lfo_state.phase += lfo_state.rate * dt;
        if (lfo_state.phase >= 1.0f) lfo_state.phase -= 1.0f;
        
        // Generate waveform value (-1.0 to 1.0)
        float lfo_value = generateWaveform(lfo_state.waveform, lfo_state.phase);
        
        // Apply envelope modulation if enabled
        if (lfo_state.envelope_mode) {
            lfo_value *= getCurrentEnvelopeLevel(); // From active ADSR
        }
        
        // Apply to all parameter assignments
        for (const auto& assignment : lfo_state.assignments) {
            float modulation = lfo_value * assignment.depth;
            applyParameterModulation(assignment.instrument_id, assignment.parameter_id, modulation);
        }
    }
}
```

### Parameter Bounds & Scaling
- **Bounds Checking**: All modulated values clamped to parameter ranges from CoreParameters.h
- **Scaling**: LFO modulation applied as: `final_value = base_value + (lfo_value * depth * parameter_range)`
- **Polarity**: Negative depth values invert LFO modulation

### Key-Trigger Integration
```cpp
void onNoteOn(uint8_t note, float velocity) {
    for (int i = 0; i < 8; i++) {
        if (g_global_lfos[i].key_sync) {
            g_global_lfos[i].phase = 0.0f; // Reset phase to 0
        }
    }
}
```

## Testing Hooks

### Verification Points
```cpp
// Test 1: Assignment Verification
ParameterLFOInfo info = ether_get_parameter_lfo_info(engine, instrument_id, parameter_id);
assert(info.active_lfo_mask & (1 << lfo_id)); // LFO assigned
assert(info.assignment_count > 0);            // Has assignments

// Test 2: Modulation Activity
float base = ether_get_core_parameter(engine, parameter_id);
// ... trigger note, process some audio blocks ...
float modulated = ether_get_core_parameter(engine, parameter_id);
assert(abs(modulated - base) > 0.01f); // Parameter is being modulated

// Test 3: Waveform Verification
ether_set_lfo_waveform(engine, 0, 2); // Square wave
assert(ether_get_lfo_waveform(engine, 0) == 2);

// Test 4: Key-Sync Behavior
float phase_before = g_global_lfos[0].phase;
ether_note_on(engine, 60, 0.8f, 0.0f);
float phase_after = g_global_lfos[0].phase;
if (key_sync_enabled) assert(phase_after == 0.0f);

// Test 5: Global Activity Mask
uint8_t global_mask = ether_get_global_lfo_mask(engine);
assert(global_mask != 0); // At least one LFO active after assignments
```

### Debug Logging
```cpp
#ifdef LFO_DEBUG
    printf("LFO %d: rate=%.2f, phase=%.3f, assignments=%d\n", 
           lfo_id, rate, phase, assignment_count);
    printf("Param %d modulation: base=%.3f, lfo=%.3f, final=%.3f\n",
           param_id, base_value, lfo_contribution, final_value);
#endif
```

## Performance Considerations

- **Assignment Limit**: Max 8 assignments per parameter (one per LFO)
- **Processing Cost**: ~0.5% CPU per active LFO at 48kHz
- **Memory**: ~2KB total for all LFO state
- **Smoothing Buffer**: 480 samples at 48kHz for parameter interpolation