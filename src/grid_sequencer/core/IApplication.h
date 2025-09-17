#pragma once
#include "DIContainer.h"

namespace GridSequencer {
namespace Core {

// Main application interface - coordinates all components
class IApplication {
public:
    virtual ~IApplication() = default;

    // Application lifecycle
    virtual Result<bool> initialize() = 0;
    virtual void run() = 0;
    virtual void shutdown() = 0;
    virtual bool isRunning() const = 0;

    // Component access
    virtual DIContainer& getContainer() = 0;

    // Configuration
    virtual void loadConfiguration() = 0;
    virtual void saveConfiguration() = 0;
};

} // namespace Core
} // namespace GridSequencer