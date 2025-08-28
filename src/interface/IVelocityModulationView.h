#pragma once
#include <cstdint>
#include <functional>

/**
 * IVelocityModulationView - Interface for velocity modulation UI updates
 * 
 * Breaks circular dependency between control and UI layers by providing
 * an abstract interface that control classes can use to update UI state
 * without directly depending on UI implementation classes.
 */

namespace VelocityModulationUI {
    // Forward declarations to avoid circular dependencies
    enum class VIconState {
        INACTIVE,
        LATCHED,
        ACTIVELY_MODULATING
    };
    
    enum class ModulationPolarity {
        POSITIVE,
        NEGATIVE,
        BIPOLAR
    };
}

class IVelocityModulationView {
public:
    virtual ~IVelocityModulationView() = default;
    
    // UI state updates (control → UI)
    virtual void updateVIconState(uint32_t parameterId, VelocityModulationUI::VIconState state) = 0;
    virtual void updateModulationDepth(uint32_t parameterId, float depth) = 0;
    virtual void updatePolarity(uint32_t parameterId, VelocityModulationUI::ModulationPolarity polarity) = 0;
    virtual void showVelocitySettings(uint32_t parameterId) = 0;
    virtual void hideVelocitySettings() = 0;
    
    // Batch updates
    virtual void updateAllVIconStates(VelocityModulationUI::VIconState state) = 0;
    virtual void enableVelocityModulation(bool enabled) = 0;
    
    // Status queries
    virtual bool isVelocityModulationActive() const = 0;
    virtual size_t getActiveModulationCount() const = 0;
};

// Callback types for UI → Control communication
using VelocityModulationCallback = std::function<void(uint32_t parameterId, float depth, bool enabled)>;
using VelocitySettingsCallback = std::function<void(uint32_t parameterId)>;