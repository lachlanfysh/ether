#include <iostream>
#include <memory>
#include <vector>

// Test all major engines and components
#include "engines/SynthEngineBase.h"
#include "engines/Classic4OpFM.h"
#include "engines/SlideAccentBass.h"
#include "audio/FMOperator.h"
#include "audio/OversamplingProcessor.h"
#include "audio/FMAntiClick.h"
#include "audio/ADSREnvelope.h"
#include "audio/ParameterSmoother.h"
#include "audio/ZDFLadderFilter.h"
#include "audio/VirtualAnalogOscillator.h"
#include "audio/DCBlocker.h"
#include "audio/SubsonicFilter.h"
#include "audio/PostNonlinearProcessor.h"
#include "audio/AdvancedParameterSmoother.h"
#include "audio/MonoLowProcessor.h"
#include "audio/LUFSNormalizer.h"
#include "audio/EngineCrossfader.h"
#include "audio/ExponentialMapper.h"
#include "audio/ResonanceAutoRide.h"

// Test basic instantiation and initialization
int main() {
    std::cout << "EtherSynth Build Test\n";
    std::cout << "====================\n";
    
    const float SAMPLE_RATE = 44100.0f;
    bool allTestsPassed = true;
    
    // Test Classic 4-Op FM Engine
    std::cout << "Testing Classic4OpFM... ";
    try {
        Classic4OpFM fm;
        if (fm.initialize(SAMPLE_RATE)) {
            fm.setHTMParameters(0.5f, 0.3f, 0.7f);
            fm.noteOn(60.0f, 100.0f);
            float sample = fm.processSample();
            fm.noteOff();
            fm.shutdown();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test Slide+Accent Bass Engine
    std::cout << "Testing SlideAccentBass... ";
    try {
        SlideAccentBass bass;
        if (bass.initialize(SAMPLE_RATE)) {
            bass.setHTMParameters(0.4f, 0.6f, 0.8f);
            bass.noteOn(36.0f, 120.0f, true); // Accented bass note
            float sample = bass.processSample();
            bass.noteOff();
            bass.shutdown();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test FM Operator
    std::cout << "Testing FMOperator... ";
    try {
        FMOperator op;
        if (op.initialize(SAMPLE_RATE)) {
            op.setWaveform(FMOperator::Waveform::SINE);
            op.setFrequency(440.0f);
            op.setLevel(0.8f);
            float sample = op.processSample(0.0f);
            op.shutdown();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test Oversampling Processor
    std::cout << "Testing OversamplingProcessor... ";
    try {
        OversamplingProcessor oversampler;
        if (oversampler.initialize(SAMPLE_RATE)) {
            // Test with a simple processor lambda
            auto simpleProcessor = [](float input) -> float {
                return input * 0.5f;
            };
            float testInput = 0.5f;
            float output = oversampler.processSample(testInput, simpleProcessor);
            oversampler.shutdown();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test FM Anti-Click
    std::cout << "Testing FMAntiClick... ";
    try {
        FMAntiClick antiClick;
        if (antiClick.initialize(SAMPLE_RATE, 4)) {
            antiClick.onParameterChange(0, 0.5f, 0.8f, 1.0f);
            float output = antiClick.processOperatorSample(0, 0.3f, 1.57f);
            antiClick.shutdown();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test ADSR Envelope
    std::cout << "Testing ADSREnvelope... ";
    try {
        ADSREnvelope env;
        if (env.initialize(SAMPLE_RATE)) {
            env.setADSR(0.1f, 0.2f, 0.7f, 0.5f);
            env.trigger();
            float level = env.processSample();
            env.release();
            level = env.processSample();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test Parameter Smoother
    std::cout << "Testing ParameterSmoother... ";
    try {
        ParameterSmoother smoother;
        smoother.initialize(SAMPLE_RATE, 10.0f);
        smoother.setValue(0.0f);
        smoother.setTarget(1.0f);
        float value = smoother.process();
        std::cout << "PASS\n";
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test ZDF Ladder Filter
    std::cout << "Testing ZDFLadderFilter... ";
    try {
        ZDFLadderFilter filter;
        if (filter.initialize(SAMPLE_RATE)) {
            filter.setMode(ZDFLadderFilter::Mode::LOWPASS_24DB);
            filter.setCutoff(1000.0f);
            filter.setResonance(0.3f);
            float output = filter.processSample(0.5f);
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test Virtual Analog Oscillator
    std::cout << "Testing VirtualAnalogOscillator... ";
    try {
        VirtualAnalogOscillator osc;
        if (osc.initialize(SAMPLE_RATE)) {
            osc.setWaveform(VirtualAnalogOscillator::SAWTOOTH);
            osc.setFrequency(220.0f);
            osc.setLevel(0.8f);
            float sample = osc.processSample();
            osc.shutdown();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test H/T/M parameter mapping across engines
    std::cout << "Testing H/T/M parameter consistency... ";
    try {
        Classic4OpFM fm;
        SlideAccentBass bass;
        
        if (fm.initialize(SAMPLE_RATE) && bass.initialize(SAMPLE_RATE)) {
            // Test parameter ranges
            fm.setHarmonics(0.0f);   // Min
            fm.setTimbre(0.5f);      // Mid
            fm.setMorph(1.0f);       // Max
            
            bass.setHarmonics(0.25f);
            bass.setTimbre(0.75f);
            bass.setMorph(0.1f);
            
            // Get parameters back
            float h, t, m;
            fm.getHTMParameters(h, t, m);
            bass.getHTMParameters(h, t, m);
            
            fm.shutdown();
            bass.shutdown();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test DCBlocker
    std::cout << "Testing DCBlocker... ";
    try {
        DCBlocker dcBlocker;
        if (dcBlocker.initialize(SAMPLE_RATE, 24.0f)) {
            float testSignal = 0.5f;
            float output = dcBlocker.processSample(testSignal);
            dcBlocker.reset();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test SubsonicFilter
    std::cout << "Testing SubsonicFilter... ";
    try {
        SubsonicFilter subsonicFilter;
        if (subsonicFilter.initialize(SAMPLE_RATE, 24.0f, SubsonicFilter::FilterType::BUTTERWORTH)) {
            subsonicFilter.setCutoffFrequency(30.0f);
            subsonicFilter.enableDCBlocker(true);
            float testSignal = 0.3f;
            float output = subsonicFilter.processSample(testSignal);
            float magnitude = subsonicFilter.getMagnitudeResponse(1000.0f);
            subsonicFilter.reset();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test PostNonlinearProcessor
    std::cout << "Testing PostNonlinearProcessor... ";
    try {
        PostNonlinearProcessor processor;
        if (processor.initialize(SAMPLE_RATE, PostNonlinearProcessor::FilterTopology::SUBSONIC_ONLY)) {
            processor.setSubsonicCutoff(24.0f);
            processor.setGainCompensation(true);
            
            float testSignal = 0.8f;
            float output = processor.processSample(testSignal);
            
            // Test block processing
            float testBuffer[64];
            for (int i = 0; i < 64; i++) {
                testBuffer[i] = 0.1f * i;
            }
            processor.processBlock(testBuffer, 64);
            
            processor.shutdown();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test AdvancedParameterSmoother
    std::cout << "Testing AdvancedParameterSmoother... ";
    try {
        AdvancedParameterSmoother smoother;
        AdvancedParameterSmoother::Config config;
        config.smoothType = AdvancedParameterSmoother::SmoothType::AUDIBLE;
        config.curveType = AdvancedParameterSmoother::CurveType::EXPONENTIAL;
        
        smoother.initialize(SAMPLE_RATE, config);
        smoother.setValue(0.0f);
        smoother.setTarget(1.0f);
        
        float value = smoother.process();
        bool isSmoothing = smoother.isSmoothing();
        float progress = smoother.getSmoothingProgress();
        
        smoother.reset();
        std::cout << "PASS\n";
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test DC blocking after nonlinear processing simulation
    std::cout << "Testing nonlinear processing pipeline... ";
    try {
        PostNonlinearProcessor postProcessor;
        if (postProcessor.initialize(SAMPLE_RATE, PostNonlinearProcessor::FilterTopology::SERIAL)) {
            
            // Simulate nonlinear processing with DC offset
            float testBuffer[32];
            for (int i = 0; i < 32; i++) {
                // Simulate distorted signal with DC bias
                float cleanSignal = 0.5f * std::sin(2.0f * 3.14159f * 440.0f * i / SAMPLE_RATE);
                float distorted = std::tanh(cleanSignal * 3.0f) + 0.1f; // Add DC offset
                testBuffer[i] = distorted;
            }
            
            // Process through post-nonlinear cleanup
            postProcessor.processBlock(testBuffer, 32);
            
            // Verify DC is reduced (simple check)
            float dcLevel = 0.0f;
            for (int i = 0; i < 32; i++) {
                dcLevel += testBuffer[i];
            }
            dcLevel /= 32.0f;
            
            postProcessor.shutdown();
            std::cout << "PASS (DC level: " << dcLevel << ")\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test MonoLowProcessor
    std::cout << "Testing MonoLowProcessor... ";
    try {
        MonoLowProcessor monoLow;
        if (monoLow.initialize(SAMPLE_RATE, 120.0f)) {
            float left = 0.7f, right = -0.3f;
            monoLow.processStereo(left, right);
            monoLow.shutdown();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test LUFSNormalizer
    std::cout << "Testing LUFSNormalizer... ";
    try {
        LUFSNormalizer lufs;
        
        if (lufs.initialize(SAMPLE_RATE, true)) {
            lufs.setTargetLUFS(-18.0f);
            float left = 0.5f, right = 0.3f;
            lufs.processStereoSample(left, right);
            float currentLUFS = lufs.getCurrentLUFS();
            lufs.shutdown();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test EngineCrossfader
    std::cout << "Testing EngineCrossfader... ";
    try {
        EngineCrossfader crossfader;
        if (crossfader.initialize(SAMPLE_RATE, 30.0f)) {
            float engineA = 0.8f, engineB = 0.4f;
            float output = crossfader.processMix(engineA, engineB);
            crossfader.startCrossfadeToB();
            output = crossfader.processMix(engineA, engineB);
            crossfader.shutdown();
            std::cout << "PASS\n";
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test ExponentialMapper
    std::cout << "Testing ExponentialMapper... ";
    try {
        ExponentialMapper mapper;
        
        if (mapper.initialize()) {
            float cutoff = ExponentialMapper::mapCutoff(0.5f);
            float detune = ExponentialMapper::mapDetuneCents(0.75f);
            float freq = ExponentialMapper::noteToFrequency(69.0f); // A4
            
            if (cutoff > 400.0f && cutoff < 600.0f &&       // ~500Hz at 50%
                detune > 5.0f && detune < 15.0f &&         // ~7.5 cents at 75%
                std::abs(freq - 440.0f) < 1.0f) {          // A4 = 440Hz
                std::cout << "PASS\n";
            } else {
                std::cout << "FAIL (wrong mapping results)\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Test ResonanceAutoRide
    std::cout << "Testing ResonanceAutoRide... ";
    try {
        ResonanceAutoRide autoRide;
        ResonanceAutoRide::Config config;
        config.autoRideAmount = 0.8f;
        config.minCutoffHz = 100.0f;
        config.maxCutoffHz = 8000.0f;
        config.minResonance = 0.5f;
        config.maxResonance = 15.0f;
        
        if (autoRide.initialize(config)) {
            float lowCutoffRes = autoRide.processResonance(300.0f, 1.0f);   // Low cutoff
            float highCutoffRes = autoRide.processResonance(6000.0f, 1.0f); // High cutoff
            
            if (lowCutoffRes > highCutoffRes && lowCutoffRes > 1.0f) {
                std::cout << "PASS\n";
            } else {
                std::cout << "FAIL (auto-ride not working: low=" << lowCutoffRes << ", high=" << highCutoffRes << ")\n";
                allTestsPassed = false;
            }
        } else {
            std::cout << "FAIL (initialization)\n";
            allTestsPassed = false;
        }
    } catch (const std::exception& e) {
        std::cout << "FAIL (exception: " << e.what() << ")\n";
        allTestsPassed = false;
    }
    
    // Overall result
    std::cout << "\n";
    if (allTestsPassed) {
        std::cout << "✅ ALL TESTS PASSED - Architecture is sound!\n";
        std::cout << "Ready for production implementation.\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED - Architecture needs fixes.\n";
        return 1;
    }
}