#include "GlobalLFOSystem.h"
#include <cmath>
#include <random>
#include <algorithm>

// Static clock division array definition
constexpr float GlobalLFOSystem::CLOCK_DIVISIONS[];

GlobalLFOSystem::GlobalLFOSystem() {
    // Initialize all assignments as empty
    for (auto& slotAssignments : assignments_) {
        for (auto& assignment : slotAssignments) {
            assignment.mask = 0;
            assignment.depths.fill(0.0f);
        }
    }
    
    // Initialize all LFO states
    for (auto& slotLFOs : lfoStates_) {
        for (auto& lfo : slotLFOs) {
            lfo.phase = 0.0f;
            lfo.lastValue = 0.0f;
            lfo.active = true;
            lfo.envPhase = LFOState::ENV_IDLE;
            lfo.envLevel = 0.0f;
            lfo.holdValue = 0.0f;
            lfo.noiseTarget = 0.0f;
            lfo.noiseSmooth = 0.0f;
        }
    }
}

void GlobalLFOSystem::init(float sampleRate, float bpm) {
    sampleRate_ = sampleRate;
    bpm_ = bpm;
}

void GlobalLFOSystem::setBPM(float bpm) {
    bpm_ = std::clamp(bpm, 60.0f, 200.0f);
}

void GlobalLFOSystem::setSampleRate(float sampleRate) {
    sampleRate_ = std::max(8000.0f, sampleRate);
}

void GlobalLFOSystem::setLFO(int slot, int idx, const LFOSettings& settings) {
    if (!isValidSlot(slot) || !isValidLFO(idx)) return;
    
    auto& lfo = lfoStates_[slot][idx];
    lfo.settings = settings;
    lfo.settings.depth = clamp(settings.depth, 0.0f, 1.0f);
    lfo.settings.pulseWidth = clamp(settings.pulseWidth, 0.1f, 0.9f);
    
    // Reset LFO state when settings change
    if (settings.sync == SyncMode::Envelope) {
        lfo.envPhase = LFOState::ENV_IDLE;
        lfo.envLevel = 0.0f;
    } else {
        lfo.phase = 0.0f;
    }
    
    lfo.active = (settings.depth > 0.001f);
}

void GlobalLFOSystem::getLFO(int slot, int idx, LFOSettings& settings) const {
    if (!isValidSlot(slot) || !isValidLFO(idx)) return;
    settings = lfoStates_[slot][idx].settings;
}

void GlobalLFOSystem::assign(int slot, int paramId, int idx, float depth) {
    if (!isValidSlot(slot) || !isValidParam(paramId) || !isValidLFO(idx)) return;
    
    auto& assignment = assignments_[slot][paramId];
    assignment.mask |= (1 << idx);
    assignment.depths[idx] = clamp(depth, -1.0f, 1.0f);
}

void GlobalLFOSystem::unassign(int slot, int paramId, int idx) {
    if (!isValidSlot(slot) || !isValidParam(paramId) || !isValidLFO(idx)) return;
    
    auto& assignment = assignments_[slot][paramId];
    assignment.mask &= ~(1 << idx);
    assignment.depths[idx] = 0.0f;
}

void GlobalLFOSystem::clearAssignments(int slot, int paramId) {
    if (!isValidSlot(slot) || !isValidParam(paramId)) return;
    
    auto& assignment = assignments_[slot][paramId];
    assignment.mask = 0;
    assignment.depths.fill(0.0f);
}

void GlobalLFOSystem::retrigger(int slot) {
    if (!isValidSlot(slot)) return;
    
    for (int idx = 0; idx < MAX_LFOS; ++idx) {
        retriggerLFO(slot, idx);
    }
}

void GlobalLFOSystem::retriggerLFO(int slot, int idx) {
    if (!isValidSlot(slot) || !isValidLFO(idx)) return;
    
    auto& lfo = lfoStates_[slot][idx];
    
    switch (lfo.settings.sync) {
        case SyncMode::Key:
        case SyncMode::OneShot:
            lfo.phase = 0.0f;
            break;
            
        case SyncMode::Envelope:
            lfo.envPhase = LFOState::ENV_ATTACK;
            lfo.envLevel = 0.0f;
            lfo.envRate = 1.0f / (lfo.settings.envA * sampleRate_);
            break;
            
        default:
            // Free and Tempo modes don't retrigger
            break;
    }
}

void GlobalLFOSystem::releaseEnvelopes(int slot) {
    if (!isValidSlot(slot)) return;
    
    for (auto& lfo : lfoStates_[slot]) {
        if (lfo.settings.sync == SyncMode::Envelope && 
            lfo.envPhase != LFOState::ENV_IDLE) {
            lfo.envPhase = LFOState::ENV_RELEASE;
            lfo.envRate = -lfo.envLevel / (lfo.settings.envR * sampleRate_);
        }
    }
}

void GlobalLFOSystem::stepBlock(int slot, int frames) {
    if (!isValidSlot(slot)) return;
    
    for (auto& lfo : lfoStates_[slot]) {
        if (lfo.active) {
            updateLFO(lfo, frames);
        } else {
            lfo.lastValue = 0.0f;
        }
    }
}

float GlobalLFOSystem::combinedValue(int slot, int paramId) const {
    if (!isValidSlot(slot) || !isValidParam(paramId)) return 0.0f;
    
    const auto& assignment = assignments_[slot][paramId];
    float sum = 0.0f;
    
    for (int idx = 0; idx < MAX_LFOS; ++idx) {
        if (assignment.mask & (1 << idx)) {
            sum += lfoStates_[slot][idx].lastValue * assignment.depths[idx];
        }
    }
    
    return clamp(sum);
}

uint8_t GlobalLFOSystem::mask(int slot, int paramId) const {
    if (!isValidSlot(slot) || !isValidParam(paramId)) return 0;
    return assignments_[slot][paramId].mask;
}

float GlobalLFOSystem::getLFOValue(int slot, int idx) const {
    if (!isValidSlot(slot) || !isValidLFO(idx)) return 0.0f;
    return lfoStates_[slot][idx].lastValue;
}

bool GlobalLFOSystem::isActive(int slot, int idx) const {
    if (!isValidSlot(slot) || !isValidLFO(idx)) return false;
    return lfoStates_[slot][idx].active;
}

void GlobalLFOSystem::updateLFO(LFOState& lfo, int frames) {
    if (lfo.settings.sync == SyncMode::Envelope) {
        updateEnvelope(lfo, frames);
        return;
    }
    
    // Calculate phase increment for this block
    float phaseInc = calculatePhaseIncrement(lfo, frames);
    lfo.phase += phaseInc;
    
    // Handle OneShot completion
    if (lfo.settings.sync == SyncMode::OneShot) {
        if (lfo.phase >= 2.0f * M_PI) {
            lfo.phase = 2.0f * M_PI;
            lfo.active = false;
            lfo.lastValue = 0.0f;
            return;
        }
    } else {
        // Wrap phase for other modes
        while (lfo.phase >= 2.0f * M_PI) {
            lfo.phase -= 2.0f * M_PI;
        }
    }
    
    // Generate waveform
    float rawValue = generateWaveform(lfo.settings.wave, lfo.phase, lfo.settings.pulseWidth);
    lfo.lastValue = clamp(rawValue * lfo.settings.depth);
}

void GlobalLFOSystem::updateEnvelope(LFOState& lfo, int frames) {
    float frameRate = 1.0f / sampleRate_ * frames;
    
    switch (lfo.envPhase) {
        case LFOState::ENV_ATTACK:
            lfo.envLevel += lfo.envRate * frames;
            if (lfo.envLevel >= 1.0f) {
                lfo.envLevel = 1.0f;
                lfo.envPhase = LFOState::ENV_DECAY;
                lfo.envRate = -(1.0f - lfo.settings.envS) / (lfo.settings.envD * sampleRate_);
            }
            break;
            
        case LFOState::ENV_DECAY:
            lfo.envLevel += lfo.envRate * frames;
            if (lfo.envLevel <= lfo.settings.envS) {
                lfo.envLevel = lfo.settings.envS;
                lfo.envPhase = LFOState::ENV_SUSTAIN;
                lfo.envRate = 0.0f;
            }
            break;
            
        case LFOState::ENV_SUSTAIN:
            lfo.envLevel = lfo.settings.envS;
            break;
            
        case LFOState::ENV_RELEASE:
            lfo.envLevel += lfo.envRate * frames;
            if (lfo.envLevel <= 0.0f) {
                lfo.envLevel = 0.0f;
                lfo.envPhase = LFOState::ENV_IDLE;
                lfo.envRate = 0.0f;
            }
            break;
            
        case LFOState::ENV_IDLE:
        default:
            lfo.envLevel = 0.0f;
            break;
    }
    
    lfo.lastValue = clamp(lfo.envLevel * lfo.settings.depth);
}

float GlobalLFOSystem::calculatePhaseIncrement(const LFOState& lfo, int frames) {
    float rate = 0.0f;
    
    switch (lfo.settings.sync) {
        case SyncMode::Free:
        case SyncMode::Key:
        case SyncMode::OneShot:
            rate = lfo.settings.rateHz;
            break;
            
        case SyncMode::Tempo:
            if (lfo.settings.clockDiv > 0 && lfo.settings.clockDiv < MAX_CLOCK_DIVS) {
                float beatsPerSecond = bpm_ / 60.0f;
                float divisionRate = CLOCK_DIVISIONS[lfo.settings.clockDiv];
                rate = beatsPerSecond * divisionRate;
            } else {
                rate = lfo.settings.rateHz; // Fallback to free rate
            }
            break;
            
        default:
            rate = lfo.settings.rateHz;
            break;
    }
    
    return (2.0f * M_PI * rate * frames) / sampleRate_;
}

float GlobalLFOSystem::generateWaveform(Waveform wave, float phase, float pulseWidth) {
    switch (wave) {
        case Waveform::Sine:
            return generateSine(phase);
            
        case Waveform::Tri:
            return generateTriangle(phase);
            
        case Waveform::SawUp:
            return generateSawtooth(phase, true);
            
        case Waveform::SawDown:
            return generateSawtooth(phase, false);
            
        case Waveform::Square:
        case Waveform::Pulse:
            return generateSquare(phase, pulseWidth);
            
        default:
            return 0.0f;
    }
}

float GlobalLFOSystem::generateSine(float phase) {
    return std::sin(phase);
}

float GlobalLFOSystem::generateTriangle(float phase) {
    float normalized = phase / (2.0f * M_PI); // 0-1
    if (normalized < 0.5f) {
        return -1.0f + 4.0f * normalized;
    } else {
        return 3.0f - 4.0f * normalized;
    }
}

float GlobalLFOSystem::generateSawtooth(float phase, bool rising) {
    float normalized = phase / (2.0f * M_PI); // 0-1
    if (rising) {
        return -1.0f + 2.0f * normalized;
    } else {
        return 1.0f - 2.0f * normalized;
    }
}

float GlobalLFOSystem::generateSquare(float phase, float pulseWidth) {
    float normalized = phase / (2.0f * M_PI); // 0-1
    return (normalized < pulseWidth) ? 1.0f : -1.0f;
}

float GlobalLFOSystem::generateSampleHold(LFOState& lfo) {
    float normalized = lfo.phase / (2.0f * M_PI); // 0-1
    
    // Trigger new value when phase wraps
    if (normalized < lfo.lastPhase) {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        lfo.holdValue = dist(gen);
    }
    lfo.lastPhase = normalized;
    
    return lfo.holdValue;
}

float GlobalLFOSystem::generateNoise(LFOState& lfo) {
    // Generate new target occasionally
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    
    if (std::abs(lfo.noiseSmooth - lfo.noiseTarget) < 0.01f) {
        lfo.noiseTarget = dist(gen);
    }
    
    // Smooth toward target (simple lowpass)
    float smoothRate = 0.01f; // Adjust for desired smoothness
    lfo.noiseSmooth += (lfo.noiseTarget - lfo.noiseSmooth) * smoothRate;
    
    return lfo.noiseSmooth;
}

float GlobalLFOSystem::generateExponential(float phase, bool rising) {
    float normalized = phase / (2.0f * M_PI); // 0-1
    
    if (rising) {
        // Exponential rise: starts slow, accelerates
        float exp_val = (std::exp(normalized * 4.0f) - 1.0f) / (std::exp(4.0f) - 1.0f);
        return -1.0f + 2.0f * exp_val;
    } else {
        // Exponential fall: starts fast, decelerates  
        float exp_val = (std::exp((1.0f - normalized) * 4.0f) - 1.0f) / (std::exp(4.0f) - 1.0f);
        return 1.0f - 2.0f * exp_val;
    }
}

float GlobalLFOSystem::clamp(float value, float min, float max) {
    return std::min(max, std::max(min, value));
}

bool GlobalLFOSystem::isValidSlot(int slot) const {
    return slot >= 0 && slot < MAX_SLOTS;
}

bool GlobalLFOSystem::isValidLFO(int idx) const {
    return idx >= 0 && idx < MAX_LFOS;
}

bool GlobalLFOSystem::isValidParam(int paramId) const {
    return paramId >= 0 && paramId < MAX_PARAMS;
}