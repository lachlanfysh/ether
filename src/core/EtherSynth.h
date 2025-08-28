#pragma once
#include "Types.h"
#include "../platform/hardware/HardwareInterface.h"
#include <memory>
#include <atomic>
#include <thread>

class AudioEngine;

/**
 * Main application class for ether synthesizer
 * Coordinates all subsystems and manages the main application loop
 */
class EtherSynth {
public:
    EtherSynth();
    ~EtherSynth();
    
    // Application lifecycle
    bool initialize();
    void run();
    void shutdown();
    
    // System state
    bool isRunning() const { return running_; }
    void requestShutdown() { running_ = false; }
    
    // Mode management
    void setMode(Mode mode);
    Mode getCurrentMode() const { return currentMode_; }
    void nextScreen();
    void previousScreen();
    int getCurrentScreen() const { return currentScreen_; }
    
    // Hardware access
    HardwareInterface* getHardware() { return hardware_.get(); }
    AudioEngine* getAudioEngine() { return audioEngine_.get(); }
    
    // Performance monitoring
    float getSystemCPUUsage() const;
    size_t getFreeMemory() const;
    float getBatteryLevel() const;
    
    // Error handling (legacy support)
    enum class ErrorCode {
        NONE = 0,
        AUDIO_INIT_FAILED,
        HARDWARE_INIT_FAILED,
        UI_INIT_FAILED,
        OUT_OF_MEMORY,
        FILE_SYSTEM_ERROR,
        UNKNOWN_ERROR
    };
    
    ErrorCode getLastError() const { return lastError_; }
    const char* getErrorMessage() const;
    
    // New comprehensive error handling
    void initializeErrorReporting();
    std::string generateSystemReport() const;
    bool isSystemHealthy() const;
    
private:
    // Core components
    std::unique_ptr<HardwareInterface> hardware_;
    std::unique_ptr<AudioEngine> audioEngine_;
    
    // Application state
    std::atomic<bool> running_{false};
    std::atomic<Mode> currentMode_{Mode::INSTRUMENT};
    std::atomic<int> currentScreen_{0};
    std::atomic<ErrorCode> lastError_{ErrorCode::NONE};
    
    // Threading
    std::thread uiThread_;
    std::thread controllerThread_;
    
    // Main loops (run in separate threads)
    void uiLoop();          // UI updates and touch handling
    void controllerLoop();  // Hardware input processing
    
    // Initialization helpers
    bool initializeHardware();
    bool initializeAudio();
    bool initializeUI();
    bool initializeControllers();
    
    // Error handling
    void setError(ErrorCode error);
    void handleError(ErrorCode error);
    
    // Input handling
    void handleKeyPress(uint8_t keyIndex, float velocity, float aftertouch);
    void handleKeyRelease(uint8_t keyIndex);
    void handleEncoderChange(uint8_t encoderIndex, float value);
    void handleSmartKnobChange(float value);
    void handleTouch(const TouchPoint& touch);
    void handleTransportButton(bool play, bool stop, bool record);
    
    // System monitoring
    void updatePerformanceMetrics();
    
    // Configuration
    static constexpr float UI_UPDATE_RATE = 60.0f;          // 60 FPS UI updates
    static constexpr float CONTROLLER_UPDATE_RATE = 1000.0f; // 1kHz controller polling
    static constexpr uint32_t UI_UPDATE_INTERVAL_MS = static_cast<uint32_t>(1000.0f / UI_UPDATE_RATE);
    static constexpr uint32_t CONTROLLER_UPDATE_INTERVAL_MS = static_cast<uint32_t>(1000.0f / CONTROLLER_UPDATE_RATE);
};