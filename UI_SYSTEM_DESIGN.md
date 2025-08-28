# 🎹 ether UI System Design

*"Song-focused workflow with flexible instrument slots and intelligent physical controls"*

## 🎯 Core Architecture

### Mode Structure
1. **SONG MODE** (Primary) - Timeline-based composition and arrangement
2. **TAPE MODE** (Alternative) - TP-7 style linear audio manipulation  
3. **INSTRUMENT MODE** (Quick Edit) - Touch slot → Edit → Back to song

### Control Philosophy
- **Touch = Navigate & Select** (instruments, parameters, timeline)
- **Physical = Control & Perform** (smart knob, encoders, keyboard)
- **Song Context = Always Maintained** (never lose musical thread)

## 🎵 Song Mode (Primary Interface)

### Smart Knob Behavior
```
DEFAULT: Pattern/Section Selection
🎛️ ┌─●─┐ Pattern A    B    C    D
   Strong magnetic detents for each pattern

TOUCH TIMELINE: Scrub Through Song  
🎛️ ┌────●────┐ Bar 1          Bar 32
   Smooth scrubbing with audio feedback

TOUCH BPM: Global Tempo Control
🎛️ ┌──●──┐ 60 BPM              200 BPM
   Detents every 5 BPM for precise setting

PLAYBACK: Live scrubbing with haptic downbeat pulse
```

### Timeline Interface
```
BAR: 1    5    9    13   17   21   25   29
┌────┬────┬────┬────┬────┬────┬────┬────┐
│A   │A   │B   │B   │C   │B   │D   │A   │
│    │    │    │●   │    │    │    │    │
└────┴────┴────┴────┴────┴────┴────┴────┘
                ↑ Smart knob controls this section

OPERATIONS:
• Touch section: Select for editing/playback
• Long press: Copy/paste menu
• Drag: Move sections in timeline
• Smart knob: Navigate between sections
```

### 8 Flexible Instrument Slots
```
EMPTY BY DEFAULT:
🔴 [EMPTY]  🟠 [EMPTY]  🟡 [EMPTY]  🟢 [EMPTY]
🔵 [EMPTY]  🟣 [EMPTY]  🟪 [EMPTY]  ⚫ [EMPTY]

USER POPULATES WITH ANY INSTRUMENTS:
• Could be: 4 basses, 2 drums, 1 lead, 1 FX
• Or: 2 basses, 2 pads, 2 leads, 1 drum, 1 chord
• Colors = visual organization only (not functional roles)

WORKFLOW: Touch empty slot → Instrument browser → Select
```

## 📼 Tape Mode (TP-7 Style Alternative)

### Smart Knob as Jog Wheel
```
TAPE INTERFACE:
┌──────────────────────────────────────────────────┐
│ ▐▌ ▐ ▌▌    ▐▐ ▌ ▐▌▐     ▌▐  ▌▌▐ ▌  ▐▌▐▌ ▐▌▐ ▐ │
│                    ↑ Playhead (jog controlled)    │
└──────────────────────────────────────────────────┘

🎛️ SMART KNOB JOG WHEEL:
• Audio scrubbing with tape-style sound effects
• Precise positioning for cut/splice operations  
• Variable speed playback (half/double speed)
• Loop start/end point setting with haptic feedback

4 TRACK RECORDING:
Track 1: [████████████████░░░░░░░░] Recording
Track 2: [████████░░░░░░░░░░░░░░░░] Ready
Track 3: [░░░░░░░░░░░░░░░░░░░░░░░░░] Empty
Track 4: [░░░░░░░░░░░░░░░░░░░░░░░░░] Empty

OPERATIONS: [RECORD] [PLAY] [CUT] [SPLICE] [LIFT] [DROP]
```

## 🎛️ Instrument Mode (Quick Edit)

### No "Primary" Parameter
```
🔴 RED SLOT: [TB-303 Bass] ← User-assigned instrument

🎛️ SMART KNOB: Contextual to touched parameter
• Touch Filter → Knob controls filter with appropriate haptics
• Touch Engine → Knob selects synthesis engine type
• Touch Envelope → Knob controls envelope timing

PARAMETER ACCESS:
┌─────┬─────┬─────┬─────┬─────┬─────┐
│ENGINE│FILT │RESO │ATTCK│DECAY│RELESE│
│Subtr│800Hz│ 65% │25ms │200ms│600ms│
│  ○  │  ●  │  ○  │  ○  │  ○  │  ○  │ ← Filter selected
└─────┴─────┴─────┴─────┴─────┴─────┘

🎛️ Smart Knob: Filter Cutoff (800Hz)
┌───●───┐ 20Hz              20kHz
Smooth rotation with octave detents

[BACK TO SONG] ← Always available one-touch return
```

### Engine Selection
```
AVAILABLE ENGINES:
[SUBTRACTIVE] [FM] [SAMPLER] [GRANULAR] [CHORD_GEN]
      ●        ○      ○         ○          ○

• Touch engine type → Smart knob selects with strong detents
• Each engine brings its own parameter set
• Instrument can combine multiple engines
• Settings persist per instrument slot
```

## 🎚️ Encoder Latching System

### Persistent Parameter Control
```
ASSIGNMENT WORKFLOW:
1. Touch parameter on screen (any mode)
2. Double-tap any encoder to latch permanently  
3. Encoder LED shows instrument color + intensity
4. OLED shows parameter name + current value

ENCODER STATUS DISPLAY:
ENC1     ENC2     ENC3     ENC4
┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐
│FILT │ │VERB │ │ BPM │ │ --- │
│🔴●● │ │🔵●○ │ │⚫●● │ │ ○○○ │
└─────┘ └─────┘ └─────┘ └─────┘

ENC1: Filter latched to RED instrument (full intensity)
ENC2: Reverb latched to BLUE instrument (50% depth)  
ENC3: Global BPM control (not instrument-specific)
ENC4: Available for assignment

GLOBAL PERSISTENCE: Latched assignments work across all modes
```

## 🔄 Flexible Modulation System

### 8 Configurable Sources
```
DEFAULT ALLOCATION:
MOD 1: [LFO] - Unassigned
MOD 2: [LFO] - Unassigned
MOD 3: [LFO] - Unassigned  
MOD 4: [LFO] - Unassigned
MOD 5: [ENVELOPE] - Unassigned
MOD 6: [ENVELOPE] - Unassigned
MOD 7: [LFO] - Unassigned
MOD 8: [LFO] - Unassigned

USER CAN RECONFIGURE:
• MOD 1: [LFO] → RED instrument Filter (Global)
• MOD 2: [ENVELOPE] → RED instrument Amp (Note-triggered)
• MOD 3: [LFO] → BLUE instrument Pitch (Global)
• MOD 4: [ENVELOPE] → BLUE instrument Filter (Note-triggered)
• etc.

ASSIGNMENT: Touch parameter → Long press → Select MOD source
```

### LFO vs Envelope Behavior
```
LFO (Global):
• Runs continuously independent of notes
• Same modulation affects all voices of instrument
• Good for: Filter sweeps, tremolo, vibrato

ENVELOPE (Note-triggered):
• Triggered by each note-on event
• Per-voice modulation (polyphonic)
• Good for: Pluck attacks, filter snaps, volume pumping

CONVERSION: Any MOD source can switch LFO ↔ ENVELOPE
```

## 🎹 Physical Integration

### 26-Key Keyboard
```
SONG MODE: Pattern programming and chord input
• Play notes → Record to current pattern/section
• Chord detection for harmonic progression
• Step programming with visual feedback

TAPE MODE: Triggering sounds while recording
• Real-time playing recorded to audio tracks
• No MIDI - direct audio capture

INSTRUMENT MODE: Auditioning while editing
• Hear parameter changes in real-time
• Test synthesis engine variations
```

### Transport Controls
```
PHYSICAL BUTTONS:
⏯️ PLAY: Start from selected timeline section
⏺️ RECORD: Capture to selected section/track  
⏹️ STOP: Stop playback/recording

INTEGRATION WITH MODES:
• SONG MODE: Controls pattern/section playback
• TAPE MODE: Controls linear audio recording
• INSTRUMENT MODE: Solo instrument for editing
```

## 📱 Touch Screen Navigation

### Touch Gestures
```
SINGLE TOUCH:
• Tap instrument slot: Select/edit
• Tap timeline section: Select for playback
• Tap parameter: Smart knob takes control
• Tap transport area: Mode switching

LONG PRESS:
• Instrument: Solo/mute/copy options
• Timeline: Copy/paste/delete menu
• Parameter: Modulation assignment

DRAG OPERATIONS:
• Timeline sections: Reorder/move
• Instrument volume: Quick level adjust
• Waveform display: Audio scrubbing

MULTI-TOUCH:
• Pinch timeline: Zoom in/out
• Two-finger tap: Undo
```

### Visual Feedback
```
SELECTION STATES:
• Cyan border: Currently selected for smart knob control
• Instrument color: Shows which slot is active
• White outline: Parameter being touched
• Pulsing: Currently playing/recording

REAL-TIME DISPLAY:
• Waveform visualization during playback
• Parameter values update during knob control
• Timeline playhead with smooth animation
• Level meters for all active instruments
```

## 🎯 Design Principles

### Song-Centric Workflow
- **Timeline always visible** - Never lose song context
- **Quick instrument editing** - Touch → Edit → Back to song  
- **Pattern-based composition** - Build songs from sections
- **Copy-modify workflow** - Natural song development

### Flexible Architecture  
- **User-defined instrument slots** - No preset roles
- **Configurable modulation** - LFOs and envelopes as needed
- **Touch navigation + physical control** - Best of both worlds
- **Mode persistence** - Settings maintained across mode switches

### Performance Ready
- **Encoder latching** - Critical parameters always accessible
- **Haptic feedback** - Feel parameter changes without looking
- **Transport integration** - Physical buttons work in context
- **Real-time operation** - No workflow interruption for live use

---

*This UI system balances immediate creativity with professional depth, allowing users to build their own workflow while maintaining song-focused composition throughout.*