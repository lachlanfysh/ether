# Code Review Template for 32B Model

## Review Checklist for Each Engine

### 1. Architecture Compliance
- [ ] Inherits from PolyphonicBaseEngine<VoiceType>?
- [ ] Voice class inherits from BaseVoice?
- [ ] Follows exact pattern of MacroVA/MacroFM?
- [ ] Proper constructor/destructor implementation?
- [ ] Virtual functions properly overridden?

### 2. Real-Time Safety
- [ ] No memory allocations in audio thread?
- [ ] No blocking operations in renderSample()?
- [ ] No exceptions thrown in audio code?
- [ ] Deterministic execution paths?
- [ ] Proper bounds checking?

### 3. Parameter System
- [ ] HARMONICS/TIMBRE/MORPH properly mapped?
- [ ] Parameter metadata complete and accurate?
- [ ] Haptic information provided?
- [ ] Parameter ranges and defaults sensible?
- [ ] Discrete parameters handled correctly?

### 4. Performance & CPU Budget
- [ ] Within target CPU budget (see requirements)?
- [ ] Efficient algorithms used?
- [ ] Unnecessary computations avoided?
- [ ] SIMD-friendly code structure?
- [ ] Hot paths optimized?

### 5. Code Quality
- [ ] Clear, readable code structure?
- [ ] Appropriate comments for complex sections?
- [ ] Consistent naming conventions?
- [ ] Proper const correctness?
- [ ] No memory leaks or dangling pointers?

### 6. Audio Quality
- [ ] Anti-aliasing where needed?
- [ ] Proper sample rate handling?
- [ ] No audio artifacts (clicks, pops)?
- [ ] Good dynamic range?
- [ ] Musically useful parameter ranges?

### 7. Integration
- [ ] Compiles without errors/warnings?
- [ ] Properly registered in EngineFactory?
- [ ] Compatible with channel strip processing?
- [ ] Works with polyphonic voice allocation?

## Specific Issues to Look For

### Common 7B Model Problems
1. **Incorrect inheritance patterns**
2. **Missing virtual destructors**
3. **Wrong parameter ID mappings**
4. **CPU budget violations**
5. **Memory allocation in audio code**
6. **Missing bounds checking**
7. **Incorrect sample rate handling**

### Optimization Opportunities
1. **Redundant calculations**
2. **Inefficient loops**
3. **Unnecessary function calls**
4. **Better algorithm choices**
5. **SIMD vectorization potential**

## Review Actions
For each issue found:
1. **Document the problem clearly**
2. **Explain why it's problematic**
3. **Provide specific fix recommendations**
4. **Estimate performance impact**
5. **Note any breaking changes needed**