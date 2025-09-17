#include "TerminalUISystem.h"
#include "../utils/Constants.h"
#include <iostream>
#include <iomanip>
#include <sys/ioctl.h>
#include <unistd.h>

namespace GridSequencer {
namespace UI {

TerminalUISystem::TerminalUISystem(
    std::shared_ptr<IStateManager> stateManager,
    std::shared_ptr<IParameterSystem> parameterSystem,
    std::shared_ptr<IAudioEngine> audioEngine)
    : stateManager_(stateManager)
    , parameterSystem_(parameterSystem)
    , audioEngine_(audioEngine) {

    LOG_INFO("TerminalUISystem created");
}

TerminalUISystem::~TerminalUISystem() {
    shutdown();
    LOG_INFO("TerminalUISystem destroyed");
}

Result<bool> TerminalUISystem::initialize() {
    if (initialized_.load()) {
        return Result<bool>::success(true);
    }

    LOG_INFO("Initializing TerminalUISystem...");

    // Update screen size
    auto sizeResult = updateScreenSize();
    if (sizeResult.isError()) {
        LOG_WARNING("Could not determine screen size: " + sizeResult.error());
    }

    // Setup terminal
    hideCursor();
    clear();

    initialized_ = true;
    LOG_INFO("TerminalUISystem initialized");
    return Result<bool>::success(true);
}

void TerminalUISystem::shutdown() {
    if (!initialized_.load()) {
        return;
    }

    showCursor();
    clear();
    initialized_ = false;
    LOG_INFO("TerminalUISystem shut down");
}

void TerminalUISystem::render() {
    if (!initialized_.load()) {
        return;
    }

    // Clear and home cursor
    homeCaretAndClearScreen();

    // Render based on current mode
    switch (currentMode_) {
        case DisplayMode::MAIN_SEQUENCER:
            renderMainSequencer();
            break;
        case DisplayMode::ENGINE_SELECT:
            renderEngineSelect();
            break;
        case DisplayMode::PARAMETER_EDIT:
            renderParameterEdit();
            break;
        case DisplayMode::PATTERN_BANK:
            renderPatternBank();
            break;
        default:
            renderMainSequencer();
            break;
    }

    flushOutput();
}

void TerminalUISystem::clear() {
    clearScreen();
}

void TerminalUISystem::renderMainSequencer() {
    renderHeader();
    renderParameterSection();
    renderInstructions();
}

void TerminalUISystem::renderEngineSelect() {
    renderHeader();
    printLine("Engine Selection Mode", Color::BRIGHT_CYAN);
    printLine("Use arrow keys to select engine, Enter to confirm", Color::YELLOW);

    auto engines = buildEngineStatusList();
    renderEngineList(engines, stateManager_->getCurrentEngine());
}

void TerminalUISystem::renderParameterEdit() {
    renderHeader();
    printLine("Parameter Edit Mode", Color::BRIGHT_GREEN);
    renderParameterSection();
    renderInstructions();
}

void TerminalUISystem::renderPatternBank() {
    renderHeader();
    printLine("Pattern Bank Mode", Color::BRIGHT_MAGENTA);
    printLine("Bank: " + std::to_string(stateManager_->getCurrentPatternBank()) +
              " Slot: " + std::to_string(stateManager_->getCurrentPatternSlot()), Color::WHITE);
}

void TerminalUISystem::renderSystemStatus(const SystemStatus& status) {
    std::ostringstream oss;
    oss << "Ether Grid Sequencer | " << status.buildVersion
        << " | Engine: " << status.currentEngine
        << " | BPM: " << std::fixed << std::setprecision(0) << status.bpm
        << " | " << (status.playing ? "PLAYING" : "STOPPED")
        << " | Step: " << status.currentStep
        << " | Bank " << status.currentBank << " Pattern " << status.currentPattern
        << " | CPU: " << std::fixed << std::setprecision(1) << status.cpuUsage << "%"
        << " | MEM: " << std::fixed << std::setprecision(1) << status.memoryMB << " MB";

    printLine(oss.str(), Color::BRIGHT_WHITE);
}

void TerminalUISystem::renderParameterList(const std::vector<ParameterDisplay>& parameters) {
    printLine("Parameters (↑/↓ select, ←/→ adjust, space play/stop, w write, c clear, q quit)", Color::YELLOW);
    printLine("[E]=Engine  [FX]=Post  [—]=Unsupported", Color::CYAN);

    for (size_t i = 0; i < parameters.size(); ++i) {
        const auto& param = parameters[i];
        const char* sel = (static_cast<int>(i) == selectedParameterIndex_) ? ">" : " ";

        Color valueColor = param.supported ? Color::WHITE : Color::BRIGHT_BLACK;
        if (param.selected) {
            valueColor = Color::BRIGHT_YELLOW;
        }

        std::ostringstream oss;
        oss << sel << " " << param.route << " " << std::left << std::setw(12) << param.name
            << " : " << param.value;

        printLine(oss.str(), valueColor);
    }
}

void TerminalUISystem::renderParameterValue(const std::string& name, const std::string& value, bool selected) {
    Color color = selected ? Color::BRIGHT_YELLOW : Color::WHITE;
    std::ostringstream oss;
    oss << std::left << std::setw(12) << name << " : " << value;
    printLine(oss.str(), color);
}

void TerminalUISystem::renderEngineStatus(const EngineStatus& status) {
    std::ostringstream oss;
    oss << "Engine " << status.engineId << ": " << status.name;
    if (!status.category.empty()) {
        oss << " (" << status.category << ")";
    }
    oss << " | Voices: " << status.voiceCount
        << " | CPU: " << std::fixed << std::setprecision(1) << status.cpuUsage << "%";

    Color color = status.active ? Color::BRIGHT_GREEN : Color::WHITE;
    printLine(oss.str(), color);
}

void TerminalUISystem::renderEngineList(const std::vector<EngineStatus>& engines, int selectedEngine) {
    for (const auto& engine : engines) {
        bool isSelected = (engine.engineId == selectedEngine);
        const char* sel = isSelected ? ">" : " ";

        std::ostringstream oss;
        oss << sel << " Engine " << std::setw(2) << engine.engineId << ": " << engine.name;

        Color color = isSelected ? Color::BRIGHT_YELLOW : Color::WHITE;
        printLine(oss.str(), color);
    }
}

void TerminalUISystem::print(const std::string& text, Color color) {
    std::cout << colorize(text, color);
}

void TerminalUISystem::printLine(const std::string& text, Color color) {
    std::cout << colorize(text, color) << std::endl;
}

void TerminalUISystem::printAt(int x, int y, const std::string& text, Color color) {
    setCursor(x, y);
    print(text, color);
}

void TerminalUISystem::setCursor(int x, int y) {
    std::cout << "\x1b[" << y << ";" << x << "H";
}

void TerminalUISystem::hideCursor() {
    std::cout << "\x1b[?25l";
}

void TerminalUISystem::showCursor() {
    std::cout << "\x1b[?25h";
}

std::string TerminalUISystem::colorize(const std::string& text, Color color) {
    if (color == Color::RESET) {
        return "\x1b[0m" + text;
    }
    return "\x1b[" + std::to_string(static_cast<int>(color)) + "m" + text + "\x1b[0m";
}

Result<bool> TerminalUISystem::updateScreenSize() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        screenWidth_ = w.ws_col;
        screenHeight_ = w.ws_row;
        return Result<bool>::success(true);
    }
    return Result<bool>::error("Failed to get terminal size");
}

void TerminalUISystem::renderHeader() {
    auto status = buildSystemStatus();
    renderSystemStatus(status);
    printLine("", Color::RESET); // Empty line
}

void TerminalUISystem::renderInstructions() {
    printLine("", Color::RESET); // Empty line
    printLine("Controls:", Color::BRIGHT_CYAN);
    printLine("  ↑/↓     Select parameter", Color::WHITE);
    printLine("  ←/→     Adjust parameter", Color::WHITE);
    printLine("  Space   Play/Stop", Color::WHITE);
    printLine("  w       Write mode", Color::WHITE);
    printLine("  e       Engine select", Color::WHITE);
    printLine("  c       Clear pattern", Color::WHITE);
    printLine("  q       Quit", Color::WHITE);
}

void TerminalUISystem::renderParameterSection() {
    auto parameters = buildParameterDisplayList();
    renderParameterList(parameters);
}

std::vector<ParameterDisplay> TerminalUISystem::buildParameterDisplayList() {
    std::vector<ParameterDisplay> parameters;

    int currentEngine = stateManager_->getCurrentEngine();
    auto visibleParams = parameterSystem_->getExtendedParameters(currentEngine);

    for (size_t i = 0; i < visibleParams.size(); ++i) {
        ParameterDisplay param;
        auto paramId = static_cast<ParameterID>(visibleParams[i]);

        param.name = parameterSystem_->getParameterName(paramId);
        param.value = parameterSystem_->getParameterDisplayValue(currentEngine, paramId);

        auto route = parameterSystem_->getParameterRoute(currentEngine, paramId);
        param.route = parameterSystem_->getRouteDisplayTag(route);
        param.supported = parameterSystem_->isParameterSupported(currentEngine, paramId);
        param.selected = (static_cast<int>(i) == selectedParameterIndex_);

        parameters.push_back(param);
    }

    return parameters;
}

SystemStatus TerminalUISystem::buildSystemStatus() {
    SystemStatus status;

    status.cpuUsage = audioEngine_->getCPUUsage();
    status.memoryMB = audioEngine_->getMemoryUsageKB() / 1024.0f;
    status.bpm = 120.0f; // Would need to get from sequencer
    status.playing = stateManager_->isPlaying();
    status.currentStep = stateManager_->getCurrentStep();
    status.currentEngine = stateManager_->getCurrentEngine();
    status.currentBank = stateManager_->getCurrentPatternBank();
    status.currentPattern = stateManager_->getCurrentPatternSlot();
    status.buildVersion = BUILD_VERSION;

    return status;
}

std::vector<EngineStatus> TerminalUISystem::buildEngineStatusList() {
    std::vector<EngineStatus> engines;

    for (int i = 0; i < MAX_ENGINES; ++i) {
        EngineStatus engine;
        engine.engineId = i;

        auto typeResult = audioEngine_->getInstrumentEngineType(i);
        if (typeResult.isSuccess()) {
            auto nameResult = audioEngine_->getEngineTypeName(typeResult.value());
            engine.name = nameResult.isSuccess() ? nameResult.value() : "Unknown";
        } else {
            engine.name = "Not Set";
        }

        engine.voiceCount = audioEngine_->getEngineVoiceCount(i);
        engine.cpuUsage = 0.0f; // Would need engine-specific CPU usage
        engine.active = (i == stateManager_->getCurrentEngine());

        engines.push_back(engine);
    }

    return engines;
}

void TerminalUISystem::clearScreen() {
    std::cout << "\x1b[2J";
}

void TerminalUISystem::homeCaretAndClearScreen() {
    std::cout << "\x1b[2J\x1b[H";
}

std::string TerminalUISystem::getColorCode(Color color) {
    return "\x1b[" + std::to_string(static_cast<int>(color)) + "m";
}

void TerminalUISystem::flushOutput() {
    std::cout.flush();
}

} // namespace UI
} // namespace GridSequencer