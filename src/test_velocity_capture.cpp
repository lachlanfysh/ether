#include <iostream>
#include <thread>
#include <chrono>
#include "sequencer/VelocityCapture.h"

int main() {
    std::cout << "EtherSynth Velocity Capture Test\n";
    std::cout << "================================\n";
    
    bool allTestsPassed = true;
    
    // Test basic velocity capture creation
    std::cout << "Testing VelocityCapture creation... ";
    try {
        VelocityCapture capture;
        
        if (capture.getConfig().primarySource == VelocityCapture::VelocitySource::HALL_EFFECT_KEYS &&
            capture.getConfig().minVelocity == 10 &&
            capture.getConfig().maxVelocity == 127 &&
            !capture.isCapturing()) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (wrong default configuration)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test SmartKnob velocity capture
    std::cout << "Testing SmartKnob velocity capture... ";
    try {
        VelocityCapture capture;
        
        // Simulate slow knob turn
        capture.updateSmartKnobVelocity(2.0f);  // 2 rad/s
        uint8_t slowVelocity = capture.captureVelocityFromSource(VelocityCapture::VelocitySource::SMARTKNOB_TURN);
        
        // Simulate fast knob turn
        capture.updateSmartKnobVelocity(8.0f);  // 8 rad/s
        uint8_t fastVelocity = capture.captureVelocityFromSource(VelocityCapture::VelocitySource::SMARTKNOB_TURN);
        
        if (fastVelocity > slowVelocity && 
            slowVelocity >= 10 && fastVelocity <= 127) {
            std::cout << "PASS (slow: " << (int)slowVelocity << ", fast: " << (int)fastVelocity << ")\n";
        } else {
            std::cout << "FAIL (velocity scaling issue: slow=" << (int)slowVelocity << ", fast=" << (int)fastVelocity << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test touch pressure capture
    std::cout << "Testing touch pressure capture... ";
    try {
        VelocityCapture capture;
        
        // Light touch
        capture.updateTouchPressure(0.3f, true);
        uint8_t lightTouch = capture.captureVelocityFromSource(VelocityCapture::VelocitySource::TOUCH_PRESSURE);
        
        // Heavy touch
        capture.updateTouchPressure(0.9f, true);
        uint8_t heavyTouch = capture.captureVelocityFromSource(VelocityCapture::VelocitySource::TOUCH_PRESSURE);
        
        // No touch
        capture.updateTouchPressure(0.5f, false);
        uint8_t noTouch = capture.captureVelocityFromSource(VelocityCapture::VelocitySource::TOUCH_PRESSURE);
        
        if (heavyTouch > lightTouch && lightTouch > noTouch) {
            std::cout << "PASS (light: " << (int)lightTouch << ", heavy: " << (int)heavyTouch << ", none: " << (int)noTouch << ")\n";
        } else {
            std::cout << "FAIL (touch pressure not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test MIDI velocity pass-through
    std::cout << "Testing MIDI velocity pass-through... ";
    try {
        VelocityCapture capture;
        
        capture.updateMidiVelocity(64);  // Mid velocity
        uint8_t midVel = capture.captureVelocityFromSource(VelocityCapture::VelocitySource::MIDI_INPUT);
        
        capture.updateMidiVelocity(127); // Max velocity
        uint8_t maxVel = capture.captureVelocityFromSource(VelocityCapture::VelocitySource::MIDI_INPUT);
        
        capture.updateMidiVelocity(10);  // Low velocity
        uint8_t lowVel = capture.captureVelocityFromSource(VelocityCapture::VelocitySource::MIDI_INPUT);
        
        if (midVel == 64 && maxVel == 127 && lowVel == 10) {
            std::cout << "PASS (MIDI values preserved)\n";
        } else {
            std::cout << "FAIL (MIDI not preserved: " << (int)midVel << ", " << (int)maxVel << ", " << (int)lowVel << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test velocity curve application
    std::cout << "Testing velocity curve application... ";
    try {
        VelocityCapture capture;
        
        // Test linear curve (1.0)
        float linear = VelocityCapture::applyCurve(0.5f, 1.0f);
        
        // Test exponential curve (0.5)
        float exponential = VelocityCapture::applyCurve(0.5f, 0.5f);
        
        // Test logarithmic curve (2.0)
        float logarithmic = VelocityCapture::applyCurve(0.5f, 2.0f);
        
        if (std::abs(linear - 0.5f) < 0.01f &&      // Linear should be unchanged
            exponential > 0.6f &&                   // Exponential (curve < 1.0) should be higher
            logarithmic > 0.6f) {                   // Logarithmic (curve > 1.0) should also be higher 
            std::cout << "PASS (linear: " << linear << ", exp: " << exponential << ", log: " << logarithmic << ")\n";
        } else {
            std::cout << "FAIL (curve application incorrect: linear=" << linear << ", exp=" << exponential << ", log=" << logarithmic << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test velocity range scaling
    std::cout << "Testing velocity range scaling... ";
    try {
        uint8_t scaled50 = VelocityCapture::scaleToVelocityRange(0.5f, 20, 120);
        uint8_t scaled0 = VelocityCapture::scaleToVelocityRange(0.0f, 20, 120);
        uint8_t scaled100 = VelocityCapture::scaleToVelocityRange(1.0f, 20, 120);
        
        if (scaled50 == 70 &&      // 20 + 0.5 * (120-20) = 70
            scaled0 == 20 &&       // Min value
            scaled100 == 120) {    // Max value
            std::cout << "PASS (0%: " << (int)scaled0 << ", 50%: " << (int)scaled50 << ", 100%: " << (int)scaled100 << ")\n";
        } else {
            std::cout << "FAIL (scaling incorrect: " << (int)scaled0 << ", " << (int)scaled50 << ", " << (int)scaled100 << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test source priority selection
    std::cout << "Testing source priority selection... ";
    try {
        VelocityCapture capture;
        
        // Set MIDI as primary source
        VelocityCapture::CaptureConfig config = capture.getConfig();
        config.primarySource = VelocityCapture::VelocitySource::MIDI_INPUT;
        capture.setConfig(config);
        
        // Activate multiple sources
        capture.updateMidiVelocity(80);
        capture.updateSmartKnobVelocity(5.0f);
        capture.updateTouchPressure(0.7f, true);
        
        // Primary source should be selected
        VelocityCapture::VelocitySource activeSource = capture.getActiveSource();
        uint8_t capturedVel = capture.captureVelocity();
        
        if (activeSource == VelocityCapture::VelocitySource::MIDI_INPUT && capturedVel == 80) {
            std::cout << "PASS (primary source selected, velocity: " << (int)capturedVel << ")\n";
        } else {
            std::cout << "FAIL (source priority not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test velocity history and averaging
    std::cout << "Testing velocity history and averaging... ";
    try {
        VelocityCapture capture;
        
        // Configure for history tracking
        VelocityCapture::CaptureConfig config = capture.getConfig();
        config.historyLength = 4;
        capture.setConfig(config);
        
        // Capture several velocities
        capture.updateMidiVelocity(60);
        capture.captureVelocity();
        
        capture.updateMidiVelocity(80);
        capture.captureVelocity();
        
        capture.updateMidiVelocity(100);
        capture.captureVelocity();
        
        capture.updateMidiVelocity(120);
        capture.captureVelocity();
        
        uint8_t lastVel = capture.getLastVelocity();
        uint8_t avgVel = capture.getAverageVelocity();
        size_t historySize = capture.getVelocityHistory().size();
        
        if (lastVel == 120 && avgVel == 90 && historySize == 4) {  // (60+80+100+120)/4 = 90
            std::cout << "PASS (last: " << (int)lastVel << ", avg: " << (int)avgVel << ", history: " << historySize << ")\n";
        } else {
            std::cout << "FAIL (history not working: last=" << (int)lastVel << ", avg=" << (int)avgVel << ", size=" << historySize << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test source activity timeout
    std::cout << "Testing source activity timeout... ";
    try {
        VelocityCapture capture;
        
        // Update SmartKnob velocity
        capture.updateSmartKnobVelocity(5.0f);
        bool activeImmediate = capture.isSourceActive(VelocityCapture::VelocitySource::SMARTKNOB_TURN);
        
        // Wait a short time (SmartKnob should still be active)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        bool activeShortDelay = capture.isSourceActive(VelocityCapture::VelocitySource::SMARTKNOB_TURN);
        
        // Simulate longer timeout (would need to wait 500ms+ for real timeout, but we can test the logic)
        capture.updateSmartKnobVelocity(0.0f);  // Zero velocity should make it inactive
        bool inactiveAfterZero = !capture.isSourceActive(VelocityCapture::VelocitySource::SMARTKNOB_TURN);
        
        if (activeImmediate && activeShortDelay && inactiveAfterZero) {
            std::cout << "PASS (activity tracking working)\n";
        } else {
            std::cout << "FAIL (activity timeout not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test configuration validation
    std::cout << "Testing configuration validation... ";
    try {
        VelocityCapture capture;
        VelocityCapture::CaptureConfig config;
        
        // Test invalid ranges
        config.minVelocity = 100;
        config.maxVelocity = 50;  // Min > Max (should be swapped)
        config.sensitivityScale = 5.0f;  // Too high (should be clamped)
        config.velocityCurve = -1.0f;    // Too low (should be clamped)
        
        capture.setConfig(config);
        const auto& validatedConfig = capture.getConfig();
        
        if (validatedConfig.minVelocity == 50 &&           // Should be swapped
            validatedConfig.maxVelocity == 100 &&          // Should be swapped
            validatedConfig.sensitivityScale == 2.0f &&    // Should be clamped to max
            validatedConfig.velocityCurve == 0.1f) {       // Should be clamped to min
            std::cout << "PASS (config validation working)\n";
        } else {
            std::cout << "FAIL (config validation not working)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL VELOCITY CAPTURE TESTS PASSED!\n";
        std::cout << "Real-time velocity capture system is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}