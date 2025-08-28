#include "MacHardware.h"
#include <CoreMIDI/CoreMIDI.h>
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <mach/mach.h>

MacHardware::MacHardware() {
    // Initialize arrays
    for (auto& pressed : keyPressed_) pressed.store(false);
    for (auto& velocity : keyVelocity_) velocity.store(0.0f);
    for (auto& aftertouch : keyAftertouch_) aftertouch.store(0.0f);
    for (auto& pressTime : keyPressTime_) pressTime = 0;
    
    for (auto& value : encoderValues_) value.store(0.5f);
    for (auto& changed : encoderChanged_) changed.store(false);
    for (auto& color : keyLEDColors_) color = 0;
    for (auto& color : encoderLEDColors_) color = 0;
    
    // Initialize touch points
    for (auto& touch : touchPoints_) {
        touch.x = 0.0f;
        touch.y = 0.0f;
        touch.active = false;
        touch.id = 0;
    }
    
    initializeKeyboardMapping();
}

MacHardware::~MacHardware() {
    if (audioInitialized_) {
        AudioUnitUninitialize(audioUnit_);
        AudioComponentInstanceDispose(audioUnit_);
    }
}

bool MacHardware::initializeAudio() {
    std::cout << "Initializing Core Audio..." << std::endl;
    
    // Find default output unit
    AudioComponentDescription desc = {};
    desc.componentType = kAudioUnitType_Output;
    desc.componentSubType = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;
    
    AudioComponent component = AudioComponentFindNext(nullptr, &desc);
    if (!component) {
        std::cerr << "Failed to find default output audio unit" << std::endl;
        return false;
    }
    
    // Create audio unit instance
    OSStatus result = AudioComponentInstanceNew(component, &audioUnit_);
    if (result != noErr) {
        std::cerr << "Failed to create audio unit instance: " << result << std::endl;
        return false;
    }
    
    // Configure audio format
    AudioStreamBasicDescription format = {};
    format.mSampleRate = SAMPLE_RATE;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    format.mChannelsPerFrame = 2;  // Stereo
    format.mBitsPerChannel = 32;
    format.mBytesPerFrame = sizeof(float) * 2;
    format.mBytesPerPacket = sizeof(float) * 2;
    format.mFramesPerPacket = 1;
    
    result = AudioUnitSetProperty(audioUnit_, 
                                  kAudioUnitProperty_StreamFormat,
                                  kAudioUnitScope_Input, 
                                  0, 
                                  &format, 
                                  sizeof(format));
    if (result != noErr) {
        std::cerr << "Failed to set audio format: " << result << std::endl;
        return false;
    }
    
    // Set render callback
    AURenderCallbackStruct callback = {};
    callback.inputProc = audioRenderCallback;
    callback.inputProcRefCon = this;
    
    result = AudioUnitSetProperty(audioUnit_,
                                  kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Input,
                                  0,
                                  &callback,
                                  sizeof(callback));
    if (result != noErr) {
        std::cerr << "Failed to set render callback: " << result << std::endl;
        return false;
    }
    
    // Initialize and start audio unit
    result = AudioUnitInitialize(audioUnit_);
    if (result != noErr) {
        std::cerr << "Failed to initialize audio unit: " << result << std::endl;
        return false;
    }
    
    result = AudioOutputUnitStart(audioUnit_);
    if (result != noErr) {
        std::cerr << "Failed to start audio unit: " << result << std::endl;
        return false;
    }
    
    audioInitialized_ = true;
    std::cout << "Core Audio initialized successfully" << std::endl;
    std::cout << "Sample Rate: " << SAMPLE_RATE << " Hz" << std::endl;
    std::cout << "Buffer Size: " << BUFFER_SIZE << " samples" << std::endl;
    std::cout << "Latency: " << (BUFFER_SIZE / SAMPLE_RATE * 1000.0f) << " ms" << std::endl;
    
    return true;
}

void MacHardware::setAudioCallback(std::function<void(EtherAudioBuffer&)> callback) {
    audioCallback_ = callback;
}

OSStatus MacHardware::audioRenderCallback(
    void* inRefCon,
    AudioUnitRenderActionFlags* ioActionFlags,
    const AudioTimeStamp* inTimeStamp,
    UInt32 inBusNumber,
    UInt32 inNumberFrames,
    AudioBufferList* ioData) {
    
    MacHardware* hardware = static_cast<MacHardware*>(inRefCon);
    
    // Clear output buffers
    for (UInt32 i = 0; i < ioData->mNumberBuffers; i++) {
        memset(ioData->mBuffers[i].mData, 0, ioData->mBuffers[i].mDataByteSize);
    }
    
    if (hardware->audioCallback_ && inNumberFrames <= BUFFER_SIZE) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Create audio buffer from Core Audio format
        EtherAudioBuffer buffer;
        
        // Call the ether audio engine
        hardware->audioCallback_(buffer);
        
        // Convert from ether AudioFrame format to Core Audio format
        float* leftChannel = static_cast<float*>(ioData->mBuffers[0].mData);
        float* rightChannel = static_cast<float*>(ioData->mBuffers[1].mData);
        
        for (UInt32 i = 0; i < inNumberFrames && i < BUFFER_SIZE; i++) {
            leftChannel[i] = buffer[i].left;
            rightChannel[i] = buffer[i].right;
        }
        
        // Calculate CPU usage
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        float processingTime = duration.count() / 1000.0f; // Convert to milliseconds
        float maxTime = (inNumberFrames / SAMPLE_RATE) * 1000.0f; // Available time in ms
        hardware->cpuUsage_.store(std::min(100.0f, (processingTime / maxTime) * 100.0f));
    }
    
    return noErr;
}

KeyState MacHardware::getKeyState(uint8_t keyIndex) const {
    if (keyIndex >= 26) return {};
    
    KeyState state;
    state.pressed = keyPressed_[keyIndex].load();
    state.velocity = keyVelocity_[keyIndex].load();
    state.aftertouch = keyAftertouch_[keyIndex].load();
    state.pressTime = keyPressTime_[keyIndex];
    
    return state;
}

void MacHardware::setKeyLED(uint8_t keyIndex, uint32_t color) {
    if (keyIndex < 26) {
        keyLEDColors_[keyIndex] = color;
        
        // In a real implementation, we'd send this to hardware
        // For simulation, we could send MIDI feedback or update UI
        std::cout << "Key " << static_cast<int>(keyIndex) << " LED: #" 
                  << std::hex << color << std::dec << std::endl;
    }
}

void MacHardware::setKeyLEDBrightness(uint8_t keyIndex, float brightness) {
    // Simulate LED brightness control
    // Could be implemented by dimming the color
}

EncoderState MacHardware::getEncoderState(uint8_t encoderIndex) const {
    if (encoderIndex >= 4) return {};
    
    EncoderState state;
    state.value = encoderValues_[encoderIndex].load();
    state.changed = encoderChanged_[encoderIndex].load(); // Read current state
    state.lastUpdate = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    
    return state;
}

void MacHardware::setEncoderLED(uint8_t encoderIndex, uint32_t color) {
    if (encoderIndex < 4) {
        encoderLEDColors_[encoderIndex] = color;
        std::cout << "Encoder " << static_cast<int>(encoderIndex) << " LED: #" 
                  << std::hex << color << std::dec << std::endl;
    }
}

void MacHardware::setEncoderOLED(uint8_t encoderIndex, const std::string& text) {
    if (encoderIndex < 4) {
        encoderOLEDTexts_[encoderIndex] = text;
        std::cout << "Encoder " << static_cast<int>(encoderIndex) << " OLED: \"" 
                  << text << "\"" << std::endl;
    }
}

void MacHardware::setSmartKnobHaptic(float intensity, uint32_t duration_ms) {
    // Simulate haptic feedback - could trigger trackpad haptics on MacBook
    std::cout << "Smart Knob Haptic: " << intensity << " for " << duration_ms << "ms" << std::endl;
}

void MacHardware::setSmartKnobDetents(bool enabled, float detentStrength) {
    std::cout << "Smart Knob Detents: " << (enabled ? "ON" : "OFF") 
              << " strength: " << detentStrength << std::endl;
}

void MacHardware::setSmartKnobSpring(bool enabled, float springStrength, float centerPosition) {
    std::cout << "Smart Knob Spring: " << (enabled ? "ON" : "OFF") 
              << " strength: " << springStrength << " center: " << centerPosition << std::endl;
}

void MacHardware::updateDisplay() {
    // In the full implementation, this would trigger a SwiftUI update
    // For now, just indicate that display should be refreshed
}

std::array<TouchPoint, 10> MacHardware::getTouchPoints() const {
    return touchPoints_;
}

void MacHardware::setDisplayBrightness(float brightness) {
    displayBrightness_.store(brightness);
}

void MacHardware::sendMIDI(const uint8_t* data, size_t length) {
    // Implementation would send MIDI data out
    std::cout << "MIDI Out: ";
    for (size_t i = 0; i < length; i++) {
        std::cout << std::hex << static_cast<int>(data[i]) << " ";
    }
    std::cout << std::dec << std::endl;
}

void MacHardware::setMIDICallback(std::function<void(const uint8_t*, size_t)> callback) {
    midiCallback_ = callback;
}

bool MacHardware::saveFile(const std::string& path, const uint8_t* data, size_t size) {
    try {
        std::string fullPath = getDocumentsPath() + "/" + path;
        std::filesystem::create_directories(std::filesystem::path(fullPath).parent_path());
        
        std::ofstream file(fullPath, std::ios::binary);
        if (!file) return false;
        
        file.write(reinterpret_cast<const char*>(data), size);
        return file.good();
    } catch (...) {
        return false;
    }
}

bool MacHardware::loadFile(const std::string& path, uint8_t* buffer, size_t maxSize, size_t& actualSize) {
    try {
        std::string fullPath = getDocumentsPath() + "/" + path;
        std::ifstream file(fullPath, std::ios::binary);
        if (!file) return false;
        
        file.seekg(0, std::ios::end);
        actualSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        if (actualSize > maxSize) return false;
        
        file.read(reinterpret_cast<char*>(buffer), actualSize);
        return file.good();
    } catch (...) {
        return false;
    }
}

std::vector<std::string> MacHardware::listFiles(const std::string& directory) {
    std::vector<std::string> files;
    try {
        std::string fullPath = getDocumentsPath() + "/" + directory;
        for (const auto& entry : std::filesystem::directory_iterator(fullPath)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().filename().string());
            }
        }
    } catch (...) {
        // Directory doesn't exist or other error
    }
    return files;
}

std::string MacHardware::getDeviceID() const {
    return "MAC-PROTOTYPE-001";
}

std::string MacHardware::getFirmwareVersion() const {
    return "1.0.0-prototype";
}

float MacHardware::getCPUUsage() const {
    return cpuUsage_.load();
}

size_t MacHardware::getFreeMemory() const {
    vm_size_t page_size;
    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t host_size = sizeof(vm_statistics64_data_t) / sizeof(natural_t);
    
    host_page_size(mach_host_self(), &page_size);
    host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stat, &host_size);
    
    return vm_stat.free_count * page_size;
}

// Input simulation methods
void MacHardware::simulateKeyPress(uint8_t keyIndex, float velocity, float aftertouch) {
    if (keyIndex >= 26) return;
    
    keyPressed_[keyIndex].store(true);
    keyVelocity_[keyIndex].store(velocity);
    keyAftertouch_[keyIndex].store(aftertouch);
    keyPressTime_[keyIndex] = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    
    std::cout << "Key " << static_cast<int>(keyIndex) << " pressed: vel=" << velocity 
              << " aftertouch=" << aftertouch << std::endl;
}

void MacHardware::simulateKeyRelease(uint8_t keyIndex) {
    if (keyIndex >= 26) return;
    
    keyPressed_[keyIndex].store(false);
    keyVelocity_[keyIndex].store(0.0f);
    // Keep aftertouch for release phase
    
    std::cout << "Key " << static_cast<int>(keyIndex) << " released" << std::endl;
}

void MacHardware::simulateEncoderChange(uint8_t encoderIndex, float deltaValue) {
    if (encoderIndex >= 4) return;
    
    float currentValue = encoderValues_[encoderIndex].load();
    float newValue = std::clamp(currentValue + deltaValue, 0.0f, 1.0f);
    
    encoderValues_[encoderIndex].store(newValue);
    encoderChanged_[encoderIndex].store(true);
    
    std::cout << "Encoder " << static_cast<int>(encoderIndex) << " changed: " 
              << newValue << " (delta: " << deltaValue << ")" << std::endl;
}

void MacHardware::simulateSmartKnobChange(float deltaValue) {
    float currentValue = smartKnobValue_.load();
    float newValue = std::clamp(currentValue + deltaValue, 0.0f, 1.0f);
    smartKnobValue_.store(newValue);
    
    std::cout << "Smart Knob changed: " << newValue << " (delta: " << deltaValue << ")" << std::endl;
}

void MacHardware::simulateTouch(float x, float y, bool active, uint32_t touchID) {
    if (touchID >= 10) return;
    
    touchPoints_[touchID].x = std::clamp(x, 0.0f, 1.0f);
    touchPoints_[touchID].y = std::clamp(y, 0.0f, 1.0f);
    touchPoints_[touchID].active = active;
    touchPoints_[touchID].id = touchID;
    
    std::cout << "Touch " << touchID << ": (" << x << ", " << y << ") " 
              << (active ? "active" : "inactive") << std::endl;
}

void MacHardware::simulateTransportButton(bool play, bool stop, bool record) {
    playButton_.store(play);
    stopButton_.store(stop);
    recordButton_.store(record);
    
    std::cout << "Transport: play=" << play << " stop=" << stop << " record=" << record << std::endl;
}

void MacHardware::initializeKeyboardMapping() {
    // Map computer keyboard to ether's 26-key layout
    // Bottom row (white keys 1-8)
    keyboardMapping_['a'] = 0;  // C
    keyboardMapping_['s'] = 2;  // D
    keyboardMapping_['d'] = 4;  // E
    keyboardMapping_['f'] = 5;  // F
    keyboardMapping_['g'] = 7;  // G
    keyboardMapping_['h'] = 9;  // A
    keyboardMapping_['j'] = 11; // B
    keyboardMapping_['k'] = 12; // C
    
    // Black keys (bottom row)
    keyboardMapping_['w'] = 1;  // C#
    keyboardMapping_['e'] = 3;  // D#
    keyboardMapping_['t'] = 6;  // F#
    keyboardMapping_['y'] = 8;  // G#
    keyboardMapping_['u'] = 10; // A#
    
    // Top row (white keys 9-16)
    keyboardMapping_['z'] = 13; // C
    keyboardMapping_['x'] = 14; // D
    keyboardMapping_['c'] = 15; // E
    keyboardMapping_['v'] = 16; // F
    keyboardMapping_['b'] = 17; // G
    keyboardMapping_['n'] = 18; // A
    keyboardMapping_['m'] = 19; // B
    keyboardMapping_[','] = 20; // C
    
    // Black keys (top row)
    keyboardMapping_['q'] = 21; // C#
    keyboardMapping_['r'] = 22; // D#
    keyboardMapping_['i'] = 23; // F#
    keyboardMapping_['o'] = 24; // G#
    keyboardMapping_['p'] = 25; // A#
}

std::string MacHardware::getDocumentsPath() const {
    const char* home = getenv("HOME");
    if (!home) return "/tmp";
    return std::string(home) + "/Documents/ether";
}

std::string MacHardware::getPresetsPath() const {
    return getDocumentsPath() + "/presets";
}