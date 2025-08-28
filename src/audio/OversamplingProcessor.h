#pragma once
#include <vector>
#include <array>

/**
 * OversamplingProcessor - Polyphase oversampling for aliasing reduction
 * 
 * Features:
 * - 2x or 4x oversampling with polyphase filters
 * - Anti-aliasing filters for upsampling and downsampling
 * - Optimized for real-time processing
 * - Low latency design suitable for synthesis
 * - Supports both block and sample-by-sample processing
 */
class OversamplingProcessor {
public:
    enum class Factor {
        X2 = 2,    // 2x oversampling
        X4 = 4     // 4x oversampling
    };
    
    OversamplingProcessor();
    ~OversamplingProcessor();
    
    // Initialization
    bool initialize(float sampleRate, Factor oversampleFactor = Factor::X2);
    void shutdown();
    
    // Configuration
    void setOversampleFactor(Factor factor);
    Factor getOversampleFactor() const { return oversampleFactor_; }
    
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Processing
    template<typename ProcessorFunc>
    float processSample(float input, ProcessorFunc processor);
    
    template<typename ProcessorFunc>
    void processBlock(const float* input, float* output, int numSamples, ProcessorFunc processor);
    
    // Analysis
    float getLatencyMs() const;
    int getLatencySamples() const { return latencySamples_; }
    float getCPUUsage() const { return cpuUsage_; }
    
private:
    // Configuration
    Factor oversampleFactor_ = Factor::X2;
    float sampleRate_ = 44100.0f;
    bool enabled_ = true;
    bool initialized_ = false;
    
    // Filter coefficients (half-band filters for polyphase implementation)
    std::vector<float> upsampleCoeffs_;
    std::vector<float> downsampleCoeffs_;
    
    // Delay lines for polyphase filters
    std::vector<float> upsampleDelay_;
    std::vector<float> downsampleDelay_;
    
    // Processing buffers
    std::vector<float> oversampledBuffer_;
    std::vector<float> processedBuffer_;
    
    // State
    int filterLength_;
    int latencySamples_;
    float cpuUsage_ = 0.0f;
    
    // Filter design
    void designHalfBandFilter();
    void initializeDelayLines();
    
    // Polyphase processing
    void upsample2x(const float* input, float* output, int numInputSamples);
    void downsample2x(const float* input, float* output, int numInputSamples);
    void upsample4x(const float* input, float* output, int numInputSamples);
    void downsample4x(const float* input, float* output, int numInputSamples);
    
    // Filter processing
    float processUpsampleFilter(float input);
    float processDownsampleFilter(float input);
    
    // Utility functions
    void clearDelayLines();
    float calculateCPUUsage(int numSamples, float processingTimeMs);
    
    // Constants
    static constexpr int MAX_FILTER_LENGTH = 31;
    static constexpr float CPU_USAGE_SMOOTH = 0.99f;
    
    // Half-band filter coefficients (31-tap, optimized for real-time use)
    static constexpr std::array<float, MAX_FILTER_LENGTH> HALFBAND_COEFFS = {{
        -0.000244140625f, 0.0f, 0.000732421875f, 0.0f, -0.00152587890625f,
        0.0f, 0.00262451171875f, 0.0f, -0.004150390625f, 0.0f,
        0.006103515625f, 0.0f, -0.008544921875f, 0.0f, 0.011962890625f,
        0.0f, -0.017333984375f, 0.0f, 0.026611328125f, 0.0f,
        -0.044677734375f, 0.0f, 0.099853515625f, 0.0f, 0.3505859375f,
        0.5f, 0.3505859375f, 0.0f, 0.099853515625f, 0.0f, -0.044677734375f
    }};
};

// Template method implementations
template<typename ProcessorFunc>
float OversamplingProcessor::processSample(float input, ProcessorFunc processor) {
    if (!initialized_ || !enabled_) {
        return processor(input);
    }
    
    int oversampleRate = static_cast<int>(oversampleFactor_);
    
    // Upsample
    std::vector<float> upsampled(oversampleRate);
    if (oversampleFactor_ == Factor::X2) {
        float upsampledArray[2] = {0.0f, 0.0f};
        upsample2x(&input, upsampledArray, 1);
        upsampled[0] = upsampledArray[0];
        upsampled[1] = upsampledArray[1];
    } else {
        float upsampledArray[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        upsample4x(&input, upsampledArray, 1);
        for (int i = 0; i < 4; i++) {
            upsampled[i] = upsampledArray[i];
        }
    }
    
    // Process at higher sample rate
    std::vector<float> processed(oversampleRate);
    for (int i = 0; i < oversampleRate; i++) {
        processed[i] = processor(upsampled[i]);
    }
    
    // Downsample
    float output;
    if (oversampleFactor_ == Factor::X2) {
        downsample2x(processed.data(), &output, 2);
    } else {
        downsample4x(processed.data(), &output, 4);
    }
    
    return output;
}

template<typename ProcessorFunc>
void OversamplingProcessor::processBlock(const float* input, float* output, int numSamples, ProcessorFunc processor) {
    if (!initialized_ || !enabled_) {
        for (int i = 0; i < numSamples; i++) {
            output[i] = processor(input[i]);
        }
        return;
    }
    
    int oversampleRate = static_cast<int>(oversampleFactor_);
    int oversampledSize = numSamples * oversampleRate;
    
    // Ensure buffers are large enough
    if (oversampledBuffer_.size() < static_cast<size_t>(oversampledSize)) {
        oversampledBuffer_.resize(oversampledSize);
        processedBuffer_.resize(oversampledSize);
    }
    
    // Upsample
    if (oversampleFactor_ == Factor::X2) {
        upsample2x(input, oversampledBuffer_.data(), numSamples);
    } else {
        upsample4x(input, oversampledBuffer_.data(), numSamples);
    }
    
    // Process at higher sample rate
    for (int i = 0; i < oversampledSize; i++) {
        processedBuffer_[i] = processor(oversampledBuffer_[i]);
    }
    
    // Downsample
    if (oversampleFactor_ == Factor::X2) {
        downsample2x(processedBuffer_.data(), output, oversampledSize);
    } else {
        downsample4x(processedBuffer_.data(), output, oversampledSize);
    }
}