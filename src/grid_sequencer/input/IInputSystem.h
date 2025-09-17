#pragma once
#include "../core/DIContainer.h"
#include <functional>
#include <string>

namespace GridSequencer {
namespace Input {

using Core::Result;

// Input event types
enum class InputType {
    GRID_KEY,
    KEYBOARD_KEY,
    ENCODER_TURN,
    ENCODER_BUTTON
};

// Input event structure
struct InputEvent {
    InputType type;
    int x, y;           // For grid keys
    int state;          // Key state (press/release)
    char key;           // For keyboard
    int encoderId;      // For encoders
    int delta;          // For encoder turns
    std::string data;   // Additional data
};

// Input event handler type
using InputEventHandler = std::function<void(const InputEvent& event)>;

// Input system interface - handles all user input
class IInputSystem {
public:
    virtual ~IInputSystem() = default;

    // System lifecycle
    virtual Result<bool> initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;

    // Input processing
    virtual void processInput() = 0;
    virtual void setEventHandler(InputEventHandler handler) = 0;

    // Grid input handling
    virtual void handleGridKey(int x, int y, int state) = 0;

    // Keyboard input handling
    virtual void enableKeyboardInput() = 0;
    virtual void disableKeyboardInput() = 0;
    virtual bool isKeyboardEnabled() const = 0;

    // Encoder input handling
    virtual void handleEncoderTurn(int encoderId, int delta) = 0;
    virtual void handleEncoderButton(int encoderId, int state) = 0;

    // Input state queries
    virtual bool isKeyPressed(char key) const = 0;
    virtual bool isShiftHeld() const = 0;

    // Configuration
    virtual void setDebounceTime(int milliseconds) = 0;
    virtual void setKeyRepeatRate(int rate) = 0;
};

} // namespace Input
} // namespace GridSequencer