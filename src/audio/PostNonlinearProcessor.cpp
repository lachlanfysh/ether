#include "PostNonlinearProcessor.h"
#include <algorithm>
#include <chrono>

PostNonlinearProcessor::PostNonlinearProcessor() {
    topology_ = FilterTopology::SUBSONIC_ONLY;
    bypassed_ = false;
    gainCompensationEnabled_ = true;
    initialized_ = false;
    sampleRate_ = 44100.0f;
    gainCompensation_ = 1.0f;
    cpuUsage_ = 0.0f;
}

bool PostNonlinearProcessor::initialize(float sampleRate, FilterTopology topology) {
    if (sampleRate <= 0.0f) {
        return false;
    }
    
    sampleRate_ = sampleRate;
    topology_ = topology;
    
    // Initialize components based on topology
    switch (topology_) {
        case FilterTopology::DC_ONLY:
            if (!dcBlocker_.initialize(sampleRate_, 24.0f)) {
                return false;
            }
            break;
            
        case FilterTopology::SUBSONIC_ONLY:
            if (!subsonicFilter_.initialize(sampleRate_, 24.0f, SubsonicFilter::FilterType::BUTTERWORTH)) {
                return false;
            }
            subsonicFilter_.enableDCBlocker(true); // Built-in DC blocker
            break;
            
        case FilterTopology::SERIAL:
            if (!dcBlocker_.initialize(sampleRate_, 5.0f)) {
                return false;
            }
            if (!subsonicFilter_.initialize(sampleRate_, 24.0f, SubsonicFilter::FilterType::BUTTERWORTH)) {
                return false;
            }
            subsonicFilter_.enableDCBlocker(false); // DC blocking handled separately
            break;
            
        case FilterTopology::PARALLEL:
            if (!dcBlocker_.initialize(sampleRate_, 24.0f)) {
                return false;
            }
            if (!subsonicFilter_.initialize(sampleRate_, 24.0f, SubsonicFilter::FilterType::BUTTERWORTH)) {
                return false;
            }
            subsonicFilter_.enableDCBlocker(false); // DC blocking handled separately
            break;
    }
    
    calculateGainCompensation();
    reset();
    
    initialized_ = true;
    return true;
}

void PostNonlinearProcessor::shutdown() {
    if (!initialized_) {
        return;
    }
    
    dcBlocker_.shutdown();
    subsonicFilter_.shutdown();
    
    initialized_ = false;
}

float PostNonlinearProcessor::processSample(float input) {
    if (!initialized_ || bypassed_) {
        return input;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    float output = input;
    
    switch (topology_) {
        case FilterTopology::DC_ONLY:
            output = dcBlocker_.processSample(input);
            break;
            
        case FilterTopology::SUBSONIC_ONLY:
            output = subsonicFilter_.processSample(input);
            break;
            
        case FilterTopology::SERIAL:
            output = dcBlocker_.processSample(input);
            output = subsonicFilter_.processSample(output);
            break;
            
        case FilterTopology::PARALLEL:
            {
                float dcOutput = dcBlocker_.processSample(input);
                float subsonicOutput = subsonicFilter_.processSample(input);
                output = (dcOutput + subsonicOutput) * 0.5f; // Simple blend
            }
            break;
    }
    
    // Apply gain compensation if enabled
    if (gainCompensationEnabled_) {
        output *= gainCompensation_;
    }
    
    // Update CPU usage
    auto endTime = std::chrono::high_resolution_clock::now();
    float processingTime = std::chrono::duration<float, std::micro>(endTime - startTime).count();
    cpuUsage_ = cpuUsage_ * 0.999f + processingTime * 0.001f;
    
    return output;
}

void PostNonlinearProcessor::processBlock(float* output, const float* input, int numSamples) {
    if (!initialized_ || bypassed_) {
        // Pass through if not initialized or bypassed
        for (int i = 0; i < numSamples; i++) {
            output[i] = input[i];
        }
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    switch (topology_) {
        case FilterTopology::DC_ONLY:
            dcBlocker_.processBlock(output, input, numSamples);
            break;
            
        case FilterTopology::SUBSONIC_ONLY:
            subsonicFilter_.processBlock(output, input, numSamples);
            break;
            
        case FilterTopology::SERIAL:
            // Process through DC blocker first
            dcBlocker_.processBlock(output, input, numSamples);
            // Then through subsonic filter (in-place)
            subsonicFilter_.processBlock(output, numSamples);
            break;
            
        case FilterTopology::PARALLEL:
            {
                // Create temporary buffer for parallel processing
                static thread_local std::vector<float> tempBuffer;
                if (tempBuffer.size() < static_cast<size_t>(numSamples)) {
                    tempBuffer.resize(numSamples);
                }
                
                // Process both paths
                dcBlocker_.processBlock(output, input, numSamples);
                subsonicFilter_.processBlock(tempBuffer.data(), input, numSamples);
                
                // Blend the outputs
                for (int i = 0; i < numSamples; i++) {
                    output[i] = (output[i] + tempBuffer[i]) * 0.5f;
                }
            }
            break;
    }
    
    // Apply gain compensation if enabled
    if (gainCompensationEnabled_) {
        for (int i = 0; i < numSamples; i++) {
            output[i] *= gainCompensation_;
        }
    }
    
    // Update CPU usage
    auto endTime = std::chrono::high_resolution_clock::now();
    float processingTime = std::chrono::duration<float, std::micro>(endTime - startTime).count();
    cpuUsage_ = cpuUsage_ * 0.99f + (processingTime / numSamples) * 0.01f;
}

void PostNonlinearProcessor::processBlock(float* buffer, int numSamples) {
    if (!initialized_ || bypassed_) {
        return;
    }
    
    auto startTime = std::chrono::high_resolution_clock::now();
    
    switch (topology_) {
        case FilterTopology::DC_ONLY:
            dcBlocker_.processBlock(buffer, numSamples);
            break;
            
        case FilterTopology::SUBSONIC_ONLY:
            subsonicFilter_.processBlock(buffer, numSamples);
            break;
            
        case FilterTopology::SERIAL:
            dcBlocker_.processBlock(buffer, numSamples);
            subsonicFilter_.processBlock(buffer, numSamples);
            break;
            
        case FilterTopology::PARALLEL:
            {
                // Create temporary buffer for parallel processing
                static thread_local std::vector<float> tempBuffer;
                static thread_local std::vector<float> originalBuffer;
                if (tempBuffer.size() < static_cast<size_t>(numSamples)) {
                    tempBuffer.resize(numSamples);
                    originalBuffer.resize(numSamples);
                }
                
                // Save original input
                std::copy(buffer, buffer + numSamples, originalBuffer.begin());
                
                // Process DC blocker path (in-place)
                dcBlocker_.processBlock(buffer, numSamples);
                
                // Process subsonic filter path
                subsonicFilter_.processBlock(tempBuffer.data(), originalBuffer.data(), numSamples);
                
                // Blend the outputs
                for (int i = 0; i < numSamples; i++) {
                    buffer[i] = (buffer[i] + tempBuffer[i]) * 0.5f;
                }
            }
            break;
    }
    
    // Apply gain compensation if enabled
    if (gainCompensationEnabled_) {
        for (int i = 0; i < numSamples; i++) {
            buffer[i] *= gainCompensation_;
        }
    }
    
    // Update CPU usage
    auto endTime = std::chrono::high_resolution_clock::now();
    float processingTime = std::chrono::duration<float, std::micro>(endTime - startTime).count();
    cpuUsage_ = cpuUsage_ * 0.99f + (processingTime / numSamples) * 0.01f;
}

void PostNonlinearProcessor::setFilterTopology(FilterTopology topology) {
    if (topology != topology_) {
        topology_ = topology;
        if (initialized_) {
            // Reinitialize with new topology
            float currentSampleRate = sampleRate_;
            shutdown();
            initialize(currentSampleRate, topology_);
        }
    }
}

void PostNonlinearProcessor::setSubsonicCutoff(float hz) {
    if (topology_ == FilterTopology::SUBSONIC_ONLY || 
        topology_ == FilterTopology::SERIAL || 
        topology_ == FilterTopology::PARALLEL) {
        subsonicFilter_.setCutoffFrequency(hz);
        calculateGainCompensation();
    }
}

void PostNonlinearProcessor::setSubsonicType(SubsonicFilter::FilterType type) {
    if (topology_ == FilterTopology::SUBSONIC_ONLY || 
        topology_ == FilterTopology::SERIAL || 
        topology_ == FilterTopology::PARALLEL) {
        subsonicFilter_.setFilterType(type);
        calculateGainCompensation();
    }
}

void PostNonlinearProcessor::setSampleRate(float sampleRate) {
    if (sampleRate > 0.0f && std::abs(sampleRate - sampleRate_) > 0.1f) {
        sampleRate_ = sampleRate;
        if (initialized_) {
            dcBlocker_.setSampleRate(sampleRate);
            subsonicFilter_.setSampleRate(sampleRate);
            calculateGainCompensation();
        }
    }
}

void PostNonlinearProcessor::setBypass(bool bypass) {
    bypassed_ = bypass;
}

void PostNonlinearProcessor::setGainCompensation(bool enable) {
    gainCompensationEnabled_ = enable;
    if (enable && initialized_) {
        calculateGainCompensation();
    }
}

float PostNonlinearProcessor::getSubsonicCutoff() const {
    return subsonicFilter_.getCutoffFrequency();
}

float PostNonlinearProcessor::getMagnitudeResponse(float frequency) const {
    if (!initialized_) {
        return 1.0f;
    }
    
    float magnitude = 1.0f;
    
    switch (topology_) {
        case FilterTopology::DC_ONLY:
            // Approximate DC blocker response (high-pass characteristic)
            if (frequency > 0.0f) {
                float omega = 2.0f * 3.14159f * frequency / sampleRate_;
                float cosOmega = std::cos(omega);
                magnitude = std::sqrt(2.0f * (1.0f - cosOmega)); // |1 - e^(-jω)|
            }
            break;
            
        case FilterTopology::SUBSONIC_ONLY:
            magnitude = subsonicFilter_.getMagnitudeResponse(frequency);
            break;
            
        case FilterTopology::SERIAL:
            {
                // Multiply both responses
                float dcMag = 0.0f;
                if (frequency > 0.0f) {
                    float omega = 2.0f * 3.14159f * frequency / sampleRate_;
                    float cosOmega = std::cos(omega);
                    dcMag = std::sqrt(2.0f * (1.0f - cosOmega));
                }
                float subMag = subsonicFilter_.getMagnitudeResponse(frequency);
                magnitude = dcMag * subMag;
            }
            break;
            
        case FilterTopology::PARALLEL:
            {
                // Average of both responses (simplified)
                float dcMag = 0.0f;
                if (frequency > 0.0f) {
                    float omega = 2.0f * 3.14159f * frequency / sampleRate_;
                    float cosOmega = std::cos(omega);
                    dcMag = std::sqrt(2.0f * (1.0f - cosOmega));
                }
                float subMag = subsonicFilter_.getMagnitudeResponse(frequency);
                magnitude = (dcMag + subMag) * 0.5f;
            }
            break;
    }
    
    return magnitude * gainCompensation_;
}

void PostNonlinearProcessor::reset() {
    dcBlocker_.reset();
    subsonicFilter_.reset();
    cpuUsage_ = 0.0f;
}

void PostNonlinearProcessor::reset(float initialValue) {
    dcBlocker_.reset(initialValue);
    subsonicFilter_.reset(initialValue);
    cpuUsage_ = 0.0f;
}

void PostNonlinearProcessor::processMultiple(PostNonlinearProcessor* processors, float** buffers, int numChannels, int numSamples) {
    for (int ch = 0; ch < numChannels; ch++) {
        if (processors[ch].isInitialized() && !processors[ch].isBypassed()) {
            processors[ch].processBlock(buffers[ch], numSamples);
        }
    }
}

// Private method implementations

void PostNonlinearProcessor::calculateGainCompensation() {
    if (!gainCompensationEnabled_ || !initialized_) {
        gainCompensation_ = 1.0f;
        return;
    }
    
    // Calculate gain compensation at reference frequency
    float referenceResponse = getMagnitudeResponse(GAIN_COMP_FREQ);
    
    if (referenceResponse > 0.01f) {
        gainCompensation_ = 1.0f / referenceResponse;
    } else {
        gainCompensation_ = 1.0f;
    }
    
    // Limit gain compensation to reasonable range
    gainCompensation_ = clamp(gainCompensation_, 0.5f, 2.0f);
}

void PostNonlinearProcessor::updateFilters() {
    // This method can be used for dynamic parameter updates
    if (initialized_) {
        calculateGainCompensation();
    }
}

float PostNonlinearProcessor::clamp(float value, float min, float max) const {
    return std::max(min, std::min(max, value));
}