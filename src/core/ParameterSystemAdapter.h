#pragma once
#include "ParameterSystem.h"
#include "../control/modulation/VelocityDepthControl.h"
#include <memory>
#include <map>

/**
 * ParameterSystemAdapter - Compatibility layer for migrating to UnifiedParameterSystem
 * 
 * This adapter provides backward compatibility while migrating existing code
 * to use the new unified parameter system. It:
 * - Maintains existing API surfaces for VelocityParameterScaling
 * - Integrates VelocityDepthControl with the unified system
 * - Provides migration utilities for existing parameter usage
 * - Ensures smooth transition without breaking existing functionality
 * 
 * Migration Strategy:
 * 1. Replace direct parameter access with adapter calls
 * 2. Gradually migrate components to use UnifiedParameterSystem directly
 * 3. Remove adapter once migration is complete
 */
class ParameterSystemAdapter {
public:
    ParameterSystemAdapter();
    ~ParameterSystemAdapter();
    
    // Initialization - must be called before use
    bool initialize(float sampleRate);
    void shutdown();
    
    // Velocity Parameter Scaling Compatibility Layer
    // These methods wrap the existing VelocityParameterScaling functionality
    
    void setParameterVelocityScale(uint32_t parameterId, float scale);
    float getParameterVelocityScale(uint32_t parameterId) const;
    void setParameterPolarity(uint32_t parameterId, VelocityModulationUI::ModulationPolarity polarity);
    VelocityModulationUI::ModulationPolarity getParameterPolarity(uint32_t parameterId) const;
    
    // Apply velocity modulation using the unified system
    float applyVelocityModulation(uint32_t parameterId, float baseValue, float velocity);
    
    // Velocity Depth Control Integration
    void setMasterVelocityDepth(float depth);
    float getMasterVelocityDepth() const;
    void setParameterVelocityDepth(uint32_t parameterId, float depth);
    float getParameterVelocityDepth(uint32_t parameterId) const;
    
    // Parameter Access Compatibility Layer
    // These provide the same interface as before but use the unified system
    
    // Convert between old parameter IDs and new ParameterID enum
    static ParameterID legacyParameterToParameterID(uint32_t legacyId);
    static uint32_t parameterIDToLegacyParameter(ParameterID paramId);
    
    // Legacy parameter access (maintains old uint32_t API)
    void setParameter(uint32_t parameterId, float value);
    void setParameter(uint32_t parameterId, size_t instrumentIndex, float value);
    float getParameter(uint32_t parameterId) const;
    float getParameter(uint32_t parameterId, size_t instrumentIndex) const;
    
    // Parameter with velocity (legacy API)
    void setParameterWithVelocity(uint32_t parameterId, float baseValue, float velocity);
    void setInstrumentParameterWithVelocity(uint32_t parameterId, size_t instrumentIndex, 
                                           float baseValue, float velocity);
    
    // Preset System Integration
    // Provides compatibility with existing preset loading/saving code
    
    struct LegacyPresetData {
        std::map<uint32_t, float> globalParameters;
        std::array<std::map<uint32_t, float>, MAX_INSTRUMENTS> instrumentParameters;
        std::string presetName;
    };
    
    bool saveLegacyPreset(LegacyPresetData& preset) const;
    bool loadLegacyPreset(const LegacyPresetData& preset);
    bool convertLegacyPreset(const LegacyPresetData& legacy, UnifiedParameterSystem::PresetData& unified) const;
    bool convertUnifiedPreset(const UnifiedParameterSystem::PresetData& unified, LegacyPresetData& legacy) const;
    
    // Migration Utilities
    // Help migrate existing code to the new system
    
    // Migrate all current parameter values to the unified system
    void migrateParametersToUnified();
    
    // Check if all parameters have been successfully migrated
    bool verifyMigration() const;
    
    // Get recommendations for code changes needed
    struct MigrationRecommendation {
        std::string component;
        std::string currentUsage;
        std::string recommendedChange;
        int priority; // 1 = critical, 2 = important, 3 = nice to have
    };
    std::vector<MigrationRecommendation> getMigrationRecommendations() const;
    
    // System Integration
    // Ensure the adapter works with existing systems
    
    // Register callbacks to maintain synchronization
    void registerParameterChangeCallback(std::function<void(uint32_t, float, float)> callback);
    void registerVelocityScaleChangeCallback(std::function<void(uint32_t, float, float)> callback);
    
    // Audio processing integration
    void processAudioBlock();
    
    // Error handling and diagnostics
    bool hasErrors() const { return !lastError_.empty(); }
    std::string getLastError() const { return lastError_; }
    void clearErrors() { lastError_.clear(); }
    
    // Statistics for migration tracking
    struct MigrationStats {
        size_t totalParametersFound;
        size_t parametersMigrated;
        size_t parametersWithVelocityScaling;
        size_t parametersWithDepthControl;
        size_t legacyAPICallsRemaining;
        float migrationCompleteness; // 0.0 to 1.0
    };
    MigrationStats getMigrationStats() const;

private:
    // Core systems
    UnifiedParameterSystem* unifiedSystem_;
    std::unique_ptr<VelocityDepthControl> depthControl_;
    
    // Compatibility state
    bool initialized_;
    mutable std::string lastError_;
    
    // Legacy parameter mapping
    static std::unordered_map<uint32_t, ParameterID> legacyToUnifiedMap_;
    static std::unordered_map<ParameterID, uint32_t> unifiedToLegacyMap_;
    
    // Callback management
    std::vector<std::function<void(uint32_t, float, float)>> parameterChangeCallbacks_;
    std::vector<std::function<void(uint32_t, float, float)>> velocityScaleChangeCallbacks_;
    
    // Migration tracking
    mutable MigrationStats migrationStats_;
    std::vector<MigrationRecommendation> migrationRecommendations_;
    
    // Internal methods
    void initializeLegacyMapping();
    void setupDepthControlIntegration();
    void updateMigrationStats() const;
    void generateMigrationRecommendations();
    
    // Error handling
    void setError(const std::string& error) const;
    
    // Validation
    bool isValidLegacyParameterID(uint32_t parameterId) const;
    
    // Constants
    static constexpr size_t MAX_LEGACY_PARAMETERS = 256;
};

// Global adapter instance for backward compatibility
extern ParameterSystemAdapter g_parameterAdapter;

// Legacy convenience macros (for gradual migration)
#define LEGACY_GET_PARAM(id) g_parameterAdapter.getParameter(id)
#define LEGACY_SET_PARAM(id, value) g_parameterAdapter.setParameter(id, value)
#define LEGACY_GET_INSTRUMENT_PARAM(id, inst) g_parameterAdapter.getParameter(id, inst)
#define LEGACY_SET_INSTRUMENT_PARAM(id, inst, value) g_parameterAdapter.setParameter(id, inst, value)

// Migration helper macros
#define MIGRATE_PARAMETER_ACCESS(oldCall, newCall) \
    do { \
        static bool _migration_warning_shown = false; \
        if (!_migration_warning_shown) { \
            /* TODO: Add logging of migration recommendation */ \
            _migration_warning_shown = true; \
        } \
        newCall; \
    } while(0)