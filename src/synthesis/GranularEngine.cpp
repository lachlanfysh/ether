#include "GranularEngine.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <chrono>

GranularEngine::GranularEngine() {
    std::cout << "GranularEngine created" << std::endl;
    
    // Initialize random number generator
    rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
    
    // Initialize source waveforms
    initializeSourceWaveforms();
    
    // Clear voice states
    for (auto& voice : voices_) {
        voice.active = false;
    }
    
    std::cout << "GranularEngine: Initialized with " << sourceWaveforms_.size() << " source waveforms" << std::endl;
}

GranularEngine::~GranularEngine() {
    allNotesOff();
    std::cout << "GranularEngine destroyed" << std::endl;
}

void GranularEngine::processAudio(EtherAudioBuffer& buffer) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    // Clear output buffer
    for (auto& frame : buffer) {
        frame.left = 0.0f;
        frame.right = 0.0f;
    }
    
    // Process each active voice
    for (auto& voice : voices_) {
        if (!voice.active) continue;
        
        for (size_t i = 0; i < BUFFER_SIZE; i++) {
            float sample = voice.process(this);
            sample *= volume_;
            
            // Apply stereo spread based on grainSpread_
            float left = sample * (1.0f - grainSpread_ * 0.5f);
            float right = sample * (1.0f + grainSpread_ * 0.5f);
            
            buffer[i].left += left;
            buffer[i].right += right;
        }
        
        // Remove finished voices (when all grains are silent)
        bool hasActiveGrains = false;
        for (const auto& grain : voice.grains) {
            if (grain.active) {
                hasActiveGrains = true;
                break;
            }
        }
        if (!hasActiveGrains) {
            voice.active = false;
        }
    }
    
    // Apply light compression/limiting
    for (auto& frame : buffer) {
        frame.left = std::tanh(frame.left);
        frame.right = std::tanh(frame.right);
    }
    
    // Update CPU usage estimate
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
    float processingTime = duration.count() / 1000.0f;
    cpuUsage_ = (processingTime / (BUFFER_SIZE / SAMPLE_RATE * 1000.0f)) * 100.0f;
}

void GranularEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    GranularVoice* voice = findFreeVoice();
    if (!voice) return;
    
    voice->note = note;
    voice->velocity = velocity;
    voice->baseFrequency = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    voice->active = true;
    voice->noteOnTime = static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    
    // Reset grain spawn timer
    voice->grainSpawnTimer = 0.0f;
    voice->grainSpawnInterval = 1.0f / grainDensity_;
    
    // Clear existing grains
    for (auto& grain : voice->grains) {
        grain.active = false;
    }
    
    std::cout << "GranularEngine: Note ON " << static_cast<int>(note) 
              << " vel=" << velocity << std::endl;
}

void GranularEngine::noteOff(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.active && voice.note == note) {
            // Don't immediately stop - let grains finish naturally
            std::cout << "GranularEngine: Note OFF " << static_cast<int>(note) << std::endl;
            break;
        }
    }
}

void GranularEngine::setAftertouch(uint8_t note, float aftertouch) {
    // Map aftertouch to grain randomness
    for (auto& voice : voices_) {
        if (voice.active && voice.note == note) {
            setGrainRandomness(aftertouch);
        }
    }
}

void GranularEngine::allNotesOff() {
    for (auto& voice : voices_) {
        if (voice.active) {
            for (auto& grain : voice.grains) {
                grain.active = false;
            }
            voice.active = false;
        }
    }
    std::cout << "GranularEngine: All notes off" << std::endl;
}

void GranularEngine::setParameter(ParameterID param, float value) {
    value = std::clamp(value, 0.0f, 1.0f);
    
    switch (param) {
        case ParameterID::VOLUME:
            volume_ = value;
            break;
        case ParameterID::ATTACK:
            attack_ = value * 2.0f; // 0-2 seconds
            break;
        case ParameterID::DECAY:
            decay_ = value * 3.0f; // 0-3 seconds
            break;
        case ParameterID::SUSTAIN:
            sustain_ = value;
            break;
        case ParameterID::RELEASE:
            release_ = value * 5.0f; // 0-5 seconds
            break;
        case ParameterID::LFO_RATE:
            // Map to grain density
            setGrainDensity(value * 100.0f); // 0-100 grains/sec
            break;
        case ParameterID::LFO_DEPTH:
            // Map to grain size
            setGrainSize(10.0f + value * 490.0f); // 10-500ms
            break;
        case ParameterID::FILTER_CUTOFF:
            // Map to grain pitch
            setGrainPitch(0.25f + value * 3.75f); // 0.25x to 4x
            break;
        case ParameterID::FILTER_RESONANCE:
            // Map to grain randomness
            setGrainRandomness(value);
            break;
        default:
            break;
    }
}

float GranularEngine::getParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::VOLUME: return volume_;
        case ParameterID::ATTACK: return attack_ / 2.0f;
        case ParameterID::DECAY: return decay_ / 3.0f;
        case ParameterID::SUSTAIN: return sustain_;
        case ParameterID::RELEASE: return release_ / 5.0f;
        case ParameterID::LFO_RATE: return grainDensity_ / 100.0f;
        case ParameterID::LFO_DEPTH: return (grainSize_ - 10.0f) / 490.0f;
        case ParameterID::FILTER_CUTOFF: return (grainPitch_ - 0.25f) / 3.75f;
        case ParameterID::FILTER_RESONANCE: return grainRandomness_;
        default: return 0.0f;
    }
}

bool GranularEngine::hasParameter(ParameterID param) const {
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

void GranularEngine::setGrainSize(float sizeMs) {
    grainSize_ = std::clamp(sizeMs, 1.0f, 500.0f);
}

void GranularEngine::setGrainDensity(float density) {
    grainDensity_ = std::clamp(density, 0.1f, 200.0f);
    
    // Update all active voices
    for (auto& voice : voices_) {
        if (voice.active) {
            voice.grainSpawnInterval = 1.0f / grainDensity_;
        }
    }
}

void GranularEngine::setGrainPitch(float pitch) {
    grainPitch_ = std::clamp(pitch, 0.1f, 8.0f);
}

void GranularEngine::setGrainSpread(float spread) {
    grainSpread_ = std::clamp(spread, 0.0f, 1.0f);
}

void GranularEngine::setGrainRandomness(float randomness) {
    grainRandomness_ = std::clamp(randomness, 0.0f, 1.0f);
}

void GranularEngine::setTextureMode(int mode) {
    textureMode_ = std::clamp(mode, 0, static_cast<int>(TextureMode::COUNT) - 1);
}

void GranularEngine::setTouchPosition(float x, float y) {
    touchX_ = std::clamp(x, 0.0f, 1.0f);
    touchY_ = std::clamp(y, 0.0f, 1.0f);
    
    // Map touch to parameters
    setGrainDensity(touchX_ * 100.0f); // X controls density
    setGrainSize(10.0f + touchY_ * 490.0f); // Y controls grain size
}

// Grain implementation
float GranularEngine::Grain::process() {
    if (!active) return 0.0f;
    
    // Calculate envelope value (Hann window)
    float envValue = 1.0f;
    if (envPhase < 1.0f) {
        envValue = 0.5f * (1.0f - std::cos(2.0f * M_PI * envPhase));
    } else {
        active = false;
        return 0.0f;
    }
    
    // Get sample from waveform
    float sample = 0.0f;
    if (waveform && waveformSize > 0) {
        size_t index = static_cast<size_t>(position);
        if (index < waveformSize - 1) {
            // Linear interpolation
            float frac = position - index;
            sample = waveform[index] * (1.0f - frac) + waveform[index + 1] * frac;
        } else if (index < waveformSize) {
            sample = waveform[index];
        }
    }
    
    return sample * amplitude * envValue;
}

void GranularEngine::Grain::trigger(float* source, size_t sourceSize, float pitch, float amp, float panPos, float duration) {
    waveform = source;
    waveformSize = sourceSize;
    position = 0.0f;
    increment = pitch;
    amplitude = amp;
    pan = panPos;
    envPhase = 0.0f;
    envDuration = duration;
    active = true;
}

void GranularEngine::Grain::updateEnvelope(float deltaTime) {
    if (active && envDuration > 0.0f) {
        envPhase += deltaTime / envDuration;
    }
}

float GranularEngine::grainEnvelope(float phase) const {
    // Hann window
    if (phase >= 1.0f) return 0.0f;
    return 0.5f * (1.0f - std::cos(2.0f * M_PI * phase));
}

// GranularVoice implementation
float GranularEngine::GranularVoice::process(GranularEngine* engine) {
    const float deltaTime = 1.0f / SAMPLE_RATE;
    
    // Update grain spawn timer
    grainSpawnTimer += deltaTime;
    if (grainSpawnTimer >= grainSpawnInterval) {
        spawnGrain(engine);
        grainSpawnTimer = 0.0f;
    }
    
    // Process all active grains
    float output = 0.0f;
    for (auto& grain : grains) {
        if (grain.active) {
            grain.updateEnvelope(deltaTime);
            grain.position += grain.increment;
            
            // Wrap or stop grain based on texture mode
            if (grain.position >= grain.waveformSize) {
                switch (static_cast<GranularEngine::TextureMode>(engine->textureMode_)) {
                    case GranularEngine::TextureMode::FORWARD:
                        grain.active = false;
                        break;
                    case GranularEngine::TextureMode::REVERSE:
                        grain.increment = -std::abs(grain.increment);
                        break;
                    case GranularEngine::TextureMode::PINGPONG:
                        grain.increment = -grain.increment;
                        break;
                    case GranularEngine::TextureMode::RANDOM_JUMP:
                        grain.position = engine->getRandomValue() * grain.waveformSize;
                        break;
                    case GranularEngine::TextureMode::FREEZE:
                        grain.position = grain.waveformSize * 0.5f;
                        break;
                    default:
                        grain.active = false;
                        break;
                }
            }
            
            output += grain.process();
        }
    }
    
    return output * velocity;
}

void GranularEngine::GranularVoice::spawnGrain(GranularEngine* engine) {
    Grain* grain = findFreeGrain();
    if (!grain) return;
    
    // Select source waveform
    if (engine->sourceWaveforms_.empty()) return;
    auto& waveform = engine->sourceWaveforms_[engine->currentWaveform_];
    
    // Calculate grain parameters with randomization
    float pitch = engine->grainPitch_ * engine->applyRandomness(1.0f, engine->grainRandomness_);
    float amp = engine->applyRandomness(0.8f, engine->grainRandomness_ * 0.5f);
    float pan = engine->applyRandomness(0.5f, engine->grainSpread_);
    float duration = (engine->grainSize_ / 1000.0f) * engine->applyRandomness(1.0f, engine->grainRandomness_);
    
    grain->trigger(waveform.data(), waveform.size(), pitch, amp, pan, duration);
}

GranularEngine::Grain* GranularEngine::GranularVoice::findFreeGrain() {
    for (auto& grain : grains) {
        if (!grain.active) return &grain;
    }
    return nullptr; // All grains are active
}

GranularEngine::GranularVoice* GranularEngine::findFreeVoice() {
    // Find completely free voice first
    for (auto& voice : voices_) {
        if (!voice.active) return &voice;
    }
    
    // Steal oldest voice if no free voices
    GranularVoice* oldest = &voices_[0];
    for (auto& voice : voices_) {
        if (voice.noteOnTime < oldest->noteOnTime) {
            oldest = &voice;
        }
    }
    return oldest;
}

void GranularEngine::initializeSourceWaveforms() {
    sourceWaveforms_.resize(static_cast<size_t>(WaveformType::COUNT));
    
    const size_t waveformSize = 1024;
    
    for (int i = 0; i < static_cast<int>(WaveformType::COUNT); i++) {
        sourceWaveforms_[i].resize(waveformSize);
        generateWaveform(sourceWaveforms_[i], i);
    }
    
    currentWaveform_ = 0;
}

void GranularEngine::generateWaveform(std::vector<float>& waveform, int type) {
    const size_t size = waveform.size();
    
    switch (static_cast<WaveformType>(type)) {
        case WaveformType::SINE:
            for (size_t i = 0; i < size; i++) {
                waveform[i] = std::sin(2.0f * M_PI * i / size);
            }
            break;
            
        case WaveformType::TRIANGLE:
            for (size_t i = 0; i < size; i++) {
                float phase = static_cast<float>(i) / size;
                waveform[i] = (phase < 0.5f) ? (4.0f * phase - 1.0f) : (3.0f - 4.0f * phase);
            }
            break;
            
        case WaveformType::SAW:
            for (size_t i = 0; i < size; i++) {
                waveform[i] = 2.0f * static_cast<float>(i) / size - 1.0f;
            }
            break;
            
        case WaveformType::SQUARE:
            for (size_t i = 0; i < size; i++) {
                waveform[i] = (i < size / 2) ? 1.0f : -1.0f;
            }
            break;
            
        case WaveformType::NOISE:
            for (size_t i = 0; i < size; i++) {
                waveform[i] = getRandomValue() * 2.0f - 1.0f;
            }
            break;
            
        case WaveformType::HARMONIC_RICH:
            for (size_t i = 0; i < size; i++) {
                float phase = 2.0f * M_PI * i / size;
                waveform[i] = std::sin(phase) + 0.5f * std::sin(2.0f * phase) + 0.25f * std::sin(3.0f * phase);
            }
            break;
            
        case WaveformType::FORMANT:
            for (size_t i = 0; i < size; i++) {
                float phase = 2.0f * M_PI * i / size;
                waveform[i] = std::sin(phase) * std::exp(-phase * 0.5f);
            }
            break;
            
        case WaveformType::VOCAL:
            for (size_t i = 0; i < size; i++) {
                float phase = 2.0f * M_PI * i / size;
                // Simulate vocal formants
                waveform[i] = std::sin(phase) + 0.3f * std::sin(2.8f * phase) + 0.15f * std::sin(4.2f * phase);
            }
            break;
            
        default:
            std::fill(waveform.begin(), waveform.end(), 0.0f);
            break;
    }
}

float GranularEngine::getRandomValue() const {
    return uniformDist_(rng_);
}

float GranularEngine::applyRandomness(float value, float randomness) const {
    if (randomness <= 0.0f) return value;
    
    float deviation = randomness * (getRandomValue() * 2.0f - 1.0f);
    return std::clamp(value + deviation, 0.0f, 2.0f);
}

size_t GranularEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.active) count++;
    }
    return count;
}

void GranularEngine::setVoiceCount(size_t maxVoices) {
    std::cout << "GranularEngine: Voice count set to " << maxVoices << std::endl;
}

float GranularEngine::getCPUUsage() const {
    return cpuUsage_;
}

void GranularEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
    struct GranularPreset {
        float volume;
        float grainSize;
        float grainDensity;
        float grainPitch;
        float grainSpread;
        float grainRandomness;
        int textureMode;
        int currentWaveform;
    };
    
    actualSize = sizeof(GranularPreset);
    if (maxSize < actualSize) return;
    
    GranularPreset* preset = reinterpret_cast<GranularPreset*>(data);
    preset->volume = volume_;
    preset->grainSize = grainSize_;
    preset->grainDensity = grainDensity_;
    preset->grainPitch = grainPitch_;
    preset->grainSpread = grainSpread_;
    preset->grainRandomness = grainRandomness_;
    preset->textureMode = textureMode_;
    preset->currentWaveform = currentWaveform_;
}

bool GranularEngine::loadPreset(const uint8_t* data, size_t size) {
    struct GranularPreset {
        float volume;
        float grainSize;
        float grainDensity;
        float grainPitch;
        float grainSpread;
        float grainRandomness;
        int textureMode;
        int currentWaveform;
    };
    
    if (size < sizeof(GranularPreset)) return false;
    
    const GranularPreset* preset = reinterpret_cast<const GranularPreset*>(data);
    volume_ = preset->volume;
    grainSize_ = preset->grainSize;
    grainDensity_ = preset->grainDensity;
    grainPitch_ = preset->grainPitch;
    grainSpread_ = preset->grainSpread;
    grainRandomness_ = preset->grainRandomness;
    textureMode_ = preset->textureMode;
    currentWaveform_ = preset->currentWaveform;
    
    return true;
}

void GranularEngine::setSampleRate(float sampleRate) {
    std::cout << "GranularEngine: Sample rate set to " << sampleRate << "Hz" << std::endl;
}

void GranularEngine::setBufferSize(size_t bufferSize) {
    std::cout << "GranularEngine: Buffer size set to " << bufferSize << " samples" << std::endl;
}