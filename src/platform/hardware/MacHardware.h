#pragma once
#include "HardwareInterface.h"
#include <CoreAudio/CoreAudio.h>
#include <AudioUnit/AudioUnit.h>
#include <array>
#include <atomic>
#include <map>

/**
 * Mac implementation of hardware interface for prototyping
 * Uses Core Audio for audio I/O and simulates hardware controls
 */
class MacHardware : public HardwareInterface {
public:
    MacHardware();
    ~MacHardware() override;
    
    // Audio I/O
    bool initializeAudio() override;
    void setAudioCallback(std::function<void(EtherAudioBuffer&)> callback) override;
    float getSampleRate() const override { return SAMPLE_RATE; }
    size_t getBufferSize() const override { return BUFFER_SIZE; }
    
    // Key interface (simulated via MIDI controller or computer keyboard)
    KeyState getKeyState(uint8_t keyIndex) const override;
    void setKeyLED(uint8_t keyIndex, uint32_t color) override;
    void setKeyLEDBrightness(uint8_t keyIndex, float brightness) override;
    
    // Assignable encoders (simulated via MIDI CC or keyboard)
    EncoderState getEncoderState(uint8_t encoderIndex) const override;
    void setEncoderLED(uint8_t encoderIndex, uint32_t color) override;
    void setEncoderOLED(uint8_t encoderIndex, const std::string& text) override;
    
    // Smart knob (simulated via mouse wheel or MIDI)
    float getSmartKnobValue() const override { return smartKnobValue_; }
    void setSmartKnobHaptic(float intensity, uint32_t duration_ms) override;
    void setSmartKnobDetents(bool enabled, float detentStrength) override;
    void setSmartKnobSpring(bool enabled, float springStrength, float centerPosition) override;
    
    // Main display (simulated via window)
    void updateDisplay() override;
    std::array<TouchPoint, 10> getTouchPoints() const override;
    void setDisplayBrightness(float brightness) override;
    
    // Master volume
    float getMasterVolume() const override { return masterVolume_; }
    
    // Transport controls (simulated via keyboard)
    bool getPlayButton() const override { return playButton_; }
    bool getStopButton() const override { return stopButton_; }
    bool getRecordButton() const override { return recordButton_; }
    
    // Power management (simulated)
    float getBatteryLevel() const override { return 0.85f; }  // Simulated battery level
    bool isCharging() const override { return true; }        // Always "charging" on Mac
    void setPowerMode(bool lowPower) override {}            // No-op on Mac
    
    // MIDI I/O
    void sendMIDI(const uint8_t* data, size_t length) override;
    void setMIDICallback(std::function<void(const uint8_t*, size_t)> callback) override;
    
    // File system
    bool saveFile(const std::string& path, const uint8_t* data, size_t size) override;
    bool loadFile(const std::string& path, uint8_t* buffer, size_t maxSize, size_t& actualSize) override;
    std::vector<std::string> listFiles(const std::string& directory) override;
    
    // System info
    std::string getDeviceID() const override;
    std::string getFirmwareVersion() const override;
    float getCPUUsage() const override;
    size_t getFreeMemory() const override;
    
    // Mac-specific: Input simulation methods
    void simulateKeyPress(uint8_t keyIndex, float velocity, float aftertouch = 0.0f);
    void simulateKeyRelease(uint8_t keyIndex);
    void simulateEncoderChange(uint8_t encoderIndex, float deltaValue);
    void simulateSmartKnobChange(float deltaValue);
    void simulateTouch(float x, float y, bool active, uint32_t touchID = 0);
    void simulateTransportButton(bool play, bool stop, bool record);
    
    // Mac-specific: MIDI controller integration
    void setupMIDIController(const std::string& controllerName);
    void handleMIDIInput(const uint8_t* data, size_t length);
    
private:
    // Core Audio components
    AudioUnit audioUnit_;
    bool audioInitialized_ = false;
    std::function<void(EtherAudioBuffer&)> audioCallback_;
    
    // State
    std::atomic<float> smartKnobValue_{0.5f};
    std::atomic<float> masterVolume_{0.8f};
    std::atomic<bool> playButton_{false};
    std::atomic<bool> stopButton_{false};
    std::atomic<bool> recordButton_{false};
    
    // Simulated hardware state
    std::array<std::atomic<bool>, 26> keyPressed_{};
    std::array<std::atomic<float>, 26> keyVelocity_{};
    std::array<std::atomic<float>, 26> keyAftertouch_{};
    std::array<std::atomic<uint32_t>, 26> keyPressTime_{};
    
    std::array<std::atomic<float>, 4> encoderValues_{};
    std::array<std::atomic<bool>, 4> encoderChanged_{};
    std::array<uint32_t, 26> keyLEDColors_{};
    std::array<uint32_t, 4> encoderLEDColors_{};
    std::array<std::string, 4> encoderOLEDTexts_{};
    
    std::array<TouchPoint, 10> touchPoints_{};
    std::atomic<float> displayBrightness_{1.0f};
    
    // MIDI support
    std::function<void(const uint8_t*, size_t)> midiCallback_;
    
    // Core Audio callback
    static OSStatus audioRenderCallback(
        void* inRefCon,
        AudioUnitRenderActionFlags* ioActionFlags,
        const AudioTimeStamp* inTimeStamp,
        UInt32 inBusNumber,
        UInt32 inNumberFrames,
        AudioBufferList* ioData
    );
    
    // MIDI callback
    static void midiInputCallback(
        const MIDIPacketList* packetList,
        void* readProcRefCon,
        void* srcConnRefCon
    );
    
    // Key mapping for computer keyboard simulation
    std::map<char, uint8_t> keyboardMapping_;
    void initializeKeyboardMapping();
    
    // Performance monitoring
    mutable std::atomic<float> cpuUsage_{0.0f};
    void updateCPUUsage();
    
    // File system helpers
    std::string getDocumentsPath() const;
    std::string getPresetsPath() const;
};