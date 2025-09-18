#include <iostream>
#include "rtmidi/RtMidi.h"
#include <vector>
#include <string>

int main() {
    try {
        // Create MIDI input instance
        RtMidiIn *midiin = new RtMidiIn();
        
        // Check available ports
        unsigned int nPorts = midiin->getPortCount();
        std::cout << "Available MIDI input ports (" << nPorts << "):" << std::endl;
        
        for (unsigned int i = 0; i < nPorts; i++) {
            std::string portName = midiin->getPortName(i);
            std::cout << "  Port " << i << ": " << portName << std::endl;
        }
        
        if (nPorts == 0) {
            std::cout << "No MIDI input ports found." << std::endl;
            std::cout << "Please connect your QT-PY 2040 device and try again." << std::endl;
        }
        
        delete midiin;
    } catch (RtMidiError &error) {
        std::cout << "RtMidi error: " << error.getMessage() << std::endl;
    }
    
    return 0;
}