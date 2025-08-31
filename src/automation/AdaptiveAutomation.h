#pragma once
#include "../analysis/SpectrumAnalyzer.h"
#include <vector>
#include <array>
#include <memory>
#include <functional>
#include <map>
#include <mutex>
#include <atomic>

/**
 * Adaptive Parameter Automation for EtherSynth V1.0
 * 
 * AI-powered parameter automation that responds to:
 * - Real-time spectrum analysis
 * - Musical context and genre detection
 * - Performance patterns and user behavior
 * - Mix balance and frequency content
 * 
 * Features:
 * - Intelligent parameter mapping based on audio analysis
 * - Adaptive filtering and EQ adjustments
 * - Dynamic range management
 * - Frequency-aware modulation routing
 * - Real-time mix enhancement suggestions
 */

namespace EtherSynth {

class AdaptiveAutomation {
public:
    // Automation mode types
    enum class Mode : uint8_t {
        DISABLED = 0,
        GENTLE,          // Subtle adjustments
        MODERATE,        // Noticeable improvements
        AGGRESSIVE,      // Strong corrections
        CREATIVE,        // Artistic interpretations
        MASTERING        // Final mix enhancements
    };
    
    // Parameter automation targets
    enum class Target : uint8_t {
        // Engine parameters
        ENGINE_FILTER_CUTOFF = 0,
        ENGINE_FILTER_RESONANCE,
        ENGINE_AMPLITUDE,
        ENGINE_PITCH,
        ENGINE_TIMBRE,
        ENGINE_MODULATION_DEPTH,
        
        // Effects parameters
        FX_REVERB_SIZE,
        FX_REVERB_DAMPING,
        FX_DELAY_TIME,
        FX_DELAY_FEEDBACK,
        FX_COMPRESSOR_THRESHOLD,
        FX_COMPRESSOR_RATIO,
        FX_EQ_LOW_GAIN,
        FX_EQ_MID_GAIN,
        FX_EQ_HIGH_GAIN,
        
        // Mix parameters
        MIX_TRACK_LEVEL,
        MIX_TRACK_PAN,
        MIX_SEND_LEVEL,
        
        COUNT
    };
    
    // Automation rule configuration
    struct AutomationRule {
        Target target = Target::ENGINE_FILTER_CUTOFF;
        int trackIndex = 0;
        int parameterIndex = 0;
        
        // Spectrum analysis triggers
        float bassEnergyMin = 0.0f;         // Minimum bass energy to activate
        float bassEnergyMax = 1.0f;         // Maximum bass energy
        float midEnergyMin = 0.0f;          // Minimum mid energy
        float midEnergyMax = 1.0f;          // Maximum mid energy
        float highEnergyMin = 0.0f;         // Minimum high energy
        float highEnergyMax = 1.0f;         // Maximum high energy
        
        float centroidMin = 0.0f;           // Minimum spectral centroid (Hz)
        float centroidMax = 20000.0f;       // Maximum spectral centroid (Hz)
        float spreadMin = 0.0f;             // Minimum spectral spread
        float spreadMax = 10000.0f;         // Maximum spectral spread
        
        // Parameter mapping
        float outputMin = 0.0f;             // Minimum output value
        float outputMax = 1.0f;             // Maximum output value
        float sensitivity = 1.0f;           // Response sensitivity
        float smoothing = 0.9f;             // Temporal smoothing
        bool inverted = false;              // Invert response
        
        // Conditions
        bool requiresActivity = true;       // Only activate when signal present
        float activityThreshold = 0.001f;   // Minimum signal level
        
        // Timing
        float attackTime = 0.1f;            // Attack time (seconds)
        float releaseTime = 0.5f;           // Release time (seconds)
        
        bool enabled = true;                // Rule enabled
        
        AutomationRule() = default;
        AutomationRule(Target t, int track, int param) 
            : target(t), trackIndex(track), parameterIndex(param) {}
    };
    
    // Intelligent automation suggestions
    struct AutomationSuggestion {
        std::string description;            // Human-readable description
        Target target;                      // Parameter to automate
        int trackIndex;                     // Target track
        int parameterIndex;                 // Target parameter
        AutomationRule suggestedRule;       // Suggested automation rule
        float confidence;                   // AI confidence (0-1)
        float potentialImprovement;         // Expected improvement
        bool isEssential;                   // Critical for mix
        
        std::string reason;                 // Why this is suggested
        std::vector<std::string> alternatives; // Alternative approaches
    };
    
    AdaptiveAutomation();
    ~AdaptiveAutomation();
    
    // Core functionality
    void initialize(std::shared_ptr<SpectrumAnalyzer> analyzer);
    void shutdown();
    void update(float deltaTime);
    
    // Mode management
    void setMode(Mode mode);
    Mode getMode() const { return currentMode_; }
    void setIntensity(float intensity);  // 0.0 = off, 1.0 = maximum
    float getIntensity() const { return intensity_; }
    
    // Rule management
    void addAutomationRule(const AutomationRule& rule);
    void removeAutomationRule(int ruleIndex);
    void updateAutomationRule(int ruleIndex, const AutomationRule& rule);
    const std::vector<AutomationRule>& getAutomationRules() const { return automationRules_; }
    void enableRule(int ruleIndex, bool enabled);
    void clearAllRules();
    
    // Preset rule collections
    void loadPresetRules(const std::string& presetName);
    void savePresetRules(const std::string& presetName);
    std::vector<std::string> getAvailablePresets() const;
    
    // AI suggestions
    std::vector<AutomationSuggestion> generateSuggestions();
    void applySuggestion(const AutomationSuggestion& suggestion);
    void dismissSuggestion(int suggestionIndex);
    
    // Parameter automation callback
    using AutomationCallback = std::function<void(Target target, int trackIndex, 
                                                 int parameterIndex, float value)>;
    void setAutomationCallback(AutomationCallback callback);
    
    // Analysis integration
    void processSpectrum(const SpectrumAnalyzer::SpectrumData& spectrum,
                        const SpectrumAnalyzer::AudioFeatures& features);
    
    // Performance monitoring
    float getProcessingLoad() const;
    int getActiveRuleCount() const;
    
    // Genre and context detection
    enum class MusicGenre : uint8_t {
        UNKNOWN = 0, HOUSE, TECHNO, AMBIENT, DRUM_AND_BASS, 
        TRAP, DUBSTEP, MINIMAL, EXPERIMENTAL, JAZZ, CLASSICAL
    };
    
    MusicGenre detectGenre() const;
    float getGenreConfidence() const;
    void setGenreHint(MusicGenre genre, float confidence);
    
    // Mix analysis and suggestions
    struct MixAnalysis {
        float bassBalance = 0.0f;           // -1 = too little, +1 = too much
        float midBalance = 0.0f;            // Mid-frequency balance
        float highBalance = 0.0f;           // High-frequency balance
        float stereoWidth = 0.0f;           // Stereo field utilization
        float dynamicRange = 0.0f;          // Dynamic range score
        float loudness = 0.0f;              // Perceptual loudness
        float clarity = 0.0f;               // Mix clarity score
        
        bool hasClipping = false;           // Digital clipping detected
        bool hasImbalance = false;          // Frequency imbalance
        bool needsCompression = false;      // Dynamic range issues
        bool needsEQ = false;               // EQ correction needed
        
        std::vector<std::string> suggestions; // Textual suggestions
    };
    
    MixAnalysis analyzeMix() const;
    
    // Real-time parameter adjustment
    void setParameterOverride(Target target, int trackIndex, int parameterIndex, 
                             float value, bool temporary = false);
    void clearParameterOverride(Target target, int trackIndex, int parameterIndex);
    void clearAllOverrides();
    
    // Learning system
    void enableLearning(bool enabled);
    void recordUserAdjustment(Target target, int trackIndex, int parameterIndex, 
                             float oldValue, float newValue);
    void adaptToUserPreferences();
    
    // Hardware integration
    void processHardwareInput(int controlIndex, float value);
    void visualizeAutomation(uint32_t* displayBuffer, int width, int height) const;
    
private:
    // Rule processing
    void processAutomationRules(const SpectrumAnalyzer::SpectrumData& spectrum,
                               const SpectrumAnalyzer::AudioFeatures& features);
    float calculateRuleOutput(const AutomationRule& rule,
                             const SpectrumAnalyzer::SpectrumData& spectrum,
                             const SpectrumAnalyzer::AudioFeatures& features) const;
    bool evaluateRuleConditions(const AutomationRule& rule,
                               const SpectrumAnalyzer::SpectrumData& spectrum,
                               const SpectrumAnalyzer::AudioFeatures& features) const;
    
    // AI analysis
    void analyzeMusicalContext(const SpectrumAnalyzer::SpectrumData& spectrum,
                              const SpectrumAnalyzer::AudioFeatures& features);
    void detectMixingIssues();
    void generateIntelligentSuggestions();
    
    // Genre detection implementation
    void updateGenreClassification(const SpectrumAnalyzer::SpectrumData& spectrum,
                                  const SpectrumAnalyzer::AudioFeatures& features);
    float calculateGenreScore(MusicGenre genre,
                             const SpectrumAnalyzer::SpectrumData& spectrum,
                             const SpectrumAnalyzer::AudioFeatures& features) const;
    
    // Parameter smoothing
    struct SmoothedParameter {
        float currentValue = 0.0f;
        float targetValue = 0.0f;
        float smoothingRate = 0.1f;
        uint64_t lastUpdateTime = 0;
        
        void update(float newTarget, float deltaTime, float smoothing);
        float getValue() const { return currentValue; }
    };
    
    // Member variables
    std::shared_ptr<SpectrumAnalyzer> spectrumAnalyzer_;
    AutomationCallback automationCallback_;
    
    Mode currentMode_ = Mode::GENTLE;
    std::atomic<float> intensity_;
    
    std::vector<AutomationRule> automationRules_;
    std::vector<AutomationSuggestion> activeSuggestions_;
    std::map<std::string, std::vector<AutomationRule>> presetRules_;
    
    // Smoothed parameter tracking
    std::map<std::tuple<Target, int, int>, SmoothedParameter> smoothedParameters_;
    
    // Analysis history
    std::vector<SpectrumAnalyzer::SpectrumData> spectrumHistory_;
    std::vector<SpectrumAnalyzer::AudioFeatures> featureHistory_;
    static constexpr int HISTORY_SIZE = 100;
    
    // Genre classification
    std::array<float, static_cast<size_t>(MusicGenre::CLASSICAL) + 1> genreScores_;
    MusicGenre detectedGenre_ = MusicGenre::UNKNOWN;
    float genreConfidence_ = 0.0f;
    
    // Mix analysis
    mutable MixAnalysis currentMixAnalysis_;
    
    // Learning system
    bool learningEnabled_ = false;
    struct UserPreference {
        Target target;
        int trackIndex;
        int parameterIndex;
        float preferredValue;
        float confidence;
        int adjustmentCount;
    };
    std::vector<UserPreference> userPreferences_;
    
    // Performance monitoring
    mutable std::atomic<float> processingLoad_;
    mutable std::mutex processingMutex_;
    
    // Utility functions
    float mapRange(float value, float inMin, float inMax, float outMin, float outMax) const;
    float applySmoothingFilter(float current, float target, float smoothing) const;
    uint64_t getCurrentTimeMs() const;
};

// Preset automation rule collections
namespace AutomationPresets {
    // Genre-specific automation sets
    std::vector<AdaptiveAutomation::AutomationRule> createHouseRules();
    std::vector<AdaptiveAutomation::AutomationRule> createTechnoRules();
    std::vector<AdaptiveAutomation::AutomationRule> createAmbientRules();
    std::vector<AdaptiveAutomation::AutomationRule> createDnBRules();
    
    // Mix enhancement rules
    std::vector<AdaptiveAutomation::AutomationRule> createMixEnhancementRules();
    std::vector<AdaptiveAutomation::AutomationRule> createMasteringRules();
    std::vector<AdaptiveAutomation::AutomationRule> createCreativeRules();
    
    // Problem-solving rules
    std::vector<AdaptiveAutomation::AutomationRule> createAntiMuddyRules();
    std::vector<AdaptiveAutomation::AutomationRule> createBrightnessRules();
    std::vector<AdaptiveAutomation::AutomationRule> createDynamicRules();
}

} // namespace EtherSynth