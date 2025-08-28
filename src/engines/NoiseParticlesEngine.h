#pragma once
#include "../BaseEngine.h"
#include "../DSPUtils.h"
#include <array>

namespace NoiseParticles {

/**
 * Single particle grain with random timing and spectral content
 */
class ParticleGrain {
public:
    ParticleGrain() : active_(false), position_(0.0f), duration_(0.0f), amplitude_(0.0f),
                     frequency_(440.0f), phase_(0.0f) {}
    
    void trigger(float frequency, float amplitude, float durationMs, float randomSpray) {
        active_ = true;
        position_ = 0.0f;
        
        // Apply spray randomization
        frequency_ = frequency * (1.0f + randomSpray * random_.uniform(-0.5f, 0.5f));
        amplitude_ = amplitude * (0.5f + randomSpray * random_.uniform(0.0f, 0.5f));
        duration_ = (durationMs / 1000.0f) * (1.0f + randomSpray * random_.uniform(-0.3f, 0.3f));
        duration_ = std::max(0.005f, duration_);  // Minimum 5ms grain
        
        phase_ = random_.uniform(0.0f, 2.0f * M_PI);  // Random phase
    }
    
    float render(float sampleRate) {
        if (!active_) return 0.0f;
        
        position_ += 1.0f / sampleRate;
        
        if (position_ >= duration_) {
            active_ = false;
            return 0.0f;
        }
        
        // Generate noise burst with envelope
        float progress = position_ / duration_;
        float envelope = std::sin(progress * M_PI);  // Sine envelope
        
        // Colored noise generation
        float noise = random_.uniform(-1.0f, 1.0f);
        
        // Add some spectral coloring based on frequency
        float coloredNoise = noise;
        if (frequency_ > 1000.0f) {
            // High frequency emphasis - simple high-pass
            static float lastSample = 0.0f;
            coloredNoise = noise - lastSample * 0.5f;
            lastSample = noise;
        }
        
        return coloredNoise * envelope * amplitude_;
    }
    
    bool isActive() const { return active_; }
    void reset() { active_ = false; }
    
private:
    bool active_;
    float position_;     // Current position in grain (seconds)
    float duration_;     // Grain duration (seconds)  
    float amplitude_;    // Grain amplitude
    float frequency_;    // Center frequency for coloring
    float phase_;        // Random phase offset
    DSP::Random random_;
};

/**
 * Bandpass filter for spectral shaping
 */
class BandpassFilter {
public:
    BandpassFilter() : centerFreq_(1000.0f), Q_(2.0f) {
        reset();
    }
    
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        updateCoefficients();
    }
    
    void setParameters(float centerFreq, float Q) {
        centerFreq_ = std::clamp(centerFreq, 100.0f, sampleRate_ * 0.45f);
        Q_ = std::clamp(Q, 0.1f, 20.0f);
        updateCoefficients();
    }
    
    float process(float input) {
        float output = b0_ * input + b1_ * x1_ + b2_ * x2_ - a1_ * y1_ - a2_ * y2_;
        
        // Shift delay line
        x2_ = x1_;
        x1_ = input;
        y2_ = y1_;
        y1_ = output;
        
        return output;
    }
    
    void reset() {
        x1_ = x2_ = y1_ = y2_ = 0.0f;
    }
    
private:
    float sampleRate_ = 48000.0f;
    float centerFreq_, Q_;
    
    // Biquad coefficients
    float b0_, b1_, b2_, a1_, a2_;
    float x1_, x2_, y1_, y2_;
    
    void updateCoefficients() {
        if (sampleRate_ <= 0) return;
        
        float omega = 2.0f * M_PI * centerFreq_ / sampleRate_;
        float alpha = std::sin(omega) / (2.0f * Q_);
        
        float norm = 1.0f / (1.0f + alpha);
        b0_ = alpha * norm;
        b1_ = 0.0f;
        b2_ = -alpha * norm;
        a1_ = -2.0f * std::cos(omega) * norm;
        a2_ = (1.0f - alpha) * norm;
    }
};

/**
 * Particle generator with density control
 */
class ParticleGenerator {
public:
    static constexpr int MAX_PARTICLES = 32;
    
    ParticleGenerator() : densityHz_(50.0f), grainMs_(20.0f), nextTriggerTime_(0.0f),
                         spray_(0.0f), currentTime_(0.0f) {}
    
    void init(float sampleRate) {
        sampleRate_ = sampleRate;
        currentTime_ = 0.0f;
        nextTriggerTime_ = 0.0f;
        for (int i = 0; i < MAX_PARTICLES; ++i) {
            particles_[i].reset();
        }
    }
    
    void setDensity(float densityHz) {
        densityHz_ = std::clamp(densityHz, 1.0f, 200.0f);
    }
    
    void setGrainSize(float grainMs) {
        grainMs_ = std::clamp(grainMs, 5.0f, 60.0f);
    }
    
    void setSpray(float spray) {
        spray_ = std::clamp(spray, 0.0f, 1.0f);
    }
    
    float render() {
        currentTime_ += 1.0f / sampleRate_;
        
        // Check if it's time to trigger a new particle
        if (currentTime_ >= nextTriggerTime_) {
            triggerNextParticle();
            
            // Schedule next particle with spray randomization
            float interval = 1.0f / densityHz_;
            float sprayVariation = spray_ * interval * random_.uniform(-0.5f, 0.5f);
            nextTriggerTime_ = currentTime_ + interval + sprayVariation;
        }
        
        // Render all active particles
        float output = 0.0f;
        int activeCount = 0;
        
        for (int i = 0; i < MAX_PARTICLES; ++i) {
            if (particles_[i].isActive()) {
                output += particles_[i].render(sampleRate_);
                activeCount++;
            }
        }
        
        // Normalize by active particle count to prevent clipping
        if (activeCount > 0) {
            output /= std::sqrt(static_cast<float>(activeCount));
        }
        
        return output;
    }
    
    void reset() {
        currentTime_ = 0.0f;
        nextTriggerTime_ = 0.0f;
        for (int i = 0; i < MAX_PARTICLES; ++i) {
            particles_[i].reset();
        }
    }
    
private:
    std::array<ParticleGrain, MAX_PARTICLES> particles_;
    float sampleRate_;
    float densityHz_;       // Particles per second
    float grainMs_;         // Grain duration in milliseconds
    float spray_;           // Randomization amount
    float currentTime_;     // Current time in seconds
    float nextTriggerTime_; // When to trigger next particle
    DSP::Random random_;
    
    void triggerNextParticle() {
        // Find an inactive particle slot
        for (int i = 0; i < MAX_PARTICLES; ++i) {
            if (!particles_[i].isActive()) {
                // Trigger particle with random frequency and amplitude
                float frequency = 200.0f + random_.uniform(0.0f, 1800.0f);  // 200-2000 Hz
                float amplitude = 0.1f + random_.uniform(0.0f, 0.4f);       // Variable amplitude
                
                particles_[i].trigger(frequency, amplitude, grainMs_, spray_);
                break;
            }
        }
    }
};

} // namespace NoiseParticles

/**
 * NoiseParticles Voice - granular noise synthesis voice
 */
class NoiseParticlesVoice : public BaseVoice {
public:
    NoiseParticlesVoice() : densityHz_(50.0f), centerFreq_(1000.0f), Q_(4.0f), spray_(0.0f) {}
    
    void setSampleRate(float sampleRate) override {
        BaseVoice::setSampleRate(sampleRate);
        particleGen_.init(sampleRate);
        bandpassFilter_.init(sampleRate);
        bandpassFilter_.setParameters(centerFreq_, Q_);
    }
    
    void noteOn(float note, float velocity) override {
        BaseVoice::noteOn(note, velocity);
        
        // Use note to influence particle characteristics
        float freq = DSP::Oscillator::noteToFreq(note);
        centerFreq_ = std::clamp(freq, 200.0f, 4000.0f);
        bandpassFilter_.setParameters(centerFreq_, Q_);
        
        particleGen_.reset();
    }
    
    float renderSample(const RenderContext& ctx) override {
        if (!active_) return 0.0f;
        
        float envelope = ampEnv_.process();
        
        if (envelope <= 0.001f && releasing_) {
            active_ = false;
            return 0.0f;
        }
        
        // Generate particle cloud
        float sample = particleGen_.render();
        
        // Apply bandpass filtering
        sample = bandpassFilter_.process(sample);
        
        // Apply envelope and velocity
        sample *= envelope * velocity_ * 0.5f;  // Particles can be loud
        
        // Apply channel strip
        if (channelStrip_) {
            sample = channelStrip_->process(sample, note_);
        }
        
        return sample;
    }
    
    void renderBlock(const RenderContext& ctx, float* output, int count) override {
        for (int i = 0; i < count; ++i) {
            output[i] = renderSample(ctx);
        }
    }
    
    // Parameter setters
    void setDensity(float density) {
        densityHz_ = 1.0f + density * 199.0f;  // 1-200 Hz
        particleGen_.setDensity(densityHz_);
    }
    
    void setBandpassCenter(float center) {
        centerFreq_ = 100.0f + center * 3900.0f;  // 100-4000 Hz
        bandpassFilter_.setParameters(centerFreq_, Q_);
    }
    
    void setBandpassQ(float q) {
        Q_ = 0.5f + q * 19.5f;  // 0.5-20.0 Q
        bandpassFilter_.setParameters(centerFreq_, Q_);
    }
    
    void setSpray(float spray) {
        spray_ = spray;
        particleGen_.setSpray(spray_);
    }
    
    void setGrainSize(float grainMs) {
        particleGen_.setGrainSize(5.0f + grainMs * 55.0f);  // 5-60ms
    }
    
    uint32_t getAge() const { return 0; }  // TODO: Implement proper age tracking
    
private:
    NoiseParticles::ParticleGenerator particleGen_;
    NoiseParticles::BandpassFilter bandpassFilter_;
    
    // Voice parameters
    float densityHz_;    // Particle density (Hz)
    float centerFreq_;   // Bandpass center frequency
    float Q_;            // Bandpass Q factor
    float spray_;        // Randomization amount
};

/**
 * NoiseParticles Engine - Granular noise synthesis
 */
class NoiseParticlesEngine : public PolyphonicBaseEngine<NoiseParticlesVoice> {
public:
    NoiseParticlesEngine() 
        : PolyphonicBaseEngine("NoiseParticles", "NOISE", 
                              static_cast<int>(EngineFactory::EngineType::NOISE_PARTICLES),
                              CPUClass::Light, 6) {}  // Lower poly due to particle count
    
    // IEngine interface
    const char* getName() const override { return "NoiseParticles"; }
    const char* getShortName() const override { return "NOISE"; }
    
    void setParam(int paramID, float v01) override {
        BaseEngine::setParam(paramID, v01);  // Handle common parameters
        
        switch (static_cast<EngineParamID>(paramID)) {
            case EngineParamID::HARMONICS:
                // HARMONICS controls particle density (1-200 Hz)
                densityHz_ = v01;
                for (auto& voice : voices_) {
                    voice->setDensity(densityHz_);
                }
                break;
                
            case EngineParamID::TIMBRE:
                // TIMBRE controls bandpass center frequency
                centerFreq_ = v01;
                for (auto& voice : voices_) {
                    voice->setBandpassCenter(centerFreq_);
                }
                break;
                
            case EngineParamID::MORPH:
                // MORPH controls spray randomization
                spray_ = v01;
                for (auto& voice : voices_) {
                    voice->setSpray(spray_);
                }
                break;
                
            case EngineParamID::DENSITY_HZ:
                // Direct density control
                densityHz_ = v01;
                for (auto& voice : voices_) {
                    voice->setDensity(densityHz_);
                }
                break;
                
            case EngineParamID::GRAIN_MS:
                // Grain size control
                grainMs_ = v01;
                for (auto& voice : voices_) {
                    voice->setGrainSize(grainMs_);
                }
                break;
                
            case EngineParamID::BP_CENTER:
                // Bandpass center frequency
                centerFreq_ = v01;
                for (auto& voice : voices_) {
                    voice->setBandpassCenter(centerFreq_);
                }
                break;
                
            case EngineParamID::BP_Q:
                // Bandpass Q factor
                bandpassQ_ = v01;
                for (auto& voice : voices_) {
                    voice->setBandpassQ(bandpassQ_);
                }
                break;
                
            case EngineParamID::SPRAY:
                // Spray randomization
                spray_ = v01;
                for (auto& voice : voices_) {
                    voice->setSpray(spray_);
                }
                break;
                
            default:
                break;
        }
    }
    
    // Parameter metadata
    int getParameterCount() const override { return 10; }
    
    const ParameterInfo* getParameterInfo(int index) const override {
        static const ParameterInfo params[] = {
            {static_cast<int>(EngineParamID::HARMONICS), "Density", "Hz", 0.25f, 0.0f, 1.0f, false, 0, "Particles"},
            {static_cast<int>(EngineParamID::TIMBRE), "Center", "Hz", 0.5f, 0.0f, 1.0f, false, 0, "Filter"},
            {static_cast<int>(EngineParamID::MORPH), "Spray", "", 0.0f, 0.0f, 1.0f, false, 0, "Particles"},
            {static_cast<int>(EngineParamID::DENSITY_HZ), "Rate", "Hz", 0.25f, 0.0f, 1.0f, false, 0, "Generator"},
            {static_cast<int>(EngineParamID::GRAIN_MS), "Size", "ms", 0.3f, 0.0f, 1.0f, false, 0, "Generator"},
            {static_cast<int>(EngineParamID::BP_CENTER), "BP Freq", "Hz", 0.5f, 0.0f, 1.0f, false, 0, "Filter"},
            {static_cast<int>(EngineParamID::BP_Q), "BP Res", "", 0.2f, 0.0f, 1.0f, false, 0, "Filter"},
            {static_cast<int>(EngineParamID::SPRAY), "Random", "", 0.0f, 0.0f, 1.0f, false, 0, "Generator"},
            {static_cast<int>(EngineParamID::LPF_CUTOFF), "Filter", "Hz", 0.8f, 0.0f, 1.0f, false, 0, "Filter"},
            {static_cast<int>(EngineParamID::DRIVE), "Drive", "", 0.1f, 0.0f, 1.0f, false, 0, "Channel"}
        };
        
        return (index >= 0 && index < 10) ? &params[index] : nullptr;
    }
    
private:
    // Engine-specific parameters
    float densityHz_ = 0.25f;      // Particle generation rate
    float grainMs_ = 0.3f;         // Grain duration
    float centerFreq_ = 0.5f;      // Bandpass center
    float bandpassQ_ = 0.2f;       // Bandpass resonance
    float spray_ = 0.0f;           // Randomization amount
};