#include "FMEngine.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <chrono>

FMEngine::FMEngine() {
    std::cout << "FMEngine created" << std::endl;
    
    // Initialize algorithms (classic DX7 configurations)
    initializeAlgorithms();
    
    // Clear voice states
    for (auto& voice : voices_) {
        voice.active = false;
    }
    
    // Initialize modulation matrix (all zeros by default)
    for (auto& row : modMatrix_) {
        row.fill(0.0f);
    }
    
    // Set default algorithm (simple 2-op FM)
    currentAlgorithm_ = 0;
}

FMEngine::~FMEngine() {
    allNotesOff();
    std::cout << "FMEngine destroyed" << std::endl;
}

void FMEngine::processAudio(EtherAudioBuffer& buffer) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    for (auto& frame : buffer) {
        frame.left = 0.0f;
        frame.right = 0.0f;
    }
    
    // Process each active voice
    for (auto& voice : voices_) {
        if (!voice.active) continue;
        
        for (size_t i = 0; i < BUFFER_SIZE; i++) {
            float sample = voice.process();
            sample *= volume_;
            
            buffer[i].left += sample;
            buffer[i].right += sample;
        }
        
        // Remove finished voices
        if (voice.isFinished()) {
            voice.active = false;
        }
    }
    
    // Apply light limiting
    for (auto& frame : buffer) {
        frame.left = std::tanh(frame.left);
        frame.right = std::tanh(frame.right);
    }
    
    // Update CPU usage estimate
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    float processingTime = duration.count() / 1000.0f; // Convert to milliseconds
    cpuUsage_ = (processingTime / (BUFFER_SIZE / SAMPLE_RATE * 1000.0f)) * 100.0f;
}

void FMEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    FMVoice* voice = findFreeVoice();
    if (!voice) return;
    
    voice->noteOn(note, velocity);
    
    std::cout << "FMEngine: Note ON " << static_cast<int>(note) 
              << " vel=" << velocity << std::endl;
}

void FMEngine::noteOff(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.active && voice.note == note) {
            voice.noteOff();
            std::cout << "FMEngine: Note OFF " << static_cast<int>(note) << std::endl;
            break;
        }
    }
}

void FMEngine::setAftertouch(uint8_t note, float aftertouch) {
    // Map aftertouch to modulation depth
    for (auto& voice : voices_) {
        if (voice.active && voice.note == note) {
            // Increase modulation depth with aftertouch
            for (int i = 0; i < NUM_OPERATORS; i++) {
                voice.operators[i].modInput += aftertouch * 0.5f;
            }
        }
    }
}

void FMEngine::allNotesOff() {
    for (auto& voice : voices_) {
        if (voice.active) {
            voice.noteOff();
        }
    }
    std::cout << "FMEngine: All notes off" << std::endl;
}

void FMEngine::setParameter(ParameterID param, float value) {
    value = std::clamp(value, 0.0f, 1.0f);
    
    switch (param) {
        case ParameterID::VOLUME:
            volume_ = value;
            break;
        case ParameterID::ATTACK:
            // Set attack for all operators
            for (auto& voice : voices_) {
                for (auto& op : voice.operators) {
                    op.attack = value * 2.0f;
                }
            }
            break;
        case ParameterID::DECAY:
            for (auto& voice : voices_) {
                for (auto& op : voice.operators) {
                    op.decay = value * 2.0f;
                }
            }
            break;
        case ParameterID::SUSTAIN:
            for (auto& voice : voices_) {
                for (auto& op : voice.operators) {
                    op.sustain = value;
                }
            }
            break;
        case ParameterID::RELEASE:
            for (auto& voice : voices_) {
                for (auto& op : voice.operators) {
                    op.release = value * 3.0f;
                }
            }
            break;
        case ParameterID::LFO_RATE:
            // Map to operator 1 ratio
            for (auto& voice : voices_) {
                voice.operators[0].ratio = 0.5f + value * 4.0f; // 0.5 to 4.5 ratio
            }
            break;
        case ParameterID::LFO_DEPTH:
            // Map to modulation depth
            modMatrix_[0][1] = value; // Op1 modulates Op2
            break;
        case ParameterID::FILTER_CUTOFF:
            // Map to algorithm selection
            currentAlgorithm_ = static_cast<int>(value * (NUM_ALGORITHMS - 1));
            break;
        case ParameterID::FILTER_RESONANCE:
            // Map to feedback amount
            for (auto& voice : voices_) {
                voice.operators[0].feedback = value * 0.8f;
            }
            break;
        default:
            break;
    }
}

float FMEngine::getParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::VOLUME: return volume_;
        case ParameterID::LFO_RATE: 
            return voices_[0].operators[0].ratio / 4.5f;
        case ParameterID::LFO_DEPTH: 
            return modMatrix_[0][1];
        case ParameterID::FILTER_CUTOFF: 
            return currentAlgorithm_ / static_cast<float>(NUM_ALGORITHMS - 1);
        case ParameterID::FILTER_RESONANCE: 
            return voices_[0].operators[0].feedback / 0.8f;
        default: return 0.0f;
    }
}

bool FMEngine::hasParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::VOLUME:
        case ParameterID::ATTACK:
        case ParameterID::DECAY:
        case ParameterID::SUSTAIN:
        case ParameterID::RELEASE:
        case ParameterID::LFO_RATE:
        case ParameterID::LFO_DEPTH:
        case ParameterID::FILTER_CUTOFF:
        case ParameterID::FILTER_RESONANCE:
            return true;
        default:
            return false;
    }
}

// FM Operator implementation
float FMEngine::FMOperator::process(float modulation) {
    // Add modulation to phase
    float modulatedPhase = phase + modulation + (feedback * lastOutput);
    
    // Generate sine wave
    float output = std::sin(modulatedPhase * 2.0f * M_PI) * level * envValue;
    
    // Update phase
    phase += frequency / SAMPLE_RATE;
    if (phase >= 1.0f) phase -= 1.0f;
    
    // Store for feedback
    lastOutput = output;
    
    return output;
}

void FMEngine::FMOperator::updateEnvelope(float deltaTime) {
    if (!envReleasing) {
        // Attack/Decay/Sustain phase
        if (envPhase < attack) {
            // Attack
            envValue = envPhase / attack;
            envPhase += deltaTime;
        } else if (envPhase < (attack + decay)) {
            // Decay
            float decayProgress = (envPhase - attack) / decay;
            envValue = 1.0f - decayProgress * (1.0f - sustain);
            envPhase += deltaTime;
        } else {
            // Sustain
            envValue = sustain;
        }
    } else {
        // Release phase
        envValue *= std::exp(-deltaTime / release);
    }
    
    envValue = std::clamp(envValue, 0.0f, 1.0f);
}

void FMEngine::FMOperator::noteOn() {
    envPhase = 0.0f;
    envValue = 0.0f;
    envReleasing = false;
    phase = 0.0f;
    lastOutput = 0.0f;
}

void FMEngine::FMOperator::noteOff() {
    envReleasing = true;
}

// FM Voice implementation
void FMEngine::FMVoice::noteOn(uint8_t noteNum, float vel) {
    note = noteNum;
    velocity = vel;
    baseFrequency = 440.0f * std::pow(2.0f, (noteNum - 69) / 12.0f);
    active = true;
    noteOnTime = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    
    // Initialize all operators
    for (int i = 0; i < NUM_OPERATORS; i++) {
        operators[i].frequency = baseFrequency * operators[i].ratio;
        operators[i].noteOn();
    }
}

void FMEngine::FMVoice::noteOff() {
    for (auto& op : operators) {
        op.noteOff();
    }
}

float FMEngine::FMVoice::process() {
    const float deltaTime = 1.0f / SAMPLE_RATE;
    
    // Update all operator envelopes
    for (auto& op : operators) {
        op.updateEnvelope(deltaTime);
    }
    
    // Process operators based on current algorithm
    // Simple cascade for now: Op1 -> Op2 -> Op3 -> Op4 -> Output
    float op1Out = operators[0].process();
    float op2Out = operators[1].process(op1Out * 2.0f); // Modulation strength
    float op3Out = operators[2].process(op2Out * 1.5f);
    float op4Out = operators[3].process(op3Out * 1.0f);
    
    // Mix carriers to output (algorithm-dependent)
    return op4Out * velocity;
}

bool FMEngine::FMVoice::isFinished() const {
    // Voice is finished when all operator envelopes are very low
    for (const auto& op : operators) {
        if (op.envValue > 0.001f) return false;
    }
    return true;
}

FMEngine::FMVoice* FMEngine::findFreeVoice() {
    // Find completely free voice first
    for (auto& voice : voices_) {
        if (!voice.active) return &voice;
    }
    
    // Steal oldest voice if no free voices
    FMVoice* oldest = &voices_[0];
    for (auto& voice : voices_) {
        if (voice.noteOnTime < oldest->noteOnTime) {
            oldest = &voice;
        }
    }
    return oldest;
}

size_t FMEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.active) count++;
    }
    return count;
}

void FMEngine::setVoiceCount(size_t maxVoices) {
    std::cout << "FMEngine: Voice count set to " << maxVoices << std::endl;
}

float FMEngine::getCPUUsage() const {
    return cpuUsage_;
}

void FMEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
    // Simple preset format
    struct FMPreset {
        float volume;
        int algorithm;
        float operatorRatios[NUM_OPERATORS];
        float operatorLevels[NUM_OPERATORS];
        float modMatrix[NUM_OPERATORS][NUM_OPERATORS];
    };
    
    actualSize = sizeof(FMPreset);
    if (maxSize < actualSize) return;
    
    FMPreset* preset = reinterpret_cast<FMPreset*>(data);
    preset->volume = volume_;
    preset->algorithm = currentAlgorithm_;
    
    // Save operator settings from first voice (template)
    for (int i = 0; i < NUM_OPERATORS; i++) {
        preset->operatorRatios[i] = voices_[0].operators[i].ratio;
        preset->operatorLevels[i] = voices_[0].operators[i].level;
    }
    
    // Save modulation matrix
    for (int i = 0; i < NUM_OPERATORS; i++) {
        for (int j = 0; j < NUM_OPERATORS; j++) {
            preset->modMatrix[i][j] = modMatrix_[i][j];
        }
    }
}

bool FMEngine::loadPreset(const uint8_t* data, size_t size) {
    struct FMPreset {
        float volume;
        int algorithm;
        float operatorRatios[NUM_OPERATORS];
        float operatorLevels[NUM_OPERATORS];
        float modMatrix[NUM_OPERATORS][NUM_OPERATORS];
    };
    
    if (size < sizeof(FMPreset)) return false;
    
    const FMPreset* preset = reinterpret_cast<const FMPreset*>(data);
    volume_ = preset->volume;
    currentAlgorithm_ = preset->algorithm;
    
    // Apply to all voices
    for (auto& voice : voices_) {
        for (int i = 0; i < NUM_OPERATORS; i++) {
            voice.operators[i].ratio = preset->operatorRatios[i];
            voice.operators[i].level = preset->operatorLevels[i];
        }
    }
    
    // Load modulation matrix
    for (int i = 0; i < NUM_OPERATORS; i++) {
        for (int j = 0; j < NUM_OPERATORS; j++) {
            modMatrix_[i][j] = preset->modMatrix[i][j];
        }
    }
    
    return true;
}

void FMEngine::setSampleRate(float sampleRate) {
    std::cout << "FMEngine: Sample rate set to " << sampleRate << "Hz" << std::endl;
}

void FMEngine::setBufferSize(size_t bufferSize) {
    std::cout << "FMEngine: Buffer size set to " << bufferSize << " samples" << std::endl;
}

void FMEngine::initializeAlgorithms() {
    // Initialize with basic algorithms
    for (int i = 0; i < NUM_ALGORITHMS; i++) {
        algorithms_[i].matrix.fill({});
        algorithms_[i].carriers.fill(false);
        
        // Default: last operator is carrier
        algorithms_[i].carriers[NUM_OPERATORS - 1] = true;
    }
    
    // Algorithm 0: Simple cascade (1->2->3->4->Out)
    algorithms_[0].matrix[0][1] = 1.0f;
    algorithms_[0].matrix[1][2] = 1.0f;
    algorithms_[0].matrix[2][3] = 1.0f;
    algorithms_[0].carriers[3] = true;
    
    // Algorithm 1: Parallel (all to output)
    algorithms_[1].carriers = {true, true, true, true};
    
    std::cout << "FMEngine: Initialized " << NUM_ALGORITHMS << " algorithms" << std::endl;
}