#include "InputSystem.h"
#include "../utils/Constants.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

namespace GridSequencer {
namespace Input {

InputSystem::InputSystem(std::shared_ptr<IStateManager> stateManager)
    : stateManager_(stateManager) {
    LOG_INFO("InputSystem created");
}

InputSystem::~InputSystem() {
    shutdown();
    LOG_INFO("InputSystem destroyed");
}

Result<bool> InputSystem::initialize() {
    if (initialized_.load()) {
        return Result<bool>::success(true);
    }

    LOG_INFO("Initializing InputSystem...");

    // Setup keyboard input if needed
    // Note: Raw mode will be enabled when keyboard input is explicitly enabled

    initialized_ = true;
    LOG_INFO("InputSystem initialized");
    return Result<bool>::success(true);
}

void InputSystem::shutdown() {
    if (!initialized_.load()) {
        return;
    }

    disableKeyboardInput();
    initialized_ = false;
    LOG_INFO("InputSystem shut down");
}

void InputSystem::processInput() {
    if (!initialized_.load()) {
        return;
    }

    if (keyboardEnabled_.load()) {
        processKeyboardInput();
    }
}

void InputSystem::handleGridKey(int x, int y, int state) {
    if (!isValidGridPosition(x, y)) {
        LOG_WARNING("Invalid grid position: (" + std::to_string(x) + ", " + std::to_string(y) + ")");
        return;
    }

    InputEvent event;
    event.type = InputType::GRID_KEY;
    event.x = x;
    event.y = y;
    event.state = state;

    dispatchEvent(event);
}

void InputSystem::enableKeyboardInput() {
    if (keyboardEnabled_.load()) {
        return;
    }

    auto result = setupRawMode();
    if (result.isError()) {
        LOG_ERROR("Failed to enable keyboard input: " + result.error());
        return;
    }

    keyboardEnabled_ = true;
    LOG_INFO("Keyboard input enabled");
}

void InputSystem::disableKeyboardInput() {
    if (!keyboardEnabled_.load()) {
        return;
    }

    restoreTerminal();
    keyboardEnabled_ = false;
    LOG_INFO("Keyboard input disabled");
}

void InputSystem::handleEncoderTurn(int encoderId, int delta) {
    InputEvent event;
    event.type = InputType::ENCODER_TURN;
    event.encoderId = encoderId;
    event.delta = delta;

    dispatchEvent(event);
}

void InputSystem::handleEncoderButton(int encoderId, int state) {
    InputEvent event;
    event.type = InputType::ENCODER_BUTTON;
    event.encoderId = encoderId;
    event.state = state;

    dispatchEvent(event);
}

bool InputSystem::isKeyPressed(char key) const {
    auto it = keyStates_.find(key);
    return it != keyStates_.end() && it->second;
}

bool InputSystem::isShiftHeld() const {
    return stateManager_->isShiftHeld();
}

void InputSystem::processKeyboardInput() {
    while (hasKeyboardInput()) {
        char ch = readKeyboardChar();
        if (ch != 0) {
            handleKeyboardChar(ch);
        }
    }
}

void InputSystem::handleKeyboardChar(char ch) {
    // Update key state
    updateKeyState(ch, true);

    // Create input event
    InputEvent event;
    event.type = InputType::KEYBOARD_KEY;
    event.key = ch;
    event.state = 1; // Press

    dispatchEvent(event);

    // Handle special system keys
    processSystemKeys();
}

void InputSystem::processSystemKeys() {
    // Handle quit key
    if (isKeyPressed('q')) {
        LOG_INFO("Quit key pressed");
        // Application should handle this through the event handler
    }

    // Handle shift key (could be mapped to different key)
    if (isKeyPressed('s')) {
        stateManager_->setShiftHeld(true);
    } else {
        stateManager_->setShiftHeld(false);
    }
}

void InputSystem::updateKeyState(char key, bool pressed) {
    keyStates_[key] = pressed;
    lastKeyTime_ = std::chrono::steady_clock::now();

    // Auto-release key after a short time (simulating key release)
    // This is a simplified approach - a full implementation would handle key up/down separately
    if (pressed) {
        // Schedule key release (simplified)
        std::thread([this, key]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            keyStates_[key] = false;
        }).detach();
    }
}

Result<bool> InputSystem::setupRawMode() {
    // Get current terminal attributes
    if (tcgetattr(STDIN_FILENO, &originalTermios_) != 0) {
        return Result<bool>::error("Failed to get terminal attributes: " + std::string(strerror(errno)));
    }

    // Set up raw mode
    termios raw = originalTermios_;
    raw.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHONL | ISIG);
    raw.c_iflag &= ~(IXON | IXOFF | IXANY | IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL);
    raw.c_oflag &= ~OPOST;
    raw.c_cc[VTIME] = 0;
    raw.c_cc[VMIN] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) != 0) {
        return Result<bool>::error("Failed to set raw mode: " + std::string(strerror(errno)));
    }

    // Set stdin to non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    return Result<bool>::success(true);
}

void InputSystem::restoreTerminal() {
    if (keyboardEnabled_.load()) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios_);

        // Restore blocking mode
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);
    }
}

bool InputSystem::hasKeyboardInput() const {
    if (!keyboardEnabled_.load()) {
        return false;
    }

    // Check if there's input available without blocking
    char ch;
    int result = read(STDIN_FILENO, &ch, 1);
    if (result == 1) {
        // Put the character back (simple approach)
        // In a real implementation, we'd use a proper buffer
        return true;
    }
    return false;
}

char InputSystem::readKeyboardChar() {
    if (!keyboardEnabled_.load()) {
        return 0;
    }

    char ch;
    int result = read(STDIN_FILENO, &ch, 1);
    if (result == 1) {
        return ch;
    }
    return 0;
}

void InputSystem::dispatchEvent(const InputEvent& event) {
    if (eventHandler_) {
        eventHandler_(event);
    }

    // Log significant events
    switch (event.type) {
        case InputType::GRID_KEY:
            LOG_DEBUG("Grid key: (" + std::to_string(event.x) + "," + std::to_string(event.y) + ") state=" + std::to_string(event.state));
            break;
        case InputType::KEYBOARD_KEY:
            LOG_DEBUG("Keyboard: '" + std::string(1, event.key) + "'");
            break;
        case InputType::ENCODER_TURN:
            LOG_DEBUG("Encoder " + std::to_string(event.encoderId) + " turn: " + std::to_string(event.delta));
            break;
        case InputType::ENCODER_BUTTON:
            LOG_DEBUG("Encoder " + std::to_string(event.encoderId) + " button: " + std::to_string(event.state));
            break;
    }
}

bool InputSystem::isValidGridPosition(int x, int y) const {
    return x >= 0 && x < GRID_WIDTH && y >= 0 && y < GRID_HEIGHT;
}

} // namespace Input
} // namespace GridSequencer