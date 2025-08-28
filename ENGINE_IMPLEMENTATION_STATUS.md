# Engine Implementation Progress

## ✅ COMPLETED ENGINES

### 1. MacroVA (Virtual Analog)
- **File**: `src/synthesis/engines/MacroVAEngine.h`
- **Features**: Variable shape oscillators (tri→saw→square), unison voices, sub oscillator, noise mixing
- **Parameters**: 
  - HARMONICS → Shape blend
  - TIMBRE → Pulse width  
  - MORPH → Unison spread
  - UNISON_COUNT, UNISON_SPREAD, SUB_LEVEL, NOISE_MIX, SYNC_DEPTH
- **CPU**: Light (≤3k cycles)
- **Status**: COMPLETE ✅

### 2. MacroFM (2-Op FM)  
- **File**: `src/synthesis/engines/MacroFMEngine.h`
- **Features**: 2-operator FM with 3 algorithms, high-precision phase accumulators, oversampling
- **Parameters**:
  - HARMONICS → Frequency ratio {0.5,1,1.5,2,3,4,5,7,9}
  - TIMBRE → FM index (exponential 0..~8)
  - MORPH → Feedback (-1..+1)
  - ALGO, BRIGHT_TILT, FIXED_MOD
- **CPU**: Light (≤2k cycles)
- **Status**: COMPLETE ✅

## 🚧 IN PROGRESS (Local LLM Working)

### 3. MacroWaveshaper (Fold)
- **Target**: Waveshaping/folding with 2x oversampling
- **Parameters**: HARMONICS(pre-emphasis), TIMBRE(fold depth), MORPH(symmetry)
- **CPU**: Medium (≤3.5k cycles)
- **Status**: LLM is implementing

## 📋 PENDING ENGINES (LLM Task List)

### 4. MacroWavetable
- **Requirements**: 4 banks×64 frames, 4 mip levels, cubic interpolation
- **Parameters**: HARMONICS(frame 0-63), TIMBRE(intra-morph), MORPH(bank 0-3)
- **CPU**: Light (≤2.5k cycles)

### 5. MacroChord  
- **Requirements**: Chord generation with strumming, 3-5 voices
- **Parameters**: HARMONICS(chord type), TIMBRE(spread 0-12st), MORPH(detune)
- **CPU**: Medium (≤4k cycles)

### 6. MacroHarmonics (Additive)
- **Requirements**: 1-16 partials, 1/f^x decay, inharmonicity
- **Parameters**: PARTIAL_COUNT, DECAY_EXP, INHARMONICITY, EVEN_ODD_BIAS
- **CPU**: Medium (≤4k cycles @16 partials)

### 7. Formant/Vocal
- **Requirements**: A-E-I-O-U vowel morphing, formant filtering
- **Parameters**: VOWEL, BANDWIDTH, BREATH, FORMANT_SHIFT, GLOTTAL_SHAPE
- **CPU**: Light (≤3k cycles)

### 8. Noise/Particles
- **Requirements**: Granular/particle synthesis with bandpass filtering
- **Parameters**: DENSITY_HZ(1-200), GRAIN_MS(5-60), BP_CENTER, BP_Q, SPRAY
- **CPU**: Light (≤2k cycles)

### 9. TidesOsc (Slope Oscillator)
- **Requirements**: Based on Mutable Tides, slope/contour morphing
- **Parameters**: CONTOUR(sine→ramp→pulse), SLOPE, UNISON, CHAOS, LFO_MODE
- **CPU**: Light (≤2.5k cycles)

### 10. RingsVoice (Modal Resonator)
- **Requirements**: Based on Mutable Rings, exciter + modal resonator
- **Parameters**: STRUCTURE, BRIGHTNESS, POSITION, EXCITER, DAMPING, SPACE_MIX
- **CPU**: Heavy (≤12k cycles, ≤4 voices recommended)

### 11. ElementsVoice (Physical Model)
- **Requirements**: Based on Mutable Elements, rich physical modeling
- **Parameters**: GEOMETRY, ENERGY, DAMPING, EXCITER_BAL, SPACE, NOISE_COLOR  
- **CPU**: Very Heavy (≤18k cycles, ≤2-4 voices recommended)

## 📚 REFERENCE MATERIALS

### Mutable Source Code
- **Location**: `/Users/lachlanfysh/Desktop/ether/mutable_extracted/`
- **Key Files**:
  - `plaits/engine/*` - All synthesis engines
  - `plaits/oscillator/*` - Oscillator implementations
  - `rings/dsp/*` - Modal resonator algorithms
  - `elements/dsp/*` - Physical modeling
  - `tides/poly_slope_generator.*` - Slope generator
  - `clouds/dsp/*` - Granular synthesis
  - `braids/*oscillator.*` - Waveshaping algorithms

### Requirements Document
- **Location**: `/Users/lachlanfysh/Desktop/ether/revised engine requirements 2.txt`
- **Contains**: Detailed specifications for all engines, CPU budgets, parameter mappings

### Architecture Templates
- **BaseVoice**: `/Users/lachlanfysh/Desktop/ether/src/synthesis/BaseEngine.h`
- **IEngine**: `/Users/lachlanfysh/Desktop/ether/src/synthesis/IEngine.h`
- **DSP Utils**: `/Users/lachlanfysh/Desktop/ether/src/synthesis/DSPUtils.h`

## 🎯 NEXT STEPS WHEN RETURNING

1. **Check LLM Progress**: Review what engines the local LLM has completed
2. **Code Review**: Verify implementations match architecture and requirements  
3. **Integration**: Add new engines to factory and bridge
4. **Testing**: Build and test new engines with SwiftUI app
5. **Bridge Updates**: Update enhanced_bridge.cpp to expose all engines
6. **UI Updates**: Ensure SwiftUI can access all new engines

## 💡 IMPLEMENTATION NOTES

- **Pattern Consistency**: All engines follow MacroVA/MacroFM template
- **Real-time Safety**: No allocations after prepare(), deterministic processing
- **CPU Budgets**: Strict per-engine limits for real-time performance  
- **Parameter Mapping**: HARMONICS/TIMBRE/MORPH consistently mapped per engine
- **Channel Strip**: All voices get automatic channel strip processing
- **Anti-aliasing**: BLEP/oversampling used where needed for audio quality