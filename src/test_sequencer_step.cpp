#include <iostream>
#include <cmath>
#include "sequencer/SequencerStep.h"
#include "sequencer/SequencerPattern.h"

int main() {
    std::cout << "EtherSynth Sequencer Step Test\n";
    std::cout << "==============================\n";
    
    bool allTestsPassed = true;
    
    // Test basic SequencerStep creation
    std::cout << "Testing SequencerStep creation... ";
    try {
        SequencerStep step;
        
        if (step.getNote() == 60 &&           // C4 default
            step.getVelocity() == 100 &&      // Default velocity
            !step.isEnabled() &&               // Not enabled by default (empty step)
            !step.isAccent() &&                // No accent by default
            !step.isSlide()) {                 // No slide by default
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (wrong default values)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test step parameter setting
    std::cout << "Testing step parameter setting... ";
    try {
        SequencerStep step;
        
        step.setNote(48);        // C3
        step.setVelocity(127);   // Max velocity
        step.setSlideTime(50);   // 50ms slide
        step.setAccentAmount(100); // High accent
        step.setAccent(true);
        step.setSlide(true);
        
        if (step.getNote() == 48 &&
            step.getVelocity() == 127 &&
            step.getSlideTime() == 50 &&
            step.getAccentAmount() == 100 &&
            step.isAccent() &&
            step.isSlide()) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (parameter setting failed)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test slide time conversion
    std::cout << "Testing slide time conversion... ";
    try {
        SequencerStep step;
        
        step.setSlideTime(30);  // 30ms
        float timeSeconds = step.getSlideTimeSeconds();
        
        step.setSlideTimeSeconds(0.075f);  // 75ms
        uint8_t timeMs = step.getSlideTime();
        
        if (std::abs(timeSeconds - 0.03f) < 0.001f && timeMs == 75) {
            std::cout << "PASS (30ms = " << timeSeconds << "s, 75ms set correctly)\n";
        } else {
            std::cout << "FAIL (conversion error: " << timeSeconds << "s, " << timeMs << "ms)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test accent gain conversion
    std::cout << "Testing accent gain conversion... ";
    try {
        SequencerStep step;
        
        step.setAccentAmount(64);  // Half accent
        float gainDB = step.getAccentGainDB();
        float cutoffBoost = step.getAccentCutoffBoost();
        
        step.setAccentGainDB(6.0f);  // 6dB boost
        uint8_t newAmount = step.getAccentAmount();
        
        if (std::abs(gainDB - 4.0f) < 0.1f &&           // ~4dB at 50%
            std::abs(cutoffBoost - 0.125f) < 0.01f &&   // ~12.5% boost at 50%
            newAmount > 90 && newAmount < 100) {         // ~95 for 6dB
            std::cout << "PASS (64 amount: " << gainDB << "dB, " << cutoffBoost*100 << "% boost)\n";
        } else {
            std::cout << "FAIL (accent conversion error)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test step flags
    std::cout << "Testing step flags... ";
    try {
        SequencerStep step;
        
        step.setEnabled(true);    // Enable first
        step.setMute(true);
        step.setTie(true);
        step.setVelocityLatch(true);
        
        if (step.isMute() && step.isTie() && step.isVelocityLatch() && 
            step.isEnabled() && !step.isActive()) {  // Enabled but muted = not active
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (flag management error)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test step serialization
    std::cout << "Testing step serialization... ";
    try {
        SequencerStep step1;
        step1.setNote(72);        // C5
        step1.setVelocity(110);
        step1.setSlideTime(25);
        step1.setAccentAmount(80);
        step1.setAccent(true);
        step1.setSlide(true);
        step1.setProbability(90);
        step1.setMicroTiming(10);
        
        uint64_t serialized = step1.serialize();
        
        SequencerStep step2;
        step2.deserialize(serialized);
        
        if (step1 == step2) {
            std::cout << "PASS (serialization roundtrip successful)\n";
        } else {
            std::cout << "FAIL (serialization roundtrip failed)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test SequencerPattern creation
    std::cout << "Testing SequencerPattern creation... ";
    try {
        SequencerPattern pattern(16, 4);  // 16 steps, 4 tracks
        
        if (pattern.getLength() == 16 &&
            pattern.getNumTracks() == 4 &&
            pattern.isEmpty()) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (pattern creation failed: length=" << pattern.getLength() 
                      << ", tracks=" << pattern.getNumTracks() << ", empty=" << pattern.isEmpty() << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test pattern step operations
    std::cout << "Testing pattern step operations... ";
    try {
        SequencerPattern pattern(8, 2);
        
        // Set some steps
        pattern.setStepNote(0, 0, 60, 120);  // Track 0, Step 0: C4, vel 120
        pattern.setStepNote(0, 4, 64, 100);  // Track 0, Step 4: E4, vel 100
        pattern.setStepAccent(1, 2, true, 90); // Track 1, Step 2: Accent
        pattern.setStepSlide(0, 3, true, 40);   // Track 0, Step 3: Slide
        
        // Check step data
        const SequencerStep* step0 = pattern.getStep(0, 0);
        const SequencerStep* step4 = pattern.getStep(0, 4);
        const SequencerStep* step2 = pattern.getStep(1, 2);
        const SequencerStep* step3 = pattern.getStep(0, 3);
        
        if (step0 && step0->getNote() == 60 && step0->getVelocity() == 120 &&
            step4 && step4->getNote() == 64 && step4->getVelocity() == 100 &&
            step2 && step2->isAccent() && step2->getAccentAmount() == 90 &&
            step3 && step3->isSlide() && step3->getSlideTime() == 40) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (step operations failed)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test track configuration
    std::cout << "Testing track configuration... ";
    try {
        SequencerPattern pattern(16, 3);
        
        pattern.setTrackType(0, SequencerPattern::TrackType::MONO_SYNTH);
        pattern.setTrackType(1, SequencerPattern::TrackType::DRUM);
        pattern.setTrackLevel(0, 0.9f);
        pattern.setTrackMute(1, true);
        pattern.setTrackSolo(2, true);
        pattern.setTrackTranspose(0, -12);  // Down octave
        
        if (pattern.getTrackConfig(0).type == SequencerPattern::TrackType::MONO_SYNTH &&
            pattern.getTrackConfig(1).type == SequencerPattern::TrackType::DRUM &&
            std::abs(pattern.getTrackLevel(0) - 0.9f) < 0.01f &&
            pattern.isTrackMuted(1) &&
            pattern.isTrackSolo(2) &&
            !pattern.isTrackAudible(0) &&  // Not audible due to track 2 solo
            pattern.isTrackAudible(2) &&   // Solo track is audible
            pattern.getTrackConfig(0).transpose == -12) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (track configuration failed)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test pattern operations
    std::cout << "Testing pattern operations... ";
    try {
        SequencerPattern pattern(8, 2);
        
        // Add some steps
        pattern.setStepNote(0, 0, 60, 100);
        pattern.setStepNote(0, 2, 64, 110);
        pattern.setStepNote(0, 4, 67, 105);
        pattern.setStepAccent(0, 2, true, 80);
        
        int initialSteps = pattern.countActiveSteps(0);
        int accentSteps = pattern.countAccentSteps(0);
        
        // Shift pattern
        pattern.shiftTrack(0, 2);
        
        // Check if steps shifted correctly
        const SequencerStep* newStep2 = pattern.getStep(0, 2);
        const SequencerStep* newStep4 = pattern.getStep(0, 4);
        
        if (initialSteps == 3 && accentSteps == 1 &&
            newStep2 && newStep2->getNote() == 60 &&    // Original step 0 -> step 2
            newStep4 && newStep4->isAccent()) {          // Original step 2 -> step 4
            std::cout << "PASS (initial: " << initialSteps << " steps, " << accentSteps << " accents)\n";
        } else {
            std::cout << "FAIL (pattern operations failed)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test pattern validation
    std::cout << "Testing pattern validation... ";
    try {
        SequencerPattern pattern(4, 2);
        
        // Test valid positions
        bool valid1 = pattern.isValidPosition(0, 0);
        bool valid2 = pattern.isValidPosition(1, 3);
        
        // Test invalid positions
        bool invalid1 = pattern.isValidPosition(-1, 0);
        bool invalid2 = pattern.isValidPosition(0, 4);
        bool invalid3 = pattern.isValidPosition(2, 0);
        
        if (valid1 && valid2 && !invalid1 && !invalid2 && !invalid3) {
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (validation logic error)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test slide time clamping
    std::cout << "Testing slide time clamping... ";
    try {
        SequencerStep step;
        
        step.setSlideTime(3);    // Below minimum (5ms)
        uint8_t clampedMin = step.getSlideTime();
        
        step.setSlideTime(150);  // Above maximum (120ms)
        uint8_t clampedMax = step.getSlideTime();
        
        step.setSlideTime(80);   // Valid value
        uint8_t validTime = step.getSlideTime();
        
        if (clampedMin == SequencerStep::MIN_SLIDE_TIME_MS &&
            clampedMax == SequencerStep::MAX_SLIDE_TIME_MS &&
            validTime == 80) {
            std::cout << "PASS (min: " << (int)clampedMin << "ms, max: " << (int)clampedMax << "ms)\n";
        } else {
            std::cout << "FAIL (clamping failed: " << (int)clampedMin << ", " << (int)clampedMax << ", " << (int)validTime << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL SEQUENCER STEP TESTS PASSED!\n";
        std::cout << "Enhanced step sequencing with slide timing and accent flags is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}