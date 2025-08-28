#pragma once
#include "DCBlocker.h"
#include "SubsonicFilter.h"

/**
 * PostNonlinearProcessor - Integrated cleanup processor for after nonlinear stages
 * 
 * Combines DC blocking and subsonic filtering to clean up audio after
 * nonlinear processing stages like saturation, distortion, and FM synthesis.
 * 
 * Features:
 * - Integrated DC blocker and 24Hz high-pass filter
 * - Configurable filter topology (serial or parallel)
 * - CPU-optimized processing with optional bypass
 * - Multi-channel batch processing
 * - Automatic gain compensation
 */
class PostNonlinearProcessor {
public:
    enum class FilterTopology {
        DC_ONLY,        // DC blocker only
        SUBSONIC_ONLY,  // Subsonic filter only (includes built-in DC blocker)
        SERIAL,         // DC blocker → Subsonic filter
        PARALLEL        // DC blocker + Subsonic filter (blended)
    };
    
    PostNonlinearProcessor();
    ~PostNonlinearProcessor() = default;
    
    // Initialization
    bool initialize(float sampleRate, FilterTopology topology = FilterTopology::SUBSONIC_ONLY);
    void shutdown();
    
    // Processing
    float processSample(float input);
    void processBlock(float* output, const float* input, int numSamples);
    void processBlock(float* buffer, int numSamples); // In-place processing
    
    // Configuration
    void setFilterTopology(FilterTopology topology);
    void setSubsonicCutoff(float hz);
    void setSubsonicType(SubsonicFilter::FilterType type);
    void setSampleRate(float sampleRate);
    void setBypass(bool bypass);
    void setGainCompensation(bool enable); // Compensate for filter gain
    
    // Analysis
    FilterTopology getFilterTopology() const { return topology_; }
    float getSubsonicCutoff() const;
    bool isBypassed() const { return bypassed_; }
    bool isInitialized() const { return initialized_; }
    float getMagnitudeResponse(float frequency) const;
    
    // State management
    void reset();
    void reset(float initialValue);
    
    // Batch processing for multiple channels
    static void processMultiple(PostNonlinearProcessor* processors, float** buffers, int numChannels, int numSamples);
    
    // Performance monitoring
    float getCPUUsage() const { return cpuUsage_; }
    
private:
    FilterTopology topology_ = FilterTopology::SUBSONIC_ONLY;
    bool bypassed_ = false;
    bool gainCompensationEnabled_ = true;
    bool initialized_ = false;
    float sampleRate_ = 44100.0f;
    
    // Filter components
    DCBlocker dcBlocker_;
    SubsonicFilter subsonicFilter_;
    
    // Gain compensation
    float gainCompensation_ = 1.0f;
    
    // Performance monitoring
    mutable float cpuUsage_ = 0.0f;
    
    // Internal methods
    void calculateGainCompensation();
    void updateFilters();
    float clamp(float value, float min, float max) const;
    
    // Constants
    static constexpr float GAIN_COMP_FREQ = 1000.0f; // Reference frequency for gain compensation
};