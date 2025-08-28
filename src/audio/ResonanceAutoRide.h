#pragma once
#include <cmath>
#include <algorithm>

/**
 * ResonanceAutoRide - Automatic resonance control with cutoff opening
 * 
 * Classic analog synthesizer feature where resonance automatically increases 
 * as the filter cutoff decreases. Creates the characteristic "acid" sound
 * and prevents filter self-oscillation at extreme settings.
 * 
 * Features:
 * - Exponential resonance curve tied to cutoff frequency
 * - Configurable auto-ride amount (0-100%)
 * - Cutoff opening compensation to maintain perceived brightness
 * - Real-time safe parameter updates
 * - Multiple curve types (exponential, logarithmic, S-curve)
 */
class ResonanceAutoRide {
public:
    enum class CurveType {
        EXPONENTIAL,    // Classic exponential curve (most common)
        LOGARITHMIC,    // Gentler curve for subtle effect
        S_CURVE,        // Smooth transition for musical control
        LINEAR,         // Linear relationship (for testing)
        CUSTOM          // User-defined curve
    };
    
    struct Config {
        float autoRideAmount;       // 0-1: amount of auto-ride effect
        float cutoffOpeningAmount;  // 0-1: compensation cutoff opening
        CurveType curveType;        // resonance curve shape
        float minCutoffHz;          // minimum cutoff for auto-ride
        float maxCutoffHz;          // maximum cutoff for auto-ride
        float minResonance;         // minimum Q factor
        float maxResonance;         // maximum Q factor
        bool enabled;               // enable/disable auto-ride
        
        // Default configuration
        Config() : 
            autoRideAmount(0.7f),
            cutoffOpeningAmount(0.3f),
            curveType(CurveType::EXPONENTIAL),
            minCutoffHz(80.0f),
            maxCutoffHz(8000.0f),
            minResonance(0.1f),
            maxResonance(12.0f),
            enabled(true) {}
    };
    
    ResonanceAutoRide();
    ~ResonanceAutoRide() = default;
    
    // Initialization
    bool initialize(const Config& config);
    void shutdown();
    void reset();
    
    // Main processing
    float processResonance(float baseCutoffHz, float baseResonance);
    float processCutoffOpening(float baseCutoffHz, float targetResonance);
    
    // Configuration
    void setConfig(const Config& config);
    void setAutoRideAmount(float amount);           // 0-1
    void setCutoffOpeningAmount(float amount);      // 0-1
    void setCurveType(CurveType type);
    void setEnabled(bool enabled);
    
    // Analysis
    float getCurrentAutoResonance() const { return currentAutoResonance_; }
    float getCurrentCutoffOpening() const { return currentCutoffOpening_; }
    float getEffectiveResonance() const { return effectiveResonance_; }
    float getEffectiveCutoff() const { return effectiveCutoff_; }
    
    // Curve utilities
    static float calculateAutoRideResonance(float cutoffHz, const Config& config);
    static float calculateCutoffOpening(float targetResonance, const Config& config);
    
    // Getters
    const Config& getConfig() const { return config_; }
    bool isInitialized() const { return initialized_; }
    bool isEnabled() const { return config_.enabled; }
    
private:
    Config config_;
    bool initialized_;
    
    // Current state
    float currentAutoResonance_;
    float currentCutoffOpening_;
    float effectiveResonance_;
    float effectiveCutoff_;
    
    // Internal curve functions
    float exponentialCurve(float normalizedCutoff) const;
    float logarithmicCurve(float normalizedCutoff) const;
    float sCurve(float normalizedCutoff) const;
    float linearCurve(float normalizedCutoff) const;
    float customCurve(float normalizedCutoff) const;
    
    // Utility functions
    float clamp(float value, float min, float max) const;
    float mapRange(float value, float inMin, float inMax, float outMin, float outMax) const;
    float normalizeFrequency(float frequency) const;
    
    // Constants
    static constexpr float DEFAULT_MIN_CUTOFF_HZ = 80.0f;
    static constexpr float DEFAULT_MAX_CUTOFF_HZ = 8000.0f;
    static constexpr float DEFAULT_MIN_RESONANCE = 0.1f;
    static constexpr float DEFAULT_MAX_RESONANCE = 12.0f;
    static constexpr float SMOOTHING_FACTOR = 0.999f;
};