#include "core/EtherSynth.h"
#include <iostream>
#include <signal.h>

// Global instance for signal handling
std::unique_ptr<EtherSynth> g_synth;

void signalHandler(int signal) {
    std::cout << "\nShutdown signal received..." << std::endl;
    if (g_synth) {
        g_synth->requestShutdown();
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== ether Portable Synthesizer ===" << std::endl;
    std::cout << "Version 1.0.0 - Mac Prototype" << std::endl;
    std::cout << "Copyright 2024 - All Rights Reserved" << std::endl << std::endl;
    
    // Set up signal handling for graceful shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    try {
        // Create main synthesizer instance
        g_synth = std::make_unique<EtherSynth>();
        
        // Initialize all systems
        std::cout << "Initializing ether synthesizer..." << std::endl;
        if (!g_synth->initialize()) {
            std::cerr << "Failed to initialize synthesizer!" << std::endl;
            std::cerr << "Error: " << g_synth->getErrorMessage() << std::endl;
            return 1;
        }
        
        std::cout << "Initialization complete!" << std::endl;
        std::cout << "Current mode: Instrument" << std::endl;
        std::cout << "Hardware: Mac Prototype" << std::endl;
        std::cout << "Audio: Core Audio (" << g_synth->getAudioEngine() << ")" << std::endl;
        std::cout << std::endl;
        
        // Print usage instructions
        std::cout << "=== Control Instructions ===" << std::endl;
        std::cout << "MIDI Controller recommended for best experience" << std::endl;
        std::cout << "Keyboard controls:" << std::endl;
        std::cout << "  Piano keys: AWSEDFTGYHUJKOLP" << std::endl;
        std::cout << "  Encoders: 1234 (select) + QWER (adjust)" << std::endl;
        std::cout << "  Smart knob: Mouse wheel" << std::endl;
        std::cout << "  Modes: ZXCVBNM (Instr/Seq/Chord/Tape/Mod/FX/Proj)" << std::endl;
        std::cout << "  Transport: Space (play/stop), R (record)" << std::endl;
        std::cout << "  Quit: Ctrl+C or ESC" << std::endl;
        std::cout << std::endl;
        
        // Performance monitoring
        std::cout << "=== System Status ===" << std::endl;
        std::cout << "CPU Usage: " << g_synth->getSystemCPUUsage() << "%" << std::endl;
        std::cout << "Free Memory: " << g_synth->getFreeMemory() / (1024*1024) << " MB" << std::endl;
        std::cout << "Battery: " << g_synth->getBatteryLevel() * 100 << "%" << std::endl;
        std::cout << std::endl;
        
        std::cout << "ether synthesizer ready! Starting main loop..." << std::endl;
        
        // Run main application loop
        g_synth->run();
        
        std::cout << "Shutting down..." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown fatal error occurred!" << std::endl;
        return 1;
    }
    
    // Clean shutdown
    g_synth.reset();
    std::cout << "Goodbye!" << std::endl;
    
    return 0;
}