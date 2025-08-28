#include "SynthEngine.h"
#include "SubtractiveEngine.h"
#include "WavetableEngine.h"
#include "FMEngine.h"
#include "GranularEngine.h"
#include <iostream>

float SynthEngine::validateParameter(ParameterID param, float value) const {
    return std::clamp(value, getParameterMin(param), getParameterMax(param));
}

constexpr float SynthEngine::getParameterMin(ParameterID param) {
    switch (param) {
        case ParameterID::HARMONICS:
        case ParameterID::TIMBRE:
        case ParameterID::MORPH:
        case ParameterID::OSC_MIX:
        case ParameterID::FILTER_RESONANCE:
        case ParameterID::ATTACK:
        case ParameterID::DECAY:
        case ParameterID::SUSTAIN:
        case ParameterID::RELEASE:
        case ParameterID::LFO_RATE:
        case ParameterID::LFO_DEPTH:
        case ParameterID::REVERB_SIZE:
        case ParameterID::REVERB_DAMPING:
        case ParameterID::REVERB_MIX:
        case ParameterID::VOLUME:
            return 0.0f;
        case ParameterID::FILTER_CUTOFF:
            return 20.0f;
        case ParameterID::DETUNE:
        case ParameterID::PAN:
            return -1.0f;
        default:
            return 0.0f;
    }
}

constexpr float SynthEngine::getParameterMax(ParameterID param) {
    switch (param) {
        case ParameterID::HARMONICS:
        case ParameterID::TIMBRE:
        case ParameterID::MORPH:
        case ParameterID::OSC_MIX:
        case ParameterID::FILTER_RESONANCE:
        case ParameterID::ATTACK:
        case ParameterID::DECAY:
        case ParameterID::SUSTAIN:
        case ParameterID::RELEASE:
        case ParameterID::LFO_RATE:
        case ParameterID::LFO_DEPTH:
        case ParameterID::REVERB_SIZE:
        case ParameterID::REVERB_DAMPING:
        case ParameterID::REVERB_MIX:
        case ParameterID::VOLUME:
            return 1.0f;
        case ParameterID::FILTER_CUTOFF:
            return 20000.0f;
        case ParameterID::DETUNE:
        case ParameterID::PAN:
            return 1.0f;
        default:
            return 1.0f;
    }
}

constexpr float SynthEngine::getParameterDefault(ParameterID param) {
    switch (param) {
        case ParameterID::SUSTAIN:
        case ParameterID::VOLUME:
            return 0.8f;
        case ParameterID::ATTACK:
            return 0.01f;
        case ParameterID::DECAY:
            return 0.3f;
        case ParameterID::RELEASE:
            return 0.5f;
        case ParameterID::FILTER_CUTOFF:
            return 1000.0f;
        case ParameterID::FILTER_RESONANCE:
            return 0.2f;
        default:
            return 0.5f;
    }
}

std::unique_ptr<SynthEngine> createSynthEngine(EngineType type) {
    switch (type) {
        case EngineType::SUBTRACTIVE:
            return std::make_unique<SubtractiveEngine>();
        case EngineType::WAVETABLE:
            return std::make_unique<WavetableEngine>();
        case EngineType::FM:
            return std::make_unique<FMEngine>();
        case EngineType::PLAITS:
            // return std::make_unique<PlaitsEngine>();
            return std::make_unique<SubtractiveEngine>(); // Fallback for now
        case EngineType::GRANULAR:
            return std::make_unique<GranularEngine>();
        case EngineType::SAMPLER:
            // return std::make_unique<SamplerEngine>();
            return std::make_unique<SubtractiveEngine>(); // Fallback for now
        case EngineType::CHORD_GENERATOR:
            // return std::make_unique<ChordGeneratorEngine>();
            return std::make_unique<SubtractiveEngine>(); // Fallback for now
        default:
            std::cerr << "Unknown engine type, using Subtractive as fallback" << std::endl;
            return std::make_unique<SubtractiveEngine>();
    }
}