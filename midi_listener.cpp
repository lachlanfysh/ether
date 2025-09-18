#include <iostream>
#include "rtmidi/RtMidi.h"
#include <vector>
#include <string>
#include <chrono>
#include <thread>

// MIDI callback function to handle incoming messages
void midiCallback(double deltatime, std::vector<unsigned char> *message, void *userData) {
    unsigned int nBytes = message->size();
    if (nBytes > 0) {
        std::cout << "MIDI message (" << nBytes << " bytes): ";
        for (unsigned int i = 0; i < nBytes; i++) {
            std::cout << "0x" << std::hex << (int)message->at(i) << " ";
        }
        std::cout << std::dec << std::endl;
        
        // Parse common MIDI messages
        if (nBytes >= 3) {
            unsigned char status = message->at(0);
            unsigned char data1 = message->at(1);  // CC number or note
            unsigned char data2 = message->at(2);  // CC value or velocity
            
            if ((status & 0xF0) == 0xB0) {  // Control Change
                std::cout << "  Control Change: CC" << (int)data1 << " = " << (int)data2 << std::endl;
            } else if ((status & 0xF0) == 0x90) {  // Note On
                std::cout << "  Note On: Note " << (int)data1 << ", Velocity " << (int)data2 << std::endl;
            } else if ((status & 0xF0) == 0x80) {  // Note Off
                std::cout << "  Note Off: Note " << (int)data1 << ", Velocity " << (int)data2 << std::endl;
            }
        }
    }
}

int main() {
    try {
        RtMidiIn *midiin = new RtMidiIn();
        
        // Check available ports
        unsigned int nPorts = midiin->getPortCount();
        std::cout << "Available MIDI input ports (" << nPorts << "):" << std::endl;
        
        int qtpyPort = -1;
        for (unsigned int i = 0; i < nPorts; i++) {
            std::string portName = midiin->getPortName(i);
            std::cout << "  Port " << i << ": " << portName << std::endl;
            if (portName.find("QT Py") != std::string::npos) {
                qtpyPort = i;
            }
        }
        
        if (qtpyPort == -1) {
            std::cout << "QT-PY device not found!" << std::endl;
            return 1;
        }
        
        // Open the QT-PY port
        midiin->openPort(qtpyPort);
        std::cout << "\nOpened QT-PY port " << qtpyPort << std::endl;
        std::cout << "Listening for MIDI from your 4 encoders..." << std::endl;
        std::cout << "Press Ctrl+C to exit\n" << std::endl;
        
        // Set callback function
        midiin->setCallback(&midiCallback);
        
        // Don't ignore sysex, timing, or active sensing messages
        midiin->ignoreTypes(false, false, false);
        
        // Keep listening
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        delete midiin;
    } catch (RtMidiError &error) {
        std::cout << "RtMidi error: " << error.getMessage() << std::endl;
    }
    
    return 0;
}