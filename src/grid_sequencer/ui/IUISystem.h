#pragma once
#include "../core/DIContainer.h"
#include <string>
#include <vector>

namespace GridSequencer {
namespace UI {

using Core::Result;

// UI display modes
enum class DisplayMode {
    MAIN_SEQUENCER,
    ENGINE_SELECT,
    PARAMETER_EDIT,
    PATTERN_BANK,
    SETTINGS,
    HELP
};

// UI color codes for terminal
enum class Color {
    RESET = 0,
    BLACK = 30,
    RED = 31,
    GREEN = 32,
    YELLOW = 33,
    BLUE = 34,
    MAGENTA = 35,
    CYAN = 36,
    WHITE = 37,
    BRIGHT_BLACK = 90,
    BRIGHT_RED = 91,
    BRIGHT_GREEN = 92,
    BRIGHT_YELLOW = 93,
    BRIGHT_BLUE = 94,
    BRIGHT_MAGENTA = 95,
    BRIGHT_CYAN = 96,
    BRIGHT_WHITE = 97
};

// Parameter display information
struct ParameterDisplay {
    std::string name;
    std::string value;
    std::string route; // [E], [FX], [—]
    bool selected;
    bool supported;
};

// Engine status information
struct EngineStatus {
    int engineId;
    std::string name;
    std::string category;
    float cpuUsage;
    int voiceCount;
    bool active;
};

// System status information
struct SystemStatus {
    float cpuUsage;
    float memoryMB;
    float bpm;
    bool playing;
    int currentStep;
    int currentEngine;
    int currentBank;
    int currentPattern;
    std::string buildVersion;
};

// UI system interface - handles terminal-based user interface
class IUISystem {
public:
    virtual ~IUISystem() = default;

    // System lifecycle
    virtual Result<bool> initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isInitialized() const = 0;

    // Display management
    virtual void render() = 0;
    virtual void clear() = 0;
    virtual void setDisplayMode(DisplayMode mode) = 0;
    virtual DisplayMode getCurrentDisplayMode() const = 0;

    // Content rendering
    virtual void renderMainSequencer() = 0;
    virtual void renderEngineSelect() = 0;
    virtual void renderParameterEdit() = 0;
    virtual void renderPatternBank() = 0;
    virtual void renderSystemStatus(const SystemStatus& status) = 0;

    // Parameter display
    virtual void renderParameterList(const std::vector<ParameterDisplay>& parameters) = 0;
    virtual void renderParameterValue(const std::string& name, const std::string& value, bool selected = false) = 0;

    // Engine display
    virtual void renderEngineStatus(const EngineStatus& status) = 0;
    virtual void renderEngineList(const std::vector<EngineStatus>& engines, int selectedEngine) = 0;

    // Text utilities
    virtual void print(const std::string& text, Color color = Color::WHITE) = 0;
    virtual void printLine(const std::string& text, Color color = Color::WHITE) = 0;
    virtual void printAt(int x, int y, const std::string& text, Color color = Color::WHITE) = 0;

    // Cursor and formatting
    virtual void setCursor(int x, int y) = 0;
    virtual void hideCursor() = 0;
    virtual void showCursor() = 0;
    virtual std::string colorize(const std::string& text, Color color) = 0;

    // Screen information
    virtual int getScreenWidth() const = 0;
    virtual int getScreenHeight() const = 0;
    virtual Result<bool> updateScreenSize() = 0;
};

} // namespace UI
} // namespace GridSequencer