#include "Application.h"
#include "../audio/EtherSynthAudioEngine.h"
#include "../parameter/ParameterSystem.h"
#include "../state/StateManager.h"

namespace GridSequencer {
namespace Core {

Application::Application()
    : running_(false), initialized_(false) {
    LOG_INFO("GridSequencer Application created");
}

Application::~Application() {
    shutdown();
    LOG_INFO("GridSequencer Application destroyed");
}

Result<bool> Application::initialize() {
    if (initialized_) {
        return Result<bool>::success(true);
    }

    LOG_INFO("Initializing GridSequencer Application...");

    // Setup dependency injection
    auto setupResult = setupDependencies();
    if (setupResult.isError()) {
        return setupResult;
    }

    // Initialize core components
    auto initResult = initializeComponents();
    if (initResult.isError()) {
        return initResult;
    }

    // Register all services in DI container
    registerServices();

    initialized_ = true;
    LOG_INFO("GridSequencer Application initialized successfully");
    return Result<bool>::success(true);
}

void Application::run() {
    if (!initialized_) {
        LOG_ERROR("Cannot run application - not initialized");
        return;
    }

    running_ = true;
    LOG_INFO("GridSequencer Application running");

    // For now, this is a placeholder - the actual main loop would be implemented here
    // In Phase 3, this would coordinate the UI system, input handling, etc.
    while (running_) {
        // Main application loop would go here
        // For now, just break to avoid infinite loop
        break;
    }

    LOG_INFO("GridSequencer Application main loop ended");
}

void Application::shutdown() {
    if (!initialized_) {
        return;
    }

    running_ = false;

    // Shutdown components in reverse order
    if (audioEngine_) {
        audioEngine_->shutdown();
    }

    // Clear container
    container_.clear();

    initialized_ = false;
    LOG_INFO("GridSequencer Application shut down");
}

void Application::loadConfiguration() {
    // Placeholder for configuration loading
    LOG_INFO("Loading configuration...");
}

void Application::saveConfiguration() {
    // Placeholder for configuration saving
    LOG_INFO("Saving configuration...");
}

Result<bool> Application::setupDependencies() {
    try {
        // Create core components
        audioEngine_ = std::make_shared<Audio::EtherSynthAudioEngine>();
        stateManager_ = std::make_shared<State::StateManager>();
        parameterSystem_ = std::make_shared<Parameter::ParameterSystem>(audioEngine_);

        LOG_DEBUG("Core components created");
        return Result<bool>::success(true);

    } catch (const std::exception& e) {
        return Result<bool>::error("Failed to setup dependencies: " + std::string(e.what()));
    }
}

Result<bool> Application::initializeComponents() {
    // Initialize audio engine
    if (!audioEngine_->initialize()) {
        return Result<bool>::error("Failed to initialize audio engine");
    }

    // Initialize parameter system defaults
    for (int engine = 0; engine < MAX_ENGINES; ++engine) {
        parameterSystem_->initializeDefaults(engine);
    }

    LOG_DEBUG("Components initialized");
    return Result<bool>::success(true);
}

void Application::registerServices() {
    // Register all services in DI container for other components to use
    container_.registerSingleton<Audio::IAudioEngine, Audio::EtherSynthAudioEngine>(*audioEngine_);
    container_.registerSingleton<Parameter::IParameterSystem, Parameter::ParameterSystem>(*parameterSystem_);
    container_.registerSingleton<State::IStateManager, State::StateManager>(*stateManager_);

    LOG_DEBUG("Services registered in DI container");
}

} // namespace Core
} // namespace GridSequencer