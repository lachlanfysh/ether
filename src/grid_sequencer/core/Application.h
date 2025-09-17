#pragma once
#include "IApplication.h"
#include "../audio/IAudioEngine.h"
#include "../parameter/IParameterSystem.h"
#include "../state/IStateManager.h"
#include "../utils/Logger.h"
#include <memory>

namespace GridSequencer {
namespace Core {

class Application : public IApplication {
public:
    Application();
    virtual ~Application();

    // Application lifecycle
    Result<bool> initialize() override;
    void run() override;
    void shutdown() override;
    bool isRunning() const override { return running_; }

    // Component access
    DIContainer& getContainer() override { return container_; }

    // Configuration
    void loadConfiguration() override;
    void saveConfiguration() override;

private:
    DIContainer container_;
    bool running_;
    bool initialized_;

    // Core components (held as shared_ptr for DI)
    std::shared_ptr<Audio::IAudioEngine> audioEngine_;
    std::shared_ptr<Parameter::IParameterSystem> parameterSystem_;
    std::shared_ptr<State::IStateManager> stateManager_;

    // Setup methods
    Result<bool> setupDependencies();
    Result<bool> initializeComponents();
    void registerServices();
};

} // namespace Core
} // namespace GridSequencer