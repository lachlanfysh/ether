#include "VoiceManager.h"
#include <iostream>

VoiceManager::VoiceManager() {
    std::cout << "VoiceManager created" << std::endl;
}

VoiceManager::~VoiceManager() {
    std::cout << "VoiceManager destroyed" << std::endl;
}

void VoiceManager::noteOn(uint8_t note, float velocity, float aftertouch) {
    // Stub implementation
    activeVoices_++;
    std::cout << "VoiceManager: Note ON " << static_cast<int>(note) 
              << " vel=" << velocity << " after=" << aftertouch 
              << " (voices=" << activeVoices_ << ")" << std::endl;
}

void VoiceManager::noteOff(uint8_t note) {
    // Stub implementation
    if (activeVoices_ > 0) activeVoices_--;
    std::cout << "VoiceManager: Note OFF " << static_cast<int>(note) 
              << " (voices=" << activeVoices_ << ")" << std::endl;
}

void VoiceManager::allNotesOff() {
    activeVoices_ = 0;
    std::cout << "VoiceManager: All notes OFF" << std::endl;
}

void VoiceManager::processAudio(EtherAudioBuffer& outputBuffer) {
    // Stub implementation - just clear the buffer for now
    for (auto& frame : outputBuffer) {
        frame.left = 0.0f;
        frame.right = 0.0f;
    }
}