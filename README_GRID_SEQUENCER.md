# EtherSynth Advanced Grid Sequencer

A powerful 16-step grid sequencer with advanced performance effects, designed for live electronic music production with OSC grid controllers.

## Quick Start

### Building and Running
```bash
# Build the advanced grid sequencer
make -f Makefile.advanced grid_advanced

# Run the sequencer
./grid_sequencer_advanced
```

### Hardware Requirements
- **OSC Grid Controller** (16x8 pads minimum)
- **Optional**: Rotary encoders for parameter control
- **Audio Interface** recommended for low-latency performance

## Core Concepts

### Engine System
- **16 instruments** with different synthesis engines per row
- **Engine types**: MacroVA, MacroFM, Drums, Granular, Wavetables, and more
- Each row can have a different engine type for complex arrangements

### Pattern System
- **16-step patterns** with 4 pages (64 steps total)
- **Per-step velocity** and accent control
- **Pattern banking** for song arrangement
- **Live pattern switching** during performance

---

## Basic Operation

### 1. Playback Control
- **Spacebar**: Play/Stop
- **W**: Write mode (record while playing)
- **C**: Clear current pattern
- **Q**: Quit application

### 2. Step Programming
**Normal Mode:**
- Press grid pads to toggle steps on/off
- Velocity set by how hard you press (if controller supports it)

**Write Mode (W key):**
- Steps are recorded in real-time at the playhead position
- Perfect for recording live drum fills or melody lines

### 3. Navigation
- **↑/↓ arrows**: Select parameters
- **←/→ arrows**: Adjust parameter values
- **J/K**: Vim-style up/down navigation
- **Page Up/Down**: Switch between pattern pages (16-step sections)

---

## Engine Management

### Selecting Engines
1. Navigate to any row (engine)
2. Use parameter controls to adjust synthesis parameters
3. Each row shows its engine type and current settings

### Engine Parameters
Each engine has different parameters:
- **MacroVA**: Classic subtractive synthesis
- **MacroFM**: 4-operator FM synthesis with algorithm selection
- **Drums**: 16-pad drum kit with tune/decay/level/pan per pad
- **Granular**: Texture synthesis and audio manipulation
- **And many more...**

---

# Performance Effects System

## 1. Accent Mode
**Activation**: Shift + Pad #15
**Function**: Makes selected steps trigger with increased velocity

**Usage:**
1. Enable Accent mode (Shift + Pad #15)
2. Press steps to mark them as accented
3. Accented steps will play ~20% louder

---

## 2. Enhanced Retrigger System
**Activation**: Shift + Pad #11
**Settings Menu**: Press 'T' key

### Basic Retrigger
Creates rapid-fire repetitions of triggered steps with pitch/velocity variations.

### Settings (Press 'T' to open menu):

1. **Triggers** (1-8): Number of additional triggers per step
2. **Step Window** (1-4): How many steps the retrigger spans
3. **Octave Shift** (-2 to +2): Pitch change across triggers
4. **Timing Mode**:
   - **Static**: Even spacing (classic retrigger)
   - **Accelerating**: Starts slow, speeds up (rolls)
   - **Decelerating**: Starts fast, slows down (fills)
   - **Exponential/Logarithmic**: Dramatic curves
5. **Velocity Mode**:
   - **Static**: Even volume fade
   - **Crescendo**: Gets louder (drum rolls)
   - **Diminuendo**: Gets quieter (decay fills)
   - **Accent First/Last**: Emphasize first or last hit
6. **Curve Intensity** (0.0-1.0): Controls how dramatic the timing/velocity curves are

### Drum-Aware Retrigger
When used with drum engines:
- Pitch shifts the same drum element (doesn't change drum pads)
- Uses the drum's tune parameter for pitch control
- Perfect for snare rolls, kick drum fills, etc.

---

## 3. Arpeggiator System
**Activation**: Shift + Pad #10
**Settings Menu**: Press 'A' key

### Arpeggiator Modes
Turns single triggers into melodic sequences.

### Settings (Press 'A' to open menu):

1. **Pattern**:
   - **Up**: C-E-G-C ascending
   - **Down**: C-G-E-C descending
   - **Up-Down**: C-E-G-E-C-E-G-E
   - **Random**: Randomized note order
   - **As-Played**: Uses the order you pressed keys
   - **Chord**: All notes simultaneously

2. **Length** (1-8): Number of notes in the arpeggio
3. **Cycles** (1-16 or infinite): How many times the pattern repeats
4. **Octaves** (1-4): Octave range for the arpeggio
5. **Speed**: 1/16, 1/8, 1/4, 1/2, Whole note timing
6. **Gate** (25-100%): Note length as percentage of timing

### Drum-Aware Arpeggiator
When used with drum engines:
- Creates tuned variations of the same drum sound
- Uses drum tune parameter to create pitched drum sequences
- Great for pitched tom fills, tuned percussion, etc.

---

## 4. Pattern Gate Effect
**Activation**: Shift + Pad #9
**Settings Menu**: Press 'G' key

### What It Does
Creates rhythmic gating/stuttering patterns with probabilistic variations.

### Settings (Press 'G' to open menu):

1. **Mode**:
   - **Deterministic**: Fixed on/off pattern
   - **Probabilistic**: Each step has % chance to trigger
   - **Hybrid**: Pattern modified by probability

2. **Pattern**: 16-step on/off pattern
   - Cycles through: Alternating, Quarter notes, Triplets, etc.
   - Shows as: ■□■□■□■□ (■=on, □=off)

3. **Probability** (0-100%): Chance each step triggers
4. **Resolution**: 1/16, 1/8, 1/4, 1/2, Whole note subdivisions
5. **Swing** (-0.5 to +0.5): Timing humanization

### Pattern Gate Examples
- **Trance Gate**: Deterministic mode with 1/16 resolution
- **Glitch Stutter**: Probabilistic mode at 60% with 1/32 resolution
- **Evolving Patterns**: Hybrid mode combines fixed patterns with random variations

---

# Advanced Features

## LFO System
**LFO Menu**: Press 'L' key
**LFO Settings**: Press 'S' key

### LFO Assignment
1. Open LFO menu (L key)
2. Select parameter with ↑/↓
3. Press Shift + Pad 1-8 to assign LFOs
4. Adjust LFO rate (R/r keys), depth (D/d keys), waveform (V key)

### Global LFOs
- **8 independent LFOs** available
- Can modulate any synthesis parameter
- **12 waveform types**: sine, saw, square, triangle, and more
- **Rate range**: 0.01-50 Hz

## Octave Control
**For Melodic Engines Only:**
- **Shift + Pad #12**: Increase octave (+1 to +4)
- **Shift + Pad #16**: Decrease octave (-1 to -4)
- **Current octave** shown in status: "OCT:+2"

## Pattern Management

### Pattern Pages
- **4 pages** of 16 steps each = 64 total steps
- **Shift + Pad #13/14**: Navigate between pages
- **Current page** shown in status
- Each page can have different patterns per engine

### Pattern Banking
- **Pattern slots 1-16** for song arrangement
- Press pads 1-16 (without Shift) to switch patterns
- Patterns automatically saved when switching

### Copy/Paste System
**Activation**: Hold any pad + press another pad
1. **Copy source**: Hold the pad you want to copy from
2. **Paste target**: While holding, press destination pad
3. **Copy mode active**: Shows "COPY" in status
4. Copies step data including velocity and effects

---

# Effect Combinations

## Effect Priority
Effects are **mutually exclusive** - only one can be active per step:
1. **Retrigger** takes priority over Arpeggiator
2. **Pattern Gate** affects all triggered notes
3. **Accent** can combine with any effect

## Typical Workflows

### Building Energy (Drum Fill)
1. Enable Retrigger (Shift + Pad #11)
2. Set to "Crescendo" velocity mode
3. Set 6-8 triggers with +1 octave shift
4. Trigger on snare steps for classic drum fill

### Melodic Arpeggios
1. Enable Arpeggiator (Shift + Pad #10)
2. Set "Up" pattern with 4 notes
3. Set speed to 1/16 for fast arpeggios
4. Use with chord progressions

### Rhythmic Gating
1. Enable Pattern Gate (Shift + Pad #9)
2. Set "Deterministic" mode
3. Choose alternating pattern (■□■□■□■□)
4. Perfect for trance-style gating

### Evolving Textures
1. Combine Pattern Gate "Hybrid" mode
2. Set 70% probability with triplet pattern
3. Add LFO modulation to filter parameters
4. Creates ever-changing rhythmic textures

---

# Tips and Tricks

## Performance Tips
- **Write mode** is perfect for recording live fills and variations
- **Pattern banking** lets you pre-arrange song sections
- **Effect combinations** create complex rhythmic variations
- **Copy/paste** quickly duplicates successful patterns

## Sound Design Tips
- **Drum engines** respond well to retrigger pitch effects
- **FM engines** create interesting results with LFO modulation
- **Granular engines** work great with Pattern Gate stuttering
- **Layer multiple engines** for rich, complex textures

## Workflow Suggestions
1. **Start simple**: Program basic patterns first
2. **Add effects gradually**: One effect at a time
3. **Use pattern pages**: Build 64-step song sections
4. **Save variations**: Use pattern banking for different song parts
5. **Practice live**: Effects are designed for real-time performance

---

# Keyboard Reference

## Main Controls
- **Space**: Play/Stop
- **W**: Write mode toggle
- **C**: Clear pattern
- **Q**: Quit

## Settings Menus
- **T**: Retrigger settings
- **A**: Arpeggiator settings
- **G**: Pattern Gate settings
- **L**: LFO assignment menu
- **S**: LFO parameter settings

## Navigation
- **↑/↓** or **J/K**: Parameter selection
- **←/→**: Parameter adjustment
- **Page Up/Down**: Pattern page navigation

## LFO Controls
- **[/]**: Select LFO (1-8)
- **V**: Cycle waveform
- **R/r**: Increase/decrease rate
- **D/d**: Increase/decrease depth

---

# Grid Controller Reference

## Shift + Pad Functions
| Pad # | Function | Description |
|-------|----------|-------------|
| 1-8 | LFO Assign | Assign LFO to selected parameter |
| 9 | Pattern Gate | Toggle Pattern Gate effect |
| 10 | Arpeggiator | Toggle Arpeggiator effect |
| 11 | Retrigger | Toggle Retrigger effect |
| 12 | Octave Up | Increase octave (+1 to +4) |
| 13 | Page Left | Previous pattern page |
| 14 | Page Right | Next pattern page |
| 15 | Accent Mode | Toggle accent marking |
| 16 | Octave Down | Decrease octave (-1 to -4) |

## Status Display
The status line shows active modes:
```
Modes: OCT:+1 ACCENT RETRIG ARP GATE COPY
```

---

# Technical Notes

## Audio Latency
- Uses **PortAudio** for cross-platform audio
- **Recommended**: Use ASIO drivers on Windows, CoreAudio on macOS
- **Buffer sizes**: 64-256 samples for live performance

## Grid Controller Setup
- **OSC protocol** on port 8080 (default)
- Compatible with **monome grid**, **Launchpad**, and custom controllers
- **LED feedback** shows current pattern state

## Engine Integration
- **15+ synthesis engines** with unique parameter sets
- **Real-time parameter control** via grid and encoders
- **Engine-aware effects** adapt to synthesis type

---

# Troubleshooting

## Common Issues

**No Audio Output:**
1. Check audio interface connections
2. Verify PortAudio device selection
3. Try different buffer sizes

**Grid Controller Not Responding:**
1. Check OSC port configuration (default 8080)
2. Verify network connection (if using WiFi grid)
3. Test with OSC monitor applications

**Effects Not Working:**
1. Ensure only one effect is active per step
2. Check effect settings menus (T/A/G keys)
3. Verify steps are actually triggered

**Performance Issues:**
1. Increase audio buffer size
2. Reduce number of active LFOs
3. Optimize synthesis engine settings

## Getting Help
- Check console output for error messages
- Use 'Q' to quit safely and preserve patterns
- Effects can be disabled individually if causing issues

---

*Built with EtherSynth - Advanced modular synthesis for live performance*