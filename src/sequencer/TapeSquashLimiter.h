#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <functional>

/**
 * TapeSquashLimiter - Track count limits and resource management for squashing operations
 * 
 * Provides intelligent limitation and optimization for tape squashing workflow:
 * - Configurable track count limits based on system performance
 * - Dynamic limit adjustment based on available memory and CPU load
 * - Track priority system for automatic selection when over limits
 * - Resource usage estimation and optimization recommendations
 * - Integration with pattern data replacer for seamless workflow
 * - Hardware-optimized for STM32 H7 embedded platform constraints
 * 
 * Features:
 * - Configurable track limits (default 4-8 tracks maximum)
 * - Dynamic performance-based limit adjustment
 * - Track complexity analysis for optimal selection
 * - Memory usage prediction and warnings
 * - CPU load estimation for real-time audio processing
 * - Automatic track consolidation suggestions
 * - Integration with TouchGFX for user feedback and selection
 * - Support for future expansion as hardware performance improves
 */
class TapeSquashLimiter {
public:
    // Limit enforcement modes
    enum class LimitMode {
        STRICT_LIMIT,       // Hard limit - reject operations over limit
        WARNING_LIMIT,      // Warn user but allow operations over limit  
        DYNAMIC_LIMIT,      // Adjust limit based on system performance
        PERFORMANCE_BASED   // Analyze each operation individually
    };
    
    // Track complexity factors
    enum class ComplexityFactor {
        TRACK_COUNT,        // Number of active tracks
        EFFECT_COUNT,       // Number of effects per track
        MODULATION_DEPTH,   // Amount of parameter modulation
        AUDIO_DENSITY,      // Audio content complexity
        PROCESSING_LOAD     // Real-time processing requirements
    };
    
    // Resource limit configuration
    struct LimitConfig {
        uint8_t maxTracks;              // Maximum tracks for squashing
        uint8_t recommendedTracks;      // Recommended track count for optimal performance
        uint8_t warningThreshold;       // Track count that triggers warning
        LimitMode mode;                 // How limits are enforced
        uint32_t maxMemoryUsageKB;      // Maximum memory usage for operation (KB)
        float maxCpuLoadPercentage;     // Maximum CPU load percentage
        bool enableDynamicAdjustment;   // Allow automatic limit adjustment
        bool showPerformanceWarnings;   // Display performance warnings to user
        
        LimitConfig() :
            maxTracks(6),
            recommendedTracks(4),
            warningThreshold(5),
            mode(LimitMode::WARNING_LIMIT),
            maxMemoryUsageKB(2048),  // 2MB limit for audio buffers
            maxCpuLoadPercentage(75.0f),
            enableDynamicAdjustment(true),
            showPerformanceWarnings(true) {}
    };
    
    // Track analysis result
    struct TrackAnalysis {
        uint8_t trackId;                // Track identifier
        uint8_t activeSteps;            // Number of active steps
        uint8_t effectCount;            // Number of effects on track
        uint8_t modulationCount;        // Number of modulated parameters
        float estimatedCpuLoad;         // Estimated CPU load (0.0-1.0)
        uint32_t estimatedMemoryKB;     // Estimated memory usage (KB)
        float complexityScore;          // Overall complexity score (0.0-1.0)
        uint8_t priority;               // Selection priority (higher = more important)
        bool isRecommended;             // Whether track should be included
        
        TrackAnalysis() :
            trackId(255),
            activeSteps(0),
            effectCount(0),
            modulationCount(0),
            estimatedCpuLoad(0.0f),
            estimatedMemoryKB(0),
            complexityScore(0.0f),
            priority(128),
            isRecommended(true) {}
    };
    
    // Squash operation analysis
    struct SquashAnalysis {
        std::vector<TrackAnalysis> trackAnalyses;   // Per-track analysis
        std::vector<uint8_t> recommendedTracks;     // Recommended track selection
        std::vector<uint8_t> alternativeSelections; // Alternative selections if over limit
        uint32_t totalEstimatedMemoryKB;            // Total memory estimate
        float totalEstimatedCpuLoad;                // Total CPU load estimate
        uint32_t estimatedProcessingTimeMs;         // Estimated processing time
        bool withinLimits;                          // Whether operation is within limits
        bool requiresOptimization;                  // Whether optimization is needed
        std::string recommendations;                // User-readable recommendations
        std::string warningMessage;                 // Warning message if applicable
        
        SquashAnalysis() :
            totalEstimatedMemoryKB(0),
            totalEstimatedCpuLoad(0.0f),
            estimatedProcessingTimeMs(0),
            withinLimits(true),
            requiresOptimization(false) {}
    };
    
    // Performance metrics
    struct PerformanceMetrics {
        uint32_t successfulOperations;      // Number of successful squash operations
        uint32_t rejectedOperations;        // Number of rejected operations
        uint32_t warningOperations;         // Number of operations with warnings
        float averageProcessingTime;        // Average processing time (seconds)
        float averageMemoryUsage;           // Average memory usage (KB)
        float averageCpuLoad;               // Average CPU load during operations
        uint32_t lastOperationTime;         // Time of last operation
        
        PerformanceMetrics() :
            successfulOperations(0),
            rejectedOperations(0),
            warningOperations(0),
            averageProcessingTime(0.0f),
            averageMemoryUsage(0.0f),
            averageCpuLoad(0.0f),
            lastOperationTime(0) {}
    };
    
    TapeSquashLimiter();
    ~TapeSquashLimiter() = default;
    
    // Configuration
    void setLimitConfig(const LimitConfig& config);
    const LimitConfig& getLimitConfig() const { return config_; }
    void setMaxTracks(uint8_t maxTracks);
    void setLimitMode(LimitMode mode);
    void setMemoryLimit(uint32_t maxMemoryKB);
    void setCpuLimit(float maxCpuPercentage);
    
    // Track Analysis
    SquashAnalysis analyzeSquashOperation(const std::vector<uint8_t>& trackIds, 
                                         uint8_t startStep, uint8_t endStep);
    TrackAnalysis analyzeTrack(uint8_t trackId, uint8_t startStep, uint8_t endStep);
    std::vector<uint8_t> selectOptimalTracks(const std::vector<uint8_t>& candidateTracks, 
                                           uint8_t maxTracks);
    
    // Limit Enforcement
    bool checkTrackCountLimit(uint8_t trackCount) const;
    bool checkMemoryLimit(uint32_t estimatedMemoryKB) const;
    bool checkCpuLimit(float estimatedCpuLoad) const;
    bool isOperationAllowed(const SquashAnalysis& analysis) const;
    std::string getRecommendations(const SquashAnalysis& analysis) const;
    
    // Dynamic Limit Adjustment
    void updateDynamicLimits();
    void adjustLimitsForSystemLoad();
    void adjustLimitsForMemoryPressure();
    uint8_t getEffectiveTrackLimit() const;
    
    // Track Selection Optimization
    std::vector<uint8_t> rankTracksByComplexity(const std::vector<uint8_t>& trackIds, 
                                               uint8_t startStep, uint8_t endStep);
    std::vector<uint8_t> rankTracksByPriority(const std::vector<uint8_t>& trackIds);
    std::vector<uint8_t> selectTracksByComplexityBudget(const std::vector<uint8_t>& trackIds,
                                                       float complexityBudget);
    
    // Resource Estimation
    uint32_t estimateMemoryUsage(const std::vector<uint8_t>& trackIds, 
                                 uint8_t startStep, uint8_t endStep) const;
    float estimateCpuLoad(const std::vector<uint8_t>& trackIds, 
                         uint8_t startStep, uint8_t endStep) const;
    uint32_t estimateProcessingTime(const std::vector<uint8_t>& trackIds, 
                                   uint8_t startStep, uint8_t endStep) const;
    
    // Performance Monitoring
    void recordOperationStart(const SquashAnalysis& analysis);
    void recordOperationComplete(bool success, uint32_t actualTimeMs, 
                                uint32_t actualMemoryKB, float actualCpuLoad);
    void recordOperationRejected(const std::string& reason);
    const PerformanceMetrics& getPerformanceMetrics() const { return metrics_; }
    void resetPerformanceMetrics();
    
    // User Interface Integration
    std::vector<std::string> getWarningMessages(const SquashAnalysis& analysis) const;
    std::vector<std::string> getOptimizationSuggestions(const SquashAnalysis& analysis) const;
    std::string formatResourceUsage(uint32_t memoryKB, float cpuLoad) const;
    std::string formatRecommendedSelection(const std::vector<uint8_t>& trackIds) const;
    
    // Callbacks
    using LimitExceededCallback = std::function<void(const SquashAnalysis&, const std::string&)>;
    using OptimizationSuggestedCallback = std::function<void(const SquashAnalysis&, const std::vector<std::string>&)>;
    using PerformanceWarningCallback = std::function<void(const std::string&)>;
    
    void setLimitExceededCallback(LimitExceededCallback callback) { limitExceededCallback_ = callback; }
    void setOptimizationSuggestedCallback(OptimizationSuggestedCallback callback) { optimizationSuggestedCallback_ = callback; }
    void setPerformanceWarningCallback(PerformanceWarningCallback callback) { performanceWarningCallback_ = callback; }
    
    // System Integration
    void integrateWithSystemMonitor(std::function<float()> getCpuLoad, 
                                   std::function<uint32_t()> getMemoryUsage);
    void integrateWithSequencer(std::function<bool(uint8_t)> hasActiveSteps,
                               std::function<uint8_t(uint8_t)> getEffectCount);
    
    // Performance Analysis
    size_t getEstimatedMemoryUsage() const;
    float getAverageAnalysisTime() const;
    uint8_t getCurrentTrackLimit() const;
    bool isDynamicLimitingActive() const;

private:
    // Configuration
    LimitConfig config_;
    uint8_t effectiveTrackLimit_;
    
    // Performance tracking
    PerformanceMetrics metrics_;
    std::vector<uint32_t> recentAnalysisTimes_;
    uint32_t totalAnalysisTime_;
    uint32_t analysisCount_;
    
    // System integration
    std::function<float()> systemCpuLoadCallback_;
    std::function<uint32_t()> systemMemoryCallback_;
    std::function<bool(uint8_t)> hasActiveStepsCallback_;
    std::function<uint8_t(uint8_t)> getEffectCountCallback_;
    
    // Callbacks
    LimitExceededCallback limitExceededCallback_;
    OptimizationSuggestedCallback optimizationSuggestedCallback_;
    PerformanceWarningCallback performanceWarningCallback_;
    
    // Internal methods
    float calculateTrackComplexity(uint8_t trackId, uint8_t startStep, uint8_t endStep) const;
    uint8_t calculateTrackPriority(uint8_t trackId) const;
    bool isTrackActive(uint8_t trackId, uint8_t startStep, uint8_t endStep) const;
    uint8_t countActiveSteps(uint8_t trackId, uint8_t startStep, uint8_t endStep) const;
    
    // Resource calculation helpers
    uint32_t calculateTrackMemoryUsage(uint8_t trackId, uint8_t stepCount) const;
    float calculateTrackCpuLoad(uint8_t trackId, uint8_t stepCount, uint8_t effectCount) const;
    uint32_t calculateProcessingTime(uint32_t memoryKB, float cpuLoad) const;
    
    // Dynamic adjustment helpers
    float getCurrentSystemCpuLoad() const;
    uint32_t getCurrentSystemMemoryUsage() const;
    void updateEffectiveTrackLimit();
    
    // Analysis helpers
    std::vector<TrackAnalysis> analyzeAllTracks(const std::vector<uint8_t>& trackIds, 
                                               uint8_t startStep, uint8_t endStep);
    void rankAnalysesByComplexity(std::vector<TrackAnalysis>& analyses) const;
    void rankAnalysesByPriority(std::vector<TrackAnalysis>& analyses) const;
    
    // Validation helpers
    void validateTrackSelection(std::vector<uint8_t>& trackIds) const;
    void sanitizeStepRange(uint8_t& startStep, uint8_t& endStep) const;
    void validateLimitConfig(LimitConfig& config) const;
    
    // Notification helpers
    void notifyLimitExceeded(const SquashAnalysis& analysis, const std::string& reason);
    void notifyOptimizationSuggested(const SquashAnalysis& analysis);
    void notifyPerformanceWarning(const std::string& message);
    
    // Utility methods
    uint32_t getCurrentTimeMs() const;
    std::string generateRecommendationText(const SquashAnalysis& analysis) const;
    std::string generateWarningMessage(const SquashAnalysis& analysis) const;
    
    // Constants
    static constexpr uint8_t MIN_TRACK_LIMIT = 1;
    static constexpr uint8_t MAX_TRACK_LIMIT = 16;
    static constexpr uint8_t DEFAULT_TRACK_LIMIT = 6;
    static constexpr uint32_t MIN_MEMORY_LIMIT_KB = 128;   // 128KB minimum
    static constexpr uint32_t MAX_MEMORY_LIMIT_KB = 8192;  // 8MB maximum
    static constexpr float MIN_CPU_LIMIT = 10.0f;          // 10% minimum
    static constexpr float MAX_CPU_LIMIT = 95.0f;          // 95% maximum
    static constexpr uint32_t MEMORY_PER_TRACK_KB = 64;    // Base memory per track
    static constexpr float CPU_LOAD_PER_TRACK = 0.08f;     // Base CPU load per track
    static constexpr uint32_t PERFORMANCE_HISTORY_SIZE = 50;
};