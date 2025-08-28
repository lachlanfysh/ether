#include "HardwareInterface.h"

#ifdef PLATFORM_MAC
#include "MacHardware.h"
#endif

std::unique_ptr<HardwareInterface> createHardwareInterface() {
#ifdef PLATFORM_MAC
    return std::make_unique<MacHardware>();
#else
    return nullptr; // Will implement other platforms later
#endif
}

HardwareCapabilities getHardwareCapabilities() {
    HardwareCapabilities caps;
    
#ifdef PLATFORM_MAC
    // Mac prototype capabilities
    caps.hasPolyAftertouch = true;   // Simulated via MIDI
    caps.hasHapticFeedback = false;  // Limited trackpad haptics
    caps.hasMotorizedKnob = false;   // Simulated via mouse wheel
    caps.hasRGBLEDs = false;         // Simulated via console output
    caps.hasOLEDDisplays = false;    // Simulated via console output
    caps.hasBatteryMonitoring = false; // Always shows "charging"
    caps.hasMIDI = true;             // Core MIDI support
    caps.hasFileSystem = true;       // macOS file system
    caps.maxPolyphony = 32;          // No hardware limitations
    caps.numEncoders = 4;
    caps.numKeys = 26;
#else
    // Default/unknown platform
    caps = {};
#endif
    
    return caps;
}