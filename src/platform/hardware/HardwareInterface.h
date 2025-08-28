#pragma once
#include "../../core/Types.h"
#include <array>
#include <string>
#include <functional>

/**
 * Abstract hardware interface for ether synthesizer
 * Provides platform-agnostic access to all hardware components
 */
class HardwareInterface {
public:
    virtual ~HardwareInterface() = default;
    
    // Audio I/O
    virtual bool initializeAudio() = 0;
    virtual void setAudioCallback(std::function<void(EtherAudioBuffer&)> callback) = 0;
    virtual float getSampleRate() const = 0;
    virtual size_t getBufferSize() const = 0;
    
    // Key interface (26 keys: 16 white + 10 black)
    virtual KeyState getKeyState(uint8_t keyIndex) const = 0;
    virtual void setKeyLED(uint8_t keyIndex, uint32_t color) = 0;
    virtual void setKeyLEDBrightness(uint8_t keyIndex, float brightness) = 0;
    
    // Assignable encoders (4 total)
    virtual EncoderState getEncoderState(uint8_t encoderIndex) const = 0;
    virtual void setEncoderLED(uint8_t encoderIndex, uint32_t color) = 0;
    virtual void setEncoderOLED(uint8_t encoderIndex, const std::string& text) = 0;
    
    // Smart knob (BLDC motor with haptic feedback)
    virtual float getSmartKnobValue() const = 0;
    virtual void setSmartKnobHaptic(float intensity, uint32_t duration_ms) = 0;
    virtual void setSmartKnobDetents(bool enabled, float detentStrength) = 0;
    virtual void setSmartKnobSpring(bool enabled, float springStrength, float centerPosition) = 0;
    
    // Main display (960×320 touch screen)
    virtual void updateDisplay() = 0;
    virtual std::array<TouchPoint, 10> getTouchPoints() const = 0;  // Multi-touch support
    virtual void setDisplayBrightness(float brightness) = 0;
    
    // Master volume (analog control)
    virtual float getMasterVolume() const = 0;
    
    // Transport controls
    virtual bool getPlayButton() const = 0;
    virtual bool getStopButton() const = 0;
    virtual bool getRecordButton() const = 0;
    
    // Power management
    virtual float getBatteryLevel() const = 0;
    virtual bool isCharging() const = 0;
    virtual void setPowerMode(bool lowPower) = 0;
    
    // MIDI I/O
    virtual void sendMIDI(const uint8_t* data, size_t length) = 0;
    virtual void setMIDICallback(std::function<void(const uint8_t*, size_t)> callback) = 0;
    
    // File system
    virtual bool saveFile(const std::string& path, const uint8_t* data, size_t size) = 0;
    virtual bool loadFile(const std::string& path, uint8_t* buffer, size_t maxSize, size_t& actualSize) = 0;
    virtual std::vector<std::string> listFiles(const std::string& directory) = 0;
    
    // System info
    virtual std::string getDeviceID() const = 0;
    virtual std::string getFirmwareVersion() const = 0;
    virtual float getCPUUsage() const = 0;
    virtual size_t getFreeMemory() const = 0;
    
protected:
    // Common state that implementations can use
    std::array<KeyState, 26> keyStates_{};
    std::array<EncoderState, 4> encoderStates_{};
    std::array<TouchPoint, 10> touchPoints_{};
    float smartKnobValue_ = 0.5f;
    float masterVolume_ = 0.8f;
    bool playButton_ = false;
    bool stopButton_ = false;
    bool recordButton_ = false;
};

/**
 * Hardware capability flags for different implementations
 */
struct HardwareCapabilities {
    bool hasPolyAftertouch = false;
    bool hasHapticFeedback = false;
    bool hasMotorizedKnob = false;
    bool hasRGBLEDs = false;
    bool hasOLEDDisplays = false;
    bool hasBatteryMonitoring = false;
    bool hasMIDI = false;
    bool hasFileSystem = false;
    uint8_t maxPolyphony = 16;
    uint8_t numEncoders = 4;
    uint8_t numKeys = 26;
};

/**
 * Factory function to create platform-specific hardware interface
 */
std::unique_ptr<HardwareInterface> createHardwareInterface();

/**
 * Get capabilities of current hardware implementation
 */
HardwareCapabilities getHardwareCapabilities();