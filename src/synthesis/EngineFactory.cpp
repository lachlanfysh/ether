#include "IEngine.h"
#include "engines/MacroVAEngine.h"
#include "engines/MacroFMEngine.h"
#include "engines/MacroWaveshaperEngine.h"
#include "engines/MacroWavetableEngine.h"
#include "engines/MacroChordEngine.h"
#include "engines/MacroHarmonicsEngine.h"
#include "engines/FormantVocalEngine.h"
#include "engines/NoiseParticlesEngine.h"
#include "engines/TidesOscEngine.h"
#include "engines/RingsVoiceEngine.h"
#include "engines/ElementsVoiceEngine.h"
#include "engines/DrumKitEngine.h"
#include "engines/SamplerKitEngine.h"
#include "engines/SamplerSlicerEngine.h"

std::unique_ptr<IEngine> EngineFactory::createEngine(EngineType type) {
    switch (type) {
        case EngineType::MACRO_VA:
            return std::make_unique<MacroVAEngine>();
            
        case EngineType::MACRO_FM:
            return std::make_unique<MacroFMEngine>();
            
        case EngineType::MACRO_WAVESHAPER:
            return std::make_unique<MacroWaveshaperEngine>();
            
        case EngineType::MACRO_WAVETABLE:
            return std::make_unique<MacroWavetableEngine>();
            
        case EngineType::MACRO_CHORD:
            return std::make_unique<MacroChordEngine>();
            
        case EngineType::MACRO_HARMONICS:
            return std::make_unique<MacroHarmonicsEngine>();
            
        case EngineType::FORMANT_VOCAL:
            return std::make_unique<FormantVocalEngine>();
            
        case EngineType::NOISE_PARTICLES:
            return std::make_unique<NoiseParticlesEngine>();
            
        case EngineType::TIDES_OSC:
            return std::make_unique<TidesOscEngine>();
            
        case EngineType::RINGS_VOICE:
            return std::make_unique<RingsVoiceEngine>();
            
        case EngineType::ELEMENTS_VOICE:
            return std::make_unique<ElementsVoiceEngine>();
            
        case EngineType::DRUM_KIT:
            return std::make_unique<DrumKitEngine>();
            
        case EngineType::SAMPLER_KIT:
            return std::make_unique<SamplerKitEngine>();
            
        case EngineType::SAMPLER_SLICER:
            return std::make_unique<SamplerSlicerEngine>();
            
        default:
            return nullptr;
    }
}

const char* EngineFactory::getEngineName(EngineType type) {
    switch (type) {
        case EngineType::MACRO_VA:
            return "MacroVA";
            
        case EngineType::MACRO_FM:
            return "MacroFM";
            
        case EngineType::MACRO_WAVESHAPER:
            return "MacroWaveshaper";
            
        case EngineType::MACRO_WAVETABLE:
            return "MacroWavetable";
            
        case EngineType::MACRO_CHORD:
            return "MacroChord";
            
        case EngineType::MACRO_HARMONICS:
            return "MacroHarmonics";
            
        case EngineType::FORMANT_VOCAL:
            return "FormantVocal";
            
        case EngineType::NOISE_PARTICLES:
            return "NoiseParticles";
            
        case EngineType::TIDES_OSC:
            return "TidesOsc";
            
        case EngineType::RINGS_VOICE:
            return "RingsVoice";
            
        case EngineType::ELEMENTS_VOICE:
            return "ElementsVoice";
            
        case EngineType::DRUM_KIT:
            return "DrumKit";
            
        case EngineType::SAMPLER_KIT:
            return "SamplerKit";
            
        case EngineType::SAMPLER_SLICER:
            return "SamplerSlicer";
            
        default:
            return "Unknown";
    }
}