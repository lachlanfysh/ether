#include "EtherSynth.h"
#include "../audio/AudioEngine.h"
#include "../platform/hardware/HardwareInterface.h"
#include <iostream>
#include <thread>
#include <chrono>

EtherSynth::EtherSynth() {
    std::cout << "EtherSynth constructor" << std::endl;
}

EtherSynth::~EtherSynth() {
    shutdown();
    std::cout << "EtherSynth destructor" << std::endl;
}

bool EtherSynth::initialize() {
    std::cout << "Initializing EtherSynth..." << std::endl;
    
    try {
        // Initialize hardware first
        if (!initializeHardware()) {
            setError(ErrorCode::HARDWARE_INIT_FAILED);
            return false;
        }
        
        // Initialize audio engine
        if (!initializeAudio()) {
            setError(ErrorCode::AUDIO_INIT_FAILED);
            return false;
        }
        
        // Initialize controllers
        if (!initializeControllers()) {
            setError(ErrorCode::HARDWARE_INIT_FAILED);
            return false;
        }
        
        std::cout << "EtherSynth initialized successfully!" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Initialization failed: " << e.what() << std::endl;
        setError(ErrorCode::UNKNOWN_ERROR);
        return false;
    }
}

void EtherSynth::run() {
    if (!running_.load()) {
        std::cout << "Starting EtherSynth main loop..." << std::endl;
        running_.store(true);
        
        // Start background threads
        uiThread_ = std::thread(&EtherSynth::uiLoop, this);
        controllerThread_ = std::thread(&EtherSynth::controllerLoop, this);
        
        // Main thread can do other work or just wait
        while (running_.load()) {
            updatePerformanceMetrics();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Update every second
        }
        
        // Wait for threads to finish
        if (uiThread_.joinable()) {
            uiThread_.join();
        }
        if (controllerThread_.joinable()) {
            controllerThread_.join();
        }
        
        std::cout << "EtherSynth main loop ended" << std::endl;
    }
}

void EtherSynth::shutdown() {
    if (running_.load()) {
        std::cout << "Shutting down EtherSynth..." << std::endl;
        running_.store(false);
        
        // Wait for threads to finish
        if (uiThread_.joinable()) {
            uiThread_.join();
        }
        if (controllerThread_.joinable()) {
            controllerThread_.join();
        }
        
        // Shutdown components
        if (audioEngine_) {
            audioEngine_->shutdown();
        }
        
        audioEngine_.reset();
        hardware_.reset();
        
        std::cout << "EtherSynth shutdown complete" << std::endl;
    }
}

void EtherSynth::setMode(Mode mode) {
    Mode oldMode = currentMode_.exchange(mode);
    currentScreen_.store(0); // Reset to first screen of new mode
    
    std::cout << "Mode changed from " << static_cast<int>(oldMode) 
              << " to " << static_cast<int>(mode) << std::endl;
}

void EtherSynth::nextScreen() {
    int current = currentScreen_.load();
    // Mode-specific screen counts (simplified)
    int maxScreens = 4; // Default
    currentScreen_.store((current + 1) % maxScreens);
}

void EtherSynth::previousScreen() {
    int current = currentScreen_.load();
    int maxScreens = 4; // Default
    currentScreen_.store((current + maxScreens - 1) % maxScreens);
}

float EtherSynth::getSystemCPUUsage() const {
    if (hardware_) {
        return hardware_->getCPUUsage();
    }
    return 0.0f;
}

size_t EtherSynth::getFreeMemory() const {
    if (hardware_) {
        return hardware_->getFreeMemory();
    }
    return 0;
}

float EtherSynth::getBatteryLevel() const {
    if (hardware_) {
        return hardware_->getBatteryLevel();
    }
    return 1.0f;
}

const char* EtherSynth::getErrorMessage() const {
    switch (lastError_.load()) {
        case ErrorCode::NONE: return "No error";
        case ErrorCode::AUDIO_INIT_FAILED: return "Audio initialization failed";
        case ErrorCode::HARDWARE_INIT_FAILED: return "Hardware initialization failed";
        case ErrorCode::UI_INIT_FAILED: return "UI initialization failed";
        case ErrorCode::OUT_OF_MEMORY: return "Out of memory";
        case ErrorCode::FILE_SYSTEM_ERROR: return "File system error";
        case ErrorCode::UNKNOWN_ERROR: return "Unknown error";
        default: return "Undefined error";
    }
}

bool EtherSynth::initializeHardware() {
    std::cout << "Initializing hardware interface..." << std::endl;
    
    hardware_ = createHardwareInterface();
    if (!hardware_) {
        std::cerr << "Failed to create hardware interface" << std::endl;
        return false;
    }
    
    if (!hardware_->initializeAudio()) {
        std::cerr << "Failed to initialize hardware audio" << std::endl;
        return false;
    }
    
    std::cout << "Hardware interface initialized" << std::endl;
    return true;
}

bool EtherSynth::initializeAudio() {
    std::cout << "Initializing audio engine..." << std::endl;
    
    audioEngine_ = std::make_unique<AudioEngine>();
    if (!audioEngine_->initialize(hardware_.get())) {
        std::cerr << "Failed to initialize audio engine" << std::endl;
        return false;
    }
    
    std::cout << "Audio engine initialized" << std::endl;
    return true;
}

bool EtherSynth::initializeUI() {
    std::cout << "Initializing UI..." << std::endl;
    // UI initialization will be implemented later
    return true;
}

bool EtherSynth::initializeControllers() {
    std::cout << "Initializing controllers..." << std::endl;
    // Controller initialization will be implemented later
    return true;
}

void EtherSynth::setError(ErrorCode error) {
    lastError_.store(error);
    std::cerr << "Error set: " << getErrorMessage() << std::endl;
}

void EtherSynth::handleError(ErrorCode error) {
    setError(error);
    // Could implement error recovery logic here
}

void EtherSynth::uiLoop() {
    std::cout << "UI thread started" << std::endl;
    
    while (running_.load()) {
        // Update UI at 60 FPS
        if (hardware_) {
            hardware_->updateDisplay();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(UI_UPDATE_INTERVAL_MS));
    }
    
    std::cout << "UI thread ended" << std::endl;
}

void EtherSynth::controllerLoop() {
    std::cout << "Controller thread started" << std::endl;
    
    while (running_.load()) {
        if (hardware_) {
            // Poll hardware inputs
            
            // Check keys
            for (uint8_t i = 0; i < 26; i++) {
                KeyState key = hardware_->getKeyState(i);
                static std::array<bool, 26> lastPressed = {};
                
                if (key.pressed && !lastPressed[i]) {
                    handleKeyPress(i, key.velocity, key.aftertouch);
                } else if (!key.pressed && lastPressed[i]) {
                    handleKeyRelease(i);
                }
                lastPressed[i] = key.pressed;
            }
            
            // Check encoders
            for (uint8_t i = 0; i < 4; i++) {
                EncoderState encoder = hardware_->getEncoderState(i);
                if (encoder.changed) {
                    handleEncoderChange(i, encoder.value);
                }
            }
            
            // Check touch points
            auto touchPoints = hardware_->getTouchPoints();
            for (const auto& touch : touchPoints) {
                if (touch.active) {
                    handleTouch(touch);
                }
            }
            
            // Check transport buttons
            static bool lastPlay = false, lastStop = false, lastRecord = false;
            bool play = hardware_->getPlayButton();
            bool stop = hardware_->getStopButton();
            bool record = hardware_->getRecordButton();
            
            if (play != lastPlay || stop != lastStop || record != lastRecord) {
                handleTransportButton(play, stop, record);
                lastPlay = play;
                lastStop = stop;
                lastRecord = record;
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(CONTROLLER_UPDATE_INTERVAL_MS));
    }
    
    std::cout << "Controller thread ended" << std::endl;
}

void EtherSynth::handleKeyPress(uint8_t keyIndex, float velocity, float aftertouch) {
    if (audioEngine_) {
        audioEngine_->noteOn(keyIndex, velocity, aftertouch);
    }
}

void EtherSynth::handleKeyRelease(uint8_t keyIndex) {
    if (audioEngine_) {
        audioEngine_->noteOff(keyIndex);
    }
}

void EtherSynth::handleEncoderChange(uint8_t encoderIndex, float value) {
    std::cout << "Encoder " << static_cast<int>(encoderIndex) 
              << " changed to " << value << std::endl;
    // Parameter assignment logic will be implemented later
}

void EtherSynth::handleSmartKnobChange(float value) {
    std::cout << "Smart knob changed to " << value << std::endl;
    // Smart knob logic will be implemented later
}

void EtherSynth::handleTouch(const TouchPoint& touch) {
    std::cout << "Touch at (" << touch.x << ", " << touch.y << ")" << std::endl;
    // Touch handling logic will be implemented later
}

void EtherSynth::handleTransportButton(bool play, bool stop, bool record) {
    if (audioEngine_) {
        if (play && !audioEngine_->isPlaying()) {
            audioEngine_->play();
        } else if (stop && audioEngine_->isPlaying()) {
            audioEngine_->stop();
        }
        
        if (record) {
            audioEngine_->record(true);
        } else {
            audioEngine_->record(false);
        }
    }
}

void EtherSynth::updatePerformanceMetrics() {
    // Update performance metrics periodically
    if (audioEngine_) {
        float cpuUsage = audioEngine_->getCPUUsage();
        size_t activeVoices = audioEngine_->getActiveVoiceCount();
        
        // Only log if significant change
        static float lastCPU = 0.0f;
        static size_t lastVoices = 0;
        
        if (abs(cpuUsage - lastCPU) > 5.0f || activeVoices != lastVoices) {
            std::cout << "Performance: CPU " << cpuUsage << "%, Voices: " 
                      << activeVoices << std::endl;
            lastCPU = cpuUsage;
            lastVoices = activeVoices;
        }
    }
}