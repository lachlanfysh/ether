# EtherSynth Engine Catalog

## Engine Overview (10 Consolidated Engines)

### **Analogue** → `MacroVAEngine`
Classic virtual analog synthesis with oscillators, filters, and envelopes. The cornerstone of subtractive synthesis.  
**Strengths**: Fat bass lines, lush pads, classic lead sounds, intuitive parameter response.  
**Typical Use**: Bass, leads, pads, strings, classic electronic music sounds.

### **FM** → `FMEngine` (Macro + Classic4Op modes)
Frequency modulation synthesis with both macro-style and classic 4-operator configurations.  
**Strengths**: Metallic timbres, bell sounds, complex harmonic content, dramatic frequency sweeps.  
**Typical Use**: Bells, metallic percussion, sci-fi sounds, complex pads, aggressive leads.

### **Wavetable** → `MacroWavetableEngine`
Wavetable synthesis with smooth interpolation between stored waveforms for evolving textures.  
**Strengths**: Evolving textures, smooth timbral morphing, rich harmonic content, modern EDM sounds.  
**Typical Use**: EDM leads, evolving pads, atmospheric textures, modern electronic music.

### **Shaper** → `MacroWaveshaperEngine`
Waveshaping/distortion synthesis focused on nonlinear signal processing and harmonic generation.  
**Strengths**: Aggressive distortion, harmonic enhancement, overdrive effects, tube-like warmth.  
**Typical Use**: Distorted leads, aggressive basses, tube emulation, harmonic enhancement.

### **Vocal** → `FormantEngine`
Formant synthesis for vocal and speech-like sounds using resonant filtering and vowel simulation.  
**Strengths**: Human-like vocal textures, vowel morphing, speech synthesis, organic character.  
**Typical Use**: Vocal pads, choir sounds, speech synthesis, human-like textures.

### **Modal** → `PhysicalEngine` (Modal/Rings mode)
Modal synthesis simulating resonant objects, strings, and physical structures via resonator modeling.  
**Strengths**: Realistic string/percussion sounds, natural decay characteristics, harmonic resonance.  
**Typical Use**: Plucked strings, mallet percussion, resonant objects, ethnic instruments.

### **Exciter** → `PhysicalEngine` (Exciter/Elements mode)
Excitation-based physical modeling using bow/blow/strike paradigms for acoustic instrument simulation.  
**Strengths**: Realistic acoustic instrument modeling, expressive dynamics, natural articulation.  
**Typical Use**: Bowed strings, blown pipes, struck percussion, acoustic instrument emulation.

### **Morph** → `MorphEngine` (Harmonics/Tides modes)
Morphing oscillators and harmonic manipulation providing smooth transitions between timbral states.  
**Strengths**: Smooth parameter sweeps, complex harmonic relationships, evolving timbres.  
**Typical Use**: Evolving leads, morphing pads, complex harmonic textures, experimental sounds.

### **Noise** → `NoiseEngine`
Noise-based synthesis for percussion, textures, and stochastic sound generation with filtering.  
**Strengths**: Percussion synthesis, textural elements, wind sounds, random/chaotic textures.  
**Typical Use**: Hi-hats, cymbals, percussion, wind/water sounds, textural backgrounds.

### **Sampler** → `SamplerEngine` (Slicer/Oneshot modes)
Sample playback with slicing capabilities and oneshot triggering for recorded audio manipulation.  
**Strengths**: Real recordings, familiar sounds, creative slicing/chopping, authentic textures.  
**Typical Use**: Drums, vocal chops, found sounds, breakbeats, realistic instruments.

### **Filter** → `SerialHPLPEngine`
Filter-centric synthesis using serial high-pass/low-pass combinations as primary tone generation.  
**Strengths**: Resonant sweeps, filter self-oscillation, minimal CPU usage, clean filtering.  
**Typical Use**: Filter sweeps, minimal techno, resonant leads, clean filtering effects.

### **Acid** → `SlideAccentBassEngine`
TB-303 style acid bass synthesis with slide and accent characteristics for classic acid house.  
**Strengths**: Authentic acid bass character, slide effects, accent dynamics, classic rave sounds.  
**Typical Use**: Acid bass lines, classic house/techno, rave sounds, electronic dance music.

### **Multi-Voice** → `MacroChordEngine`
Polyphonic chord synthesis with multiple voice management and harmonic distribution across instruments.  
**Strengths**: Rich chord textures, voice leading, harmonic complexity, layered arrangements.  
**Typical Use**: Complex chords, layered harmony, orchestral textures, rich polyphonic passages.

### **Drums** → `DrumKitEngine`
Synthesized drum sounds using dedicated synthesis algorithms for kick, snare, and hi-hat generation.  
**Strengths**: Punchy electronic drums, consistent timing, synthesis-based flexibility, low CPU.  
**Typical Use**: Electronic drum kits, sequenced beats, synthetic percussion, EDM drums.

## Engine Selection Guide

### **For Beginners**: Start with Analogue, Drums, Noise
These engines provide immediate results with intuitive parameter responses and familiar sounds.

### **For Bass**: Analogue, Acid, FM  
Analogue for classic bass, Acid for 303-style lines, FM for aggressive/metallic bass.

### **For Leads**: Analogue, Wavetable, Shaper, FM
Wide range from classic analog leads to modern digital textures and aggressive distorted sounds.

### **For Pads**: Analogue, Wavetable, Vocal, Morph, Multi-Voice
Rich harmonic content and evolving textures perfect for atmospheric background sounds.

### **For Drums**: Drums, Noise, Sampler
Drums for electronic kits, Noise for hi-hats/cymbals, Sampler for realistic acoustic drums.

### **For Experimental**: Modal, Exciter, Morph, Shaper, Filter
Advanced synthesis methods for unique textures and unconventional sounds.

### **For Realistic Instruments**: Modal, Exciter, Vocal, Sampler
Physical modeling and sample-based engines for authentic acoustic instrument sounds.

## CPU Usage Guide (Per Voice, 48kHz)

| Engine | CPU Cost | Notes |
|--------|----------|-------|
| Filter | ~2% | Most efficient |
| Noise | ~3% | Simple synthesis |  
| Analogue | ~4% | Standard reference |
| Drums | ~4% | Optimized percussion |
| Acid | ~5% | TB-303 modeling |
| Wavetable | ~6% | Table interpolation |
| FM | ~6% | Modulator calculations |
| Shaper | ~7% | Nonlinear processing |
| Sampler | ~8% | Sample playback + effects |
| Vocal | ~9% | Formant processing |
| Morph | ~10% | Complex oscillators |
| Modal | ~12% | Resonator modeling |
| Exciter | ~15% | Physical modeling |
| Multi-Voice | ~20% | Multiple voice processing |

## Parameter Compatibility

### **Engines with Native Core Support**:
- **Analogue**: HARMONICS (osc mix), TIMBRE (cutoff), MORPH (filter type)
- **FM**: HARMONICS (ratio), TIMBRE (mod index), MORPH (algorithm)
- **Wavetable**: HARMONICS (position), TIMBRE (morph), MORPH (bank)
- **Physical**: HARMONICS (harmonics), TIMBRE (brightness), MORPH (damping)

### **Engines Using Post-Chain Fallback**:
- **Noise, Filter, Drums**: All core parameters via post-chain processing
- **Sampler**: Partial native support, fallback for synthesis parameters