#include "PerformanceMacros.h"
#include "../core/Logger.h"
#include <algorithm>
#include <cmath>
#include <chrono>

namespace EtherSynth {

PerformanceMacros::PerformanceMacros() {
    Logger::getInstance().log("PerformanceMacros: Initializing performance system");
    
    // Initialize key and knob states
    keyStates_.fill(false);
    knobValues_.fill(0.5f);
    
    // Load factory macros
    loadFactoryMacros();
    
    Logger::getInstance().log("PerformanceMacros: Initialized with " + 
                             std::to_string(macros_.size()) + " factory macros");
}

uint32_t PerformanceMacros::createMacro(const std::string& name, MacroType type, TriggerMode triggerMode) {
    PerformanceMacro macro;
    macro.name = name;
    macro.type = type;
    macro.triggerMode = triggerMode;
    
    uint32_t id = macro.id;
    macros_[id] = macro;
    
    Logger::getInstance().log("PerformanceMacros: Created macro '" + name + "' with ID " + std::to_string(id));
    return id;
}

bool PerformanceMacros::deleteMacro(uint32_t macroId) {
    auto it = macros_.find(macroId);
    if (it == macros_.end()) return false;
    
    // Stop macro if active
    stopMacro(macroId);
    
    // Remove key bindings
    unbindMacroFromKey(macroId);
    
    macros_.erase(it);
    
    Logger::getInstance().log("PerformanceMacros: Deleted macro ID " + std::to_string(macroId));
    return true;
}

PerformanceMacros::PerformanceMacro* PerformanceMacros::getMacro(uint32_t macroId) {
    auto it = macros_.find(macroId);
    return (it != macros_.end()) ? &it->second : nullptr;
}

std::vector<PerformanceMacros::PerformanceMacro> PerformanceMacros::getAllMacros() const {
    std::vector<PerformanceMacro> result;
    for (const auto& pair : macros_) {
        result.push_back(pair.second);
    }
    return result;
}

void PerformanceMacros::executeMacro(uint32_t macroId, float intensity) {
    auto* macro = getMacro(macroId);
    if (!macro || !macro->isEnabled) return;
    
    // Check probability
    if (intensity * macro->probability < (rand() / float(RAND_MAX))) return;
    
    // Check execution limit
    if (macro->maxExecutions > 0 && macro->currentExecutions >= macro->maxExecutions) return;
    
    // Check quantization
    if (macro->quantizeToBar && !isQuantizationPoint()) {
        // Queue for next quantization point
        return;
    }
    
    Logger::getInstance().log("PerformanceMacros: Executing macro '" + macro->name + 
                             "' with intensity " + std::to_string(intensity));
    
    // Execute based on type
    switch (macro->type) {
        case MacroType::PARAMETER_SET:
            executePareameterSetMacro(*macro, intensity);
            break;
        case MacroType::FILTER_SWEEP:
            executeFilterSweepMacro(*macro, intensity);
            break;
        case MacroType::VOLUME_FADE:
            executeVolumeFadeMacro(*macro, intensity);
            break;
        case MacroType::SCENE_MORPH:
            executeSceneMorphMacro(*macro, intensity);
            break;
        case MacroType::LOOP_CAPTURE:
            executeLoopCaptureMacro(*macro, intensity);
            break;
        default:
            Logger::getInstance().log("PerformanceMacros: Unhandled macro type");
    }
    
    // Update state
    macro->isActive = true;
    macro->currentExecutions++;
    macro->progress = 0.0f;
    
    if (macro->duration > 0.0f) {
        activeMacroTimers_[macroId] = macro->duration;
    }
    
    // Update stats
    stats_.macrosExecuted++;
    stats_.macroUsageCount[macroId]++;
}

void PerformanceMacros::stopMacro(uint32_t macroId) {
    auto* macro = getMacro(macroId);
    if (!macro) return;
    
    macro->isActive = false;
    macro->progress = 1.0f;
    activeMacroTimers_.erase(macroId);
    
    Logger::getInstance().log("PerformanceMacros: Stopped macro '" + macro->name + "'");
}

void PerformanceMacros::processMacroUpdates(float deltaTime) {
    std::vector<uint32_t> completedMacros;
    
    for (auto& pair : activeMacroTimers_) {
        uint32_t macroId = pair.first;
        float& remainingTime = pair.second;
        
        auto* macro = getMacro(macroId);
        if (!macro) {
            completedMacros.push_back(macroId);
            continue;
        }
        
        remainingTime -= deltaTime;
        macro->progress = 1.0f - (remainingTime / macro->duration);
        
        if (remainingTime <= 0.0f) {
            macro->isActive = false;
            macro->progress = 1.0f;
            completedMacros.push_back(macroId);
        }
    }
    
    // Clean up completed macros
    for (uint32_t macroId : completedMacros) {
        activeMacroTimers_.erase(macroId);
    }
}

void PerformanceMacros::bindMacroToKey(uint32_t macroId, int keyIndex, bool requiresShift, bool requiresAlt) {
    if (keyIndex < 0 || keyIndex >= 32) return;
    
    auto* macro = getMacro(macroId);
    if (!macro) return;
    
    // Remove existing bindings for this macro
    unbindMacroFromKey(macroId);
    
    // Set new binding
    if (requiresShift) {
        shiftKeyBindings_[keyIndex] = macroId;
    } else if (requiresAlt) {
        altKeyBindings_[keyIndex] = macroId;
    } else {
        keyBindings_[keyIndex] = macroId;
    }
    
    macro->keyIndex = keyIndex;
    macro->requiresShift = requiresShift;
    macro->requiresAlt = requiresAlt;
    
    Logger::getInstance().log("PerformanceMacros: Bound macro '" + macro->name + 
                             "' to key " + std::to_string(keyIndex) +
                             (requiresShift ? " (SHIFT)" : "") +
                             (requiresAlt ? " (ALT)" : ""));
}

void PerformanceMacros::unbindMacroFromKey(uint32_t macroId) {
    auto* macro = getMacro(macroId);
    if (!macro || macro->keyIndex < 0) return;
    
    // Remove from all binding maps
    for (auto it = keyBindings_.begin(); it != keyBindings_.end(); ++it) {
        if (it->second == macroId) {
            keyBindings_.erase(it);
            break;
        }
    }
    for (auto it = shiftKeyBindings_.begin(); it != shiftKeyBindings_.end(); ++it) {
        if (it->second == macroId) {
            shiftKeyBindings_.erase(it);
            break;
        }
    }
    for (auto it = altKeyBindings_.begin(); it != altKeyBindings_.end(); ++it) {
        if (it->second == macroId) {
            altKeyBindings_.erase(it);
            break;
        }
    }
    
    macro->keyIndex = -1;
    macro->requiresShift = false;
    macro->requiresAlt = false;
}

uint32_t PerformanceMacros::getMacroForKey(int keyIndex, bool shiftHeld, bool altHeld) const {
    if (keyIndex < 0 || keyIndex >= 32) return 0;
    
    if (shiftHeld) {
        auto it = shiftKeyBindings_.find(keyIndex);
        return (it != shiftKeyBindings_.end()) ? it->second : 0;
    } else if (altHeld) {
        auto it = altKeyBindings_.find(keyIndex);
        return (it != altKeyBindings_.end()) ? it->second : 0;
    } else {
        auto it = keyBindings_.find(keyIndex);
        return (it != keyBindings_.end()) ? it->second : 0;
    }
}

void PerformanceMacros::processPerformanceKey(int keyIndex, bool pressed, bool shiftHeld, bool altHeld) {
    if (keyIndex < 0 || keyIndex >= 32) return;
    
    keyStates_[keyIndex] = pressed;
    shiftHeld_ = shiftHeld;
    altHeld_ = altHeld;
    
    uint32_t macroId = getMacroForKey(keyIndex, shiftHeld, altHeld);
    if (macroId == 0) return;
    
    auto* macro = getMacro(macroId);
    if (!macro) return;
    
    Logger::getInstance().log("PerformanceMacros: Key " + std::to_string(keyIndex) + 
                             " " + (pressed ? "pressed" : "released") + 
                             " -> macro '" + macro->name + "'");
    
    if (pressed) {
        switch (macro->triggerMode) {
            case TriggerMode::IMMEDIATE:
                executeMacro(macroId);
                break;
            case TriggerMode::QUANTIZED:
                // Queue for quantized execution
                executeMacro(macroId);  // Simplified for now
                break;
            case TriggerMode::HOLD:
                macroHoldStates_[macroId] = true;
                executeMacro(macroId);
                break;
            case TriggerMode::TOGGLE:
                if (macro->isActive) {
                    stopMacro(macroId);
                } else {
                    executeMacro(macroId);
                }
                break;
        }
    } else {  // Key released
        if (macro->triggerMode == TriggerMode::HOLD) {
            macroHoldStates_[macroId] = false;
            stopMacro(macroId);
        }
    }
    
    // Update performance stats
    static auto lastKeyPress = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    auto timeSince = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastKeyPress);
    
    if (timeSince.count() < 1000) {  // Within last second
        stats_.keyPressesPerMinute++;
    }
    lastKeyPress = now;
}

uint32_t PerformanceMacros::captureScene(const std::string& name) {
    SceneSnapshot scene;
    scene.name = name;
    
    // Capture current synth state
    // This would interface with the actual synth state
    Logger::getInstance().log("PerformanceMacros: Capturing scene '" + name + "'");
    
    uint32_t id = scene.id;
    scenes_[id] = scene;
    
    stats_.scenesRecalled++;
    return id;
}

bool PerformanceMacros::recallScene(uint32_t sceneId, float morphTime) {
    auto it = scenes_.find(sceneId);
    if (it == scenes_.end()) return false;
    
    const SceneSnapshot& scene = it->second;
    
    Logger::getInstance().log("PerformanceMacros: Recalling scene '" + scene.name + 
                             "' with morph time " + std::to_string(morphTime));
    
    if (morphTime <= 0.0f) {
        // Immediate recall
        applySceneParameters(scene);
    } else {
        // Start morphing
        morphingActive_ = true;
        morphToScene_ = sceneId;
        morphDuration_ = morphTime;
        morphProgress_ = 0.0f;
    }
    
    return true;
}

void PerformanceMacros::loadFactoryMacros() {
    // Create factory performance macros
    
    // Filter sweep macro
    auto filterSweepId = createMacro("Filter Sweep", MacroType::FILTER_SWEEP, TriggerMode::IMMEDIATE);
    auto* filterMacro = getMacro(filterSweepId);
    if (filterMacro) {
        filterMacro->duration = 2.0f;
        filterMacro->parameters["startCutoff"] = 100.0f;
        filterMacro->parameters["endCutoff"] = 8000.0f;
        filterMacro->color = 0xFF6B6B;
        filterMacro->category = "Effects";
    }
    
    // Volume swell macro
    auto volumeSwellId = createMacro("Volume Swell", MacroType::VOLUME_FADE, TriggerMode::HOLD);
    auto* volumeMacro = getMacro(volumeSwellId);
    if (volumeMacro) {
        volumeMacro->duration = 1.0f;
        volumeMacro->parameters["targetVolume"] = 1.0f;
        volumeMacro->fadeTime = 0.5f;
        volumeMacro->color = 0x4ECDC4;
        volumeMacro->category = "Mixing";
    }
    
    // Tempo halftime macro
    auto halftimeId = createMacro("Halftime", MacroType::TEMPO_RAMP, TriggerMode::TOGGLE);
    auto* halftimeMacro = getMacro(halftimeId);
    if (halftimeMacro) {
        halftimeMacro->parameters["tempoMultiplier"] = 0.5f;
        halftimeMacro->duration = 0.1f;
        halftimeMacro->color = 0xFFE66D;
        halftimeMacro->category = "Timing";
    }
    
    // Reverb throw macro
    auto reverbThrowId = createMacro("Reverb Throw", MacroType::EFFECT_CHAIN, TriggerMode::IMMEDIATE);
    auto* reverbMacro = getMacro(reverbThrowId);
    if (reverbMacro) {
        reverbMacro->duration = 4.0f;
        reverbMacro->parameters["reverbSend"] = 1.0f;
        reverbMacro->parameters["reverbDecay"] = 8.0f;
        reverbMacro->color = 0xA8E6CF;
        reverbMacro->category = "Effects";
    }
    
    Logger::getInstance().log("PerformanceMacros: Loaded " + std::to_string(macros_.size()) + " factory macros");
}

void PerformanceMacros::executePareameterSetMacro(const PerformanceMacro& macro, float intensity) {
    Logger::getInstance().log("PerformanceMacros: Executing parameter set macro");
    
    for (const auto& param : macro.parameters) {
        float scaledValue = param.second * intensity;
        Logger::getInstance().log("  Setting " + param.first + " = " + std::to_string(scaledValue));
        
        // This would interface with the actual synth parameter system
        // synthEngine->setParameter(param.first, scaledValue);
    }
}

void PerformanceMacros::executeFilterSweepMacro(const PerformanceMacro& macro, float intensity) {
    Logger::getInstance().log("PerformanceMacros: Executing filter sweep macro");
    
    auto startIt = macro.parameters.find("startCutoff");
    auto endIt = macro.parameters.find("endCutoff");
    
    if (startIt != macro.parameters.end() && endIt != macro.parameters.end()) {
        float startCutoff = startIt->second;
        float endCutoff = endIt->second;
        float currentCutoff = startCutoff + (endCutoff - startCutoff) * macro.progress;
        
        Logger::getInstance().log("  Filter cutoff: " + std::to_string(currentCutoff));
        
        // This would interface with the actual filter
        // synthEngine->setFilterCutoff(currentCutoff * intensity);
    }
}

void PerformanceMacros::executeVolumeFadeMacro(const PerformanceMacro& macro, float intensity) {
    Logger::getInstance().log("PerformanceMacros: Executing volume fade macro");
    
    auto targetIt = macro.parameters.find("targetVolume");
    if (targetIt != macro.parameters.end()) {
        float targetVolume = targetIt->second * intensity;
        
        // Apply fade curve
        float fadeProgress = std::min(1.0f, macro.progress / macro.fadeTime);
        float currentVolume = fadeProgress * targetVolume;
        
        Logger::getInstance().log("  Volume: " + std::to_string(currentVolume));
        
        // This would interface with the actual mixer
        // synthEngine->setMasterVolume(currentVolume);
    }
}

void PerformanceMacros::executeSceneMorphMacro(const PerformanceMacro& macro, float intensity) {
    Logger::getInstance().log("PerformanceMacros: Executing scene morph macro");
    
    // This would trigger scene morphing
    // Implementation would depend on scene management system
}

void PerformanceMacros::executeLoopCaptureMacro(const PerformanceMacro& macro, float intensity) {
    Logger::getInstance().log("PerformanceMacros: Executing loop capture macro");
    
    // This would interface with the live looping system
    // Implementation would depend on audio recording capabilities
}

bool PerformanceMacros::isQuantizationPoint() const {
    // Simplified quantization check
    // In real implementation, this would check against the current transport
    return true;  // Always allow for now
}

void PerformanceMacros::applySceneParameters(const SceneSnapshot& scene, float weight) {
    Logger::getInstance().log("PerformanceMacros: Applying scene parameters with weight " + std::to_string(weight));
    
    // This would apply all scene parameters to the synth
    // Implementation would depend on the actual parameter system
}

} // namespace EtherSynth