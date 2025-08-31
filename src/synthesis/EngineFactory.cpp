#include "IEngine.h"
#include "SubtractiveEngine.h"
#include "WavetableEngine.h"
#include "FMEngine.h"
#include "GranularEngine.h"
// TODO: Add back macro engines after fixing compilation

std::unique_ptr<IEngine> EngineFactory::createEngine(EngineType type) {
    switch (type) {
        case EngineType::MACRO_VA:
            // TODO: Fix inheritance - SubtractiveEngine inherits from SynthEngine, not IEngine
            return nullptr;
            
        // TODO: Add all other engine types and proper implementations
        default:
            return nullptr; // Temporary - need to fix inheritance hierarchy
    }
}

const char* EngineFactory::getEngineName(EngineType type) {
    switch (type) {
        case EngineType::MACRO_VA:
            return "MacroVA";
            
        // TODO: Add all other engine names
        default:
            return "Unknown";
    }
}