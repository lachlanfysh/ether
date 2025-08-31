#include "AdaptiveAutomation.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>

namespace EtherSynth {

AdaptiveAutomation::AdaptiveAutomation() 
    : intensity_(0.5f)
    , processingLoad_(0.0f)
{
    // Initialize genre scores
    genreScores_.fill(0.0f);
    
    // Reserve space for history
    spectrumHistory_.reserve(HISTORY_SIZE);
    featureHistory_.reserve(HISTORY_SIZE);
    
    std::cout << "AdaptiveAutomation: Initialized intelligent parameter automation\n";
}

AdaptiveAutomation::~AdaptiveAutomation() {
    shutdown();
}

void AdaptiveAutomation::initialize(std::shared_ptr<SpectrumAnalyzer> analyzer) {
    spectrumAnalyzer_ = analyzer;
    
    // Load default automation rules for general use
    loadPresetRules("Default");
    
    std::cout << "AdaptiveAutomation: Initialized with spectrum analyzer\n";
}

void AdaptiveAutomation::shutdown() {
    clearAllRules();
    activeSuggestions_.clear();
    smoothedParameters_.clear();
    
    std::cout << "AdaptiveAutomation: Shutdown complete\n";
}

void AdaptiveAutomation::update(float deltaTime) {
    if (!spectrumAnalyzer_) return;
    
    auto startTime = std::chrono::high_resolution_clock::now();
    std::lock_guard<std::mutex> lock(processingMutex_);
    
    // Update smoothed parameters
    for (auto& [key, param] : smoothedParameters_) {
        param.update(param.targetValue, deltaTime, 0.9f);
    }
    
    // Calculate processing load
    auto endTime = std::chrono::high_resolution_clock::now();
    float processTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
    processingLoad_.store(processTime / (deltaTime * 1000.0f));
}

void AdaptiveAutomation::setMode(Mode mode) {
    currentMode_ = mode;
    
    // Adjust intensity based on mode
    switch (mode) {
        case Mode::DISABLED:
            setIntensity(0.0f);
            break;
        case Mode::GENTLE:
            setIntensity(0.3f);
            break;
        case Mode::MODERATE:
            setIntensity(0.6f);
            break;
        case Mode::AGGRESSIVE:
            setIntensity(0.9f);
            break;
        case Mode::CREATIVE:
            setIntensity(0.7f);
            break;
        case Mode::MASTERING:
            setIntensity(0.8f);
            break;
    }
    
    std::cout << "AdaptiveAutomation: Set mode to " << static_cast<int>(mode) << "\n";
}

void AdaptiveAutomation::setIntensity(float intensity) {
    intensity_.store(std::clamp(intensity, 0.0f, 1.0f));
}

void AdaptiveAutomation::addAutomationRule(const AutomationRule& rule) {
    automationRules_.push_back(rule);
    std::cout << "AdaptiveAutomation: Added automation rule for target " 
              << static_cast<int>(rule.target) << "\n";
}

void AdaptiveAutomation::removeAutomationRule(int ruleIndex) {
    if (ruleIndex >= 0 && ruleIndex < automationRules_.size()) {
        automationRules_.erase(automationRules_.begin() + ruleIndex);
        std::cout << "AdaptiveAutomation: Removed automation rule " << ruleIndex << "\n";
    }
}

void AdaptiveAutomation::clearAllRules() {
    automationRules_.clear();
    smoothedParameters_.clear();
    std::cout << "AdaptiveAutomation: Cleared all automation rules\n";
}

void AdaptiveAutomation::processSpectrum(const SpectrumAnalyzer::SpectrumData& spectrum,
                                       const SpectrumAnalyzer::AudioFeatures& features) {
    if (currentMode_ == Mode::DISABLED || intensity_.load() <= 0.0f) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(processingMutex_);
    
    // Store in history
    spectrumHistory_.push_back(spectrum);
    if (spectrumHistory_.size() > HISTORY_SIZE) {
        spectrumHistory_.erase(spectrumHistory_.begin());
    }
    
    featureHistory_.push_back(features);
    if (featureHistory_.size() > HISTORY_SIZE) {
        featureHistory_.erase(featureHistory_.begin());
    }
    
    // Process automation rules
    processAutomationRules(spectrum, features);
    
    // Update genre classification
    updateGenreClassification(spectrum, features);
    
    // Analyze musical context
    analyzeMusicalContext(spectrum, features);
    
    // Detect mixing issues
    detectMixingIssues();
}

void AdaptiveAutomation::processAutomationRules(const SpectrumAnalyzer::SpectrumData& spectrum,
                                               const SpectrumAnalyzer::AudioFeatures& features) {
    if (!automationCallback_) return;
    
    float intensityFactor = intensity_.load();
    
    for (const auto& rule : automationRules_) {
        if (!rule.enabled) continue;
        
        // Evaluate rule conditions
        if (!evaluateRuleConditions(rule, spectrum, features)) continue;
        
        // Calculate output value
        float outputValue = calculateRuleOutput(rule, spectrum, features);
        
        // Apply intensity scaling
        outputValue = mapRange(outputValue, 0.0f, 1.0f, 
                              rule.outputMin + (rule.outputMax - rule.outputMin) * (1.0f - intensityFactor),
                              rule.outputMax);
        
        // Get or create smoothed parameter
        auto key = std::make_tuple(rule.target, rule.trackIndex, rule.parameterIndex);
        auto& smoothedParam = smoothedParameters_[key];
        
        // Update smoothed parameter
        smoothedParam.targetValue = outputValue;
        smoothedParam.smoothingRate = 1.0f - rule.smoothing;
        
        // Send automation value
        automationCallback_(rule.target, rule.trackIndex, rule.parameterIndex, 
                          smoothedParam.getValue());
    }
}

bool AdaptiveAutomation::evaluateRuleConditions(const AutomationRule& rule,
                                               const SpectrumAnalyzer::SpectrumData& spectrum,
                                               const SpectrumAnalyzer::AudioFeatures& features) const {
    // Check activity requirement
    if (rule.requiresActivity && !spectrum.hasActivity) {
        return false;
    }
    
    if (rule.requiresActivity && spectrum.totalEnergy < rule.activityThreshold) {
        return false;
    }
    
    // Check frequency band conditions
    if (spectrum.bassEnergy < rule.bassEnergyMin || spectrum.bassEnergy > rule.bassEnergyMax) {
        return false;
    }
    
    if (spectrum.midEnergy < rule.midEnergyMin || spectrum.midEnergy > rule.midEnergyMax) {
        return false;
    }
    
    if (spectrum.highEnergy < rule.highEnergyMin || spectrum.highEnergy > rule.highEnergyMax) {
        return false;
    }
    
    // Check spectral conditions
    if (spectrum.spectralCentroid < rule.centroidMin || spectrum.spectralCentroid > rule.centroidMax) {
        return false;
    }
    
    if (spectrum.spectralSpread < rule.spreadMin || spectrum.spectralSpread > rule.spreadMax) {
        return false;
    }
    
    return true;
}

float AdaptiveAutomation::calculateRuleOutput(const AutomationRule& rule,
                                             const SpectrumAnalyzer::SpectrumData& spectrum,
                                             const SpectrumAnalyzer::AudioFeatures& features) const {
    float output = 0.0f;
    
    // Primary parameter mapping based on rule target
    switch (rule.target) {
        case Target::ENGINE_FILTER_CUTOFF:
            // Map spectral centroid to filter cutoff
            output = mapRange(spectrum.spectralCentroid, 100.0f, 8000.0f, 0.0f, 1.0f);
            break;
            
        case Target::ENGINE_FILTER_RESONANCE:
            // Higher resonance for narrower spectrum
            output = mapRange(spectrum.spectralSpread, 2000.0f, 500.0f, 0.0f, 1.0f);
            break;
            
        case Target::ENGINE_AMPLITUDE:
            // Dynamic amplitude based on total energy
            output = mapRange(spectrum.totalEnergy, 0.0f, 0.1f, 0.3f, 1.0f);
            break;
            
        case Target::FX_REVERB_SIZE:
            // Larger reverb for lower frequency content
            output = mapRange(spectrum.lowMidRatio, 0.5f, 2.0f, 0.3f, 0.8f);
            break;
            
        case Target::FX_COMPRESSOR_THRESHOLD:
            // Adjust compression based on dynamic range
            output = mapRange(spectrum.peak - spectrum.rms, 0.1f, 0.5f, 0.2f, 0.8f);
            break;
            
        case Target::FX_EQ_LOW_GAIN:
            // EQ adjustment based on bass content
            if (spectrum.bassEnergy < 0.2f * spectrum.totalEnergy) {
                output = 0.6f; // Boost bass if lacking
            } else if (spectrum.bassEnergy > 0.6f * spectrum.totalEnergy) {
                output = 0.4f; // Reduce bass if excessive
            } else {
                output = 0.5f; // Neutral
            }
            break;
            
        case Target::FX_EQ_HIGH_GAIN:
            // EQ adjustment based on high frequency content
            if (spectrum.highEnergy < 0.1f * spectrum.totalEnergy) {
                output = 0.6f; // Boost highs if lacking
            } else if (spectrum.highEnergy > 0.3f * spectrum.totalEnergy) {
                output = 0.4f; // Reduce highs if excessive
            } else {
                output = 0.5f; // Neutral
            }
            break;
            
        default:
            // Generic mapping based on spectral centroid
            output = mapRange(spectrum.spectralCentroid, 200.0f, 4000.0f, 0.0f, 1.0f);
            break;
    }
    
    // Apply sensitivity
    output = mapRange(output, 0.0f, 1.0f, 0.5f - rule.sensitivity * 0.5f, 
                     0.5f + rule.sensitivity * 0.5f);
    
    // Apply inversion if requested
    if (rule.inverted) {
        output = 1.0f - output;
    }
    
    return std::clamp(output, rule.outputMin, rule.outputMax);
}

void AdaptiveAutomation::updateGenreClassification(const SpectrumAnalyzer::SpectrumData& spectrum,
                                                  const SpectrumAnalyzer::AudioFeatures& features) {
    // Simple genre classification based on spectral characteristics
    
    // House: Strong bass, regular rhythm, mid-range emphasis
    genreScores_[static_cast<size_t>(MusicGenre::HOUSE)] = 
        (spectrum.bassEnergy > 0.4f ? 0.3f : 0.0f) +
        (features.isPercussive ? 0.3f : 0.0f) +
        (spectrum.spectralCentroid > 800.0f && spectrum.spectralCentroid < 2000.0f ? 0.4f : 0.0f);
    
    // Techno: Heavy bass, percussive, high energy
    genreScores_[static_cast<size_t>(MusicGenre::TECHNO)] = 
        (spectrum.bassEnergy > 0.5f ? 0.4f : 0.0f) +
        (features.isPercussive ? 0.4f : 0.0f) +
        (spectrum.totalEnergy > 0.01f ? 0.2f : 0.0f);
    
    // Ambient: Low bass, wide spectrum, melodic
    genreScores_[static_cast<size_t>(MusicGenre::AMBIENT)] = 
        (spectrum.bassEnergy < 0.3f ? 0.3f : 0.0f) +
        (features.isMelodic ? 0.4f : 0.0f) +
        (spectrum.spectralSpread > 1500.0f ? 0.3f : 0.0f);
    
    // Find highest scoring genre
    auto maxIt = std::max_element(genreScores_.begin(), genreScores_.end());
    if (*maxIt > 0.5f) {
        detectedGenre_ = static_cast<MusicGenre>(std::distance(genreScores_.begin(), maxIt));
        genreConfidence_ = *maxIt;
    } else {
        detectedGenre_ = MusicGenre::UNKNOWN;
        genreConfidence_ = 0.0f;
    }
}

void AdaptiveAutomation::analyzeMusicalContext(const SpectrumAnalyzer::SpectrumData& spectrum,
                                              const SpectrumAnalyzer::AudioFeatures& features) {
    // This would be expanded with more sophisticated analysis
    // For now, basic context detection
    
    currentMixAnalysis_.bassBalance = (spectrum.bassEnergy > 0.4f) ? 
        (spectrum.bassEnergy - 0.4f) * 2.0f : -(0.4f - spectrum.bassEnergy) * 2.0f;
    currentMixAnalysis_.midBalance = (spectrum.midEnergy > 0.3f) ? 
        (spectrum.midEnergy - 0.3f) * 2.0f : -(0.3f - spectrum.midEnergy) * 2.0f;
    currentMixAnalysis_.highBalance = (spectrum.highEnergy > 0.2f) ? 
        (spectrum.highEnergy - 0.2f) * 2.0f : -(0.2f - spectrum.highEnergy) * 2.0f;
    
    currentMixAnalysis_.dynamicRange = spectrum.peak - spectrum.rms;
    currentMixAnalysis_.loudness = spectrum.rms;
    currentMixAnalysis_.clarity = 1.0f - (spectrum.spectralSpread / 5000.0f);
}

void AdaptiveAutomation::detectMixingIssues() {
    currentMixAnalysis_.hasImbalance = 
        (std::abs(currentMixAnalysis_.bassBalance) > 0.7f) ||
        (std::abs(currentMixAnalysis_.midBalance) > 0.7f) ||
        (std::abs(currentMixAnalysis_.highBalance) > 0.7f);
    
    currentMixAnalysis_.needsCompression = 
        (currentMixAnalysis_.dynamicRange > 0.4f);
    
    currentMixAnalysis_.needsEQ = currentMixAnalysis_.hasImbalance;
}

std::vector<AdaptiveAutomation::AutomationSuggestion> AdaptiveAutomation::generateSuggestions() {
    std::vector<AutomationSuggestion> suggestions;
    
    // Analyze current mix state and suggest improvements
    if (currentMixAnalysis_.hasImbalance) {
        if (currentMixAnalysis_.bassBalance > 0.5f) {
            AutomationSuggestion suggestion;
            suggestion.description = "Reduce excessive bass content";
            suggestion.target = Target::FX_EQ_LOW_GAIN;
            suggestion.trackIndex = -1; // Apply to master
            suggestion.parameterIndex = 0;
            suggestion.confidence = 0.8f;
            suggestion.potentialImprovement = 0.6f;
            suggestion.isEssential = true;
            suggestion.reason = "Bass frequencies are dominating the mix";
            
            // Create automation rule
            suggestion.suggestedRule = AutomationRule(suggestion.target, -1, 0);
            suggestion.suggestedRule.bassEnergyMin = 0.5f;
            suggestion.suggestedRule.bassEnergyMax = 1.0f;
            suggestion.suggestedRule.outputMin = 0.2f;
            suggestion.suggestedRule.outputMax = 0.4f;
            suggestion.suggestedRule.sensitivity = 0.8f;
            
            suggestions.push_back(suggestion);
        }
        
        if (currentMixAnalysis_.highBalance < -0.5f) {
            AutomationSuggestion suggestion;
            suggestion.description = "Add brightness and presence";
            suggestion.target = Target::FX_EQ_HIGH_GAIN;
            suggestion.trackIndex = -1;
            suggestion.parameterIndex = 0;
            suggestion.confidence = 0.7f;
            suggestion.potentialImprovement = 0.5f;
            suggestion.isEssential = false;
            suggestion.reason = "Mix lacks high-frequency content and clarity";
            
            suggestion.suggestedRule = AutomationRule(suggestion.target, -1, 0);
            suggestion.suggestedRule.highEnergyMin = 0.0f;
            suggestion.suggestedRule.highEnergyMax = 0.15f;
            suggestion.suggestedRule.outputMin = 0.6f;
            suggestion.suggestedRule.outputMax = 0.8f;
            
            suggestions.push_back(suggestion);
        }
    }
    
    if (currentMixAnalysis_.needsCompression) {
        AutomationSuggestion suggestion;
        suggestion.description = "Apply dynamic range compression";
        suggestion.target = Target::FX_COMPRESSOR_THRESHOLD;
        suggestion.trackIndex = -1;
        suggestion.parameterIndex = 0;
        suggestion.confidence = 0.9f;
        suggestion.potentialImprovement = 0.7f;
        suggestion.isEssential = true;
        suggestion.reason = "Excessive dynamic range affects mix consistency";
        
        suggestions.push_back(suggestion);
    }
    
    return suggestions;
}

void AdaptiveAutomation::loadPresetRules(const std::string& presetName) {
    clearAllRules();
    
    if (presetName == "Default") {
        // Load basic adaptive rules
        
        // Filter cutoff follows brightness
        AutomationRule filterRule(Target::ENGINE_FILTER_CUTOFF, 0, 0);
        filterRule.centroidMin = 200.0f;
        filterRule.centroidMax = 8000.0f;
        filterRule.outputMin = 0.2f;
        filterRule.outputMax = 0.9f;
        filterRule.sensitivity = 0.6f;
        filterRule.smoothing = 0.85f;
        addAutomationRule(filterRule);
        
        // Reverb size inversely related to bass content
        AutomationRule reverbRule(Target::FX_REVERB_SIZE, -1, 0);
        reverbRule.bassEnergyMin = 0.0f;
        reverbRule.bassEnergyMax = 1.0f;
        reverbRule.outputMin = 0.3f;
        reverbRule.outputMax = 0.8f;
        reverbRule.sensitivity = 0.5f;
        reverbRule.smoothing = 0.9f;
        reverbRule.inverted = true; // More reverb when less bass
        addAutomationRule(reverbRule);
        
        // EQ high boost for dull content
        AutomationRule eqHighRule(Target::FX_EQ_HIGH_GAIN, -1, 0);
        eqHighRule.centroidMin = 200.0f;
        eqHighRule.centroidMax = 2000.0f;
        eqHighRule.outputMin = 0.5f;
        eqHighRule.outputMax = 0.7f;
        eqHighRule.sensitivity = 0.4f;
        eqHighRule.inverted = true; // Boost when centroid is low
        addAutomationRule(eqHighRule);
        
        std::cout << "AdaptiveAutomation: Loaded default automation rules\n";
    }
    // Additional presets would be loaded here
}

void AdaptiveAutomation::setAutomationCallback(AutomationCallback callback) {
    automationCallback_ = callback;
}

float AdaptiveAutomation::mapRange(float value, float inMin, float inMax, 
                                  float outMin, float outMax) const {
    if (inMax - inMin == 0.0f) return outMin;
    
    float normalized = (value - inMin) / (inMax - inMin);
    normalized = std::clamp(normalized, 0.0f, 1.0f);
    return outMin + normalized * (outMax - outMin);
}

AdaptiveAutomation::MusicGenre AdaptiveAutomation::detectGenre() const {
    return detectedGenre_;
}

float AdaptiveAutomation::getGenreConfidence() const {
    return genreConfidence_;
}

AdaptiveAutomation::MixAnalysis AdaptiveAutomation::analyzeMix() const {
    return currentMixAnalysis_;
}

float AdaptiveAutomation::getProcessingLoad() const {
    return processingLoad_.load();
}

int AdaptiveAutomation::getActiveRuleCount() const {
    return std::count_if(automationRules_.begin(), automationRules_.end(),
                        [](const AutomationRule& rule) { return rule.enabled; });
}

void AdaptiveAutomation::applySuggestion(const AutomationSuggestion& suggestion) {
    // Add the suggested automation rule
    addAutomationRule(suggestion.suggestedRule);
    
    std::cout << "AdaptiveAutomation: Applied suggestion - " << suggestion.description << "\n";
}

// SmoothedParameter implementation
void AdaptiveAutomation::SmoothedParameter::update(float newTarget, float deltaTime, float smoothing) {
    targetValue = newTarget;
    
    // Exponential smoothing
    float alpha = 1.0f - std::pow(smoothing, deltaTime);
    currentValue += alpha * (targetValue - currentValue);
    
    lastUpdateTime = std::chrono::duration_cast<std::chrono::milliseconds>
        (std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

} // namespace EtherSynth