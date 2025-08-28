#include "src/velocity/VelocityVolumeControl.h"
#include <iostream>
#include <cassert>

int main() {
    std::cout << "=== VelocityVolumeControl Debug ===\n\n";
    
    VelocityVolumeControl volumeControl;
    
    // Debug the voice management issue
    uint32_t voice1 = 1000;
    
    volumeControl.addVoice(voice1, 32);  // Low velocity
    float vol1 = volumeControl.getVoiceVolume(voice1);
    std::cout << "Initial volume with velocity 32: " << vol1 << "\n";
    
    // Test velocity update
    volumeControl.updateVoiceVelocity(voice1, 100);
    float newVol1 = volumeControl.getVoiceVolume(voice1);
    std::cout << "Updated volume with velocity 100: " << newVol1 << "\n";
    
    // Test direct volume calculations
    auto config = volumeControl.getGlobalVolumeConfig();
    float directVol32 = volumeControl.calculateDirectVolume(32, config);
    float directVol100 = volumeControl.calculateDirectVolume(100, config);
    
    std::cout << "Direct volume calculation for velocity 32: " << directVol32 << "\n";
    std::cout << "Direct volume calculation for velocity 100: " << directVol100 << "\n";
    
    // Check configuration
    std::cout << "Global config - enableVelocityToVolume: " << config.enableVelocityToVolume << "\n";
    std::cout << "Global config - volumeMin: " << config.volumeMin << "\n";
    std::cout << "Global config - volumeMax: " << config.volumeMax << "\n";
    
    return 0;
}