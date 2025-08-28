#include "MultiSampleTrack.h"
#include <algorithm>
#include <random>
#include <cmath>

MultiSampleTrack::MultiSampleTrack(uint8_t trackId) {
    trackConfig_.trackId = trackId;
    trackConfig_ = TrackConfig();
    
    nextVoiceId_ = 0;
    hotLoadingEnabled_ = false;
    
    sampleLoader_ = nullptr;
    sequencer_ = nullptr;
    
    // Initialize sample slots as inactive
    for (uint8_t i = 0; i < MAX_SAMPLE_SLOTS; ++i) {
        sampleSlots_[i] = SampleSlotConfig();
        sampleSlots_[i].slotId = i;
    }
    
    // Reserve voice storage
    voices_.reserve(MAX_VOICES);
}

// Configuration
void MultiSampleTrack::setTrackConfig(const TrackConfig& config) {
    trackConfig_ = config;
    
    // Validate and clamp values
    trackConfig_.maxPolyphony = std::min(trackConfig_.maxPolyphony, static_cast<uint8_t>(MAX_VOICES));
    trackConfig_.masterGain = std::max(0.0f, std::min(trackConfig_.masterGain, 4.0f));
    trackConfig_.masterPitch = std::max(-24.0f, std::min(trackConfig_.masterPitch, 24.0f));
    trackConfig_.masterPan = std::max(-1.0f, std::min(trackConfig_.masterPan, 1.0f));
    
    // Adjust active voices if polyphony was reduced
    while (voices_.size() > trackConfig_.maxPolyphony) {
        releaseVoice(voices_.back().voiceId, true);
    }
}

void MultiSampleTrack::setTriggerMode(TriggerMode mode) {
    trackConfig_.triggerMode = mode;
    
    // Reset round-robin position when changing modes
    if (mode == TriggerMode::ROUND_ROBIN) {
        trackConfig_.roundRobinPosition = 0;
    }
}

// Sample Slot Management
bool MultiSampleTrack::assignSampleToSlot(uint8_t slotId, uint8_t sampleSlotId, const SampleSlotConfig& config) {
    if (!isValidSlotId(slotId) || !isValidSampleSlotId(sampleSlotId)) {
        return false;
    }
    
    // Validate configuration
    SampleSlotConfig validatedConfig = config;
    validatedConfig.slotId = slotId;
    validatedConfig.velocityMin = std::max(0.0f, std::min(validatedConfig.velocityMin, 1.0f));
    validatedConfig.velocityMax = std::max(validatedConfig.velocityMin, std::min(validatedConfig.velocityMax, 1.0f));
    validatedConfig.gain = std::max(0.0f, std::min(validatedConfig.gain, 4.0f));
    validatedConfig.pitchOffset = std::max(-24.0f, std::min(validatedConfig.pitchOffset, 24.0f));
    validatedConfig.panPosition = std::max(-1.0f, std::min(validatedConfig.panPosition, 1.0f));
    
    sampleSlots_[slotId] = validatedConfig;
    sampleSlots_[slotId].isActive = true;
    
    // Auto-analyze and configure sample if sample loader is available
    if (sampleLoader_) {
        analyzeAndConfigureSample(slotId, sampleSlotId);
    }
    
    return true;
}

bool MultiSampleTrack::removeSampleFromSlot(uint8_t slotId) {
    if (!isValidSlotId(slotId)) {
        return false;
    }
    
    // Stop any voices using this slot
    for (auto it = voices_.begin(); it != voices_.end();) {
        if (it->sampleSlot == slotId) {
            notifyVoiceStateChange(it->voiceId, false);
            it = voices_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Deactivate slot
    sampleSlots_[slotId].isActive = false;
    sampleSlots_[slotId] = SampleSlotConfig();
    sampleSlots_[slotId].slotId = slotId;
    
    return true;
}

void MultiSampleTrack::clearAllSamples() {
    // Stop all voices
    stopAllVoices();
    
    // Clear all sample slots
    for (uint8_t i = 0; i < MAX_SAMPLE_SLOTS; ++i) {
        sampleSlots_[i] = SampleSlotConfig();
        sampleSlots_[i].slotId = i;
        sampleSlots_[i].isActive = false;
    }
    
    // Reset round-robin position
    trackConfig_.roundRobinPosition = 0;
}

MultiSampleTrack::SampleSlotConfig MultiSampleTrack::getSampleSlotConfig(uint8_t slotId) const {
    if (!isValidSlotId(slotId)) {
        return SampleSlotConfig();
    }
    return sampleSlots_[slotId];
}

bool MultiSampleTrack::setSampleSlotConfig(uint8_t slotId, const SampleSlotConfig& config) {
    if (!isValidSlotId(slotId)) {
        return false;
    }
    
    SampleSlotConfig validatedConfig = config;
    validatedConfig.slotId = slotId;
    
    // Validate ranges
    validatedConfig.velocityMin = std::max(0.0f, std::min(validatedConfig.velocityMin, 1.0f));
    validatedConfig.velocityMax = std::max(validatedConfig.velocityMin, std::min(validatedConfig.velocityMax, 1.0f));
    
    sampleSlots_[slotId] = validatedConfig;
    
    notifyParameterChange(slotId, "config_updated", 1.0f);
    return true;
}

// Sample Information
std::vector<uint8_t> MultiSampleTrack::getActiveSampleSlots() const {
    std::vector<uint8_t> activeSlots;
    
    for (uint8_t i = 0; i < MAX_SAMPLE_SLOTS; ++i) {
        if (sampleSlots_[i].isActive) {
            activeSlots.push_back(i);
        }
    }
    
    return activeSlots;
}

bool MultiSampleTrack::isSampleSlotActive(uint8_t slotId) const {
    if (!isValidSlotId(slotId)) {
        return false;
    }
    return sampleSlots_[slotId].isActive;
}

uint8_t MultiSampleTrack::getSampleCount() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_SAMPLE_SLOTS; ++i) {
        if (sampleSlots_[i].isActive) {
            ++count;
        }
    }
    return count;
}

// Playback Control
void MultiSampleTrack::triggerSample(float velocity, float pitch, float pan) {
    velocity = std::max(MIN_VELOCITY, std::min(velocity, MAX_VELOCITY));
    
    // Select sample slot(s) based on trigger mode
    std::vector<uint8_t> selectedSlots;
    
    switch (trackConfig_.triggerMode) {
        case TriggerMode::SINGLE_SHOT: {
            uint8_t slot = selectSampleSlot(velocity, trackConfig_.triggerMode);
            if (slot < MAX_SAMPLE_SLOTS) {
                selectedSlots.push_back(slot);
            }
            break;
        }
        
        case TriggerMode::VELOCITY_LAYERS:
        case TriggerMode::VELOCITY_CROSSFADE: {
            selectedSlots = selectMultipleSampleSlots(velocity, trackConfig_.triggerMode);
            break;
        }
        
        case TriggerMode::ROUND_ROBIN: {
            uint8_t slot = getRoundRobinSlot();
            if (slot < MAX_SAMPLE_SLOTS && sampleSlots_[slot].isActive) {
                selectedSlots.push_back(slot);
                advanceRoundRobin();
            }
            break;
        }
        
        case TriggerMode::RANDOM: {
            auto activeSlots = getActiveSampleSlots();
            if (!activeSlots.empty()) {
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, activeSlots.size() - 1);
                selectedSlots.push_back(activeSlots[dis(gen)]);
            }
            break;
        }
        
        case TriggerMode::STACK_ALL: {
            selectedSlots = getActiveSampleSlots();
            break;
        }
        
        case TriggerMode::CHORD_MODE: {
            selectedSlots = getActiveSampleSlots();
            break;
        }
    }
    
    // Trigger selected samples
    for (uint8_t slotId : selectedSlots) {
        if (voices_.size() < trackConfig_.maxPolyphony) {
            uint8_t voiceId = allocateVoice(slotId, velocity);
            if (voiceId != 255) {
                SampleVoice* voice = getVoice(voiceId);
                if (voice) {
                    // Apply pitch and pan modulation
                    if (trackConfig_.triggerMode == TriggerMode::CHORD_MODE) {
                        voice->targetPitch = pitch * std::pow(2.0f, sampleSlots_[slotId].chordInterval / 12.0f);
                    } else {
                        voice->targetPitch = pitch;
                    }
                    voice->targetPan = pan;
                }
                notifySampleTriggered(slotId, velocity);
            }
        }
    }
}

void MultiSampleTrack::stopAllVoices() {
    for (auto& voice : voices_) {
        releaseVoice(voice.voiceId, false);
    }
}

void MultiSampleTrack::stopVoice(uint8_t voiceId) {
    releaseVoice(voiceId, false);
}

bool MultiSampleTrack::isAnyVoicePlaying() const {
    return !voices_.empty();
}

uint8_t MultiSampleTrack::getActiveVoiceCount() const {
    return static_cast<uint8_t>(voices_.size());
}

// Voice Management
MultiSampleTrack::SampleVoice* MultiSampleTrack::getVoice(uint8_t voiceId) {
    auto it = std::find_if(voices_.begin(), voices_.end(),
                          [voiceId](const SampleVoice& voice) {
                              return voice.voiceId == voiceId;
                          });
    return (it != voices_.end()) ? &(*it) : nullptr;
}

uint8_t MultiSampleTrack::allocateVoice(uint8_t sampleSlot, float velocity) {
    if (!isValidSlotId(sampleSlot) || !sampleSlots_[sampleSlot].isActive) {
        return 255;  // Invalid slot
    }
    
    uint8_t voiceId = 255;
    
    // Try to find free voice
    if (voices_.size() < trackConfig_.maxPolyphony) {
        voiceId = nextVoiceId_++;
        if (nextVoiceId_ == 255) nextVoiceId_ = 0;  // Wrap around
        
        voices_.emplace_back();
        SampleVoice& voice = voices_.back();
        initializeVoice(voice, sampleSlot, velocity);
        voice.voiceId = voiceId;
    } else {
        // Steal least important voice
        voiceId = stealVoice();
        if (voiceId != 255) {
            SampleVoice* voice = getVoice(voiceId);
            if (voice) {
                initializeVoice(*voice, sampleSlot, velocity);
            }
        }
    }
    
    if (voiceId != 255) {
        notifyVoiceStateChange(voiceId, true);
    }
    
    return voiceId;
}

void MultiSampleTrack::releaseVoice(uint8_t voiceId, bool immediate) {
    auto it = std::find_if(voices_.begin(), voices_.end(),
                          [voiceId](const SampleVoice& voice) {
                              return voice.voiceId == voiceId;
                          });
    
    if (it != voices_.end()) {
        if (immediate) {
            notifyVoiceStateChange(voiceId, false);
            voices_.erase(it);
        } else {
            // Start fade out
            it->fadeOutSamples = msToSamples(trackConfig_.voiceFadeTimeMs, 48000);  // Assume 48kHz
            it->targetGain = 0.0f;
        }
    }
}

// Parameter Control
void MultiSampleTrack::setMasterGain(float gain) {
    trackConfig_.masterGain = std::max(0.0f, std::min(gain, 4.0f));
    notifyParameterChange(255, "master_gain", trackConfig_.masterGain);
}

void MultiSampleTrack::setMasterPitch(float pitchOffset) {
    trackConfig_.masterPitch = std::max(-24.0f, std::min(pitchOffset, 24.0f));
    notifyParameterChange(255, "master_pitch", trackConfig_.masterPitch);
}

void MultiSampleTrack::setMasterPan(float pan) {
    trackConfig_.masterPan = std::max(-1.0f, std::min(pan, 1.0f));
    notifyParameterChange(255, "master_pan", trackConfig_.masterPan);
}

void MultiSampleTrack::setSampleSlotGain(uint8_t slotId, float gain) {
    if (isValidSlotId(slotId)) {
        sampleSlots_[slotId].gain = std::max(0.0f, std::min(gain, 4.0f));
        notifyParameterChange(slotId, "gain", sampleSlots_[slotId].gain);
    }
}

void MultiSampleTrack::setSampleSlotPitch(uint8_t slotId, float pitchOffset) {
    if (isValidSlotId(slotId)) {
        sampleSlots_[slotId].pitchOffset = std::max(-24.0f, std::min(pitchOffset, 24.0f));
        notifyParameterChange(slotId, "pitch", sampleSlots_[slotId].pitchOffset);
    }
}

void MultiSampleTrack::setSampleSlotPan(uint8_t slotId, float pan) {
    if (isValidSlotId(slotId)) {
        sampleSlots_[slotId].panPosition = std::max(-1.0f, std::min(pan, 1.0f));
        notifyParameterChange(slotId, "pan", sampleSlots_[slotId].panPosition);
    }
}

// Real-time Modulation
void MultiSampleTrack::modulateParameter(uint8_t slotId, const std::string& parameter, float value) {
    if (!isValidSlotId(slotId)) return;
    
    if (parameter == "gain") {
        setSampleSlotGain(slotId, value);
    } else if (parameter == "pitch") {
        setSampleSlotPitch(slotId, value);
    } else if (parameter == "pan") {
        setSampleSlotPan(slotId, value);
    }
}

void MultiSampleTrack::setGlobalModulation(const std::string& parameter, float value) {
    if (parameter == "master_gain") {
        setMasterGain(value);
    } else if (parameter == "master_pitch") {
        setMasterPitch(value);
    } else if (parameter == "master_pan") {
        setMasterPan(value);
    }
}

void MultiSampleTrack::updateVoiceParameters() {
    // Called from audio thread - update smoothed parameters
    for (auto& voice : voices_) {
        smoothParameter(voice.currentGain, voice.targetGain, 1.0f / PARAMETER_SMOOTH_RATE_HZ);
        smoothParameter(voice.currentPitch, voice.targetPitch, 1.0f / PARAMETER_SMOOTH_RATE_HZ);
        smoothParameter(voice.currentPan, voice.targetPan, 1.0f / PARAMETER_SMOOTH_RATE_HZ);
    }
}

// Audio Processing
void MultiSampleTrack::processAudio(float* outputBuffer, uint32_t sampleCount, uint32_t sampleRate) {
    if (voices_.empty()) {
        // No voices - output silence
        std::fill(outputBuffer, outputBuffer + sampleCount * 2, 0.0f);  // Assume stereo
        return;
    }
    
    // Clear output buffer
    std::fill(outputBuffer, outputBuffer + sampleCount * 2, 0.0f);
    
    // Mix all active voices
    for (auto it = voices_.begin(); it != voices_.end();) {
        SampleVoice& voice = *it;
        
        // Update voice
        updateVoice(voice, sampleCount, sampleRate);
        
        // Mix voice to output
        if (voice.isActive) {
            mixVoiceToBuffer(voice, outputBuffer, sampleCount, sampleRate);
            ++it;
        } else {
            // Voice finished - remove it
            notifyVoiceStateChange(voice.voiceId, false);
            it = voices_.erase(it);
        }
    }
}

void MultiSampleTrack::mixVoiceToBuffer(const SampleVoice& voice, float* buffer, 
                                       uint32_t sampleCount, uint32_t sampleRate) {
    if (!sampleAccessCallback_) return;
    
    const auto& sampleSlot = sampleAccessCallback_(voice.sampleSlot);
    if (!sampleSlot.isOccupied || !sampleSlot.audioData) return;
    
    const auto& audioData = sampleSlot.audioData->audioData;
    uint8_t channels = sampleSlot.audioData->format.channelCount;
    uint32_t totalSamples = sampleSlot.audioData->sampleCount;
    
    float gain = calculateVoiceGain(voice, 1.0f) * trackConfig_.masterGain;
    float pitch = calculateVoicePitch(voice, 1.0f);
    float leftGain = (voice.currentPan <= 0.0f) ? 1.0f : (1.0f - voice.currentPan);
    float rightGain = (voice.currentPan >= 0.0f) ? 1.0f : (1.0f + voice.currentPan);
    
    for (uint32_t i = 0; i < sampleCount; ++i) {
        if (voice.samplePosition >= totalSamples) {
            if (voice.isLooping && voice.loopEnd > voice.loopStart) {
                // Handle looping
                const_cast<SampleVoice&>(voice).samplePosition = voice.loopStart;
            } else {
                // Sample finished
                const_cast<SampleVoice&>(voice).isActive = false;
                break;
            }
        }
        
        // Interpolate sample
        float sampleValue = interpolateSample(audioData, voice.samplePosition * pitch, channels);
        
        // Apply gain and pan
        buffer[i * 2] += sampleValue * gain * leftGain;      // Left channel
        buffer[i * 2 + 1] += sampleValue * gain * rightGain; // Right channel
        
        // Advance sample position
        const_cast<SampleVoice&>(voice).samplePosition++;
    }
}

// Integration
void MultiSampleTrack::integrateWithAutoSampleLoader(AutoSampleLoader* sampleLoader) {
    sampleLoader_ = sampleLoader;
}

void MultiSampleTrack::integrateWithSequencer(SequencerEngine* sequencer) {
    sequencer_ = sequencer;
}

void MultiSampleTrack::setSampleAccessCallback(std::function<const AutoSampleLoader::SamplerSlot&(uint8_t)> callback) {
    sampleAccessCallback_ = callback;
}

// Hot Loading
bool MultiSampleTrack::hotSwapSample(uint8_t slotId, uint8_t newSampleSlotId) {
    if (!hotLoadingEnabled_ || !isValidSlotId(slotId) || !isValidSampleSlotId(newSampleSlotId)) {
        return false;
    }
    
    // Stop any voices using this slot
    for (auto& voice : voices_) {
        if (voice.sampleSlot == slotId) {
            releaseVoice(voice.voiceId, false);  // Fade out
        }
    }
    
    // Update slot configuration to point to new sample
    if (sampleSlots_[slotId].isActive) {
        analyzeAndConfigureSample(slotId, newSampleSlotId);
        return true;
    }
    
    return false;
}

bool MultiSampleTrack::hotLoadSample(uint8_t slotId, std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> audioData) {
    if (!hotLoadingEnabled_ || !sampleLoader_) {
        return false;
    }
    
    // Load sample into sample loader first
    auto result = sampleLoader_->loadSample(audioData, "Hot Load");
    if (result.success) {
        return hotSwapSample(slotId, result.assignedSlot);
    }
    
    return false;
}

void MultiSampleTrack::enableHotLoadingMode(bool enabled) {
    hotLoadingEnabled_ = enabled;
}

// Sample Analysis and Auto-Configuration
void MultiSampleTrack::analyzeAndConfigureSample(uint8_t slotId, uint8_t sampleSlotId) {
    if (!sampleAccessCallback_ || !isValidSlotId(slotId) || !isValidSampleSlotId(sampleSlotId)) {
        return;
    }
    
    const auto& sampleSlot = sampleAccessCallback_(sampleSlotId);
    if (!sampleSlot.isOccupied || !sampleSlot.audioData) {
        return;
    }
    
    // Analyze audio content
    float contentComplexity = analyzeAudioContent(sampleSlot.audioData->audioData);
    
    // Configure based on analysis
    SampleSlotConfig& config = sampleSlots_[slotId];
    
    // Set default velocity range based on content
    if (contentComplexity > 0.7f) {
        // Complex content - higher velocity range
        config.velocityMin = 0.6f;
        config.velocityMax = 1.0f;
    } else if (contentComplexity > 0.3f) {
        // Medium content - mid velocity range
        config.velocityMin = 0.3f;
        config.velocityMax = 0.8f;
    } else {
        // Simple content - lower velocity range
        config.velocityMin = 0.0f;
        config.velocityMax = 0.5f;
    }
    
    // Set priority based on content
    config.priority = static_cast<uint8_t>(contentComplexity * 15.0f);
}

void MultiSampleTrack::autoConfigureVelocityLayers() {
    auto activeSlots = getActiveSampleSlots();
    if (activeSlots.empty()) return;
    
    // Distribute velocity ranges evenly
    float rangePerSlot = 1.0f / activeSlots.size();
    
    for (size_t i = 0; i < activeSlots.size(); ++i) {
        uint8_t slotId = activeSlots[i];
        sampleSlots_[slotId].velocityMin = i * rangePerSlot;
        sampleSlots_[slotId].velocityMax = (i + 1) * rangePerSlot;
        
        // Overlap ranges slightly for crossfading
        if (i > 0) {
            sampleSlots_[slotId].velocityMin -= 0.05f;
        }
        if (i < activeSlots.size() - 1) {
            sampleSlots_[slotId].velocityMax += 0.05f;
        }
    }
}

void MultiSampleTrack::optimizeSlotConfiguration() {
    // Remove inactive slots and compact active ones
    uint8_t writeIndex = 0;
    
    for (uint8_t readIndex = 0; readIndex < MAX_SAMPLE_SLOTS; ++readIndex) {
        if (sampleSlots_[readIndex].isActive) {
            if (writeIndex != readIndex) {
                sampleSlots_[writeIndex] = sampleSlots_[readIndex];
                sampleSlots_[writeIndex].slotId = writeIndex;
                
                // Update any voices using old slot ID
                for (auto& voice : voices_) {
                    if (voice.sampleSlot == readIndex) {
                        voice.sampleSlot = writeIndex;
                    }
                }
            }
            writeIndex++;
        }
    }
    
    // Clear remaining slots
    for (uint8_t i = writeIndex; i < MAX_SAMPLE_SLOTS; ++i) {
        sampleSlots_[i] = SampleSlotConfig();
        sampleSlots_[i].slotId = i;
        sampleSlots_[i].isActive = false;
    }
}

// Callbacks
void MultiSampleTrack::setVoiceStateChangeCallback(VoiceStateChangeCallback callback) {
    voiceStateChangeCallback_ = callback;
}

void MultiSampleTrack::setSampleTriggerCallback(SampleTriggerCallback callback) {
    sampleTriggerCallback_ = callback;
}

void MultiSampleTrack::setParameterChangeCallback(ParameterChangeCallback callback) {
    parameterChangeCallback_ = callback;
}

// Memory Management
size_t MultiSampleTrack::getEstimatedMemoryUsage() const {
    return sizeof(MultiSampleTrack) + 
           voices_.capacity() * sizeof(SampleVoice);
}

void MultiSampleTrack::optimizeMemoryUsage() {
    voices_.shrink_to_fit();
}

// Internal methods
uint8_t MultiSampleTrack::selectSampleSlot(float velocity, TriggerMode mode) {
    auto activeSlots = getActiveSampleSlots();
    if (activeSlots.empty()) {
        return 255;  // No active slots
    }
    
    // Find best matching slot based on velocity
    uint8_t bestSlot = 255;
    float bestWeight = -1.0f;
    
    for (uint8_t slotId : activeSlots) {
        if (isSlotInVelocityRange(slotId, velocity)) {
            float weight = calculateSlotWeight(slotId, velocity);
            if (weight > bestWeight) {
                bestWeight = weight;
                bestSlot = slotId;
            }
        }
    }
    
    // If no slot matches velocity range, use first active slot
    return (bestSlot == 255) ? activeSlots[0] : bestSlot;
}

std::vector<uint8_t> MultiSampleTrack::selectMultipleSampleSlots(float velocity, TriggerMode mode) {
    std::vector<uint8_t> selectedSlots;
    auto activeSlots = getActiveSampleSlots();
    
    for (uint8_t slotId : activeSlots) {
        if (isSlotInVelocityRange(slotId, velocity) || mode == TriggerMode::VELOCITY_CROSSFADE) {
            selectedSlots.push_back(slotId);
        }
    }
    
    return selectedSlots;
}

bool MultiSampleTrack::isSlotInVelocityRange(uint8_t slotId, float velocity) const {
    const SampleSlotConfig& config = sampleSlots_[slotId];
    return velocity >= config.velocityMin && velocity <= config.velocityMax;
}

float MultiSampleTrack::calculateSlotWeight(uint8_t slotId, float velocity) const {
    const SampleSlotConfig& config = sampleSlots_[slotId];
    
    // Calculate distance from velocity range center
    float rangeCenter = (config.velocityMin + config.velocityMax) * 0.5f;
    float distance = std::abs(velocity - rangeCenter);
    float rangeSize = config.velocityMax - config.velocityMin;
    
    // Weight based on proximity to range center and priority
    float proximityWeight = 1.0f - (distance / (rangeSize * 0.5f));
    float priorityWeight = config.priority / 15.0f;
    
    return proximityWeight * 0.7f + priorityWeight * 0.3f;
}

// Voice allocation helpers
uint8_t MultiSampleTrack::findFreeVoice() {
    if (voices_.size() < trackConfig_.maxPolyphony) {
        return nextVoiceId_++;
    }
    return 255;  // No free voice
}

uint8_t MultiSampleTrack::stealVoice() {
    if (voices_.empty()) {
        return 255;
    }
    
    // Find voice with lowest priority (lowest gain * priority)
    auto lowestIt = std::min_element(voices_.begin(), voices_.end(),
                                    [this](const SampleVoice& a, const SampleVoice& b) {
                                        float aPriority = a.currentGain * sampleSlots_[a.sampleSlot].priority;
                                        float bPriority = b.currentGain * sampleSlots_[b.sampleSlot].priority;
                                        return aPriority < bPriority;
                                    });
    
    uint8_t voiceId = lowestIt->voiceId;
    
    // Release the voice (it will be reinitialized by caller)
    notifyVoiceStateChange(voiceId, false);
    
    return voiceId;
}

void MultiSampleTrack::initializeVoice(SampleVoice& voice, uint8_t sampleSlot, float velocity) {
    voice.sampleSlot = sampleSlot;
    voice.isActive = true;
    voice.samplePosition = 0;
    
    const SampleSlotConfig& config = sampleSlots_[sampleSlot];
    
    // Set initial parameters
    voice.targetGain = config.gain * (trackConfig_.enableVelocityScaling ? velocity : 1.0f);
    voice.currentGain = 0.0f;  // Start from silence
    voice.targetPitch = std::pow(2.0f, (config.pitchOffset + trackConfig_.masterPitch) / 12.0f);
    voice.currentPitch = voice.targetPitch;
    voice.targetPan = config.panPosition + trackConfig_.masterPan;
    voice.currentPan = voice.targetPan;
    
    // Set fade times
    voice.fadeInSamples = msToSamples(trackConfig_.voiceFadeTimeMs, 48000);
    voice.fadeOutSamples = 0;
    
    // Initialize looping (would be set based on sample metadata)
    voice.isLooping = false;
    voice.loopStart = 0;
    voice.loopEnd = 0;
}

void MultiSampleTrack::updateVoice(SampleVoice& voice, uint32_t sampleCount, uint32_t sampleRate) {
    // Update parameter smoothing
    smoothParameter(voice.currentGain, voice.targetGain, 
                   static_cast<float>(sampleCount) / sampleRate / DEFAULT_CROSSFADE_TIME);
    smoothParameter(voice.currentPitch, voice.targetPitch, 
                   static_cast<float>(sampleCount) / sampleRate / DEFAULT_CROSSFADE_TIME);
    smoothParameter(voice.currentPan, voice.targetPan, 
                   static_cast<float>(sampleCount) / sampleRate / DEFAULT_CROSSFADE_TIME);
    
    // Handle fade out
    if (voice.fadeOutSamples > 0) {
        if (voice.fadeOutSamples <= sampleCount) {
            voice.isActive = false;
            voice.fadeOutSamples = 0;
        } else {
            voice.fadeOutSamples -= sampleCount;
        }
    }
    
    // Check if voice gain is too low
    if (voice.currentGain < 0.001f && voice.targetGain < 0.001f) {
        voice.isActive = false;
    }
}

// Audio processing helpers
float MultiSampleTrack::interpolateSample(const std::vector<float>& audioData, float position, uint8_t channels) const {
    if (audioData.empty() || channels == 0) return 0.0f;
    
    uint32_t index = static_cast<uint32_t>(position);
    float fraction = position - index;
    
    // Bounds checking
    if (index >= audioData.size() / channels - 1) {
        return 0.0f;
    }
    
    // Linear interpolation (mono or take left channel if stereo)
    float sample1 = audioData[index * channels];
    float sample2 = audioData[(index + 1) * channels];
    
    return sample1 + fraction * (sample2 - sample1);
}

void MultiSampleTrack::applyCrossfade(float* buffer, uint32_t sampleCount, float fadeIn, float fadeOut) {
    for (uint32_t i = 0; i < sampleCount; ++i) {
        float fadeGain = 1.0f;
        
        if (fadeIn > 0.0f && i < fadeIn * sampleCount) {
            fadeGain *= i / (fadeIn * sampleCount);
        }
        
        if (fadeOut > 0.0f && i > (1.0f - fadeOut) * sampleCount) {
            fadeGain *= 1.0f - ((i - (1.0f - fadeOut) * sampleCount) / (fadeOut * sampleCount));
        }
        
        buffer[i * 2] *= fadeGain;      // Left
        buffer[i * 2 + 1] *= fadeGain;  // Right
    }
}

float MultiSampleTrack::calculateVoiceGain(const SampleVoice& voice, float velocity) const {
    const SampleSlotConfig& config = sampleSlots_[voice.sampleSlot];
    return voice.currentGain * config.gain;
}

float MultiSampleTrack::calculateVoicePitch(const SampleVoice& voice, float pitch) const {
    const SampleSlotConfig& config = sampleSlots_[voice.sampleSlot];
    return voice.currentPitch * pitch * std::pow(2.0f, trackConfig_.masterPitch / 12.0f);
}

// Parameter smoothing
void MultiSampleTrack::smoothParameter(float& current, float target, float rate) {
    float diff = target - current;
    current += diff * std::min(1.0f, rate);
}

uint32_t MultiSampleTrack::msToSamples(uint16_t ms, uint32_t sampleRate) const {
    return (ms * sampleRate) / 1000;
}

// Round-robin helpers
void MultiSampleTrack::advanceRoundRobin() {
    auto activeSlots = getActiveSampleSlots();
    if (!activeSlots.empty()) {
        trackConfig_.roundRobinPosition = (trackConfig_.roundRobinPosition + 1) % activeSlots.size();
    }
}

uint8_t MultiSampleTrack::getRoundRobinSlot() const {
    auto activeSlots = getActiveSampleSlots();
    if (activeSlots.empty()) {
        return 255;
    }
    
    uint8_t index = trackConfig_.roundRobinPosition % activeSlots.size();
    return activeSlots[index];
}

// Auto-configuration helpers
void MultiSampleTrack::detectVelocityRanges(uint8_t slotId) {
    // Mock implementation - would analyze sample characteristics
}

void MultiSampleTrack::suggestOptimalTriggerMode() {
    uint8_t sampleCount = getSampleCount();
    
    if (sampleCount <= 1) {
        trackConfig_.triggerMode = TriggerMode::SINGLE_SHOT;
    } else if (sampleCount <= 4) {
        trackConfig_.triggerMode = TriggerMode::VELOCITY_LAYERS;
    } else {
        trackConfig_.triggerMode = TriggerMode::ROUND_ROBIN;
    }
}

float MultiSampleTrack::analyzeAudioContent(const std::vector<float>& audioData) const {
    if (audioData.empty()) return 0.0f;
    
    // Simple analysis - calculate dynamic range and frequency content estimate
    float minValue = *std::min_element(audioData.begin(), audioData.end());
    float maxValue = *std::max_element(audioData.begin(), audioData.end());
    float dynamicRange = maxValue - minValue;
    
    // Calculate rough complexity based on variation
    float avgVariation = 0.0f;
    for (size_t i = 1; i < audioData.size(); ++i) {
        avgVariation += std::abs(audioData[i] - audioData[i-1]);
    }
    avgVariation /= audioData.size();
    
    return std::min(1.0f, (dynamicRange * 2.0f + avgVariation * 10.0f) * 0.5f);
}

// Validation helpers
bool MultiSampleTrack::isValidSlotId(uint8_t slotId) const {
    return slotId < MAX_SAMPLE_SLOTS;
}

bool MultiSampleTrack::isValidVoiceId(uint8_t voiceId) const {
    return std::find_if(voices_.begin(), voices_.end(),
                       [voiceId](const SampleVoice& voice) {
                           return voice.voiceId == voiceId;
                       }) != voices_.end();
}

bool MultiSampleTrack::isValidSampleSlotId(uint8_t sampleSlotId) const {
    return sampleSlotId < 16;  // Assuming max 16 sample loader slots
}

// Notification helpers
void MultiSampleTrack::notifyVoiceStateChange(uint8_t voiceId, bool started) {
    if (voiceStateChangeCallback_) {
        voiceStateChangeCallback_(voiceId, started);
    }
}

void MultiSampleTrack::notifySampleTriggered(uint8_t slotId, float velocity) {
    if (sampleTriggerCallback_) {
        sampleTriggerCallback_(slotId, velocity);
    }
}

void MultiSampleTrack::notifyParameterChange(uint8_t slotId, const std::string& parameter, float value) {
    if (parameterChangeCallback_) {
        parameterChangeCallback_(slotId, parameter, value);
    }
}