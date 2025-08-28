#include <iostream>
#include <cmath>
#include "audio/ExponentialMapper.h"

int main() {
    std::cout << "EtherSynth Exponential Mapper Test\n";
    std::cout << "==================================\n";
    
    bool allTestsPassed = true;
    
    // Test cutoff mapping (20Hz-12kHz)
    std::cout << "Testing cutoff mapping... ";
    try {
        float cutoff0 = ExponentialMapper::mapCutoff(0.0f);    // Should be ~20Hz
        float cutoff50 = ExponentialMapper::mapCutoff(0.5f);   // Should be ~500Hz
        float cutoff100 = ExponentialMapper::mapCutoff(1.0f);  // Should be ~12kHz
        
        if (cutoff0 >= 19.0f && cutoff0 <= 21.0f &&           // ~20Hz
            cutoff50 >= 400.0f && cutoff50 <= 600.0f &&       // ~500Hz (geometric mean)
            cutoff100 >= 11800.0f && cutoff100 <= 12200.0f) { // ~12kHz
            std::cout << "PASS (0%: " << cutoff0 << "Hz, 50%: " << cutoff50 << "Hz, 100%: " << cutoff100 << "Hz)\n";
        } else {
            std::cout << "FAIL (wrong cutoff range: " << cutoff0 << ", " << cutoff50 << ", " << cutoff100 << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test detune mapping (cents = x² × 30)
    std::cout << "Testing detune mapping... ";
    try {
        float detune0 = ExponentialMapper::mapDetuneCents(0.0f);   // Should be -30 cents
        float detune50 = ExponentialMapper::mapDetuneCents(0.5f);  // Should be 0 cents
        float detune75 = ExponentialMapper::mapDetuneCents(0.75f); // Should be ~7.5 cents
        float detune100 = ExponentialMapper::mapDetuneCents(1.0f); // Should be +30 cents
        
        if (std::abs(detune0 - (-30.0f)) < 0.1f &&     // -30 cents
            std::abs(detune50) < 0.1f &&               // 0 cents
            detune75 > 5.0f && detune75 < 10.0f &&     // ~7.5 cents
            std::abs(detune100 - 30.0f) < 0.1f) {      // +30 cents
            std::cout << "PASS (0%: " << detune0 << "¢, 50%: " << detune50 << "¢, 75%: " << detune75 << "¢, 100%: " << detune100 << "¢)\n";
        } else {
            std::cout << "FAIL (wrong detune mapping)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test inverse mapping (roundtrip)
    std::cout << "Testing inverse mapping roundtrip... ";
    try {
        float originalInput = 0.3f;
        float frequency = ExponentialMapper::mapCutoff(originalInput);
        float recoveredInput = ExponentialMapper::unmapCutoff(frequency);
        
        if (std::abs(recoveredInput - originalInput) < 0.01f) {
            std::cout << "PASS (input: " << originalInput << " → freq: " << frequency << "Hz → recovered: " << recoveredInput << ")\n";
        } else {
            std::cout << "FAIL (roundtrip error: " << originalInput << " → " << recoveredInput << ")\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test musical utility functions
    std::cout << "Testing musical utility functions... ";
    try {
        float a4Freq = ExponentialMapper::noteToFrequency(69.0f); // A4 = MIDI note 69
        float c4Freq = ExponentialMapper::noteToFrequency(60.0f); // C4 = MIDI note 60
        float a4Note = ExponentialMapper::frequencyToNote(440.0f);
        
        if (std::abs(a4Freq - 440.0f) < 0.1f &&                // A4 should be 440Hz
            c4Freq >= 260.0f && c4Freq <= 265.0f &&            // C4 should be ~262Hz
            std::abs(a4Note - 69.0f) < 0.01f) {                // 440Hz should be note 69
            std::cout << "PASS (A4: " << a4Freq << "Hz, C4: " << c4Freq << "Hz, 440Hz: note " << a4Note << ")\n";
        } else {
            std::cout << "FAIL (wrong musical conversions)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test cents and ratio conversion
    std::cout << "Testing cents and ratio conversion... ";
    try {
        float ratio100cents = ExponentialMapper::centsToRatio(100.0f); // 100 cents = 1 semitone
        float cents1200 = ExponentialMapper::ratioToCents(2.0f);       // 2:1 ratio = 1200 cents (octave)
        
        if (std::abs(ratio100cents - 1.059463f) < 0.001f &&    // 2^(100/1200) ≈ 1.059463
            std::abs(cents1200 - 1200.0f) < 0.1f) {            // 2:1 ratio = 1200 cents
            std::cout << "PASS (100¢: ratio " << ratio100cents << ", 2:1 ratio: " << cents1200 << "¢)\n";
        } else {
            std::cout << "FAIL (wrong cents/ratio conversion)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test resonance mapping
    std::cout << "Testing resonance mapping... ";
    try {
        float res0 = ExponentialMapper::mapResonance(0.0f);   // Should be min Q
        float res50 = ExponentialMapper::mapResonance(0.5f);  // Should be mid-range Q
        float res100 = ExponentialMapper::mapResonance(1.0f); // Should be max Q
        
        if (res0 >= 0.05f && res0 <= 0.15f &&      // Min Q ~0.1
            res50 >= 1.0f && res50 <= 5.0f &&      // Mid Q ~2-3
            res100 >= 45.0f && res100 <= 55.0f) {  // Max Q ~50
            std::cout << "PASS (0%: Q=" << res0 << ", 50%: Q=" << res50 << ", 100%: Q=" << res100 << ")\n";
        } else {
            std::cout << "FAIL (wrong resonance range)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test envelope time mapping
    std::cout << "Testing envelope time mapping... ";
    try {
        float time0 = ExponentialMapper::mapEnvelopeTime(0.0f);   // Should be ~0.1ms
        float time50 = ExponentialMapper::mapEnvelopeTime(0.5f);  // Should be ~10ms
        float time100 = ExponentialMapper::mapEnvelopeTime(1.0f); // Should be ~10s
        
        // The actual range should be 0.1ms to 10s, with 50% around ~10ms
        if (time0 >= 0.00009f && time0 <= 0.00011f &&    // ~0.1ms
            time50 >= 0.009f && time50 <= 0.032f &&       // ~10-30ms (geometric mean)
            time100 >= 9.0f && time100 <= 11.0f) {        // ~10s
            std::cout << "PASS (0%: " << time0*1000 << "ms, 50%: " << time50*1000 << "ms, 100%: " << time100 << "s)\n";
        } else {
            std::cout << "FAIL (envelope times: 0%=" << time0*1000 << "ms, 50%=" << time50*1000 << "ms, 100%=" << time100 << "s)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test custom exponential mapping
    std::cout << "Testing custom exponential mapping... ";
    try {
        float custom0 = ExponentialMapper::mapExponential(0.0f, 1.0f, 1000.0f);   // Should be 1.0
        float custom50 = ExponentialMapper::mapExponential(0.5f, 1.0f, 1000.0f);  // Should be ~31.6 (sqrt(1000))
        float custom100 = ExponentialMapper::mapExponential(1.0f, 1.0f, 1000.0f); // Should be 1000.0
        
        if (std::abs(custom0 - 1.0f) < 0.01f &&
            custom50 >= 30.0f && custom50 <= 35.0f &&   // ~31.6
            std::abs(custom100 - 1000.0f) < 0.1f) {
            std::cout << "PASS (0%: " << custom0 << ", 50%: " << custom50 << ", 100%: " << custom100 << ")\n";
        } else {
            std::cout << "FAIL (wrong custom mapping)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test power curve mapping
    std::cout << "Testing power curve mapping... ";
    try {
        float power0 = ExponentialMapper::mapPower(0.0f, 0.0f, 1.0f, 2.0f);   // x^2, should be 0
        float power50 = ExponentialMapper::mapPower(0.5f, 0.0f, 1.0f, 2.0f);  // x^2, should be 0.25
        float power100 = ExponentialMapper::mapPower(1.0f, 0.0f, 1.0f, 2.0f); // x^2, should be 1.0
        
        if (std::abs(power0) < 0.01f &&
            std::abs(power50 - 0.25f) < 0.01f &&
            std::abs(power100 - 1.0f) < 0.01f) {
            std::cout << "PASS (x^2: 0%: " << power0 << ", 50%: " << power50 << ", 100%: " << power100 << ")\n";
        } else {
            std::cout << "FAIL (wrong power curve)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL EXPONENTIAL MAPPER TESTS PASSED!\n";
        std::cout << "Perceptual parameter mapping system is working correctly.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}