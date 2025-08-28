# EtherSynth User Manual

## Table of Contents

1. [Getting Started](#getting-started)
2. [Interface Overview](#interface-overview)
3. [Synthesis Engines](#synthesis-engines)
4. [Timeline and Sequencing](#timeline-and-sequencing)
5. [Mix Console](#mix-console)
6. [Modulation System](#modulation-system)
7. [Sample Management](#sample-management)
8. [Effects and Processing](#effects-and-processing)
9. [Project Management](#project-management)
10. [Performance and Optimization](#performance-and-optimization)
11. [Troubleshooting](#troubleshooting)
12. [Keyboard Shortcuts](#keyboard-shortcuts)

## Getting Started

### Installation and Setup

1. **System Requirements**
   - macOS Monterey (12.0) or later
   - Apple Silicon (M1/M2) or Intel Mac
   - 8GB RAM minimum (16GB recommended)
   - 2GB free disk space

2. **First Launch**
   - Open EtherSynth from Applications folder
   - Configure audio device in Preferences
   - Set appropriate buffer size (128 samples recommended)
   - Enable haptic feedback if desired

3. **Audio Setup**
   - Go to Preferences → Audio
   - Select your audio interface or use Built-in Output
   - Choose sample rate (44.1kHz or 48kHz recommended)
   - Set buffer size based on your needs:
     - 64 samples: Lowest latency, higher CPU usage
     - 128 samples: Balanced performance (recommended)
     - 256+ samples: Lower CPU usage, higher latency

### Creating Your First Project

1. **New Project**: File → New Project (⌘N)
2. **Choose a Template**: Select "Empty 8-Track" for a blank slate
3. **Save Your Project**: File → Save (⌘S) - Choose location and name

## Interface Overview

### Main Views

EtherSynth has four primary views accessible via the top navigation bar:

#### Timeline View
- **Purpose**: Sequencing and arrangement
- **Key Features**: 8-track timeline, pattern editing, transport controls
- **Use Case**: Creating song arrangements and sequences

#### Mix View  
- **Purpose**: Mixing and effects processing
- **Key Features**: Channel strips, send buses, master section
- **Use Case**: Balancing levels and adding effects

#### Mod View
- **Purpose**: Modulation and automation
- **Key Features**: LFOs, envelopes, modulation matrix
- **Use Case**: Creating dynamic, evolving sounds

#### Browser View
- **Purpose**: Engine and sample management
- **Key Features**: Engine browser, sample library, presets
- **Use Case**: Selecting instruments and loading samples

### Transport Controls

Located at the bottom of the Timeline view:

- **Play/Pause**: Start or pause playback
- **Stop**: Stop playback and return to start
- **Record**: Enable recording mode
- **Loop**: Toggle loop mode on/off
- **BPM**: Set project tempo (60-200 BPM)

## Synthesis Engines

EtherSynth includes 14 professional synthesis engines organized into categories:

### Synthesizers (Blue)

#### MacroVA - Virtual Analog
**Best for**: Classic synth sounds, leads, pads, basses
- **HARMONICS**: Filter cutoff and resonance
- **TIMBRE**: Oscillator waveform and pulse width
- **MORPH**: Sub oscillator and noise level

*Quick Start*: Turn up HARMONICS for brightness, adjust TIMBRE for character

#### MacroFM - FM Synthesis  
**Best for**: Bell tones, aggressive basses, metallic sounds
- **HARMONICS**: FM amount and carrier frequency
- **TIMBRE**: Modulator ratio and waveform
- **MORPH**: Feedback and algorithm selection

*Quick Start*: Start with MORPH at 0, slowly increase HARMONICS for FM effect

#### MacroWaveshaper - Waveshaping
**Best for**: Aggressive leads, distorted basses, harsh textures
- **HARMONICS**: Waveshaping amount and asymmetry
- **TIMBRE**: Pre-gain and wave selection
- **MORPH**: Post-filter and saturation

*Quick Start*: Increase HARMONICS gradually for waveshaping distortion

#### MacroWavetable - Wavetable Synthesis
**Best for**: Evolving pads, modern leads, textural sounds
- **HARMONICS**: Wavetable position and scanning
- **TIMBRE**: Formant shifting and spectral tilt
- **MORPH**: Wave morphing and interpolation

*Quick Start*: Slowly move HARMONICS to scan through wavetable

#### MacroChord - Multi-Voice
**Best for**: Chord progressions, complex harmonies, ensemble sounds
- **HARMONICS**: Voice spread and detuning
- **TIMBRE**: Individual voice timbres
- **MORPH**: Voice activation and blend

*Quick Start*: Play chords, adjust HARMONICS for voice spread

#### MacroHarmonics - Additive
**Best for**: Organ sounds, harmonic series, pure tones
- **HARMONICS**: Harmonic series amplitude
- **TIMBRE**: Even/odd harmonic balance
- **MORPH**: Harmonic evolution and decay

*Quick Start*: Start simple, use HARMONICS to add upper harmonics

### Textures (Purple)

#### Formant - Vocal Synthesis
**Best for**: Vocal textures, formant filtering, speech synthesis
- **HARMONICS**: Formant frequencies and Q
- **TIMBRE**: Vowel morphing (A-E-I-O-U)
- **MORPH**: Consonant addition and breath

*Quick Start*: Use TIMBRE to morph between vowel sounds

#### Noise - Granular/Particle
**Best for**: Atmospheric textures, percussion, sound design
- **HARMONICS**: Grain density and size
- **TIMBRE**: Grain pitch scatter and time
- **MORPH**: Source material and randomness

*Quick Start*: Start with low HARMONICS, increase for density

### Physical Models (Orange)

#### TidesOsc - Slope Oscillator
**Best for**: Percussive sounds, evolving textures, rhythm
- **HARMONICS**: Slope steepness and curve
- **TIMBRE**: Frequency ratio and tracking
- **MORPH**: Output mode and waveshaping

*Quick Start*: Try different MORPH settings for various output modes

#### RingsVoice - Modal Resonator  
**Best for**: Metallic sounds, bells, percussion, plucked strings
- **HARMONICS**: Resonator frequency and Q
- **TIMBRE**: Material model (metal/wood/membrane)
- **MORPH**: Excitation type and damping

*Quick Start*: Strike-like attacks work best, adjust TIMBRE for material

#### ElementsVoice - Physical Modeling
**Best for**: Bowed strings, blown pipes, mallet instruments
- **HARMONICS**: Excitation parameters
- **TIMBRE**: Resonator characteristics  
- **MORPH**: Excitation/resonator balance

*Quick Start*: Continuous input works well, explore MORPH for character

### Percussion & Samples (Red)

#### DrumKit - Drum Machine
**Best for**: Drum patterns, percussion, rhythmic elements
- **LEVEL**: Overall kit volume
- **TUNE**: Global tuning adjustment
- **SNAP**: Transient emphasis and punch

*Usage*: Assign different drum sounds to the 12 slots, create patterns

#### SamplerKit - MPC-Style Sampler
**Best for**: One-shot samples, drums, sound effects
- **LEVEL**: Sample playback level
- **PITCH**: Sample pitch adjustment
- **FILTER**: Low-pass filter cutoff

*Usage*: Load samples to 25 pads, trigger with MIDI or mouse

#### SamplerSlicer - Loop Slicer
**Best for**: Breaking apart loops, rhythmic chopping, rearrangement
- **LEVEL**: Slice playback level
- **PITCH**: Global slice pitch
- **FILTER**: Slice filtering

*Usage*: Load loops, create slices, trigger individual parts

## Timeline and Sequencing

### Track Management

#### Creating Patterns
1. Select a track by clicking its number
2. Click "+" to add a pattern
3. Choose pattern type:
   - **Notes**: For melodic content
   - **Drums**: For percussion
   - **Automation**: For parameter changes

#### Engine Assignment
1. Click the engine assignment area on a track
2. Browse available engines by category
3. Click an engine to assign it
4. Use the sound overlay to adjust parameters

#### Pattern Editing
- **Double-click** a pattern to edit
- **Euclidean patterns**: For pitched instruments, set hits/steps/rotation
- **Drum patterns**: 16-step sequencer with velocity and probability
- **Note patterns**: Piano roll-style editing

### Transport and Timing

#### Loop Points
- Drag the loop start/end markers in the ruler
- Enable loop mode with the loop button
- Useful for working on specific sections

#### Grid Snapping
- Toggle with the grid button
- Choose resolution: 1/4, 1/8, 1/16, 1/32 notes
- Helps align patterns to musical timing

#### Tempo Changes
- Adjust BPM with the tempo control
- Tap tempo: Click multiple times on beat
- Range: 60-200 BPM

## Mix Console

### Channel Strips

Each of the 8 tracks has a complete channel strip:

#### Level Controls
- **Volume Fader**: Overall track level
- **Pan Knob**: Stereo positioning
- **Gain**: Input gain adjustment

#### EQ Section
- **High-Pass**: Remove low frequencies (20Hz-1kHz)
- **Low-Pass**: Remove high frequencies (1kHz-20kHz)
- **Use**: Cleaning up sounds and creating space

#### Send Section
- **Send A-D**: Four auxiliary sends
- **Pre/Post**: Send before or after fader
- **Use**: Adding reverb, delay, or parallel processing

#### Insert Effects
- **4 Slots**: Per channel insert effects
- **Bypass**: Disable effect while keeping it loaded
- **Order**: Effects process in series, top to bottom

#### Mute/Solo
- **Mute**: Silence the track
- **Solo**: Hear only soloed tracks
- **Multiple Solo**: Multiple tracks can be soloed

### Master Section

#### Master Controls
- **Master Volume**: Final output level
- **Master Pan**: Rarely used, for special effects
- **Limiter**: Prevents digital clipping

#### Metering
- **Peak**: Shows instantaneous levels
- **RMS**: Shows average levels  
- **Clip Indicators**: Red light shows clipping

### Send Buses

Four auxiliary buses (A, B, C, D) for effects:

1. **Setup**: Load effects on send buses
2. **Send**: Use channel sends to route to buses
3. **Return**: Adjust bus return level
4. **Common Uses**:
   - Bus A: Reverb
   - Bus B: Delay
   - Bus C: Chorus
   - Bus D: Special effects

## Modulation System

### LFOs (Low Frequency Oscillators)

8 LFOs available for modulation:

#### Waveforms
- **Sine**: Smooth, musical modulation
- **Triangle**: Linear up/down movement
- **Square**: On/off switching
- **Saw**: Ramping effect
- **Noise**: Random modulation
- **Sample & Hold**: Stepped random values

#### Parameters
- **Rate**: Speed of modulation (0.01-20 Hz)
- **Depth**: Amount of modulation
- **Phase**: Starting position in waveform
- **Sync**: Free running or tempo-synced

### Envelopes

8 ADSR envelopes for shaping modulation:

#### ADSR Parameters
- **Attack**: Time to reach peak
- **Decay**: Time to fall to sustain
- **Sustain**: Held level
- **Release**: Time to silence after note off

#### Advanced Features
- **Curve**: Linear, exponential, or logarithmic
- **Loop**: Repeat envelope cycles
- **Velocity**: Velocity sensitivity

### Modulation Matrix

Connect sources to destinations:

#### Sources
- LFOs 1-8
- Envelopes 1-8
- MIDI: Velocity, Aftertouch, Mod Wheel, Pitch Bend
- Key Tracking
- Random values

#### Destinations
- Engine parameters (HARMONICS, TIMBRE, MORPH)
- Filter cutoff and resonance
- Amplifier level and pan
- Effect parameters
- LFO and envelope parameters

#### Creating Connections
1. Go to Mod view
2. Select a destination slot
3. Choose source from dropdown
4. Adjust modulation amount (-100% to +100%)
5. Fine-tune with curve selection

## Sample Management

### Loading Samples

#### Supported Formats
- **WAV/AIFF**: Uncompressed (recommended)
- **MP3/M4A**: Compressed formats
- **FLAC**: Lossless compression

#### Loading Methods
1. **Drag & Drop**: From Finder to sampler pads
2. **Loop Browser**: Navigate file system with preview
3. **Scatter Browser**: Visual sample organization

### Scatter Browser

2D visualization of your sample library:

#### Axes
- **X-axis (Timbre)**: Brightness/darkness
- **Y-axis (Energy)**: Intensity/dynamics

#### Usage
1. Analyze samples to position them
2. Navigate by clicking regions
3. Preview by hovering
4. Select by double-clicking

#### Organization
- Samples cluster by similarity
- Colors indicate categories
- Search by name or tags

### Loop Browser

Traditional file browser with audio features:

#### Navigation
- Folder tree on left
- File list on right
- Preview current selection

#### Features
- Audio preview with waveform
- Metadata display (BPM, key, duration)
- Format filtering
- Batch import

### SamplerKit (MPC-Style)

25-pad sampler with comprehensive editing:

#### Pad Setup
1. Load samples to pads (1-25)
2. Adjust volume, pitch, pan per pad
3. Set up choke groups for realistic drums
4. Configure one-shot vs. loop mode

#### Pad Inspector
- **Volume**: Individual pad level
- **Pitch**: Transpose in semitones  
- **Pan**: Stereo position
- **Start/End**: Sample playback range
- **Envelope**: Attack and decay
- **Filter**: Low-pass filtering
- **Choke Group**: For hi-hat groups

### SamplerSlicer

Slice long samples into triggerable parts:

#### Slice Creation
1. **Load Sample**: Drag audio file to slicer
2. **Choose Method**:
   - **Manual**: Click to create slices
   - **Grid**: Evenly divide sample
   - **Transient**: Auto-detect attacks
   - **Beat**: Musical divisions

#### Slice Editing
- **Move**: Drag slice boundaries
- **Delete**: Right-click slice marker
- **Preview**: Click slice to hear
- **Adjust**: Per-slice volume, pitch, pan

#### Playback
- Slices are mapped to MIDI notes
- Trigger individual slices or sequences
- Real-time slice manipulation

## Effects and Processing

### Insert Effects

#### TransientShaper
**Purpose**: Enhance or reduce attack transients
- **Attack**: Boost or cut initial transient
- **Sustain**: Adjust sustain portion
- **Use**: Making drums punchy or smooth

#### Chorus
**Purpose**: Thicken and widen sounds
- **Rate**: LFO speed for modulation
- **Depth**: Amount of pitch modulation
- **Mix**: Wet/dry balance
- **Use**: Guitars, synths, vocals

#### Bitcrusher
**Purpose**: Digital distortion and lo-fi effects
- **Bits**: Bit depth reduction (1-16 bits)
- **Sample Rate**: Sample rate reduction
- **Mix**: Wet/dry balance
- **Use**: Aggressive digital distortion

#### MicroDelay
**Purpose**: Short delays and doubling
- **Time**: Delay time (1-100ms)
- **Feedback**: Delay regeneration
- **Filter**: High/low cut
- **Use**: Thickening, slap-back echo

#### Saturator
**Purpose**: Analog-style harmonic distortion
- **Drive**: Input gain and saturation
- **Character**: Saturation curve
- **Output**: Compensating gain
- **Use**: Warmth, harmonic enhancement

### Effect Routing

#### Insert Effects
- Process signal in series
- Affect entire channel signal
- Use for EQ, compression, distortion

#### Send Effects
- Process signal in parallel
- Mix with original signal
- Use for reverb, delay, modulation

#### Effect Ordering
- Insert effects: Order matters
- Typical order: EQ → Compressor → Saturation
- Experiment with different orders

## Project Management

### File Operations

#### Saving Projects
- **Save**: Ctrl+S (updates current file)
- **Save As**: Create new file with different name
- **Auto-save**: Automatic backup every 30 seconds

#### Project Files
- **.ether**: Native project format
- Contains: Track data, engine settings, patterns
- References: Links to external sample files

#### Export Options
- **Audio Export**: Render to WAV/AIFF
- **MIDI Export**: Save patterns as MIDI files
- **Preset Export**: Save engine settings

### Templates

#### Creating Templates
1. Set up desired configuration
2. File → Save as Template
3. Name your template
4. Available in New Project dialog

#### Included Templates
- **Empty 8-Track**: Blank project
- **Electronic**: Common electronic music setup
- **Drum Machine**: Focused on percussion
- **Ambient**: Textural pad-focused setup

### Presets

#### Engine Presets
- Save/load engine configurations
- Include all parameters and modulations
- Organize by category and tags

#### Pattern Presets
- Save commonly used rhythms
- Load into any compatible engine
- Build personal groove library

## Performance and Optimization

### Audio Settings

#### Buffer Size
- **64 samples**: 1.5ms latency, high CPU
- **128 samples**: 3ms latency, balanced (recommended)
- **256 samples**: 6ms latency, low CPU
- **512+ samples**: 12ms+ latency, lowest CPU

#### Sample Rate
- **44.1kHz**: CD quality, efficient
- **48kHz**: Video standard, common
- **96kHz**: High resolution, higher CPU

### CPU Optimization

#### Engine Usage
- Each engine uses ~3% CPU per voice
- Disable unused engines
- Use fewer simultaneous voices when needed

#### Effect Optimization
- Bypass unused effects rather than removing
- Send effects are more CPU efficient than inserts
- Monitor CPU usage in performance view

#### Project Optimization
- Bounce complex MIDI to audio
- Freeze tracks with heavy processing
- Use appropriate sample rates for material

### Memory Management

#### Sample Loading
- Samples load into RAM for instant access
- Large projects may require more RAM
- Consider sample resolution for memory usage

#### Project Size
- Keep sample files organized
- Remove unused samples
- Compress samples when appropriate

## Troubleshooting

### Audio Issues

#### No Sound
1. Check audio device selection
2. Verify output routing
3. Check master volume
4. Ensure track isn't muted

#### Crackling/Dropouts
1. Increase buffer size
2. Close unnecessary applications
3. Check CPU usage
4. Verify audio driver status

#### High Latency
1. Decrease buffer size
2. Use direct audio drivers
3. Close background applications
4. Check system audio settings

### Performance Issues

#### High CPU Usage
1. Increase buffer size
2. Disable unnecessary engines
3. Reduce polyphony
4. Freeze CPU-heavy tracks

#### Slow Interface
1. Disable haptic feedback
2. Reduce visual effects
3. Close other applications
4. Restart EtherSynth

### MIDI Issues

#### MIDI Not Working
1. Check MIDI device selection
2. Verify MIDI input routing
3. Test with virtual keyboard
4. Check MIDI channel settings

#### Wrong Notes/Timing
1. Verify MIDI channel
2. Check transpose settings
3. Confirm tempo sync
4. Test MIDI device separately

## Keyboard Shortcuts

### Global
- **⌘N**: New Project
- **⌘O**: Open Project  
- **⌘S**: Save Project
- **⌘Z**: Undo
- **⌘⇧Z**: Redo
- **⌘Q**: Quit Application

### Transport
- **Space**: Play/Pause
- **Return**: Stop
- **R**: Record
- **L**: Loop
- **Home**: Go to Start
- **End**: Go to End

### View Navigation
- **1**: Timeline View
- **2**: Mix View
- **3**: Mod View
- **4**: Browser View
- **F**: Toggle Fullscreen

### Timeline
- **⌘A**: Select All
- **Delete**: Delete Selected
- **⌘D**: Duplicate Selected
- **⌘C**: Copy
- **⌘V**: Paste
- **G**: Grid Snap Toggle

### Mixing
- **M**: Mute Selected Track
- **S**: Solo Selected Track
- **⌘M**: Mute All
- **⌘S**: Clear Solo All

### Engine Control
- **0-9**: Select Track 1-8 (numbers)
- **⌘0-9**: Assign Engine to Track
- **P**: Open Pattern Editor
- **E**: Open Engine Settings

## Quick Start Guide

### Your First Beat

1. **New Project**: Create empty 8-track project
2. **Load DrumKit**: Assign DrumKit engine to Track 1
3. **Create Pattern**: Add drum pattern to Track 1
4. **Edit Pattern**: Double-click pattern, add kick and snare
5. **Play**: Press spacebar to hear your beat
6. **Add Bass**: Assign MacroVA to Track 2, create note pattern
7. **Mix**: Adjust levels in Mix view
8. **Save**: Save your project

### Your First Melody

1. **Choose Engine**: Try MacroWavetable for evolving sounds
2. **Create Pattern**: Add note pattern to timeline
3. **Edit Notes**: Double-click pattern, add melody
4. **Modulation**: Add LFO to HARMONICS for movement
5. **Effects**: Add chorus for width
6. **Automation**: Automate TIMBRE for evolution

### Exploring Samples

1. **Load SamplerKit**: Assign to empty track
2. **Browse Samples**: Use Loop Browser or Scatter Browser
3. **Load to Pads**: Drag samples to pads 1-8
4. **Create Pattern**: Make drum pattern triggering pads
5. **Process**: Add effects and EQ in Mix view
6. **Slice Loops**: Try SamplerSlicer for complex rhythms

Remember: EtherSynth is designed for exploration. Don't be afraid to experiment with different engines, modulation routings, and creative processing chains. The best way to learn is by doing!

---

*For additional support, updates, and community resources, visit the EtherSynth website or join our user forum.*