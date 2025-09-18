#pragma once

#include "../../src/core/Types.h"

// These inline helpers are included in the main TU to avoid changing linkage.
// They rely on symbols (etherEngine, rowToSlot, engineParameters) defined there.

enum class ParamRoute : uint8_t { Engine, PostFX, Unsupported };

inline ParamRoute resolveParamRoute(int row, int pid) {
    extern int rowToSlot[];
    extern void* etherEngine;
    if (row < 0) row = 0;
    int slot = rowToSlot[row]; if (slot < 0) slot = 0;

    // Prefer engine parameters when the bridge reports support.
    if (ether_engine_has_parameter(etherEngine, slot, pid)) {
        return ParamRoute::Engine;
    }

    #if defined(ETHER_HAVE_GLOBAL_FILTER_FX)
    if (pid == static_cast<int>(ParameterID::FILTER_CUTOFF) ||
        pid == static_cast<int>(ParameterID::FILTER_RESONANCE) ||
        pid == static_cast<int>(ParameterID::HPF) ||
        pid == static_cast<int>(ParameterID::AMPLITUDE) ||
        pid == static_cast<int>(ParameterID::CLIP)) {
        return ParamRoute::PostFX;
    }
    #endif

    return ParamRoute::Unsupported;
}

inline const char* routeTag(ParamRoute r) {
    switch (r) {
        case ParamRoute::Engine:  return "[E]";
        case ParamRoute::PostFX:  return "[FX]";
        default:                  return "[—]";
    }
}

inline float getParamNormForDisplay(int row, int pid) {
    extern std::array<std::map<int, float>, MAX_ENGINES> engineParameters;
    return engineParameters[row][pid];
}

inline const char* pidName(int pid) {
    switch ((ParameterID)pid) {
        case ParameterID::FILTER_CUTOFF:    return "LPF";
        case ParameterID::FILTER_RESONANCE: return "RES";
        case ParameterID::HPF:              return "HPF";
        case ParameterID::AMPLITUDE:        return "AMP";
        case ParameterID::CLIP:             return "CLIP";
        default:                            return "OTHER";
    }
}

inline void debugPrintParamSupportAllRows() {
    extern int rowToSlot[];
    extern void* etherEngine;
    printf("\n=== Engine Param Support (LPF/RES/HPF/AMP/CLIP) ===\n");
    for (int row = 0; row < 16; ++row) {
        int slot = rowToSlot[row]; if (slot < 0) slot = 0;
        int t = ether_get_instrument_engine_type(etherEngine, slot);
        const char* name = ether_get_engine_type_name(t);
        int pids[] = {(int)ParameterID::FILTER_CUTOFF,(int)ParameterID::FILTER_RESONANCE,(int)ParameterID::HPF,(int)ParameterID::AMPLITUDE,(int)ParameterID::CLIP};
        printf("Row %02d  %-12s  ", row, name ? name : "Unknown");
        for (int pid : pids) {
            ParamRoute r = resolveParamRoute(row, pid);
            printf("%s=%s  ", pidName(pid), routeTag(r));
        }
        printf("\n");
    }
    printf("=== end ===\n");
}

