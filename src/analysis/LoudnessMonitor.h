#pragma once
#include <vector>
#include <array>
#include <memory>
#include <mutex>
#include <atomic>
#include <deque>

/**
 * Professional LUFS Loudness Monitor for EtherSynth V1.0
 * 
 * ITU-R BS.1770-4 compliant loudness measurement:
 * - Integrated LUFS (long-term loudness)
 * - Short-term LUFS (3-second window)
 * - Momentary LUFS (400ms window)
 * - True Peak measurement with oversampling
 * - Loudness range (LRA) calculation
 * - Auto-normalization and limiting
 * - Real-time visualization for 960×320 display
 */

namespace EtherSynth {

class LoudnessMonitor {
public:
    static constexpr float SAMPLE_RATE = 48000.0f;
    static constexpr int MAX_CHANNELS = 8;
    
    // LUFS measurement standards
    static constexpr float TARGET_LUFS_STREAMING = -16.0f;    // Spotify, Apple Music
    static constexpr float TARGET_LUFS_BROADCAST = -23.0f;    // EBU R128
    static constexpr float TARGET_LUFS_MASTERING = -14.0f;    // Mastering target
    static constexpr float TARGET_LUFS_CLUB = -9.0f;         // Club/festival
    
    // Measurement windows (in samples at 48kHz)
    static constexpr int MOMENTARY_WINDOW = 19200;     // 400ms
    static constexpr int SHORT_TERM_WINDOW = 144000;   // 3 seconds
    static constexpr int INTEGRATED_MIN_TIME = 1920000; // 40 seconds minimum
    
    // Loudness measurement results
    struct LoudnessData {
        // Core measurements
        float momentaryLUFS = -120.0f;         // Current 400ms LUFS
        float shortTermLUFS = -120.0f;         // Current 3s LUFS
        float integratedLUFS = -120.0f;        // Integrated LUFS since start
        float loudnessRange = 0.0f;            // LRA (dB)
        
        // Peak measurements
        float truePeakL = -120.0f;             // True peak left (dBTP)
        float truePeakR = -120.0f;             // True peak right (dBTP)
        float maxTruePeak = -120.0f;           // Maximum true peak
        
        // Analysis metrics
        float dynamicRange = 0.0f;             // DR measurement
        float crestFactor = 0.0f;              // Peak/RMS ratio
        float stereoCorrelation = 0.0f;        // Stereo correlation
        
        // Compliance indicators
        bool meetsStreamingStandard = false;   // -16 LUFS compliance
        bool meetsBroadcastStandard = false;   // -23 LUFS compliance
        bool hasClipping = false;              // Digital clipping detected
        bool hasOverload = false;              // Overload detected
        
        // Gating information
        int gatedBlocks = 0;                   // Number of gated blocks
        int totalBlocks = 0;                   // Total measured blocks
        float gatingThreshold = -70.0f;        // Current gating threshold
        
        uint64_t measurementTime = 0;          // Total measurement time (ms)
        uint64_t timestamp = 0;                // Last update timestamp
    };
    
    // Auto-normalization modes
    enum class NormalizationMode : uint8_t {
        DISABLED = 0,
        TARGET_LUFS,           // Normalize to target LUFS
        PREVENT_CLIPPING,      // Prevent digital clipping only
        GENTLE_LIMITING,       // Gentle limiting with target
        BROADCAST_COMPLIANCE,  // EBU R128 compliance
        STREAMING_READY,       // Streaming platform ready
        MASTERING_CHAIN        // Mastering-style processing
    };
    
    // Visualization modes
    enum class DisplayMode : uint8_t {
        INTEGRATED_LUFS = 0,
        SHORT_TERM_LUFS,
        MOMENTARY_LUFS,
        TRUE_PEAK,
        LOUDNESS_RANGE,
        STEREO_CORRELATION,
        DYNAMIC_RANGE
    };
    
    LoudnessMonitor();
    ~LoudnessMonitor();
    
    // Core functionality
    void initialize(float sampleRate = SAMPLE_RATE, int channels = 2);
    void shutdown();
    void reset();
    
    // Audio processing
    void processAudioBuffer(const float* const* channelBuffers, int numChannels, int bufferSize);
    void processInterleavedBuffer(const float* buffer, int numChannels, int bufferSize);
    
    // Measurement access
    const LoudnessData& getLoudnessData() const;
    float getMomentaryLUFS() const;
    float getShortTermLUFS() const;
    float getIntegratedLUFS() const;
    float getLoudnessRange() const;
    float getTruePeak() const;
    
    // Target and compliance
    void setTargetLUFS(float targetLUFS);
    float getTargetLUFS() const { return targetLUFS_; }
    bool isCompliant(float tolerance = 1.0f) const;
    float getComplianceOffset() const;
    
    // Auto-normalization
    void setNormalizationMode(NormalizationMode mode);
    NormalizationMode getNormalizationMode() const { return normalizationMode_; }
    void enableAutoNormalization(bool enabled);
    bool isAutoNormalizationEnabled() const { return autoNormalizationEnabled_; }
    
    // Gating controls
    void setAbsoluteGatingThreshold(float threshold);
    void setRelativeGatingThreshold(float threshold);
    float getAbsoluteGatingThreshold() const { return absoluteGatingThreshold_; }
    float getRelativeGatingThreshold() const { return relativeGatingThreshold_; }
    
    // Measurement window controls
    void setMomentaryWindow(float seconds);
    void setShortTermWindow(float seconds);
    void setIntegratedMinTime(float seconds);
    
    // Analysis features
    float calculateDynamicRange() const;
    float calculateCrestFactor() const;
    float calculateStereoCorrelation() const;
    std::vector<float> getLoudnessHistory(int windowSeconds) const;
    std::vector<float> getTruePeakHistory(int windowSeconds) const;
    
    // Real-time processing
    void enableRealTimeMode(bool enabled);
    void setProcessingLatency(float maxLatencyMs);
    float getProcessingLoad() const;
    
    // Visualization
    void renderLoudnessMeter(uint32_t* displayBuffer, int width, int height,
                            DisplayMode mode = DisplayMode::INTEGRATED_LUFS) const;
    void renderLoudnessHistory(uint32_t* displayBuffer, int width, int height) const;
    void renderTruePeakMeter(uint32_t* displayBuffer, int width, int height) const;
    void renderComplianceIndicator(uint32_t* displayBuffer, int width, int height) const;
    
    // Hardware integration
    void mapToHardwareLEDs(uint8_t* ledBuffer, int ledCount) const;
    void triggerComplianceAlert(bool isCompliant) const;
    
    // Export and logging
    struct MeasurementSummary {
        float integratedLUFS;
        float loudnessRange;
        float maxTruePeak;
        float dynamicRange;
        uint64_t measurementDuration;
        bool compliant;
        std::string targetStandard;
        std::vector<float> loudnessHistogram;
    };
    
    MeasurementSummary getMeasurementSummary() const;
    void exportMeasurementData(const std::string& filename) const;
    void enableLogging(bool enabled, const std::string& logFile = "");
    
private:
    // K-weighting filter implementation (ITU-R BS.1770-4)
    struct KWeightingFilter {
        // High-frequency shelving filter (HSF)
        struct HighShelfFilter {
            float b0, b1, b2, a1, a2;
            float z1 = 0.0f, z2 = 0.0f;
            void reset() { z1 = z2 = 0.0f; }
            float process(float input);
        } hsfFilter;
        
        // High-pass filter (HPF)  
        struct HighPassFilter {
            float b0, b1, b2, a1, a2;
            float z1 = 0.0f, z2 = 0.0f;
            void reset() { z1 = z2 = 0.0f; }
            float process(float input);
        } hpfFilter;
        
        void initialize(float sampleRate);
        void reset();
        float process(float input);
    };
    
    // True peak measurement with 4x oversampling
    struct TruePeakDetector {
        static constexpr int OVERSAMPLE_FACTOR = 4;
        std::vector<float> upsampleBuffer;
        std::vector<float> filterBuffer;
        float maxTruePeak = -120.0f;
        int bufferIndex = 0;
        
        void initialize(int maxBufferSize);
        void reset();
        float process(const float* buffer, int bufferSize);
        float getCurrentPeak() const { return maxTruePeak; }
    };
    
    // Loudness measurement block
    struct LoudnessBlock {
        float meanSquare = 0.0f;           // Mean square value
        float loudness = -120.0f;          // Block loudness (LUFS)
        uint64_t timestamp = 0;            // Block timestamp
        bool gated = false;                // Gating status
    };
    
    // Audio processing
    void updateKWeightedSignal(const float* const* channelBuffers, int numChannels, int bufferSize);
    void updateLoudnessBlocks();
    void updateMomentaryLUFS();
    void updateShortTermLUFS();
    void updateIntegratedLUFS();
    void updateLoudnessRange();
    void updateTruePeaks(const float* const* channelBuffers, int numChannels, int bufferSize);
    
    // Gating implementation
    void applyAbsoluteGating(std::vector<LoudnessBlock>& blocks) const;
    void applyRelativeGating(std::vector<LoudnessBlock>& blocks) const;
    float calculateMeanLoudness(const std::vector<LoudnessBlock>& blocks) const;
    
    // Auto-normalization processing
    void processAutoNormalization(float* const* channelBuffers, int numChannels, int bufferSize);
    float calculateNormalizationGain() const;
    void applyGentleLimiting(float* const* channelBuffers, int numChannels, int bufferSize);
    
    // Member variables
    mutable std::mutex processingMutex_;
    
    // Configuration
    float sampleRate_ = SAMPLE_RATE;
    int numChannels_ = 2;
    float targetLUFS_ = TARGET_LUFS_STREAMING;
    NormalizationMode normalizationMode_ = NormalizationMode::DISABLED;
    std::atomic<bool> autoNormalizationEnabled_;
    
    // Gating thresholds
    float absoluteGatingThreshold_ = -70.0f;   // ITU-R BS.1770-4 standard
    float relativeGatingThreshold_ = -10.0f;   // Relative to ungated loudness
    
    // Window sizes (in samples)
    int momentaryWindowSize_ = MOMENTARY_WINDOW;
    int shortTermWindowSize_ = SHORT_TERM_WINDOW;
    int integratedMinSamples_ = INTEGRATED_MIN_TIME;
    
    // K-weighting filters (per channel)
    std::vector<KWeightingFilter> kWeightingFilters_;
    
    // True peak detectors (per channel)
    std::vector<TruePeakDetector> truePeakDetectors_;
    
    // Audio buffers
    std::vector<std::vector<float>> kWeightedBuffers_;  // K-weighted audio per channel
    std::deque<LoudnessBlock> loudnessBlocks_;          // Block-based measurements
    
    // Current measurements
    LoudnessData currentData_;
    
    // History buffers for visualization
    std::deque<float> momentaryHistory_;
    std::deque<float> shortTermHistory_;
    std::deque<float> truePeakHistory_;
    static constexpr int MAX_HISTORY_SIZE = 3000; // 1 minute at 50Hz update rate
    
    // Performance monitoring
    mutable std::atomic<float> processingLoad_;
    uint64_t lastProcessTime_;
    
    // Auto-normalization state
    float currentNormalizationGain_ = 1.0f;
    float targetNormalizationGain_ = 1.0f;
    
    // Utility functions
    float linearToLUFS(float linear) const;
    float LUFSToLinear(float lufs) const;
    float dBTPFromLinear(float linear) const;
    uint64_t getCurrentTimeMs() const;
    
    // Visualization helpers
    void renderMeterBar(uint32_t* displayBuffer, int width, int height,
                       float value, float minValue, float maxValue,
                       uint32_t barColor, uint32_t bgColor) const;
    uint32_t getLoudnessColor(float lufs) const;
    uint32_t getTruePeakColor(float dbtp) const;
};

} // namespace EtherSynth