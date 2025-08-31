#include "ProfessionalEffectsChain.h"
#include <algorithm>
#include <cstring>
#include <cmath>

namespace EtherSynth {

ProfessionalEffectsChain::ProfessionalEffectsChain() {
    // Initialize default performance state
    perfState_.reverbSend = 0.0f;
    perfState_.delaySend = 0.0f;
    perfState_.filterCutoff = 1.0f;
    perfState_.filterResonance = 0.0f;
    perfState_.filterType = 0;
    perfState_.limiterEnabled = true;
    perfState_.lufsTarget = -14.0f;
    
    tempBuffer1_.fill(AudioFrame());
    tempBuffer2_.fill(AudioFrame());
    sendBuffer1_.fill(AudioFrame());
    sendBuffer2_.fill(AudioFrame());
}

void ProfessionalEffectsChain::initialize(float sampleRate) {
    sampleRate_ = sampleRate;
    
    // Create essential effects
    performanceFilter_ = std::make_unique<PerformanceFilter>();
    performanceFilter_->setSampleRate(sampleRate);
    
    noteRepeatProcessor_ = std::make_unique<NoteRepeatProcessor>();
    noteRepeatProcessor_->setSampleRate(sampleRate);
    
    // Initialize default presets
    initializeDefaultPresets();
    
    // Create basic effect chain
    uint32_t tapeId = addEffect(EffectType::TAPE_SATURATION, EffectSlot::POST_ENGINE);
    uint32_t reverbId = addEffect(EffectType::REVERB, EffectSlot::SEND_1);
    uint32_t delayId = addEffect(EffectType::DELAY, EffectSlot::SEND_2);
    uint32_t limiterId = addEffect(EffectType::PEAK_LIMITER, EffectSlot::OUTPUT_PROCESSING);
    
    // Configure basic tape effect
    if (auto* tapeEffect = getEffect(tapeId)) {
        setEffectParameter(tapeId, 0, 0.3f);  // Saturation amount
        setEffectParameter(tapeId, 1, 0.4f);  // Compression amount
        setEffectParameter(tapeId, 2, 0.2f);  // Harmonic content
        setEffectParameter(tapeId, 3, 0.8f);  // Wet/dry mix
    }
    
    std::cout << "Professional Effects Chain initialized with " << effects_.size() << " effects" << std::endl;
}

uint32_t ProfessionalEffectsChain::addEffect(EffectType type, EffectSlot slot) {
    auto effect = std::make_unique<EffectInstance>();
    effect->type = type;
    effect->slot = slot;
    effect->id = nextEffectId_++;
    
    // Initialize effect-specific processors and parameters
    initializeEffectParameters(effect.get());
    
    switch (type) {
    case EffectType::TAPE_SATURATION:
        effect->tapeProcessor = std::make_unique<TapeEffectsProcessor>();
        effect->tapeProcessor->setSampleRate(sampleRate_);
        mapTapeEffectParameters(effect.get());
        strcpy(effect->name, "TAPE");
        break;
        
    case EffectType::DELAY:
        // effect->delayProcessor = std::make_unique<DelayEffect>();
        mapDelayEffectParameters(effect.get());
        strcpy(effect->name, "DELAY");
        break;
        
    case EffectType::REVERB:
        // effect->reverbProcessor = std::make_unique<ReverbEffect>();
        mapReverbEffectParameters(effect.get());
        strcpy(effect->name, "REVERB");
        break;
        
    case EffectType::FILTER:
        mapFilterEffectParameters(effect.get());
        strcpy(effect->name, "FILTER");
        break;
        
    default:
        // Basic parameter mapping for other effects
        for (int i = 0; i < 16; i++) {
            effect->parameterNames[i] = "PARAM" + std::to_string(i + 1);
        }
        strcpy(effect->name, "EFFECT");
        break;
    }
    
    uint32_t effectId = effect->id;
    effects_.push_back(std::move(effect));
    
    std::cout << "Added " << effectTypeToString(type) << " effect (ID: " << effectId << ") to " << effectSlotToString(slot) << std::endl;
    return effectId;
}

EffectInstance* ProfessionalEffectsChain::getEffect(uint32_t effectId) {
    auto it = std::find_if(effects_.begin(), effects_.end(),
        [effectId](const std::unique_ptr<EffectInstance>& effect) {
            return effect->id == effectId;
        });
    
    return (it != effects_.end()) ? it->get() : nullptr;
}

void ProfessionalEffectsChain::processInstrumentChannel(AudioFrame* buffer, int bufferSize, int instrumentIndex) {
    if (!buffer || bufferSize > BUFFER_SIZE) return;
    
    // Copy to temp buffer for processing
    std::copy(buffer, buffer + bufferSize, tempBuffer1_.begin());
    
    // Process PRE_FILTER effects
    processEffectSlot(EffectSlot::PRE_FILTER, tempBuffer1_.data(), bufferSize);
    
    // Process POST_ENGINE effects (tape saturation, etc.)
    processEffectSlot(EffectSlot::POST_ENGINE, tempBuffer1_.data(), bufferSize);
    
    // Apply send effects to separate buffers
    std::copy(tempBuffer1_.begin(), tempBuffer1_.begin() + bufferSize, sendBuffer1_.begin());
    std::copy(tempBuffer1_.begin(), tempBuffer1_.begin() + bufferSize, sendBuffer2_.begin());
    
    processEffectSlot(EffectSlot::SEND_1, sendBuffer1_.data(), bufferSize);  // Reverb
    processEffectSlot(EffectSlot::SEND_2, sendBuffer2_.data(), bufferSize);  // Delay
    
    // Mix sends back to main signal
    for (int i = 0; i < bufferSize; i++) {
        tempBuffer1_[i] += sendBuffer1_[i] * perfState_.reverbSend;
        tempBuffer1_[i] += sendBuffer2_[i] * perfState_.delaySend;
    }
    
    // Copy result back to output buffer
    std::copy(tempBuffer1_.begin(), tempBuffer1_.begin() + bufferSize, buffer);
    
    updateMetering(buffer, bufferSize);
}

void ProfessionalEffectsChain::processMasterBus(AudioFrame* buffer, int bufferSize) {
    if (!buffer || bufferSize > BUFFER_SIZE) return;
    
    // Process master insert effects
    processEffectSlot(EffectSlot::MASTER_INSERT, buffer, bufferSize);
    
    // Apply performance effects
    processPerformanceEffects(buffer, bufferSize);
    
    // Final output processing (LUFS, limiter)
    processEffectSlot(EffectSlot::OUTPUT_PROCESSING, buffer, bufferSize);
    
    updateMetering(buffer, bufferSize);
}

void ProfessionalEffectsChain::processPerformanceEffects(AudioFrame* buffer, int bufferSize) {
    // Performance filter
    if (perfState_.filterCutoff < 1.0f || perfState_.filterResonance > 0.0f) {
        performanceFilter_->setCutoff(perfState_.filterCutoff * 20000.0f);
        performanceFilter_->setResonance(perfState_.filterResonance);
        performanceFilter_->setType(static_cast<PerformanceFilter::Type>(perfState_.filterType));
        
        for (int i = 0; i < bufferSize; i++) {
            buffer[i].left = performanceFilter_->process(buffer[i].left);
            buffer[i].right = performanceFilter_->process(buffer[i].right);
        }
    }
    
    // Note repeat
    if (perfState_.noteRepeatActive) {
        noteRepeatProcessor_->process(buffer, bufferSize);
    }
}

void ProfessionalEffectsChain::setEffectParameter(uint32_t effectId, int keyIndex, float value) {
    auto* effect = getEffect(effectId);
    if (!effect || keyIndex < 0 || keyIndex >= 16) return;
    
    // Clamp value to parameter range
    auto range = effect->parameterRanges[keyIndex];
    float scaledValue = range.first + value * (range.second - range.first);
    effect->parameters[keyIndex] = scaledValue;
    
    // Apply to specific effect processor
    switch (effect->type) {
    case EffectType::TAPE_SATURATION:
        if (effect->tapeProcessor) {
            switch (keyIndex) {
            case 0: effect->tapeProcessor->setSaturationAmount(scaledValue); break;
            case 1: effect->tapeProcessor->setCompressionAmount(scaledValue); break;
            case 2: /* Harmonic content - would need specific setter */ break;
            case 3: effect->tapeProcessor->setWetDryMix(scaledValue); break;
            }
        }
        break;
        
    case EffectType::FILTER:
        if (performanceFilter_) {
            switch (keyIndex) {
            case 0: perfState_.filterCutoff = scaledValue; break;
            case 1: perfState_.filterResonance = scaledValue; break;
            case 2: perfState_.filterType = static_cast<int>(scaledValue); break;
            }
        }
        break;
        
    default:
        // Generic parameter storage
        break;
    }
    
    std::cout << "Set " << effect->name << " parameter " << keyIndex 
             << " (" << effect->parameterNames[keyIndex] << ") = " << scaledValue << std::endl;
}

void ProfessionalEffectsChain::processEffectSlot(EffectSlot slot, AudioFrame* buffer, int bufferSize) {
    auto slotEffects = getEffectsInSlot(slot);
    
    for (auto* effect : slotEffects) {
        if (!effect->enabled || effect->bypassed) continue;
        
        // Process based on effect type
        switch (effect->type) {
        case EffectType::TAPE_SATURATION:
            if (effect->tapeProcessor) {
                for (int i = 0; i < bufferSize; i++) {
                    buffer[i].left = effect->tapeProcessor->processSample(buffer[i].left);
                    buffer[i].right = effect->tapeProcessor->processSample(buffer[i].right);
                }
            }
            break;
            
        case EffectType::PEAK_LIMITER:
            // Basic limiter implementation
            for (int i = 0; i < bufferSize; i++) {
                buffer[i].left = std::tanh(buffer[i].left * 0.8f) * 1.2f;
                buffer[i].right = std::tanh(buffer[i].right * 0.8f) * 1.2f;
                
                // Hard limit to prevent clipping
                buffer[i].left = std::clamp(buffer[i].left, -1.0f, 1.0f);
                buffer[i].right = std::clamp(buffer[i].right, -1.0f, 1.0f);
            }
            break;
            
        default:
            // Placeholder for other effects
            break;
        }
    }
}

std::vector<EffectInstance*> ProfessionalEffectsChain::getEffectsInSlot(EffectSlot slot) {
    std::vector<EffectInstance*> slotEffects;
    
    for (auto& effect : effects_) {
        if (effect->slot == slot) {
            slotEffects.push_back(effect.get());
        }
    }
    
    return slotEffects;
}

void ProfessionalEffectsChain::mapTapeEffectParameters(EffectInstance* effect) {
    // Map tape effect parameters to 16-key interface
    effect->parameterNames[0] = "SAT AMT";    // Saturation amount
    effect->parameterNames[1] = "COMP AMT";   // Compression amount
    effect->parameterNames[2] = "HARMONICS"; // Harmonic content
    effect->parameterNames[3] = "WET/DRY";   // Wet/dry mix
    effect->parameterNames[4] = "WOW";       // Wow amount
    effect->parameterNames[5] = "FLUTTER";   // Flutter amount
    effect->parameterNames[6] = "NOISE";     // Tape noise
    effect->parameterNames[7] = "BIAS";      // Tape bias
    effect->parameterNames[8] = "LOW FREQ";  // Low frequency boost
    effect->parameterNames[9] = "HIGH FREQ"; // High frequency rolloff
    effect->parameterNames[10] = "ATTACK";   // Compressor attack
    effect->parameterNames[11] = "RELEASE";  // Compressor release
    effect->parameterNames[12] = "RATIO";    // Compression ratio
    effect->parameterNames[13] = "ASYMM";    // Saturation asymmetry
    effect->parameterNames[14] = "HYSTER";   // Hysteresis
    effect->parameterNames[15] = "PRINT";    // Print-through
    
    // Set appropriate ranges
    effect->parameterRanges[0] = {0.0f, 1.0f};   // Saturation
    effect->parameterRanges[1] = {0.0f, 1.0f};   // Compression
    effect->parameterRanges[2] = {0.0f, 1.0f};   // Harmonics
    effect->parameterRanges[3] = {0.0f, 1.0f};   // Wet/dry
    effect->parameterRanges[4] = {0.0f, 0.1f};   // Wow
    effect->parameterRanges[5] = {0.0f, 0.1f};   // Flutter
    effect->parameterRanges[6] = {-80.0f, -40.0f}; // Noise floor (dB)
    effect->parameterRanges[7] = {0.0f, 1.0f};   // Bias
    effect->parameterRanges[8] = {0.0f, 0.5f};   // Low boost
    effect->parameterRanges[9] = {0.0f, 0.5f};   // High rolloff
    effect->parameterRanges[10] = {1.0f, 50.0f}; // Attack (ms)
    effect->parameterRanges[11] = {50.0f, 500.0f}; // Release (ms)
    effect->parameterRanges[12] = {1.0f, 10.0f}; // Ratio
    effect->parameterRanges[13] = {0.0f, 0.5f};  // Asymmetry
    effect->parameterRanges[14] = {0.0f, 0.3f};  // Hysteresis
    effect->parameterRanges[15] = {0.0f, 0.1f};  // Print-through
}

void ProfessionalEffectsChain::mapDelayEffectParameters(EffectInstance* effect) {
    effect->parameterNames[0] = "TIME";      // Delay time
    effect->parameterNames[1] = "FEEDBACK";  // Feedback amount
    effect->parameterNames[2] = "MIX";       // Wet/dry mix
    effect->parameterNames[3] = "HP FREQ";   // High-pass filter
    effect->parameterNames[4] = "LP FREQ";   // Low-pass filter
    effect->parameterNames[5] = "STEREO";    // Stereo width
    effect->parameterNames[6] = "PING PONG"; // Ping-pong amount
    effect->parameterNames[7] = "TEMPO SYNC"; // Tempo sync
    effect->parameterNames[8] = "MODULATION"; // Modulation depth
    effect->parameterNames[9] = "MOD RATE";  // Modulation rate
    effect->parameterNames[10] = "DIFFUSION"; // Diffusion
    effect->parameterNames[11] = "SATURATION"; // Delay line saturation
    effect->parameterNames[12] = "REVERSE";  // Reverse delay
    effect->parameterNames[13] = "DUCK";     // Ducking amount
    effect->parameterNames[14] = "SPREAD";   // Multi-tap spread
    effect->parameterNames[15] = "CHARACTER"; // Character control
}

void ProfessionalEffectsChain::mapReverbEffectParameters(EffectInstance* effect) {
    effect->parameterNames[0] = "SIZE";      // Room size
    effect->parameterNames[1] = "DECAY";     // Decay time
    effect->parameterNames[2] = "DAMPING";   // High frequency damping
    effect->parameterNames[3] = "MIX";       // Wet/dry mix
    effect->parameterNames[4] = "PRE DELAY"; // Pre-delay
    effect->parameterNames[5] = "DIFFUSION"; // Diffusion
    effect->parameterNames[6] = "HP FREQ";   // High-pass filter
    effect->parameterNames[7] = "LP FREQ";   // Low-pass filter
    effect->parameterNames[8] = "MODULATION"; // Chorus modulation
    effect->parameterNames[9] = "DENSITY";   // Echo density
    effect->parameterNames[10] = "EARLY REF"; // Early reflections
    effect->parameterNames[11] = "LATE REV"; // Late reverb
    effect->parameterNames[12] = "STEREO";   // Stereo width
    effect->parameterNames[13] = "SHIMMER";  // Shimmer effect
    effect->parameterNames[14] = "FREEZE";   // Freeze mode
    effect->parameterNames[15] = "ALGORITHM"; // Reverb algorithm
}

void ProfessionalEffectsChain::mapFilterEffectParameters(EffectInstance* effect) {
    effect->parameterNames[0] = "CUTOFF";    // Cutoff frequency
    effect->parameterNames[1] = "RESONANCE"; // Resonance/Q
    effect->parameterNames[2] = "TYPE";      // Filter type
    effect->parameterNames[3] = "DRIVE";     // Input drive
    effect->parameterNames[4] = "KEY TRACK"; // Keyboard tracking
    effect->parameterNames[5] = "ENV DEPTH"; // Envelope depth
    effect->parameterNames[6] = "LFO DEPTH"; // LFO modulation depth
    effect->parameterNames[7] = "SLOPE";     // Filter slope (12/24dB)
    
    // Additional filter controls
    for (int i = 8; i < 16; i++) {
        effect->parameterNames[i] = "PARAM" + std::to_string(i + 1);
    }
}

void ProfessionalEffectsChain::updateMetering(const AudioFrame* buffer, int bufferSize) {
    // Simple peak metering
    float peakL = 0.0f, peakR = 0.0f;
    
    for (int i = 0; i < bufferSize; i++) {
        peakL = std::max(peakL, std::abs(buffer[i].left));
        peakR = std::max(peakR, std::abs(buffer[i].right));
    }
    
    peakLevel_ = std::max(peakL, peakR);
    limiterActive_ = peakLevel_ > 0.95f;
    
    // Basic LUFS estimation (simplified)
    lufsLevel_ = -14.0f + (peakLevel_ - 0.5f) * 10.0f;
}

void ProfessionalEffectsChain::initializeDefaultPresets() {
    // Initialize with basic preset
    presets_[0] = EffectPreset();
    strcpy(presets_[0].name, "Basic");
}

// Utility functions
const char* effectTypeToString(ProfessionalEffectsChain::EffectType type) {
    static const char* names[] = {
        "TAPE SAT", "DELAY", "REVERB", "FILTER", "BITCRUSH", "CHORUS",
        "PHASER", "COMPRESSOR", "EQ 3BAND", "DISTORTION", "LUFS NORM", "LIMITER"
    };
    
    int index = static_cast<int>(type);
    return (index >= 0 && index < 12) ? names[index] : "UNKNOWN";
}

const char* effectSlotToString(ProfessionalEffectsChain::EffectSlot slot) {
    static const char* names[] = {
        "PRE FILTER", "POST ENGINE", "SEND 1", "SEND 2", "MASTER INSERT", "OUTPUT"
    };
    
    int index = static_cast<int>(slot);
    return (index >= 0 && index < 6) ? names[index] : "UNKNOWN";
}

uint32_t getEffectTypeColor(ProfessionalEffectsChain::EffectType type) {
    static const uint32_t colors[] = {
        0xD2691E, // TAPE_SATURATION - SaddleBrown
        0x4169E1, // DELAY - RoyalBlue
        0x9370DB, // REVERB - MediumPurple
        0xFF6347, // FILTER - Tomato
        0x32CD32, // BITCRUSH - LimeGreen
        0xFF69B4, // CHORUS - HotPink
        0xFFA500, // PHASER - Orange
        0x8A2BE2, // COMPRESSOR - BlueViolet
        0x20B2AA, // EQ_3BAND - LightSeaGreen
        0xDC143C, // DISTORTION - Crimson
        0x00CED1, // LUFS_NORMALIZER - DarkTurquoise
        0xFF4500  // PEAK_LIMITER - OrangeRed
    };
    
    int index = static_cast<int>(type);
    return (index >= 0 && index < 12) ? colors[index] : 0x808080;
}

// PerformanceFilter implementation
void PerformanceFilter::updateCoefficients() {
    float omega = 2.0f * M_PI * cutoff_ / sampleRate_;
    float sin_omega = std::sin(omega);
    float cos_omega = std::cos(omega);
    
    g_ = std::tan(omega * 0.5f);
    k_ = 2.0f * resonance_;
    
    a1_ = 1.0f / (1.0f + g_ * (g_ + k_));
    a2_ = g_ * a1_;
    a3_ = g_ * a2_;
}

float PerformanceFilter::process(float input) {
    float v0 = input;
    float v1 = (a1_ * ic1eq_) + (a2_ * (v0 + ic2eq_));
    float v2 = ic2eq_ + (a2_ * ic1eq_) + (a3_ * v0);
    
    ic1eq_ = (2.0f * v1) - ic1eq_;
    ic2eq_ = (2.0f * v2) - ic2eq_;
    
    switch (type_) {
    case LOWPASS: return v2;
    case HIGHPASS: return v0 - k_ * v1 - v2;
    case BANDPASS: return v1;
    case NOTCH: return v0 - k_ * v1;
    default: return input;
    }
}

void PerformanceFilter::reset() {
    ic1eq_ = ic2eq_ = 0.0f;
}

// NoteRepeatProcessor implementation
void NoteRepeatProcessor::process(AudioFrame* buffer, int bufferSize) {
    if (!enabled_ || !triggered_) return;
    
    float phaseIncrement = (rate_ * division_ / 4.0f) / sampleRate_;
    
    for (int i = 0; i < bufferSize; i++) {
        phase_ += phaseIncrement;
        
        // Generate gate signal
        float gate = (std::fmod(phase_, 1.0f) < 0.5f) ? 1.0f : 0.0f;
        
        // Apply gate to audio
        buffer[i].left *= gate;
        buffer[i].right *= gate;
        
        if (phase_ >= 1.0f) phase_ -= 1.0f;
    }
}

} // namespace EtherSynth