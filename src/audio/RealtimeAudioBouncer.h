#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <atomic>
#include "FileHandle.h"

/**
 * RealtimeAudioBouncer - Real-time audio rendering and format conversion for tape squashing
 * 
 * Provides comprehensive real-time audio capture and processing capabilities:
 * - Real-time audio rendering from pattern data with full effects chain
 * - Multi-format output support (WAV, AIFF, RAW) with configurable bit depths
 * - Lock-free circular buffer system for real-time safe operation
 * - Dynamic sample rate conversion and format transformation
 * - Integration with sequencer for precise pattern timing
 * - Hardware-optimized for STM32 H7 embedded platform performance
 * 
 * Features:
 * - Real-time safe audio capture during pattern playback
 * - Multiple output formats: 16/24/32-bit WAV, AIFF, and raw PCM
 * - Sample rate conversion: 44.1kHz, 48kHz, 96kHz support
 * - Automatic level management with optional normalization
 * - Circular buffer system with configurable latency settings
 * - Integration with tape squashing progress indication
 * - Memory-efficient streaming write for large audio files
 * - Hardware-accelerated audio processing on STM32 H7 DSP
 */
class RealtimeAudioBouncer {
public:
    // Audio format options
    enum class AudioFormat {
        WAV_16BIT,          // 16-bit WAV file
        WAV_24BIT,          // 24-bit WAV file  
        WAV_32BIT_FLOAT,    // 32-bit floating point WAV
        AIFF_16BIT,         // 16-bit AIFF file
        AIFF_24BIT,         // 24-bit AIFF file
        RAW_PCM_16,         // Raw 16-bit PCM data
        RAW_PCM_24,         // Raw 24-bit PCM data
        RAW_PCM_32_FLOAT    // Raw 32-bit float PCM data
    };
    
    // Sample rate options
    enum class SampleRate {
        SR_44100 = 44100,
        SR_48000 = 48000,
        SR_88200 = 88200,
        SR_96000 = 96000
    };
    
    // Bounce operation status
    enum class BounceStatus {
        IDLE,               // Not currently bouncing
        INITIALIZING,       // Setting up audio pipeline
        RECORDING,          // Actively capturing audio
        PROCESSING,         // Post-processing audio data
        FINALIZING,         // Writing final file and cleanup
        COMPLETED,          // Bounce completed successfully
        CANCELLED,          // Operation cancelled by user
        ERROR              // Error occurred during bounce
    };
    
    // Audio bounce configuration
    struct BounceConfig {
        AudioFormat format;             // Output audio format
        SampleRate sampleRate;          // Output sample rate
        uint8_t channels;               // Number of audio channels (1=mono, 2=stereo)
        bool enableNormalization;       // Normalize output to full scale
        float normalizationLevel;       // Target level for normalization (-6dB = 0.5f)
        bool enableDithering;           // Apply dithering for bit depth reduction
        uint32_t bufferSizeFrames;      // Internal buffer size in frames
        uint32_t maxRecordingLengthMs;  // Maximum recording length (safety limit)
        std::string outputPath;         // Output file path
        bool overwriteExisting;         // Allow overwriting existing files
        
        BounceConfig() :
            format(AudioFormat::WAV_16BIT),
            sampleRate(SampleRate::SR_44100),
            channels(2),
            enableNormalization(true),
            normalizationLevel(0.95f),
            enableDithering(true),
            bufferSizeFrames(1024),
            maxRecordingLengthMs(300000),  // 5 minutes max
            overwriteExisting(false) {}
    };
    
    // Real-time audio processing parameters
    struct ProcessingParams {
        float inputGain;                // Input gain multiplier
        float outputGain;               // Output gain multiplier
        bool enableLimiter;             // Enable output limiting
        float limiterThreshold;         // Limiter threshold (0.0-1.0)
        float limiterRelease;           // Limiter release time (ms)
        bool enableHighpassFilter;      // Remove DC offset
        float highpassFrequency;        // Highpass filter frequency (Hz)
        
        ProcessingParams() :
            inputGain(1.0f),
            outputGain(1.0f),
            enableLimiter(true),
            limiterThreshold(0.98f),
            limiterRelease(50.0f),
            enableHighpassFilter(true),
            highpassFrequency(20.0f) {}
    };
    
    // Bounce operation result
    struct BounceResult {
        BounceStatus status;            // Final operation status
        std::string outputFilePath;     // Path to generated audio file
        uint32_t totalSamples;          // Total samples captured
        uint32_t durationMs;            // Duration of captured audio
        float peakLevel;                // Peak audio level detected
        float rmsLevel;                 // RMS audio level
        uint32_t fileSizeBytes;         // Size of output file
        std::string errorMessage;       // Error message if status == ERROR
        
        BounceResult() :
            status(BounceStatus::IDLE),
            totalSamples(0),
            durationMs(0),
            peakLevel(0.0f),
            rmsLevel(0.0f),
            fileSizeBytes(0) {}
    };
    
    // Real-time audio metrics
    struct AudioMetrics {
        float currentPeakLevel;         // Current peak level
        float currentRmsLevel;          // Current RMS level
        uint32_t samplesProcessed;      // Samples processed so far
        uint32_t bufferUnderruns;       // Number of buffer underruns
        uint32_t bufferOverruns;        // Number of buffer overruns
        float cpuLoad;                  // CPU load percentage
        uint32_t latencyMs;             // Current audio latency
        
        AudioMetrics() :
            currentPeakLevel(0.0f),
            currentRmsLevel(0.0f),
            samplesProcessed(0),
            bufferUnderruns(0),
            bufferOverruns(0),
            cpuLoad(0.0f),
            latencyMs(0) {}
    };
    
    // Captured audio data structure
    struct CapturedAudio {
        std::vector<float> audioData;       // Interleaved audio samples
        uint32_t sampleCount;               // Number of samples per channel
        uint8_t channels;                   // Number of audio channels
        uint32_t sampleRate;                // Sample rate in Hz
        float peakLevel;                    // Peak level in dB
        float rmsLevel;                     // RMS level in dB
        AudioFormat format;                 // Audio format information
        
        CapturedAudio() :
            sampleCount(0), channels(2), sampleRate(48000),
            peakLevel(-96.0f), rmsLevel(-96.0f), format(AudioFormat::WAV_24BIT) {}
    };
    
    RealtimeAudioBouncer();
    ~RealtimeAudioBouncer();
    
    // Configuration
    void setBounceConfig(const BounceConfig& config);
    const BounceConfig& getBounceConfig() const { return config_; }
    void setProcessingParams(const ProcessingParams& params);
    const ProcessingParams& getProcessingParams() const { return processingParams_; }
    
    // Bounce Operations
    bool startBounce(const std::string& outputPath, uint32_t durationMs);
    bool pauseBounce();
    bool resumeBounce();
    void stopBounce();
    void cancelBounce();
    
    // State Management
    BounceStatus getStatus() const { return status_.load(); }
    bool isActive() const { return status_.load() != BounceStatus::IDLE; }
    bool isRecording() const { return status_.load() == BounceStatus::RECORDING; }
    AudioMetrics getCurrentMetrics() const;
    float getProgressPercentage() const;
    uint32_t getElapsedTimeMs() const;
    uint32_t getRemainingTimeMs() const;
    
    // Audio Processing
    void processAudioBlock(const float* inputBuffer, uint32_t sampleCount);
    void processInterleavedStereo(const float* leftBuffer, const float* rightBuffer, uint32_t sampleCount);
    void processMono(const float* inputBuffer, uint32_t sampleCount);
    
    // Format Conversion
    void convertToOutputFormat(const float* inputBuffer, uint32_t sampleCount);
    std::vector<uint8_t> convertFloatToFormat(const float* samples, uint32_t sampleCount, AudioFormat format);
    void applySampleRateConversion(const float* input, uint32_t inputSamples, 
                                  float* output, uint32_t& outputSamples);
    
    // Audio Analysis
    void updateLevelMeters(const float* buffer, uint32_t sampleCount);
    float calculatePeakLevel(const float* buffer, uint32_t sampleCount);
    float calculateRmsLevel(const float* buffer, uint32_t sampleCount);
    void resetLevelMeters();
    
    // File I/O
    bool createOutputFile(const std::string& path);
    bool writeAudioData(const uint8_t* data, uint32_t dataSize);
    bool finalizeOutputFile();
    void writeWavHeader(uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample, uint32_t dataSize);
    void writeAiffHeader(uint32_t sampleRate, uint16_t channels, uint16_t bitsPerSample, uint32_t dataSize);
    
    // Buffer Management
    bool initializeBuffers();
    void resetBuffers();
    void flushBuffers();
    uint32_t getBufferUsage() const;
    bool hasBufferUnderrun() const;
    bool hasBufferOverrun() const;
    
    // Callbacks
    using ProgressCallback = std::function<void(float percentage, uint32_t elapsedMs, uint32_t remainingMs)>;
    using StatusCallback = std::function<void(BounceStatus status, const std::string& message)>;
    using MetricsCallback = std::function<void(const AudioMetrics& metrics)>;
    using CompletedCallback = std::function<void(const BounceResult& result)>;
    
    void setProgressCallback(ProgressCallback callback) { progressCallback_ = callback; }
    void setStatusCallback(StatusCallback callback) { statusCallback_ = callback; }
    void setMetricsCallback(MetricsCallback callback) { metricsCallback_ = callback; }
    void setCompletedCallback(CompletedCallback callback) { completedCallback_ = callback; }
    
    // Integration
    void integrateWithSequencer(std::function<void(float*, uint32_t)> audioCallback);
    void integrateWithEffectsChain(std::function<void(float*, uint32_t)> effectsCallback);
    void setSampleRateCallback(std::function<uint32_t()> callback) { sampleRateCallback_ = callback; }
    
    // Performance Analysis
    size_t getEstimatedMemoryUsage() const;
    float getAverageCpuLoad() const;
    uint32_t getTotalSamplesProcessed() const;
    void resetPerformanceCounters();

private:
    // Configuration
    BounceConfig config_;
    ProcessingParams processingParams_;
    
    // State
    std::atomic<BounceStatus> status_;
    uint32_t targetDurationMs_;
    uint32_t startTimeMs_;
    uint32_t samplesRecorded_;
    uint32_t targetSampleCount_;
    
    // Audio buffers
    std::vector<float> circularBuffer_;
    std::atomic<uint32_t> writeIndex_;
    std::atomic<uint32_t> readIndex_;
    uint32_t bufferSize_;
    
    // Processing state
    float peakLevel_;
    float rmsLevel_;
    float rmsAccumulator_;
    uint32_t rmsSampleCount_;
    
    // File I/O
    FileHandle outputFile_;
    std::string currentOutputPath_;
    uint32_t bytesWritten_;
    
    // Sample rate conversion
    std::vector<float> srcBuffer_;
    float srcRatio_;
    uint32_t srcState_;
    
    // Audio processing
    float limiterState_;
    float highpassState_[2];  // Biquad filter state
    
    // Performance metrics
    AudioMetrics currentMetrics_;
    uint32_t performanceUpdateCounter_;
    uint32_t totalProcessingTime_;
    
    // Integration callbacks
    std::function<void(float*, uint32_t)> sequencerCallback_;
    std::function<void(float*, uint32_t)> effectsCallback_;
    std::function<uint32_t()> sampleRateCallback_;
    
    // User callbacks
    ProgressCallback progressCallback_;
    StatusCallback statusCallback_;
    MetricsCallback metricsCallback_;
    CompletedCallback completedCallback_;
    
    // Internal methods
    void updateStatus(BounceStatus newStatus, const std::string& message = "");
    void updateProgress();
    void updateMetrics();
    
    // Audio processing helpers
    void processLimiter(float* buffer, uint32_t sampleCount);
    void processHighpassFilter(float* buffer, uint32_t sampleCount);
    void applyDithering(float* buffer, uint32_t sampleCount);
    void applyNormalization(float* buffer, uint32_t sampleCount, float targetLevel);
    
    // Format conversion helpers
    void convertToInt16(const float* input, int16_t* output, uint32_t sampleCount);
    void convertToInt24(const float* input, uint8_t* output, uint32_t sampleCount);
    void convertToFloat32(const float* input, float* output, uint32_t sampleCount);
    
    // Buffer management helpers
    uint32_t getAvailableWriteSpace() const;
    uint32_t getAvailableReadSpace() const;
    bool writeToBuffer(const float* data, uint32_t sampleCount);
    bool readFromBuffer(float* data, uint32_t sampleCount);
    
    // File format helpers
    void writeInt16LE(uint16_t value);
    void writeInt32LE(uint32_t value);
    void writeFloat32LE(float value);
    uint32_t calculateWavDataSize(uint32_t sampleCount, uint16_t channels, uint16_t bitsPerSample);
    uint32_t calculateAiffDataSize(uint32_t sampleCount, uint16_t channels, uint16_t bitsPerSample);
    
    // Validation helpers
    bool validateConfig(const BounceConfig& config) const;
    bool validateOutputPath(const std::string& path) const;
    void sanitizeProcessingParams(ProcessingParams& params) const;
    
    // Utility methods
    uint32_t getCurrentTimeMs() const;
    std::string getFormatExtension(AudioFormat format) const;
    uint32_t getBitsPerSample(AudioFormat format) const;
    bool isFloatFormat(AudioFormat format) const;
    
    // Constants
    static constexpr uint32_t DEFAULT_BUFFER_SIZE = 8192;
    static constexpr uint32_t MAX_BUFFER_SIZE = 65536;
    static constexpr uint32_t MIN_BUFFER_SIZE = 512;
    static constexpr float LEVEL_METER_DECAY = 0.99f;
    static constexpr float RMS_WINDOW_SIZE = 0.3f;  // 300ms RMS window
    static constexpr uint32_t METRICS_UPDATE_INTERVAL = 100;  // Update every 100 samples
    static constexpr float DEFAULT_LIMITER_RATIO = 10.0f;
};