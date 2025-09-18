// Simple composition context to wire subsystems. No DI container.

#pragma once

#include "EngineBridge.h"
#include "GridLEDManager.h"
#include "ParameterCache.h"

namespace light {

struct AppContext {
    // Engine handle is owned externally by the monolith for now.
    EngineBridge::Handle engine{nullptr};

    // Utilities available to UI/IO threads.
    GridLEDManager<16, 8> leds; // default 16x8 grid
    ParameterCache params;
};

} // namespace light

