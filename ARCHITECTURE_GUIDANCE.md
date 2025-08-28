# Synthesis Engine Architecture Guidance

## Overview
This document captures the exact patterns and requirements for implementing synthesis engines in our system. It will be continuously updated based on implementation feedback to improve 32B model conformance.

## Core Architecture Pattern

### File Structure
```cpp
#pragma once
#include "../BaseEngine.h"
#include "../DSPUtils.h"
// Additional includes as needed (no EngineFactory.h - creates circular dependency)
```

### Voice Class Pattern
```cpp
class [EngineName]Voice : public BaseVoice {
public:
    [EngineName]Voice() { /* initialization */ }
    
    void setSampleRate(float sampleRate) override {
        BaseVoice::setSampleRate(sampleRate);
        // Initialize DSP components with sample rate
    }
    
    void noteOn(float note, float velocity) override {
        BaseVoice::noteOn(note, velocity);
        // Setup note-specific parameters
    }
    
    float renderSample(const RenderContext& ctx) override {
        if (!active_) return 0.0f;
        
        float envelope = ampEnv_.process();
        if (envelope <= 0.001f && releasing_) {
            active_ = false;
            return 0.0f;
        }
        
        // Generate audio sample
        float sample = /* synthesis code */;
        
        // Apply envelope and velocity
        sample *= envelope * velocity_;
        
        // Apply channel strip if present
        if (channelStrip_) {
            sample = channelStrip_->process(sample, note_);
        }
        
        return sample;
    }
    
    void renderBlock(const RenderContext& ctx, float* output, int count) override {
        for (int i = 0; i < count; ++i) {
            output[i] = renderSample(ctx);
        }
    }
    
    uint32_t getAge() const { return 0; }  // TODO: Implement proper age tracking
    
    // Parameter setter methods
    void setParameterName(float value) { /* implementation */ }
    
private:
    // Voice-specific state and DSP components
};
```

### Engine Class Pattern
```cpp
class [EngineName]Engine : public PolyphonicBaseEngine<[EngineName]Voice> {
public:
    [EngineName]Engine() 
        : PolyphonicBaseEngine("EngineName", "SHORT", 
                              static_cast<int>(EngineFactory::EngineType::ENGINE_TYPE),
                              CPUClass::Light, maxVoices) {}
    
    // IEngine interface
    const char* getName() const override { return "EngineName"; }
    const char* getShortName() const override { return "SHORT"; }
    
    void setParam(int paramID, float v01) override {
        BaseEngine::setParam(paramID, v01);  // Handle common parameters
        
        switch (static_cast<EngineParamID>(paramID)) {
            case EngineParamID::HARMONICS:
                // Map to primary synthesis parameter
                for (auto& voice : voices_) {
                    voice->setParameter(v01);
                }
                break;
                
            case EngineParamID::TIMBRE:
                // Map to secondary parameter
                break;
                
            case EngineParamID::MORPH:
                // Map to morphing/modulation parameter
                break;
                
            default:
                break;
        }
    }
    
    // Parameter metadata
    int getParameterCount() const override { return N; }
    
    const ParameterInfo* getParameterInfo(int index) const override {
        static const ParameterInfo params[] = {
            {static_cast<int>(EngineParamID::HARMONICS), "Name", "unit", default, 0.0f, 1.0f, discrete, steps, "group"},
            // ... more parameters
        };
        return (index >= 0 && index < N) ? &params[index] : nullptr;
    }
    
private:
    // Engine-level parameters
    float engineParam_ = defaultValue;
};
```

## Parameter Mapping Standards

### Hero Macros (Always Map These)
- **HARMONICS**: Primary synthesis characteristic (harmonic content, wave shape, density, etc.)
- **TIMBRE**: Secondary shaping parameter (filter, bandwidth, formants, etc.)  
- **MORPH**: Morphing/modulation parameter (blend, randomization, movement, etc.)

### Engine-Specific Parameters
Use the predefined EngineParamID enum values for engine-specific controls:
- Each engine has a range in the enum (e.g., ENGINE_SPECIFIC_START + 40 for MacroChord)
- Always include LPF_CUTOFF and DRIVE for channel strip integration

### Parameter Metadata Format
```cpp
{paramID, "DisplayName", "unit", defaultValue, minValue, maxValue, isDiscrete, stepCount, "groupName"}
```

## Common Mistakes to Avoid

### ❌ Wrong Includes
```cpp
#include "PolyphonicBaseEngine.h"  // Wrong - this doesn't exist
#include "EngineFactory.h"         // Wrong - creates circular dependency
```

### ✅ Correct Includes
```cpp
#include "../BaseEngine.h"    // Correct - provides PolyphonicBaseEngine
#include "../DSPUtils.h"      // Correct - provides DSP utilities
```

### ❌ Wrong Method Signatures
```cpp
void process(float* output, int numSamples);  // Wrong - not our interface
void setParameter(int index, float value);   // Wrong - not our interface
```

### ✅ Correct Method Signatures
```cpp
float renderSample(const RenderContext& ctx) override;
void setParam(int paramID, float v01) override;
```

### ❌ Wrong Parameter Handling
```cpp
enum Parameters { HARMONICS, TIMBRE, MORPH };  // Wrong - creates new enum
```

### ✅ Correct Parameter Handling
```cpp
switch (static_cast<EngineParamID>(paramID)) {
    case EngineParamID::HARMONICS:
        // Use existing enum from IEngine.h
```

### ❌ Missing Essential Methods
- No setSampleRate() override
- No noteOn() override  
- No renderBlock() implementation
- No parameter metadata

### ✅ Required Methods
All voice classes must implement the full BaseVoice interface and all engine classes must implement the full parameter metadata system.

## DSP Implementation Guidelines

### Random Number Generation
```cpp
DSP::Random random_;
random_.setSeed(reinterpret_cast<uintptr_t>(this) + someOffset);
float value = random_.uniform(-1.0f, 1.0f);
```

### Envelope Usage
```cpp
float envelope = ampEnv_.process();  // Inherited from BaseVoice
```

### Real-Time Safety
- No heap allocation in audio thread
- Use pre-allocated buffers
- Clamp all parameter values
- Handle edge cases gracefully

### CPU Classes
- **Light**: Simple synthesis (≤2k cycles)
- **Medium**: Moderate complexity (≤4k cycles)  
- **Heavy**: Complex synthesis (≤8k cycles)
- **VeryHeavy**: Physical modeling (≤16k cycles)

## Update History

### v1.0 (Initial) - Aug 21, 2025
- Created based on analysis of 32B model implementation patterns
- Documented core architecture from successful engines (MacroVA through NoiseParticles)
- Identified common mistakes and correction patterns

### v1.1 Updates (ElementsVoice feedback) - Aug 21, 2025
- **Critical**: renderSample() signature must be `float renderSample(const RenderContext& ctx) override`
- **Never use**: Non-existent DSPUtils methods like `DSPUtils::sineWave()` or `DSPUtils::oscillator()`
- **Required**: All engines must implement complete setParam() with EngineParamID switch statement
- **Required**: All engines must include ParameterInfo metadata array
- **Pattern**: Use only confirmed DSP utilities from our codebase (DSP::Random, DSP::ADSR, etc.)
- **Success**: ElementsVoice implementation shows proper physical modeling architecture with Karplus-Strong synthesis

### v1.2 Updates (SamplerKit feedback) - Aug 21, 2025
- **NEVER use JUCE patterns**: No SynthesiserVoice, SynthesiserSound, AudioBuffer<float>, startNote(), stopNote(), renderNextBlock()
- **ALWAYS use our BaseVoice methods**: noteOn(float note, float velocity), renderSample(const RenderContext& ctx), setSampleRate()
- **NEVER create custom enums**: Don't create new EngineParamID enums - use the existing one from IEngine.h
- **ALWAYS use exact includes**: `#include "../BaseEngine.h"` and `#include "../DSPUtils.h"` - never BaseVoice.h or PolyphonicBaseEngine.h
- **Namespace naming**: Use PascalCase (SamplerKit::) not lowercase (samplerkit::)
- **Parameter structure**: Keep hero macros simple (3-8 total parameters), don't create 96-parameter monsters
- **Integration patterns**: Use shared infrastructure classes (Sample::SampleBuffer, DSP::ADSR) - don't invent new ones
- **Architecture guidance is MANDATORY**: When guidance document is referenced, follow it exactly - no deviations or "improvements"

### Common 32B Model Errors to Avoid:
1. **Framework confusion**: Mixing JUCE patterns with our BaseEngine architecture
2. **Method signature drift**: Using generic audio framework methods instead of our specific signatures  
3. **Over-parameterization**: Creating hundreds of parameters instead of focusing on hero macros
4. **Custom type creation**: Inventing new enums/classes instead of using existing infrastructure
5. **Include path errors**: Wrong relative paths or non-existent header files
6. **Namespace inconsistency**: Using different naming conventions than established patterns

### Success Pattern Template:
```cpp
#pragma once
#include "../BaseEngine.h"
#include "../DSPUtils.h"
// Include any shared infrastructure classes

class [Name]Voice : public BaseVoice {
    void setSampleRate(float sampleRate) override;
    void noteOn(float note, float velocity) override;
    float renderSample(const RenderContext& ctx) override;
    void renderBlock(const RenderContext& ctx, float* output, int count) override;
    uint32_t getAge() const;
    // Parameter setters for voice-level controls
};

class [Name]Engine : public PolyphonicBaseEngine<[Name]Voice> {
    const char* getName() const override;
    const char* getShortName() const override;
    void setParam(int paramID, float v01) override;
    int getParameterCount() const override;
    const ParameterInfo* getParameterInfo(int index) const override;
};
```

## Usage Instructions

When prompting the 32B model, always include:
1. Reference to this guidance document
2. Specific examples from working engines
3. Exact method signatures required
4. Clear parameter mapping requirements

This should significantly improve conformance and reduce rework cycles.