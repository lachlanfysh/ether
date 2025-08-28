#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include "../sequencer/PatternSelection.h"
#include "RealtimeAudioBouncer.h"

/**
 * AudioCaptureSystem - Audio capture of all tracks/effects/modulation within selection
 * 
 * Provides comprehensive audio capture functionality for selected pattern regions:
 * - Multi-track audio capture with individual track isolation
 * - Full effects chain capture (pre-FX, post-FX, master effects)
 * - Modulation source capture and playback synchronization
 * - Real-time safe capture pipeline with minimal audio thread impact
 * - Track grouping and submix capture capabilities
 * - Hardware-optimized for STM32 H7 embedded platform
 * 
 * Features:
 * - Individual track capture with effects chain processing
 * - Master bus capture including all processing and effects
 * - Modulation capture (velocity, LFOs, envelopes, macro controls)
 * - Selective capture modes (dry tracks, wet tracks, master mix)
 * - Multi-format export (mono, stereo, individual stems)
 * - Real-time analysis and metering during capture
 * - Integration with sequencer timing and pattern playback
 * - Memory-efficient streaming capture for multi-track sessions
 */
class AudioCaptureSystem {
public:
    // Capture modes for different audio sources
    enum class CaptureMode {
        MASTER_MIX,         // Capture final master mix
        INDIVIDUAL_TRACKS,  // Capture each track separately
        TRACK_STEMS,        // Capture track groups/stems
        DRY_SIGNALS,        // Capture dry signals (pre-FX)
        WET_SIGNALS,        // Capture wet signals (post-FX)
        MODULATION_ONLY,    // Capture modulation signals only
        FULL_MULTITRACK     // Capture everything simultaneously
    };
    
    // Track capture configuration
    struct TrackCaptureConfig {
        uint8_t trackId;                // Track to capture (0-15)
        bool capturePreFX;              // Capture before effects
        bool capturePostFX;             // Capture after effects
        bool includeModulation;         // Include modulation in capture
        bool enableSolo;                // Solo this track during capture
        float gainAdjustment;           // Gain adjustment (-12dB to +12dB)
        bool enableGating;              // Noise gate for quiet passages
        float gateThreshold;            // Gate threshold (-60dB to 0dB)
        
        TrackCaptureConfig() :
            trackId(0),
            capturePreFX(false),
            capturePostFX(true),
            includeModulation(true),
            enableSolo(false),
            gainAdjustment(0.0f),
            enableGating(false),
            gateThreshold(-40.0f) {}
    };
    
    // Capture session configuration
    struct CaptureSessionConfig {
        PatternSelection::SelectionBounds selection;    // Pattern region to capture
        CaptureMode mode;                              // Capture mode
        std::vector<TrackCaptureConfig> tracks;        // Per-track configurations
        RealtimeAudioBouncer::AudioFormat format;      // Audio format settings
        bool enableClickTrack;                         // Include click/metronome
        bool enableCountIn;                            // Count-in before capture
        uint8_t countInBars;                          // Number of count-in bars
        bool captureMasterEffects;                     // Include master effects
        bool captureAutomation;                        // Capture parameter automation
        std::string sessionName;                       // Name for the capture session
        
        CaptureSessionConfig() :
            mode(CaptureMode::MASTER_MIX),
            enableClickTrack(false),
            enableCountIn(true),
            countInBars(1),
            captureMasterEffects(true),
            captureAutomation(true),
            sessionName("Capture") {}
    };
    
    // Individual track capture result
    struct TrackCaptureResult {
        uint8_t trackId;
        std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> preFXAudio;
        std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> postFXAudio;
        std::vector<float> modulationData;  // Modulation signal capture
        float peakLevel;
        float rmsLevel;
        bool wasClipped;
        std::string trackName;
        
        TrackCaptureResult() :
            trackId(0),
            peakLevel(-96.0f),
            rmsLevel(-96.0f),
            wasClipped(false) {}
    };
    
    // Complete capture session result
    struct CaptureSessionResult {
        std::string sessionName;
        CaptureMode mode;
        std::shared_ptr<RealtimeAudioBouncer::CapturedAudio> masterMix;
        std::vector<TrackCaptureResult> trackResults;
        RealtimeAudioBouncer::AudioFormat format;
        uint32_t totalSampleCount;
        float sessionPeakLevel;
        float sessionRMSLevel;
        bool anyTrackClipped;
        uint32_t captureDurationMs;
        
        CaptureSessionResult() :
            mode(CaptureMode::MASTER_MIX),
            totalSampleCount(0),
            sessionPeakLevel(-96.0f),
            sessionRMSLevel(-96.0f),
            anyTrackClipped(false),
            captureDurationMs(0) {}
    };
    
    // Capture progress tracking
    struct CaptureSessionProgress {
        uint8_t currentTrack;               // Currently capturing track
        uint8_t totalTracks;                // Total tracks to capture
        float overallProgress;              // Overall progress (0.0-100.0)
        float currentTrackProgress;         // Current track progress (0.0-100.0)
        uint32_t elapsedTimeMs;            // Total elapsed time
        uint32_t estimatedRemainingMs;      // Estimated remaining time
        std::string currentOperation;       // Current operation description
        
        CaptureSessionProgress() :
            currentTrack(0),
            totalTracks(0),
            overallProgress(0.0f),
            currentTrackProgress(0.0f),
            elapsedTimeMs(0),
            estimatedRemainingMs(0),
            currentOperation("Preparing") {}
    };
    
    AudioCaptureSystem();
    ~AudioCaptureSystem();
    
    // Session Management
    bool startCaptureSession(const CaptureSessionConfig& config);
    void cancelCaptureSession();
    bool isCaptureSessionActive() const;
    
    // Configuration
    void setCaptureSessionConfig(const CaptureSessionConfig& config);
    const CaptureSessionConfig& getCaptureSessionConfig() const { return sessionConfig_; }
    void addTrackToCapture(const TrackCaptureConfig& trackConfig);
    void removeTrackFromCapture(uint8_t trackId);
    void clearTrackConfigurations();
    
    // Progress Monitoring
    CaptureSessionProgress getCaptureProgress() const;
    void updateProgress();
    
    // Audio Processing Integration
    void processTrackAudio(uint8_t trackId, const float* preFXBuffer, const float* postFXBuffer,
                          uint32_t sampleCount, uint8_t channelCount);
    void processMasterAudio(const float* masterBuffer, uint32_t sampleCount, uint8_t channelCount);
    void processModulationSignal(uint8_t trackId, uint32_t parameterId, float modulationValue);
    
    // Sequencer Integration
    void notifyPatternStart();
    void notifyPatternEnd();
    void notifySelectionRegionStart();
    void notifySelectionRegionEnd();
    void updateSequencerPosition(uint16_t currentTrack, uint16_t currentStep);
    
    // Results Management
    bool hasCaptureResult() const;
    std::shared_ptr<CaptureSessionResult> getCaptureResult();
    void clearCaptureResult();
    
    // Individual track results
    std::shared_ptr<TrackCaptureResult> getTrackResult(uint8_t trackId);
    std::vector<uint8_t> getCompletedTrackIds() const;
    
    // Integration
    void integrateWithRealtimeBouncer(std::shared_ptr<RealtimeAudioBouncer> bouncer);
    void integrateWithSequencer(class SequencerEngine* sequencer);
    
    // Callbacks
    using SessionProgressCallback = std::function<void(const CaptureSessionProgress&)>;
    using SessionCompleteCallback = std::function<void(std::shared_ptr<CaptureSessionResult>)>;
    using TrackCompleteCallback = std::function<void(uint8_t trackId, std::shared_ptr<TrackCaptureResult>)>;
    using ErrorCallback = std::function<void(const std::string& error)>;
    
    void setSessionProgressCallback(SessionProgressCallback callback);
    void setSessionCompleteCallback(SessionCompleteCallback callback);
    void setTrackCompleteCallback(TrackCompleteCallback callback);
    void setErrorCallback(ErrorCallback callback);
    
    // Memory and Performance
    size_t getEstimatedMemoryUsage() const;
    bool hasEnoughMemoryForSession(const CaptureSessionConfig& config) const;
    void optimizeForLowLatency(bool enable);
    
private:
    // Session state
    std::atomic<bool> sessionActive_;
    CaptureSessionConfig sessionConfig_;
    CaptureSessionProgress sessionProgress_;
    
    // Audio bouncers for different capture targets
    std::shared_ptr<RealtimeAudioBouncer> masterBouncer_;
    std::unordered_map<uint8_t, std::shared_ptr<RealtimeAudioBouncer>> trackBouncers_;  // Per-track bouncers
    
    // Capture state tracking
    std::atomic<bool> isCapturingRegion_;
    std::atomic<uint8_t> currentCaptureTrack_;
    std::atomic<uint32_t> sessionStartTime_;
    
    // Results storage
    std::shared_ptr<CaptureSessionResult> sessionResult_;
    std::unordered_map<uint8_t, std::shared_ptr<TrackCaptureResult>> trackResults_;
    mutable std::mutex resultsMutex_;
    
    // Audio analysis per track
    struct TrackAnalysis {
        std::atomic<float> peakLevel;
        std::atomic<float> rmsLevel;
        std::atomic<bool> hasClipped;
        std::vector<float> modulationCapture;  // Real-time modulation data
        
        TrackAnalysis() : peakLevel(-96.0f), rmsLevel(-96.0f), hasClipped(false) {}
    };
    std::unordered_map<uint8_t, std::unique_ptr<TrackAnalysis>> trackAnalysis_;
    
    // Master audio analysis
    std::atomic<float> masterPeakLevel_;
    std::atomic<float> masterRMSLevel_;
    std::atomic<bool> masterHasClipped_;
    
    // Integration
    class SequencerEngine* sequencer_;
    
    // Callbacks
    SessionProgressCallback sessionProgressCallback_;
    SessionCompleteCallback sessionCompleteCallback_;
    TrackCompleteCallback trackCompleteCallback_;
    ErrorCallback errorCallback_;
    
    // Performance settings
    bool lowLatencyMode_;
    
    // Internal methods
    bool initializeCapturePipeline();
    void cleanupCapturePipeline();
    void startTrackCapture(uint8_t trackId);
    void finalizeCaptureSession();
    void processTrackCaptureComplete(uint8_t trackId);
    
    // Audio processing helpers
    void updateTrackAnalysis(uint8_t trackId, const float* buffer, uint32_t sampleCount);
    void updateMasterAnalysis(const float* buffer, uint32_t sampleCount);
    float calculateRMS(const float* buffer, uint32_t sampleCount);
    float findPeak(const float* buffer, uint32_t sampleCount);
    
    // Configuration helpers
    bool validateSessionConfig(const CaptureSessionConfig& config) const;
    void setupTrackBouncers();
    void setupMasterBouncer();
    
    // Progress calculation
    void calculateOverallProgress();
    void updateTimeEstimates();
    
    // Memory management
    size_t calculateMemoryRequirement(const CaptureSessionConfig& config) const;
    void optimizeMemoryUsage();
    
    // Error handling
    void handleSessionError(const std::string& error);
    void notifySessionProgress();
    void notifySessionComplete();
    void notifyTrackComplete(uint8_t trackId);
    
    // Utility methods
    uint32_t getCurrentTimeMs() const;
    float linearToDb(float linear) const;
    
    // Constants
    static constexpr uint8_t MAX_TRACKS = 16;
    static constexpr uint32_t MODULATION_CAPTURE_RATE = 1000;  // 1kHz modulation sampling
    static constexpr size_t MAX_MODULATION_SAMPLES = 30000;    // 30 seconds at 1kHz
};