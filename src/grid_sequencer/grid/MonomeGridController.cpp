#include "MonomeGridController.h"
#include "../utils/MathUtils.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace GridSequencer {
namespace Grid {

using Utils::clamp;

MonomeGridController::MonomeGridController(std::shared_ptr<IStateManager> stateManager)
    : stateManager_(stateManager)
    , server_(nullptr)
    , gridAddress_(nullptr)
    , gridPrefix_("/monome")
    , localPort_(DEFAULT_GRID_OSC_PORT)
    , connected_(false)
    , initialized_(false)
    , devicePort_(8080) {

    LOG_INFO("MonomeGridController created");
}

MonomeGridController::~MonomeGridController() {
    disconnect();
    LOG_INFO("MonomeGridController destroyed");
}

Result<bool> MonomeGridController::connect() {
    if (connected_.load()) {
        return Result<bool>::success(true);
    }

    LOG_INFO("Connecting to Monome grid...");

    // Setup OSC server
    auto serverResult = setupOSCServer();
    if (serverResult.isError()) {
        return serverResult;
    }

    // Discover and connect to device
    auto discoverResult = discoverDevice();
    if (discoverResult.isError()) {
        shutdownOSCServer();
        return discoverResult;
    }

    connected_ = true;
    stateManager_->setGridConnected(true);

    LOG_INFO("Grid connected successfully");
    return Result<bool>::success(true);
}

void MonomeGridController::disconnect() {
    if (!connected_.load()) {
        return;
    }

    connected_ = false;
    stateManager_->setGridConnected(false);

    // Clear all LEDs before disconnecting
    clearAllLEDs();

    // Free grid address
    if (gridAddress_) {
        lo_address_free(gridAddress_);
        gridAddress_ = nullptr;
    }

    // Shutdown OSC server
    shutdownOSCServer();

    LOG_INFO("Grid disconnected");
}

void MonomeGridController::setLED(int x, int y, int brightness) {
    setLEDLevel(x, y, brightness);
}

void MonomeGridController::setLEDLevel(int x, int y, int brightness) {
    if (!connected_.load() || !gridAddress_ || !isValidPosition(x, y)) {
        return;
    }

    int clampedBrightness = clamp(brightness, 0, 15);
    if (clampedBrightness > 0) {
        lo_send(gridAddress_, (gridPrefix_ + "/grid/led/level/set").c_str(), "iii", x, y, clampedBrightness);
    }
}

void MonomeGridController::clearAllLEDs() {
    if (!connected_.load() || !gridAddress_) {
        return;
    }

    lo_send(gridAddress_, (gridPrefix_ + "/grid/led/all").c_str(), "i", 0);
}

void MonomeGridController::updateDisplay() {
    if (!connected_.load()) {
        return;
    }

    // Clear all LEDs first
    clearAllLEDs();

    // Update different display modes based on state
    if (stateManager_->isEngineHold()) {
        updateEngineSelectDisplay();
    } else {
        updateStepSequencerDisplay();
    }

    // Always update function buttons
    updateFunctionButtons();
}

std::string MonomeGridController::getDeviceInfo() const {
    return "Monome Grid (" + deviceId_ + ") on port " + std::to_string(devicePort_);
}

Result<bool> MonomeGridController::discoverDevice() {
    // Create address for device discovery
    if (gridAddress_) {
        lo_address_free(gridAddress_);
    }

    // Default to localhost:8080 (serialosc default)
    gridAddress_ = lo_address_new("127.0.0.1", std::to_string(devicePort_).c_str());
    if (!gridAddress_) {
        return Result<bool>::error("Failed to create grid address");
    }

    // Send device configuration
    sendDeviceConfiguration();

    LOG_INFO("Grid device discovery initiated");
    return Result<bool>::success(true);
}

void MonomeGridController::sendDeviceConfiguration() {
    if (!gridAddress_) {
        return;
    }

    // Configure device for our OSC server
    lo_send(gridAddress_, "/sys/host", "s", "127.0.0.1");
    lo_send(gridAddress_, "/sys/port", "i", localPort_);
    lo_send(gridAddress_, "/sys/prefix", "s", gridPrefix_.c_str());
    lo_send(gridAddress_, "/sys/info", "");

    LOG_DEBUG("Device configuration sent");
}

Result<bool> MonomeGridController::setupOSCServer() {
    if (initialized_.load()) {
        return Result<bool>::success(true);
    }

    // Create OSC server
    server_ = lo_server_thread_new(std::to_string(localPort_).c_str(), nullptr);
    if (!server_) {
        return Result<bool>::error("Failed to create OSC server on port " + std::to_string(localPort_));
    }

    // Add method handlers
    lo_server_thread_add_method(server_, (gridPrefix_ + "/grid/key").c_str(), "iii", gridKeyHandler, this);
    lo_server_thread_add_method(server_, "/serialosc/device", "ssi", serialOSCDeviceHandler, this);
    lo_server_thread_add_method(server_, "/serialosc/add", "ssi", serialOSCDeviceHandler, this);

    // Start server thread
    lo_server_thread_start(server_);

    initialized_ = true;
    LOG_INFO("OSC server started on port " + std::to_string(localPort_));
    return Result<bool>::success(true);
}

void MonomeGridController::shutdownOSCServer() {
    if (!initialized_.load()) {
        return;
    }

    if (server_) {
        lo_server_thread_stop(server_);
        lo_server_thread_free(server_);
        server_ = nullptr;
    }

    initialized_ = false;
    LOG_INFO("OSC server shut down");
}

void MonomeGridController::updateEngineSelectDisplay() {
    static constexpr int PAD_W = 4;
    static constexpr int PAD_H = 4;
    static constexpr int PAD_ORIGIN_X = 0;
    static constexpr int PAD_ORIGIN_Y = 1;

    // Show engine selection across 4x4 grid
    for (int i = 0; i < PAD_W * PAD_H; i++) {
        int x = PAD_ORIGIN_X + (i % PAD_W);
        int y = PAD_ORIGIN_Y + (i / PAD_W);
        int brightness = (i == stateManager_->getCurrentEngine()) ? 15 : 4;
        setLEDLevel(x, y, brightness);
    }
}

void MonomeGridController::updateStepSequencerDisplay() {
    static constexpr int PAD_W = 4;
    static constexpr int PAD_H = 4;
    static constexpr int PAD_ORIGIN_X = 0;
    static constexpr int PAD_ORIGIN_Y = 1;

    // Update 4x4 step display - simplified for now
    for (int i = 0; i < 16; i++) {
        int x = PAD_ORIGIN_X + (i % PAD_W);
        int y = PAD_ORIGIN_Y + (i / PAD_W);

        int brightness = 4; // Default dim

        // Highlight current step
        if (stateManager_->isPlaying() && i == stateManager_->getCurrentStep()) {
            brightness = 15;
        }
        // Show active steps with medium brightness
        else if (stateManager_->getStepTrigger(stateManager_->getCurrentEngine(), i)) {
            brightness = 8;
        }

        setLEDLevel(x, y, brightness);
    }
}

void MonomeGridController::updateFunctionButtons() {
    // Function buttons based on current grid layout
    // SHIFT button at (0,0)
    setLEDLevel(0, 0, stateManager_->isShiftHeld() ? 15 : 4);

    // ENGINE button at (1,0)
    setLEDLevel(1, 0, stateManager_->isEngineHold() ? 15 : 4);

    // PATTERN button at (2,0)
    setLEDLevel(2, 0, 4);

    // Clear button at (3,0)
    setLEDLevel(3, 0, 4);

    // PLAY/PAUSE button at (4,0)
    setLEDLevel(4, 0, stateManager_->isPlaying() ? 15 : 4);

    // COPY button at (4,2)
    setLEDLevel(4, 2, 4); // Would need copy mode state

    // DELETE button at (4,3)
    setLEDLevel(4, 3, 4); // Would need delete mode state

    // WRITE button at (4,4)
    setLEDLevel(4, 4, stateManager_->isWriteMode() ? 15 : 4);
}

bool MonomeGridController::isValidPosition(int x, int y) const {
    return x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT;
}

// Static C callback handlers
int MonomeGridController::gridKeyHandler(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data) {
    (void)path; (void)types; (void)argc; (void)msg; // Suppress unused warnings

    auto* controller = static_cast<MonomeGridController*>(user_data);
    if (!controller || !controller->keyHandler_) {
        return 0;
    }

    if (argc >= 3) {
        int x = argv[0]->i;
        int y = argv[1]->i;
        int state = argv[2]->i;

        // Call the registered key handler
        controller->keyHandler_(x, y, state);
    }

    return 0;
}

int MonomeGridController::serialOSCDeviceHandler(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data) {
    (void)path; (void)types; (void)argc; (void)msg; // Suppress unused warnings

    auto* controller = static_cast<MonomeGridController*>(user_data);
    if (!controller) {
        return 0;
    }

    if (argc >= 3) {
        std::string deviceId = &argv[0]->s;
        std::string deviceType = &argv[1]->s;
        int devicePort = argv[2]->i;

        controller->deviceId_ = deviceId;
        controller->devicePort_ = devicePort;

        LOG_INFO("SerialOSC device found: " + deviceId + " (" + deviceType + ") on port " + std::to_string(devicePort));

        // Update grid address with discovered port
        if (controller->gridAddress_) {
            lo_address_free(controller->gridAddress_);
        }
        controller->gridAddress_ = lo_address_new("127.0.0.1", std::to_string(devicePort).c_str());

        if (controller->gridAddress_) {
            controller->sendDeviceConfiguration();
        }
    }

    return 0;
}

} // namespace Grid
} // namespace GridSequencer