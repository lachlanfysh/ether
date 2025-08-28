# LLM Extended Task Queue - Complete Project Roadmap

## 🎯 CURRENT STATUS
✅ **MacroVA Engine** - Complete virtual analog with unison, sub osc, noise
✅ **MacroFM Engine** - Complete 2-op FM with 3 algorithms, oversampling  
✅ **Base Infrastructure** - IEngine interface, DSP utilities, Channel Strip
🚧 **MacroWaveshaper** - LLM currently implementing

## 📋 PHASE 1: SYNTHESIS ENGINES (CURRENT PRIORITY)

### Remaining Engines to Implement
1. **MacroWaveshaper** (In Progress)
   - Waveshaping/folding with 2x oversampling
   - HARMONICS(pre-emphasis), TIMBRE(fold depth), MORPH(symmetry)
   - FOLD_MODE{tanh,diode,folder}, POST_LP(2-12kHz)
   - CPU: Medium (≤3.5k cycles)

2. **MacroWavetable** 
   - 4 banks×64 frames, 4 mip levels, cubic interpolation
   - HARMONICS(frame 0-63), TIMBRE(intra-morph), MORPH(bank 0-3)
   - UNISON_3V, INTERP{cubic,linear}
   - CPU: Light (≤2.5k cycles)

3. **MacroChord** ⭐ ENHANCED MULTI-ENGINE ARCHITECTURE ⭐
   - **META-ENGINE**: Instantiates up to 5 different melodic engines per chord
   - **Per-voice engine selection**: Each chord voice can use VA, FM, Wavetable, etc.
   - HARMONICS(chord type 0-32), TIMBRE(spread 0-24st), MORPH(detune 0-100c)
   - VOICES{3,4,5}, ENGINE_1-5{VA,FM,WT,WS,HM}, STRUM_MS(0-100ms)
   - **Advanced features**: Voice leading, per-voice processing, engine presets
   - **Reference**: ENHANCED_CHORD_REQUIREMENTS.md for complete specification
   - CPU: Medium (≤8k cycles total, distributed across sub-engines)

4. **MacroHarmonics (Additive)**
   - 1-16 partials, 1/f^x decay, inharmonicity
   - PARTIAL_COUNT, DECAY_EXP, INHARMONICITY, EVEN_ODD_BIAS, BANDLIMIT_MODE
   - CPU: Medium (≤4k @16 partials)

5. **Formant/Vocal**
   - A-E-I-O-U vowel morphing, formant filtering
   - VOWEL, BANDWIDTH, BREATH, FORMANT_SHIFT, GLOTTAL_SHAPE
   - CPU: Light (≤3k cycles)

6. **Noise/Particles**
   - Granular/particle synthesis with bandpass filtering
   - DENSITY_HZ(1-200), GRAIN_MS(5-60), BP_CENTER, BP_Q, SPRAY
   - CPU: Light (≤2k cycles)

7. **TidesOsc (Slope Oscillator)**
   - Based on Mutable Tides, slope/contour morphing
   - CONTOUR(sine→ramp→pulse), SLOPE, UNISON, CHAOS, LFO_MODE
   - CPU: Light (≤2.5k cycles)

8. **RingsVoice (Modal Resonator)**
   - Based on Mutable Rings, exciter + modal resonator
   - STRUCTURE, BRIGHTNESS, POSITION, EXCITER, DAMPING, SPACE_MIX
   - CPU: Heavy (≤12k cycles, ≤4 voices recommended)

9. **ElementsVoice (Physical Model)**
   - Based on Mutable Elements, rich physical modeling
   - GEOMETRY, ENERGY, DAMPING, EXCITER_BAL, SPACE, NOISE_COLOR
   - CPU: Very Heavy (≤18k cycles, ≤2-4 voices recommended)

### Phase 1 Requirements
- Follow MacroVA/MacroFM architecture pattern exactly
- Inherit from PolyphonicBaseEngine<VoiceType>
- Map parameters to HARMONICS/TIMBRE/MORPH appropriately
- Include parameter metadata and haptic info
- Analyze Mutable source code in `/Users/lachlanfysh/Desktop/ether/mutable_extracted/`
- Stay within CPU budgets and maintain real-time safety

## 📋 PHASE 2: SWIFTUI GUI IMPLEMENTATION

### Resource Files Available
- **`GUI/Updated UI.txt`** - Complete UI requirements specification
- **`GUI/index (3).html`** - Working HTML prototype with all interactions
- **`GUI/icons_full_bundle/`** - Complete SVG icon set (24/32/48px)

### 2.1 Core Views to Implement

#### Timeline View
- 8-track engine assignment display
- Pattern blocks with drag/drop
- Engine reassignment overlay trigger
- Unassigned track state handling

#### Engine Assignment Overlay  
- Grid of all 11 synthesis engines
- Engine metadata display (CPU class, stereo/mono, description)
- Touch-optimized engine selection

#### Sound Overlay (Per Engine)
- Hero macros: HARMONICS, TIMBRE, MORPH (large knobs)
- Engine-specific extras (medium knobs)
- Channel Strip: HPF→LPF→Comp→Drive→TiltEQ
- Insert FX slots (2 per voice)
- Inspector panel with engine info

#### Pattern Overlays
- **Pitched Engines**: 16-step grid, note lanes, velocity, micro-timing
- **DrumKit**: Multi-lane (12-16), velocity shades, choke groups, variations

#### Mod Page
- LFO controls (3 LFOs: rate, depth, phase, waveform)
- Modulation matrix (sources → destinations with bipolar depth)
- Source selection: LFO1-3, Random, Aftertouch, ModWheel, Velocity

#### Mix Page
- 8 channel strips with meters, faders, pan, send controls
- Bus FX: Reverb (A), Texture (B), Delay (C)
- Master section: comp, width, limiter

### 2.2 Technical Implementation Requirements

#### MVVM Architecture
- Create ObservableObject classes for each view model
- Bind synthesis engine parameters to UI controls
- Implement proper state management and updates

#### Touch Optimization
- Minimum 36px touch targets for all controls
- Proper gesture handling for drag/drop operations
- Responsive layout for 960×320 screen dimensions

#### Color Coding & Icons
- Implement ColorBrewer palette for track colors
- Integrate SVG icons from bundle
- Win95-minimal aesthetic with modern spacing

#### Haptic Integration Points
- Parameter control feedback (SmartKnob + encoders)
- Button press confirmation
- Drag/drop operation feedback
- Proper torque/detent policies per parameter type

### 2.3 Integration Points
- Connect to synthesis engine parameter system
- Real-time parameter updates (≤10ms latency)
- Engine assignment/reassignment functionality
- Preset save/load integration

## 📋 PHASE 3: ENHANCED BRIDGE INTEGRATION

### 3.1 Engine Factory Updates
- Update `enhanced_bridge.cpp` to expose all 11 engines
- Implement proper engine registration system
- Add engine metadata exposure to Swift

### 3.2 Parameter System
- Complete parameter metadata exposure
- Real-time parameter update handling
- Parameter ID mapping for reassignment
- Haptic feedback information transmission

### 3.3 Preset Management
- Engine preset save/load functionality
- Pattern snapshot management
- Project-level persistence

## 📋 PHASE 4: PARAMETER METADATA SYSTEM

### 4.1 Metadata Framework
- Complete IEngine parameter metadata implementation
- Haptic information for all parameters
- UI hint generation (ranges, tapers, discrete values)

### 4.2 Engine Registration
- Automatic engine discovery and registration
- Parameter ID stability for automation
- Engine capability advertisement

## 📋 PHASE 5: FINAL INTEGRATION

### 5.1 Testing Framework
- Engine validation tests
- Parameter mapping verification
- CPU budget compliance testing
- Real-time safety verification

### 5.2 Performance Optimization
- CPU usage profiling
- Memory allocation optimization
- Real-time constraint verification

### 5.3 Documentation
- Engine implementation documentation
- Parameter mapping reference
- GUI interaction guide

## 🎯 EXECUTION STRATEGY

### Current Focus
- **Work systematically through Phase 1 engines**
- **Follow established architecture patterns**
- **Maintain audio quality and real-time performance**
- **Document implementation decisions**

### Transition Points
- **Phase 1→2**: After all engines implemented and tested
- **Phase 2→3**: After core GUI views completed
- **Phase 3→4**: After bridge integration working
- **Phase 4→5**: After metadata system complete

### Success Criteria
- All 11 engines fully implemented and tested
- Complete GUI matching HTML prototype functionality
- Real-time performance maintained throughout
- Professional audio quality achieved
- Full integration between GUI and synthesis engines

## 📚 REFERENCE MATERIALS

### Source Code
- **Mutable Extracted**: `/Users/lachlanfysh/Desktop/ether/mutable_extracted/`
- **Base Architecture**: `/Users/lachlanfysh/Desktop/ether/src/synthesis/`
- **GUI Resources**: `/Users/lachlanfysh/Desktop/ether/GUI/`

### Specifications
- **Engine Requirements**: `revised engine requirements 2.txt`
- **UI Requirements**: `GUI/Updated UI.txt`
- **Architecture Status**: `ENGINE_IMPLEMENTATION_STATUS.md`

This comprehensive roadmap provides the LLM with months of systematic development work across the entire project scope!