#include "VelocityPitchRangeManager.h"
#include "MultiSampleTrack.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <random>

VelocityPitchRangeManager::VelocityPitchRangeManager() {
    config_ = RangeConfig();
    
    lastVelocity_ = 0.5f;
    lastMidiNote_ = 60;  // Middle C
    lastSelectionTime_ = 0;
    
    sampleLoader_ = nullptr;
    multiTrack_ = nullptr;
    
    selectionCount_ = 0;
    totalSelectionTime_ = 0;
    
    // Initialize round-robin positions
    for (uint8_t i = 0; i < MAX_ROUND_ROBIN_GROUPS; ++i) {
        roundRobinPositions_[i] = 0;
    }
}

// Configuration
void VelocityPitchRangeManager::setRangeConfig(const RangeConfig& config) {
    if (validateRangeConfig(config)) {
        config_ = config;
        
        // Update round-robin groups if mode changed
        if (config_.mode == RangeMode::ROUND_ROBIN || 
            config_.mode == RangeMode::VELOCITY_ROUND_ROBIN ||
            config_.mode == RangeMode::PITCH_ROUND_ROBIN ||
            config_.mode == RangeMode::FULL_MATRIX) {
            updateRoundRobinGroups();
        }
    }
}

void VelocityPitchRangeManager::setRangeMode(RangeMode mode) {
    config_.mode = mode;
    updateRoundRobinGroups();
}

// Range Management
bool VelocityPitchRangeManager::addSampleRange(const SampleRange& range) {
    if (!validateSampleRange(range)) {
        return false;
    }
    
    SampleRange sanitizedRange = range;
    sanitizeRangeValues(sanitizedRange);
    
    sampleRanges_[range.sampleSlot] = sanitizedRange;
    
    // Update round-robin groups
    updateRoundRobinGroups();
    
    // Notify callback
    notifyRangeUpdated(range.sampleSlot, sanitizedRange);
    
    return true;
}

bool VelocityPitchRangeManager::removeSampleRange(uint8_t sampleSlot) {
    auto it = sampleRanges_.find(sampleSlot);
    if (it == sampleRanges_.end()) {
        return false;
    }
    
    sampleRanges_.erase(it);
    
    // Update round-robin groups
    updateRoundRobinGroups();
    
    return true;
}

bool VelocityPitchRangeManager::updateSampleRange(uint8_t sampleSlot, const SampleRange& range) {
    if (!validateSampleRange(range)) {
        return false;
    }
    
    if (sampleRanges_.find(sampleSlot) == sampleRanges_.end()) {
        return false;
    }
    
    SampleRange sanitizedRange = range;
    sanitizedRange.sampleSlot = sampleSlot;  // Ensure consistency
    sanitizeRangeValues(sanitizedRange);
    
    sampleRanges_[sampleSlot] = sanitizedRange;
    
    // Update round-robin groups
    updateRoundRobinGroups();
    
    // Notify callback
    notifyRangeUpdated(sampleSlot, sanitizedRange);
    
    return true;
}

void VelocityPitchRangeManager::clearAllRanges() {
    sampleRanges_.clear();
    roundRobinGroups_.clear();
    resetRoundRobin();
}

// Range Information
std::vector<VelocityPitchRangeManager::SampleRange> VelocityPitchRangeManager::getAllRanges() const {
    std::vector<SampleRange> ranges;
    
    for (const auto& pair : sampleRanges_) {
        ranges.push_back(pair.second);
    }
    
    return ranges;
}

VelocityPitchRangeManager::SampleRange VelocityPitchRangeManager::getSampleRange(uint8_t sampleSlot) const {
    auto it = sampleRanges_.find(sampleSlot);
    if (it != sampleRanges_.end()) {
        return it->second;
    }
    return SampleRange();  // Return default range
}

bool VelocityPitchRangeManager::hasSampleRange(uint8_t sampleSlot) const {
    return sampleRanges_.find(sampleSlot) != sampleRanges_.end();
}

uint8_t VelocityPitchRangeManager::getRangeCount() const {
    return static_cast<uint8_t>(sampleRanges_.size());
}

// Sample Selection
VelocityPitchRangeManager::RangeSelectionResult VelocityPitchRangeManager::selectSamples(
    float velocity, uint8_t midiNote, uint8_t channel) {
    
    uint32_t startTime = getCurrentTimeMs();
    
    RangeSelectionResult result;
    result.usedMode = config_.mode;
    
    // Store for smoothing calculations
    lastVelocity_ = velocity;
    lastMidiNote_ = midiNote;
    
    // Find candidate ranges
    std::vector<uint8_t> candidates = findCandidateRanges(velocity, midiNote);
    
    if (candidates.empty()) {
        updatePerformanceCounters(getCurrentTimeMs() - startTime);
        return result;  // No matching ranges
    }
    
    // Apply range mode specific selection
    switch (config_.mode) {
        case RangeMode::VELOCITY_ONLY:
        case RangeMode::PITCH_ONLY:
        case RangeMode::VELOCITY_PITCH: {
            // Filter by priority and select top candidates
            std::vector<uint8_t> prioritized = filterByPriority(candidates);
            
            // Limit to max simultaneous slots
            if (prioritized.size() > config_.maxSimultaneousSlots) {
                prioritized.resize(config_.maxSimultaneousSlots);
            }
            
            result.selectedSlots = prioritized;
            break;
        }
        
        case RangeMode::ROUND_ROBIN: {
            // Select one sample using round-robin
            if (!candidates.empty()) {
                uint8_t selected = selectRoundRobinSample(0);  // Use group 0 for simple round-robin
                if (std::find(candidates.begin(), candidates.end(), selected) != candidates.end()) {
                    result.selectedSlots.push_back(selected);
                } else {
                    result.selectedSlots.push_back(candidates[0]);  // Fallback
                }
                result.hasRoundRobin = true;
                result.roundRobinPosition = roundRobinPositions_[0];
            }
            break;
        }
        
        case RangeMode::VELOCITY_ROUND_ROBIN:
        case RangeMode::PITCH_ROUND_ROBIN:
        case RangeMode::FULL_MATRIX: {
            // Apply round-robin within groups
            for (uint8_t candidate : candidates) {
                const SampleRange& range = sampleRanges_[candidate];
                std::vector<uint8_t> groupCandidates = {candidate};  // Simplified for now
                std::vector<uint8_t> selected = applyRoundRobinSelection(groupCandidates, range.roundRobinGroup);
                
                for (uint8_t slot : selected) {
                    if (result.selectedSlots.size() < config_.maxSimultaneousSlots) {
                        result.selectedSlots.push_back(slot);
                    }
                }
            }
            result.hasRoundRobin = true;
            break;
        }
    }
    
    // Calculate blend weights and adjustments
    for (size_t i = 0; i < result.selectedSlots.size(); ++i) {
        uint8_t slot = result.selectedSlots[i];
        const SampleRange& range = sampleRanges_[slot];
        
        // Calculate crossfade weight
        float weight = calculateRangeWeight(velocity, midiNote, range);
        result.blendWeights.push_back(weight);
        
        // Apply range-specific adjustments
        result.gainAdjustments.push_back(range.gain);
        result.pitchAdjustments.push_back(range.pitchOffset);
        result.panAdjustments.push_back(range.panPosition);
    }
    
    // Normalize blend weights for equal-power mixing
    if (!result.blendWeights.empty()) {
        float totalWeight = 0.0f;
        for (float weight : result.blendWeights) {
            totalWeight += weight * weight;  // Sum of squares for equal power
        }
        
        if (totalWeight > 0.0f) {
            float normalizer = 1.0f / std::sqrt(totalWeight);
            for (float& weight : result.blendWeights) {
                weight *= normalizer;
            }
        }
    }
    
    // Update performance counters
    updatePerformanceCounters(getCurrentTimeMs() - startTime);
    
    // Notify callback
    notifyRangeSelected(result);
    
    return result;
}

VelocityPitchRangeManager::RangeSelectionResult VelocityPitchRangeManager::selectSamplesWithContext(
    float velocity, uint8_t midiNote, uint8_t channel, const std::string& context) {
    
    // For now, context is ignored - could be used for future enhancements
    return selectSamples(velocity, midiNote, channel);
}

// Range Analysis
std::vector<uint8_t> VelocityPitchRangeManager::getSamplesInVelocityRange(float velocityMin, float velocityMax) const {
    std::vector<uint8_t> samples;
    
    for (const auto& pair : sampleRanges_) {
        const SampleRange& range = pair.second;
        
        // Check for overlap
        if (range.velocityMax >= velocityMin && range.velocityMin <= velocityMax) {
            samples.push_back(pair.first);
        }
    }
    
    return samples;
}

std::vector<uint8_t> VelocityPitchRangeManager::getSamplesInPitchRange(uint8_t pitchMin, uint8_t pitchMax) const {
    std::vector<uint8_t> samples;
    
    for (const auto& pair : sampleRanges_) {
        const SampleRange& range = pair.second;
        
        // Check for overlap
        if (range.pitchMax >= pitchMin && range.pitchMin <= pitchMax) {
            samples.push_back(pair.first);
        }
    }
    
    return samples;
}

std::vector<uint8_t> VelocityPitchRangeManager::getOverlappingSamples(float velocity, uint8_t midiNote) const {
    std::vector<uint8_t> overlapping;
    
    for (const auto& pair : sampleRanges_) {
        const SampleRange& range = pair.second;
        
        if (isVelocityInRange(velocity, range) && isPitchInRange(midiNote, range)) {
            overlapping.push_back(pair.first);
        }
    }
    
    return overlapping;
}

// Auto-Assignment
void VelocityPitchRangeManager::autoAssignVelocityRanges(const std::vector<uint8_t>& sampleSlots, uint8_t layerCount) {
    if (sampleSlots.empty() || layerCount == 0) return;
    
    distributeVelocityRanges(sampleSlots, layerCount);
    updateRoundRobinGroups();
}

void VelocityPitchRangeManager::autoAssignPitchRanges(const std::vector<uint8_t>& sampleSlots, 
                                                      uint8_t keyMin, uint8_t keyMax) {
    if (sampleSlots.empty() || keyMin >= keyMax) return;
    
    distributePitchRanges(sampleSlots, keyMin, keyMax);
    updateRoundRobinGroups();
}

void VelocityPitchRangeManager::autoAssignMatrix(const std::vector<uint8_t>& sampleSlots, 
                                                 uint8_t velocityLayers, uint8_t pitchZones) {
    if (sampleSlots.empty() || velocityLayers == 0 || pitchZones == 0) return;
    
    createMatrixAssignment(sampleSlots, velocityLayers, pitchZones);
    updateRoundRobinGroups();
}

// Range Optimization
void VelocityPitchRangeManager::optimizeRangeAssignments() {
    detectAndFixGaps();
    detectAndFixOverlaps(true);
    normalizeRangeWeights();
}

void VelocityPitchRangeManager::detectAndFixGaps() {
    fillVelocityGaps();
    fillPitchGaps();
}

void VelocityPitchRangeManager::detectAndFixOverlaps(bool allowControlledOverlaps) {
    if (!allowControlledOverlaps) {
        resolveOverlaps();
    }
}

void VelocityPitchRangeManager::normalizeRangeWeights() {
    adjustRangeWeights();
}

// Round-Robin Management
void VelocityPitchRangeManager::resetRoundRobin() {
    for (auto& pair : roundRobinPositions_) {
        pair.second = 0;
    }
}

void VelocityPitchRangeManager::resetRoundRobinForGroup(uint8_t group) {
    if (group < MAX_ROUND_ROBIN_GROUPS) {
        roundRobinPositions_[group] = 0;
    }
}

void VelocityPitchRangeManager::advanceRoundRobin(uint8_t group) {
    if (group < MAX_ROUND_ROBIN_GROUPS) {
        auto it = roundRobinGroups_.find(group);
        if (it != roundRobinGroups_.end() && !it->second.empty()) {
            roundRobinPositions_[group] = (roundRobinPositions_[group] + 1) % it->second.size();
            notifyRoundRobinAdvanced(group, roundRobinPositions_[group]);
        }
    }
}

uint8_t VelocityPitchRangeManager::getCurrentRoundRobinPosition(uint8_t group) const {
    auto it = roundRobinPositions_.find(group);
    return (it != roundRobinPositions_.end()) ? it->second : 0;
}

// Crossfade Management
void VelocityPitchRangeManager::setCrossfadeMode(CrossfadeMode mode) {
    for (auto& pair : sampleRanges_) {
        pair.second.crossfadeMode = mode;
    }
}

void VelocityPitchRangeManager::setCrossfadeWidth(float width) {
    width = std::max(MIN_CROSSFADE_WIDTH, std::min(width, MAX_CROSSFADE_WIDTH));
    
    for (auto& pair : sampleRanges_) {
        pair.second.crossfadeWidth = width;
    }
}

void VelocityPitchRangeManager::setCustomCrossfadeCurve(const std::vector<float>& curve) {
    if (!curve.empty()) {
        if (customCrossfadeCurves_.empty()) {
            customCrossfadeCurves_.resize(1);
        }
        customCrossfadeCurves_[0] = curve;
    }
}

float VelocityPitchRangeManager::calculateCrossfadeWeight(float position, float rangeMin, float rangeMax, 
                                                         CrossfadeMode mode, float width) const {
    switch (mode) {
        case CrossfadeMode::NONE:
            return (position >= rangeMin && position <= rangeMax) ? 1.0f : 0.0f;
            
        case CrossfadeMode::LINEAR:
            return calculateLinearCrossfade(position, rangeMin, rangeMax, width);
            
        case CrossfadeMode::EQUAL_POWER:
            return calculateEqualPowerCrossfade(position, rangeMin, rangeMax, width);
            
        case CrossfadeMode::EXPONENTIAL:
            return calculateExponentialCrossfade(position, rangeMin, rangeMax, width);
            
        case CrossfadeMode::CUSTOM_CURVE:
            if (!customCrossfadeCurves_.empty()) {
                return calculateCustomCrossfade(position, rangeMin, rangeMax, customCrossfadeCurves_[0]);
            }
            return calculateLinearCrossfade(position, rangeMin, rangeMax, width);
            
        default:
            return calculateEqualPowerCrossfade(position, rangeMin, rangeMax, width);
    }
}

// Integration
void VelocityPitchRangeManager::integrateWithAutoSampleLoader(AutoSampleLoader* sampleLoader) {
    sampleLoader_ = sampleLoader;
}

void VelocityPitchRangeManager::setSampleAccessCallback(std::function<const AutoSampleLoader::SamplerSlot&(uint8_t)> callback) {
    sampleAccessCallback_ = callback;
}

void VelocityPitchRangeManager::integrateWithMultiSampleTrack(MultiSampleTrack* track) {
    multiTrack_ = track;
}

// Callbacks
void VelocityPitchRangeManager::setRangeSelectedCallback(RangeSelectedCallback callback) {
    rangeSelectedCallback_ = callback;
}

void VelocityPitchRangeManager::setRangeUpdatedCallback(RangeUpdatedCallback callback) {
    rangeUpdatedCallback_ = callback;
}

void VelocityPitchRangeManager::setRoundRobinAdvancedCallback(RoundRobinAdvancedCallback callback) {
    roundRobinAdvancedCallback_ = callback;
}

// Performance Analysis
size_t VelocityPitchRangeManager::getEstimatedMemoryUsage() const {
    return sizeof(VelocityPitchRangeManager) +
           sampleRanges_.size() * sizeof(SampleRange) +
           roundRobinGroups_.size() * sizeof(std::vector<uint8_t>) +
           presets_.size() * sizeof(RangePreset);
}

float VelocityPitchRangeManager::getAverageSelectionTime() const {
    return (selectionCount_ > 0) ? static_cast<float>(totalSelectionTime_) / selectionCount_ : 0.0f;
}

void VelocityPitchRangeManager::resetPerformanceCounters() {
    selectionCount_ = 0;
    totalSelectionTime_ = 0;
}

// Internal methods
std::vector<uint8_t> VelocityPitchRangeManager::findCandidateRanges(float velocity, uint8_t midiNote) const {
    std::vector<uint8_t> candidates;
    
    for (const auto& pair : sampleRanges_) {
        const SampleRange& range = pair.second;
        
        bool velocityMatch = (config_.mode == RangeMode::PITCH_ONLY) || 
                            isVelocityInRange(velocity, range);
        bool pitchMatch = (config_.mode == RangeMode::VELOCITY_ONLY) || 
                         isPitchInRange(midiNote, range);
        
        if (velocityMatch && pitchMatch) {
            candidates.push_back(pair.first);
        }
    }
    
    return candidates;
}

std::vector<uint8_t> VelocityPitchRangeManager::filterByPriority(const std::vector<uint8_t>& candidates) const {
    if (candidates.empty()) return candidates;
    
    // Sort by priority (higher priority first)
    std::vector<std::pair<uint8_t, uint8_t>> prioritized;  // (slot, priority)
    
    for (uint8_t slot : candidates) {
        const SampleRange& range = sampleRanges_.at(slot);
        prioritized.emplace_back(slot, range.priority);
    }
    
    std::sort(prioritized.begin(), prioritized.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<uint8_t> result;
    for (const auto& pair : prioritized) {
        result.push_back(pair.first);
    }
    
    return result;
}

std::vector<uint8_t> VelocityPitchRangeManager::applyRoundRobinSelection(const std::vector<uint8_t>& candidates, uint8_t group) {
    if (candidates.empty()) return candidates;
    
    auto it = roundRobinGroups_.find(group);
    if (it == roundRobinGroups_.end() || it->second.empty()) {
        return {candidates[0]};  // Return first candidate if no round-robin group
    }
    
    uint8_t selectedSlot = selectRoundRobinSample(group);
    
    // Check if selected slot is in candidates
    if (std::find(candidates.begin(), candidates.end(), selectedSlot) != candidates.end()) {
        return {selectedSlot};
    }
    
    return {candidates[0]};  // Fallback to first candidate
}

// Range calculation helpers
bool VelocityPitchRangeManager::isVelocityInRange(float velocity, const SampleRange& range) const {
    return velocity >= range.velocityMin && velocity <= range.velocityMax;
}

bool VelocityPitchRangeManager::isPitchInRange(uint8_t midiNote, const SampleRange& range) const {
    return midiNote >= range.pitchMin && midiNote <= range.pitchMax;
}

float VelocityPitchRangeManager::calculateRangeWeight(float velocity, uint8_t midiNote, const SampleRange& range) const {
    float velocityWeight = 1.0f;
    float pitchWeight = 1.0f;
    
    // Calculate velocity-based weight with crossfade
    if (config_.mode != RangeMode::PITCH_ONLY) {
        float velocityCenter = (range.velocityMin + range.velocityMax) * 0.5f;
        velocityWeight = calculateCrossfadeWeight(velocity, range.velocityMin, range.velocityMax,
                                                 range.crossfadeMode, range.crossfadeWidth);
    }
    
    // Calculate pitch-based weight with crossfade
    if (config_.mode != RangeMode::VELOCITY_ONLY) {
        float pitchCenter = (range.pitchMin + range.pitchMax) * 0.5f;
        float normalizedPitch = midiNote / 127.0f;
        float normalizedPitchMin = range.pitchMin / 127.0f;
        float normalizedPitchMax = range.pitchMax / 127.0f;
        
        pitchWeight = calculateCrossfadeWeight(normalizedPitch, normalizedPitchMin, normalizedPitchMax,
                                              range.crossfadeMode, range.crossfadeWidth);
    }
    
    return velocityWeight * pitchWeight;
}

// Crossfade calculation
float VelocityPitchRangeManager::calculateLinearCrossfade(float position, float rangeMin, float rangeMax, float width) const {
    float rangeSize = rangeMax - rangeMin;
    float fadeZone = rangeSize * width;
    
    if (position < rangeMin) {
        // Fade in from left
        float fadeStart = rangeMin - fadeZone;
        if (position < fadeStart) return 0.0f;
        return (position - fadeStart) / fadeZone;
    } else if (position > rangeMax) {
        // Fade out to right
        float fadeEnd = rangeMax + fadeZone;
        if (position > fadeEnd) return 0.0f;
        return (fadeEnd - position) / fadeZone;
    } else {
        // Within range
        return 1.0f;
    }
}

float VelocityPitchRangeManager::calculateEqualPowerCrossfade(float position, float rangeMin, float rangeMax, float width) const {
    float linearWeight = calculateLinearCrossfade(position, rangeMin, rangeMax, width);
    return std::sqrt(linearWeight);
}

float VelocityPitchRangeManager::calculateExponentialCrossfade(float position, float rangeMin, float rangeMax, float width) const {
    float linearWeight = calculateLinearCrossfade(position, rangeMin, rangeMax, width);
    return linearWeight * linearWeight;  // Square for exponential curve
}

float VelocityPitchRangeManager::calculateCustomCrossfade(float position, float rangeMin, float rangeMax, 
                                                         const std::vector<float>& curve) const {
    if (curve.empty()) {
        return calculateLinearCrossfade(position, rangeMin, rangeMax, 0.1f);
    }
    
    // Map position to curve index
    float normalizedPosition = (position - rangeMin) / (rangeMax - rangeMin);
    normalizedPosition = std::max(0.0f, std::min(1.0f, normalizedPosition));
    
    float curveIndex = normalizedPosition * (curve.size() - 1);
    size_t index = static_cast<size_t>(curveIndex);
    float fraction = curveIndex - index;
    
    if (index >= curve.size() - 1) {
        return curve.back();
    }
    
    // Linear interpolation between curve points
    return curve[index] * (1.0f - fraction) + curve[index + 1] * fraction;
}

// Auto-assignment helpers
void VelocityPitchRangeManager::distributeVelocityRanges(const std::vector<uint8_t>& slots, uint8_t layerCount) {
    if (slots.empty() || layerCount == 0) return;
    
    float rangeSize = 1.0f / layerCount;
    float overlap = rangeSize * 0.1f;  // 10% overlap for crossfading
    
    for (size_t i = 0; i < slots.size(); ++i) {
        SampleRange range;
        range.sampleSlot = slots[i];
        
        uint8_t layer = i % layerCount;
        range.velocityMin = std::max(0.0f, layer * rangeSize - overlap);
        range.velocityMax = std::min(1.0f, (layer + 1) * rangeSize + overlap);
        
        range.pitchMin = 0;
        range.pitchMax = 127;
        range.roundRobinGroup = layer;
        range.priority = 128 + layer * 10;  // Higher layers get higher priority
        
        addSampleRange(range);
    }
}

void VelocityPitchRangeManager::distributePitchRanges(const std::vector<uint8_t>& slots, uint8_t keyMin, uint8_t keyMax) {
    if (slots.empty() || keyMin >= keyMax) return;
    
    uint8_t keyRange = keyMax - keyMin;
    uint8_t zoneSize = keyRange / slots.size();
    uint8_t overlap = std::max(1, zoneSize / 10);  // 10% overlap
    
    for (size_t i = 0; i < slots.size(); ++i) {
        SampleRange range;
        range.sampleSlot = slots[i];
        
        range.velocityMin = 0.0f;
        range.velocityMax = 1.0f;
        
        range.pitchMin = std::max(keyMin, static_cast<uint8_t>(keyMin + i * zoneSize - overlap));
        range.pitchMax = std::min(keyMax, static_cast<uint8_t>(keyMin + (i + 1) * zoneSize + overlap));
        
        range.roundRobinGroup = static_cast<uint8_t>(i);
        range.priority = 128;
        
        addSampleRange(range);
    }
}

void VelocityPitchRangeManager::createMatrixAssignment(const std::vector<uint8_t>& slots, 
                                                       uint8_t velocityLayers, uint8_t pitchZones) {
    if (slots.empty() || velocityLayers == 0 || pitchZones == 0) return;
    
    float velocityRangeSize = 1.0f / velocityLayers;
    uint8_t pitchRangeSize = 127 / pitchZones;
    
    for (size_t i = 0; i < slots.size(); ++i) {
        SampleRange range;
        range.sampleSlot = slots[i];
        
        // Calculate matrix position
        uint8_t velocityLayer = (i / pitchZones) % velocityLayers;
        uint8_t pitchZone = i % pitchZones;
        
        // Set velocity range
        range.velocityMin = velocityLayer * velocityRangeSize;
        range.velocityMax = (velocityLayer + 1) * velocityRangeSize;
        
        // Set pitch range
        range.pitchMin = pitchZone * pitchRangeSize;
        range.pitchMax = std::min(127, (pitchZone + 1) * pitchRangeSize);
        
        // Set round-robin group (combine velocity layer and pitch zone)
        range.roundRobinGroup = velocityLayer * pitchZones + pitchZone;
        range.priority = 128 + velocityLayer * 10;
        
        addSampleRange(range);
    }
}

// Round-robin helpers
void VelocityPitchRangeManager::updateRoundRobinGroups() {
    roundRobinGroups_.clear();
    
    for (const auto& pair : sampleRanges_) {
        const SampleRange& range = pair.second;
        uint8_t group = range.roundRobinGroup;
        
        if (group < MAX_ROUND_ROBIN_GROUPS) {
            roundRobinGroups_[group].push_back(pair.first);
        }
    }
    
    // Ensure all groups have valid positions
    for (auto& pair : roundRobinGroups_) {
        if (pair.second.size() > 0) {
            roundRobinPositions_[pair.first] = roundRobinPositions_[pair.first] % pair.second.size();
        }
    }
}

uint8_t VelocityPitchRangeManager::selectRoundRobinSample(uint8_t group) {
    auto it = roundRobinGroups_.find(group);
    if (it == roundRobinGroups_.end() || it->second.empty()) {
        return 255;  // Invalid slot
    }
    
    uint8_t position = roundRobinPositions_[group];
    uint8_t selectedSlot = it->second[position];
    
    // Advance to next position
    advanceRoundRobin(group);
    
    return selectedSlot;
}

// Validation helpers
bool VelocityPitchRangeManager::validateSampleRange(const SampleRange& range) const {
    return range.velocityMin >= 0.0f && range.velocityMax <= 1.0f &&
           range.velocityMin <= range.velocityMax &&
           range.pitchMin <= range.pitchMax &&
           range.pitchMin <= 127 && range.pitchMax <= 127 &&
           range.roundRobinGroup < MAX_ROUND_ROBIN_GROUPS &&
           range.crossfadeWidth >= MIN_CROSSFADE_WIDTH &&
           range.crossfadeWidth <= MAX_CROSSFADE_WIDTH;
}

bool VelocityPitchRangeManager::validateRangeConfig(const RangeConfig& config) const {
    return config.maxSimultaneousSlots > 0 &&
           config.globalCrossfadeTime >= 0.0f &&
           config.velocitySmoothingTime >= 0.0f &&
           config.pitchSmoothingTime >= 0.0f;
}

void VelocityPitchRangeManager::sanitizeRangeValues(SampleRange& range) const {
    range.velocityMin = std::max(0.0f, std::min(range.velocityMin, 1.0f));
    range.velocityMax = std::max(range.velocityMin, std::min(range.velocityMax, 1.0f));
    range.pitchMin = std::min(range.pitchMin, static_cast<uint8_t>(127));
    range.pitchMax = std::max(range.pitchMin, std::min(range.pitchMax, static_cast<uint8_t>(127)));
    range.roundRobinGroup = std::min(range.roundRobinGroup, static_cast<uint8_t>(MAX_ROUND_ROBIN_GROUPS - 1));
    range.crossfadeWidth = std::max(MIN_CROSSFADE_WIDTH, std::min(range.crossfadeWidth, MAX_CROSSFADE_WIDTH));
}

// Performance helpers
uint32_t VelocityPitchRangeManager::getCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

void VelocityPitchRangeManager::updatePerformanceCounters(uint32_t selectionTime) {
    selectionCount_++;
    totalSelectionTime_ += selectionTime;
    
    // Keep counters from overflowing
    if (selectionCount_ > PERFORMANCE_HISTORY_SIZE) {
        selectionCount_ /= 2;
        totalSelectionTime_ /= 2;
    }
}

// Notification helpers
void VelocityPitchRangeManager::notifyRangeSelected(const RangeSelectionResult& result) {
    if (rangeSelectedCallback_) {
        rangeSelectedCallback_(result);
    }
}

void VelocityPitchRangeManager::notifyRangeUpdated(uint8_t sampleSlot, const SampleRange& range) {
    if (rangeUpdatedCallback_) {
        rangeUpdatedCallback_(sampleSlot, range);
    }
}

void VelocityPitchRangeManager::notifyRoundRobinAdvanced(uint8_t group, uint8_t position) {
    if (roundRobinAdvancedCallback_) {
        roundRobinAdvancedCallback_(group, position);
    }
}

// Stub implementations for optimization methods (simplified for now)
void VelocityPitchRangeManager::fillVelocityGaps() {
    // Implementation would detect gaps in velocity coverage and adjust ranges
}

void VelocityPitchRangeManager::fillPitchGaps() {
    // Implementation would detect gaps in pitch coverage and adjust ranges
}

void VelocityPitchRangeManager::resolveOverlaps() {
    // Implementation would detect and resolve problematic overlaps
}

void VelocityPitchRangeManager::adjustRangeWeights() {
    // Implementation would normalize priorities and weights
}