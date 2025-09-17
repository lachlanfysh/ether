#include "EtherSynthAudioEngine.h"
#include "../utils/MathUtils.h"
#include "../utils/Constants.h"

namespace GridSequencer {
namespace Audio {

using Utils::clamp01;

EtherSynthAudioEngine::EtherSynthAudioEngine()
    : synth_(nullptr), initialized_(false) {
    LOG_DEBUG("EtherSynthAudioEngine created");
}

EtherSynthAudioEngine::~EtherSynthAudioEngine() {
    shutdown();
    LOG_DEBUG("EtherSynthAudioEngine destroyed");
}

bool EtherSynthAudioEngine::initialize() {
    if (initialized_.load()) {
        LOG_WARNING("Audio engine already initialized");
        return true;
    }

    synth_ = ether_create();
    if (!synth_) {
        LOG_ERROR("Failed to create EtherSynth instance");
        return false;
    }

    int result = ether_initialize(synth_);
    if (result != 0) {
        LOG_ERROR("Failed to initialize EtherSynth: " + std::to_string(result));
        ether_destroy(synth_);
        synth_ = nullptr;
        return false;
    }

    initialized_ = true;
    LOG_INFO("EtherSynth audio engine initialized successfully");
    return true;
}

void EtherSynthAudioEngine::shutdown() {
    if (!initialized_.load()) {
        return;
    }

    if (synth_) {
        ether_shutdown(synth_);
        ether_destroy(synth_);
        synth_ = nullptr;
    }

    initialized_ = false;
    LOG_INFO("EtherSynth audio engine shut down");
}

void EtherSynthAudioEngine::processAudio(float* outputBuffer, size_t bufferSize) {
    if (!initialized_.load() || !synth_) {
        // Fill with silence if not initialized
        for (size_t i = 0; i < bufferSize * 2; ++i) {
            outputBuffer[i] = 0.0f;
        }
        return;
    }

    ether_process_audio(synth_, outputBuffer, bufferSize);
}

void EtherSynthAudioEngine::play() {
    if (synth_) {
        ether_play(synth_);
        LOG_DEBUG("Audio engine play");
    }
}

void EtherSynthAudioEngine::stop() {
    if (synth_) {
        ether_stop(synth_);
        LOG_DEBUG("Audio engine stop");
    }
}

Result<bool> EtherSynthAudioEngine::setInstrumentEngineType(int instrument, int engineType) {
    if (!synth_) {
        return Result<bool>::error("Audio engine not initialized");
    }

    if (!isValidInstrument(instrument)) {
        return Result<bool>::error("Invalid instrument index: " + std::to_string(instrument));
    }

    if (engineType < 0 || engineType >= getEngineTypeCount()) {
        return Result<bool>::error("Invalid engine type: " + std::to_string(engineType));
    }

    ether_set_instrument_engine_type(synth_, instrument, engineType);
    return Result<bool>::success(true);
}

Result<int> EtherSynthAudioEngine::getInstrumentEngineType(int instrument) {
    if (!synth_) {
        return Result<int>::error("Audio engine not initialized");
    }

    if (!isValidInstrument(instrument)) {
        return Result<int>::error("Invalid instrument index: " + std::to_string(instrument));
    }

    int engineType = ether_get_instrument_engine_type(synth_, instrument);
    return Result<int>::success(engineType);
}

Result<std::string> EtherSynthAudioEngine::getEngineTypeName(int engineType) {
    if (engineType < 0 || engineType >= getEngineTypeCount()) {
        return Result<std::string>::error("Invalid engine type: " + std::to_string(engineType));
    }

    const char* name = ether_get_engine_type_name(engineType);
    return Result<std::string>::success(name ? std::string(name) : "Unknown");
}

int EtherSynthAudioEngine::getEngineTypeCount() {
    return ether_get_engine_type_count();
}

Result<bool> EtherSynthAudioEngine::setParameter(int instrument, ParameterID param, float value) {
    if (!synth_) {
        return Result<bool>::error("Audio engine not initialized");
    }

    if (!isValidInstrument(instrument)) {
        return Result<bool>::error("Invalid instrument index: " + std::to_string(instrument));
    }

    if (!isValidParameter(param)) {
        return Result<bool>::error("Invalid parameter");
    }

    // Clamp value to valid range
    float clampedValue = clamp01(value);

    // Route parameter based on support and routing logic
    ParamRoute route = getParameterRoute(instrument, param);
    int paramId = static_cast<int>(param);

    switch (route) {
        case ParamRoute::Engine:
            ether_set_instrument_parameter(synth_, instrument, paramId, clampedValue);
            break;

        case ParamRoute::PostFX:
            // Force filter/dynamics parameters to global FX unit 2
            ether_set_fx_global(synth_, 2, paramId, clampedValue);
            break;

        case ParamRoute::Unsupported:
            return Result<bool>::error("Parameter not supported on this engine");
    }

    return Result<bool>::success(true);
}

Result<float> EtherSynthAudioEngine::getParameter(int instrument, ParameterID param) {
    if (!synth_) {
        return Result<float>::error("Audio engine not initialized");
    }

    if (!isValidInstrument(instrument)) {
        return Result<float>::error("Invalid instrument index: " + std::to_string(instrument));
    }

    if (!isValidParameter(param)) {
        return Result<float>::error("Invalid parameter");
    }

    ParamRoute route = getParameterRoute(instrument, param);
    int paramId = static_cast<int>(param);
    float value = 0.0f;

    switch (route) {
        case ParamRoute::Engine:
            value = ether_get_instrument_parameter(synth_, instrument, paramId);
            break;

        case ParamRoute::PostFX:
            value = ether_get_fx_global(synth_, 2, paramId);
            break;

        case ParamRoute::Unsupported:
            return Result<float>::error("Parameter not supported on this engine");
    }

    return Result<float>::success(value);
}

bool EtherSynthAudioEngine::hasParameter(int instrument, ParameterID param) {
    if (!synth_ || !isValidInstrument(instrument) || !isValidParameter(param)) {
        return false;
    }

    return ether_engine_has_parameter(synth_, instrument, static_cast<int>(param));
}

ParamRoute EtherSynthAudioEngine::getParameterRoute(int instrument, ParameterID param) {
    return resolveParameterRoute(instrument, param);
}

Result<bool> EtherSynthAudioEngine::noteOn(int keyIndex, float velocity, float aftertouch) {
    if (!synth_) {
        return Result<bool>::error("Audio engine not initialized");
    }

    if (keyIndex < 0 || keyIndex > 127) {
        return Result<bool>::error("Invalid MIDI note: " + std::to_string(keyIndex));
    }

    float clampedVelocity = clamp01(velocity);
    float clampedAftertouch = clamp01(aftertouch);

    ether_note_on(synth_, keyIndex, clampedVelocity, clampedAftertouch);
    return Result<bool>::success(true);
}

Result<bool> EtherSynthAudioEngine::noteOff(int keyIndex) {
    if (!synth_) {
        return Result<bool>::error("Audio engine not initialized");
    }

    if (keyIndex < 0 || keyIndex > 127) {
        return Result<bool>::error("Invalid MIDI note: " + std::to_string(keyIndex));
    }

    ether_note_off(synth_, keyIndex);
    return Result<bool>::success(true);
}

void EtherSynthAudioEngine::allNotesOff() {
    if (synth_) {
        ether_all_notes_off(synth_);
    }
}

void EtherSynthAudioEngine::setActiveInstrument(int instrument) {
    if (synth_ && isValidInstrument(instrument)) {
        ether_set_active_instrument(synth_, instrument);
    }
}

int EtherSynthAudioEngine::getActiveInstrument() {
    return synth_ ? ether_get_active_instrument(synth_) : 0;
}

int EtherSynthAudioEngine::getActiveVoiceCount() {
    return synth_ ? ether_get_active_voice_count(synth_) : 0;
}

float EtherSynthAudioEngine::getCPUUsage() {
    return synth_ ? ether_get_cpu_usage(synth_) : 0.0f;
}

float EtherSynthAudioEngine::getMemoryUsageKB() {
    return synth_ ? ether_get_memory_usage_kb(synth_) : 0.0f;
}

void EtherSynthAudioEngine::setMasterVolume(float volume) {
    if (synth_) {
        ether_set_master_volume(synth_, clamp01(volume));
    }
}

float EtherSynthAudioEngine::getMasterVolume() {
    return synth_ ? ether_get_master_volume(synth_) : 0.0f;
}

void EtherSynthAudioEngine::setEngineVoiceCount(int instrument, int voices) {
    if (synth_ && isValidInstrument(instrument)) {
        int clampedVoices = Utils::clamp(voices, 1, 16);
        ether_set_engine_voice_count(synth_, instrument, clampedVoices);
    }
}

int EtherSynthAudioEngine::getEngineVoiceCount(int instrument) {
    return (synth_ && isValidInstrument(instrument)) ?
           ether_get_engine_voice_count(synth_, instrument) : 1;
}

void EtherSynthAudioEngine::setEngineFXSend(int instrument, int which, float value) {
    if (synth_ && isValidInstrument(instrument)) {
        ether_set_engine_fx_send(synth_, instrument, which, clamp01(value));
    }
}

float EtherSynthAudioEngine::getEngineFXSend(int instrument, int which) {
    return (synth_ && isValidInstrument(instrument)) ?
           ether_get_engine_fx_send(synth_, instrument, which) : 0.0f;
}

void EtherSynthAudioEngine::setGlobalFX(int which, int param, float value) {
    if (synth_) {
        ether_set_fx_global(synth_, which, param, clamp01(value));
    }
}

float EtherSynthAudioEngine::getGlobalFX(int which, int param) {
    return synth_ ? ether_get_fx_global(synth_, which, param) : 0.0f;
}

void EtherSynthAudioEngine::setLFORate(unsigned char lfoId, float rate) {
    if (synth_) {
        ether_set_lfo_rate(synth_, lfoId, clamp01(rate));
    }
}

void EtherSynthAudioEngine::setLFODepth(unsigned char lfoId, float depth) {
    if (synth_) {
        ether_set_lfo_depth(synth_, lfoId, clamp01(depth));
    }
}

void EtherSynthAudioEngine::setLFOWaveform(unsigned char lfoId, unsigned char waveform) {
    if (synth_) {
        ether_set_lfo_waveform(synth_, lfoId, waveform);
    }
}

void EtherSynthAudioEngine::triggerInstrumentLFOs(int instrument) {
    if (synth_ && isValidInstrument(instrument)) {
        ether_trigger_instrument_lfos(synth_, instrument);
    }
}

bool EtherSynthAudioEngine::isValidInstrument(int instrument) const {
    return instrument >= 0 && instrument < MAX_ENGINES;
}

bool EtherSynthAudioEngine::isValidParameter(ParameterID param) const {
    // Basic parameter validation - can be expanded
    return static_cast<int>(param) >= 0;
}

ParamRoute EtherSynthAudioEngine::resolveParameterRoute(int instrument, ParameterID param) {
    // Force filter and dynamics parameters to PostFX for consistency across all engines
    int paramId = static_cast<int>(param);
    if (paramId == static_cast<int>(ParameterID::FILTER_CUTOFF) ||
        paramId == static_cast<int>(ParameterID::FILTER_RESONANCE) ||
        paramId == static_cast<int>(ParameterID::HPF) ||
        paramId == static_cast<int>(ParameterID::AMPLITUDE) ||
        paramId == static_cast<int>(ParameterID::CLIP)) {
        return ParamRoute::PostFX;
    }

    // Other parameters: prefer per-engine param if supported
    if (hasParameter(instrument, param)) {
        return ParamRoute::Engine;
    }

    return ParamRoute::Unsupported;
}

} // namespace Audio
} // namespace GridSequencer