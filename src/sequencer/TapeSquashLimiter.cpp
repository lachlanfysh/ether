#include "TapeSquashLimiter.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <chrono>

TapeSquashLimiter::TapeSquashLimiter() :
    config_(LimitConfig()),
    effectiveTrackLimit_(DEFAULT_TRACK_LIMIT),
    totalAnalysisTime_(0),
    analysisCount_(0) {
    
    recentAnalysisTimes_.reserve(PERFORMANCE_HISTORY_SIZE);
    updateEffectiveTrackLimit();
}

// Configuration
void TapeSquashLimiter::setLimitConfig(const LimitConfig& config) {
    LimitConfig validatedConfig = config;
    validateLimitConfig(validatedConfig);
    config_ = validatedConfig;
    updateEffectiveTrackLimit();
}

void TapeSquashLimiter::setMaxTracks(uint8_t maxTracks) {
    config_.maxTracks = std::max(MIN_TRACK_LIMIT, std::min(maxTracks, MAX_TRACK_LIMIT));
    updateEffectiveTrackLimit();
}

void TapeSquashLimiter::setLimitMode(LimitMode mode) {
    config_.mode = mode;
    updateEffectiveTrackLimit();
}

void TapeSquashLimiter::setMemoryLimit(uint32_t maxMemoryKB) {
    config_.maxMemoryUsageKB = std::max(MIN_MEMORY_LIMIT_KB, std::min(maxMemoryKB, MAX_MEMORY_LIMIT_KB));
}

void TapeSquashLimiter::setCpuLimit(float maxCpuPercentage) {
    config_.maxCpuLoadPercentage = std::max(MIN_CPU_LIMIT, std::min(maxCpuPercentage, MAX_CPU_LIMIT));
}

// Track Analysis
TapeSquashLimiter::SquashAnalysis TapeSquashLimiter::analyzeSquashOperation(
    const std::vector<uint8_t>& trackIds, uint8_t startStep, uint8_t endStep) {
    
    uint32_t analysisStartTime = getCurrentTimeMs();
    
    SquashAnalysis analysis;
    
    // Validate input
    std::vector<uint8_t> validatedTracks = trackIds;
    validateTrackSelection(validatedTracks);
    sanitizeStepRange(startStep, endStep);
    
    // Analyze each track
    analysis.trackAnalyses = analyzeAllTracks(validatedTracks, startStep, endStep);
    
    // Calculate totals
    analysis.totalEstimatedMemoryKB = 0;
    analysis.totalEstimatedCpuLoad = 0.0f;
    
    for (const auto& trackAnalysis : analysis.trackAnalyses) {
        analysis.totalEstimatedMemoryKB += trackAnalysis.estimatedMemoryKB;
        analysis.totalEstimatedCpuLoad += trackAnalysis.estimatedCpuLoad;
    }
    
    // Estimate processing time
    analysis.estimatedProcessingTimeMs = calculateProcessingTime(
        analysis.totalEstimatedMemoryKB, analysis.totalEstimatedCpuLoad);
    
    // Check limits
    analysis.withinLimits = isOperationAllowed(analysis);
    
    // Generate recommendations if needed
    if (!analysis.withinLimits || validatedTracks.size() > config_.warningThreshold) {
        analysis.requiresOptimization = true;
        analysis.recommendedTracks = selectOptimalTracks(validatedTracks, config_.recommendedTracks);
        
        // Generate alternative selections if over limit
        if (validatedTracks.size() > effectiveTrackLimit_) {
            analysis.alternativeSelections = selectOptimalTracks(validatedTracks, effectiveTrackLimit_);
        }
        
        analysis.recommendations = generateRecommendationText(analysis);
        analysis.warningMessage = generateWarningMessage(analysis);
    } else {
        analysis.recommendedTracks = validatedTracks;
    }
    
    // Update performance tracking
    uint32_t analysisTime = getCurrentTimeMs() - analysisStartTime;
    if (recentAnalysisTimes_.size() >= PERFORMANCE_HISTORY_SIZE) {
        recentAnalysisTimes_.erase(recentAnalysisTimes_.begin());
    }
    recentAnalysisTimes_.push_back(analysisTime);
    totalAnalysisTime_ += analysisTime;
    analysisCount_++;
    
    return analysis;
}

TapeSquashLimiter::TrackAnalysis TapeSquashLimiter::analyzeTrack(
    uint8_t trackId, uint8_t startStep, uint8_t endStep) {
    
    TrackAnalysis analysis;
    analysis.trackId = trackId;
    
    // Count active steps
    analysis.activeSteps = countActiveSteps(trackId, startStep, endStep);
    
    // Get effect count
    analysis.effectCount = getEffectCountCallback_ ? getEffectCountCallback_(trackId) : 0;
    
    // Estimate modulation count (simplified)
    analysis.modulationCount = analysis.activeSteps / 4;  // Assume 25% of steps have modulation
    
    // Calculate complexity score
    analysis.complexityScore = calculateTrackComplexity(trackId, startStep, endStep);
    
    // Calculate priority
    analysis.priority = calculateTrackPriority(trackId);
    
    // Estimate resource usage
    analysis.estimatedMemoryKB = calculateTrackMemoryUsage(trackId, analysis.activeSteps);
    analysis.estimatedCpuLoad = calculateTrackCpuLoad(trackId, analysis.activeSteps, analysis.effectCount);
    
    // Determine if recommended
    analysis.isRecommended = analysis.complexityScore <= 0.8f && analysis.activeSteps > 0;
    
    return analysis;
}

std::vector<uint8_t> TapeSquashLimiter::selectOptimalTracks(
    const std::vector<uint8_t>& candidateTracks, uint8_t maxTracks) {
    
    if (candidateTracks.size() <= maxTracks) {
        return candidateTracks;
    }
    
    // Analyze all candidate tracks
    std::vector<TrackAnalysis> analyses;
    for (uint8_t trackId : candidateTracks) {
        analyses.push_back(analyzeTrack(trackId, 0, 16));  // Use full pattern for selection
    }
    
    // Rank by combination of priority and inverse complexity
    std::sort(analyses.begin(), analyses.end(),
              [](const TrackAnalysis& a, const TrackAnalysis& b) {
                  float scoreA = a.priority - (a.complexityScore * 50.0f);
                  float scoreB = b.priority - (b.complexityScore * 50.0f);
                  return scoreA > scoreB;
              });
    
    // Select top tracks
    std::vector<uint8_t> selectedTracks;
    for (size_t i = 0; i < std::min(static_cast<size_t>(maxTracks), analyses.size()); ++i) {
        selectedTracks.push_back(analyses[i].trackId);
    }
    
    return selectedTracks;
}

// Limit Enforcement
bool TapeSquashLimiter::checkTrackCountLimit(uint8_t trackCount) const {
    return trackCount <= effectiveTrackLimit_;
}

bool TapeSquashLimiter::checkMemoryLimit(uint32_t estimatedMemoryKB) const {
    return estimatedMemoryKB <= config_.maxMemoryUsageKB;
}

bool TapeSquashLimiter::checkCpuLimit(float estimatedCpuLoad) const {
    return estimatedCpuLoad <= (config_.maxCpuLoadPercentage / 100.0f);
}

bool TapeSquashLimiter::isOperationAllowed(const SquashAnalysis& analysis) const {
    switch (config_.mode) {
        case LimitMode::STRICT_LIMIT:
            return checkTrackCountLimit(static_cast<uint8_t>(analysis.trackAnalyses.size())) &&
                   checkMemoryLimit(analysis.totalEstimatedMemoryKB) &&
                   checkCpuLimit(analysis.totalEstimatedCpuLoad);
            
        case LimitMode::WARNING_LIMIT:
            return true;  // Always allow but warn
            
        case LimitMode::DYNAMIC_LIMIT:
            return checkMemoryLimit(analysis.totalEstimatedMemoryKB) &&
                   checkCpuLimit(analysis.totalEstimatedCpuLoad);
            
        case LimitMode::PERFORMANCE_BASED:
            return analysis.totalEstimatedCpuLoad <= 0.9f;  // 90% CPU limit
            
        default:
            return true;
    }
}

std::string TapeSquashLimiter::getRecommendations(const SquashAnalysis& analysis) const {
    return analysis.recommendations;
}

// Dynamic Limit Adjustment
void TapeSquashLimiter::updateDynamicLimits() {
    if (config_.enableDynamicAdjustment) {
        adjustLimitsForSystemLoad();
        adjustLimitsForMemoryPressure();
        // Don't call updateEffectiveTrackLimit() here to avoid recursion
    }
}

void TapeSquashLimiter::adjustLimitsForSystemLoad() {
    float currentCpuLoad = getCurrentSystemCpuLoad();
    
    if (currentCpuLoad > 0.8f) {
        // High system load - reduce track limit
        effectiveTrackLimit_ = std::max(MIN_TRACK_LIMIT, 
                                       static_cast<uint8_t>(config_.maxTracks * 0.6f));
    } else if (currentCpuLoad < 0.3f) {
        // Low system load - allow more tracks
        effectiveTrackLimit_ = config_.maxTracks;
    }
}

void TapeSquashLimiter::adjustLimitsForMemoryPressure() {
    uint32_t currentMemoryUsage = getCurrentSystemMemoryUsage();
    uint32_t availableMemory = config_.maxMemoryUsageKB;
    
    if (currentMemoryUsage > availableMemory * 0.8f) {
        // High memory pressure - reduce limits
        effectiveTrackLimit_ = std::max(MIN_TRACK_LIMIT, 
                                       static_cast<uint8_t>(effectiveTrackLimit_ * 0.75f));
    }
}

uint8_t TapeSquashLimiter::getEffectiveTrackLimit() const {
    return effectiveTrackLimit_;
}

// Track Selection Optimization
std::vector<uint8_t> TapeSquashLimiter::rankTracksByComplexity(
    const std::vector<uint8_t>& trackIds, uint8_t startStep, uint8_t endStep) {
    
    std::vector<std::pair<uint8_t, float>> trackComplexities;
    
    for (uint8_t trackId : trackIds) {
        float complexity = calculateTrackComplexity(trackId, startStep, endStep);
        trackComplexities.emplace_back(trackId, complexity);
    }
    
    // Sort by complexity (lower first)
    std::sort(trackComplexities.begin(), trackComplexities.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    std::vector<uint8_t> rankedTracks;
    for (const auto& pair : trackComplexities) {
        rankedTracks.push_back(pair.first);
    }
    
    return rankedTracks;
}

std::vector<uint8_t> TapeSquashLimiter::rankTracksByPriority(const std::vector<uint8_t>& trackIds) {
    std::vector<std::pair<uint8_t, uint8_t>> trackPriorities;
    
    for (uint8_t trackId : trackIds) {
        uint8_t priority = calculateTrackPriority(trackId);
        trackPriorities.emplace_back(trackId, priority);
    }
    
    // Sort by priority (higher first)
    std::sort(trackPriorities.begin(), trackPriorities.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    std::vector<uint8_t> rankedTracks;
    for (const auto& pair : trackPriorities) {
        rankedTracks.push_back(pair.first);
    }
    
    return rankedTracks;
}

std::vector<uint8_t> TapeSquashLimiter::selectTracksByComplexityBudget(
    const std::vector<uint8_t>& trackIds, float complexityBudget) {
    
    std::vector<uint8_t> rankedTracks = rankTracksByComplexity(trackIds, 0, 16);
    std::vector<uint8_t> selectedTracks;
    
    float currentComplexity = 0.0f;
    for (uint8_t trackId : rankedTracks) {
        float trackComplexity = calculateTrackComplexity(trackId, 0, 16);
        
        if (currentComplexity + trackComplexity <= complexityBudget) {
            selectedTracks.push_back(trackId);
            currentComplexity += trackComplexity;
        }
    }
    
    return selectedTracks;
}

// Resource Estimation
uint32_t TapeSquashLimiter::estimateMemoryUsage(
    const std::vector<uint8_t>& trackIds, uint8_t startStep, uint8_t endStep) const {
    
    uint32_t totalMemory = 0;
    
    for (uint8_t trackId : trackIds) {
        uint8_t activeSteps = countActiveSteps(trackId, startStep, endStep);
        totalMemory += calculateTrackMemoryUsage(trackId, activeSteps);
    }
    
    return totalMemory;
}

float TapeSquashLimiter::estimateCpuLoad(
    const std::vector<uint8_t>& trackIds, uint8_t startStep, uint8_t endStep) const {
    
    float totalCpuLoad = 0.0f;
    
    for (uint8_t trackId : trackIds) {
        uint8_t activeSteps = countActiveSteps(trackId, startStep, endStep);
        uint8_t effectCount = getEffectCountCallback_ ? getEffectCountCallback_(trackId) : 0;
        totalCpuLoad += calculateTrackCpuLoad(trackId, activeSteps, effectCount);
    }
    
    return totalCpuLoad;
}

uint32_t TapeSquashLimiter::estimateProcessingTime(
    const std::vector<uint8_t>& trackIds, uint8_t startStep, uint8_t endStep) const {
    
    uint32_t memoryUsage = estimateMemoryUsage(trackIds, startStep, endStep);
    float cpuLoad = estimateCpuLoad(trackIds, startStep, endStep);
    
    return calculateProcessingTime(memoryUsage, cpuLoad);
}

// Performance Monitoring
void TapeSquashLimiter::recordOperationStart(const SquashAnalysis& analysis) {
    metrics_.lastOperationTime = getCurrentTimeMs();
}

void TapeSquashLimiter::recordOperationComplete(bool success, uint32_t actualTimeMs, 
                                               uint32_t actualMemoryKB, float actualCpuLoad) {
    if (success) {
        metrics_.successfulOperations++;
        
        // Update running averages
        float count = static_cast<float>(metrics_.successfulOperations);
        metrics_.averageProcessingTime = (metrics_.averageProcessingTime * (count - 1.0f) + 
                                        actualTimeMs / 1000.0f) / count;
        metrics_.averageMemoryUsage = (metrics_.averageMemoryUsage * (count - 1.0f) + 
                                      actualMemoryKB) / count;
        metrics_.averageCpuLoad = (metrics_.averageCpuLoad * (count - 1.0f) + actualCpuLoad) / count;
    }
}

void TapeSquashLimiter::recordOperationRejected(const std::string& reason) {
    metrics_.rejectedOperations++;
    
    if (limitExceededCallback_) {
        // Would pass analysis, but simplified for now
        SquashAnalysis dummyAnalysis;
        limitExceededCallback_(dummyAnalysis, reason);
    }
}

void TapeSquashLimiter::resetPerformanceMetrics() {
    metrics_ = PerformanceMetrics();
}

// User Interface Integration
std::vector<std::string> TapeSquashLimiter::getWarningMessages(const SquashAnalysis& analysis) const {
    std::vector<std::string> warnings;
    
    if (analysis.trackAnalyses.size() > config_.warningThreshold) {
        warnings.push_back("High track count may impact performance");
    }
    
    if (analysis.totalEstimatedMemoryKB > config_.maxMemoryUsageKB * 0.8f) {
        warnings.push_back("High memory usage detected");
    }
    
    if (analysis.totalEstimatedCpuLoad > config_.maxCpuLoadPercentage / 100.0f * 0.8f) {
        warnings.push_back("High CPU load estimated");
    }
    
    return warnings;
}

std::vector<std::string> TapeSquashLimiter::getOptimizationSuggestions(const SquashAnalysis& analysis) const {
    std::vector<std::string> suggestions;
    
    if (analysis.requiresOptimization) {
        suggestions.push_back("Consider reducing track count to " + std::to_string(config_.recommendedTracks));
        suggestions.push_back("Focus on tracks with highest priority");
        suggestions.push_back("Remove tracks with minimal audio content");
    }
    
    return suggestions;
}

std::string TapeSquashLimiter::formatResourceUsage(uint32_t memoryKB, float cpuLoad) const {
    std::ostringstream oss;
    oss << "Memory: " << memoryKB << "KB, CPU: " << static_cast<int>(cpuLoad * 100) << "%";
    return oss.str();
}

std::string TapeSquashLimiter::formatRecommendedSelection(const std::vector<uint8_t>& trackIds) const {
    if (trackIds.empty()) {
        return "No tracks recommended";
    }
    
    std::ostringstream oss;
    oss << "Recommended tracks: ";
    for (size_t i = 0; i < trackIds.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << static_cast<int>(trackIds[i] + 1);  // Display as 1-based
    }
    
    return oss.str();
}

// System Integration
void TapeSquashLimiter::integrateWithSystemMonitor(std::function<float()> getCpuLoad, 
                                                   std::function<uint32_t()> getMemoryUsage) {
    systemCpuLoadCallback_ = getCpuLoad;
    systemMemoryCallback_ = getMemoryUsage;
}

void TapeSquashLimiter::integrateWithSequencer(std::function<bool(uint8_t)> hasActiveSteps,
                                              std::function<uint8_t(uint8_t)> getEffectCount) {
    hasActiveStepsCallback_ = hasActiveSteps;
    getEffectCountCallback_ = getEffectCount;
}

// Performance Analysis
size_t TapeSquashLimiter::getEstimatedMemoryUsage() const {
    return sizeof(TapeSquashLimiter) + 
           recentAnalysisTimes_.capacity() * sizeof(uint32_t);
}

float TapeSquashLimiter::getAverageAnalysisTime() const {
    if (recentAnalysisTimes_.empty()) return 0.0f;
    
    uint32_t total = 0;
    for (uint32_t time : recentAnalysisTimes_) {
        total += time;
    }
    
    return static_cast<float>(total) / recentAnalysisTimes_.size();
}

uint8_t TapeSquashLimiter::getCurrentTrackLimit() const {
    return effectiveTrackLimit_;
}

bool TapeSquashLimiter::isDynamicLimitingActive() const {
    return config_.enableDynamicAdjustment && effectiveTrackLimit_ != config_.maxTracks;
}

// Internal methods
float TapeSquashLimiter::calculateTrackComplexity(uint8_t trackId, uint8_t startStep, uint8_t endStep) const {
    float complexity = 0.0f;
    
    uint8_t activeSteps = countActiveSteps(trackId, startStep, endStep);
    uint8_t effectCount = getEffectCountCallback_ ? getEffectCountCallback_(trackId) : 0;
    
    // Base complexity from active steps
    complexity += static_cast<float>(activeSteps) / 16.0f * 0.4f;
    
    // Effect complexity
    complexity += static_cast<float>(effectCount) / 8.0f * 0.6f;
    
    return std::min(1.0f, complexity);
}

uint8_t TapeSquashLimiter::calculateTrackPriority(uint8_t trackId) const {
    // Simple priority based on track position (lower track numbers = higher priority)
    return 255 - trackId * 16;
}

bool TapeSquashLimiter::isTrackActive(uint8_t trackId, uint8_t startStep, uint8_t endStep) const {
    return hasActiveStepsCallback_ ? hasActiveStepsCallback_(trackId) : true;
}

uint8_t TapeSquashLimiter::countActiveSteps(uint8_t trackId, uint8_t startStep, uint8_t endStep) const {
    if (!isTrackActive(trackId, startStep, endStep)) {
        return 0;
    }
    
    // Simplified - assume 50% of steps are active
    return (endStep - startStep + 1) / 2;
}

// Resource calculation helpers
uint32_t TapeSquashLimiter::calculateTrackMemoryUsage(uint8_t trackId, uint8_t stepCount) const {
    uint32_t baseMemory = MEMORY_PER_TRACK_KB;
    uint32_t stepMemory = stepCount * 4;  // 4KB per active step
    return baseMemory + stepMemory;
}

float TapeSquashLimiter::calculateTrackCpuLoad(uint8_t trackId, uint8_t stepCount, uint8_t effectCount) const {
    float baseCpuLoad = CPU_LOAD_PER_TRACK;
    float stepLoad = stepCount * 0.01f;
    float effectLoad = effectCount * 0.05f;
    return baseCpuLoad + stepLoad + effectLoad;
}

uint32_t TapeSquashLimiter::calculateProcessingTime(uint32_t memoryKB, float cpuLoad) const {
    // Simple estimation: more memory and CPU load = longer processing time
    uint32_t baseTime = 1000;  // 1 second base
    uint32_t memoryTime = memoryKB;  // 1ms per KB
    uint32_t cpuTime = static_cast<uint32_t>(cpuLoad * 5000.0f);  // Up to 5s for full CPU load
    
    return baseTime + memoryTime + cpuTime;
}

// Dynamic adjustment helpers
float TapeSquashLimiter::getCurrentSystemCpuLoad() const {
    return systemCpuLoadCallback_ ? systemCpuLoadCallback_() : 0.5f;  // Default 50%
}

uint32_t TapeSquashLimiter::getCurrentSystemMemoryUsage() const {
    return systemMemoryCallback_ ? systemMemoryCallback_() : config_.maxMemoryUsageKB / 2;
}

void TapeSquashLimiter::updateEffectiveTrackLimit() {
    effectiveTrackLimit_ = config_.maxTracks;
    
    if (config_.enableDynamicAdjustment) {
        updateDynamicLimits();
    }
}

// Analysis helpers
std::vector<TapeSquashLimiter::TrackAnalysis> TapeSquashLimiter::analyzeAllTracks(
    const std::vector<uint8_t>& trackIds, uint8_t startStep, uint8_t endStep) {
    
    std::vector<TrackAnalysis> analyses;
    
    for (uint8_t trackId : trackIds) {
        analyses.push_back(analyzeTrack(trackId, startStep, endStep));
    }
    
    return analyses;
}

// Validation helpers
void TapeSquashLimiter::validateTrackSelection(std::vector<uint8_t>& trackIds) const {
    // Remove duplicates and invalid track IDs
    std::sort(trackIds.begin(), trackIds.end());
    trackIds.erase(std::unique(trackIds.begin(), trackIds.end()), trackIds.end());
    
    // Remove tracks beyond reasonable limit
    trackIds.erase(std::remove_if(trackIds.begin(), trackIds.end(),
                   [](uint8_t id) { return id >= 16; }), trackIds.end());
}

void TapeSquashLimiter::sanitizeStepRange(uint8_t& startStep, uint8_t& endStep) const {
    if (startStep >= endStep) {
        endStep = startStep + 1;
    }
    if (endStep > 64) {
        endStep = 64;
    }
}

void TapeSquashLimiter::validateLimitConfig(LimitConfig& config) const {
    config.maxTracks = std::max(MIN_TRACK_LIMIT, std::min(config.maxTracks, MAX_TRACK_LIMIT));
    config.recommendedTracks = std::min(config.recommendedTracks, config.maxTracks);
    config.warningThreshold = std::min(config.warningThreshold, config.maxTracks);
    config.maxMemoryUsageKB = std::max(MIN_MEMORY_LIMIT_KB, std::min(config.maxMemoryUsageKB, MAX_MEMORY_LIMIT_KB));
    config.maxCpuLoadPercentage = std::max(MIN_CPU_LIMIT, std::min(config.maxCpuLoadPercentage, MAX_CPU_LIMIT));
}

// Utility methods
uint32_t TapeSquashLimiter::getCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

std::string TapeSquashLimiter::generateRecommendationText(const SquashAnalysis& analysis) const {
    std::ostringstream oss;
    
    if (analysis.trackAnalyses.size() > effectiveTrackLimit_) {
        oss << "Consider reducing from " << analysis.trackAnalyses.size() 
            << " to " << effectiveTrackLimit_ << " tracks for optimal performance. ";
    }
    
    if (analysis.totalEstimatedCpuLoad > config_.maxCpuLoadPercentage / 100.0f) {
        oss << "High CPU usage expected. ";
    }
    
    if (!analysis.recommendedTracks.empty()) {
        oss << formatRecommendedSelection(analysis.recommendedTracks);
    }
    
    return oss.str();
}

std::string TapeSquashLimiter::generateWarningMessage(const SquashAnalysis& analysis) const {
    if (!analysis.withinLimits) {
        return "Operation exceeds performance limits";
    } else if (analysis.requiresOptimization) {
        return "Performance optimization recommended";
    }
    return "";
}

// Notification helpers
void TapeSquashLimiter::notifyLimitExceeded(const SquashAnalysis& analysis, const std::string& reason) {
    if (limitExceededCallback_) {
        limitExceededCallback_(analysis, reason);
    }
}

void TapeSquashLimiter::notifyOptimizationSuggested(const SquashAnalysis& analysis) {
    if (optimizationSuggestedCallback_) {
        std::vector<std::string> suggestions = getOptimizationSuggestions(analysis);
        optimizationSuggestedCallback_(analysis, suggestions);
    }
}

void TapeSquashLimiter::notifyPerformanceWarning(const std::string& message) {
    if (performanceWarningCallback_) {
        performanceWarningCallback_(message);
    }
}