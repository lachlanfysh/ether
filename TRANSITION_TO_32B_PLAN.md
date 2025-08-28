# Transition to Qwen2.5-Coder 32B Model Plan

## Current Status (Pre-32B)
- **Completed Engines**: MacroVA, MacroFM, MacroWaveshaper, MacroWavetable (4/11)
- **Remaining Engines**: MacroChord, MacroHarmonics, Formant, Noise, Tides, Rings, Elements (7/11)
- **7B Model Work**: Base infrastructure, 4 engines implemented following architecture

## Phase 1: Model Transition & Validation
1. **Verify 32B Model Installation**
   - Test model responsiveness and quality
   - Compare code generation quality vs 7B model
   - Validate understanding of project architecture

2. **Code Review of 7B Model Work**
   - Review all 4 implemented engines for:
     - Architecture compliance
     - Real-time safety
     - Parameter mapping correctness
     - CPU budget adherence
     - Code quality and optimization opportunities

## Phase 2: 32B Model Work Queue
1. **Complete Remaining 7 Engines** (in priority order)
   - MacroChord (chord generation with strumming)
   - MacroHarmonics (additive synthesis, 1-16 partials)
   - Formant/Vocal (vowel morphing, formant filtering)
   - Noise/Particles (granular synthesis)
   - TidesOsc (slope oscillator from Mutable Tides)
   - RingsVoice (modal resonator from Mutable Rings)
   - ElementsVoice (physical modeling from Mutable Elements)

2. **Enhanced Bridge Integration**
   - Update enhanced_bridge.cpp to expose all 11 engines
   - Test SwiftUI integration with all engines
   - Verify parameter mapping works correctly

3. **Code Quality Improvements**
   - Optimize all engines for better performance
   - Add proper error handling
   - Improve documentation and comments
   - Validate real-time constraints

## Phase 3: Final Integration & Testing
1. **Build System Integration**
   - Ensure all engines compile correctly
   - Update build scripts
   - Test with Swift Package Manager and Xcode

2. **GUI Phase Preparation**
   - Review GUI requirements and resources
   - Plan SwiftUI implementation approach
   - Prepare for GUI development phase

## Success Criteria
- ✅ All 11 synthesis engines implemented and tested
- ✅ Enhanced bridge exposes all engines to SwiftUI
- ✅ Real-time performance maintained (<2.5-18k cycles per engine)
- ✅ Code quality meets professional standards
- ✅ Ready for GUI implementation phase

## Files to Monitor/Review
- All engine files: `src/synthesis/engines/Macro*.h`
- Factory: `src/synthesis/EngineFactory.cpp`
- Bridge: `enhanced_bridge.cpp`
- Status docs: `ENGINE_IMPLEMENTATION_STATUS.md`, `LLM_EXTENDED_TASK_QUEUE.md`