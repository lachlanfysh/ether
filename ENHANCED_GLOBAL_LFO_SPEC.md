# Enhanced Global LFO System Specification

## Overview
The Enhanced Global LFO System provides 8 global LFOs per instrument slot with comprehensive modulation capabilities, tempo sync, and advanced waveform generation.

## Features Implemented

### 1. Tempo Divisions (13 total)
- **4 Bars** - 1/16 BPS (16 beats per cycle)
- **2 Bars** - 1/8 BPS (8 beats per cycle) 
- **1 Bar** - 1/4 BPS (4 beats per cycle)
- **1/2** - 1/2 BPS (2 beats per cycle)
- **1/4** - 1 BPS (1 beat per cycle)
- **1/8** - 2 BPS (2 beats per cycle)
- **1/16** - 4 BPS (4 beats per cycle)
- **1/4.** (dotted) - 2/3 BPS
- **1/8.** (dotted) - 4/3 BPS  
- **1/16.** (dotted) - 8/3 BPS
- **1/4T** (triplet) - 3 BPS
- **1/8T** (triplet) - 6 BPS
- **1/16T** (triplet) - 12 BPS

### 2. Sync Modes (5 total)
- **FREE** - Free running at specified Hz rate
- **TEMPO** - Synchronized to host/internal tempo using divisions
- **KEY** - Restart phase on key trigger
- **ONESHOT** - Single cycle then stop
- **ENV** - ADSR envelope mode with per-LFO attack/decay/sustain/release

### 3. Waveforms (9 total)
- **Sine** - Classic sine wave
- **Triangle** - Linear triangle wave
- **Saw Up** - Rising sawtooth
- **Saw Down** - Falling sawtooth  
- **Square** - 50% duty cycle square wave
- **Pulse** - Variable pulse width (10%-90%)
- **Sample & Hold** - Stepped random values
- **Noise** - Smoothed random noise
- **Exp Up** - Exponential rise curve

### 4. Smoothing and Clamping
- Built-in smoothing with 95% coefficient for click-free operation
- All parameters clamped to safe ranges
- Block-based processing for CPU efficiency
- Per-sample modulation application

## API Functions

### Core LFO Configuration
```c
void ether_set_lfo_waveform(void* synth, int instrument, int lfoIndex, int waveform);
void ether_set_lfo_rate(void* synth, int instrument, int lfoIndex, float rateHz);
void ether_set_lfo_depth(void* synth, int instrument, int lfoIndex, float depth);
void ether_set_lfo_sync_mode(void* synth, int instrument, int lfoIndex, int syncMode);
void ether_set_lfo_active(void* synth, int instrument, int lfoIndex, bool active);
```

### Enhanced Features
```c
void ether_set_lfo_tempo_division(void* synth, int instrument, int lfoIndex, int division);
void ether_set_lfo_pulse_width(void* synth, int instrument, int lfoIndex, float width);
void ether_set_lfo_adsr(void* synth, int instrument, int lfoIndex, float attack, float decay, float sustain, float release);
```

### Parameter Assignment
```c
void ether_assign_lfo_to_param(void* synth, int instrument, int param, int lfoIndex, float depth);
void ether_unassign_lfo_from_param(void* synth, int instrument, int param, int lfoIndex);
void ether_clear_all_lfo_assignments(void* synth, int instrument);
```

### Triggers
```c
void ether_trigger_lfo_key_sync(void* synth, int instrument);
void ether_trigger_lfo_envelope(void* synth, int instrument, int lfoIndex, bool noteOn);
```

## Implementation Details

### Files Created/Modified
- **src/modulation/GlobalLFOSystem.h** - Complete LFO system header
- **src/modulation/GlobalLFOSystem.cpp** - Implementation with all features
- **harmonized_13_engines_bridge.cpp** - Integration patch
- **EtherSynthBridge.h** - Enhanced API declarations

### Performance Characteristics
- **CPU Usage**: Low - block-based processing with per-sample modulation
- **Memory**: ~2KB per slot for LFO state
- **Thread Safety**: Lock-free design suitable for real-time audio
- **Latency**: Single-block processing latency

### Integration Points
1. **Initialization**: GlobalLFOSystem created in setSampleRate()
2. **Processing**: processBlock() called once per audio block
3. **Parameter Application**: getCombinedValue() called per parameter
4. **Triggers**: Key sync and envelope triggers integrated with note events

## Usage Examples

### Basic LFO Setup
```c
// Set LFO 0 to sine wave, 2Hz, full depth, free running
ether_set_lfo_waveform(synth, 0, 0, 0);  // Sine
ether_set_lfo_rate(synth, 0, 0, 2.0f);
ether_set_lfo_depth(synth, 0, 0, 1.0f);
ether_set_lfo_sync_mode(synth, 0, 0, 0); // Free
ether_set_lfo_active(synth, 0, 0, true);

// Assign to filter cutoff with 50% depth
ether_assign_lfo_to_param(synth, 0, PARAM_FILTER_CUTOFF, 0, 0.5f);
```

### Tempo Sync Setup  
```c
// Set LFO 1 to tempo sync, 1/4 note divisions
ether_set_lfo_sync_mode(synth, 0, 1, 1); // Tempo sync
ether_set_lfo_tempo_division(synth, 0, 1, 4); // Quarter note
```

### Envelope Mode Setup
```c
// Set LFO 2 to envelope mode with custom ADSR
ether_set_lfo_sync_mode(synth, 0, 2, 4); // Envelope mode
ether_set_lfo_adsr(synth, 0, 2, 0.1f, 0.3f, 0.7f, 0.5f); // A=100ms, D=300ms, S=0.7, R=500ms

// Trigger envelope on note events
ether_trigger_lfo_envelope(synth, 0, 2, true);  // Note on
ether_trigger_lfo_envelope(synth, 0, 2, false); // Note off
```

## Testing and Validation

The enhanced system maintains backward compatibility with existing LFO API while adding comprehensive new features. All parameters are range-validated and the system provides smooth, click-free modulation suitable for professional audio applications.

## Build Integration

Add to Makefile:
```makefile
SOURCES += src/modulation/GlobalLFOSystem.cpp
```

Apply patches:
```bash
patch -p1 < enhanced_lfo_bridge_patch.diff
patch -p1 < enhanced_lfo_header_patch.diff  
patch -p1 < enhanced_lfo_complete_patch.diff
```