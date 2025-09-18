#pragma once

#include <array>
#include <vector>
#include <cstdint>

#include "../../light_refactor/GridLEDManager.h"
#include "../sequencer/StepData.h"

namespace grid {

template<int MAX_ENGINES>
inline void renderGrid(light::GridLEDManager<16,8>& leds,
                       bool patternHold,
                       bool chainingMode,
                       int currentPatternBank,
                       int currentPatternSlot,
                       const std::vector<int>& patternChain,
                       bool muteHold,
                       int soloEngine,
                       const bool* rowMuted,
                       bool engineHold,
                       bool writeMode,
                       int currentEngineRow,
                       bool isCurrentEngineDrum,
                       int selectedDrumPad,
                       const std::array<uint16_t,16>& drumMasks,
                       const std::array<std::vector<StepData>, MAX_ENGINES>& enginePatterns,
                       int currentStep,
                       bool playing)
{
    constexpr int PAD_ORIGIN_X = 0;
    constexpr int PAD_ORIGIN_Y = 1;
    constexpr int PAD_W = 4;
    constexpr int PAD_H = 4;

    // Pattern hold view: show 4x4 pattern slots
    if (patternHold) {
        for (int i = 0; i < PAD_W * PAD_H; i++) {
            int x = PAD_ORIGIN_X + (i % PAD_W);
            int y = PAD_ORIGIN_Y + (i / PAD_W);
            int brightness = 0;

            if (chainingMode) {
                int absolutePattern = currentPatternBank * 16 + i;
                bool inChain = false;
                for (int p : patternChain) { if (p == absolutePattern) { inChain = true; break; } }
                if (inChain)       brightness = 15;  // Patterns in chain bright
                else if (i == currentPatternSlot) brightness = 8;   // Current pattern medium
                else              brightness = 2;   // Others very dim
            } else {
                brightness = (i == currentPatternSlot) ? 15 : 4;
            }
            if (brightness > 0) leds.set(x, y, (uint8_t)brightness);
        }
        // Pattern button indicator handled by caller (function row)
        return;
    }

    // Mute hold view: show 4x4 engine mute states
    if (muteHold) {
        for (int i = 0; i < PAD_W * PAD_H; i++) {
            int x = PAD_ORIGIN_X + (i % PAD_W);
            int y = PAD_ORIGIN_Y + (i / PAD_W);
            int brightness = 0;
            int eng = i;
            if (eng < MAX_ENGINES) {
                if (soloEngine >= 0) brightness = (eng == soloEngine) ? 15 : 2;
                else                 brightness = rowMuted[eng] ? 2 : 12;
            }
            if (brightness > 0) leds.set(x, y, (uint8_t)brightness);
        }
        return;
    }

    // Engine select view
    if (engineHold) {
        for (int i = 0; i < PAD_W * PAD_H; i++) {
            int x = PAD_ORIGIN_X + (i % PAD_W);
            int y = PAD_ORIGIN_Y + (i / PAD_W);
            int b = (i == currentEngineRow) ? 15 : 4;
            leds.set(x, y, (uint8_t)b);
        }
        return;
    }

    // Write mode
    if (writeMode) {
        int engine = currentEngineRow;
        for (int i = 0; i < PAD_W * PAD_H; i++) {
            int x = PAD_ORIGIN_X + (i % PAD_W);
            int y = PAD_ORIGIN_Y + (i / PAD_W);
            int b = 0;
            if (isCurrentEngineDrum) {
                bool on = (drumMasks[selectedDrumPad] >> i) & 1u;
                b = on ? 12 : ((playing && i == currentStep) ? 2 : 0);
            } else {
                bool ghost = false;
                for (int e = 0; e < MAX_ENGINES; ++e) {
                    if (e == engine) continue;
                    if (e < (int)enginePatterns.size() && i < (int)enginePatterns[e].size() && enginePatterns[e][i].active) { ghost = true; break; }
                }
                if (ghost) b = 3;
                if (i < (int)enginePatterns[engine].size() && enginePatterns[engine][i].active) {
                    b = (playing && i == currentStep) ? 15 : 8;
                } else if (playing && i == currentStep) {
                    b = std::max(b, 2);
                }
            }
            if (b > 0) leds.set(x, y, (uint8_t)b);
        }
        return;
    }

    // Notes mode: show current engine pattern brightly, others ghost; playhead dimly
    for (int i = 0; i < PAD_W * PAD_H; i++) {
        int x = PAD_ORIGIN_X + (i % PAD_W);
        int y = PAD_ORIGIN_Y + (i / PAD_W);
        int b = 0;
        if (isCurrentEngineDrum) {
            bool on = ((drumMasks[selectedDrumPad] >> i) & 1u) != 0;
            if (on) b = 12;
        } else {
            if (i < (int)enginePatterns[currentEngineRow].size() && enginePatterns[currentEngineRow][i].active) {
                b = 12;
            } else {
                bool ghost = false;
                for (int e = 0; e < MAX_ENGINES; ++e) {
                    if (e == currentEngineRow) continue;
                    if (e < (int)enginePatterns.size() && i < (int)enginePatterns[e].size() && enginePatterns[e][i].active) { ghost = true; break; }
                }
                if (ghost) b = 3;
            }
        }
        if (playing && i == currentStep) b = std::max(b, 2);
        if (b > 0) leds.set(x, y, (uint8_t)b);
    }
}

} // namespace grid
