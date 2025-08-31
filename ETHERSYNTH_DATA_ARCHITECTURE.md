# EtherSynth Data Architecture Specification
**Version 2.0 - Corrected Entity Relationships**  
**Date: 2025-08-29**

---

## **OVERVIEW**

EtherSynth uses a **layered architecture** with **entity relationships** designed for complex musical sequencing:

```
SwiftUI Frontend (UI Layer)
    ↕ State Binding & User Interaction
Swift SynthController (Logic Layer) 
    ↕ C++ Bridge Functions
C++ Audio Engine (Real-time Synthesis)
```

**Key Architectural Principle**: Steps are **containers** for multiple note events, not direct instrument mappings.

---

## **CORE DATA ENTITIES**

### **1. NOTE_EVENT (Atomic Musical Unit)**

```swift
struct NoteEvent {
    let instrumentSlotIndex: UInt32     // Which instrument (0-15)
    let pitch: UInt8                    // MIDI note (0-127)
    let velocity: UInt8                 // Note velocity (0-127)
    let lengthSteps: UInt8              // Note length (1-8 steps)
    let microTiming: Int8               // -60..+60 tick offset
    let ratchetCount: UInt8             // Sub-hits (1-4)
    let retrigger: RetriggerConfig      // Advanced retrigger settings
    
    // Sparse parameter locks (max 4 per note - memory efficient)
    let overrideCount: UInt8            // How many p-locks (0-4)
    let overrides: [ParameterOverride]  // P-lock values
    
    // FX lock (optional per-note effect override)
    let fxLockEffectId: UInt32?         // Which effect to override
    let fxLockValues: [Float]           // FX parameter overrides
}
```

**Purpose**: Represents a single musical event that can trigger an instrument with specific parameters.

**Key Features**:
- **Sparse Parameter Locks**: Max 4 p-locks per note (covers 99% of use cases)
- **Advanced Retriggers**: Support for "zipper rolls" with pitch ramping
- **Per-Note FX**: Individual effect overrides per note

---

### **2. STEP_SLOT (Step Container)**

```swift
struct StepSlot {
    let stepIndex: UInt8                        // Position in pattern (0-63)
    var noteEvents: [NoteEvent]                 // 0-many note events at this step
    var mixerEvents: [MixerAutomationEvent]     // Drop/solo/mute automation
}
```

**Purpose**: Container for all musical events that happen at a single step position.

**Key Features**:
- **Multiple Notes Per Step**: Can contain 0-many NOTE_EVENTs
- **Cross-Instrument Support**: Same step can trigger multiple instruments
- **Same-Instrument Multi-Notes**: Same instrument can play multiple pitches
- **Mixer Automation**: Supports Drop feature (solo/mute automation)

**Examples**:
```swift
// Empty step
stepSlot.noteEvents = []

// Single kick drum
stepSlot.noteEvents = [NoteEvent(instrument: 0, pitch: 36, velocity: 127)]

// Chord (multiple instruments)  
stepSlot.noteEvents = [
    NoteEvent(instrument: 3, pitch: 60, velocity: 100),  // Piano: C
    NoteEvent(instrument: 7, pitch: 64, velocity: 95),   // Bass: E  
    NoteEvent(instrument: 12, pitch: 67, velocity: 90)   // Pad: G
]

// Multi-note same instrument
stepSlot.noteEvents = [
    NoteEvent(instrument: 5, pitch: 60, velocity: 100),  // Lead: C
    NoteEvent(instrument: 5, pitch: 67, velocity: 80)    // Lead: G
]
```

---

### **3. PATTERN_DATA (64-Step Patterns)**

```swift
struct PatternData {
    let patternId: UInt32               // Unique pattern identifier
    var name: String                    // Pattern name
    var length: UInt8                   // Pattern length in steps (1-64)
    var currentPage: UInt8              // Which 16-step page is visible (0-3)
    
    // 64 step slots (each can contain multiple NOTE_EVENTs)
    var stepSlots: [StepSlot]           // stepSlots[0-63]
    
    // Pattern-level parameters
    var swing: Float                    // 0.0-1.0 swing amount
    var humanize: Float                 // 0.0-1.0 humanization
    var quantizeGrid: UInt8             // Quantization resolution
    var tempoOverride: Float?           // Pattern-specific tempo (nil = use global)
    
    // Pattern-level effects (applied to entire pattern output)
    var patternEffectIds: [UInt32]      // Up to 4 pattern-wide effects
    var patternFxActive: [Bool]         // Which pattern effects are enabled
    
    // Pattern chaining
    var nextPatternId: UInt32?          // For pattern chaining (nil = no chain)
    var chainMode: ChainMode            // How to handle chaining
}
```

**Purpose**: Container for 64 step slots, representing a complete musical pattern.

**Key Features**:
- **Scalable**: Can support 1024+ patterns per project
- **Flexible Length**: Patterns can be 1-64 steps long
- **Pattern-Level Effects**: Effects that apply to entire pattern output
- **Pattern Chaining**: Support for creating song structures

---

### **4. CHORD_INSTRUMENT (Special Instrument Type)**

```swift
struct ChordInstrument {
    var linkedInstruments: [UInt32]     // Indices of linked instruments (max 8)
    var chordDefinition: ChordDefinition // Chord type, voicing, etc.
    var velocitySpread: Float           // Velocity variation between notes (0-1)
    var pitchSpread: Int8               // Octave/pitch spread (-12 to +12)
    var enabled: Bool                   // Is chord instrument active
}
```

**Purpose**: Special instrument that references multiple other instruments to create complex harmonies.

**Key Features**:
- **Multi-Instrument Triggering**: Single trigger creates multiple NOTE_EVENTs
- **Chord Theory Integration**: Major, minor, 7ths, inversions, voicings
- **Humanization**: Velocity and pitch spread for natural feel
- **Flexible Linking**: Can reference any combination of existing instruments

**Operation**:
```swift
// When CHORD_INSTRUMENT is triggered:
let chordInstrument = ChordInstrument(
    linkedInstruments: [3, 7, 12],           // Piano, Bass, Pad
    chordDefinition: ChordDefinition(.major7, inversion: 1, voicing: .close)
)

// Single trigger creates multiple NOTE_EVENTs:
let noteEvents = chordInstrument.generateNoteEvents(rootPitch: 60, velocity: 100)
// Results in:
// - NoteEvent(instrument: 3, pitch: 64, velocity: 100)  // Piano: E (1st inversion)
// - NoteEvent(instrument: 7, pitch: 67, velocity: 95)   // Bass: G  
// - NoteEvent(instrument: 12, pitch: 71, velocity: 90)  // Pad: B
// - NoteEvent(instrument: 3, pitch: 72, velocity: 85)   // Piano: C (octave)
```

---

### **5. PLAYBACK_STATE (Dual-Mode System)**

```swift
struct PlaybackState {
    // Playback Mode
    var mode: PlaybackMode              // .songMode vs .patternMode
    
    // Song Mode State  
    var songPosition_ticks: UInt32      // Global song playhead position
    var currentSectionId: UInt32        // Which song section is playing
    var currentPatternInSong: UInt32    // Which pattern the song is on
    
    // Pattern Mode State
    var patternPlayhead_steps: UInt32   // Position within current pattern (0-63)
    var currentPatternId: UInt32        // Which pattern is looping
    var patternLooping: Bool            // Loop the pattern vs play once
    
    // Shared State
    var isPlaying: Bool
    var isRecording: Bool
    var currentBPM: Float
    
    // Per-Pattern Playback Overrides (for current pattern view)
    var instrumentMuted: [Bool]         // Mute states for current pattern
    var instrumentSoloed: [Bool]        // Solo states for current pattern
    var soloCount: Int                  // How many instruments are soloed
    
    // SmartKnob Scrubbing
    var scrubMode: Bool                 // Is user scrubbing timeline?
    var scrubPosition: Float            // Scrub position (0.0-1.0)
}
```

**Purpose**: Manages complex dual-mode playback system.

**Playback Logic**:
```
PLAY Button Pressed:
├─ If mode == .songMode → Resume playing from songPosition_ticks
├─ If mode == .patternMode → Start looping currentPatternId
└─ SmartKnob always scrubs songPosition_ticks (regardless of view mode)

User Switches to Pattern View:
├─ Playback continues in background at songPosition_ticks  
├─ UI shows pattern view with per-pattern mute/solo controls
└─ Pattern playhead syncs to (songPosition % pattern_length)

User Switches to Song View:
├─ Playback continues at songPosition_ticks
└─ UI shows full song timeline
```

---

## **PARAMETER LOCK SYSTEM**

### **Parameter Override Structure**

```swift
struct ParameterOverride {
    let parameterIndex: UInt8           // Which parameter (0-15)
    let value: Float                    // Override value
}
```

### **Inheritance Hierarchy (Priority Order)**

```
1. NOTE_EVENT.overrides[X]              ← HIGHEST (per-note p-lock)
2. PATTERN.automation[X]                ← Pattern-level automation  
3. INSTRUMENT_SLOT.parameters[X]        ← Base instrument value
```

### **Duration Behavior**

- **Full Note Duration**: P-locks affect the entire length of the note
- **Real-Time Resolution**: P-locks resolved during playback (flexible, slight CPU cost)
- **Max 4 P-Locks Per Note**: Covers 99% of musical use cases (filter + volume + pitch + one effect)

### **Example P-Lock Usage**

```swift
// Create a note with filter cutoff and volume p-locks
let noteWithPLocks = NoteEvent(
    instrument: 5, 
    pitch: 60, 
    velocity: 100,
    overrides: [
        ParameterOverride(parameterIndex: 8, value: 0.3),   // Filter cutoff low
        ParameterOverride(parameterIndex: 0, value: 0.8)    // Volume reduced
    ]
)

// During playback, this note will:
// 1. Trigger instrument 5 with pitch 60
// 2. Override filter cutoff to 0.3 for the note duration
// 3. Override volume to 0.8 for the note duration
// 4. All other parameters use instrument default values
```

---

## **ENTITY RELATIONSHIPS**

### **Hierarchical Containment**
```
PROJECT
├─ PATTERNS[0-1023] 
│   └─ STEP_SLOTS[0-63]
│       └─ NOTE_EVENTS[0-many]
│           └─ PARAMETER_OVERRIDES[0-4]
├─ INSTRUMENT_SLOTS[0-15]
│   ├─ SYNTHESIS_INSTRUMENT (normal)
│   └─ CHORD_INSTRUMENT (special - references other instruments)
└─ PLAYBACK_STATE (singleton)
```

### **Cross-References**
```
NOTE_EVENT ──targets──> INSTRUMENT_SLOT
CHORD_INSTRUMENT ──references──> [INSTRUMENT_SLOTS]
PATTERN ──chains_to──> NEXT_PATTERN  
PLAYBACK_STATE ──tracks──> CURRENT_PATTERN + SONG_POSITION
```

### **Many-to-Many Relationships**
```
STEP_SLOTS ↔ NOTE_EVENTS (step can contain multiple notes)
CHORD_INSTRUMENTS ↔ INSTRUMENT_SLOTS (chord can reference multiple instruments)
PATTERNS ↔ SONG_SECTIONS (pattern can appear in multiple sections)
```

---

## **MUSICAL WORKFLOW EXAMPLES**

### **Example 1: Simple Drum Beat**
```swift
// Step 0: Kick drum
pattern.stepSlots[0].addNote(NoteEvent(instrument: 0, pitch: 36, velocity: 127))

// Step 4: Snare drum  
pattern.stepSlots[4].addNote(NoteEvent(instrument: 1, pitch: 38, velocity: 100))

// Step 8: Kick drum
pattern.stepSlots[8].addNote(NoteEvent(instrument: 0, pitch: 36, velocity: 127))

// Step 12: Snare drum
pattern.stepSlots[12].addNote(NoteEvent(instrument: 1, pitch: 38, velocity: 100))
```

### **Example 2: Complex Chord Progression**
```swift
// Step 0: Em7 chord using chord instrument
let em7Chord = chordInstrument.generateNoteEvents(rootPitch: 64, velocity: 100) // E
pattern.stepSlots[0].noteEvents = em7Chord

// Step 8: Am7 chord
let am7Chord = chordInstrument.generateNoteEvents(rootPitch: 57, velocity: 100) // A
pattern.stepSlots[8].noteEvents = am7Chord

// Results in multiple instruments playing simultaneously per step
```

### **Example 3: Parameter-Locked Lead Line**
```swift
// Step 0: High filter cutoff
let brightNote = NoteEvent(
    instrument: 5, pitch: 72, velocity: 120,
    overrides: [ParameterOverride(parameterIndex: 8, value: 0.9)] // Bright filter
)
pattern.stepSlots[0].addNote(brightNote)

// Step 2: Low filter cutoff  
let darkNote = NoteEvent(
    instrument: 5, pitch: 75, velocity: 100,
    overrides: [ParameterOverride(parameterIndex: 8, value: 0.2)] // Dark filter
)
pattern.stepSlots[2].addNote(darkNote)

// Same instrument, different filter settings per note
```

---

## **SEQUENCER PROCESSING LOGIC**

### **Per-Step Processing**
```swift
func processStep(_ stepIndex: UInt8, pattern: PatternData, playbackState: PlaybackState) {
    let stepSlot = pattern.stepSlots[Int(stepIndex)]
    
    // Process all NOTE_EVENTs in this step
    for noteEvent in stepSlot.noteEvents {
        // Check per-pattern mute/solo
        if playbackState.instrumentMuted[Int(noteEvent.instrumentSlotIndex)] { continue }
        if playbackState.soloCount > 0 && !playbackState.instrumentSoloed[Int(noteEvent.instrumentSlotIndex)] { continue }
        
        // Apply parameter locks (p-lock > instrument parameter)
        applyParameterOverrides(noteEvent: noteEvent)
        
        // Trigger the note with C++ bridge
        triggerNoteEvent(noteEvent)
    }
    
    // Process mixer automation events
    for mixerEvent in stepSlot.mixerEvents {
        applyMixerAutomation(mixerEvent)
    }
}
```

### **Parameter Resolution**
```swift
func resolveParameter(_ instrumentIndex: UInt32, _ paramIndex: UInt8, noteEvent: NoteEvent?) -> Float {
    // 1. Check for p-lock override (highest priority)
    if let noteEvent = noteEvent {
        for override in noteEvent.overrides {
            if override.parameterIndex == paramIndex {
                return override.value
            }
        }
    }
    
    // 2. Fall back to instrument default
    return instrumentSlots[Int(instrumentIndex)].parameters[Int(paramIndex)]
}
```

---

## **IMPLEMENTATION STATUS**

### ✅ **Completed**
- [x] NOTE_EVENT data structure with sparse p-locks
- [x] STEP_SLOT container system  
- [x] PATTERN_DATA with 64-step architecture
- [x] CHORD_INSTRUMENT special instrument type
- [x] PLAYBACK_STATE dual-mode system
- [x] Complete Swift data model implementation

### 🚧 **In Progress**
- [ ] Parameter lock resolution system
- [ ] C++ bridge integration for new data model
- [ ] Sequencer timing and playback logic

### ⏳ **Pending**
- [ ] Pattern storage/persistence 
- [ ] UI integration with new data model
- [ ] Advanced features (LFO system, effects chain)
- [ ] Song arrangement system

---

## **ARCHITECTURAL BENEFITS**

1. **Flexibility**: Steps can contain any number of notes for any instruments
2. **Memory Efficiency**: Sparse parameter locks minimize memory usage
3. **Musical Complexity**: Support for chords, multi-instrument arrangements, per-note automation
4. **Performance**: Real-time parameter resolution with minimal CPU overhead
5. **Scalability**: Architecture supports complex compositions with 1024+ patterns
6. **User Experience**: Dual-mode playback allows seamless workflow between pattern and song editing

---

**This document serves as the definitive reference for EtherSynth's data architecture and should be consulted frequently during implementation.**