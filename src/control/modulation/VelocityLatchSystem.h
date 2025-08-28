#pragma once
#include <cstdint>
#include <unordered_map>
#include <functional>
#include <memory>
#include "../../interface/IVelocityModulationView.h"
#include "../sequencer/VelocityCapture.h"

/**
 * VelocityLatchSystem - Central system for velocity parameter modulation
 * 
 * Manages velocity modulation latching for all synthesizer parameters:
 * - Tracks which parameters have velocity modulation enabled
 * - Stores per-parameter velocity latch state and settings
 * - Connects V-icon UI interactions to parameter modulation
 * - Manages real-time velocity modulation calculations
 * - Handles preset save/recall of velocity modulation state
 * 
 * Features:
 * - Per-parameter velocity latch toggle (tap V-icon to enable/disable)
 * - Parameter-specific modulation depth and polarity settings
 * - Real-time velocity modulation with smooth parameter updates
 * - Integration with VelocityCapture for live velocity input
 * - Persistent storage of velocity modulation configuration
 * - Special handling for velocity→volume modulation
 */
class VelocityLatchSystem {
public:
    // Parameter velocity modulation configuration
    struct ParameterVelocityConfig {
        bool enabled;                           // Velocity modulation enabled
        float modulationDepth;                  // Modulation depth (-2.0 to +2.0)
        VelocityModulationUI::ModulationPolarity polarity;  // Modulation direction
        bool invertVelocity;                    // Invert velocity curve
        float velocityScale;                    // Velocity sensitivity (0.1-2.0)
        float baseValue;                        // Base parameter value (0.0-1.0)
        bool enableVelocityToVolume;            // Special velocity→volume handling
        
        ParameterVelocityConfig() :
            enabled(false),
            modulationDepth(1.0f),
            polarity(VelocityModulationUI::ModulationPolarity::POSITIVE),
            invertVelocity(false),
            velocityScale(1.0f),
            baseValue(0.5f),
            enableVelocityToVolume(true) {}
    };
    
    // Velocity modulation event callbacks
    using ParameterUpdateCallback = std::function<void(uint32_t parameterId, float modulatedValue)>;
    using VIconStateUpdateCallback = std::function<void(uint32_t parameterId, VelocityModulationUI::VIconState state)>;
    
    VelocityLatchSystem();
    ~VelocityLatchSystem() = default;
    
    // System initialization
    void initialize(std::shared_ptr<VelocityCapture> velocityCapture);
    void setVelocityModulationPanel(std::shared_ptr<IVelocityModulationView> panel);
    
    // Parameter registration
    void registerParameter(uint32_t parameterId, const ParameterVelocityConfig& config);
    void unregisterParameter(uint32_t parameterId);
    bool isParameterRegistered(uint32_t parameterId) const;
    
    // Velocity latch control
    void toggleVelocityLatch(uint32_t parameterId);          // Toggle velocity modulation on/off
    void enableVelocityLatch(uint32_t parameterId, bool enabled = true);
    void disableVelocityLatch(uint32_t parameterId);
    bool isVelocityLatchEnabled(uint32_t parameterId) const;
    
    // Parameter configuration
    void setParameterConfig(uint32_t parameterId, const ParameterVelocityConfig& config);
    const ParameterVelocityConfig& getParameterConfig(uint32_t parameterId) const;
    void setParameterBaseValue(uint32_t parameterId, float baseValue);
    void setParameterModulationDepth(uint32_t parameterId, float depth);
    void setParameterPolarity(uint32_t parameterId, VelocityModulationUI::ModulationPolarity polarity);
    
    // Real-time velocity modulation
    void updateVelocityModulation();                         // Called each audio frame
    float getModulatedParameterValue(uint32_t parameterId) const;
    uint8_t getCurrentVelocity() const;
    VelocityCapture::VelocitySource getActiveVelocitySource() const;
    
    // Velocity modulation calculation
    float calculateVelocityModulation(uint32_t parameterId, uint8_t velocity) const;
    float applyVelocityToParameter(uint32_t parameterId, float baseValue, uint8_t velocity) const;
    
    // Batch operations
    void enableAllVelocityLatches();
    void disableAllVelocityLatches();
    void setAllModulationDepths(float depth);
    void setAllPolarities(VelocityModulationUI::ModulationPolarity polarity);
    
    // Preset management
    void saveVelocityLatchState();                           // Save current latch configuration
    void loadVelocityLatchState();                           // Load saved latch configuration
    void clearVelocityLatchState();                          // Clear all velocity modulation
    
    // Statistics and monitoring
    size_t getActiveVelocityLatchCount() const;              // Count of enabled velocity latches
    std::vector<uint32_t> getActiveVelocityLatchIds() const; // List of parameters with velocity latch
    float getSystemVelocityModulationLoad() const;          // CPU load estimate for velocity modulation
    
    // Callbacks
    void setParameterUpdateCallback(ParameterUpdateCallback callback);
    void setVIconStateUpdateCallback(VIconStateUpdateCallback callback);
    
    // System control
    void setEnabled(bool enabled);
    bool isEnabled() const { return systemEnabled_; }
    void reset();
    
private:
    // System state
    bool systemEnabled_;
    std::shared_ptr<VelocityCapture> velocityCapture_;
    std::shared_ptr<IVelocityModulationView> modulationPanel_;
    
    // Parameter velocity configurations
    std::unordered_map<uint32_t, ParameterVelocityConfig> parameterConfigs_;
    std::unordered_map<uint32_t, float> currentModulatedValues_;
    
    // Velocity state tracking
    uint8_t lastVelocity_;
    VelocityCapture::VelocitySource lastVelocitySource_;
    std::chrono::steady_clock::time_point lastVelocityUpdateTime_;
    
    // Callbacks
    ParameterUpdateCallback parameterUpdateCallback_;
    VIconStateUpdateCallback vIconStateUpdateCallback_;
    
    // Default configuration
    static ParameterVelocityConfig defaultConfig_;
    
    // Internal methods
    void handleVIconTap(uint32_t parameterId);
    void handleVIconLongPress(uint32_t parameterId);
    void updateParameterVIcon(uint32_t parameterId);
    void updateAllVIconStates();
    
    VelocityModulationUI::VIconState getVIconStateForParameter(uint32_t parameterId) const;
    float clampParameterValue(float value) const;
    float applyVelocityCurve(float velocity, bool invert) const;
    
    // Velocity modulation helpers
    float calculatePositiveModulation(float base, float depth, float normalizedVelocity) const;
    float calculateNegativeModulation(float base, float depth, float normalizedVelocity) const;
    float calculateBipolarModulation(float base, float depth, float normalizedVelocity) const;
    
    // Constants
    static constexpr float MIN_MODULATION_DEPTH = -2.0f;
    static constexpr float MAX_MODULATION_DEPTH = 2.0f;
    static constexpr float MIN_VELOCITY_SCALE = 0.1f;
    static constexpr float MAX_VELOCITY_SCALE = 2.0f;
    static constexpr std::chrono::milliseconds VELOCITY_UPDATE_TIMEOUT{100};  // 100ms timeout for velocity activity
};