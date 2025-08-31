#include "EuclideanSequencer.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iostream>

namespace EtherSynth {

EuclideanSequencer::EuclideanSequencer() 
    : rng_(std::chrono::high_resolution_clock::now().time_since_epoch().count())
    , dist_(0.0f, 1.0f)
{
    std::cout << "EuclideanSequencer: Initializing euclidean rhythm engine\n";
    
    // Initialize all tracks with default patterns
    for (int i = 0; i < 8; i++) {
        trackPatterns_[i] = EuclideanPattern();
        trackPatterns_[i].pulses = (i < 4) ? 4 : 2;  // Drums get 4, others get 2
        polyrhythmEnabled_[i] = false;
        patternOffsets_[i] = 0;
        
        // Initialize linking matrix
        for (int j = 0; j < 8; j++) {
            linkedTracks_[i][j] = 0;
        }
    }
    
    // Generate initial patterns
    regenerateAllPatterns();
    
    // Load preset patterns
    initializePresets();
    
    std::cout << "EuclideanSequencer: Initialized with default patterns\n";
}

void EuclideanSequencer::generatePattern(EuclideanPattern& pattern, Algorithm algorithm) {
    if (!pattern.isDirty) return;
    
    std::cout << "EuclideanSequencer: Generating euclidean pattern (" << 
                 pattern.totalSteps << ", " << 
                 pattern.pulses << ", " << 
                 pattern.rotation << ")\n";
    
    // Generate base euclidean distribution
    std::vector<bool> basePattern;
    
    switch (algorithm) {
        case Algorithm::BJORKLUND:
            basePattern = bjorklundAlgorithm(pattern.totalSteps, pattern.pulses);
            break;
        case Algorithm::BRESENHAM:
            basePattern = bresenhamAlgorithm(pattern.totalSteps, pattern.pulses);
            break;
        case Algorithm::FRACTIONAL:
            basePattern = fractionalAlgorithm(pattern.totalSteps, pattern.pulses);
            break;
        case Algorithm::GOLDEN_RATIO:
            basePattern = goldenRatioAlgorithm(pattern.totalSteps, pattern.pulses);
            break;
        case Algorithm::COUNT:
        default:
            basePattern = bjorklundAlgorithm(pattern.totalSteps, pattern.pulses);
            break;
    }
    
    // Apply rotation
    applyRotation(basePattern, pattern.rotation);
    
    // Copy to pattern array
    for (int i = 0; i < 16; i++) {
        pattern.pattern[i] = (i < basePattern.size()) ? basePattern[i] : false;
    }
    
    // Apply swing and humanization
    applySwing(pattern);
    applyHumanization(pattern);
    generateVelocities(pattern);
    
    pattern.isDirty = false;
    
    std::cout << "EuclideanSequencer: Pattern generated successfully\n";
}

void EuclideanSequencer::regenerateAllPatterns() {
    for (int i = 0; i < 8; i++) {
        trackPatterns_[i].isDirty = true;
        generatePattern(trackPatterns_[i], defaultAlgorithm_);
    }
}

void EuclideanSequencer::setTrackPattern(int trackIndex, int pulses, int rotation) {
    validateTrackIndex(trackIndex);
    
    EuclideanPattern& pattern = trackPatterns_[trackIndex];
    
    if (pattern.pulses != pulses || pattern.rotation != rotation) {
        pattern.pulses = std::clamp(pulses, 0, 16);
        pattern.rotation = std::clamp(rotation, 0, 15);
        pattern.isDirty = true;
        
        generatePattern(pattern, defaultAlgorithm_);
        
        // Apply to linked tracks
        for (int i = 0; i < 8; i++) {
            if (linkedTracks_[trackIndex][i] > 0) {
                setTrackPattern(i, pulses, rotation);
            }
        }
        
        std::cout << "EuclideanSequencer: Set track " << trackIndex << " pattern (" << pulses << ", " << rotation << ")\n";
    }
}

void EuclideanSequencer::setTrackProbability(int trackIndex, float probability) {
    validateTrackIndex(trackIndex);
    
    trackPatterns_[trackIndex].probability = std::clamp(probability, 0.0f, 1.0f);
    
    std::cout << "EuclideanSequencer: Set track " << trackIndex << " probability " << probability << "\n";
}

void EuclideanSequencer::setTrackSwing(int trackIndex, float swing) {
    validateTrackIndex(trackIndex);
    
    EuclideanPattern& pattern = trackPatterns_[trackIndex];
    
    if (std::abs(pattern.swing - swing) > 0.01f) {
        pattern.swing = std::clamp(swing, -1.0f, 1.0f);
        pattern.isDirty = true;
        generatePattern(pattern, defaultAlgorithm_);
    }
}

bool EuclideanSequencer::shouldTriggerStep(int trackIndex, int stepIndex, float randomValue) const {
    validateTrackIndex(trackIndex);
    
    if (stepIndex < 0 || stepIndex >= 16) return false;
    
    const EuclideanPattern& pattern = trackPatterns_[trackIndex];
    
    // Check if step is active in euclidean pattern
    if (!pattern.pattern[stepIndex]) return false;
    
    // Apply probability
    if (pattern.probability < 1.0f) {
        float rand = (randomValue >= 0.0f) ? randomValue : const_cast<EuclideanSequencer*>(this)->dist_(const_cast<EuclideanSequencer*>(this)->rng_);
        if (rand > pattern.probability) return false;
    }
    
    return true;
}

float EuclideanSequencer::getStepVelocity(int trackIndex, int stepIndex) const {
    validateTrackIndex(trackIndex);
    
    if (stepIndex < 0 || stepIndex >= 16) return 0.0f;
    
    const EuclideanPattern& pattern = trackPatterns_[trackIndex];
    return pattern.velocities[stepIndex];
}

float EuclideanSequencer::getStepTiming(int trackIndex, int stepIndex) const {
    validateTrackIndex(trackIndex);
    
    if (stepIndex < 0 || stepIndex >= 16) return 0.0f;
    
    const EuclideanPattern& pattern = trackPatterns_[trackIndex];
    return pattern.timingOffsets[stepIndex];
}

std::vector<bool> EuclideanSequencer::bjorklundAlgorithm(int steps, int pulses) const {
    if (pulses <= 0 || steps <= 0 || pulses > steps) {
        return std::vector<bool>(steps, false);
    }
    
    std::vector<bool> pattern(steps, false);
    
    // Bjorklund algorithm implementation
    std::vector<int> counts(steps, 0);
    std::vector<int> remainders(steps, 0);
    
    int divisor = steps - pulses;
    remainders[0] = pulses;
    int level = 0;
    
    do {
        counts[level] = divisor / remainders[level];
        remainders[level + 1] = divisor % remainders[level];
        divisor = remainders[level];
        level++;
    } while (remainders[level] > 1);
    
    counts[level] = divisor;
    
    // Build the pattern
    int index = 0;
    for (int i = 0; i < level; i++) {
        for (int j = 0; j < counts[i]; j++) {
            pattern[index++] = true;
            if (remainders[i] > 0) {
                for (int k = 0; k < remainders[i]; k++) {
                    if (index < steps) pattern[index++] = false;
                }
            }
        }
    }
    
    // Fill remaining with the final remainder pattern
    for (int i = 0; i < counts[level] && index < steps; i++) {
        pattern[index++] = true;
    }
    
    return pattern;
}

std::vector<bool> EuclideanSequencer::bresenhamAlgorithm(int steps, int pulses) const {
    std::vector<bool> pattern(steps, false);
    
    if (pulses <= 0 || steps <= 0) return pattern;
    
    int error = steps / 2;
    
    for (int i = 0; i < steps; i++) {
        error -= pulses;
        if (error < 0) {
            pattern[i] = true;
            error += steps;
        }
    }
    
    return pattern;
}

std::vector<bool> EuclideanSequencer::fractionalAlgorithm(int steps, int pulses) const {
    std::vector<bool> pattern(steps, false);
    
    if (pulses <= 0 || steps <= 0) return pattern;
    
    float interval = float(steps) / float(pulses);
    
    for (int i = 0; i < pulses; i++) {
        int position = int(i * interval);
        if (position < steps) {
            pattern[position] = true;
        }
    }
    
    return pattern;
}

std::vector<bool> EuclideanSequencer::goldenRatioAlgorithm(int steps, int pulses) const {
    std::vector<bool> pattern(steps, false);
    
    if (pulses <= 0 || steps <= 0) return pattern;
    
    const float goldenRatio = 1.618033988749f;
    float angle = 2.0f * M_PI / goldenRatio;
    
    for (int i = 0; i < pulses; i++) {
        int position = int(i * angle * steps / (2.0f * M_PI)) % steps;
        pattern[position] = true;
    }
    
    return pattern;
}

void EuclideanSequencer::applyRotation(std::vector<bool>& pattern, int rotation) const {
    if (rotation == 0 || pattern.empty()) return;
    
    int actualRotation = ((rotation % int(pattern.size())) + int(pattern.size())) % int(pattern.size());
    std::rotate(pattern.begin(), pattern.begin() + actualRotation, pattern.end());
}

void EuclideanSequencer::applySwing(EuclideanPattern& pattern) const {
    if (std::abs(pattern.swing) < 0.01f) {
        // No swing, reset timing offsets
        pattern.timingOffsets.fill(0.0f);
        return;
    }
    
    for (int i = 0; i < 16; i++) {
        pattern.timingOffsets[i] = calculateSwingOffset(i, pattern.swing);
    }
}

void EuclideanSequencer::applyHumanization(EuclideanPattern& pattern) {
    if (pattern.humanization <= 0.0f) {
        return;
    }
    
    for (int i = 0; i < 16; i++) {
        if (pattern.pattern[i]) {
            // Add random timing variation
            float variation = (dist_(rng_) - 0.5f) * pattern.humanization * 0.1f; // Up to 100ms at full humanization
            pattern.timingOffsets[i] += variation;
        }
    }
}

void EuclideanSequencer::generateVelocities(EuclideanPattern& pattern) {
    float baseVelocity = 0.7f;
    float variation = pattern.velocityVariation;
    
    for (int i = 0; i < 16; i++) {
        if (pattern.pattern[i]) {
            float velocity = baseVelocity;
            
            if (variation > 0.0f) {
                velocity += (dist_(rng_) - 0.5f) * variation;
            }
            
            // Add accent on strong beats
            if (i % 4 == 0) velocity += 0.1f;  // Downbeat accent
            
            pattern.velocities[i] = std::clamp(velocity, 0.1f, 1.0f);
        } else {
            pattern.velocities[i] = 0.0f;
        }
    }
}

float EuclideanSequencer::calculateSwingOffset(int stepIndex, float swingAmount) const {
    // Apply swing to off-beats (2nd and 4th 16th notes in each beat)
    int beatPosition = stepIndex % 4;
    
    if (beatPosition == 1 || beatPosition == 3) {
        // Off-beat - apply swing
        return swingAmount * 0.1f; // Up to 100ms swing
    }
    
    return 0.0f;
}

void EuclideanSequencer::validateTrackIndex(int trackIndex) const {
    if (trackIndex < 0 || trackIndex >= 8) {
        throw std::out_of_range("Track index out of range: " + std::to_string(trackIndex));
    }
}

void EuclideanSequencer::initializePresets() {
    std::cout << "EuclideanSequencer: Loading preset patterns\n";
    
    // Drum patterns
    presetPatterns_["Four On Floor"] = EuclideanPresets::fourOnFloor();
    presetPatterns_["Off-Beat Hats"] = EuclideanPresets::offBeatHats();
    presetPatterns_["Snare Backbeat"] = EuclideanPresets::snareBackbeat();
    presetPatterns_["Clave"] = EuclideanPresets::clave();
    presetPatterns_["Tresillo"] = EuclideanPresets::tresillo();
    
    // Rhythmic patterns
    presetPatterns_["Five Against Four"] = EuclideanPresets::fiveAgainstFour();
    presetPatterns_["Seven Eight"] = EuclideanPresets::sevenEight();
    presetPatterns_["Golden Ratio"] = EuclideanPresets::goldenRatio();
    
    // Melodic patterns
    presetPatterns_["Arpeggio Pattern"] = EuclideanPresets::arpeggioPattern();
    presetPatterns_["Bass Line"] = EuclideanPresets::bassLine();
    presetPatterns_["Ambient Pulse"] = EuclideanPresets::ambientPulse();
    
    // Generative patterns
    presetPatterns_["Random Walk"] = EuclideanPresets::randomWalk();
    presetPatterns_["Fibonacci"] = EuclideanPresets::fibonacci();
    presetPatterns_["Prime Pulses"] = EuclideanPresets::primePulses();
    
    std::cout << "EuclideanSequencer: Loaded " << presetPatterns_.size() << " presets\n";
}

EuclideanSequencer::EuclideanPattern& EuclideanSequencer::getTrackPattern(int trackIndex) {
    validateTrackIndex(trackIndex);
    return trackPatterns_[trackIndex];
}

const EuclideanSequencer::EuclideanPattern& EuclideanSequencer::getTrackPattern(int trackIndex) const {
    validateTrackIndex(trackIndex);
    return trackPatterns_[trackIndex];
}

float EuclideanSequencer::getPatternDensity(int trackIndex) const {
    validateTrackIndex(trackIndex);
    
    const EuclideanPattern& pattern = trackPatterns_[trackIndex];
    return float(pattern.pulses) / float(pattern.totalSteps);
}

std::vector<int> EuclideanSequencer::getActiveSteps(int trackIndex) const {
    validateTrackIndex(trackIndex);
    
    const EuclideanPattern& pattern = trackPatterns_[trackIndex];
    std::vector<int> activeSteps;
    
    for (int i = 0; i < 16; i++) {
        if (pattern.pattern[i]) {
            activeSteps.push_back(i);
        }
    }
    
    return activeSteps;
}

void EuclideanSequencer::setTrackHumanization(int trackIndex, float humanization) {
    validateTrackIndex(trackIndex);
    
    EuclideanPattern& pattern = trackPatterns_[trackIndex];
    
    if (std::abs(pattern.humanization - humanization) > 0.01f) {
        pattern.humanization = std::clamp(humanization, 0.0f, 1.0f);
        pattern.isDirty = true;
        generatePattern(pattern, defaultAlgorithm_);
        
        std::cout << "EuclideanSequencer: Set track " << trackIndex << " humanization " << humanization << "\n";
    }
}

int EuclideanSequencer::getPatternComplexity(int trackIndex) const {
    validateTrackIndex(trackIndex);
    
    const EuclideanPattern& pattern = trackPatterns_[trackIndex];
    
    // Calculate complexity based on pattern distribution
    int changes = 0;
    for (int i = 0; i < 15; i++) {
        if (pattern.pattern[i] != pattern.pattern[i + 1]) {
            changes++;
        }
    }
    
    // Check wrap-around
    if (pattern.pattern[15] != pattern.pattern[0]) {
        changes++;
    }
    
    return changes;
}

void EuclideanSequencer::loadPresetPattern(int trackIndex, const std::string& presetName) {
    validateTrackIndex(trackIndex);
    
    auto it = presetPatterns_.find(presetName);
    if (it != presetPatterns_.end()) {
        trackPatterns_[trackIndex] = it->second;
        trackPatterns_[trackIndex].isDirty = true;
        generatePattern(trackPatterns_[trackIndex], defaultAlgorithm_);
        
        std::cout << "EuclideanSequencer: Loaded preset '" << presetName << "' to track " << trackIndex << "\n";
    } else {
        std::cout << "EuclideanSequencer: Preset '" << presetName << "' not found\n";
    }
}

void EuclideanSequencer::savePresetPattern(int trackIndex, const std::string& presetName) {
    validateTrackIndex(trackIndex);
    
    presetPatterns_[presetName] = trackPatterns_[trackIndex];
    
    std::cout << "EuclideanSequencer: Saved track " << trackIndex << " as preset '" << presetName << "'\n";
}

std::vector<std::string> EuclideanSequencer::getAvailablePresets() const {
    std::vector<std::string> presetNames;
    for (const auto& pair : presetPatterns_) {
        presetNames.push_back(pair.first);
    }
    return presetNames;
}

void EuclideanSequencer::morphBetweenPatterns(int trackIndex, const EuclideanPattern& targetPattern, float morphAmount) {
    validateTrackIndex(trackIndex);
    
    EuclideanPattern& currentPattern = trackPatterns_[trackIndex];
    morphAmount = std::clamp(morphAmount, 0.0f, 1.0f);
    
    // Morph parameters
    float invMorph = 1.0f - morphAmount;
    
    currentPattern.pulses = int(currentPattern.pulses * invMorph + targetPattern.pulses * morphAmount);
    currentPattern.rotation = int(currentPattern.rotation * invMorph + targetPattern.rotation * morphAmount);
    currentPattern.probability = currentPattern.probability * invMorph + targetPattern.probability * morphAmount;
    currentPattern.swing = currentPattern.swing * invMorph + targetPattern.swing * morphAmount;
    currentPattern.humanization = currentPattern.humanization * invMorph + targetPattern.humanization * morphAmount;
    currentPattern.velocityVariation = currentPattern.velocityVariation * invMorph + targetPattern.velocityVariation * morphAmount;
    
    currentPattern.isDirty = true;
    generatePattern(currentPattern, defaultAlgorithm_);
    
    std::cout << "EuclideanSequencer: Morphed track " << trackIndex << " (amount: " << morphAmount << ")\n";
}

void EuclideanSequencer::randomizePattern(int trackIndex, float amount) {
    validateTrackIndex(trackIndex);
    
    EuclideanPattern& pattern = trackPatterns_[trackIndex];
    amount = std::clamp(amount, 0.0f, 1.0f);
    
    // Randomize parameters
    if (amount > 0.0f) {
        float variation = amount * 0.3f; // Max 30% variation
        
        int pulsesVariation = int((dist_(rng_) - 0.5f) * variation * 16);
        pattern.pulses = std::clamp(pattern.pulses + pulsesVariation, 0, 16);
        
        int rotationVariation = int((dist_(rng_) - 0.5f) * variation * 16);
        pattern.rotation = std::clamp(pattern.rotation + rotationVariation, 0, 15);
        
        float probVariation = (dist_(rng_) - 0.5f) * variation;
        pattern.probability = std::clamp(pattern.probability + probVariation, 0.0f, 1.0f);
        
        pattern.isDirty = true;
        generatePattern(pattern, defaultAlgorithm_);
        
        std::cout << "EuclideanSequencer: Randomized track " << trackIndex << " (amount: " << amount << ")\n";
    }
}

void EuclideanSequencer::processHardwareInput(int keyIndex, bool pressed, int trackIndex) {
    if (keyIndex < 0 || keyIndex >= 16 || trackIndex < 0 || trackIndex >= 8) return;
    
    if (pressed) {
        EuclideanPattern& pattern = trackPatterns_[trackIndex];
        
        // Map key index to euclidean parameters
        if (keyIndex < 8) {
            // First 8 keys control pulses (1-8)
            pattern.pulses = keyIndex + 1;
        } else {
            // Last 8 keys control rotation (0-7, scaled to 0-15)
            pattern.rotation = (keyIndex - 8) * 2;
        }
        
        pattern.isDirty = true;
        generatePattern(pattern, defaultAlgorithm_);
        
        std::cout << "EuclideanSequencer: Hardware input - key " << keyIndex << " on track " << trackIndex << "\n";
    }
}

void EuclideanSequencer::visualizePattern(int trackIndex, uint32_t* displayBuffer, int width, int height) const {
    validateTrackIndex(trackIndex);
    
    if (!displayBuffer || width <= 0 || height <= 0) return;
    
    const EuclideanPattern& pattern = trackPatterns_[trackIndex];
    
    // Simple visualization - 16 horizontal segments
    int segmentWidth = width / 16;
    uint32_t activeColor = 0xFF6B73;   // Warm red
    uint32_t inactiveColor = 0x333333; // Dark gray
    
    for (int step = 0; step < 16; step++) {
        uint32_t color = pattern.pattern[step] ? activeColor : inactiveColor;
        
        // Fill segment
        for (int x = step * segmentWidth; x < (step + 1) * segmentWidth && x < width; x++) {
            for (int y = 0; y < height; y++) {
                if (y * width + x < width * height) {
                    displayBuffer[y * width + x] = color;
                }
            }
        }
    }
}

void EuclideanSequencer::enablePolyrhythm(int trackIndex, bool enabled) {
    validateTrackIndex(trackIndex);
    
    polyrhythmEnabled_[trackIndex] = enabled;
    
    std::cout << "EuclideanSequencer: " << (enabled ? "Enabled" : "Disabled") << " polyrhythm for track " << trackIndex << "\n";
}

void EuclideanSequencer::setPatternOffset(int trackIndex, int offset) {
    validateTrackIndex(trackIndex);
    
    patternOffsets_[trackIndex] = std::clamp(offset, 0, 15);
    
    std::cout << "EuclideanSequencer: Set pattern offset " << offset << " for track " << trackIndex << "\n";
}

void EuclideanSequencer::linkPatterns(int track1, int track2, bool linked) {
    validateTrackIndex(track1);
    validateTrackIndex(track2);
    
    linkedTracks_[track1][track2] = linked ? 1 : 0;
    linkedTracks_[track2][track1] = linked ? 1 : 0; // Bidirectional link
    
    std::cout << "EuclideanSequencer: " << (linked ? "Linked" : "Unlinked") << " tracks " << track1 << " and " << track2 << "\n";
}

} // namespace EtherSynth

// Preset implementations
namespace EtherSynth {
namespace EuclideanPresets {

EuclideanSequencer::EuclideanPattern fourOnFloor() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 4;
    pattern.rotation = 0;
    pattern.isDirty = true;
    return pattern;
}

EuclideanSequencer::EuclideanPattern offBeatHats() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 8;
    pattern.rotation = 2;
    pattern.isDirty = true;
    return pattern;
}

EuclideanSequencer::EuclideanPattern snareBackbeat() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 2;
    pattern.rotation = 8;
    pattern.isDirty = true;
    return pattern;
}

EuclideanSequencer::EuclideanPattern clave() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 5;
    pattern.rotation = 0;
    pattern.isDirty = true;
    return pattern;
}

EuclideanSequencer::EuclideanPattern tresillo() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 3;
    pattern.rotation = 0;
    pattern.isDirty = true;
    return pattern;
}

EuclideanSequencer::EuclideanPattern fiveAgainstFour() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 5;
    pattern.rotation = 0;
    pattern.isDirty = true;
    return pattern;
}

EuclideanSequencer::EuclideanPattern sevenEight() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 7;
    pattern.rotation = 0;
    pattern.isDirty = true;
    return pattern;
}

EuclideanSequencer::EuclideanPattern goldenRatio() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 3;
    pattern.rotation = 5;
    pattern.isDirty = true;
    return pattern;
}

EuclideanSequencer::EuclideanPattern arpeggioPattern() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 6;
    pattern.rotation = 1;
    pattern.isDirty = true;
    return pattern;
}

EuclideanSequencer::EuclideanPattern bassLine() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 3;
    pattern.rotation = 0;
    pattern.isDirty = true;
    return pattern;
}

EuclideanSequencer::EuclideanPattern ambientPulse() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 2;
    pattern.rotation = 0;
    pattern.probability = 0.8f;
    pattern.isDirty = true;
    return pattern;
}

EuclideanSequencer::EuclideanPattern randomWalk() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 4;
    pattern.rotation = 0;
    pattern.probability = 0.75f;
    pattern.velocityVariation = 0.3f;
    pattern.humanization = 0.2f;
    pattern.isDirty = true;
    return pattern;
}

EuclideanSequencer::EuclideanPattern fibonacci() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 5;
    pattern.rotation = 8;
    pattern.isDirty = true;
    return pattern;
}

EuclideanSequencer::EuclideanPattern primePulses() {
    EuclideanSequencer::EuclideanPattern pattern;
    pattern.pulses = 7;
    pattern.rotation = 2;
    pattern.isDirty = true;
    return pattern;
}

} // namespace EuclideanPresets
} // namespace EtherSynth