# ether Portable Synthesizer

A professional-grade, portable synthesizer with advanced synthesis engines, comprehensive modulation, and cross-platform capabilities.

## 🎹 Overview

The ether synthesizer is a sophisticated 26-key portable synthesizer featuring:
- **8-instrument color architecture** (ROYGBIV + Grey)
- **Multiple synthesis engines** (Subtractive, FM, Wavetable, Granular)
- **Advanced modulation matrix** with 20+ sources
- **Professional MIDI support** with MIDI learn
- **Cross-platform development** (Mac prototype → STM32 production)
- **NSynth-inspired touch interface** for spatial control

## 🏗️ Architecture

### Core Components

```
ether/
├── src/
│   ├── core/           # Core system (EtherSynth, PresetManager)
│   ├── audio/          # Audio engine and voice management
│   ├── synthesis/      # Synthesis engines (Subtractive, FM, Wavetable, Granular)
│   ├── instruments/    # 8-color instrument slots
│   ├── modulation/     # Modulation matrix and routing
│   ├── effects/        # Effects processing chain
│   ├── sequencer/      # Timeline and rhythm generation
│   ├── hardware/       # Hardware abstraction layer
│   └── midi/           # MIDI input/output management
├── Sources/            # SwiftUI Mac interface
├── test_build.cpp      # Test harness
└── Makefile           # Build system
```

### Hardware Abstraction

The synthesizer uses a hardware abstraction layer (HAL) that allows the same codebase to run on:
- **Mac** (prototype with Core Audio)
- **STM32** (production hardware with TouchGFX)

## 🎛️ Synthesis Engines

### 1. Subtractive Engine
Classic analog-style subtractive synthesis with:
- Dual oscillators with sync and FM
- 24dB/oct lowpass filter with resonance
- ADSR envelopes
- LFO modulation

### 2. FM Engine
4-operator FM synthesis featuring:
- DX7-style algorithms (32 configurations)
- Per-operator ADSR envelopes
- Real-time modulation matrix
- Classic FM sounds (E.Piano, Bells, Bass)

### 3. Wavetable Engine
Morphing wavetable synthesis with:
- 64 wavetables with real-time morphing
- Touch X/Y control for wavetable navigation
- Spectral processing capabilities
- Modern wavetable sounds

### 4. Granular Engine
Real-time granular synthesis offering:
- Up to 64 grains per voice
- 6 texture modes (Forward, Reverse, Ping-pong, Random, Freeze, Stretch)
- 8 source waveforms
- Atmospheric and evolving textures

## 🎚️ Modulation System

### Advanced Modulation Matrix
- **20+ Modulation Sources**: Smart Knob, Touch X/Y, Aftertouch, LFOs, Envelopes, Audio Analysis, etc.
- **Curve Processing**: Exponential, Logarithmic, S-Curve, Quantized, Sample & Hold
- **Conditional Modulation**: Route modulation based on other sources
- **Macro Controls**: Combine multiple sources into macro knobs

### LFO System
- **3 Independent LFOs** with multiple waveforms
- **Sync Options**: Free-running or note-sync
- **Phase Control**: Rhythmic modulation patterns

### Modulation Templates
Pre-built setups for common modulation scenarios:
- Classic Filter Sweep (LFO → Filter)
- Performance Touch (Touch → Multiple Parameters)
- Audio Reactive (Audio Level → Filter + Resonance)
- Chaotic Modulation (Random → Everything)

## 🎨 8-Color Instrument Architecture

Each color represents a different instrument slot:
- **Red**: Fire/Aggressive Bass
- **Orange**: Warm Lead
- **Yellow**: Bright Pluck
- **Green**: Organic Pad
- **Blue**: Deep Strings
- **Indigo**: Mystic FX
- **Violet**: Ethereal Lead
- **Grey**: Utility/Template

## 🎵 MIDI Integration

### Complete MIDI Support
- **Device Detection**: Automatic MIDI device scanning
- **MIDI Learn**: One-click parameter assignment
- **Standard Mappings**: CC7 (Volume), CC71 (Filter), etc.
- **Velocity Curves**: Linear, Exponential, Logarithmic
- **Transpose**: ±24 semitones
- **Clock Sync**: MIDI clock, start/stop

### MIDI Features
- Full MIDI 1.0 implementation
- Latency compensation
- MIDI thru
- SysEx support for preset transfer

## 🎛️ Preset Management

### Factory Presets (18 included)
**Classic Emulations**:
- TB-303 Bass, Moog Lead, DX7 E.Piano, Juno Strings

**Modern Sounds**:
- Future Bass, Dubstep Wobble, Ambient Texture, Granular Cloud

**Color-Themed**:
- Red Fire, Orange Warm, Yellow Bright, Green Organic, etc.

### Preset Features
- **Categories**: Bass, Lead, Pad, Pluck, FX, Percussion
- **Import/Export**: Single presets or banks
- **Preset Morphing**: Blend between any two presets
- **Favorites System**: Quick access to preferred sounds
- **Auto-Save**: Automatic backup of current state

## 🖥️ User Interface

### SwiftUI Mac Interface
NSynth-inspired design with:
- **960×320 display simulation** for hardware matching
- **Spatial touch control** with X/Y parameter mapping
- **8-color instrument selection** with visual feedback
- **Smart knob visualization** with parameter assignment
- **Real-time waveform display** and performance monitoring

### Interface Panels
- **Left Panel**: Instrument selection and smart knob
- **Center Panel**: Spatial touch control and synthesis parameters
- **Right Panel**: Effects, modulation, and global controls

## 🔧 Development

### Building for Mac
```bash
make clean
make test_build
./test_build
```

### SwiftUI Interface (requires Xcode)
```bash
# Open in Xcode after macOS update
open Sources/EtherSynth.xcodeproj
```

### Requirements
- **Mac**: Xcode, Core Audio framework
- **STM32**: TouchGFX, STM32CubeIDE (future)

## 📊 Performance

### Specifications
- **Sample Rate**: 48kHz, 24-bit
- **Latency**: <10ms (Mac: ~2.7ms)
- **Polyphony**: 8 voices per instrument
- **CPU Monitoring**: Real-time performance tracking
- **Memory**: Efficient preset management and caching

### Optimization Features
- Smart parameter updates
- Voice stealing algorithms
- Efficient modulation processing
- Minimal memory footprint

## 🎯 Hardware Target

### Planned Hardware
- **26-key keyboard** with polyphonic aftertouch
- **960×320 color display** with touch interface
- **Smart knob** with LED ring and tactile feedback
- **STM32 microcontroller** for real-time processing
- **Battery powered** for true portability

### Development Strategy
1. **Mac Prototype**: Full feature development and testing
2. **Hardware Simulation**: Exact display and interaction matching
3. **STM32 Port**: TouchGFX UI with optimized C++ engine
4. **Hardware Integration**: Final assembly and calibration

## 🚀 Getting Started

### Quick Start
1. Clone the repository
2. Run `make test_build` to compile
3. Execute `./test_build` to run audio engine test
4. Install Xcode for full SwiftUI interface
5. Connect MIDI controller for hardware interaction

### First Steps
1. **Audio Test**: Verify Core Audio initialization
2. **MIDI Setup**: Connect MIDI keyboard and test note input
3. **Preset Loading**: Try factory presets across different engines
4. **Modulation**: Experiment with touch X/Y control
5. **MIDI Learn**: Map your hardware controls to synthesis parameters

## 📚 API Reference

### Core Classes
- `EtherSynth`: Main synthesizer class
- `AudioEngine`: Real-time audio processing
- `SynthEngine`: Base class for synthesis engines
- `AdvancedModulationMatrix`: Modulation routing and processing
- `PresetManager`: Preset loading, saving, and management
- `MIDIManager`: MIDI input/output and device management

### Key Methods
```cpp
// Note control
void noteOn(uint8_t note, float velocity, float aftertouch = 0.0f);
void noteOff(uint8_t note);

// Parameter control  
void setParameter(ParameterID param, float value);
float getParameter(ParameterID param) const;

// Modulation
void addModulation(ModSource source, ParameterID dest, float amount);
float getModulatedValue(ParameterID param, float baseValue) const;

// Presets
bool savePreset(const Preset& preset);
bool loadPreset(const std::string& name, Preset& preset);
```

## 🔮 Future Enhancements

### Planned Features
- **Pattern Sequencer**: Advanced step sequencing with Euclidean rhythms
- **Advanced Effects**: Reverb, delay, chorus, distortion
- **Sample Playback**: WAV file loading and manipulation
- **Chord Generator**: Intelligent harmony and voicing
- **Wireless MIDI**: Bluetooth connectivity for mobile control

### Hardware Extensions
- **Expansion Ports**: Additional controls and I/O
- **SD Card**: Sample storage and preset banks
- **Audio I/O**: Line in/out for processing external audio
- **Expression Pedals**: Continuous controller inputs

## 🤝 Contributing

This is a personal project, but feedback and suggestions are welcome! The codebase is designed to be modular and extensible.

## 📄 License

Personal project - All rights reserved.

---

*The ether synthesizer represents a modern approach to portable synthesis, combining classic synthesis techniques with contemporary interface design and professional-grade audio processing.*