#pragma once
#include <cstdint>

// Platform detection for SIMD
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
    #include <arm_neon.h>
    #define SIMD_NEON 1
#elif defined(__AVX2__)
    #include <immintrin.h>
    #define SIMD_AVX2 1
#elif defined(__SSE2__)
    #include <emmintrin.h>
    #define SIMD_SSE2 1
#endif

namespace EtherSynthSIMD {
namespace SIMD {

/**
 * Actual SIMD implementations for EtherSynth performance-critical paths
 * These replace the existing scalar operations in the real codebase
 */

// Fast buffer clearing (replaces std::fill in render loops)
inline void clearBuffer(float* buffer, int count) {
#ifdef SIMD_NEON
    // ARM NEON implementation (4 floats at once)
    const float32x4_t zero = vdupq_n_f32(0.0f);
    int simdCount = count & ~3; // Round down to multiple of 4
    
    for (int i = 0; i < simdCount; i += 4) {
        vst1q_f32(&buffer[i], zero);
    }
    
    // Handle remainder
    for (int i = simdCount; i < count; ++i) {
        buffer[i] = 0.0f;
    }
#elif defined(SIMD_AVX2)
    // AVX2 implementation (8 floats at once)
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
    // Scalar fallback
    for (int i = 0; i < count; ++i) {
        buffer[i] = 0.0f;
    }
#endif
}

// Fast voice summing (replaces the loops in BaseEngine render)
inline void accumulateVoices(float* output, const float* const* voiceBuffers, 
                            int numVoices, int blockSize) {
#ifdef SIMD_NEON
    // NEON implementation
    for (int i = 0; i < blockSize; i += 4) {
        float32x4_t sum = vld1q_f32(&output[i]);
        
        for (int voice = 0; voice < numVoices; ++voice) {
            float32x4_t voiceData = vld1q_f32(&voiceBuffers[voice][i]);
            sum = vaddq_f32(sum, voiceData);
        }
        
        vst1q_f32(&output[i], sum);
    }
#elif defined(SIMD_AVX2)
    // AVX2 implementation
    int simdCount = blockSize & ~7;
    for (int i = 0; i < simdCount; i += 8) {
        __m256 sum = _mm256_loadu_ps(&output[i]);
        
        for (int voice = 0; voice < numVoices; ++voice) {
            __m256 voiceData = _mm256_loadu_ps(&voiceBuffers[voice][i]);
            sum = _mm256_add_ps(sum, voiceData);
        }
        
        _mm256_storeu_ps(&output[i], sum);
    }
    
    // Handle remainder
    for (int i = simdCount; i < blockSize; ++i) {
        for (int voice = 0; voice < numVoices; ++voice) {
            output[i] += voiceBuffers[voice][i];
        }
    }
#else
    // Scalar implementation
    for (int i = 0; i < blockSize; ++i) {
        for (int voice = 0; voice < numVoices; ++voice) {
            output[i] += voiceBuffers[voice][i];
        }
    }
#endif
}

// Fast parameter smoothing (for parameter interpolation)
inline void smoothParameters(float* current, const float* target, 
                           float smoothing, int count) {
#ifdef SIMD_NEON
    const float32x4_t smoothingVec = vdupq_n_f32(smoothing);
    const float32x4_t invSmoothingVec = vdupq_n_f32(1.0f - smoothing);
    
    int simdCount = count & ~3;
    for (int i = 0; i < simdCount; i += 4) {
        float32x4_t currentVec = vld1q_f32(&current[i]);
        float32x4_t targetVec = vld1q_f32(&target[i]);
        
        float32x4_t result = vaddq_f32(
            vmulq_f32(currentVec, smoothingVec),
            vmulq_f32(targetVec, invSmoothingVec)
        );
        
        vst1q_f32(&current[i], result);
    }
    
    // Handle remainder
    for (int i = simdCount; i < count; ++i) {
        current[i] = current[i] * smoothing + target[i] * (1.0f - smoothing);
    }
#else
    // Scalar fallback
    for (int i = 0; i < count; ++i) {
        current[i] = current[i] * smoothing + target[i] * (1.0f - smoothing);
    }
#endif
}

// Fast envelope processing (ADSR optimization)
inline void processADSRBlock(float* envelopes, const float* rates, 
                           const float* targets, int count) {
#ifdef SIMD_NEON
    int simdCount = count & ~3;
    for (int i = 0; i < simdCount; i += 4) {
        float32x4_t envVec = vld1q_f32(&envelopes[i]);
        float32x4_t rateVec = vld1q_f32(&rates[i]);
        float32x4_t targetVec = vld1q_f32(&targets[i]);
        
        // envelope += rate * (target - envelope)
        float32x4_t diff = vsubq_f32(targetVec, envVec);
        float32x4_t delta = vmulq_f32(rateVec, diff);
        float32x4_t result = vaddq_f32(envVec, delta);
        
        vst1q_f32(&envelopes[i], result);
    }
    
    // Handle remainder
    for (int i = simdCount; i < count; ++i) {
        envelopes[i] += rates[i] * (targets[i] - envelopes[i]);
    }
#else
    // Scalar fallback
    for (int i = 0; i < count; ++i) {
        envelopes[i] += rates[i] * (targets[i] - envelopes[i]);
    }
#endif
}

// Fast oscillator processing (sine wave table lookup)
inline void processOscillatorBlock(float* output, float* phases, 
                                 const float* frequencies, const float* sineTable,
                                 int tableSize, int count) {
    const float tableScale = static_cast<float>(tableSize - 1);
    
    // This is inherently serial due to phase accumulation, 
    // but we can optimize the table lookup
    for (int i = 0; i < count; ++i) {
        float phase = phases[i];
        
        // Table lookup with linear interpolation
        float tableIndex = phase * tableScale;
        int idx = static_cast<int>(tableIndex);
        float frac = tableIndex - idx;
        
        int nextIdx = (idx + 1) % tableSize;
        output[i] = sineTable[idx] * (1.0f - frac) + sineTable[nextIdx] * frac;
        
        // Update phase
        phases[i] = phase + frequencies[i];
        if (phases[i] >= 1.0f) phases[i] -= 1.0f;
    }
}

} // namespace SIMD
} // namespace EtherSynthSIMD