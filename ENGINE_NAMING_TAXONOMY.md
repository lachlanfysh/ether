# Engine Naming & Taxonomy (Neutral UI Labels)

Display categories and names separate engine identity from backend IDs. This mapping keeps UI consistent and scan‑friendly.

Categories
- Synthesizers: MacroVA, MacroFM, MacroWaveshaper, MacroWavetable, MacroHarmonics
- Multi‑Voice: MacroChord
- Textures: FormantVocal, NoiseParticles
- Physical Models: TidesOsc, RingsVoice, ElementsVoice
- Drums: DrumKit (synth drum engine)
- Sampler: SamplerKit, SamplerSlicer
- Granular: Granular (new)
- Filter/Other: SerialHPLP (and unclassified engines)

Display name mapping
- Analogue: MacroVA
  - Rationale: Subtractive VA synthesis with classic filtering.
- FM: MacroFM, Classic4OpFM
  - Rationale: Frequency modulation engines; 4‑op grouped under FM too.
- Wavetable: MacroWavetable
  - Rationale: Morphing wavetable oscillator.
- Shaper: MacroWaveshaper
  - Rationale: Waveshaping/folding domain.
- Vocal: FormantVocal
  - Rationale: Vowel/formant synthesis.
- Modal: RingsVoice
  - Rationale: Resonant modes (modal resonator).
- Exciter: ElementsVoice
  - Rationale: Excitation/material modeling.
- Morph: MacroHarmonics, TidesOsc
  - Rationale: Harmonic/morphing oscillators; both fit morph semantics.
- Noise: NoiseParticles
  - Rationale: Noise/particle textures.
- Drums: DrumKit
  - Rationale: Synth drum engine — distinct from samplers; always present.
- Sampler: SamplerKit, SamplerSlicer
  - Rationale: Sample playback and slicing.
- Filter: SerialHPLP
  - Rationale: Serial HP/LP processing engine.
- Granular: Granular
  - Rationale: Grain cloud synthesis; future expansion.

Ambiguity notes
- Morph: Both MacroHarmonics and TidesOsc are grouped here for UI simplicity (they differ in method but share “morphing” semantics).
- Drums vs Sampler: “Drums” exclusively refers to the synth DrumKit; sampler engines stay in “Sampler”.

Implementation notes
- C++ enhanced_bridge getEngineCategory() returns "Drums" for DrumKit and "Sampler" for sampler engines; display names reflect this.
- Standalone bridge and UI overlays should reflect these categories for clarity.

