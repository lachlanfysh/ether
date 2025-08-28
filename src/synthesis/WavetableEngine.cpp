#include "WavetableEngine.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <iostream>
#include <chrono>

WavetableEngine::WavetableEngine() {
    std::cout << "WavetableEngine created" << std::endl;
    
    // Initialize wavetables with diverse content
    initializeWavetables();
    
    // Clear voice states
    for (auto& voice : voices_) {
        voice.active = false;
    }
}

WavetableEngine::~WavetableEngine() {
    allNotesOff();
    std::cout << "WavetableEngine destroyed" << std::endl;
}

void WavetableEngine::processAudio(EtherAudioBuffer& buffer) {
    for (auto& frame : buffer) {
        frame.left = 0.0f;
        frame.right = 0.0f;
    }
    
    // Process each active voice
    for (auto& voice : voices_) {
        if (!voice.active) continue;
        
        for (size_t i = 0; i < BUFFER_SIZE; i++) {
            // Update envelope
            updateEnvelope(voice, 1.0f / SAMPLE_RATE);
            
            // Generate wavetable sample
            float sample = interpolateWavetable(voice.phase, voice.wavetablePos, voice.morphAmount);
            sample *= voice.envValue * voice.velocity * volume_;
            
            // Apply to buffer
            buffer[i].left += sample;
            buffer[i].right += sample;
            
            // Advance phase
            voice.phase += voice.frequency * PHASE_INCREMENT;
            if (voice.phase >= 1.0f) voice.phase -= 1.0f;
        }
        
        // Remove finished voices
        if (voice.envReleasing && voice.envValue < ENV_MIN) {
            voice.active = false;
        }
    }
    
    // Apply master volume and light limiting
    for (auto& frame : buffer) {
        frame.left = std::tanh(frame.left);
        frame.right = std::tanh(frame.right);
    }
}

void WavetableEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    WavetableVoice* voice = findFreeVoice();
    if (!voice) return;
    
    voice->note = note;
    voice->frequency = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    voice->velocity = velocity;
    voice->amplitude = velocity;
    voice->phase = 0.0f;
    voice->wavetablePos = wavetablePosition_;
    voice->morphAmount = morphAmount_;
    voice->active = true;
    voice->envPhase = 0.0f;
    voice->envValue = 0.0f;
    voice->envReleasing = false;
    voice->noteOnTime = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    
    std::cout << "WavetableEngine: Note ON " << static_cast<int>(note) 
              << " freq=" << voice->frequency << " vel=" << velocity << std::endl;
}

void WavetableEngine::noteOff(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.active && voice.note == note && !voice.envReleasing) {
            voice.envReleasing = true;
            std::cout << "WavetableEngine: Note OFF " << static_cast<int>(note) << std::endl;
            break;
        }
    }
}

void WavetableEngine::setAftertouch(uint8_t note, float aftertouch) {
    for (auto& voice : voices_) {
        if (voice.active && voice.note == note) {
            // Map aftertouch to wavetable morphing
            voice.morphAmount = morphAmount_ + aftertouch * 0.3f;
            voice.morphAmount = std::clamp(voice.morphAmount, 0.0f, 1.0f);
        }
    }
}

void WavetableEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.active = false;
    }
    std::cout << "WavetableEngine: All notes off" << std::endl;
}

void WavetableEngine::setParameter(ParameterID param, float value) {
    value = std::clamp(value, 0.0f, 1.0f);
    
    switch (param) {
        case ParameterID::VOLUME:
            volume_ = value;
            break;
        case ParameterID::ATTACK:
            attack_ = value * 2.0f; // 0-2 seconds
            break;
        case ParameterID::DECAY:
            decay_ = value * 2.0f;
            break;
        case ParameterID::SUSTAIN:
            sustain_ = value;
            break;
        case ParameterID::RELEASE:
            release_ = value * 3.0f; // 0-3 seconds
            break;
        case ParameterID::FILTER_CUTOFF:
            filterCutoff_ = value;
            break;
        case ParameterID::FILTER_RESONANCE:
            filterResonance_ = value;
            break;
        case ParameterID::LFO_RATE:
            wavetablePosition_ = value;
            break;
        case ParameterID::LFO_DEPTH:
            morphAmount_ = value;
            break;
        default:
            break;
    }
}

float WavetableEngine::getParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::VOLUME: return volume_;
        case ParameterID::ATTACK: return attack_ / 2.0f;
        case ParameterID::DECAY: return decay_ / 2.0f;
        case ParameterID::SUSTAIN: return sustain_;
        case ParameterID::RELEASE: return release_ / 3.0f;
        case ParameterID::FILTER_CUTOFF: return filterCutoff_;
        case ParameterID::FILTER_RESONANCE: return filterResonance_;
        case ParameterID::LFO_RATE: return wavetablePosition_;
        case ParameterID::LFO_DEPTH: return morphAmount_;
        default: return 0.0f;
    }
}

bool WavetableEngine::hasParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::VOLUME:
        case ParameterID::ATTACK:
        case ParameterID::DECAY:
        case ParameterID::SUSTAIN:
        case ParameterID::RELEASE:
        case ParameterID::FILTER_CUTOFF:
        case ParameterID::FILTER_RESONANCE:
        case ParameterID::LFO_RATE:
        case ParameterID::LFO_DEPTH:
            return true;
        default:
            return false;
    }
}

void WavetableEngine::setWavetablePosition(float position) {
    wavetablePosition_ = std::clamp(position, 0.0f, 1.0f);
}

void WavetableEngine::setMorphAmount(float amount) {
    morphAmount_ = std::clamp(amount, 0.0f, 1.0f);
}

void WavetableEngine::setTouchPosition(float x, float y) {
    touchX_ = std::clamp(x, 0.0f, 1.0f);
    touchY_ = std::clamp(y, 0.0f, 1.0f);
    
    // Map touch position to wavetable parameters
    setWavetablePosition(touchX_);
    setMorphAmount(touchY_);
    
    // Update active voices
    for (auto& voice : voices_) {
        if (voice.active) {
            voice.wavetablePos = wavetablePosition_;
            voice.morphAmount = morphAmount_;
        }
    }
}

float WavetableEngine::interpolateWavetable(float phase, float tablePos, float morphAmount) {
    // Calculate which wavetables to interpolate between
    float scaledPos = tablePos * (NUM_WAVETABLES - 1);
    size_t table1 = static_cast<size_t>(scaledPos);
    size_t table2 = std::min(table1 + 1, NUM_WAVETABLES - 1);
    float tableMix = scaledPos - table1;
    
    // Get samples from both tables
    float sample1 = getWavetableSample(table1, phase);
    float sample2 = getWavetableSample(table2, phase);
    
    // Interpolate between tables
    float result = sample1 + tableMix * (sample2 - sample1);
    
    // Apply morphing (could be cross-fade with another set of tables)
    if (morphAmount > 0.0f) {
        // Simple morphing: apply spectral shifting
        float morphed = result * (1.0f - morphAmount) + 
                       std::sin(phase * 2.0f * M_PI * (1.0f + morphAmount)) * morphAmount;
        result = result * (1.0f - morphAmount) + morphed * morphAmount;
    }
    
    return result;
}

float WavetableEngine::getWavetableSample(size_t tableIndex, float phase) {
    if (tableIndex >= NUM_WAVETABLES) return 0.0f;
    
    // Linear interpolation within wavetable
    float scaledPhase = phase * (WAVETABLE_SIZE - 1);
    size_t index1 = static_cast<size_t>(scaledPhase);
    size_t index2 = (index1 + 1) % WAVETABLE_SIZE;
    float fraction = scaledPhase - index1;
    
    float sample1 = wavetables_[tableIndex][index1];
    float sample2 = wavetables_[tableIndex][index2];
    
    return sample1 + fraction * (sample2 - sample1);
}

void WavetableEngine::updateEnvelope(WavetableVoice& voice, float deltaTime) {
    if (!voice.envReleasing) {
        // Attack/Decay/Sustain phase
        if (voice.envPhase < attack_) {
            // Attack
            voice.envValue = (voice.envPhase / attack_);
            voice.envPhase += deltaTime;
        } else if (voice.envPhase < (attack_ + decay_)) {
            // Decay
            float decayProgress = (voice.envPhase - attack_) / decay_;
            voice.envValue = 1.0f - decayProgress * (1.0f - sustain_);
            voice.envPhase += deltaTime;
        } else {
            // Sustain
            voice.envValue = sustain_;
        }
    } else {
        // Release phase
        voice.envValue *= std::exp(-deltaTime / release_);
    }
    
    voice.envValue = std::clamp(voice.envValue, 0.0f, 1.0f);
}

WavetableEngine::WavetableVoice* WavetableEngine::findFreeVoice() {
    // Find completely free voice first
    for (auto& voice : voices_) {
        if (!voice.active) return &voice;
    }
    
    // Steal oldest voice if no free voices
    WavetableVoice* oldest = &voices_[0];
    for (auto& voice : voices_) {
        if (voice.noteOnTime < oldest->noteOnTime) {
            oldest = &voice;
        }
    }
    return oldest;
}

void WavetableEngine::initializeWavetables() {
    std::cout << "Generating wavetables..." << std::endl;
    
    // Generate basic waveforms first
    generateBasicWaveforms();
    
    // Generate spectral content
    generateSpectralWaveforms();
    
    std::cout << "Generated " << NUM_WAVETABLES << " wavetables" << std::endl;
}

size_t WavetableEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.active) count++;
    }
    return count;
}

void WavetableEngine::setVoiceCount(size_t maxVoices) {
    // For now, we use a fixed voice count, but this could be dynamic
    std::cout << "WavetableEngine: Voice count set to " << maxVoices << std::endl;
}

float WavetableEngine::getCPUUsage() const {
    // Simple CPU usage estimation based on active voices
    return (getActiveVoiceCount() / static_cast<float>(MAX_VOICES)) * 50.0f;
}

void WavetableEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
    // Simple preset format: basic parameters
    struct WavetablePreset {
        float wavetablePosition;
        float morphAmount;
        float volume;
        float attack, decay, sustain, release;
        float filterCutoff, filterResonance;
    };
    
    actualSize = sizeof(WavetablePreset);
    if (maxSize < actualSize) return;
    
    WavetablePreset* preset = reinterpret_cast<WavetablePreset*>(data);
    preset->wavetablePosition = wavetablePosition_;
    preset->morphAmount = morphAmount_;
    preset->volume = volume_;
    preset->attack = attack_;
    preset->decay = decay_;
    preset->sustain = sustain_;
    preset->release = release_;
    preset->filterCutoff = filterCutoff_;
    preset->filterResonance = filterResonance_;
}

bool WavetableEngine::loadPreset(const uint8_t* data, size_t size) {
    struct WavetablePreset {
        float wavetablePosition;
        float morphAmount;
        float volume;
        float attack, decay, sustain, release;
        float filterCutoff, filterResonance;
    };
    
    if (size < sizeof(WavetablePreset)) return false;
    
    const WavetablePreset* preset = reinterpret_cast<const WavetablePreset*>(data);
    wavetablePosition_ = preset->wavetablePosition;
    morphAmount_ = preset->morphAmount;
    volume_ = preset->volume;
    attack_ = preset->attack;
    decay_ = preset->decay;
    sustain_ = preset->sustain;
    release_ = preset->release;
    filterCutoff_ = preset->filterCutoff;
    filterResonance_ = preset->filterResonance;
    
    return true;
}

void WavetableEngine::setSampleRate(float sampleRate) {
    // Update internal sample rate dependent calculations
    std::cout << "WavetableEngine: Sample rate set to " << sampleRate << "Hz" << std::endl;
}

void WavetableEngine::setBufferSize(size_t bufferSize) {
    // Update buffer size dependent calculations
    std::cout << "WavetableEngine: Buffer size set to " << bufferSize << " samples" << std::endl;
}

void WavetableEngine::generateBasicWaveforms() {
    // Table 0: Pure sine
    WavetableUtils::generateSine(wavetables_[0]);
    
    // Table 1: Sawtooth
    WavetableUtils::generateSaw(wavetables_[1]);
    
    // Table 2: Square
    WavetableUtils::generateSquare(wavetables_[2]);
    
    // Table 3: Triangle
    WavetableUtils::generateTriangle(wavetables_[3]);
    
    // Tables 4-15: Harmonic series
    for (size_t i = 4; i < 16; i++) {
        WavetableUtils::generateHarmonic(wavetables_[i], i - 3, 1.0f / (i - 3));
    }
}

void WavetableEngine::generateSpectralWaveforms() {
    // Generate evolving harmonic content for smooth morphing
    for (size_t i = 16; i < NUM_WAVETABLES; i++) {
        float progress = static_cast<float>(i - 16) / (NUM_WAVETABLES - 16);
        
        // Create harmonic spectrum that evolves
        std::vector<float> harmonics;
        for (int h = 1; h <= 32; h++) {
            float amplitude = 1.0f / h; // Basic harmonic rolloff
            amplitude *= (1.0f - progress * 0.8f); // Reduce high harmonics over time
            amplitude *= std::sin(progress * M_PI); // Envelope over progression
            harmonics.push_back(amplitude);
        }
        
        WavetableUtils::generateSpectrum(wavetables_[i], harmonics);
    }
}

// Utility functions
namespace WavetableUtils {
    void generateSine(std::array<float, 2048>& table) {
        for (size_t i = 0; i < table.size(); i++) {
            float phase = static_cast<float>(i) / table.size();
            table[i] = std::sin(phase * 2.0f * M_PI);
        }
    }
    
    void generateSaw(std::array<float, 2048>& table) {
        for (size_t i = 0; i < table.size(); i++) {
            float phase = static_cast<float>(i) / table.size();
            table[i] = 2.0f * phase - 1.0f;
        }
    }
    
    void generateSquare(std::array<float, 2048>& table) {
        for (size_t i = 0; i < table.size(); i++) {
            float phase = static_cast<float>(i) / table.size();
            table[i] = (phase < 0.5f) ? 1.0f : -1.0f;
        }
    }
    
    void generateTriangle(std::array<float, 2048>& table) {
        for (size_t i = 0; i < table.size(); i++) {
            float phase = static_cast<float>(i) / table.size();
            if (phase < 0.5f) {
                table[i] = 4.0f * phase - 1.0f;
            } else {
                table[i] = 3.0f - 4.0f * phase;
            }
        }
    }
    
    void generateHarmonic(std::array<float, 2048>& table, int harmonic, float amplitude) {
        for (size_t i = 0; i < table.size(); i++) {
            float phase = static_cast<float>(i) / table.size();
            table[i] = amplitude * std::sin(phase * 2.0f * M_PI * harmonic);
        }
    }
    
    void generateSpectrum(std::array<float, 2048>& table, const std::vector<float>& harmonics) {
        // Clear table
        std::fill(table.begin(), table.end(), 0.0f);
        
        // Add each harmonic
        for (size_t h = 0; h < harmonics.size(); h++) {
            if (harmonics[h] == 0.0f) continue;
            
            for (size_t i = 0; i < table.size(); i++) {
                float phase = static_cast<float>(i) / table.size();
                table[i] += harmonics[h] * std::sin(phase * 2.0f * M_PI * (h + 1));
            }
        }
        
        // Normalize
        float maxAmp = 0.0f;
        for (float sample : table) {
            maxAmp = std::max(maxAmp, std::abs(sample));
        }
        
        if (maxAmp > 0.0f) {
            for (float& sample : table) {
                sample /= maxAmp;
            }
        }
    }
}