# Enhanced MacroChord Engine Specification

## Core Concept: Multi-Engine Chord Architecture

Instead of a simple chord generator, the MacroChord engine should be a **meta-engine** that instantiates multiple melodic engines to create rich, complex chord voicings with unprecedented depth and flexibility.

## Architecture Overview

### Multi-Engine Voice Management
- **Up to 5 chord voices** (root + 4 harmonies)
- **Each voice can use a different synthesis engine** (VA, FM, Wavetable, etc.)
- **Per-voice engine selection** allows for complex timbral combinations
- **Intelligent voice allocation** based on chord type and spread

### Engine Instantiation Strategy
```cpp
class MacroChordVoice : public BaseVoice {
private:
    std::unique_ptr<IEngine> voiceEngine_;  // Instantiated melodic engine
    EngineFactory::EngineType engineType_;
    float voiceNote_;                       // Chord note offset from root
    float voiceLevel_;                      // Mix level for this voice
};
```

## Parameter Mapping

### Hero Macros
- **HARMONICS**: Chord type selection (stepped, 0-32 chord types)
  - Major triads: Maj, Maj6, Maj7, Maj9, add9
  - Minor triads: min, min6, min7, min9, minAdd9
  - Dominant: Dom7, Dom9, Dom11, Dom13
  - Diminished: dim, dim7, half-dim7
  - Augmented: aug, aug7
  - Sus chords: sus2, sus4, 7sus4
  - Extended: maj11, maj13, min11, min13
  - Specialty: Hendrix, Quartal, Cluster

- **TIMBRE**: Voice spread (0-24 semitones)
  - Controls interval spread between voices
  - 0 = tight voicing, 24 = wide voicing across 2 octaves
  - Intelligent voice leading and doubling

- **MORPH**: Detune/humanization (0-100 cents)
  - Per-voice random detuning for organic feel
  - Prevents phase cancellation in similar engines
  - Creates ensemble effect

### Engine-Specific Parameters

#### VOICES (3-5 voices)
- **3 Voices**: Root + 3rd + 5th (basic triads)
- **4 Voices**: Root + 3rd + 5th + 7th (extended chords)
- **5 Voices**: Full extended harmony with color tones

#### SOURCE_ENGINES (per voice engine selection)
- **ENGINE_1**: Engine type for voice 1 (root)
- **ENGINE_2**: Engine type for voice 2 (3rd/5th)
- **ENGINE_3**: Engine type for voice 3 (5th/7th)
- **ENGINE_4**: Engine type for voice 4 (extensions)
- **ENGINE_5**: Engine type for voice 5 (color tones)
- Available engines: VA, FM, Wavetable, Waveshaper, Harmonics

#### STRUM_MS (0-100ms)
- **Strum timing**: Delay between voice onsets
- **0ms**: All voices start simultaneously
- **100ms**: Guitar-like strumming effect
- **Direction**: Up-strum or down-strum

#### VOICE_MIX (per-voice level control)
- **Independent volume** for each chord voice
- **Allows bass emphasis** or melody highlighting
- **Dynamic chord balance** based on musical context

## Advanced Features

### Intelligent Voice Leading
- **Smooth voice movement** between chord changes
- **Minimal finger movement** voice leading algorithms
- **Context-aware doubling** (octaves, unisons)
- **Automatic inversion selection** based on spread parameter

### Per-Voice Processing
- **Independent channel strips** for each voice
- **Per-voice effects** (chorus, delay, etc.)
- **Stereo positioning** based on voice role
- **Dynamic range scaling** to prevent muddy mixes

### Engine Combination Presets
Pre-configured engine combinations for musical styles:
- **Jazz**: Root(VA) + 3rd(FM) + 7th(Wavetable) + ext(Harmonics)
- **Orchestral**: All voices using different wavetable banks
- **Vintage**: Root(VA saw) + others(FM bell tones)
- **Modern**: Root(Waveshaper) + harmonies(Granular textures)
- **Ambient**: All voices(Wavetable pads) with heavy processing

### CPU Management
- **Smart engine selection**: Prefer lighter engines for inner voices
- **Dynamic voice allocation**: Reduce voices under CPU pressure
- **Engine sharing**: Reuse engine instances where possible
- **Target**: ≤8k cycles total (distributed across sub-engines)

## Implementation Notes

### Engine Factory Integration
```cpp
class MacroChordEngine : public PolyphonicBaseEngine<MacroChordVoice> {
private:
    struct ChordVoiceConfig {
        EngineFactory::EngineType engineType;
        float noteOffset;    // Semitones from root
        float level;         // Mix level
        float pan;          // Stereo position
    };
    
    std::array<ChordVoiceConfig, 5> voiceConfigs_;
    ChordType currentChordType_;
    int activeVoices_;
};
```

### Memory Considerations
- **Lazy instantiation**: Only create engines for active voices
- **Engine pooling**: Reuse common engine types
- **Preset caching**: Store common chord+engine combinations
- **Graceful degradation**: Fall back to simpler engines under memory pressure

## Musical Benefits

### Unprecedented Flexibility
- **Hybrid timbres**: Mix analog warmth (VA) with digital precision (FM)
- **Evolving chords**: Each voice can have different modulation
- **Complex textures**: Granular bass + FM leads + Wavetable pads
- **Performance expression**: Real-time engine swapping per voice

### Professional Applications
- **Film scoring**: Rich, complex harmonic textures
- **Jazz arrangements**: Sophisticated extended harmony
- **Electronic music**: Unique timbral combinations impossible with single engines
- **Live performance**: Instant access to complex chord sounds

## Technical Requirements

### Real-Time Constraints
- **Sub-engine CPU budgets**: Each voice ≤1.6k cycles average
- **Memory limits**: ≤64KB total for all chord voices
- **Latency**: <1ms additional delay from engine switching
- **Stability**: No glitches during engine changes

### Integration Points
- **Parameter automation**: All sub-engine parameters accessible
- **MIDI control**: Chord type, spread, engine selection via CC
- **Preset system**: Save/recall complete chord+engine configurations
- **Pattern integration**: Chord sequences with engine evolution

This enhanced chord engine would be a flagship feature - no other synthesizer offers this level of per-voice engine flexibility within a single chord generator!