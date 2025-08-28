# EtherSynth Velocity Modulation System

## Table of Contents
1. [Overview](#overview)
2. [System Architecture](#system-architecture)
3. [Core Components](#core-components)
4. [Getting Started](#getting-started)
5. [Advanced Features](#advanced-features)
6. [Engine-Specific Configurations](#engine-specific-configurations)
7. [Preset System](#preset-system)
8. [Performance Optimization](#performance-optimization)
9. [Troubleshooting](#troubleshooting)
10. [Technical Reference](#technical-reference)

## Overview

EtherSynth's velocity modulation system transforms how MIDI velocity affects synthesis parameters, providing unprecedented control over musical expression. Unlike traditional static velocity mapping, this system offers dynamic curve shaping, per-parameter depth control, engine-specific routing, and comprehensive preset management.

### Key Features
- **Advanced Curve Shaping**: 6 velocity response curves (Linear, Exponential, Logarithmic, S-Curve, Power, Stepped)
- **Flexible Depth Control**: 0-200% modulation depth with global and per-parameter scaling
- **Engine-Specific Mapping**: Optimized velocity routing for all 32+ synthesis engines
- **Comprehensive Presets**: 96+ factory presets plus custom preset creation
- **Real-time Performance**: Optimized for 100+ simultaneous voices with <1ms latency
- **Professional Integration**: JSON schema for preset exchange and backup

## System Architecture

```
┌─────────────────┐    ┌────────────────────┐    ┌─────────────────────┐
│   MIDI Input    │───▶│  Velocity System   │───▶│   Synthesis Engine  │
│   (Velocity)    │    │                    │    │   (Parameters)      │
└─────────────────┘    └────────────────────┘    └─────────────────────┘
                                │
                                ▼
                       ┌─────────────────────┐
                       │   Depth Control     │
                       │   Curve Shaping     │
                       │   Engine Mapping    │
                       │   Preset Management │
                       └─────────────────────┘
```

The velocity modulation system operates as a sophisticated parameter transformation layer between MIDI input and synthesis engines, providing musical control over how key velocity translates to sound characteristics.

## Core Components

### 1. Relative Velocity Modulation

The foundation of the system, providing advanced velocity curve processing:

#### Modulation Modes
- **ABSOLUTE**: Direct velocity-to-parameter mapping (standard MIDI behavior)
- **RELATIVE**: Velocity modulates change from current parameter value
- **ADDITIVE**: Velocity adds/subtracts from base parameter value
- **MULTIPLICATIVE**: Velocity scales base parameter proportionally
- **ENVELOPE**: Velocity creates envelope-style modulation over time
- **BIPOLAR_CENTER**: Bidirectional modulation from center point

#### Curve Types
- **LINEAR**: Direct 1:1 relationship (y = x)
- **EXPONENTIAL**: More sensitivity at low velocities (y = x^a where a > 1)
- **LOGARITHMIC**: More sensitivity at high velocities (y = log(x))
- **S_CURVE**: Gentle at extremes, steep in middle (sigmoid function)
- **POWER_CURVE**: Configurable power law with adjustable exponent
- **STEPPED**: Quantized velocity levels for rhythmic effects

### 2. Velocity Depth Control

Comprehensive depth management with safety controls:

#### Global Depth Control
- **Master Depth**: 0-200% affecting all velocity-modulated parameters
- **Safety Limiting**: Prevents runaway modulation with automatic clamping
- **Smooth Transitions**: Parameter interpolation to avoid audio artifacts
- **Emergency Limits**: System protection with 150% hard limit

#### Per-Parameter Depth
- **Individual Scaling**: Override global depth for specific parameters
- **Parameter-Specific Limits**: Tailored maximum depths per parameter type
- **Depth Presets**: Save/recall depth configurations for different contexts
- **Real-time Monitoring**: Live depth adjustment with visual feedback

### 3. Engine-Specific Velocity Mapping

Optimized parameter routing for each synthesis engine:

#### Main Macro Engines
- **MacroVA**: Attack time, filter cutoff, oscillator levels, PWM depth
- **MacroFM**: Modulation index, carrier/modulator balance, envelope rates
- **MacroHarmonics**: Drawbar levels, percussion intensity, scanner rate
- **MacroWavetable**: Table position, filter tracking, morphing speed
- **MacroChord**: Voice spread, detuning amount, harmonic content
- **MacroWaveshaper**: Fold amount, symmetry, harmonic emphasis

#### Physical Modeling Engines
- **Physical Modeling**: Bow pressure, excitation level, damping control
- **Modal Resonator**: Strike level, resonance structure, decay time
- **Slope Generator**: Shape amount, frequency tracking, modulation depth

#### Specialized Engines
- **Granular**: Grain density, texture parameters, position spread
- **Additive**: Partial levels, harmonic brightness, spectral tilt
- **Drum Synthesis**: Impact velocity, decay time, pitch envelope
- **Sample Playback**: Volume curve, filter tracking, start position

### 4. Velocity Volume Control

Independent velocity-to-volume processing:

#### Core Features
- **Global Enable/Disable**: Complete bypass of velocity→volume for advanced synthesis
- **Multiple Curve Types**: Same curve options as parameter modulation
- **Per-Engine Configuration**: Engine-specific volume response characteristics
- **Integration Override**: Works alongside or instead of parameter modulation

#### Use Cases
- **Pure Synthesis**: Disable velocity→volume for constant-level synthesis
- **Hybrid Control**: Velocity affects both volume and synthesis parameters
- **Performance Dynamics**: Traditional piano-style volume response
- **Sound Design**: Non-traditional velocity-to-dynamics relationships

## Getting Started

### Basic Setup

1. **Enable Velocity Modulation**
   - Access HUD overlay with velocity controls
   - Toggle global velocity enable (default: ON)
   - Verify depth is set to 100% for normal response

2. **Select Velocity Curve**
   - Choose curve type based on musical needs:
     - **Linear**: Natural, predictable response
     - **Exponential**: Piano-like expression
     - **S-Curve**: Balanced sensitivity across range

3. **Adjust Global Depth**
   - Start with 75-100% for natural response
   - Increase to 125-150% for dramatic expression
   - Reduce to 25-50% for subtle effects

### Quick Start Presets

#### "Natural Expression" (Recommended Starting Point)
```json
{
  "global_depth": 100,
  "curve_type": "EXPONENTIAL",
  "curve_amount": 1.2,
  "volume_enabled": true,
  "engine_mappings": "auto"
}
```

#### "Dramatic Dynamics"
```json
{
  "global_depth": 150,
  "curve_type": "S_CURVE",
  "curve_amount": 2.0,
  "volume_enabled": true,
  "engine_mappings": "enhanced"
}
```

#### "Subtle Textures"
```json
{
  "global_depth": 40,
  "curve_type": "LINEAR",
  "curve_amount": 1.0,
  "volume_enabled": false,
  "engine_mappings": "minimal"
}
```

## Advanced Features

### Custom Curve Creation

Create personalized velocity response curves:

```cpp
// Example: Custom exponential curve with variable steepness
class CustomVelocityCurve {
    float processVelocity(float input, float steepness) {
        return std::pow(input, steepness);
    }
};
```

### Per-Parameter Configuration

Fine-tune individual parameter responses:

```json
{
  "parameter_configs": {
    "filter_cutoff": {
      "depth": 120,
      "curve": "LOGARITHMIC",
      "curve_amount": 1.5,
      "enabled": true
    },
    "env_attack": {
      "depth": 80,
      "curve": "EXPONENTIAL",
      "curve_amount": 2.0,
      "invert": true
    }
  }
}
```

### Velocity History Smoothing

Smooth velocity response for consistent expression:

- **Moving Average**: Smooths velocity over N samples
- **Exponential Smoothing**: Gradual response with decay constant
- **Peak Hold**: Maintains peak velocity with slow decay
- **RMS Average**: Energy-based smoothing for percussive sources

### Envelope-Driven Modulation

Create time-based velocity effects:

```cpp
struct VelocityEnvelope {
    float attack_time;   // 1-1000ms
    float release_time;  // 10-5000ms
    float shape_curve;   // Linear to exponential
};
```

## Engine-Specific Configurations

### MacroVA Configuration
Optimized for analog-style synthesis:

```json
{
  "engine_type": "MACRO_VA",
  "mappings": {
    "ENV_ATTACK": {"amount": -0.6, "curve": "EXPONENTIAL"},
    "FILTER_CUTOFF": {"amount": 0.8, "curve": "LOGARITHMIC"},
    "OSC_LEVEL": {"amount": 0.4, "curve": "LINEAR"},
    "NOISE_LEVEL": {"amount": 0.3, "curve": "LINEAR"}
  }
}
```

### MacroFM Configuration
Tailored for FM synthesis expression:

```json
{
  "engine_type": "MACRO_FM",
  "mappings": {
    "MOD_INDEX": {"amount": 1.0, "curve": "S_CURVE"},
    "CARRIER_LEVEL": {"amount": 0.6, "curve": "LINEAR"},
    "MOD_ENVELOPE": {"amount": 0.8, "curve": "EXPONENTIAL"},
    "FEEDBACK": {"amount": 0.4, "curve": "POWER", "exponent": 1.5}
  }
}
```

### Physical Modeling Configuration
Realistic physical instrument response:

```json
{
  "engine_type": "PHYSICAL_MODELING",
  "mappings": {
    "BOW_PRESSURE": {"amount": 1.2, "curve": "EXPONENTIAL"},
    "EXCITATION": {"amount": 0.9, "curve": "LINEAR"},
    "DAMPING": {"amount": -0.3, "curve": "LOGARITHMIC"},
    "RESONANCE": {"amount": 0.5, "curve": "S_CURVE"}
  }
}
```

## Preset System

### Factory Presets

#### Clean Category (Minimal Processing)
- Pristine synthesis tones with subtle velocity response
- Linear curves with moderate depth (60-80%)
- Focus on volume control with minimal parameter modulation
- Ideal for: Pads, ambient textures, pure synthesis

#### Classic Category (Traditional Response)
- Vintage-inspired velocity behavior
- Exponential curves mimicking acoustic instruments
- Balanced parameter and volume modulation
- Ideal for: Piano, electric piano, classic synthesizers

#### Extreme Category (Maximum Expression)
- Dramatic velocity response with high depth (150-200%)
- Complex curve combinations for expressive performance
- Extensive parameter modulation across all engines
- Ideal for: Lead synthesis, dramatic effects, sound design

#### Signature Presets (Hand-Crafted)
- **"Detuned Stack Pad"**: Rich pad with velocity-controlled filter and chorus
- **"2-Op Punch"**: Percussive FM with velocity-driven modulation index
- **"Drawbar Keys"**: Harmonic synthesis with velocity-sensitive drawbars

### Custom Preset Creation

```json
{
  "schema_version": "1.0",
  "preset_info": {
    "name": "Custom Velocity Config",
    "description": "User-created velocity configuration",
    "author": "User",
    "category": "USER_CUSTOM",
    "tags": ["custom", "expressive"]
  },
  "global_config": {
    "master_depth": 125,
    "curve_type": "S_CURVE",
    "curve_amount": 1.5,
    "safety_level": "MODERATE"
  },
  "engine_configs": {
    "MACRO_VA": {
      "mappings": [
        {
          "target": "FILTER_CUTOFF",
          "amount": 0.8,
          "curve": "LOGARITHMIC",
          "enabled": true
        }
      ]
    }
  },
  "velocity_config": {
    "enable_velocity_to_volume": true,
    "velocity_mappings": {
      "volume": 0.7,
      "filter_cutoff": 0.5,
      "attack_time": -0.4
    }
  }
}
```

## Performance Optimization

### Real-time Processing

The velocity modulation system is optimized for real-time audio performance:

#### CPU Performance
- **Per-voice overhead**: <0.1ms processing time
- **100 voices**: <10ms total processing time
- **Memory usage**: <1MB for complete velocity system
- **Cache efficiency**: Optimized data structures for CPU cache

#### Optimization Techniques
- **Lookup Tables**: Pre-computed curves for common operations
- **SIMD Processing**: Vectorized operations where possible
- **Memory Pool**: Preallocated buffers to avoid real-time allocation
- **Branch Prediction**: Optimized conditional logic

### Batch Processing

Process multiple voices efficiently:

```cpp
void processVelocityBatch(const std::vector<VoiceData>& voices) {
    for (const auto& voice : voices) {
        // Vectorized velocity processing
        __m128 velocity_vec = _mm_load_ps(&voice.velocity);
        __m128 curve_result = applyCurveVector(velocity_vec, curveType);
        _mm_store_ps(&voice.processedVelocity, curve_result);
    }
}
```

### Memory Management

- **Lock-free Operations**: Atomic operations for parameter updates
- **Ring Buffers**: Circular buffers for velocity history
- **Object Pooling**: Reuse objects to minimize allocation
- **RAII**: Automatic memory management with smart pointers

## Troubleshooting

### Common Issues

#### No Velocity Response
**Problem**: Velocity changes don't affect sound
**Solutions**:
1. Check global velocity enable (should be ON)
2. Verify master depth > 0%
3. Ensure engine has velocity mappings configured
4. Check MIDI velocity is being received

#### Too Sensitive Response
**Problem**: Small velocity changes cause dramatic sound changes
**Solutions**:
1. Reduce global depth (try 50-75%)
2. Switch to logarithmic curve
3. Enable velocity smoothing
4. Check per-parameter depth settings

#### Choppy/Steppy Response  
**Problem**: Velocity response sounds quantized or stepped
**Solutions**:
1. Disable stepped curve mode
2. Increase velocity smoothing
3. Check CPU load (may cause audio dropouts)
4. Verify sample rate and buffer size settings

#### Inconsistent Response
**Problem**: Velocity response varies unpredictably
**Solutions**:
1. Reset velocity system to defaults
2. Check for conflicting modulation sources
3. Verify preset compatibility with current engine
4. Update to latest system version

### Performance Issues

#### High CPU Usage
- Reduce active voice count
- Disable complex curve types for high voice counts
- Use batch processing mode
- Optimize engine-specific mappings

#### Memory Warnings
- Clear velocity history buffers
- Reduce smoothing history length
- Limit number of active parameters
- Check for memory leaks in custom configurations

### Diagnostic Tools

#### Real-time Monitoring
```cpp
struct VelocityDiagnostics {
    float cpuUsage;           // Current CPU percentage
    size_t activeVoices;      // Number of processing voices
    size_t memoryUsage;       // Memory consumption in bytes
    float avgProcessingTime;  // Average processing time per voice
};
```

#### Debug Logging
Enable detailed logging for velocity processing:
- Parameter value changes
- Curve processing results  
- Engine mapping updates
- Performance metrics

## Technical Reference

### API Reference

#### Core Classes
- `RelativeVelocityModulation`: Advanced velocity curve processing
- `VelocityDepthControl`: Global and per-parameter depth management
- `VelocityVolumeControl`: Independent velocity-to-volume processing
- `EngineVelocityMapping`: Engine-specific parameter routing

#### Configuration Structures
- `VelocityModulationConfig`: Complete velocity configuration
- `ParameterDepthConfig`: Per-parameter depth settings
- `VolumeConfig`: Volume control configuration
- `EngineVelocityConfig`: Engine-specific mappings

### JSON Schema

#### Preset Format
Complete JSON schema for velocity presets with validation rules and compatibility checking.

#### Parameter Mapping
Standardized format for engine-parameter-velocity relationships with versioning support.

### Integration Points

#### MIDI Integration
- Real-time MIDI velocity processing
- Controller mapping for depth adjustment
- MIDI learn for parameter assignment

#### Engine Integration  
- Parameter callback system
- Real-time parameter updates
- Voice management integration

#### UI Integration
- HUD overlay controls
- Real-time status display
- Gesture control integration

### Version History

- **v1.0**: Initial velocity modulation system
- **v1.1**: Added engine-specific mappings
- **v1.2**: Preset system and JSON schema
- **v1.3**: Advanced gesture controls and UI strings
- **v1.4**: Performance optimizations and QA suite

---

*This document provides comprehensive coverage of EtherSynth's velocity modulation system. For additional support, consult the main user manual or contact technical support.*