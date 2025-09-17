#pragma once
#include "../core/DIContainer.h"
#include "../utils/Constants.h"
#include <atomic>
#include <array>

namespace GridSequencer {
namespace State {

using Core::Result;

// Application state management interface
class IStateManager {
public:
    virtual ~IStateManager() = default;

    // Transport state
    virtual bool isPlaying() const = 0;
    virtual void setPlaying(bool playing) = 0;
    virtual bool isAudioRunning() const = 0;
    virtual void setAudioRunning(bool running) = 0;

    // Step sequencer state
    virtual int getCurrentStep() const = 0;
    virtual void setCurrentStep(int step) = 0;
    virtual void advanceStep() = 0;

    // Current engine/instrument
    virtual int getCurrentEngine() const = 0;
    virtual void setCurrentEngine(int engine) = 0;

    // UI modes
    virtual bool isWriteMode() const = 0;
    virtual void setWriteMode(bool enabled) = 0;
    virtual bool isEngineHold() const = 0;
    virtual void setEngineHold(bool enabled) = 0;
    virtual bool isShiftHeld() const = 0;
    virtual void setShiftHeld(bool held) = 0;

    // Grid connection
    virtual bool isGridConnected() const = 0;
    virtual void setGridConnected(bool connected) = 0;

    // Pattern banks
    virtual int getCurrentPatternBank() const = 0;
    virtual void setCurrentPatternBank(int bank) = 0;
    virtual int getCurrentPatternSlot() const = 0;
    virtual void setCurrentPatternSlot(int slot) = 0;

    // Step triggers (for sequencer)
    virtual bool getStepTrigger(int engine, int step) = 0;
    virtual void setStepTrigger(int engine, int step, bool trigger) = 0;
    virtual bool getNoteOffTrigger(int engine, int step) = 0;
    virtual void setNoteOffTrigger(int engine, int step, bool trigger) = 0;

    // Active notes tracking
    virtual int getActiveNote(int engine, int step) = 0;
    virtual void setActiveNote(int engine, int step, int note) = 0;
    virtual void clearActiveNote(int engine, int step) = 0;

    // State validation
    virtual bool isValidEngine(int engine) const = 0;
    virtual bool isValidStep(int step) const = 0;
    virtual bool isValidPatternBank(int bank) const = 0;
    virtual bool isValidPatternSlot(int slot) const = 0;

    // State persistence
    virtual Result<bool> saveState(const std::string& filename) = 0;
    virtual Result<bool> loadState(const std::string& filename) = 0;
};

} // namespace State
} // namespace GridSequencer