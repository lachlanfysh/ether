#pragma once

#include <array>
#include <vector>
#include <atomic>
#include "../sequencer/StepData.h"

namespace pattern {

inline int absoluteIndex(int bank, int slot) { return bank * 16 + slot; }

inline int currentAbsoluteIndex(const std::atomic<int>& bank, const std::atomic<int>& slot) {
    return absoluteIndex(bank.load(), slot.load());
}

template<size_t NEngines>
void initializeBank(std::array<std::array<std::vector<StepData>, NEngines>, 64>& bank) {
    for (int pattern = 0; pattern < 64; ++pattern) {
        for (size_t engine = 0; engine < NEngines; ++engine) {
            auto& steps = bank[pattern][engine];
            steps.resize(16);
            for (auto& s : steps) { s.active = false; s.note = 60; s.velocity = 0.6f; }
        }
    }
}

template<size_t NEngines>
void saveToBank(const std::array<std::vector<StepData>, NEngines>& engines,
                std::array<std::array<std::vector<StepData>, NEngines>, 64>& bank,
                const std::atomic<int>& bankIdx,
                int slot) {
    const int abs = absoluteIndex(bankIdx.load(), slot);
    if (abs < 0 || abs >= 64) return;
    for (size_t e = 0; e < NEngines; ++e) bank[abs][e] = engines[e];
}

template<size_t NEngines>
void loadFromBank(std::array<std::vector<StepData>, NEngines>& engines,
                  const std::array<std::array<std::vector<StepData>, NEngines>, 64>& bank,
                  const std::atomic<int>& bankIdx,
                  std::atomic<int>& currentSlot,
                  int slot) {
    const int abs = absoluteIndex(bankIdx.load(), slot);
    if (abs < 0 || abs >= 64) return;
    for (size_t e = 0; e < NEngines; ++e) engines[e] = bank[abs][e];
    currentSlot = slot;
}

template<size_t NEngines>
bool cloneCurrent(const std::array<std::vector<StepData>, NEngines>& engines,
                  std::array<std::array<std::vector<StepData>, NEngines>, 64>& bank,
                  const std::atomic<int>& bankIdx,
                  int currentSlot,
                  int& outTargetSlot) {
    // Find next empty slot in current bank
    outTargetSlot = -1;
    for (int i = 0; i < 16; ++i) {
        if (i == currentSlot) continue;
        int abs = absoluteIndex(bankIdx.load(), i);
        bool empty = true;
        for (size_t e = 0; e < NEngines && empty; ++e) {
            for (const auto& s : bank[abs][e]) { if (s.active) { empty = false; break; } }
        }
        if (empty) { outTargetSlot = i; break; }
    }
    if (outTargetSlot < 0) return false;
    const int absTarget = absoluteIndex(bankIdx.load(), outTargetSlot);
    for (size_t e = 0; e < NEngines; ++e) bank[absTarget][e] = engines[e];
    return true;
}

} // namespace pattern

