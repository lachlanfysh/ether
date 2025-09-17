#pragma once
#include "../core/DIContainer.h"
#include <functional>
#include <string>

namespace GridSequencer {
namespace Grid {

using Core::Result;

// Grid key event handler type
using GridKeyHandler = std::function<void(int x, int y, int state)>;

// Grid controller interface - abstracts Monome grid communication
class IGridController {
public:
    virtual ~IGridController() = default;

    // Connection management
    virtual Result<bool> connect() = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // LED control
    virtual void setLED(int x, int y, int brightness) = 0;
    virtual void setLEDLevel(int x, int y, int brightness) = 0;
    virtual void clearAllLEDs() = 0;
    virtual void updateDisplay() = 0;

    // Input handling
    virtual void setKeyHandler(GridKeyHandler handler) = 0;

    // Grid information
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual std::string getDeviceInfo() const = 0;

    // OSC configuration
    virtual void setGridPrefix(const std::string& prefix) = 0;
    virtual void setPort(int port) = 0;
    virtual int getPort() const = 0;
};

} // namespace Grid
} // namespace GridSequencer