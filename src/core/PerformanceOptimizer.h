#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <atomic>
#include <chrono>

namespace EtherSynth {

/**
 * Performance optimization framework for EtherSynth V1.0
 * 
 * Provides comprehensive performance analysis and optimization tools:
 * - Hot path identification and optimization
 * - Memory allocation tracking and pool management
 * - CPU usage profiling and optimization hints
 * - Cache-friendly data structure recommendations
 * - SIMD vectorization opportunities
 * - Real-time constraint validation
 */

// Performance metrics
struct PerformanceMetrics {
    float cpuUsage;                 // Current CPU usage percentage
    uint32_t memoryUsage;           // Current memory usage in bytes
    uint32_t peakMemoryUsage;       // Peak memory usage since last reset
    uint32_t allocationsPerSecond;  // Dynamic allocations per second
    float cacheHitRate;             // Instruction/data cache hit rate
    uint32_t audioDropouts;         // Audio dropouts in last second
    float averageBlockTime;         // Average audio block processing time (ms)
    float maxBlockTime;             // Maximum block processing time (ms)
    bool realTimeViolation;         // True if real-time constraints violated
    
    PerformanceMetrics() : cpuUsage(0.0f), memoryUsage(0), peakMemoryUsage(0),
                          allocationsPerSecond(0), cacheHitRate(0.0f), audioDropouts(0),
                          averageBlockTime(0.0f), maxBlockTime(0.0f), realTimeViolation(false) {}
};

// Hot path identification
struct HotPath {
    const char* functionName;       // Function name
    const char* className;          // Class name if applicable
    uint32_t callCount;            // Number of calls
    float totalTime;               // Total time spent (ms)
    float averageTime;             // Average time per call (μs)
    float maxTime;                 // Maximum time for single call (μs)
    bool isRealTimeCritical;       // True if in audio render path
    
    HotPath() : functionName(""), className(""), callCount(0), totalTime(0.0f),
               averageTime(0.0f), maxTime(0.0f), isRealTimeCritical(false) {}
};

// Memory pool for real-time allocations
template<typename T, size_t PoolSize = 1024>
class RealtimeMemoryPool {
public:
    RealtimeMemoryPool() : nextIndex_(0) {
        for (size_t i = 0; i < PoolSize - 1; ++i) {
            pool_[i].next = &pool_[i + 1];
        }
        pool_[PoolSize - 1].next = nullptr;
        freeList_ = &pool_[0];
    }
    
    T* allocate() {
        if (!freeList_) return nullptr;  // Pool exhausted
        
        Node* node = freeList_;
        freeList_ = freeList_->next;
        allocatedCount_++;
        
        return reinterpret_cast<T*>(&node->data);
    }
    
    void deallocate(T* ptr) {
        if (!ptr) return;
        
        Node* node = reinterpret_cast<Node*>(ptr);
        node->next = freeList_;
        freeList_ = node;
        allocatedCount_--;
    }
    
    size_t getAvailable() const { return PoolSize - allocatedCount_; }
    size_t getAllocated() const { return allocatedCount_; }
    bool isFull() const { return freeList_ == nullptr; }
    
private:
    union Node {
        alignas(T) char data[sizeof(T)];
        Node* next;
    };
    
    Node pool_[PoolSize];
    Node* freeList_;
    std::atomic<size_t> allocatedCount_{0};
    std::atomic<size_t> nextIndex_{0};
};

// SIMD-optimized operations
namespace SIMD {
    // Vector addition (4 floats at once)
    void addVectors(const float* a, const float* b, float* result, size_t count);
    
    // Vector multiplication
    void multiplyVectors(const float* a, const float* b, float* result, size_t count);
    
    // Vector accumulation (sum multiple vectors)
    void accumulateVectors(const float* const* inputs, int numInputs, float* output, size_t count);
    
    // Fast interpolation for wavetable lookup
    void interpolateLinear(const float* table, const float* indices, float* output, size_t count);
    
    // Optimized envelope processing
    void processEnvelopes(float* envelopes, const float* rates, size_t count);
}

// Cache-optimized data structures
template<typename T>
class CacheOptimizedArray {
public:
    static constexpr size_t CACHE_LINE_SIZE = 64;
    static constexpr size_t ALIGNMENT = CACHE_LINE_SIZE;
    
    CacheOptimizedArray(size_t size) : size_(size) {
        // Allocate aligned memory
        data_ = static_cast<T*>(std::aligned_alloc(ALIGNMENT, sizeof(T) * size));
    }
    
    ~CacheOptimizedArray() {
        if (data_) std::free(data_);
    }
    
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }
    
    T* data() { return data_; }
    const T* data() const { return data_; }
    size_t size() const { return size_; }
    
    // Prefetch data for better cache performance
    void prefetch(size_t index) const {
        __builtin_prefetch(&data_[index], 0, 1);
    }
    
private:
    T* data_;
    size_t size_;
};

// Performance profiler for hot path analysis
class PerformanceProfiler {
public:
    static PerformanceProfiler& getInstance();
    
    // Profiling control
    void startProfiling();
    void stopProfiling();
    void resetStatistics();
    bool isProfilingEnabled() const { return enabled_; }
    
    // Hot path tracking
    void enterFunction(const char* functionName, const char* className = nullptr);
    void exitFunction();
    
    // Memory tracking
    void trackAllocation(size_t bytes);
    void trackDeallocation(size_t bytes);
    
    // Performance metrics
    PerformanceMetrics getMetrics() const;
    std::vector<HotPath> getHotPaths(int topN = 10) const;
    
    // Real-time validation
    void setAudioBlockDeadline(float deadlineMs);
    void validateRealTimeConstraints();
    
    // Optimization suggestions
    std::vector<std::string> getOptimizationSuggestions() const;
    
private:
    PerformanceProfiler() = default;
    
    struct FunctionProfile {
        std::string name;
        std::string className;
        uint32_t callCount = 0;
        float totalTime = 0.0f;
        float maxTime = 0.0f;
        std::chrono::high_resolution_clock::time_point startTime;
        bool isActive = false;
    };
    
    std::atomic<bool> enabled_{false};
    std::vector<FunctionProfile> profiles_;
    std::vector<FunctionProfile*> callStack_;
    
    // Memory statistics
    std::atomic<uint32_t> totalAllocations_{0};
    std::atomic<uint32_t> currentMemoryUsage_{0};
    std::atomic<uint32_t> peakMemoryUsage_{0};
    
    // Audio timing
    float audioBlockDeadline_ = 2.0f; // 2ms default for 48kHz/64 samples
    std::atomic<uint32_t> realTimeViolations_{0};
    
    FunctionProfile* findProfile(const char* functionName, const char* className);
    float getCurrentTimeMs() const;
};

// RAII profiling helper
class ProfileScope {
public:
    ProfileScope(const char* functionName, const char* className = nullptr) {
        PerformanceProfiler::getInstance().enterFunction(functionName, className);
    }
    
    ~ProfileScope() {
        PerformanceProfiler::getInstance().exitFunction();
    }
};

// Performance optimization macros
#define PROFILE_FUNCTION() \
    EtherSynth::ProfileScope _prof(__FUNCTION__, nullptr)

#define PROFILE_METHOD() \
    EtherSynth::ProfileScope _prof(__FUNCTION__, typeid(*this).name())

#define LIKELY(x)   __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#define FORCE_INLINE __attribute__((always_inline)) inline

// Optimization hints for specific patterns
namespace OptimizationHints {
    // Branch prediction hints
    template<typename T>
    FORCE_INLINE bool likely(T&& condition) {
        return LIKELY(condition);
    }
    
    template<typename T>
    FORCE_INLINE bool unlikely(T&& condition) {
        return UNLIKELY(condition);
    }
    
    // Cache optimization
    FORCE_INLINE void prefetchRead(const void* addr) {
        __builtin_prefetch(addr, 0, 1);
    }
    
    FORCE_INLINE void prefetchWrite(const void* addr) {
        __builtin_prefetch(addr, 1, 1);
    }
    
    // Loop optimization
    template<typename Func>
    FORCE_INLINE void unrolledLoop4(Func&& func, size_t count) {
        size_t unrolledCount = count & ~3; // Multiple of 4
        size_t i = 0;
        
        for (; i < unrolledCount; i += 4) {
            func(i);
            func(i + 1);
            func(i + 2);
            func(i + 3);
        }
        
        // Handle remainder
        for (; i < count; ++i) {
            func(i);
        }
    }
}

// Specific optimizations for audio processing
namespace AudioOptimizations {
    // Optimized voice summing with SIMD
    void sumVoices(const float* const* voiceOutputs, int numVoices, 
                   float* output, size_t blockSize);
    
    // Fast parameter smoothing
    void smoothParameters(float* values, const float* targets, 
                         float smoothingFactor, size_t count);
    
    // Optimized wavetable interpolation
    void wavetableLookup(const float* wavetable, size_t tableSize,
                        const float* phases, float* output, size_t count);
    
    // Branch-free envelope processing
    void processADSR(float* envelopes, const float* rates, 
                     const bool* gates, size_t count);
}

} // namespace EtherSynth