#include "ParameterSystem.h"
#include "../utils/MathUtils.h"
#include "../utils/Logger.h"
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace GridSequencer {
namespace Parameter {

using Utils::clamp;
using Utils::clamp01;

ParameterSystem::ParameterSystem(std::shared_ptr<IAudioEngine> audioEngine)
    : audioEngine_(audioEngine) {
    initializeParameterNames();
    LOG_INFO("ParameterSystem initialized");
}

void ParameterSystem::initializeParameterNames() {
    parameterNames_ = {
        {static_cast<int>(ParameterID::HARMONICS), "harmonics"},
        {static_cast<int>(ParameterID::TIMBRE), "timbre"},
        {static_cast<int>(ParameterID::MORPH), "morph"},
        {static_cast<int>(ParameterID::OSC_MIX), "oscmix"},
        {static_cast<int>(ParameterID::DETUNE), "detune"},
        {static_cast<int>(ParameterID::SUB_LEVEL), "sublevel"},
        {static_cast<int>(ParameterID::SUB_ANCHOR), "subanchor"},
        {static_cast<int>(ParameterID::FILTER_CUTOFF), "lpf"},
        {static_cast<int>(ParameterID::FILTER_RESONANCE), "resonance"},
        {static_cast<int>(ParameterID::ATTACK), "attack"},
        {static_cast<int>(ParameterID::DECAY), "decay"},
        {static_cast<int>(ParameterID::SUSTAIN), "sustain"},
        {static_cast<int>(ParameterID::RELEASE), "release"},
        {static_cast<int>(ParameterID::REVERB_SIZE), "reverb_size"},
        {static_cast<int>(ParameterID::REVERB_DAMPING), "reverb_damp"},
        {static_cast<int>(ParameterID::REVERB_MIX), "reverb_mix"},
        {static_cast<int>(ParameterID::DELAY_TIME), "delay_time"},
        {static_cast<int>(ParameterID::DELAY_FEEDBACK), "delay_fb"},
        {static_cast<int>(ParameterID::VOLUME), "volume"},
        {static_cast<int>(ParameterID::PAN), "pan"},
        {static_cast<int>(ParameterID::HPF), "hpf"},
        {static_cast<int>(ParameterID::ACCENT_AMOUNT), "accent"},
        {static_cast<int>(ParameterID::GLIDE_TIME), "glide"},
        {static_cast<int>(ParameterID::AMPLITUDE), "amp"},
        {static_cast<int>(ParameterID::CLIP), "clip"},
        // Pseudo-parameters
        {PSEUDO_PARAM_OCTAVE, "octave"},
        {PSEUDO_PARAM_PITCH, "pitch"}
    };
}

Result<bool> ParameterSystem::setParameter(int engine, ParameterID param, float value) {
    if (!isValidEngine(engine)) {
        return Result<bool>::error("Invalid engine index: " + std::to_string(engine));
    }

    if (isPseudoParameter(static_cast<int>(param))) {
        return setPseudoParameter(static_cast<int>(param), value);
    }

    // Clamp value to valid range
    float clampedValue = clampParameterValue(param, value);

    // Update cache first (cache-first pattern)
    updateCache(engine, param, clampedValue);

    // Then sync to audio engine
    auto result = audioEngine_->setParameter(engine, param, clampedValue);
    if (result.isError()) {
        LOG_ERROR("Failed to set parameter: " + result.error());
        return Result<bool>::error(result.error());
    }

    return Result<bool>::success(true);
}

Result<float> ParameterSystem::getParameter(int engine, ParameterID param) {
    if (!isValidEngine(engine)) {
        return Result<float>::error("Invalid engine index: " + std::to_string(engine));
    }

    if (isPseudoParameter(static_cast<int>(param))) {
        return getPseudoParameter(static_cast<int>(param));
    }

    // Always read from cache for UI consistency
    int paramId = static_cast<int>(param);
    auto it = parameterCache_[engine].find(paramId);
    if (it != parameterCache_[engine].end()) {
        return Result<float>::success(it->second);
    }

    // If not in cache, try to get from engine
    auto result = audioEngine_->getParameter(engine, param);
    if (result.isSuccess()) {
        updateCache(engine, param, result.value());
        return result;
    }

    return Result<float>::error("Parameter not found in cache or engine");
}

Result<bool> ParameterSystem::adjustParameter(int engine, ParameterID param, bool increment, bool useShift) {
    if (!isValidEngine(engine)) {
        return Result<bool>::error("Invalid engine index: " + std::to_string(engine));
    }

    // Handle pseudo-parameters
    if (isPseudoParameter(static_cast<int>(param))) {
        if (static_cast<int>(param) == PSEUDO_PARAM_OCTAVE) {
            int current = octaveOffset_.load();
            int newValue = increment ?
                          std::min(OCTAVE_MAX, current + 1) :
                          std::max(OCTAVE_MIN, current - 1);
            octaveOffset_ = newValue;
            return Result<bool>::success(true);
        } else if (static_cast<int>(param) == PSEUDO_PARAM_PITCH) {
            float current = pitchOffset_.load();
            float delta = increment ? 0.5f : -0.5f;
            float newValue = clamp(current + delta, PITCH_MIN, PITCH_MAX);
            pitchOffset_ = newValue;
            return Result<bool>::success(true);
        }
    }

    // Get current value from cache
    auto currentResult = getParameter(engine, param);
    if (currentResult.isError()) {
        return Result<bool>::error("Failed to get current parameter value: " + currentResult.error());
    }

    float currentValue = currentResult.value();

    // Special handling for FM algorithm (TIMBRE parameter)
    if (param == ParameterID::TIMBRE && isFMEngine(engine)) {
        int algo = static_cast<int>(std::floor(currentValue * 8.0f + 1e-6));
        if (increment) {
            algo = std::min(7, algo + 1);
        } else {
            algo = std::max(0, algo - 1);
        }
        float newValue = (algo + 0.5f) / 8.0f;
        return setParameter(engine, param, newValue);
    }

    // Normal parameter adjustment
    float delta = getParameterDelta(param, useShift);
    if (!increment) {
        delta = -delta;
    }

    float newValue = clampParameterValue(param, currentValue + delta);
    return setParameter(engine, param, newValue);
}

ParamRoute ParameterSystem::getParameterRoute(int engine, ParameterID param) {
    if (!isValidEngine(engine)) {
        return ParamRoute::Unsupported;
    }

    return audioEngine_->getParameterRoute(engine, param);
}

bool ParameterSystem::isParameterSupported(int engine, ParameterID param) {
    if (!isValidEngine(engine)) {
        return false;
    }

    ParamRoute route = getParameterRoute(engine, param);
    return route != ParamRoute::Unsupported;
}

std::string ParameterSystem::getRouteDisplayTag(ParamRoute route) {
    switch (route) {
        case ParamRoute::Engine:  return "[E]";
        case ParamRoute::PostFX:  return "[FX]";
        default:                  return "[—]";
    }
}

std::string ParameterSystem::getParameterName(ParameterID param) {
    int paramId = static_cast<int>(param);
    auto it = parameterNames_.find(paramId);
    return (it != parameterNames_.end()) ? it->second : "unknown";
}

std::string ParameterSystem::getParameterDisplayValue(int engine, ParameterID param) {
    if (isPseudoParameter(static_cast<int>(param))) {
        if (static_cast<int>(param) == PSEUDO_PARAM_OCTAVE) {
            int octave = octaveOffset_.load();
            return (octave >= 0 ? "+" : "") + std::to_string(octave);
        } else if (static_cast<int>(param) == PSEUDO_PARAM_PITCH) {
            float pitch = pitchOffset_.load();
            std::stringstream ss;
            ss << std::fixed << std::setprecision(1);
            if (pitch >= 0) ss << "+";
            ss << pitch << " st";
            return ss.str();
        }
    }

    auto result = getParameter(engine, param);
    if (result.isError()) {
        return "err";
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << result.value();
    return ss.str();
}

float ParameterSystem::getParameterDisplayNormalized(int engine, ParameterID param) {
    auto result = getParameter(engine, param);
    return result.isSuccess() ? result.value() : 0.0f;
}

std::vector<int> ParameterSystem::getVisibleParameters(int engine) {
    // Basic parameter set - can be customized based on engine type
    return {
        static_cast<int>(ParameterID::HARMONICS),
        static_cast<int>(ParameterID::TIMBRE),
        static_cast<int>(ParameterID::MORPH),
        static_cast<int>(ParameterID::FILTER_CUTOFF),
        static_cast<int>(ParameterID::FILTER_RESONANCE),
        static_cast<int>(ParameterID::ATTACK),
        static_cast<int>(ParameterID::DECAY),
        static_cast<int>(ParameterID::SUSTAIN),
        static_cast<int>(ParameterID::RELEASE),
        static_cast<int>(ParameterID::VOLUME),
        static_cast<int>(ParameterID::PAN)
    };
}

std::vector<int> ParameterSystem::getExtendedParameters(int engine) {
    auto visible = getVisibleParameters(engine);

    // Add pseudo-parameters
    visible.push_back(PSEUDO_PARAM_OCTAVE);
    visible.push_back(PSEUDO_PARAM_PITCH);

    // Add additional parameters
    visible.insert(visible.end(), {
        static_cast<int>(ParameterID::HPF),
        static_cast<int>(ParameterID::AMPLITUDE),
        static_cast<int>(ParameterID::CLIP),
        static_cast<int>(ParameterID::REVERB_MIX)
    });

    return visible;
}

bool ParameterSystem::isValidParameterValue(ParameterID param, float value) {
    if (isPseudoParameter(static_cast<int>(param))) {
        return true; // Pseudo-parameters have their own validation
    }

    return value >= PARAM_MIN && value <= PARAM_MAX;
}

float ParameterSystem::clampParameterValue(ParameterID param, float value) {
    if (isPseudoParameter(static_cast<int>(param))) {
        return value; // Pseudo-parameters handle their own clamping
    }

    return clamp01(value);
}

void ParameterSystem::syncCacheToEngine(int engine) {
    if (!isValidEngine(engine)) {
        return;
    }

    for (const auto& [paramId, value] : parameterCache_[engine]) {
        auto param = static_cast<ParameterID>(paramId);
        audioEngine_->setParameter(engine, param, value);
    }

    LOG_DEBUG("Synced cache to engine " + std::to_string(engine));
}

void ParameterSystem::syncEngineToCache(int engine) {
    if (!isValidEngine(engine)) {
        return;
    }

    // Sync common parameters from engine to cache
    std::vector<ParameterID> commonParams = {
        ParameterID::HARMONICS, ParameterID::TIMBRE, ParameterID::MORPH,
        ParameterID::FILTER_CUTOFF, ParameterID::FILTER_RESONANCE,
        ParameterID::ATTACK, ParameterID::DECAY, ParameterID::SUSTAIN, ParameterID::RELEASE,
        ParameterID::VOLUME, ParameterID::PAN, ParameterID::HPF,
        ParameterID::AMPLITUDE, ParameterID::CLIP
    };

    for (auto param : commonParams) {
        auto result = audioEngine_->getParameter(engine, param);
        if (result.isSuccess()) {
            updateCache(engine, param, result.value());
        }
    }

    LOG_DEBUG("Synced engine to cache " + std::to_string(engine));
}

void ParameterSystem::clearCache() {
    for (auto& engineCache : parameterCache_) {
        engineCache.clear();
    }
    LOG_DEBUG("Parameter cache cleared");
}

void ParameterSystem::initializeDefaults(int engine) {
    if (!isValidEngine(engine)) {
        return;
    }

    // Set conservative defaults to avoid long sustains
    parameterCache_[engine][static_cast<int>(ParameterID::ATTACK)] = 0.10f;
    parameterCache_[engine][static_cast<int>(ParameterID::DECAY)] = 0.10f;
    parameterCache_[engine][static_cast<int>(ParameterID::SUSTAIN)] = 0.10f;
    parameterCache_[engine][static_cast<int>(ParameterID::RELEASE)] = 0.10f;
    parameterCache_[engine][static_cast<int>(ParameterID::FILTER_CUTOFF)] = 0.8f;
    parameterCache_[engine][static_cast<int>(ParameterID::FILTER_RESONANCE)] = 0.2f;
    parameterCache_[engine][static_cast<int>(ParameterID::VOLUME)] = 0.8f;
    parameterCache_[engine][static_cast<int>(ParameterID::PAN)] = 0.5f;
    parameterCache_[engine][static_cast<int>(ParameterID::REVERB_MIX)] = 0.3f;

    // Sync defaults to engine
    syncCacheToEngine(engine);

    LOG_DEBUG("Initialized defaults for engine " + std::to_string(engine));
}

Result<bool> ParameterSystem::setPseudoParameter(int paramId, float value) {
    if (paramId == PSEUDO_PARAM_OCTAVE) {
        int octave = static_cast<int>(value);
        octaveOffset_ = clamp(octave, OCTAVE_MIN, OCTAVE_MAX);
        return Result<bool>::success(true);
    } else if (paramId == PSEUDO_PARAM_PITCH) {
        pitchOffset_ = clamp(value, PITCH_MIN, PITCH_MAX);
        return Result<bool>::success(true);
    }

    return Result<bool>::error("Unknown pseudo-parameter: " + std::to_string(paramId));
}

Result<float> ParameterSystem::getPseudoParameter(int paramId) {
    if (paramId == PSEUDO_PARAM_OCTAVE) {
        return Result<float>::success(static_cast<float>(octaveOffset_.load()));
    } else if (paramId == PSEUDO_PARAM_PITCH) {
        return Result<float>::success(pitchOffset_.load());
    }

    return Result<float>::error("Unknown pseudo-parameter: " + std::to_string(paramId));
}

bool ParameterSystem::isPseudoParameter(int paramId) {
    return paramId == PSEUDO_PARAM_OCTAVE || paramId == PSEUDO_PARAM_PITCH;
}

int ParameterSystem::getFMAlgorithm(int engine) {
    if (!isFMEngine(engine)) {
        return 0;
    }

    auto result = getParameter(engine, ParameterID::TIMBRE);
    if (result.isError()) {
        return 0;
    }

    return static_cast<int>(std::floor(result.value() * 8.0f + 1e-6));
}

Result<bool> ParameterSystem::setFMAlgorithm(int engine, int algorithm) {
    if (!isFMEngine(engine)) {
        return Result<bool>::error("Engine is not FM type");
    }

    int clampedAlgo = clamp(algorithm, 0, 7);
    float timbreValue = (clampedAlgo + 0.5f) / 8.0f;

    return setParameter(engine, ParameterID::TIMBRE, timbreValue);
}

bool ParameterSystem::isFMEngine(int engine) {
    if (!isValidEngine(engine)) {
        return false;
    }

    auto result = audioEngine_->getInstrumentEngineType(engine);
    if (result.isError()) {
        return false;
    }

    auto nameResult = audioEngine_->getEngineTypeName(result.value());
    if (nameResult.isError()) {
        return false;
    }

    return nameResult.value().find("FM") != std::string::npos;
}

bool ParameterSystem::isValidEngine(int engine) const {
    return engine >= 0 && engine < MAX_ENGINES;
}

float ParameterSystem::getParameterDelta(ParameterID param, bool useShift) const {
    float delta = 0.02f; // default

    switch (param) {
        case ParameterID::FILTER_CUTOFF:
        case ParameterID::HPF:
            // Small smooth steps; SHIFT speeds up
            delta = useShift ? 0.05f : 0.01f;
            break;
        case ParameterID::FILTER_RESONANCE:
            delta = 0.01f;
            break;
        case ParameterID::AMPLITUDE:
        case ParameterID::CLIP:
            delta = 0.02f;
            break;
        default:
            delta = 0.02f;
            break;
    }

    return delta;
}

void ParameterSystem::updateCache(int engine, ParameterID param, float value) {
    if (isValidEngine(engine)) {
        parameterCache_[engine][static_cast<int>(param)] = value;
    }
}

} // namespace Parameter
} // namespace GridSequencer