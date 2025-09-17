#pragma once
#include "IStateManager.h"
#include "../utils/Logger.h"
#include <atomic>
#include <array>

namespace GridSequencer {
namespace State {

class StateManager : public IStateManager {
public:
    StateManager();
    virtual ~StateManager() = default;

    // Transport state
    bool isPlaying() const override { return playing_.load(); }
    void setPlaying(bool playing) override { playing_ = playing; }
    bool isAudioRunning() const override { return audioRunning_.load(); }
    void setAudioRunning(bool running) override { audioRunning_ = running; }

    // Step sequencer state
    int getCurrentStep() const override { return currentStep_.load(); }
    void setCurrentStep(int step) override;
    void advanceStep() override;

    // Current engine/instrument
    int getCurrentEngine() const override { return currentEngine_.load(); }
    void setCurrentEngine(int engine) override;

    // UI modes
    bool isWriteMode() const override { return writeMode_.load(); }
    void setWriteMode(bool enabled) override { writeMode_ = enabled; }
    bool isEngineHold() const override { return engineHold_.load(); }
    void setEngineHold(bool enabled) override { engineHold_ = enabled; }
    bool isShiftHeld() const override { return shiftHeld_.load(); }
    void setShiftHeld(bool held) override { shiftHeld_ = held; }

    // Grid connection
    bool isGridConnected() const override { return gridConnected_.load(); }
    void setGridConnected(bool connected) override { gridConnected_ = connected; }

    // Pattern banks
    int getCurrentPatternBank() const override { return currentPatternBank_.load(); }
    void setCurrentPatternBank(int bank) override;
    int getCurrentPatternSlot() const override { return currentPatternSlot_.load(); }
    void setCurrentPatternSlot(int slot) override;

    // Step triggers (for sequencer)
    bool getStepTrigger(int engine, int step) override;
    void setStepTrigger(int engine, int step, bool trigger) override;
    bool getNoteOffTrigger(int engine, int step) override;
    void setNoteOffTrigger(int engine, int step, bool trigger) override;

    // Active notes tracking
    int getActiveNote(int engine, int step) override;
    void setActiveNote(int engine, int step, int note) override;
    void clearActiveNote(int engine, int step) override;

    // State validation
    bool isValidEngine(int engine) const override;
    bool isValidStep(int step) const override;
    bool isValidPatternBank(int bank) const override;
    bool isValidPatternSlot(int slot) const override;

    // State persistence
    Result<bool> saveState(const std::string& filename) override;
    Result<bool> loadState(const std::string& filename) override;

private:
    // Transport state
    std::atomic<bool> playing_{false};
    std::atomic<bool> audioRunning_{false};

    // Step sequencer state
    std::atomic<int> currentStep_{0};

    // Current engine/instrument
    std::atomic<int> currentEngine_{0};

    // UI modes
    std::atomic<bool> writeMode_{false};
    std::atomic<bool> engineHold_{false};
    std::atomic<bool> shiftHeld_{false};

    // Grid connection
    std::atomic<bool> gridConnected_{false};

    // Pattern banks
    std::atomic<int> currentPatternBank_{0};
    std::atomic<int> currentPatternSlot_{0};

    // Step triggers for sequencer
    std::array<std::array<std::atomic<bool>, PATTERN_STEPS>, MAX_ENGINES> stepTriggers_;
    std::array<std::array<std::atomic<bool>, PATTERN_STEPS>, MAX_ENGINES> noteOffTriggers_;

    // Active notes tracking
    std::array<std::array<std::atomic<int>, PATTERN_STEPS>, MAX_ENGINES> activeNotes_;
};

} // namespace State
} // namespace GridSequencer