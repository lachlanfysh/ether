#include "StateManager.h"
#include "../utils/MathUtils.h"
#include <fstream>
#include <algorithm>

namespace GridSequencer {
namespace State {

using Utils::clamp;

StateManager::StateManager() {
    // Initialize active notes to -1 (no note)
    for (int engine = 0; engine < MAX_ENGINES; ++engine) {
        for (int step = 0; step < PATTERN_STEPS; ++step) {
            activeNotes_[engine][step] = -1;
            stepTriggers_[engine][step] = false;
            noteOffTriggers_[engine][step] = false;
        }
    }

    LOG_INFO("StateManager initialized");
}

void StateManager::setCurrentStep(int step) {
    if (isValidStep(step)) {
        currentStep_ = step;
    } else {
        LOG_WARNING("Invalid step index: " + std::to_string(step));
    }
}

void StateManager::advanceStep() {
    int current = currentStep_.load();
    int next = (current + 1) % PATTERN_STEPS;
    currentStep_ = next;
}

void StateManager::setCurrentEngine(int engine) {
    if (isValidEngine(engine)) {
        currentEngine_ = engine;
    } else {
        LOG_WARNING("Invalid engine index: " + std::to_string(engine));
    }
}

void StateManager::setCurrentPatternBank(int bank) {
    if (isValidPatternBank(bank)) {
        currentPatternBank_ = bank;
    } else {
        LOG_WARNING("Invalid pattern bank: " + std::to_string(bank));
    }
}

void StateManager::setCurrentPatternSlot(int slot) {
    if (isValidPatternSlot(slot)) {
        currentPatternSlot_ = slot;
    } else {
        LOG_WARNING("Invalid pattern slot: " + std::to_string(slot));
    }
}

bool StateManager::getStepTrigger(int engine, int step) {
    if (!isValidEngine(engine) || !isValidStep(step)) {
        return false;
    }
    return stepTriggers_[engine][step].load();
}

void StateManager::setStepTrigger(int engine, int step, bool trigger) {
    if (isValidEngine(engine) && isValidStep(step)) {
        stepTriggers_[engine][step] = trigger;
    }
}

bool StateManager::getNoteOffTrigger(int engine, int step) {
    if (!isValidEngine(engine) || !isValidStep(step)) {
        return false;
    }
    return noteOffTriggers_[engine][step].load();
}

void StateManager::setNoteOffTrigger(int engine, int step, bool trigger) {
    if (isValidEngine(engine) && isValidStep(step)) {
        noteOffTriggers_[engine][step] = trigger;
    }
}

int StateManager::getActiveNote(int engine, int step) {
    if (!isValidEngine(engine) || !isValidStep(step)) {
        return -1;
    }
    return activeNotes_[engine][step].load();
}

void StateManager::setActiveNote(int engine, int step, int note) {
    if (isValidEngine(engine) && isValidStep(step)) {
        // Clamp note to valid MIDI range
        int clampedNote = clamp(note, 0, 127);
        activeNotes_[engine][step] = clampedNote;
    }
}

void StateManager::clearActiveNote(int engine, int step) {
    if (isValidEngine(engine) && isValidStep(step)) {
        activeNotes_[engine][step] = -1;
    }
}

bool StateManager::isValidEngine(int engine) const {
    return engine >= 0 && engine < MAX_ENGINES;
}

bool StateManager::isValidStep(int step) const {
    return step >= 0 && step < PATTERN_STEPS;
}

bool StateManager::isValidPatternBank(int bank) const {
    return bank >= 0 && bank < PATTERN_BANKS;
}

bool StateManager::isValidPatternSlot(int slot) const {
    return slot >= 0 && slot < PATTERNS_PER_BANK;
}

Result<bool> StateManager::saveState(const std::string& filename) {
    try {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return Result<bool>::error("Failed to open file for writing: " + filename);
        }

        // Save basic state
        bool playing = playing_.load();
        int currentStep = currentStep_.load();
        int currentEngine = currentEngine_.load();
        int currentBank = currentPatternBank_.load();
        int currentSlot = currentPatternSlot_.load();

        file.write(reinterpret_cast<const char*>(&playing), sizeof(playing));
        file.write(reinterpret_cast<const char*>(&currentStep), sizeof(currentStep));
        file.write(reinterpret_cast<const char*>(&currentEngine), sizeof(currentEngine));
        file.write(reinterpret_cast<const char*>(&currentBank), sizeof(currentBank));
        file.write(reinterpret_cast<const char*>(&currentSlot), sizeof(currentSlot));

        // Save active notes
        for (int engine = 0; engine < MAX_ENGINES; ++engine) {
            for (int step = 0; step < PATTERN_STEPS; ++step) {
                int note = activeNotes_[engine][step].load();
                file.write(reinterpret_cast<const char*>(&note), sizeof(note));
            }
        }

        file.close();
        LOG_INFO("State saved to: " + filename);
        return Result<bool>::success(true);

    } catch (const std::exception& e) {
        return Result<bool>::error("Failed to save state: " + std::string(e.what()));
    }
}

Result<bool> StateManager::loadState(const std::string& filename) {
    try {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return Result<bool>::error("Failed to open file for reading: " + filename);
        }

        // Load basic state
        bool playing;
        int currentStep, currentEngine, currentBank, currentSlot;

        file.read(reinterpret_cast<char*>(&playing), sizeof(playing));
        file.read(reinterpret_cast<char*>(&currentStep), sizeof(currentStep));
        file.read(reinterpret_cast<char*>(&currentEngine), sizeof(currentEngine));
        file.read(reinterpret_cast<char*>(&currentBank), sizeof(currentBank));
        file.read(reinterpret_cast<char*>(&currentSlot), sizeof(currentSlot));

        // Validate and set state
        playing_ = playing;
        setCurrentStep(currentStep);
        setCurrentEngine(currentEngine);
        setCurrentPatternBank(currentBank);
        setCurrentPatternSlot(currentSlot);

        // Load active notes
        for (int engine = 0; engine < MAX_ENGINES; ++engine) {
            for (int step = 0; step < PATTERN_STEPS; ++step) {
                int note;
                file.read(reinterpret_cast<char*>(&note), sizeof(note));
                activeNotes_[engine][step] = note;
            }
        }

        file.close();
        LOG_INFO("State loaded from: " + filename);
        return Result<bool>::success(true);

    } catch (const std::exception& e) {
        return Result<bool>::error("Failed to load state: " + std::string(e.what()));
    }
}

} // namespace State
} // namespace GridSequencer