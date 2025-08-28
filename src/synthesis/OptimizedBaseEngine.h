#pragma once
#include "BaseEngine.h"
#include "../core/PerformanceOptimizer.h"
#include <immintrin.h>  // For SIMD operations

namespace EtherSynth {

/**
 * Performance-optimized base engine implementation
 * 
 * Key optimizations:
 * - SIMD voice summing
 * - Memory pool allocation for voices
 * - Cache-optimized voice iteration
 * - Branch prediction optimization
 * - Reduced virtual function calls in hot paths
 */

// Optimized voice base with better cache locality
class OptimizedVoice : public BaseVoice {
public:
    OptimizedVoice() : BaseVoice() {
        // Pre-allocate render buffer to avoid allocations
        renderBuffer_.resize(256); // Max expected block size
    }
    
    // Non-virtual render method for better performance
    FORCE_INLINE void renderBlockOptimized(const RenderContext& ctx, float* output, int count) {
        if (UNLIKELY(!active_)) {
            std::fill(output, output + count, 0.0f);
            return;
        }
        
        // Cache-friendly single-pass rendering
        for (int i = 0; i < count; ++i) {
            output[i] = renderSampleOptimized(ctx);
        }
    }
    
    // Optimized sample rendering with reduced overhead
    FORCE_INLINE float renderSampleOptimized(const RenderContext& ctx) {
        if (UNLIKELY(!active_)) return 0.0f;
        
        // Inline critical path processing
        float envelope = ampEnv_.process();
        if (UNLIKELY(envelope <= 0.001f && releasing_)) {
            active_ = false;
            return 0.0f;
        }
        
        // Derived classes override this for specific synthesis
        float sample = generateSample(ctx);
        
        // Apply envelope and velocity
        sample *= envelope * velocity_;
        
        // Channel strip processing (if enabled)
        if (LIKELY(channelStrip_ && channelStrip_->isEnabled())) {
            sample = channelStrip_->process(sample, note_);
        }
        
        return sample;
    }
    
protected:
    // Pure virtual for derived classes to implement efficiently
    virtual float generateSample(const RenderContext& ctx) = 0;
    
private:
    CacheOptimizedArray<float> renderBuffer_;
};

/**
 * SIMD-optimized polyphonic engine template
 */
template<typename VoiceType>
class OptimizedPolyphonicEngine : public BaseEngine {
    static_assert(std::is_base_of_v<OptimizedVoice, VoiceType>, 
                  "VoiceType must derive from OptimizedVoice");
    
public:
    OptimizedPolyphonicEngine(const char* name, const char* shortName, int engineID,
                             CPUClass cpuClass, int maxVoices = 8)
        : BaseEngine(name, shortName, engineID, cpuClass), 
          maxVoices_(maxVoices),
          voiceMemoryPool_(maxVoices * 2), // Extra capacity for safety
          voiceOutputBuffers_(maxVoices),
          activeVoiceIndices_(maxVoices) {
        
        // Pre-allocate all voices using memory pool
        voices_.reserve(maxVoices);
        for (int i = 0; i < maxVoices; ++i) {
            VoiceType* voice = voiceMemoryPool_.allocate();
            if (voice) {
                new(voice) VoiceType(); // Placement new
                voices_.emplace_back(voice);
            }
        }
        
        // Pre-allocate output buffers
        for (auto& buffer : voiceOutputBuffers_) {
            buffer.resize(256); // Max block size
        }
    }
    
    ~OptimizedPolyphonicEngine() {
        // Explicitly destroy and deallocate voices
        for (auto* voice : voices_) {
            if (voice) {
                voice->~VoiceType();
                voiceMemoryPool_.deallocate(voice);
            }
        }
    }
    
    void noteOn(float note, float velocity, uint32_t id) override {
        PROFILE_METHOD();
        
        VoiceType* voice = findAvailableVoiceOptimized();
        if (UNLIKELY(!voice)) {
            voice = stealVoiceOptimized();
        }
        
        if (LIKELY(voice)) {
            voice->noteOn(note, velocity);
            assignVoiceID(voice, id, note, velocity);
        }
    }
    
    void noteOff(uint32_t id) override {
        PROFILE_METHOD();
        
        VoiceType* voice = findVoiceByID(id);
        if (LIKELY(voice)) {
            voice->noteOff();
        }
    }
    
    // Heavily optimized render method
    void render(const RenderContext& ctx, float* out, int n) override {
        PROFILE_METHOD();
        
        // Clear output buffer with SIMD
        clearBufferSIMD(out, n);
        
        // Collect active voices for cache-friendly processing
        int activeCount = collectActiveVoices();
        if (UNLIKELY(activeCount == 0)) return;
        
        // Render all active voices in parallel (cache-friendly)
        for (int voiceIdx = 0; voiceIdx < activeCount; ++voiceIdx) {
            int voiceIndex = activeVoiceIndices_[voiceIdx];
            voices_[voiceIndex]->renderBlockOptimized(ctx, 
                voiceOutputBuffers_[voiceIndex].data(), n);
        }
        
        // SIMD-optimized voice summing
        sumVoicesSIMD(activeCount, out, n);
    }
    
    void prepare(double sampleRate, int maxBlockSize) override {
        PROFILE_METHOD();
        
        BaseEngine::prepare(sampleRate, maxBlockSize);
        
        // Resize buffers if needed
        if (maxBlockSize > static_cast<int>(voiceOutputBuffers_[0].size())) {
            for (auto& buffer : voiceOutputBuffers_) {
                buffer.resize(maxBlockSize);
            }
        }
        
        // Setup voices with SIMD preparation
        for (auto* voice : voices_) {
            if (voice) {
                voice->setSampleRate(static_cast<float>(sampleRate));
            }
        }
    }
    
protected:
    int maxVoices_;
    RealtimeMemoryPool<VoiceType> voiceMemoryPool_;
    std::vector<VoiceType*> voices_;
    
    // Cache-optimized buffers
    std::vector<CacheOptimizedArray<float>> voiceOutputBuffers_;
    std::vector<int> activeVoiceIndices_;
    
    // Voice management cache
    std::unordered_map<uint32_t, VoiceType*> voiceMap_;
    
    // Optimized voice management
    FORCE_INLINE VoiceType* findAvailableVoiceOptimized() {
        // Linear search is actually faster for small voice counts due to cache locality
        for (auto* voice : voices_) {
            if (LIKELY(voice && !voice->isActive())) {
                return voice;
            }
        }
        return nullptr;
    }
    
    FORCE_INLINE VoiceType* stealVoiceOptimized() {
        // Find oldest releasing voice first
        VoiceType* candidate = nullptr;
        uint32_t oldestTime = UINT32_MAX;
        
        for (auto* voice : voices_) {
            if (LIKELY(voice && voice->isReleasing())) {
                uint32_t age = voice->getAge();
                if (age < oldestTime) {
                    candidate = voice;
                    oldestTime = age;
                }
            }
        }
        
        // If no releasing voice, steal oldest active
        if (UNLIKELY(!candidate)) {
            oldestTime = UINT32_MAX;
            for (auto* voice : voices_) {
                if (LIKELY(voice)) {
                    uint32_t age = voice->getAge();
                    if (age < oldestTime) {
                        candidate = voice;
                        oldestTime = age;
                    }
                }
            }
        }
        
        return candidate;
    }
    
    FORCE_INLINE VoiceType* findVoiceByID(uint32_t id) {
        auto it = voiceMap_.find(id);
        return (it != voiceMap_.end()) ? it->second : nullptr;
    }
    
    void assignVoiceID(VoiceType* voice, uint32_t id, float note, float velocity) {
        // Remove old mapping if voice was already assigned
        for (auto it = voiceMap_.begin(); it != voiceMap_.end(); ++it) {
            if (it->second == voice) {
                voiceMap_.erase(it);
                break;
            }
        }
        voiceMap_[id] = voice;
    }
    
private:
    // Collect indices of active voices for cache-friendly processing
    FORCE_INLINE int collectActiveVoices() {
        int activeCount = 0;
        for (int i = 0; i < static_cast<int>(voices_.size()); ++i) {
            if (LIKELY(voices_[i] && voices_[i]->isActive())) {
                activeVoiceIndices_[activeCount++] = i;
            }
        }
        return activeCount;
    }
    
    // SIMD-optimized buffer clearing
    FORCE_INLINE void clearBufferSIMD(float* buffer, int count) {
        #ifdef __AVX2__
        // Clear 8 floats at once with AVX2
        const __m256 zero = _mm256_setzero_ps();
        int simdCount = count & ~7; // Round down to multiple of 8
        
        for (int i = 0; i < simdCount; i += 8) {
            _mm256_storeu_ps(&buffer[i], zero);
        }
        
        // Handle remainder
        for (int i = simdCount; i < count; ++i) {
            buffer[i] = 0.0f;
        }
        #else
        std::fill(buffer, buffer + count, 0.0f);
        #endif
    }
    
    // SIMD-optimized voice summing
    FORCE_INLINE void sumVoicesSIMD(int activeCount, float* output, int blockSize) {
        #ifdef __AVX2__
        // Sum 8 floats at once with AVX2
        int simdCount = blockSize & ~7;
        
        for (int i = 0; i < simdCount; i += 8) {
            __m256 sum = _mm256_loadu_ps(&output[i]);
            
            for (int voiceIdx = 0; voiceIdx < activeCount; ++voiceIdx) {
                int voiceIndex = activeVoiceIndices_[voiceIdx];
                __m256 voiceData = _mm256_loadu_ps(&voiceOutputBuffers_[voiceIndex][i]);
                sum = _mm256_add_ps(sum, voiceData);
            }
            
            _mm256_storeu_ps(&output[i], sum);
        }
        
        // Handle remainder
        for (int i = simdCount; i < blockSize; ++i) {
            for (int voiceIdx = 0; voiceIdx < activeCount; ++voiceIdx) {
                int voiceIndex = activeVoiceIndices_[voiceIdx];
                output[i] += voiceOutputBuffers_[voiceIndex][i];
            }
        }
        #else
        // Fallback scalar implementation
        for (int i = 0; i < blockSize; ++i) {
            for (int voiceIdx = 0; voiceIdx < activeCount; ++voiceIdx) {
                int voiceIndex = activeVoiceIndices_[voiceIdx];
                output[i] += voiceOutputBuffers_[voiceIndex][i];
            }
        }
        #endif
    }
};

// Optimized parameter smoothing with SIMD
class OptimizedParameterSmoother {
public:
    OptimizedParameterSmoother(float smoothingTime = 0.01f) 
        : smoothingTime_(smoothingTime), sampleRate_(48000.0f) {
        updateCoefficient();
    }
    
    void setSampleRate(float sampleRate) {
        sampleRate_ = sampleRate;
        updateCoefficient();
    }
    
    void setSmoothingTime(float timeSeconds) {
        smoothingTime_ = timeSeconds;
        updateCoefficient();
    }
    
    // Process multiple parameters at once with SIMD
    void processBlock(float* currentValues, const float* targetValues, int count) {
        #ifdef __AVX2__
        const __m256 coeff = _mm256_set1_ps(coefficient_);
        const __m256 invCoeff = _mm256_set1_ps(1.0f - coefficient_);
        
        int simdCount = count & ~7;
        for (int i = 0; i < simdCount; i += 8) {
            __m256 current = _mm256_loadu_ps(&currentValues[i]);
            __m256 target = _mm256_loadu_ps(&targetValues[i]);
            
            // current = current * coeff + target * (1 - coeff)
            __m256 result = _mm256_add_ps(_mm256_mul_ps(current, coeff),
                                        _mm256_mul_ps(target, invCoeff));
            
            _mm256_storeu_ps(&currentValues[i], result);
        }
        
        // Handle remainder
        for (int i = simdCount; i < count; ++i) {
            currentValues[i] = currentValues[i] * coefficient_ + 
                             targetValues[i] * (1.0f - coefficient_);
        }
        #else
        for (int i = 0; i < count; ++i) {
            currentValues[i] = currentValues[i] * coefficient_ + 
                             targetValues[i] * (1.0f - coefficient_);
        }
        #endif
    }
    
private:
    float smoothingTime_;
    float sampleRate_;
    float coefficient_;
    
    void updateCoefficient() {
        coefficient_ = std::exp(-1.0f / (smoothingTime_ * sampleRate_));
    }
};

} // namespace EtherSynth