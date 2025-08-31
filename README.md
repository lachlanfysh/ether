# EtherSynth

A comprehensive modular synthesizer with advanced C++ audio engines and SwiftUI interface.

## Project Architecture

### Core Audio Engines (`src/engines/`)
- **MacroVAEngine**: Virtual analog synthesizer with classic subtractive synthesis
- **MacroFMEngine**: 4-operator FM synthesis engine
- **MacroWTEngine**: Wavetable synthesis with morphing capabilities
- **MacroWSEngine**: Waveshaping synthesis engine
- **ElementsVoiceEngine**: Physical modeling synthesis (Mutable Instruments Elements inspired)
- **RingsVoiceEngine**: String/modal synthesis (Mutable Instruments Rings inspired)
- **TidesOscEngine**: Complex oscillator with advanced modulation
- **FormantEngine**: Vowel-based formant synthesis
- **NoiseEngine**: Advanced noise generation and filtering
- **SamplerSlicerEngine**: Sample playback with real-time slicing
- **SerialHPLPEngine**: Serial high-pass/low-pass filtering
- **SlideAccentBassEngine**: Classic TB-303 style bass synthesis

### Audio Processing (`src/audio/`)
- **AudioEngine**: Master audio processing coordinator
- **VoiceManager**: Polyphonic voice allocation and management
- **EffectsChain**: Real-time audio effects processing
- **Timeline**: Sequencer timing and transport control
- **ModulationMatrix**: Advanced parameter modulation routing

### Hardware Interface (`src/hardware/`)
- **MacHardware**: PortAudio integration for cross-platform audio I/O
- **MIDIInterface**: MIDI input/output handling
- **ControllerInterface**: Hardware controller integration

### Pattern System (`src/data/`)
- **PatternData**: Step sequencer pattern storage and manipulation
- **InstrumentSlot**: Per-instrument configuration and state
- **StepSlot**: Individual step data with velocity, probability, and effects
- **EuclideanRhythm**: Algorithmic rhythm generation

## Features

### Synthesis Engines
- 12+ high-quality synthesis engines
- Real-time parameter modulation
- Advanced LFO and envelope systems
- Cross-engine morphing capabilities

### Sequencing
- 16-step pattern sequencer
- Per-step velocity, probability, and microtiming
- Pattern chaining and song mode
- Euclidean rhythm generation
- Advanced step modulation (accents, ratchets, ties)

### Audio Processing  
- Low-latency PortAudio integration
- Real-time effects processing
- LUFS normalization and limiting
- DC blocking and anti-aliasing

### Hardware Integration
- MIDI controller support
- Grid controller compatibility (norns/monome style)
- Real-time parameter control via hardware

## Building

### Requirements
- Swift 5.5+
- PortAudio library (`brew install portaudio`)
- macOS 11+ or Linux

### Swift Package Build
```bash
swift build
.build/debug/EtherSynth
```

### C++ Only Build
```bash
g++ -std=c++17 -I/opt/homebrew/include -L/opt/homebrew/lib -lportaudio src/main.cpp src/**/*.cpp -o ethersynth
./ethersynth
```

## Usage

The synthesizer supports both GUI and terminal interfaces:

- **GUI Mode**: Full SwiftUI interface with visual parameter control
- **Terminal Mode**: Text-based interface optimized for hardware controllers
- **Grid Mode**: Hardware grid controller integration (norns/monome compatible)

## Architecture Highlights

This project showcases several advanced audio programming concepts:

1. **Modular Engine Architecture**: Each synthesis engine is self-contained with standardized interfaces
2. **Real-time Safe Processing**: Lock-free audio processing with careful memory management  
3. **Advanced Parameter System**: Normalized parameter ranges with display formatting
4. **Pattern-based Sequencing**: Flexible step sequencer with per-step modulation
5. **Cross-platform Audio**: PortAudio integration for consistent low-latency audio
6. **Hardware Integration**: Abstracted hardware interfaces for multiple controller types

The C++ audio engines represent significant work in digital signal processing, implementing everything from classic analog modeling to advanced physical modeling synthesis.

## License

[Your License Here]

## Contributing

This project demonstrates advanced audio programming techniques and modular synthesizer design. The codebase is well-structured for learning and extension.