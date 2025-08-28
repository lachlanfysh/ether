#include "SamplerSlicerEngine.h"
#include <algorithm>
#include <cmath>

namespace SamplerSlicer {

//-----------------------------------------------------------------------------
// SliceDetector Implementation
//-----------------------------------------------------------------------------

std::vector<size_t> SliceDetector::detectTransients(const std::vector<int16_t>& buffer, 
                                                   int channels, float sampleRate, 
                                                   float sensitivity) {
    std::vector<size_t> transients;
    
    if (buffer.empty()) return transients;
    
    const int frameSize = 512;  // Analysis frame size
    const int hopSize = frameSize / 4;  // 75% overlap
    const size_t totalFrames = buffer.size() / channels;
    
    std::vector<float> energyHistory;
    std::vector<float> fluxHistory;
    
    // First pass: Calculate energy and spectral flux
    for (size_t pos = 0; pos + frameSize < totalFrames; pos += hopSize) {
        // Extract mono frame (average channels if stereo)
        std::vector<float> frame(frameSize);
        for (int i = 0; i < frameSize; ++i) {
            float sample = 0.0f;
            for (int ch = 0; ch < channels; ++ch) {
                size_t idx = (pos + i) * channels + ch;
                if (idx < buffer.size()) {
                    sample += static_cast<float>(buffer[idx]) / 32768.0f;
                }
            }
            frame[i] = sample / channels;
        }
        
        float energy = calculateEnergy(frame.data(), frameSize);
        float flux = calculateSpectralFlux(frame.data(), frameSize);
        
        energyHistory.push_back(energy);
        fluxHistory.push_back(flux);
    }
    
    // Second pass: Find peaks
    const float threshold = 0.1f + sensitivity * 0.4f;  // Adaptive threshold
    const size_t minDistance = static_cast<size_t>(0.05f * sampleRate / hopSize);  // Min 50ms between transients
    
    for (size_t i = 2; i < energyHistory.size() - 2; ++i) {
        float currentEnergy = energyHistory[i];
        float currentFlux = fluxHistory[i];
        
        // Combined energy and flux detection
        float score = currentEnergy * 0.6f + currentFlux * 0.4f;
        
        // Check if it's a local maximum
        bool isPeak = (score > energyHistory[i-1]) && (score > energyHistory[i+1]) && 
                     (score > energyHistory[i-2]) && (score > energyHistory[i+2]);
        
        if (isPeak && score > threshold) {
            size_t framePos = i * hopSize;
            
            // Check minimum distance from previous transient
            bool farEnough = transients.empty() || 
                           (framePos - transients.back()) >= minDistance;
            
            if (farEnough) {
                transients.push_back(framePos);
            }
        }
    }
    
    // Always include start and end
    if (transients.empty() || transients.front() > 0) {
        transients.insert(transients.begin(), 0);
    }
    if (transients.back() < totalFrames - 1) {
        transients.push_back(totalFrames - 1);
    }
    
    // Limit to 25 slices maximum
    if (transients.size() > 25) {
        transients.resize(25);
    }
    
    return transients;
}

std::vector<size_t> SliceDetector::detectGrid(size_t totalFrames, int divisions) {
    std::vector<size_t> slices;
    
    // Clamp divisions to reasonable range
    divisions = std::clamp(divisions, 2, 32);
    
    for (int i = 0; i <= divisions; ++i) {
        size_t pos = (totalFrames * i) / divisions;
        slices.push_back(pos);
    }
    
    // Limit to 25 slices
    if (slices.size() > 25) {
        slices.resize(25);
    }
    
    return slices;
}

std::vector<size_t> SliceDetector::snapToZeroCrossings(const std::vector<int16_t>& buffer,
                                                       const std::vector<size_t>& roughSlices,
                                                       int channels, int windowSize) {
    std::vector<size_t> snappedSlices;
    
    for (size_t roughPos : roughSlices) {
        size_t bestPos = roughPos;
        int minCrossing = std::numeric_limits<int>::max();
        
        // Search within window for zero crossing
        int startSearch = std::max(0, static_cast<int>(roughPos) - windowSize);
        int endSearch = std::min(static_cast<int>(buffer.size() / channels), 
                                static_cast<int>(roughPos) + windowSize);
        
        for (int pos = startSearch; pos < endSearch - 1; ++pos) {
            // Check for zero crossing (sign change)
            int16_t current = buffer[pos * channels];  // Use first channel
            int16_t next = buffer[(pos + 1) * channels];
            
            if ((current <= 0 && next > 0) || (current > 0 && next <= 0)) {
                // Found zero crossing - check how close to zero
                int crossingValue = std::abs(current) + std::abs(next);
                if (crossingValue < minCrossing) {
                    minCrossing = crossingValue;
                    bestPos = pos;
                }
            }
        }
        
        snappedSlices.push_back(bestPos);
    }
    
    return snappedSlices;
}

float SliceDetector::calculateEnergy(const float* window, int size) {
    float energy = 0.0f;
    for (int i = 0; i < size; ++i) {
        energy += window[i] * window[i];
    }
    return std::sqrt(energy / size);
}

float SliceDetector::calculateSpectralFlux(const float* window, int size) {
    // Simplified spectral flux using high-frequency energy
    // In a real implementation, this would use FFT
    
    float highFreqEnergy = 0.0f;
    float lowFreqEnergy = 0.0f;
    
    // Simple high-pass and low-pass filtering approximation
    for (int i = 1; i < size; ++i) {
        float diff = window[i] - window[i-1];  // High frequency content
        highFreqEnergy += diff * diff;
        lowFreqEnergy += window[i] * window[i];
    }
    
    // Return ratio of high to low frequency energy
    return (lowFreqEnergy > 0.0f) ? std::sqrt(highFreqEnergy / lowFreqEnergy) : 0.0f;
}

} // namespace SamplerSlicer