#pragma once
#include "../core/DataStructures.h"
#include "../core/DIContainer.h"
#include "../utils/Constants.h"
#include <vector>
#include <array>

namespace GridSequencer {
namespace Pattern {

using Core::Result;
using Core::StepData;

// Pattern system interface - manages sequencer patterns and banks
class IPatternSystem {
public:
    virtual ~IPatternSystem() = default;

    // Pattern management
    virtual Result<bool> setStep(int engine, int step, const StepData& data) = 0;
    virtual Result<StepData> getStep(int engine, int step) = 0;
    virtual Result<bool> clearStep(int engine, int step) = 0;
    virtual void clearPattern(int engine) = 0;
    virtual void clearAllPatterns() = 0;

    // Pattern copying
    virtual Result<bool> copyPattern(int fromEngine, int toEngine) = 0;
    virtual Result<bool> copyStep(int fromEngine, int fromStep, int toEngine, int toStep) = 0;

    // Pattern bank management
    virtual Result<bool> savePatternToBank(int bank, int slot) = 0;
    virtual Result<bool> loadPatternFromBank(int bank, int slot) = 0;
    virtual Result<bool> copyPatternInBank(int fromBank, int fromSlot, int toBank, int toSlot) = 0;

    // Pattern chaining
    virtual void addToChain(int bank, int slot) = 0;
    virtual void clearChain() = 0;
    virtual std::vector<std::pair<int, int>> getChain() const = 0;
    virtual bool isChainingEnabled() const = 0;
    virtual void setChainingEnabled(bool enabled) = 0;

    // Current pattern state
    virtual void setCurrentBank(int bank) = 0;
    virtual void setCurrentSlot(int slot) = 0;
    virtual int getCurrentBank() const = 0;
    virtual int getCurrentSlot() const = 0;
    virtual int getCurrentAbsolutePattern() const = 0;

    // Pattern information
    virtual bool hasActiveSteps(int engine) const = 0;
    virtual int getActiveStepCount(int engine) const = 0;
    virtual std::vector<int> getActiveSteps(int engine) const = 0;

    // Drum-specific patterns (16-bit masks for 16 pads)
    virtual void setDrumMask(int pad, uint16_t mask) = 0;
    virtual uint16_t getDrumMask(int pad) const = 0;
    virtual void toggleDrumStep(int pad, int step) = 0;
    virtual bool isDrumStepActive(int pad, int step) const = 0;

    // Pattern validation
    virtual bool isValidEngine(int engine) const = 0;
    virtual bool isValidStep(int step) const = 0;
    virtual bool isValidBank(int bank) const = 0;
    virtual bool isValidSlot(int slot) const = 0;
};

} // namespace Pattern
} // namespace GridSequencer