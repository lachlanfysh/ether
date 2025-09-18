#include "GranularFX.h"
#include <cstring>

GranularFX::GranularFX() : rng_(std::random_device{}()) {
    initializeDefaultParams();
    
    // Initialize 3-second capture buffer at 48kHz (configurable)
    setSampleRate(48000.0f);
    setBufferSize(256);
    
    // Initialize filters
    returnHPF_L_.setCoeff(0.1f);
    returnHPF_R_.setCoeff(0.1f);
    returnLPF_L_.setCoeff(0.1f);
    returnLPF_R_.setCoeff(0.1f);
    feedbackLPF_L_.setCoeff(0.3f);
    feedbackLPF_R_.setCoeff(0.3f);
}

void GranularFX::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    
    // Allocate 3-second circular buffer (expandable to 4s)
    captureSize_ = static_cast<size_t>(sampleRate * 3.0f);
    captureBufferL_.resize(captureSize_, 0.0f);
    captureBufferR_.resize(captureSize_, 0.0f);
    captureIndex_ = 0;
}

void GranularFX::setBufferSize(size_t bufferSize) {
    bufferSize_ = bufferSize;
}

void GranularFX::initializeDefaultParams() {
    params_.fill(0.0f);
    params_[PARAM_SIZE] = 0.3f;          // ~80ms
    params_[PARAM_DENSITY] = 0.4f;       // ~10 grains/s
    params_[PARAM_POSITION] = 0.5f;      // Middle of buffer
    params_[PARAM_JITTER] = 0.2f;        // 20% jitter
    params_[PARAM_PITCH] = 0.5f;         // 0 semitones
    params_[PARAM_SPREAD] = 0.3f;        // 30% stereo spread
    params_[PARAM_TEXTURE] = 0.0f;       // Pure Hann window
    params_[PARAM_FEEDBACK] = 0.0f;      // No feedback
    params_[PARAM_FREEZE] = 0.0f;        // Capture active
    params_[PARAM_WET] = 1.0f;           // Full wet (dry handled by sends)
    params_[PARAM_RETURN_HPF] = 0.0f;    // 20Hz (minimal HPF)
    params_[PARAM_RETURN_LPF] = 1.0f;    // 18kHz (minimal LPF)
    params_[PARAM_QUALITY] = 0.5f;       // ~72 grains max
}

void GranularFX::setParameter(ParamIndex param, float value) {
    if (param >= 0 && param < PARAM_COUNT) {
        params_[param] = std::clamp(value, 0.0f, 1.0f);
    }
}

float GranularFX::getParameter(ParamIndex param) const {
    if (param >= 0 && param < PARAM_COUNT) {
        return params_[param];
    }
    return 0.0f;
}

void GranularFX::process(const float* inputL, const float* inputR, 
                        float* outputL, float* outputR, size_t blockSize) {
    // Clear output buffer
    std::fill(outputL, outputL + blockSize, 0.0f);
    std::fill(outputR, outputR + blockSize, 0.0f);
    
    // Update capture buffer (unless frozen)
    updateCaptureBuffer(inputL, inputR, blockSize);
    
    // Schedule new grains based on density
    scheduleGrains(blockSize);
    
    // Process active grains
    processGrains(outputL, outputR, blockSize);
    
    // Apply return filtering
    applyReturnFiltering(outputL, outputR, blockSize);
    
    // Apply feedback/smear if enabled
    if (params_[PARAM_FEEDBACK] > 0.01f) {
        applyFeedback(outputL, outputR, blockSize);
    }
    
    // Apply wet level
    float wetLevel = params_[PARAM_WET];
    for (size_t i = 0; i < blockSize; i++) {
        outputL[i] *= wetLevel;
        outputR[i] *= wetLevel;
    }
}

void GranularFX::updateCaptureBuffer(const float* inputL, const float* inputR, size_t blockSize) {
    // Skip capture if frozen
    if (params_[PARAM_FREEZE] > 0.5f) {
        captureActive_ = false;
        return;
    }
    
    captureActive_ = true;
    
    // Write input to circular buffer
    for (size_t i = 0; i < blockSize; i++) {
        captureBufferL_[captureIndex_] = inputL[i];
        captureBufferR_[captureIndex_] = inputR[i];
        
        captureIndex_++;
        if (captureIndex_ >= captureSize_) {
            captureIndex_ = 0;
        }
    }
}

void GranularFX::scheduleGrains(size_t blockSize) {
    float density = mapDensity(params_[PARAM_DENSITY]);
    float blockTimeMs = (float)blockSize / sampleRate_ * 1000.0f;
    
    // Calculate grains to launch this block
    grainTimer_ += blockTimeMs;
    float grainInterval = calculateGrainInterval();
    
    int maxGrainsPerBlock = mapQuality(params_[PARAM_QUALITY]);
    int grainsLaunched = 0;
    
    while (grainTimer_ >= grainInterval && grainsLaunched < maxGrainsPerBlock) {
        // Find free grain slot
        bool grainScheduled = false;
        for (size_t i = 0; i < MAX_GRAINS && !grainScheduled; i++) {
            if (!grains_[i].active) {
                scheduleNewGrain(grains_[i]);
                grainScheduled = true;
                grainsLaunched++;
            }
        }
        
        grainTimer_ -= grainInterval;
        
        // Add random jitter to interval
        float randTime = params_[PARAM_RAND_TIME];
        if (randTime > 0.01f) {
            float jitter = uniform_(rng_) * randTime * 0.2f; // ±20% max
            grainInterval *= (1.0f + jitter);
        }
    }
}

void GranularFX::scheduleNewGrain(Grain& grain) {
    grain.active = true;
    grain.windowPhase = 0.0f;
    
    // Calculate grain size
    float grainSizeMs = mapSize(params_[PARAM_SIZE]);
    float grainSizeSamples = grainSizeMs * 0.001f * sampleRate_;
    grain.windowInc = 1.0f / grainSizeSamples;
    
    // Calculate start position with jitter
    float basePosition = params_[PARAM_POSITION];
    float jitter = params_[PARAM_JITTER];
    
    float positionJitter = 0.0f;
    if (jitter > 0.01f) {
        positionJitter = uniform_(rng_) * jitter * 0.2f; // ±20% of size
    }
    
    float startPos = (basePosition + positionJitter);
    startPos = std::fmod(startPos, 1.0f); // Wrap to 0-1
    if (startPos < 0.0f) startPos += 1.0f;
    
    grain.bufferPosL = startPos * (captureSize_ - 1);
    grain.bufferPosR = grain.bufferPosL;
    
    // Calculate pitch shift
    float pitchSemitones = mapPitch(params_[PARAM_PITCH]);
    float randPitch = params_[PARAM_RAND_PITCH] * uniform_(rng_) * 3.0f; // ±3 semitones max
    float totalPitch = pitchSemitones + randPitch;
    grain.phaseInc = std::pow(2.0f, totalPitch / 12.0f);
    
    // Calculate pan gains
    auto panGains = generatePanGains(params_[PARAM_SPREAD]);
    grain.panL = panGains.first;
    grain.panR = panGains.second;
    
    // Set amplitude with slight randomization
    grain.amplitude = 0.7f + uniform_(rng_) * 0.3f;
}

void GranularFX::processGrains(float* outputL, float* outputR, size_t blockSize) {
    for (auto& grain : grains_) {
        if (!grain.active) continue;
        
        for (size_t i = 0; i < blockSize; i++) {
            if (grain.windowPhase >= 1.0f) {
                grain.active = false;
                break;
            }
            
            // Generate window envelope
            float window = generateWindow(grain.windowPhase, params_[PARAM_TEXTURE]);
            
            // Get samples from capture buffer with interpolation
            float sampleL = getInterpolatedSample(captureBufferL_, grain.bufferPosL);
            float sampleR = getInterpolatedSample(captureBufferR_, grain.bufferPosR);
            
            // Apply window and amplitude
            sampleL *= window * grain.amplitude;
            sampleR *= window * grain.amplitude;
            
            // Apply panning and mix to output
            outputL[i] += sampleL * grain.panL;
            outputR[i] += sampleR * grain.panR;
            
            // Update grain state
            grain.windowPhase += grain.windowInc;
            grain.bufferPosL += grain.phaseInc;
            grain.bufferPosR += grain.phaseInc;
            
            // Wrap buffer positions
            if (grain.bufferPosL >= captureSize_) grain.bufferPosL -= captureSize_;
            if (grain.bufferPosR >= captureSize_) grain.bufferPosR -= captureSize_;
        }
    }
}

void GranularFX::applyReturnFiltering(float* outputL, float* outputR, size_t blockSize) {
    // Update filter coefficients based on parameters
    float hpfFreq = mapReturnHPF(params_[PARAM_RETURN_HPF]);
    float lpfFreq = mapReturnLPF(params_[PARAM_RETURN_LPF]);
    
    // Simple one-pole filter coefficient calculation
    float hpfCoeff = std::clamp(hpfFreq / (sampleRate_ * 0.5f), 0.001f, 0.999f);
    float lpfCoeff = std::clamp(lpfFreq / (sampleRate_ * 0.5f), 0.001f, 0.999f);
    
    returnHPF_L_.setCoeff(hpfCoeff);
    returnHPF_R_.setCoeff(hpfCoeff);
    returnLPF_L_.setCoeff(lpfCoeff);
    returnLPF_R_.setCoeff(lpfCoeff);
    
    // Apply filtering
    for (size_t i = 0; i < blockSize; i++) {
        // HPF (high-pass via differencer approximation)
        float hpfL = outputL[i] - returnHPF_L_.process(outputL[i]);
        float hpfR = outputR[i] - returnHPF_R_.process(outputR[i]);
        
        // LPF
        outputL[i] = returnLPF_L_.process(hpfL);
        outputR[i] = returnLPF_R_.process(hpfR);
    }
}

void GranularFX::applyFeedback(const float* wetL, const float* wetR, size_t blockSize) {
    feedbackGain_ = params_[PARAM_FEEDBACK] * 0.3f; // Limit feedback to 30% max
    
    if (!captureActive_) return; // Don't feedback when frozen
    
    // Feedback filtered wet signal back into capture buffer
    for (size_t i = 0; i < blockSize; i++) {
        float fbL = feedbackLPF_L_.process(wetL[i]) * feedbackGain_;
        float fbR = feedbackLPF_R_.process(wetR[i]) * feedbackGain_;
        
        // Add to current capture position (with small delay to avoid immediate feedback)
        size_t feedbackIndex = (captureIndex_ + captureSize_ - 512) % captureSize_;
        captureBufferL_[feedbackIndex] += fbL;
        captureBufferR_[feedbackIndex] += fbR;
        
        // Clamp to prevent overflow
        captureBufferL_[feedbackIndex] = std::clamp(captureBufferL_[feedbackIndex], -2.0f, 2.0f);
        captureBufferR_[feedbackIndex] = std::clamp(captureBufferR_[feedbackIndex], -2.0f, 2.0f);
    }
}

// Parameter mapping functions
float GranularFX::mapSize(float norm) const {
    // 10-500ms logarithmic mapping
    return 10.0f * std::pow(50.0f, norm);
}

float GranularFX::mapDensity(float norm) const {
    // 2-50 grains/s logarithmic mapping
    return 2.0f * std::pow(25.0f, norm);
}

float GranularFX::mapPitch(float norm) const {
    // -24 to +24 semitones
    return (norm * 2.0f - 1.0f) * 24.0f;
}

float GranularFX::mapReturnHPF(float norm) const {
    // 20-600Hz exponential mapping
    return 20.0f * std::pow(30.0f, norm);
}

float GranularFX::mapReturnLPF(float norm) const {
    // 1k-18kHz exponential mapping
    return 1000.0f * std::pow(18.0f, norm);
}

int GranularFX::mapQuality(float norm) const {
    // 16-128 grain cap per block
    return static_cast<int>(16 + norm * 112);
}

float GranularFX::generateWindow(float phase, float texture) const {
    // Hann to Tukey window morphing
    float hann = 0.5f - 0.5f * std::cos(2.0f * M_PI * phase);
    
    // Tukey window with variable taper (alpha)
    float alpha = 0.1f + texture * 0.8f; // 10% to 90% taper
    float tukey;
    
    if (phase <= alpha / 2.0f) {
        float x = 2.0f * phase / alpha;
        tukey = 0.5f - 0.5f * std::cos(M_PI * x);
    } else if (phase >= 1.0f - alpha / 2.0f) {
        float x = 2.0f * (1.0f - phase) / alpha;
        tukey = 0.5f - 0.5f * std::cos(M_PI * x);
    } else {
        tukey = 1.0f;
    }
    
    // Blend between Hann and Tukey
    return hann * (1.0f - texture) + tukey * texture;
}

std::pair<float, float> GranularFX::generatePanGains(float spread) const {
    // Random panning within spread range
    float pan = uniform_(rng_) * spread * 2.0f - spread; // -spread to +spread
    float angle = (pan + 1.0f) * 0.25f * M_PI; // Map to 0 to π/2
    
    return {std::cos(angle), std::sin(angle)};
}

float GranularFX::getInterpolatedSample(const std::vector<float>& buffer, float position) const {
    if (buffer.empty()) return 0.0f;
    
    int index = static_cast<int>(position);
    float frac = position - index;
    
    int i0 = index % buffer.size();
    int i1 = (index + 1) % buffer.size();
    
    return buffer[i0] + frac * (buffer[i1] - buffer[i0]);
}

float GranularFX::calculateGrainInterval() const {
    float density = mapDensity(params_[PARAM_DENSITY]);
    return 1000.0f / density; // Interval in milliseconds
}

void GranularFX::reset() {
    // Clear capture buffers
    std::fill(captureBufferL_.begin(), captureBufferL_.end(), 0.0f);
    std::fill(captureBufferR_.begin(), captureBufferR_.end(), 0.0f);
    captureIndex_ = 0;
    
    // Clear active grains
    for (auto& grain : grains_) {
        grain.active = false;
    }
    activeGrains_ = 0;
    grainTimer_ = 0.0f;
    
    // Reset filters
    returnHPF_L_.state = 0.0f;
    returnHPF_R_.state = 0.0f;
    returnLPF_L_.state = 0.0f;
    returnLPF_R_.state = 0.0f;
    feedbackLPF_L_.state = 0.0f;
    feedbackLPF_R_.state = 0.0f;
}

const char* GranularFX::getParameterName(ParamIndex param) const {
    static const char* paramNames[PARAM_COUNT] = {
        "Size", "Density", "Position", "Jitter", "Pitch", "Spread",
        "Texture", "Feedback", "Freeze", "Wet", "Return HPF", "Return LPF",
        "Sync Division", "Rand Pitch", "Rand Time", "Quality"
    };
    
    if (param >= 0 && param < PARAM_COUNT) {
        return paramNames[param];
    }
    return "Unknown";
}