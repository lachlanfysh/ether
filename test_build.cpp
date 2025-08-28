// Simple test to verify our architecture compiles
#include "src/core/EtherSynth.h"
#include "src/audio/AudioEngine.h"
#include "src/instruments/InstrumentSlot.h"
#include "src/synthesis/SynthEngine.h"
#include "src/hardware/HardwareInterface.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << "=== ether Architecture Test ===" << std::endl;
    
    try {
        // Create and initialize ether
        EtherSynth synth;
        
        std::cout << "Initializing..." << std::endl;
        if (!synth.initialize()) {
            std::cerr << "Failed to initialize: " << synth.getErrorMessage() << std::endl;
            return 1;
        }
        
        std::cout << "Initialization successful!" << std::endl;
        std::cout << "CPU Usage: " << synth.getSystemCPUUsage() << "%" << std::endl;
        std::cout << "Free Memory: " << synth.getFreeMemory() / (1024*1024) << " MB" << std::endl;
        
        // Test basic functionality
        auto* hardware = synth.getHardware();
        auto* audioEngine = synth.getAudioEngine();
        
        if (hardware && audioEngine) {
            std::cout << "Hardware: " << hardware->getDeviceID() << std::endl;
            std::cout << "Firmware: " << hardware->getFirmwareVersion() << std::endl;
            
            // Test instrument creation
            auto* redInstrument = audioEngine->getInstrument(InstrumentColor::RED);
            if (redInstrument) {
                std::cout << "Red instrument: " << redInstrument->getName() << std::endl;
                std::cout << "Engine count: " << redInstrument->getEngineCount() << std::endl;
                
                if (redInstrument->getEngineCount() > 0) {
                    auto* engine = redInstrument->getPrimaryEngine();
                    if (engine) {
                        std::cout << "Primary engine: " << engine->getName() << std::endl;
                        
                        // Test parameter setting
                        engine->setParameter(ParameterID::VOLUME, 0.5f);
                        float volume = engine->getParameter(ParameterID::VOLUME);
                        std::cout << "Volume parameter: " << volume << std::endl;
                    }
                }
            }
            
            // Test note events
            std::cout << "Testing note events..." << std::endl;
            audioEngine->noteOn(0, 0.8f); // Trigger note
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            std::cout << "Active voices: " << audioEngine->getActiveVoiceCount() << std::endl;
            
            audioEngine->noteOff(0);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Test transport
            std::cout << "Testing transport..." << std::endl;
            audioEngine->setBPM(120.0f);
            audioEngine->play();
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            audioEngine->stop();
        }
        
        std::cout << "All tests passed!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "Test completed successfully" << std::endl;
    return 0;
}