#pragma once
#include "../core/Types.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>

#ifdef PLATFORM_MAC
#include <CoreMIDI/CoreMIDI.h>
#endif

/**
 * MIDI Input/Output Manager
 * Handles MIDI device detection, input processing, and output generation
 */
class MIDIManager {
public:
    MIDIManager();
    ~MIDIManager();
    
    // Initialization
    bool initialize();
    void shutdown();
    
    // Device management
    struct MIDIDevice {
        std::string name;
        uint32_t id;
        bool isInput;
        bool isOutput;
        bool isConnected;
        std::string manufacturer;
        std::string model;
    };
    
    std::vector<MIDIDevice> getAvailableDevices() const;
    bool connectInputDevice(uint32_t deviceId);
    bool connectOutputDevice(uint32_t deviceId);
    void disconnectInputDevice(uint32_t deviceId);
    void disconnectOutputDevice(uint32_t deviceId);
    void disconnectAllDevices();
    
    // MIDI message types
    enum class MessageType : uint8_t {
        NOTE_OFF = 0x80,
        NOTE_ON = 0x90,
        POLY_AFTERTOUCH = 0xA0,
        CONTROL_CHANGE = 0xB0,
        PROGRAM_CHANGE = 0xC0,
        CHANNEL_AFTERTOUCH = 0xD0,
        PITCH_BEND = 0xE0,
        SYSTEM_EXCLUSIVE = 0xF0,
        TIME_CODE = 0xF1,
        SONG_POSITION = 0xF2,
        SONG_SELECT = 0xF3,
        TUNE_REQUEST = 0xF6,
        CLOCK = 0xF8,
        START = 0xFA,
        CONTINUE = 0xFB,
        STOP = 0xFC,
        ACTIVE_SENSING = 0xFE,
        RESET = 0xFF
    };
    
    struct MIDIMessage {
        MessageType type;
        uint8_t channel;        // 0-15
        uint8_t data1;          // Note number, CC number, etc.
        uint8_t data2;          // Velocity, CC value, etc.
        uint32_t timestamp;     // In samples
        std::vector<uint8_t> sysex; // For SysEx messages
        
        bool isNoteOn() const { return type == MessageType::NOTE_ON && data2 > 0; }
        bool isNoteOff() const { return type == MessageType::NOTE_OFF || (type == MessageType::NOTE_ON && data2 == 0); }
        bool isControlChange() const { return type == MessageType::CONTROL_CHANGE; }
        bool isPitchBend() const { return type == MessageType::PITCH_BEND; }
        bool isAftertouch() const { return type == MessageType::POLY_AFTERTOUCH || type == MessageType::CHANNEL_AFTERTOUCH; }
    };
    
    // Input callbacks
    std::function<void(const MIDIMessage&)> onMIDIReceived;
    std::function<void(uint8_t note, float velocity, uint8_t channel)> onNoteOn;
    std::function<void(uint8_t note, uint8_t channel)> onNoteOff;
    std::function<void(uint8_t controller, float value, uint8_t channel)> onControlChange;
    std::function<void(float pitchBend, uint8_t channel)> onPitchBend;
    std::function<void(float aftertouch, uint8_t channel)> onAftertouch;
    std::function<void(uint8_t note, float aftertouch, uint8_t channel)> onPolyAftertouch;
    std::function<void(uint8_t program, uint8_t channel)> onProgramChange;
    
    // Clock and timing
    std::function<void()> onMIDIClock;
    std::function<void()> onMIDIStart;
    std::function<void()> onMIDIStop;
    std::function<void()> onMIDIContinue;
    
    // Output methods
    void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0);
    void sendNoteOff(uint8_t note, uint8_t channel = 0);
    void sendControlChange(uint8_t controller, uint8_t value, uint8_t channel = 0);
    void sendPitchBend(uint16_t value, uint8_t channel = 0);
    void sendAftertouch(uint8_t value, uint8_t channel = 0);
    void sendPolyAftertouch(uint8_t note, uint8_t value, uint8_t channel = 0);
    void sendProgramChange(uint8_t program, uint8_t channel = 0);
    void sendSysEx(const std::vector<uint8_t>& data);
    
    // Clock output
    void sendClock();
    void sendStart();
    void sendStop();
    void sendContinue();
    
    // MIDI learn functionality
    struct MIDILearnSlot {
        uint8_t controller = 0;
        uint8_t channel = 0;
        ParameterID parameter = ParameterID::VOLUME;
        float minValue = 0.0f;
        float maxValue = 1.0f;
        bool learned = false;
        std::string description;
    };
    
    void startMIDILearn(ParameterID parameter);
    void stopMIDILearn();
    bool isMIDILearning() const { return midiLearning_; }
    void clearMIDIMapping(ParameterID parameter);
    void clearAllMIDIMappings();
    std::vector<MIDILearnSlot> getMIDIMappings() const;
    
    // MIDI settings
    void setInputChannel(uint8_t channel);      // 0-15, or 16 for omni
    void setOutputChannel(uint8_t channel);     // 0-15
    uint8_t getInputChannel() const { return inputChannel_; }
    uint8_t getOutputChannel() const { return outputChannel_; }
    
    void setVelocityCurve(float curve);         // -1.0 to 1.0 (linear = 0)
    float getVelocityCurve() const { return velocityCurve_; }
    
    void setTranspose(int semitones);           // -24 to +24
    int getTranspose() const { return transpose_; }
    
    // Status and monitoring
    bool isInitialized() const { return initialized_; }
    size_t getConnectedInputCount() const;
    size_t getConnectedOutputCount() const;
    uint32_t getInputMessageCount() const { return inputMessageCount_; }
    uint32_t getOutputMessageCount() const { return outputMessageCount_; }
    void resetMessageCounts();
    
    // Error handling
    enum class MIDIError {
        NONE = 0,
        INITIALIZATION_FAILED,
        DEVICE_NOT_FOUND,
        CONNECTION_FAILED,
        SEND_FAILED,
        RECEIVE_FAILED,
        INVALID_MESSAGE,
        DEVICE_DISCONNECTED
    };
    
    MIDIError getLastError() const { return lastError_; }
    std::string getErrorMessage() const;
    
    // Advanced features
    void setLatencyCompensation(float milliseconds);
    float getLatencyCompensation() const { return latencyCompensation_; }
    
    void enableMIDIThru(bool enable);
    bool isMIDIThruEnabled() const { return midiThru_; }
    
    // Preset integration
    void saveMIDISettings(std::vector<uint8_t>& data) const;
    bool loadMIDISettings(const std::vector<uint8_t>& data);

private:
    bool initialized_ = false;
    mutable MIDIError lastError_ = MIDIError::NONE;
    
    // Platform-specific MIDI handles
#ifdef PLATFORM_MAC
    MIDIClientRef midiClient_ = 0;
    MIDIPortRef inputPort_ = 0;
    MIDIPortRef outputPort_ = 0;
    std::vector<MIDIEndpointRef> connectedInputs_;
    std::vector<MIDIEndpointRef> connectedOutputs_;
#endif
    
    // Device lists
    std::vector<MIDIDevice> availableDevices_;
    
    // MIDI settings
    uint8_t inputChannel_ = 16;      // Omni by default
    uint8_t outputChannel_ = 0;      // Channel 1
    float velocityCurve_ = 0.0f;     // Linear
    int transpose_ = 0;              // No transpose
    float latencyCompensation_ = 0.0f;
    bool midiThru_ = false;
    
    // MIDI learn
    bool midiLearning_ = false;
    ParameterID learnParameter_ = ParameterID::VOLUME;
    std::vector<MIDILearnSlot> midiMappings_;
    
    // Statistics
    uint32_t inputMessageCount_ = 0;
    uint32_t outputMessageCount_ = 0;
    
    // Message processing
    void processMIDIMessage(const MIDIMessage& message);
    void applyMIDIMapping(const MIDIMessage& message);
    float applyVelocityCurve(uint8_t velocity) const;
    uint8_t applyTranspose(uint8_t note) const;
    
    // Platform-specific implementations
    bool initializePlatform();
    void shutdownPlatform();
    void scanDevices();
    bool connectInputDevicePlatform(uint32_t deviceId);
    bool connectOutputDevicePlatform(uint32_t deviceId);
    void sendMIDIMessagePlatform(const std::vector<uint8_t>& message);
    
#ifdef PLATFORM_MAC
    static void midiInputCallback(const MIDIPacketList* packetList, void* readProcRefCon, void* srcConnRefCon);
    void processMIDIPacket(const MIDIPacket* packet);
    MIDIMessage parseMIDIData(const uint8_t* data, size_t length, uint32_t timestamp);
#endif
    
    // Helper methods
    std::string getMIDIMessageTypeName(MessageType type) const;
    bool isValidChannel(uint8_t channel) const;
    bool isValidNote(uint8_t note) const;
    bool isValidController(uint8_t controller) const;
    
    // Standard MIDI controller numbers
public:
    static constexpr uint8_t CC_BANK_SELECT = 0;
    static constexpr uint8_t CC_MODULATION = 1;
    static constexpr uint8_t CC_BREATH = 2;
    static constexpr uint8_t CC_FOOT = 4;
    static constexpr uint8_t CC_PORTAMENTO_TIME = 5;
    static constexpr uint8_t CC_DATA_ENTRY = 6;
    static constexpr uint8_t CC_VOLUME = 7;
    static constexpr uint8_t CC_BALANCE = 8;
    static constexpr uint8_t CC_PAN = 10;
    static constexpr uint8_t CC_EXPRESSION = 11;
    static constexpr uint8_t CC_EFFECT_1 = 12;
    static constexpr uint8_t CC_EFFECT_2 = 13;
    static constexpr uint8_t CC_GENERAL_1 = 16;
    static constexpr uint8_t CC_GENERAL_2 = 17;
    static constexpr uint8_t CC_GENERAL_3 = 18;
    static constexpr uint8_t CC_GENERAL_4 = 19;
    static constexpr uint8_t CC_SUSTAIN = 64;
    static constexpr uint8_t CC_PORTAMENTO = 65;
    static constexpr uint8_t CC_SOSTENUTO = 66;
    static constexpr uint8_t CC_SOFT_PEDAL = 67;
    static constexpr uint8_t CC_LEGATO = 68;
    static constexpr uint8_t CC_HOLD_2 = 69;
    static constexpr uint8_t CC_SOUND_1 = 70;  // Sound Variation
    static constexpr uint8_t CC_SOUND_2 = 71;  // Timbre/Harmonics
    static constexpr uint8_t CC_SOUND_3 = 72;  // Release Time
    static constexpr uint8_t CC_SOUND_4 = 73;  // Attack Time
    static constexpr uint8_t CC_SOUND_5 = 74;  // Brightness
    static constexpr uint8_t CC_SOUND_6 = 75;  // Decay Time
    static constexpr uint8_t CC_SOUND_7 = 76;  // Vibrato Rate
    static constexpr uint8_t CC_SOUND_8 = 77;  // Vibrato Depth
    static constexpr uint8_t CC_SOUND_9 = 78;  // Vibrato Delay
    static constexpr uint8_t CC_SOUND_10 = 79; // Undefined
    static constexpr uint8_t CC_GENERAL_5 = 80;
    static constexpr uint8_t CC_GENERAL_6 = 81;
    static constexpr uint8_t CC_GENERAL_7 = 82;
    static constexpr uint8_t CC_GENERAL_8 = 83;
    static constexpr uint8_t CC_PORTAMENTO_CTRL = 84;
    static constexpr uint8_t CC_REVERB = 91;
    static constexpr uint8_t CC_TREMOLO = 92;
    static constexpr uint8_t CC_CHORUS = 93;
    static constexpr uint8_t CC_DETUNE = 94;
    static constexpr uint8_t CC_PHASER = 95;
    static constexpr uint8_t CC_DATA_INCREMENT = 96;
    static constexpr uint8_t CC_DATA_DECREMENT = 97;
    static constexpr uint8_t CC_NRPN_LSB = 98;
    static constexpr uint8_t CC_NRPN_MSB = 99;
    static constexpr uint8_t CC_RPN_LSB = 100;
    static constexpr uint8_t CC_RPN_MSB = 101;
    static constexpr uint8_t CC_ALL_SOUND_OFF = 120;
    static constexpr uint8_t CC_RESET_CONTROLLERS = 121;
    static constexpr uint8_t CC_LOCAL_CONTROL = 122;
    static constexpr uint8_t CC_ALL_NOTES_OFF = 123;
    static constexpr uint8_t CC_OMNI_OFF = 124;
    static constexpr uint8_t CC_OMNI_ON = 125;
    static constexpr uint8_t CC_MONO_ON = 126;
    static constexpr uint8_t CC_POLY_ON = 127;
};

// MIDI utility functions
namespace MIDIUtils {
    // Note number to frequency conversion
    float noteToFrequency(uint8_t note);
    uint8_t frequencyToNote(float frequency);
    
    // Note name conversion
    std::string noteToName(uint8_t note);
    uint8_t nameToNote(const std::string& name);
    
    // MIDI value conversions
    float midiToFloat(uint8_t midiValue);           // 0-127 to 0.0-1.0
    uint8_t floatToMIDI(float value);               // 0.0-1.0 to 0-127
    float pitchBendToFloat(uint16_t pitchBend);     // 0-16383 to -1.0-1.0
    uint16_t floatToPitchBend(float value);         // -1.0-1.0 to 0-16383
    
    // Timing utilities
    uint32_t beatsToTicks(float beats, uint16_t ppq = 480);
    float ticksToBeats(uint32_t ticks, uint16_t ppq = 480);
    uint32_t msToTicks(float milliseconds, float bpm, uint16_t ppq = 480);
    float ticksToMs(uint32_t ticks, float bpm, uint16_t ppq = 480);
}