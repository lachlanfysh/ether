#include "PatternChainManager.h"
#include "../patterns/PatternBlock.h"
#include "../core/Track.h"
#include <algorithm>
#include <random>
#include <cmath>
#include <iostream>

namespace EtherSynth {

PatternChainManager::PatternChainManager() {
    // Initialize default chain modes for all tracks
    for (int i = 0; i < 8; ++i) {
        chainModes_[i] = ChainMode::MANUAL;
        currentPatterns_[i] = 0;
        queuedPatterns_[i] = 0;
        armedPatterns_[i] = 0;
        chainProgressions_[i] = 0.0f;
    }
    
    // Initialize performance metrics
    performanceMetrics_ = {};
    
    std::cout << "PatternChainManager: Initialized with intelligent chaining system" << std::endl;
}

// MARK: - Chain Management

void PatternChainManager::createChain(uint32_t startPatternId, const std::vector<uint32_t>& patternIds) {
    if (patternIds.empty()) return;
    
    std::vector<ChainLink> chain;
    
    // Create chain links connecting each pattern to the next
    for (size_t i = 0; i < patternIds.size(); ++i) {
        uint32_t currentId = patternIds[i];
        uint32_t nextId = (i + 1 < patternIds.size()) ? patternIds[i + 1] : patternIds[0]; // Loop back to start
        
        ChainLink link(currentId, nextId);
        link.name = "Chain " + std::to_string(i + 1);
        link.chainColor = 0x4A90E2 + (i * 0x111111); // Vary color slightly
        
        chain.push_back(link);
    }
    
    patternChains_[startPatternId] = chain;
    
    std::cout << "PatternChainManager: Created chain starting with pattern " << startPatternId 
              << " containing " << chain.size() << " links" << std::endl;
}

void PatternChainManager::addChainLink(uint32_t fromPattern, uint32_t toPattern) {
    ChainLink newLink(fromPattern, toPattern);
    newLink.name = "Link " + std::to_string(fromPattern) + "→" + std::to_string(toPattern);
    
    patternChains_[fromPattern].push_back(newLink);
    
    std::cout << "PatternChainManager: Added chain link " << fromPattern << " → " << toPattern << std::endl;
}

void PatternChainManager::setChainCondition(uint32_t fromPattern, uint32_t toPattern, std::function<bool()> condition) {
    auto& chain = patternChains_[fromPattern];
    for (auto& link : chain) {
        if (link.nextPatternId == toPattern) {
            link.hasCondition = true;
            link.condition = condition;
            std::cout << "PatternChainManager: Set condition for chain " << fromPattern << " → " << toPattern << std::endl;
            break;
        }
    }
}

// MARK: - Chain Playback Control

void PatternChainManager::startChain(uint32_t startPatternId, int trackIndex) {
    if (trackIndex < 0 || trackIndex >= 8) return;
    
    currentPatterns_[trackIndex] = startPatternId;
    chainProgressions_[trackIndex] = 0.0f;
    
    std::cout << "PatternChainManager: Started chain on track " << trackIndex 
              << " with pattern " << startPatternId << std::endl;
    
    performanceMetrics_.activeChains++;
}

void PatternChainManager::stopChain(int trackIndex) {
    if (trackIndex < 0 || trackIndex >= 8) return;
    
    currentPatterns_[trackIndex] = 0;
    queuedPatterns_[trackIndex] = 0;
    chainProgressions_[trackIndex] = 0.0f;
    
    performanceMetrics_.activeChains = std::max(0, performanceMetrics_.activeChains - 1);
    
    std::cout << "PatternChainManager: Stopped chain on track " << trackIndex << std::endl;
}

// MARK: - Pattern Triggering

void PatternChainManager::queuePattern(uint32_t patternId, int trackIndex, PatternTrigger trigger) {
    if (trackIndex < 0 || trackIndex >= 8) return;
    
    queuedPatterns_[trackIndex] = patternId;
    
    // Handle immediate trigger
    if (trigger == PatternTrigger::IMMEDIATE) {
        triggerPattern(patternId, trackIndex, true);
        return;
    }
    
    performanceMetrics_.queuedPatterns++;
    
    std::cout << "PatternChainManager: Queued pattern " << patternId << " on track " << trackIndex 
              << " with trigger mode " << static_cast<int>(trigger) << std::endl;
}

void PatternChainManager::triggerPattern(uint32_t patternId, int trackIndex, bool immediate) {
    if (trackIndex < 0 || trackIndex >= 8) return;
    
    if (immediate || isQuantizationPoint(chainProgressions_[trackIndex], globalQuantization_)) {
        transitionToPattern(patternId, trackIndex);
        queuedPatterns_[trackIndex] = 0; // Clear queue
        
        if (performanceMetrics_.queuedPatterns > 0) {
            performanceMetrics_.queuedPatterns--;
        }
    }
}

// MARK: - Chain Logic Processing

uint32_t PatternChainManager::getNextPattern(uint32_t currentPattern, int trackIndex) {
    auto chainIt = patternChains_.find(currentPattern);
    if (chainIt == patternChains_.end() || chainIt->second.empty()) {
        return currentPattern; // No chain defined, stay on current pattern
    }
    
    const auto& chain = chainIt->second;
    
    // Find appropriate chain link based on conditions and probability
    for (const auto& link : chain) {
        if (link.patternId == currentPattern) {
            // Check condition if present
            if (link.hasCondition && !link.condition()) {
                continue;
            }
            
            // Check probability
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dis(0.0f, 1.0f);
            
            if (dis(gen) <= link.probability) {
                return link.nextPatternId;
            }
        }
    }
    
    return currentPattern; // Fallback to current pattern
}

void PatternChainManager::processChainLogic(int trackIndex, float deltaTime) {
    if (trackIndex < 0 || trackIndex >= 8) return;
    if (chainModes_[trackIndex] == ChainMode::MANUAL) return;
    
    updateChainProgression(trackIndex, deltaTime);
    
    uint32_t currentPattern = currentPatterns_[trackIndex];
    if (currentPattern == 0) return;
    
    // Check if we should trigger the next pattern
    auto chainIt = patternChains_.find(currentPattern);
    if (chainIt != patternChains_.end()) {
        for (const auto& link : chainIt->second) {
            if (shouldTriggerNext(link, trackIndex)) {
                uint32_t nextPattern = getNextPattern(currentPattern, trackIndex);
                if (nextPattern != currentPattern) {
                    queuePattern(nextPattern, trackIndex, PatternTrigger::QUANTIZED);
                }
                break;
            }
        }
    }
    
    // Process queued patterns
    uint32_t queuedPattern = queuedPatterns_[trackIndex];
    if (queuedPattern != 0) {
        triggerPattern(queuedPattern, trackIndex, false);
    }
}

bool PatternChainManager::shouldTriggerNext(const ChainLink& link, int trackIndex) {
    // Check repeat count
    if (link.currentRepeats < link.repeatCount) {
        return false;
    }
    
    // Check quantization
    if (!isQuantizationPoint(chainProgressions_[trackIndex], globalQuantization_)) {
        return false;
    }
    
    // Check custom conditions
    if (link.hasCondition && !link.condition()) {
        return false;
    }
    
    return true;
}

// MARK: - Pattern Variations and Mutations

void PatternChainManager::generatePatternVariation(uint32_t sourcePatternId, float mutationAmount) {
    std::cout << "PatternChainManager: Generating variation of pattern " << sourcePatternId 
              << " with mutation amount " << mutationAmount << std::endl;
    
    // Apply mutation based on amount (0.0 = no change, 1.0 = maximum change)
    applyPatternMutation(sourcePatternId, mutationAmount);
}

void PatternChainManager::applyEuclideanRhythm(uint32_t patternId, int steps, int pulses, int rotation) {
    std::vector<bool> sequence = generateEuclideanSequence(steps, pulses, rotation);
    
    std::cout << "PatternChainManager: Applied Euclidean rhythm to pattern " << patternId 
              << " (" << pulses << " pulses in " << steps << " steps, rotation " << rotation << ")" << std::endl;
    
    // The sequence would be applied to the actual pattern data
    // This is a placeholder for the implementation
}

void PatternChainManager::morphPatternTiming(uint32_t patternId, float swingAmount, float humanizeAmount) {
    std::cout << "PatternChainManager: Morphing timing for pattern " << patternId 
              << " (swing: " << swingAmount << ", humanize: " << humanizeAmount << ")" << std::endl;
    
    // Apply swing and humanization to pattern timing
    // This would modify the actual pattern timing data
}

// MARK: - Song Arrangement Mode

uint32_t PatternChainManager::createSection(SectionType type, const std::string& name, const std::vector<uint32_t>& patterns) {
    SongSection section;
    section.type = type;
    section.name = name;
    section.patternIds = patterns;
    section.id = nextSectionId_++;
    section.sectionColor = getSectionTypeColor(type);
    
    sections_.push_back(section);
    
    std::cout << "PatternChainManager: Created section '" << name << "' (ID: " << section.id 
              << ") with " << patterns.size() << " patterns" << std::endl;
    
    return section.id;
}

void PatternChainManager::arrangeSection(uint32_t sectionId, int position) {
    if (position < 0) {
        arrangementOrder_.push_back(sectionId);
    } else if (position < static_cast<int>(arrangementOrder_.size())) {
        arrangementOrder_.insert(arrangementOrder_.begin() + position, sectionId);
    } else {
        arrangementOrder_.push_back(sectionId);
    }
    
    std::cout << "PatternChainManager: Arranged section " << sectionId << " at position " << position << std::endl;
}

// MARK: - Scene Management

uint32_t PatternChainManager::saveScene(const std::string& name) {
    Scene scene;
    scene.name = name;
    scene.id = nextSceneId_++;
    
    // Capture current state
    scene.trackPatterns = currentPatterns_;
    
    // Capture other state (would be filled from actual system state)
    for (int i = 0; i < 8; ++i) {
        scene.trackVolumes[i] = 0.8f;  // Default volume
        scene.trackMutes[i] = false;   // Default unmuted
    }
    
    scenes_[scene.id] = scene;
    
    std::cout << "PatternChainManager: Saved scene '" << name << "' (ID: " << scene.id << ")" << std::endl;
    
    return scene.id;
}

bool PatternChainManager::loadScene(uint32_t sceneId) {
    auto it = scenes_.find(sceneId);
    if (it == scenes_.end()) {
        std::cout << "PatternChainManager: Scene " << sceneId << " not found" << std::endl;
        return false;
    }
    
    const Scene& scene = it->second;
    
    // Restore pattern state
    currentPatterns_ = scene.trackPatterns;
    
    // Would also restore track volumes, mutes, effects, etc.
    
    std::cout << "PatternChainManager: Loaded scene '" << scene.name << "' (ID: " << sceneId << ")" << std::endl;
    
    return true;
}

// MARK: - Live Performance Features

void PatternChainManager::armPattern(uint32_t patternId, int trackIndex) {
    if (trackIndex < 0 || trackIndex >= 8) return;
    
    armedPatterns_[trackIndex] = patternId;
    
    std::cout << "PatternChainManager: Armed pattern " << patternId << " on track " << trackIndex << std::endl;
}

void PatternChainManager::launchArmedPatterns() {
    for (int i = 0; i < 8; ++i) {
        uint32_t armedPattern = armedPatterns_[i];
        if (armedPattern != 0) {
            queuePattern(armedPattern, i, PatternTrigger::QUANTIZED);
            armedPatterns_[i] = 0; // Disarm after queuing
        }
    }
    
    std::cout << "PatternChainManager: Launched all armed patterns" << std::endl;
}

// MARK: - Pattern Analysis and Intelligence

PatternChainManager::PatternAnalysis PatternChainManager::analyzePattern(uint32_t patternId) {
    PatternAnalysis analysis;
    
    // Placeholder analysis - would analyze actual pattern data
    analysis.complexity = 0.7f;      // 0.0-1.0 complexity score
    analysis.energy = 0.8f;          // Energy level
    analysis.density = 0.6f;         // Note density
    analysis.dominantScale = 0;      // C major
    analysis.tempo = currentTempo_;  // Current tempo
    
    // AI-generated suggestions (placeholder)
    analysis.suggestedChains = {patternId + 1, patternId + 2, patternId - 1};
    
    patternAnalysisCache_[patternId] = analysis;
    
    return analysis;
}

std::vector<uint32_t> PatternChainManager::getSuggestedNextPatterns(uint32_t currentPattern, int count) {
    std::vector<uint32_t> suggestions;
    
    // Simple algorithm - would be more sophisticated in practice
    for (int i = 1; i <= count && i <= 10; ++i) {
        suggestions.push_back(currentPattern + i);
    }
    
    return suggestions;
}

// MARK: - Hardware Interface Integration

void PatternChainManager::processHardwareInput(int keyIndex, bool pressed, int trackIndex) {
    if (!pressed || trackIndex < 0 || trackIndex >= 8) return;
    
    // Map key index to pattern ID (simplified)
    uint32_t patternId = static_cast<uint32_t>(keyIndex + 1);
    
    if (performanceMode_) {
        // In performance mode, arm patterns for synchronized launch
        armPattern(patternId, trackIndex);
    } else {
        // Direct pattern triggering
        queuePattern(patternId, trackIndex, PatternTrigger::QUANTIZED);
    }
    
    std::cout << "PatternChainManager: Hardware key " << keyIndex << " triggered pattern " 
              << patternId << " on track " << trackIndex << std::endl;
}

void PatternChainManager::processSmartKnobInput(float value, int trackIndex) {
    if (trackIndex < 0 || trackIndex >= 8) return;
    
    uint32_t currentPattern = currentPatterns_[trackIndex];
    if (currentPattern == 0) return;
    
    // Use SmartKnob for pattern parameter morphing
    float mutationAmount = value; // 0.0-1.0
    generatePatternVariation(currentPattern, mutationAmount);
    
    std::cout << "PatternChainManager: SmartKnob morphing pattern " << currentPattern 
              << " on track " << trackIndex << " (amount: " << value << ")" << std::endl;
}

// MARK: - Helper Methods

void PatternChainManager::updateChainProgression(int trackIndex, float deltaTime) {
    float& progression = chainProgressions_[trackIndex];
    progression += deltaTime / (60.0f / currentTempo_); // Normalize to bars
    
    if (progression >= 1.0f) {
        progression = 0.0f; // Reset for next bar
    }
}

bool PatternChainManager::isQuantizationPoint(float currentTime, int quantization) {
    float quantizePoint = 1.0f / static_cast<float>(quantization);
    float modulo = fmod(currentTime, quantizePoint);
    return modulo < 0.01f; // Small tolerance for timing precision
}

void PatternChainManager::transitionToPattern(uint32_t patternId, int trackIndex) {
    uint32_t previousPattern = currentPatterns_[trackIndex];
    currentPatterns_[trackIndex] = patternId;
    chainProgressions_[trackIndex] = 0.0f;
    
    performanceMetrics_.totalTransitions++;
    
    std::cout << "PatternChainManager: Transitioned from pattern " << previousPattern 
              << " to " << patternId << " on track " << trackIndex << std::endl;
}

// MARK: - Euclidean Rhythm Generation

std::vector<bool> PatternChainManager::generateEuclideanSequence(int steps, int pulses, int rotation) {
    if (pulses >= steps) {
        // All steps are pulses
        return std::vector<bool>(steps, true);
    }
    
    if (pulses <= 0) {
        // No pulses
        return std::vector<bool>(steps, false);
    }
    
    std::vector<bool> sequence(steps, false);
    
    // Euclidean algorithm implementation
    int bucket = 0;
    for (int i = 0; i < steps; ++i) {
        bucket += pulses;
        if (bucket >= steps) {
            bucket -= steps;
            sequence[i] = true;
        }
    }
    
    // Apply rotation
    if (rotation > 0) {
        rotation = rotation % steps;
        std::rotate(sequence.begin(), sequence.begin() + rotation, sequence.end());
    }
    
    return sequence;
}

// MARK: - Performance Metrics

PatternChainManager::ChainMetrics PatternChainManager::getPerformanceMetrics() const {
    ChainMetrics metrics = performanceMetrics_;
    
    // Calculate average chain length
    float totalLength = 0.0f;
    int chainCount = 0;
    for (const auto& chain : patternChains_) {
        totalLength += chain.second.size();
        chainCount++;
    }
    metrics.averageChainLength = chainCount > 0 ? totalLength / chainCount : 0.0f;
    
    // Calculate performance stability (placeholder)
    metrics.performanceStability = 0.95f; // High stability
    
    return metrics;
}

void PatternChainManager::resetMetrics() {
    performanceMetrics_ = {};
    std::cout << "PatternChainManager: Reset performance metrics" << std::endl;
}

// MARK: - Utility Functions

const char* chainModeToString(PatternChainManager::ChainMode mode) {
    static const char* modeNames[] = {
        "Manual", "Automatic", "Conditional", "Performance", "Arrangement"
    };
    int index = static_cast<int>(mode);
    return (index >= 0 && index < static_cast<int>(PatternChainManager::ChainMode::COUNT)) 
           ? modeNames[index] : "Unknown";
}

const char* triggerModeToString(PatternChainManager::PatternTrigger trigger) {
    static const char* triggerNames[] = {
        "Immediate", "Quantized", "Queued", "Conditional"
    };
    int index = static_cast<int>(trigger);
    return (index >= 0 && index < static_cast<int>(PatternChainManager::PatternTrigger::COUNT))
           ? triggerNames[index] : "Unknown";
}

const char* sectionTypeToString(PatternChainManager::SectionType type) {
    static const char* sectionNames[] = {
        "Intro", "Verse", "Chorus", "Bridge", "Breakdown", "Build", "Drop", "Outro", "Custom"
    };
    int index = static_cast<int>(type);
    return (index >= 0 && index < static_cast<int>(PatternChainManager::SectionType::COUNT))
           ? sectionNames[index] : "Unknown";
}

uint32_t getSectionTypeColor(PatternChainManager::SectionType type) {
    static const uint32_t sectionColors[] = {
        0x4A90E2, // Intro - Blue
        0x7ED321, // Verse - Green  
        0xF5A623, // Chorus - Orange
        0xD0021B, // Bridge - Red
        0x9013FE, // Breakdown - Purple
        0x50E3C2, // Build - Teal
        0xB8E986, // Drop - Light Green
        0x4A4A4A, // Outro - Gray
        0x666666  // Custom - Dark Gray
    };
    int index = static_cast<int>(type);
    return (index >= 0 && index < static_cast<int>(PatternChainManager::SectionType::COUNT))
           ? sectionColors[index] : 0x888888;
}

// MARK: - Chain Templates

namespace ChainTemplates {

void createBasicLoop(PatternChainManager& manager, const std::vector<uint32_t>& patterns) {
    if (patterns.empty()) return;
    
    manager.createChain(patterns[0], patterns);
    std::cout << "ChainTemplates: Created basic loop with " << patterns.size() << " patterns" << std::endl;
}

void createVerseChorus(PatternChainManager& manager, uint32_t verse, uint32_t chorus) {
    std::vector<uint32_t> structure = {verse, verse, chorus, verse, chorus, chorus};
    manager.createChain(verse, structure);
    std::cout << "ChainTemplates: Created verse-chorus structure" << std::endl;
}

void createBuildAndDrop(PatternChainManager& manager, const std::vector<uint32_t>& buildPatterns, uint32_t dropPattern) {
    std::vector<uint32_t> structure = buildPatterns;
    structure.push_back(dropPattern);
    
    if (!buildPatterns.empty()) {
        manager.createChain(buildPatterns[0], structure);
    }
    
    std::cout << "ChainTemplates: Created build and drop with " << buildPatterns.size() 
              << " build patterns and drop pattern " << dropPattern << std::endl;
}

} // namespace ChainTemplates

} // namespace EtherSynth