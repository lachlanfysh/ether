#pragma once
#include "IUISystem.h"
#include "../state/IStateManager.h"
#include "../parameter/IParameterSystem.h"
#include "../audio/IAudioEngine.h"
#include "../utils/Logger.h"
#include <memory>
#include <atomic>
#include <sstream>

namespace GridSequencer {
namespace UI {

using State::IStateManager;
using Parameter::IParameterSystem;
using Audio::IAudioEngine;

class TerminalUISystem : public IUISystem {
public:
    TerminalUISystem(
        std::shared_ptr<IStateManager> stateManager,
        std::shared_ptr<IParameterSystem> parameterSystem,
        std::shared_ptr<IAudioEngine> audioEngine
    );
    virtual ~TerminalUISystem();

    // System lifecycle
    Result<bool> initialize() override;
    void shutdown() override;
    bool isInitialized() const override { return initialized_.load(); }

    // Display management
    void render() override;
    void clear() override;
    void setDisplayMode(DisplayMode mode) override { currentMode_ = mode; }
    DisplayMode getCurrentDisplayMode() const override { return currentMode_; }

    // Content rendering
    void renderMainSequencer() override;
    void renderEngineSelect() override;
    void renderParameterEdit() override;
    void renderPatternBank() override;
    void renderSystemStatus(const SystemStatus& status) override;

    // Parameter display
    void renderParameterList(const std::vector<ParameterDisplay>& parameters) override;
    void renderParameterValue(const std::string& name, const std::string& value, bool selected = false) override;

    // Engine display
    void renderEngineStatus(const EngineStatus& status) override;
    void renderEngineList(const std::vector<EngineStatus>& engines, int selectedEngine) override;

    // Text utilities
    void print(const std::string& text, Color color = Color::WHITE) override;
    void printLine(const std::string& text, Color color = Color::WHITE) override;
    void printAt(int x, int y, const std::string& text, Color color = Color::WHITE) override;

    // Cursor and formatting
    void setCursor(int x, int y) override;
    void hideCursor() override;
    void showCursor() override;
    std::string colorize(const std::string& text, Color color) override;

    // Screen information
    int getScreenWidth() const override { return screenWidth_; }
    int getScreenHeight() const override { return screenHeight_; }
    Result<bool> updateScreenSize() override;

    // UI state management
    void setSelectedParameterIndex(int index) { selectedParameterIndex_ = index; }
    int getSelectedParameterIndex() const { return selectedParameterIndex_; }

private:
    std::shared_ptr<IStateManager> stateManager_;
    std::shared_ptr<IParameterSystem> parameterSystem_;
    std::shared_ptr<IAudioEngine> audioEngine_;

    // UI state
    std::atomic<bool> initialized_{false};
    DisplayMode currentMode_{DisplayMode::MAIN_SEQUENCER};
    int selectedParameterIndex_{0};
    int screenWidth_{80};
    int screenHeight_{24};

    // Rendering helpers
    void renderHeader();
    void renderInstructions();
    void renderParameterSection();
    void renderStepSequencer();
    void renderEngineInfo();

    // Parameter helpers
    std::vector<ParameterDisplay> buildParameterDisplayList();
    SystemStatus buildSystemStatus();
    std::vector<EngineStatus> buildEngineStatusList();

    // Terminal control
    void clearScreen();
    void homeCaretAndClearScreen();
    std::string getColorCode(Color color);
    void flushOutput();
};

} // namespace UI
} // namespace GridSequencer