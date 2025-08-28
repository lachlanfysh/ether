#include "PerformanceOptimizer.h"
#include <algorithm>
#include <sstream>
#include <cstring>

#ifdef __AVX2__
#include <immintrin.h>
#endif

namespace EtherSynth {

//=============================================================================
// SIMD Operations Implementation
//=============================================================================

namespace SIMD {

void addVectors(const float* a, const float* b, float* result, size_t count) {
#ifdef __AVX2__
    const size_t simdCount = count & ~7; // Process 8 at a time
    
    for (size_t i = 0; i < simdCount; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 vr = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(&result[i], vr);
    }
    
    // Handle remainder
    for (size_t i = simdCount; i < count; ++i) {
        result[i] = a[i] + b[i];
    }
#else
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] + b[i];
    }
#endif
}

void multiplyVectors(const float* a, const float* b, float* result, size_t count) {
#ifdef __AVX2__
    const size_t simdCount = count & ~7;
    
    for (size_t i = 0; i < simdCount; i += 8) {
        __m256 va = _mm256_loadu_ps(&a[i]);
        __m256 vb = _mm256_loadu_ps(&b[i]);
        __m256 vr = _mm256_mul_ps(va, vb);
        _mm256_storeu_ps(&result[i], vr);
    }
    
    for (size_t i = simdCount; i < count; ++i) {
        result[i] = a[i] * b[i];
    }
#else
    for (size_t i = 0; i < count; ++i) {
        result[i] = a[i] * b[i];
    }
#endif
}

void accumulateVectors(const float* const* inputs, int numInputs, float* output, size_t count) {
#ifdef __AVX2__
    const size_t simdCount = count & ~7;
    
    for (size_t i = 0; i < simdCount; i += 8) {
        __m256 sum = _mm256_loadu_ps(&output[i]);
        
        for (int input = 0; input < numInputs; ++input) {
            __m256 data = _mm256_loadu_ps(&inputs[input][i]);
            sum = _mm256_add_ps(sum, data);
        }
        
        _mm256_storeu_ps(&output[i], sum);
    }
    
    for (size_t i = simdCount; i < count; ++i) {
        for (int input = 0; input < numInputs; ++input) {
            output[i] += inputs[input][i];
        }
    }
#else
    for (size_t i = 0; i < count; ++i) {
        for (int input = 0; input < numInputs; ++input) {
            output[i] += inputs[input][i];
        }
    }
#endif
}

void interpolateLinear(const float* table, const float* indices, float* output, size_t count) {
    // Linear interpolation for wavetable lookup
    for (size_t i = 0; i < count; ++i) {
        float index = indices[i];
        int idx = static_cast<int>(index);
        float frac = index - idx;
        
        // Assume table has proper bounds checking
        output[i] = table[idx] * (1.0f - frac) + table[idx + 1] * frac;
    }
}

void processEnvelopes(float* envelopes, const float* rates, size_t count) {
#ifdef __AVX2__
    const size_t simdCount = count & ~7;
    
    for (size_t i = 0; i < simdCount; i += 8) {
        __m256 env = _mm256_loadu_ps(&envelopes[i]);
        __m256 rate = _mm256_loadu_ps(&rates[i]);
        __m256 result = _mm256_mul_ps(env, rate);
        _mm256_storeu_ps(&envelopes[i], result);
    }
    
    for (size_t i = simdCount; i < count; ++i) {
        envelopes[i] *= rates[i];
    }
#else
    for (size_t i = 0; i < count; ++i) {
        envelopes[i] *= rates[i];
    }
#endif
}

} // namespace SIMD

//=============================================================================
// Audio Optimizations Implementation
//=============================================================================

namespace AudioOptimizations {

void sumVoices(const float* const* voiceOutputs, int numVoices, float* output, size_t blockSize) {
    // Clear output first
    std::memset(output, 0, blockSize * sizeof(float));
    
    // Use SIMD accumulation
    SIMD::accumulateVectors(voiceOutputs, numVoices, output, blockSize);
}

void smoothParameters(float* values, const float* targets, float smoothingFactor, size_t count) {
#ifdef __AVX2__
    const __m256 factor = _mm256_set1_ps(smoothingFactor);
    const __m256 invFactor = _mm256_set1_ps(1.0f - smoothingFactor);
    const size_t simdCount = count & ~7;
    
    for (size_t i = 0; i < simdCount; i += 8) {
        __m256 current = _mm256_loadu_ps(&values[i]);
        __m256 target = _mm256_loadu_ps(&targets[i]);
        
        __m256 result = _mm256_add_ps(_mm256_mul_ps(current, factor),
                                    _mm256_mul_ps(target, invFactor));
        
        _mm256_storeu_ps(&values[i], result);
    }
    
    for (size_t i = simdCount; i < count; ++i) {
        values[i] = values[i] * smoothingFactor + targets[i] * (1.0f - smoothingFactor);
    }
#else
    for (size_t i = 0; i < count; ++i) {
        values[i] = values[i] * smoothingFactor + targets[i] * (1.0f - smoothingFactor);
    }
#endif
}

void wavetableLookup(const float* wavetable, size_t tableSize, 
                    const float* phases, float* output, size_t count) {
    const float tableScale = static_cast<float>(tableSize - 1);
    
    for (size_t i = 0; i < count; ++i) {
        float phase = phases[i];
        
        // Wrap phase to [0, 1]
        phase = phase - std::floor(phase);
        
        // Scale to table index
        float index = phase * tableScale;
        int idx = static_cast<int>(index);
        float frac = index - idx;
        
        // Linear interpolation
        int nextIdx = (idx + 1) % tableSize;
        output[i] = wavetable[idx] * (1.0f - frac) + wavetable[nextIdx] * frac;
    }
}

void processADSR(float* envelopes, const float* rates, const bool* gates, size_t count) {
    // Branch-free ADSR processing
    for (size_t i = 0; i < count; ++i) {
        float env = envelopes[i];
        float rate = rates[i];
        bool gate = gates[i];
        
        // Branch-free envelope update
        float attack = gate ? rate : 0.0f;
        float release = gate ? 0.0f : -rate;
        
        env += attack + release;
        env = std::clamp(env, 0.0f, 1.0f);
        
        envelopes[i] = env;
    }
}

} // namespace AudioOptimizations

//=============================================================================
// Performance Profiler Implementation
//=============================================================================

PerformanceProfiler& PerformanceProfiler::getInstance() {
    static PerformanceProfiler instance;
    return instance;
}

void PerformanceProfiler::startProfiling() {
    enabled_ = true;
    resetStatistics();
}

void PerformanceProfiler::stopProfiling() {
    enabled_ = false;
}

void PerformanceProfiler::resetStatistics() {
    profiles_.clear();
    callStack_.clear();
    totalAllocations_ = 0;
    currentMemoryUsage_ = 0;
    peakMemoryUsage_ = 0;
    realTimeViolations_ = 0;
}

void PerformanceProfiler::enterFunction(const char* functionName, const char* className) {
    if (!enabled_) return;
    
    FunctionProfile* profile = findProfile(functionName, className);
    if (!profile) {
        profiles_.emplace_back();
        profile = &profiles_.back();
        profile->name = functionName;
        profile->className = className ? className : "";
    }
    
    profile->startTime = std::chrono::high_resolution_clock::now();
    profile->isActive = true;
    profile->callCount++;
    
    callStack_.push_back(profile);
}

void PerformanceProfiler::exitFunction() {
    if (!enabled_ || callStack_.empty()) return;
    
    FunctionProfile* profile = callStack_.back();
    callStack_.pop_back();
    
    if (!profile->isActive) return;
    
    auto endTime = std::chrono::high_resolution_clock::now();
    float duration = std::chrono::duration<float, std::milli>(endTime - profile->startTime).count();
    
    profile->totalTime += duration;
    profile->maxTime = std::max(profile->maxTime, duration * 1000.0f); // Convert to μs
    profile->averageTime = profile->totalTime * 1000.0f / profile->callCount; // μs
    profile->isActive = false;
    
    // Check for real-time violations
    if (duration > audioBlockDeadline_) {
        realTimeViolations_++;
    }
}

void PerformanceProfiler::trackAllocation(size_t bytes) {
    totalAllocations_++;
    currentMemoryUsage_ += bytes;
    peakMemoryUsage_ = std::max(peakMemoryUsage_.load(), currentMemoryUsage_.load());
}

void PerformanceProfiler::trackDeallocation(size_t bytes) {
    currentMemoryUsage_ = (currentMemoryUsage_ >= bytes) ? currentMemoryUsage_ - bytes : 0;
}

PerformanceMetrics PerformanceProfiler::getMetrics() const {
    PerformanceMetrics metrics;
    
    metrics.memoryUsage = currentMemoryUsage_;
    metrics.peakMemoryUsage = peakMemoryUsage_;
    metrics.allocationsPerSecond = totalAllocations_; // Simplified
    metrics.realTimeViolation = realTimeViolations_ > 0;
    
    // Calculate average block time
    float totalBlockTime = 0.0f;
    uint32_t audioCallCount = 0;
    
    for (const auto& profile : profiles_) {
        if (profile.name.find("render") != std::string::npos ||
            profile.name.find("process") != std::string::npos) {
            totalBlockTime += profile.totalTime;
            audioCallCount += profile.callCount;
        }
    }
    
    if (audioCallCount > 0) {
        metrics.averageBlockTime = totalBlockTime / audioCallCount;
    }
    
    return metrics;
}

std::vector<HotPath> PerformanceProfiler::getHotPaths(int topN) const {
    std::vector<HotPath> hotPaths;
    
    for (const auto& profile : profiles_) {
        HotPath hotPath;
        hotPath.functionName = profile.name.c_str();
        hotPath.className = profile.className.c_str();
        hotPath.callCount = profile.callCount;
        hotPath.totalTime = profile.totalTime;
        hotPath.averageTime = profile.averageTime;
        hotPath.maxTime = profile.maxTime;
        hotPath.isRealTimeCritical = (profile.name.find("render") != std::string::npos ||
                                     profile.name.find("process") != std::string::npos);
        
        hotPaths.push_back(hotPath);
    }
    
    // Sort by total time
    std::sort(hotPaths.begin(), hotPaths.end(), 
              [](const HotPath& a, const HotPath& b) {
                  return a.totalTime > b.totalTime;
              });
    
    if (static_cast<int>(hotPaths.size()) > topN) {
        hotPaths.resize(topN);
    }
    
    return hotPaths;
}

void PerformanceProfiler::setAudioBlockDeadline(float deadlineMs) {
    audioBlockDeadline_ = deadlineMs;
}

void PerformanceProfiler::validateRealTimeConstraints() {
    // Reset violation counter for next measurement period
    realTimeViolations_ = 0;
}

std::vector<std::string> PerformanceProfiler::getOptimizationSuggestions() const {
    std::vector<std::string> suggestions;
    
    // Analyze hot paths for optimization opportunities
    auto hotPaths = getHotPaths(10);
    
    for (const auto& hotPath : hotPaths) {
        if (hotPath.averageTime > 100.0f) { // > 100μs
            suggestions.push_back("Consider optimizing " + std::string(hotPath.functionName) + 
                                " (avg: " + std::to_string(hotPath.averageTime) + "μs)");
        }
        
        if (hotPath.isRealTimeCritical && hotPath.maxTime > audioBlockDeadline_ * 1000.0f) {
            suggestions.push_back("Real-time violation in " + std::string(hotPath.functionName) + 
                                " (max: " + std::to_string(hotPath.maxTime) + "μs)");
        }
    }
    
    // Memory usage suggestions
    if (peakMemoryUsage_ > 1024 * 1024 * 100) { // > 100MB
        suggestions.push_back("High memory usage detected. Consider memory pools.");
    }
    
    if (totalAllocations_ > 1000) {
        suggestions.push_back("High allocation rate. Consider pre-allocation strategies.");
    }
    
    return suggestions;
}

FunctionProfile* PerformanceProfiler::findProfile(const char* functionName, const char* className) {
    for (auto& profile : profiles_) {
        if (profile.name == functionName && 
            profile.className == (className ? className : "")) {
            return &profile;
        }
    }
    return nullptr;
}

float PerformanceProfiler::getCurrentTimeMs() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<float, std::milli>(duration).count();
}

} // namespace EtherSynth