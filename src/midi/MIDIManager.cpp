#include "MIDIManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <sstream>

#ifdef PLATFORM_MAC
#include <mach/mach_time.h>
#endif

MIDIManager::MIDIManager() {
    std::cout << "MIDIManager created" << std::endl;
}

MIDIManager::~MIDIManager() {
    shutdown();
    std::cout << "MIDIManager destroyed" << std::endl;
}

bool MIDIManager::initialize() {
    if (initialized_) {
        return true;
    }
    
    std::cout << "Initializing MIDI system..." << std::endl;
    
    if (!initializePlatform()) {
        lastError_ = MIDIError::INITIALIZATION_FAILED;
        return false;
    }
    
    // Scan for available devices
    scanDevices();
    
    // Set up default MIDI mappings
    MIDILearnSlot volumeMapping;
    volumeMapping.controller = CC_VOLUME;
    volumeMapping.channel = 0;
    volumeMapping.parameter = ParameterID::VOLUME;
    volumeMapping.minValue = 0.0f;
    volumeMapping.maxValue = 1.0f;
    volumeMapping.learned = true;
    volumeMapping.description = "Main Volume";
    midiMappings_.push_back(volumeMapping);
    
    MIDILearnSlot filterMapping;
    filterMapping.controller = CC_SOUND_1;
    filterMapping.channel = 0;
    filterMapping.parameter = ParameterID::FILTER_CUTOFF;
    filterMapping.minValue = 0.0f;
    filterMapping.maxValue = 1.0f;
    filterMapping.learned = true;
    filterMapping.description = "Filter Cutoff";
    midiMappings_.push_back(filterMapping);
    
    initialized_ = true;
    lastError_ = MIDIError::NONE;
    
    std::cout << "MIDI system initialized successfully" << std::endl;
    std::cout << "Found " << availableDevices_.size() << " MIDI devices" << std::endl;
    
    return true;
}

void MIDIManager::shutdown() {
    if (!initialized_) {
        return;
    }
    
    std::cout << "Shutting down MIDI system..." << std::endl;
    
    disconnectAllDevices();
    shutdownPlatform();
    
    availableDevices_.clear();
    midiMappings_.clear();
    
    initialized_ = false;
    std::cout << "MIDI system shut down" << std::endl;
}

std::vector<MIDIManager::MIDIDevice> MIDIManager::getAvailableDevices() const {
    return availableDevices_;
}

bool MIDIManager::connectInputDevice(uint32_t deviceId) {
    if (!initialized_) {
        lastError_ = MIDIError::INITIALIZATION_FAILED;
        return false;
    }
    
    // Find device
    auto it = std::find_if(availableDevices_.begin(), availableDevices_.end(),
        [deviceId](const MIDIDevice& device) { return device.id == deviceId && device.isInput; });
    
    if (it == availableDevices_.end()) {
        lastError_ = MIDIError::DEVICE_NOT_FOUND;
        return false;
    }
    
    if (!connectInputDevicePlatform(deviceId)) {
        lastError_ = MIDIError::CONNECTION_FAILED;
        return false;
    }
    
    it->isConnected = true;
    lastError_ = MIDIError::NONE;
    
    std::cout << "Connected MIDI input: " << it->name << std::endl;
    return true;
}

bool MIDIManager::connectOutputDevice(uint32_t deviceId) {
    if (!initialized_) {
        lastError_ = MIDIError::INITIALIZATION_FAILED;
        return false;
    }
    
    // Find device
    auto it = std::find_if(availableDevices_.begin(), availableDevices_.end(),
        [deviceId](const MIDIDevice& device) { return device.id == deviceId && device.isOutput; });
    
    if (it == availableDevices_.end()) {
        lastError_ = MIDIError::DEVICE_NOT_FOUND;
        return false;
    }
    
    if (!connectOutputDevicePlatform(deviceId)) {
        lastError_ = MIDIError::CONNECTION_FAILED;
        return false;
    }
    
    it->isConnected = true;
    lastError_ = MIDIError::NONE;
    
    std::cout << "Connected MIDI output: " << it->name << std::endl;
    return true;
}

void MIDIManager::disconnectInputDevice(uint32_t deviceId) {
    auto it = std::find_if(availableDevices_.begin(), availableDevices_.end(),
        [deviceId](const MIDIDevice& device) { return device.id == deviceId && device.isInput; });
    
    if (it != availableDevices_.end() && it->isConnected) {
        it->isConnected = false;
        std::cout << "Disconnected MIDI input: " << it->name << std::endl;
    }
}

void MIDIManager::disconnectOutputDevice(uint32_t deviceId) {
    auto it = std::find_if(availableDevices_.begin(), availableDevices_.end(),
        [deviceId](const MIDIDevice& device) { return device.id == deviceId && device.isOutput; });
    
    if (it != availableDevices_.end() && it->isConnected) {
        it->isConnected = false;
        std::cout << "Disconnected MIDI output: " << it->name << std::endl;
    }
}

void MIDIManager::disconnectAllDevices() {
    for (auto& device : availableDevices_) {
        if (device.isConnected) {
            device.isConnected = false;
        }
    }
    
#ifdef PLATFORM_MAC
    connectedInputs_.clear();
    connectedOutputs_.clear();
#endif
    
    std::cout << "Disconnected all MIDI devices" << std::endl;
}

void MIDIManager::sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    if (!isValidNote(note) || !isValidChannel(channel)) {
        return;
    }
    
    note = applyTranspose(note);
    
    std::vector<uint8_t> message = {
        static_cast<uint8_t>(static_cast<uint8_t>(MessageType::NOTE_ON) | channel),
        note,
        velocity
    };
    
    sendMIDIMessagePlatform(message);
    outputMessageCount_++;
}

void MIDIManager::sendNoteOff(uint8_t note, uint8_t channel) {
    if (!isValidNote(note) || !isValidChannel(channel)) {
        return;
    }
    
    note = applyTranspose(note);
    
    std::vector<uint8_t> message = {
        static_cast<uint8_t>(static_cast<uint8_t>(MessageType::NOTE_OFF) | channel),
        note,
        0
    };
    
    sendMIDIMessagePlatform(message);
    outputMessageCount_++;
}

void MIDIManager::sendControlChange(uint8_t controller, uint8_t value, uint8_t channel) {
    if (!isValidController(controller) || !isValidChannel(channel)) {
        return;
    }
    
    std::vector<uint8_t> message = {
        static_cast<uint8_t>(static_cast<uint8_t>(MessageType::CONTROL_CHANGE) | channel),
        controller,
        value
    };
    
    sendMIDIMessagePlatform(message);
    outputMessageCount_++;
}

void MIDIManager::sendPitchBend(uint16_t value, uint8_t channel) {
    if (!isValidChannel(channel)) {
        return;
    }
    
    uint8_t lsb = value & 0x7F;
    uint8_t msb = (value >> 7) & 0x7F;
    
    std::vector<uint8_t> message = {
        static_cast<uint8_t>(static_cast<uint8_t>(MessageType::PITCH_BEND) | channel),
        lsb,
        msb
    };
    
    sendMIDIMessagePlatform(message);
    outputMessageCount_++;
}

void MIDIManager::sendAftertouch(uint8_t value, uint8_t channel) {
    if (!isValidChannel(channel)) {
        return;
    }
    
    std::vector<uint8_t> message = {
        static_cast<uint8_t>(static_cast<uint8_t>(MessageType::CHANNEL_AFTERTOUCH) | channel),
        value
    };
    
    sendMIDIMessagePlatform(message);
    outputMessageCount_++;
}

void MIDIManager::sendPolyAftertouch(uint8_t note, uint8_t value, uint8_t channel) {
    if (!isValidNote(note) || !isValidChannel(channel)) {
        return;
    }
    
    note = applyTranspose(note);
    
    std::vector<uint8_t> message = {
        static_cast<uint8_t>(static_cast<uint8_t>(MessageType::POLY_AFTERTOUCH) | channel),
        note,
        value
    };
    
    sendMIDIMessagePlatform(message);
    outputMessageCount_++;
}

void MIDIManager::sendProgramChange(uint8_t program, uint8_t channel) {
    if (!isValidChannel(channel)) {
        return;
    }
    
    std::vector<uint8_t> message = {
        static_cast<uint8_t>(static_cast<uint8_t>(MessageType::PROGRAM_CHANGE) | channel),
        program
    };
    
    sendMIDIMessagePlatform(message);
    outputMessageCount_++;
}

void MIDIManager::sendSysEx(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return;
    }
    
    std::vector<uint8_t> message;
    message.push_back(static_cast<uint8_t>(MessageType::SYSTEM_EXCLUSIVE));
    message.insert(message.end(), data.begin(), data.end());
    message.push_back(0xF7); // End of SysEx
    
    sendMIDIMessagePlatform(message);
    outputMessageCount_++;
}

void MIDIManager::sendClock() {
    std::vector<uint8_t> message = { static_cast<uint8_t>(MessageType::CLOCK) };
    sendMIDIMessagePlatform(message);
    outputMessageCount_++;
}

void MIDIManager::sendStart() {
    std::vector<uint8_t> message = { static_cast<uint8_t>(MessageType::START) };
    sendMIDIMessagePlatform(message);
    outputMessageCount_++;
}

void MIDIManager::sendStop() {
    std::vector<uint8_t> message = { static_cast<uint8_t>(MessageType::STOP) };
    sendMIDIMessagePlatform(message);
    outputMessageCount_++;
}

void MIDIManager::sendContinue() {
    std::vector<uint8_t> message = { static_cast<uint8_t>(MessageType::CONTINUE) };
    sendMIDIMessagePlatform(message);
    outputMessageCount_++;
}

void MIDIManager::startMIDILearn(ParameterID parameter) {
    midiLearning_ = true;
    learnParameter_ = parameter;
    std::cout << "Started MIDI learn for parameter " << static_cast<int>(parameter) << std::endl;
}

void MIDIManager::stopMIDILearn() {
    midiLearning_ = false;
    std::cout << "Stopped MIDI learn" << std::endl;
}

void MIDIManager::clearMIDIMapping(ParameterID parameter) {
    auto it = std::remove_if(midiMappings_.begin(), midiMappings_.end(),
        [parameter](const MIDILearnSlot& slot) { return slot.parameter == parameter; });
    
    if (it != midiMappings_.end()) {
        midiMappings_.erase(it, midiMappings_.end());
        std::cout << "Cleared MIDI mapping for parameter " << static_cast<int>(parameter) << std::endl;
    }
}

void MIDIManager::clearAllMIDIMappings() {
    midiMappings_.clear();
    std::cout << "Cleared all MIDI mappings" << std::endl;
}

std::vector<MIDIManager::MIDILearnSlot> MIDIManager::getMIDIMappings() const {
    return midiMappings_;
}

void MIDIManager::setInputChannel(uint8_t channel) {
    inputChannel_ = std::min(channel, static_cast<uint8_t>(16));
    std::cout << "Set MIDI input channel to " << 
        (inputChannel_ == 16 ? "Omni" : std::to_string(inputChannel_ + 1)) << std::endl;
}

void MIDIManager::setOutputChannel(uint8_t channel) {
    outputChannel_ = std::clamp(channel, static_cast<uint8_t>(0), static_cast<uint8_t>(15));
    std::cout << "Set MIDI output channel to " << (outputChannel_ + 1) << std::endl;
}

void MIDIManager::setVelocityCurve(float curve) {
    velocityCurve_ = std::clamp(curve, -1.0f, 1.0f);
    std::cout << "Set velocity curve to " << velocityCurve_ << std::endl;
}

void MIDIManager::setTranspose(int semitones) {
    transpose_ = std::clamp(semitones, -24, 24);
    std::cout << "Set transpose to " << transpose_ << " semitones" << std::endl;
}

size_t MIDIManager::getConnectedInputCount() const {
    return std::count_if(availableDevices_.begin(), availableDevices_.end(),
        [](const MIDIDevice& device) { return device.isInput && device.isConnected; });
}

size_t MIDIManager::getConnectedOutputCount() const {
    return std::count_if(availableDevices_.begin(), availableDevices_.end(),
        [](const MIDIDevice& device) { return device.isOutput && device.isConnected; });
}

void MIDIManager::resetMessageCounts() {
    inputMessageCount_ = 0;
    outputMessageCount_ = 0;
}

std::string MIDIManager::getErrorMessage() const {
    switch (lastError_) {
        case MIDIError::NONE: return "No error";
        case MIDIError::INITIALIZATION_FAILED: return "MIDI initialization failed";
        case MIDIError::DEVICE_NOT_FOUND: return "MIDI device not found";
        case MIDIError::CONNECTION_FAILED: return "Failed to connect MIDI device";
        case MIDIError::SEND_FAILED: return "Failed to send MIDI message";
        case MIDIError::RECEIVE_FAILED: return "Failed to receive MIDI message";
        case MIDIError::INVALID_MESSAGE: return "Invalid MIDI message";
        case MIDIError::DEVICE_DISCONNECTED: return "MIDI device disconnected";
        default: return "Unknown MIDI error";
    }
}

void MIDIManager::setLatencyCompensation(float milliseconds) {
    latencyCompensation_ = std::clamp(milliseconds, 0.0f, 100.0f);
}

void MIDIManager::enableMIDIThru(bool enable) {
    midiThru_ = enable;
    std::cout << "MIDI thru " << (enable ? "enabled" : "disabled") << std::endl;
}

void MIDIManager::saveMIDISettings(std::vector<uint8_t>& data) const {
    data.clear();
    
    // Save basic settings
    data.push_back(inputChannel_);
    data.push_back(outputChannel_);
    
    // Save velocity curve and transpose
    float settings[] = { velocityCurve_, static_cast<float>(transpose_), latencyCompensation_ };
    const uint8_t* settingsPtr = reinterpret_cast<const uint8_t*>(settings);
    data.insert(data.end(), settingsPtr, settingsPtr + sizeof(settings));
    
    // Save MIDI mappings count
    uint32_t mappingCount = static_cast<uint32_t>(midiMappings_.size());
    const uint8_t* countPtr = reinterpret_cast<const uint8_t*>(&mappingCount);
    data.insert(data.end(), countPtr, countPtr + sizeof(mappingCount));
    
    // Save each mapping
    for (const auto& mapping : midiMappings_) {
        const uint8_t* mappingPtr = reinterpret_cast<const uint8_t*>(&mapping);
        data.insert(data.end(), mappingPtr, mappingPtr + sizeof(MIDILearnSlot) - sizeof(std::string));
        
        // Save description string separately
        uint32_t descLen = static_cast<uint32_t>(mapping.description.length());
        const uint8_t* descLenPtr = reinterpret_cast<const uint8_t*>(&descLen);
        data.insert(data.end(), descLenPtr, descLenPtr + sizeof(descLen));
        data.insert(data.end(), mapping.description.begin(), mapping.description.end());
    }
}

bool MIDIManager::loadMIDISettings(const std::vector<uint8_t>& data) {
    if (data.size() < 2 + sizeof(float) * 3 + sizeof(uint32_t)) {
        return false;
    }
    
    size_t offset = 0;
    
    // Load basic settings
    inputChannel_ = data[offset++];
    outputChannel_ = data[offset++];
    
    // Load velocity curve and transpose
    float settings[3];
    std::memcpy(settings, data.data() + offset, sizeof(settings));
    offset += sizeof(settings);
    
    velocityCurve_ = settings[0];
    transpose_ = static_cast<int>(settings[1]);
    latencyCompensation_ = settings[2];
    
    // Load MIDI mappings
    if (offset + sizeof(uint32_t) > data.size()) {
        return false;
    }
    
    uint32_t mappingCount;
    std::memcpy(&mappingCount, data.data() + offset, sizeof(mappingCount));
    offset += sizeof(mappingCount);
    
    midiMappings_.clear();
    midiMappings_.reserve(mappingCount);
    
    for (uint32_t i = 0; i < mappingCount; i++) {
        if (offset + sizeof(MIDILearnSlot) - sizeof(std::string) + sizeof(uint32_t) > data.size()) {
            return false;
        }
        
        MIDILearnSlot mapping;
        std::memcpy(&mapping, data.data() + offset, sizeof(MIDILearnSlot) - sizeof(std::string));
        offset += sizeof(MIDILearnSlot) - sizeof(std::string);
        
        // Load description string
        uint32_t descLen;
        std::memcpy(&descLen, data.data() + offset, sizeof(descLen));
        offset += sizeof(descLen);
        
        if (offset + descLen > data.size()) {
            return false;
        }
        
        mapping.description = std::string(data.begin() + offset, data.begin() + offset + descLen);
        offset += descLen;
        
        midiMappings_.push_back(mapping);
    }
    
    std::cout << "Loaded MIDI settings with " << midiMappings_.size() << " mappings" << std::endl;
    return true;
}

// Private methods
void MIDIManager::processMIDIMessage(const MIDIMessage& message) {
    inputMessageCount_++;
    
    // Check channel filtering
    if (inputChannel_ < 16 && message.channel != inputChannel_) {
        return;
    }
    
    // Apply MIDI mappings
    applyMIDIMapping(message);
    
    // Handle MIDI learn
    if (midiLearning_ && message.isControlChange()) {
        // Create new mapping
        MIDILearnSlot newMapping;
        newMapping.controller = message.data1;
        newMapping.channel = message.channel;
        newMapping.parameter = learnParameter_;
        newMapping.minValue = 0.0f;
        newMapping.maxValue = 1.0f;
        newMapping.learned = true;
        newMapping.description = "Learned CC" + std::to_string(message.data1);
        
        // Remove existing mapping for this parameter
        clearMIDIMapping(learnParameter_);
        
        // Add new mapping
        midiMappings_.push_back(newMapping);
        
        stopMIDILearn();
        std::cout << "Learned MIDI mapping: CC" << static_cast<int>(message.data1) 
                  << " -> Parameter " << static_cast<int>(learnParameter_) << std::endl;
    }
    
    // Call appropriate callbacks
    if (onMIDIReceived) {
        onMIDIReceived(message);
    }
    
    if (message.isNoteOn() && onNoteOn) {
        float velocity = applyVelocityCurve(message.data2);
        uint8_t note = applyTranspose(message.data1);
        onNoteOn(note, velocity, message.channel);
    } else if (message.isNoteOff() && onNoteOff) {
        uint8_t note = applyTranspose(message.data1);
        onNoteOff(note, message.channel);
    } else if (message.isControlChange() && onControlChange) {
        float value = MIDIUtils::midiToFloat(message.data2);
        onControlChange(message.data1, value, message.channel);
    } else if (message.isPitchBend() && onPitchBend) {
        uint16_t pitchBendValue = (message.data2 << 7) | message.data1;
        float pitchBend = MIDIUtils::pitchBendToFloat(pitchBendValue);
        onPitchBend(pitchBend, message.channel);
    } else if (message.isAftertouch()) {
        if (message.type == MessageType::CHANNEL_AFTERTOUCH && onAftertouch) {
            float aftertouch = MIDIUtils::midiToFloat(message.data1);
            onAftertouch(aftertouch, message.channel);
        } else if (message.type == MessageType::POLY_AFTERTOUCH && onPolyAftertouch) {
            uint8_t note = applyTranspose(message.data1);
            float aftertouch = MIDIUtils::midiToFloat(message.data2);
            onPolyAftertouch(note, aftertouch, message.channel);
        }
    } else if (message.type == MessageType::PROGRAM_CHANGE && onProgramChange) {
        onProgramChange(message.data1, message.channel);
    }
    
    // Handle clock messages
    switch (message.type) {
        case MessageType::CLOCK:
            if (onMIDIClock) onMIDIClock();
            break;
        case MessageType::START:
            if (onMIDIStart) onMIDIStart();
            break;
        case MessageType::STOP:
            if (onMIDIStop) onMIDIStop();
            break;
        case MessageType::CONTINUE:
            if (onMIDIContinue) onMIDIContinue();
            break;
        default:
            break;
    }
    
    // MIDI thru
    if (midiThru_ && getConnectedOutputCount() > 0) {
        // Retransmit message to outputs
        std::vector<uint8_t> messageData;
        messageData.push_back(static_cast<uint8_t>(message.type) | message.channel);
        messageData.push_back(message.data1);
        if (message.type != MessageType::PROGRAM_CHANGE && message.type != MessageType::CHANNEL_AFTERTOUCH) {
            messageData.push_back(message.data2);
        }
        sendMIDIMessagePlatform(messageData);
    }
}

void MIDIManager::applyMIDIMapping(const MIDIMessage& message) {
    if (!message.isControlChange()) {
        return;
    }
    
    for (const auto& mapping : midiMappings_) {
        if (mapping.learned && 
            mapping.controller == message.data1 && 
            mapping.channel == message.channel) {
            
            // Convert MIDI value to parameter range
            float normalizedValue = MIDIUtils::midiToFloat(message.data2);
            float mappedValue = mapping.minValue + normalizedValue * (mapping.maxValue - mapping.minValue);
            
            std::cout << "MIDI mapping: CC" << static_cast<int>(message.data1) 
                      << " (" << static_cast<int>(message.data2) << ") -> Parameter " 
                      << static_cast<int>(mapping.parameter) << " (" << mappedValue << ")" << std::endl;
            
            // Here you would typically call a parameter change callback
            // For now, just log the mapping
        }
    }
}

float MIDIManager::applyVelocityCurve(uint8_t velocity) const {
    float normalizedVel = static_cast<float>(velocity) / 127.0f;
    
    if (velocityCurve_ == 0.0f) {
        return normalizedVel; // Linear
    } else if (velocityCurve_ > 0.0f) {
        // Exponential (harder)
        return std::pow(normalizedVel, 1.0f + velocityCurve_ * 2.0f);
    } else {
        // Logarithmic (softer)
        float curve = 1.0f + std::abs(velocityCurve_) * 2.0f;
        return std::pow(normalizedVel, 1.0f / curve);
    }
}

uint8_t MIDIManager::applyTranspose(uint8_t note) const {
    int transposed = static_cast<int>(note) + transpose_;
    return static_cast<uint8_t>(std::clamp(transposed, 0, 127));
}

std::string MIDIManager::getMIDIMessageTypeName(MessageType type) const {
    switch (type) {
        case MessageType::NOTE_OFF: return "Note Off";
        case MessageType::NOTE_ON: return "Note On";
        case MessageType::POLY_AFTERTOUCH: return "Poly Aftertouch";
        case MessageType::CONTROL_CHANGE: return "Control Change";
        case MessageType::PROGRAM_CHANGE: return "Program Change";
        case MessageType::CHANNEL_AFTERTOUCH: return "Channel Aftertouch";
        case MessageType::PITCH_BEND: return "Pitch Bend";
        case MessageType::SYSTEM_EXCLUSIVE: return "System Exclusive";
        case MessageType::CLOCK: return "Clock";
        case MessageType::START: return "Start";
        case MessageType::STOP: return "Stop";
        case MessageType::CONTINUE: return "Continue";
        default: return "Unknown";
    }
}

bool MIDIManager::isValidChannel(uint8_t channel) const {
    return channel <= 15;
}

bool MIDIManager::isValidNote(uint8_t note) const {
    return note <= 127;
}

bool MIDIManager::isValidController(uint8_t controller) const {
    return controller <= 127;
}

// Platform-specific implementations
#ifdef PLATFORM_MAC

bool MIDIManager::initializePlatform() {
    OSStatus result = MIDIClientCreate(CFSTR("ether MIDI Client"), nullptr, nullptr, &midiClient_);
    if (result != noErr) {
        std::cout << "Failed to create MIDI client: " << result << std::endl;
        return false;
    }
    
    result = MIDIInputPortCreate(midiClient_, CFSTR("ether Input Port"), midiInputCallback, this, &inputPort_);
    if (result != noErr) {
        std::cout << "Failed to create MIDI input port: " << result << std::endl;
        return false;
    }
    
    result = MIDIOutputPortCreate(midiClient_, CFSTR("ether Output Port"), &outputPort_);
    if (result != noErr) {
        std::cout << "Failed to create MIDI output port: " << result << std::endl;
        return false;
    }
    
    return true;
}

void MIDIManager::shutdownPlatform() {
    if (inputPort_) {
        MIDIPortDispose(inputPort_);
        inputPort_ = 0;
    }
    
    if (outputPort_) {
        MIDIPortDispose(outputPort_);
        outputPort_ = 0;
    }
    
    if (midiClient_) {
        MIDIClientDispose(midiClient_);
        midiClient_ = 0;
    }
}

void MIDIManager::scanDevices() {
    availableDevices_.clear();
    
    // Scan input devices
    ItemCount sourceCount = MIDIGetNumberOfSources();
    for (ItemCount i = 0; i < sourceCount; i++) {
        MIDIEndpointRef source = MIDIGetSource(i);
        
        CFStringRef name = nullptr;
        MIDIObjectGetStringProperty(source, kMIDIPropertyName, &name);
        
        MIDIDevice device;
        if (name) {
            char nameBuffer[256];
            CFStringGetCString(name, nameBuffer, sizeof(nameBuffer), kCFStringEncodingUTF8);
            device.name = nameBuffer;
            CFRelease(name);
        } else {
            device.name = "Unknown Input " + std::to_string(i);
        }
        
        device.id = static_cast<uint32_t>(source);
        device.isInput = true;
        device.isOutput = false;
        device.isConnected = false;
        device.manufacturer = "Unknown";
        device.model = "Unknown";
        
        availableDevices_.push_back(device);
    }
    
    // Scan output devices
    ItemCount destCount = MIDIGetNumberOfDestinations();
    for (ItemCount i = 0; i < destCount; i++) {
        MIDIEndpointRef dest = MIDIGetDestination(i);
        
        CFStringRef name = nullptr;
        MIDIObjectGetStringProperty(dest, kMIDIPropertyName, &name);
        
        MIDIDevice device;
        if (name) {
            char nameBuffer[256];
            CFStringGetCString(name, nameBuffer, sizeof(nameBuffer), kCFStringEncodingUTF8);
            device.name = nameBuffer;
            CFRelease(name);
        } else {
            device.name = "Unknown Output " + std::to_string(i);
        }
        
        device.id = static_cast<uint32_t>(dest);
        device.isInput = false;
        device.isOutput = true;
        device.isConnected = false;
        device.manufacturer = "Unknown";
        device.model = "Unknown";
        
        availableDevices_.push_back(device);
    }
}

bool MIDIManager::connectInputDevicePlatform(uint32_t deviceId) {
    MIDIEndpointRef source = static_cast<MIDIEndpointRef>(deviceId);
    OSStatus result = MIDIPortConnectSource(inputPort_, source, nullptr);
    
    if (result == noErr) {
        connectedInputs_.push_back(source);
        return true;
    }
    
    return false;
}

bool MIDIManager::connectOutputDevicePlatform(uint32_t deviceId) {
    MIDIEndpointRef dest = static_cast<MIDIEndpointRef>(deviceId);
    connectedOutputs_.push_back(dest);
    return true;
}

void MIDIManager::sendMIDIMessagePlatform(const std::vector<uint8_t>& message) {
    if (connectedOutputs_.empty() || message.empty()) {
        return;
    }
    
    // Create MIDI packet
    Byte packetBuffer[1024];
    MIDIPacketList* packetList = reinterpret_cast<MIDIPacketList*>(packetBuffer);
    MIDIPacket* packet = MIDIPacketListInit(packetList);
    
    packet = MIDIPacketListAdd(packetList, sizeof(packetBuffer), packet, 
                              mach_absolute_time(), message.size(), message.data());
    
    if (packet) {
        // Send to all connected outputs
        for (MIDIEndpointRef dest : connectedOutputs_) {
            MIDISend(outputPort_, dest, packetList);
        }
    }
}

void MIDIManager::midiInputCallback(const MIDIPacketList* packetList, void* readProcRefCon, void* srcConnRefCon) {
    MIDIManager* manager = static_cast<MIDIManager*>(readProcRefCon);
    if (!manager) return;
    
    const MIDIPacket* packet = &packetList->packet[0];
    for (UInt32 i = 0; i < packetList->numPackets; i++) {
        manager->processMIDIPacket(packet);
        packet = MIDIPacketNext(packet);
    }
}

void MIDIManager::processMIDIPacket(const MIDIPacket* packet) {
    if (!packet || packet->length == 0) return;
    
    MIDIMessage message = parseMIDIData(packet->data, packet->length, static_cast<uint32_t>(packet->timeStamp));
    processMIDIMessage(message);
}

MIDIManager::MIDIMessage MIDIManager::parseMIDIData(const uint8_t* data, size_t length, uint32_t timestamp) {
    MIDIMessage message;
    message.timestamp = timestamp;
    
    if (length == 0) {
        message.type = MessageType::RESET;
        return message;
    }
    
    uint8_t statusByte = data[0];
    message.type = static_cast<MessageType>(statusByte & 0xF0);
    message.channel = statusByte & 0x0F;
    
    if (length > 1) message.data1 = data[1];
    if (length > 2) message.data2 = data[2];
    
    // Handle SysEx
    if (message.type == MessageType::SYSTEM_EXCLUSIVE) {
        message.sysex.assign(data + 1, data + length - 1); // Exclude F0 and F7
    }
    
    return message;
}

#else

// Stub implementations for non-Mac platforms
bool MIDIManager::initializePlatform() {
    std::cout << "MIDI not implemented for this platform" << std::endl;
    return false;
}

void MIDIManager::shutdownPlatform() {}
void MIDIManager::scanDevices() {}
bool MIDIManager::connectInputDevicePlatform(uint32_t) { return false; }
bool MIDIManager::connectOutputDevicePlatform(uint32_t) { return false; }
void MIDIManager::sendMIDIMessagePlatform(const std::vector<uint8_t>&) {}

#endif

// MIDI utility functions
namespace MIDIUtils {
    float noteToFrequency(uint8_t note) {
        return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    }
    
    uint8_t frequencyToNote(float frequency) {
        return static_cast<uint8_t>(std::round(69 + 12 * std::log2(frequency / 440.0f)));
    }
    
    std::string noteToName(uint8_t note) {
        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        int octave = (note / 12) - 1;
        int noteIndex = note % 12;
        return std::string(noteNames[noteIndex]) + std::to_string(octave);
    }
    
    uint8_t nameToNote(const std::string& name) {
        // Simple implementation - would need more sophisticated parsing in practice
        if (name.length() < 2) return 60; // Middle C default
        
        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        
        for (int i = 0; i < 12; i++) {
            if (name.substr(0, strlen(noteNames[i])) == noteNames[i]) {
                int octave = std::stoi(name.substr(strlen(noteNames[i])));
                return static_cast<uint8_t>((octave + 1) * 12 + i);
            }
        }
        
        return 60; // Default to middle C
    }
    
    float midiToFloat(uint8_t midiValue) {
        return static_cast<float>(midiValue) / 127.0f;
    }
    
    uint8_t floatToMIDI(float value) {
        return static_cast<uint8_t>(std::clamp(value * 127.0f, 0.0f, 127.0f));
    }
    
    float pitchBendToFloat(uint16_t pitchBend) {
        return (static_cast<float>(pitchBend) - 8192.0f) / 8192.0f;
    }
    
    uint16_t floatToPitchBend(float value) {
        return static_cast<uint16_t>(std::clamp((value + 1.0f) * 8192.0f, 0.0f, 16383.0f));
    }
    
    uint32_t beatsToTicks(float beats, uint16_t ppq) {
        return static_cast<uint32_t>(beats * ppq);
    }
    
    float ticksToBeats(uint32_t ticks, uint16_t ppq) {
        return static_cast<float>(ticks) / ppq;
    }
    
    uint32_t msToTicks(float milliseconds, float bpm, uint16_t ppq) {
        float beatsPerMs = bpm / 60000.0f;
        float beats = milliseconds * beatsPerMs;
        return beatsToTicks(beats, ppq);
    }
    
    float ticksToMs(uint32_t ticks, float bpm, uint16_t ppq) {
        float beats = ticksToBeats(ticks, ppq);
        float msPerBeat = 60000.0f / bpm;
        return beats * msPerBeat;
    }
}