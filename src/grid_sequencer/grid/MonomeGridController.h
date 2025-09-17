#pragma once
#include "IGridController.h"
#include "../state/IStateManager.h"
#include "../utils/Logger.h"
#include "../utils/Constants.h"
#include <lo/lo.h>
#include <string>
#include <memory>
#include <atomic>

namespace GridSequencer {
namespace Grid {

using State::IStateManager;

class MonomeGridController : public IGridController {
public:
    explicit MonomeGridController(std::shared_ptr<IStateManager> stateManager);
    virtual ~MonomeGridController();

    // Connection management
    Result<bool> connect() override;
    void disconnect() override;
    bool isConnected() const override { return connected_.load(); }

    // LED control
    void setLED(int x, int y, int brightness) override;
    void setLEDLevel(int x, int y, int brightness) override;
    void clearAllLEDs() override;
    void updateDisplay() override;

    // Input handling
    void setKeyHandler(GridKeyHandler handler) override { keyHandler_ = handler; }

    // Grid information
    int getWidth() const override { return GRID_WIDTH; }
    int getHeight() const override { return GRID_HEIGHT; }
    std::string getDeviceInfo() const override;

    // OSC configuration
    void setGridPrefix(const std::string& prefix) override { gridPrefix_ = prefix; }
    void setPort(int port) override { localPort_ = port; }
    int getPort() const override { return localPort_; }

    // Device discovery
    Result<bool> discoverDevice();

private:
    std::shared_ptr<IStateManager> stateManager_;

    // OSC communication
    lo_server_thread server_;
    lo_address gridAddress_;
    std::string gridPrefix_;
    int localPort_;

    // Connection state
    std::atomic<bool> connected_;
    std::atomic<bool> initialized_;

    // Input handling
    GridKeyHandler keyHandler_;

    // Device information
    std::string deviceId_;
    int devicePort_;

    // Static OSC handlers (C callbacks)
    static int gridKeyHandler(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data);
    static int serialOSCDeviceHandler(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data);

    // LED update methods
    void updatePatternDisplay();
    void updateEngineSelectDisplay();
    void updateMuteDisplay();
    void updateFunctionButtons();
    void updateStepSequencerDisplay();

    // Helper methods
    bool isValidPosition(int x, int y) const;
    void sendDeviceConfiguration();
    Result<bool> setupOSCServer();
    void shutdownOSCServer();
};

} // namespace Grid
} // namespace GridSequencer