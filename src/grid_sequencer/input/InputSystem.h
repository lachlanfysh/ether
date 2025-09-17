#pragma once
#include "IInputSystem.h"
#include "../state/IStateManager.h"
#include "../utils/Logger.h"
#include <memory>
#include <atomic>
#include <chrono>
#include <map>
#include <termios.h>

namespace GridSequencer {
namespace Input {

using State::IStateManager;

class InputSystem : public IInputSystem {
public:
    explicit InputSystem(std::shared_ptr<IStateManager> stateManager);
    virtual ~InputSystem();

    // System lifecycle
    Result<bool> initialize() override;
    void shutdown() override;
    bool isInitialized() const override { return initialized_.load(); }

    // Input processing
    void processInput() override;
    void setEventHandler(InputEventHandler handler) override { eventHandler_ = handler; }

    // Grid input handling
    void handleGridKey(int x, int y, int state) override;

    // Keyboard input handling
    void enableKeyboardInput() override;
    void disableKeyboardInput() override;
    bool isKeyboardEnabled() const override { return keyboardEnabled_.load(); }

    // Encoder input handling
    void handleEncoderTurn(int encoderId, int delta) override;
    void handleEncoderButton(int encoderId, int state) override;

    // Input state queries
    bool isKeyPressed(char key) const override;
    bool isShiftHeld() const override;

    // Configuration
    void setDebounceTime(int milliseconds) override { debounceTimeMs_ = milliseconds; }
    void setKeyRepeatRate(int rate) override { keyRepeatRate_ = rate; }

private:
    std::shared_ptr<IStateManager> stateManager_;

    // System state
    std::atomic<bool> initialized_{false};
    std::atomic<bool> keyboardEnabled_{false};

    // Event handling
    InputEventHandler eventHandler_;

    // Keyboard state
    termios originalTermios_;
    std::map<char, bool> keyStates_;
    std::chrono::steady_clock::time_point lastKeyTime_;

    // Input timing
    int debounceTimeMs_{50};
    int keyRepeatRate_{10};

    // Input processing methods
    void processKeyboardInput();
    void processSystemKeys();
    void handleKeyboardChar(char ch);
    void updateKeyState(char key, bool pressed);

    // Terminal management
    Result<bool> setupRawMode();
    void restoreTerminal();
    bool hasKeyboardInput() const;
    char readKeyboardChar();

    // Event dispatching
    void dispatchEvent(const InputEvent& event);
    bool isValidGridPosition(int x, int y) const;
};

} // namespace Input
} // namespace GridSequencer