#include <iostream>
#include <cmath>
#include "control/modulation/VelocityToVolumeHandler.h"

int main() {
    std::cout << "EtherSynth Velocity→Volume Handler Test\n";
    std::cout << "=======================================\n";
    
    bool allTestsPassed = true;
    
    // Test velocity-to-volume handler creation
    std::cout << "Testing VelocityToVolumeHandler creation... ";
    try {
        VelocityToVolumeHandler handler;
        
        if (handler.isEnabled() && 
            handler.getVelocityCurve() == VelocityToVolumeHandler::VelocityCurve::EXPONENTIAL &&
            std::abs(handler.getVelocityScale() - 1.0f) < 0.01f) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization issue)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test velocity curve types
    std::cout << "Testing velocity curve types... ";
    try {
        VelocityToVolumeHandler handler;
        
        float velocity = 0.5f;
        float linearVol = handler.applyVelocityCurve(velocity, VelocityToVolumeHandler::VelocityCurve::LINEAR);
        float expVol = handler.applyVelocityCurve(velocity, VelocityToVolumeHandler::VelocityCurve::EXPONENTIAL);
        float logVol = handler.applyVelocityCurve(velocity, VelocityToVolumeHandler::VelocityCurve::LOGARITHMIC);
        
        if (std::abs(linearVol - 0.5f) < 0.01f &&  // Linear should be direct
            expVol < linearVol &&                  // Exponential should be lower at 0.5
            logVol > linearVol) {                  // Logarithmic should be higher at 0.5
            std::cout << "PASS (linear: " << linearVol << ", exp: " << expVol << ", log: " << logVol << ")\n";
        } else {
            std::cout << "FAIL (curve calculations incorrect)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test velocity-to-volume calculation
    std::cout << "Testing velocity-to-volume calculation... ";
    try {
        VelocityToVolumeHandler handler;
        
        float lowVelVolume = handler.calculateVolumeFromVelocity(0.2f);
        float midVelVolume = handler.calculateVolumeFromVelocity(0.5f);
        float highVelVolume = handler.calculateVolumeFromVelocity(0.8f);
        
        if (lowVelVolume < midVelVolume &&
            midVelVolume < highVelVolume &&
            lowVelVolume >= 0.0f &&
            highVelVolume <= 1.0f) {
            std::cout << "PASS (volumes: " << lowVelVolume << " < " << midVelVolume << " < " << highVelVolume << ")\n";
        } else {
            std::cout << "FAIL (volume calculation not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test velocity-to-volume disable with compensation
    std::cout << "Testing velocity-to-volume disable with compensation... ";
    try {
        VelocityToVolumeHandler handler;
        
        float enabledVolume = handler.calculateVolumeFromVelocity(0.3f);
        handler.setEnabled(false);
        float disabledVolume = handler.calculateVolumeFromVelocity(0.3f);
        
        if (disabledVolume > enabledVolume &&  // Should be compensated when disabled
            disabledVolume <= 1.0f) {          // Should not exceed max volume
            std::cout << "PASS (enabled: " << enabledVolume << ", disabled: " << disabledVolume << ")\n";
        } else {
            std::cout << "FAIL (disable compensation not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test per-voice overrides
    std::cout << "Testing per-voice overrides... ";
    try {
        VelocityToVolumeHandler handler;
        const uint32_t VOICE_ID = 1;
        
        // Set voice-specific override
        VelocityToVolumeHandler::VoiceVolumeOverride override;
        override.hasOverride = true;
        override.enabledOverride = false;  // Disable velocity→volume for this voice
        override.scaleOverride = 2.0f;
        
        handler.setVoiceOverride(VOICE_ID, override);
        
        float globalVolume = handler.calculateVolumeFromVelocity(0.5f, 999);  // Different voice
        float overrideVolume = handler.calculateVolumeFromVelocity(0.5f, VOICE_ID);  // Voice with override
        
        if (handler.hasVoiceOverride(VOICE_ID) &&
            overrideVolume > globalVolume) {  // Override should give compensated volume
            std::cout << "PASS (global: " << globalVolume << ", override: " << overrideVolume << ")\n";
        } else {
            std::cout << "FAIL (voice override not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test custom velocity curve
    std::cout << "Testing custom velocity curve... ";
    try {
        VelocityToVolumeHandler handler;
        
        // Set custom curve points (S-shaped curve)
        std::vector<float> customCurve = {0.0f, 0.1f, 0.3f, 0.7f, 0.9f, 1.0f};
        handler.setCustomCurvePoints(customCurve);
        
        float customVol = handler.applyVelocityCurve(0.5f, VelocityToVolumeHandler::VelocityCurve::CUSTOM);
        
        if (customVol > 0.0f && customVol < 1.0f) {  // Should interpolate correctly
            std::cout << "PASS (custom curve volume: " << customVol << ")\n";
        } else {
            std::cout << "FAIL (custom curve not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test volume range limiting
    std::cout << "Testing volume range limiting... ";
    try {
        VelocityToVolumeHandler handler;
        
        // Set restricted volume range
        handler.setVolumeRange(0.3f, 0.8f);
        
        float lowVolume = handler.calculateVolumeFromVelocity(0.0f);
        float highVolume = handler.calculateVolumeFromVelocity(1.0f);
        
        if (lowVolume >= 0.25f &&  // Should be at minimum range
            highVolume <= 0.85f) { // Should be at maximum range
            std::cout << "PASS (limited range: " << lowVolume << " to " << highVolume << ")\n";
        } else {
            std::cout << "FAIL (volume range limiting not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL VELOCITY→VOLUME HANDLER TESTS PASSED!\n";
        std::cout << "Special case velocity→volume handling with disable option is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}