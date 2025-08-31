#pragma once
#include "../core/Types.h"
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <functional>

/**
 * PatternChainManager - Advanced pattern chaining and arrangement system
 * 
 * Professional groovebox workflow for EtherSynth V1.0:
 * - Intelligent pattern chaining with conditional logic
 * - Song arrangement mode with verse/chorus/bridge sections
 * - Live performance pattern triggering and queuing
 * - Pattern mutations and variations generation
 * - Scene snapshots for instant workflow switching
 * - Euclidean rhythm generation and pattern morphing
 * 
 * Hardware integration:
 * - 960×320 visual feedback for pattern chains
 * - 2×16 key interface for pattern triggering
 * - SmartKnob for pattern parameter morphing
 * - Real-time pattern switching without audio dropouts
 */

namespace EtherSynth {

// Forward declarations
class PatternBlock;
class Track;

class PatternChainManager {
public:
    enum class ChainMode : uint8_t {
        MANUAL = 0,           // Manual pattern triggering
        AUTOMATIC,            // Automatic pattern progression
        CONDITIONAL,          // Conditional chaining based on rules
        PERFORMANCE,          // Live performance mode
        ARRANGEMENT,          // Song arrangement mode
        COUNT
    };
    
    enum class PatternTrigger : uint8_t {
        IMMEDIATE = 0,        // Trigger immediately
        QUANTIZED,           // Trigger on next bar/beat
        QUEUED,              // Queue for later execution
        CONDITIONAL,         // Conditional trigger
        COUNT
    };
    
    enum class SectionType : uint8_t {
        INTRO = 0,
        VERSE,
        CHORUS, 
        BRIDGE,
        BREAKDOWN,
        BUILD,
        DROP,
        OUTRO,
        CUSTOM,
        COUNT
    };
    
    // Pattern chain link - connects patterns with conditions and parameters
    struct ChainLink {
        uint32_t patternId;                    // Source pattern ID
        uint32_t nextPatternId;                // Next pattern in chain
        
        // Playback parameters
        int repeatCount = 1;                   // How many times to repeat
        int currentRepeats = 0;                // Current repeat counter
        float probability = 1.0f;              // Probability of triggering (0.0-1.0)
        PatternTrigger triggerMode = PatternTrigger::QUANTIZED;
        
        // Conditional logic
        bool hasCondition = false;
        std::function<bool()> condition;       // Custom condition function
        
        // Variation parameters
        float mutationAmount = 0.0f;           // Amount of pattern mutation
        float swingAmount = 0.0f;              // Swing timing adjustment
        float velocityScale = 1.0f;            // Velocity scaling factor
        
        // Visual feedback
        uint32_t chainColor = 0xFFFFFF;        // Color for chain visualization
        std::string name = "Chain Link";       // User-defined name
        
        ChainLink(uint32_t pattern, uint32_t next) 
            : patternId(pattern), nextPatternId(next) {}
    };
    
    // Song section for arrangement mode
    struct SongSection {
        SectionType type = SectionType::VERSE;
        std::string name = "Section";
        std::vector<uint32_t> patternIds;      // Patterns in this section
        int barLength = 16;                    // Length in bars
        float tempo = 120.0f;                  // Tempo for this section
        
        // Section-specific effects
        float reverbSend = 0.0f;
        float delaySend = 0.0f;
        float filterCutoff = 1.0f;
        
        uint32_t id;                           // Unique section ID
        uint32_t sectionColor = 0x666666;     // Color for section visualization
    };
    
    // Scene snapshot - complete state capture
    struct Scene {
        std::string name = "Scene";
        std::map<int, uint32_t> trackPatterns;      // Active pattern per track
        std::map<int, float> trackVolumes;          // Track volume levels
        std::map<int, bool> trackMutes;             // Track mute states
        
        // Global parameters
        float masterVolume = 0.8f;
        float masterTempo = 120.0f;
        
        // Effects state
        float reverbSend = 0.0f;
        float delaySend = 0.0f;
        std::map<std::string, float> effectParameters;
        
        // Performance state
        bool noteRepeatActive = false;
        int noteRepeatDivision = 4;
        
        uint32_t id;
        uint32_t sceneColor = 0x888888;
        
        Scene() : id(generateSceneId()) {}
        
    private:
        static uint32_t generateSceneId() {
            static uint32_t counter = 1;
            return counter++;
        }
    };
    
    PatternChainManager();
    ~PatternChainManager() = default;
    
    // Chain management
    void createChain(uint32_t startPatternId, const std::vector<uint32_t>& patternIds);
    void addChainLink(uint32_t fromPattern, uint32_t toPattern);
    void removeChainLink(uint32_t fromPattern, uint32_t toPattern);
    void setChainCondition(uint32_t fromPattern, uint32_t toPattern, std::function<bool()> condition);
    
    // Chain playback control
    void startChain(uint32_t startPatternId, int trackIndex);
    void stopChain(int trackIndex);
    void pauseChain(int trackIndex);
    void resumeChain(int trackIndex);
    
    // Pattern triggering
    void queuePattern(uint32_t patternId, int trackIndex, PatternTrigger trigger = PatternTrigger::QUANTIZED);
    void triggerPattern(uint32_t patternId, int trackIndex, bool immediate = false);
    void cancelQueuedPattern(int trackIndex);
    
    // Chain progression
    uint32_t getNextPattern(uint32_t currentPattern, int trackIndex);
    void processChainLogic(int trackIndex, float deltaTime);
    bool shouldTriggerNext(const ChainLink& link, int trackIndex);
    
    // Pattern variations and mutations
    void generatePatternVariation(uint32_t sourcePatternId, float mutationAmount);
    void applyEuclideanRhythm(uint32_t patternId, int steps, int pulses, int rotation = 0);
    void morphPatternTiming(uint32_t patternId, float swingAmount, float humanizeAmount);
    
    // Song arrangement mode
    uint32_t createSection(SectionType type, const std::string& name, const std::vector<uint32_t>& patterns);
    void arrangeSection(uint32_t sectionId, int position);
    void setArrangementMode(bool enabled) { arrangementMode_ = enabled; }
    void playArrangement(int startSection = 0);
    void stopArrangement();
    
    // Scene management
    uint32_t saveScene(const std::string& name);
    bool loadScene(uint32_t sceneId);
    void deleteScene(uint32_t sceneId);
    const Scene* getScene(uint32_t sceneId) const;
    std::vector<Scene> getAllScenes() const;
    
    // Live performance features
    void setPerformanceMode(bool enabled) { performanceMode_ = enabled; }
    void armPattern(uint32_t patternId, int trackIndex); // Arm pattern for next trigger
    void launchArmedPatterns();                          // Launch all armed patterns
    void setGlobalQuantization(int bars) { globalQuantization_ = bars; }
    
    // Pattern analysis and intelligence
    struct PatternAnalysis {
        float complexity;        // Pattern complexity score
        float energy;           // Energy level
        float density;          // Note density
        int dominantScale;      // Detected scale/key
        float tempo;            // Suggested tempo
        std::vector<int> suggestedChains; // AI suggested next patterns
    };
    
    PatternAnalysis analyzePattern(uint32_t patternId);
    std::vector<uint32_t> getSuggestedNextPatterns(uint32_t currentPattern, int count = 4);
    void generateIntelligentChain(uint32_t startPattern, int chainLength);
    
    // Hardware interface integration
    void processHardwareInput(int keyIndex, bool pressed, int trackIndex);
    void processSmartKnobInput(float value, int trackIndex);
    std::vector<uint32_t> getVisiblePatterns(int trackIndex) const;
    
    // Chain state queries
    ChainMode getChainMode(int trackIndex) const;
    void setChainMode(int trackIndex, ChainMode mode);
    uint32_t getCurrentPattern(int trackIndex) const;
    uint32_t getQueuedPattern(int trackIndex) const;
    bool isChainActive(int trackIndex) const;
    float getChainProgress(int trackIndex) const;
    
    // Preset management
    void saveChainPreset(const std::string& name);
    bool loadChainPreset(const std::string& name);
    std::vector<std::string> getChainPresetNames() const;
    
    // Performance metrics
    struct ChainMetrics {
        int activeChains;
        int queuedPatterns;
        float averageChainLength;
        int totalTransitions;
        float performanceStability; // How smooth transitions are
    };
    
    ChainMetrics getPerformanceMetrics() const;
    void resetMetrics();
    
private:
    // Chain data
    std::map<uint32_t, std::vector<ChainLink>> patternChains_;
    std::map<int, uint32_t> currentPatterns_;      // Current pattern per track
    std::map<int, uint32_t> queuedPatterns_;       // Queued pattern per track
    std::map<int, uint32_t> armedPatterns_;        // Armed pattern per track
    std::map<int, ChainMode> chainModes_;          // Chain mode per track
    
    // Section and arrangement data
    std::vector<SongSection> sections_;
    std::map<uint32_t, Scene> scenes_;
    std::vector<uint32_t> arrangementOrder_;
    bool arrangementMode_ = false;
    int currentSectionIndex_ = 0;
    
    // Performance state
    bool performanceMode_ = false;
    int globalQuantization_ = 1;                   // Bars
    std::map<int, float> chainProgressions_;       // Progress per track (0.0-1.0)
    
    // Timing and synchronization
    float sampleRate_ = 48000.0f;
    float currentTempo_ = 120.0f;
    uint64_t sampleCounter_ = 0;
    
    // Pattern intelligence
    std::map<uint32_t, PatternAnalysis> patternAnalysisCache_;
    
    // Chain statistics
    mutable ChainMetrics performanceMetrics_;
    uint32_t nextSectionId_ = 1;
    uint32_t nextSceneId_ = 1;
    
    // Helper methods
    void updateChainProgression(int trackIndex, float deltaTime);
    bool isQuantizationPoint(float currentTime, int quantization);
    void transitionToPattern(uint32_t patternId, int trackIndex);
    void applyPatternMutation(uint32_t patternId, float amount);
    void calculatePatternCompatibility();
    
    // Euclidean rhythm generation
    std::vector<bool> generateEuclideanSequence(int steps, int pulses, int rotation = 0);
    
    // AI/ML pattern suggestion algorithms
    float calculatePatternSimilarity(uint32_t pattern1, uint32_t pattern2);
    std::vector<uint32_t> findCompatiblePatterns(uint32_t sourcePattern);
};

// Utility functions for UI integration
const char* chainModeToString(PatternChainManager::ChainMode mode);
const char* triggerModeToString(PatternChainManager::PatternTrigger trigger);
const char* sectionTypeToString(PatternChainManager::SectionType type);
uint32_t getSectionTypeColor(PatternChainManager::SectionType type);

// Pattern chain templates for quick setup
namespace ChainTemplates {
    void createBasicLoop(PatternChainManager& manager, const std::vector<uint32_t>& patterns);
    void createVerseChorus(PatternChainManager& manager, uint32_t verse, uint32_t chorus);
    void createBuildAndDrop(PatternChainManager& manager, const std::vector<uint32_t>& buildPatterns, uint32_t dropPattern);
    void createProgressiveHouse(PatternChainManager& manager, const std::vector<uint32_t>& patterns);
    void createDrumAndBass(PatternChainManager& manager, const std::vector<uint32_t>& patterns);
}

} // namespace EtherSynth