#include "src/sampler/VelocityPitchRangeManager.h"
#include "src/sampler/SampleLayeringSystem.h"
#include "src/interface/ui/TapeSquashProgressBar.h"
#include "src/sequencer/TapeSquashLimiter.h"
#include <iostream>
#include <cassert>
#include <vector>

void testVelocityPitchRangeIntegration() {
    std::cout << "Testing VelocityPitchRangeManager integration..." << std::endl;
    
    VelocityPitchRangeManager rangeManager;
    
    // Test range configuration
    VelocityPitchRangeManager::RangeConfig config;
    config.mode = VelocityPitchRangeManager::RangeMode::VELOCITY_PITCH;
    config.maxSimultaneousSlots = 4;
    rangeManager.setRangeConfig(config);
    
    // Test adding sample ranges
    VelocityPitchRangeManager::SampleRange range1;
    range1.sampleSlot = 0;
    range1.velocityMin = 0.0f;
    range1.velocityMax = 0.5f;
    range1.pitchMin = 36;
    range1.pitchMax = 60;
    range1.roundRobinGroup = 0;
    range1.priority = 150;
    
    assert(rangeManager.addSampleRange(range1));
    assert(rangeManager.getRangeCount() == 1);
    
    VelocityPitchRangeManager::SampleRange range2;
    range2.sampleSlot = 1;
    range2.velocityMin = 0.5f;
    range2.velocityMax = 1.0f;
    range2.pitchMin = 60;
    range2.pitchMax = 96;
    range2.roundRobinGroup = 1;
    range2.priority = 140;
    
    assert(rangeManager.addSampleRange(range2));
    assert(rangeManager.getRangeCount() == 2);
    
    // Test sample selection
    auto result = rangeManager.selectSamples(0.3f, 48, 0);  // Low velocity, mid pitch
    assert(!result.selectedSlots.empty());
    assert(result.selectedSlots[0] == 0);  // Should select first range
    
    result = rangeManager.selectSamples(0.8f, 72, 0);  // High velocity, high pitch
    assert(!result.selectedSlots.empty());
    assert(result.selectedSlots[0] == 1);  // Should select second range
    
    // Test auto-assignment
    std::vector<uint8_t> slots = {0, 1, 2, 3};
    rangeManager.autoAssignVelocityRanges(slots, 4);
    assert(rangeManager.getRangeCount() >= 4);
    
    std::cout << "✓ VelocityPitchRangeManager integration test passed" << std::endl;
}

void testSampleLayeringIntegration() {
    std::cout << "Testing SampleLayeringSystem integration..." << std::endl;
    
    SampleLayeringSystem layerSystem;
    
    // Test configuration
    SampleLayeringSystem::LayeringConfig config;
    config.maxLayers = 8;
    config.maxGroups = 4;
    config.enableAutoGainCompensation = true;
    layerSystem.setLayeringConfig(config);
    
    // Test adding layers
    SampleLayeringSystem::SampleLayer layer1;
    layer1.sampleSlot = 0;
    layer1.activationMode = SampleLayeringSystem::LayerActivationMode::VELOCITY_GATED;
    layer1.blendMode = SampleLayeringSystem::LayerBlendMode::ADDITIVE;
    layer1.sequencingMode = SampleLayeringSystem::LayerSequencingMode::INDEPENDENT_STEPS;
    layer1.velocityThreshold = 0.0f;
    layer1.velocityMax = 0.5f;
    layer1.layerGain = 1.0f;
    layer1.layerPan = -0.5f;
    
    assert(layerSystem.addLayer(layer1));
    assert(layerSystem.getLayerCount() == 1);
    
    SampleLayeringSystem::SampleLayer layer2;
    layer2.sampleSlot = 1;
    layer2.activationMode = SampleLayeringSystem::LayerActivationMode::VELOCITY_GATED;
    layer2.blendMode = SampleLayeringSystem::LayerBlendMode::ADDITIVE;
    layer2.sequencingMode = SampleLayeringSystem::LayerSequencingMode::EUCLIDEAN;
    layer2.velocityThreshold = 0.5f;
    layer2.velocityMax = 1.0f;
    layer2.euclideanSteps = 16;
    layer2.euclideanHits = 8;
    layer2.euclideanRotation = 0;
    layer2.layerGain = 0.8f;
    layer2.layerPan = 0.5f;
    
    assert(layerSystem.addLayer(layer2));
    assert(layerSystem.getLayerCount() == 2);
    
    // Test layer activation
    auto result = layerSystem.activateLayers(0.3f, 60, 0);
    assert(!result.activatedLayers.empty());
    assert(result.activatedLayers.size() >= 1);
    
    result = layerSystem.activateLayers(0.8f, 60, 0);
    assert(!result.activatedLayers.empty());
    
    // Test Euclidean pattern generation
    auto euclideanPattern = layerSystem.generateEuclideanPattern(16, 8, 0);
    assert(euclideanPattern.size() == 16);
    
    uint8_t hitCount = 0;
    for (bool hit : euclideanPattern) {
        if (hit) hitCount++;
    }
    assert(hitCount == 8);
    
    // Test layer groups
    SampleLayeringSystem::LayerGroup group;
    group.groupGain = 0.9f;
    group.groupPan = 0.0f;
    group.groupBlendMode = SampleLayeringSystem::LayerBlendMode::EQUAL_POWER;
    
    assert(layerSystem.createGroup(group));
    
    std::cout << "✓ SampleLayeringSystem integration test passed" << std::endl;
}

void testTapeSquashProgressBarIntegration() {
    std::cout << "Testing TapeSquashProgressBar integration..." << std::endl;
    
    TapeSquashProgressBar progressBar;
    
    // Test configuration
    TapeSquashProgressBar::ProgressConfig config;
    config.barWidth = 300;
    config.barHeight = 20;
    config.showPercentage = true;
    config.showTimeEstimate = true;
    config.enableAnimation = true;
    progressBar.setProgressConfig(config);
    
    assert(!progressBar.isActive());
    assert(!progressBar.isCompleted());
    
    // Test progress flow
    progressBar.startProgress(100, "Test Operation");
    assert(progressBar.isActive());
    assert(progressBar.getCurrentProgress() == 0.0f);
    assert(progressBar.getCurrentPhase() == TapeSquashProgressBar::ProgressPhase::INITIALIZING);
    
    // Test progress updates
    TapeSquashProgressBar::ProgressUpdate update;
    update.phase = TapeSquashProgressBar::ProgressPhase::ANALYZING;
    update.completionPercentage = 0.25f;
    update.currentStep = 25;
    update.totalSteps = 100;
    update.statusMessage = "Analyzing tracks...";
    update.canCancel = true;
    
    progressBar.updateProgress(update);
    assert(progressBar.getCurrentProgress() == 0.25f);
    assert(progressBar.getCurrentPhase() == TapeSquashProgressBar::ProgressPhase::ANALYZING);
    
    // Test phase progression
    progressBar.setPhase(TapeSquashProgressBar::ProgressPhase::RENDERING, "Rendering audio...");
    assert(progressBar.getCurrentPhase() == TapeSquashProgressBar::ProgressPhase::RENDERING);
    
    progressBar.setProgress(0.75f, "75% complete");
    assert(progressBar.getCurrentProgress() == 0.75f);
    
    // Test completion
    progressBar.completeProgress("Operation completed successfully");
    assert(progressBar.isCompleted());
    assert(!progressBar.isActive());
    assert(progressBar.getCurrentProgress() == 1.0f);
    
    // Test time formatting
    std::string timeStr = progressBar.formatTimeRemaining(125000);  // 2 minutes 5 seconds
    assert(timeStr.find("2m") != std::string::npos);
    assert(timeStr.find("5s") != std::string::npos);
    
    std::cout << "✓ TapeSquashProgressBar integration test passed" << std::endl;
}

void testTapeSquashLimiterIntegration() {
    std::cout << "Testing TapeSquashLimiter integration..." << std::endl;
    
    TapeSquashLimiter limiter;
    
    // Test configuration
    TapeSquashLimiter::LimitConfig config;
    config.maxTracks = 6;
    config.recommendedTracks = 4;
    config.warningThreshold = 5;
    config.mode = TapeSquashLimiter::LimitMode::WARNING_LIMIT;
    config.maxMemoryUsageKB = 2048;
    config.maxCpuLoadPercentage = 75.0f;
    
    limiter.setLimitConfig(config);
    assert(limiter.getLimitConfig().maxTracks == 6);
    assert(limiter.getEffectiveTrackLimit() == 6);
    
    // Test limit checking
    assert(limiter.checkTrackCountLimit(4));
    assert(limiter.checkTrackCountLimit(6));
    assert(!limiter.checkTrackCountLimit(8));
    
    assert(limiter.checkMemoryLimit(1024));
    assert(limiter.checkMemoryLimit(2048));
    assert(!limiter.checkMemoryLimit(4096));
    
    assert(limiter.checkCpuLimit(0.5f));
    assert(limiter.checkCpuLimit(0.75f));
    assert(!limiter.checkCpuLimit(0.9f));
    
    // Test track analysis
    std::vector<uint8_t> tracks = {0, 1, 2, 3, 4, 5, 6, 7};
    auto analysis = limiter.analyzeSquashOperation(tracks, 0, 16);
    
    assert(!analysis.trackAnalyses.empty());
    assert(analysis.trackAnalyses.size() <= 8);
    assert(analysis.totalEstimatedMemoryKB > 0);
    assert(analysis.totalEstimatedCpuLoad > 0.0f);
    
    // Test track selection optimization
    auto optimalTracks = limiter.selectOptimalTracks(tracks, 4);
    assert(optimalTracks.size() == 4);
    
    auto rankedTracks = limiter.rankTracksByComplexity(tracks, 0, 16);
    assert(rankedTracks.size() == tracks.size());
    
    // Test resource estimation
    uint32_t memoryEstimate = limiter.estimateMemoryUsage(tracks, 0, 16);
    assert(memoryEstimate > 0);
    
    float cpuEstimate = limiter.estimateCpuLoad(tracks, 0, 16);
    assert(cpuEstimate > 0.0f);
    
    uint32_t timeEstimate = limiter.estimateProcessingTime(tracks, 0, 16);
    assert(timeEstimate > 0);
    
    // Test performance monitoring
    limiter.recordOperationStart(analysis);
    limiter.recordOperationComplete(true, 5000, 1024, 0.6f);
    
    auto metrics = limiter.getPerformanceMetrics();
    assert(metrics.successfulOperations == 1);
    assert(metrics.averageProcessingTime == 5.0f);
    assert(metrics.averageMemoryUsage == 1024.0f);
    
    std::cout << "✓ TapeSquashLimiter integration test passed" << std::endl;
}

void testCrossSystemIntegration() {
    std::cout << "Testing cross-system integration..." << std::endl;
    
    // Create all systems
    VelocityPitchRangeManager rangeManager;
    SampleLayeringSystem layerSystem;
    TapeSquashProgressBar progressBar;
    TapeSquashLimiter limiter;
    
    // Test range manager + layer system integration
    VelocityPitchRangeManager::SampleRange range;
    range.sampleSlot = 0;
    range.velocityMin = 0.0f;
    range.velocityMax = 1.0f;
    range.pitchMin = 36;
    range.pitchMax = 96;
    
    rangeManager.addSampleRange(range);
    
    SampleLayeringSystem::SampleLayer layer;
    layer.sampleSlot = 0;  // Same slot as range
    layer.activationMode = SampleLayeringSystem::LayerActivationMode::VELOCITY_GATED;
    layer.velocityThreshold = 0.3f;
    layer.velocityMax = 0.8f;
    
    layerSystem.addLayer(layer);
    
    // Test velocity-based selection and layering
    auto rangeResult = rangeManager.selectSamples(0.6f, 60, 0);
    auto layerResult = layerSystem.activateLayers(0.6f, 60, 0);
    
    assert(!rangeResult.selectedSlots.empty());
    assert(!layerResult.activatedLayers.empty());
    
    // Test progress bar + limiter integration
    std::vector<uint8_t> tracks = {0, 1, 2, 3, 4, 5};
    auto analysis = limiter.analyzeSquashOperation(tracks, 0, 16);
    
    progressBar.startProgress(analysis.estimatedProcessingTimeMs / 100, "Tape Squashing");
    
    // Simulate progress phases
    TapeSquashProgressBar::ProgressUpdate update;
    update.phase = TapeSquashProgressBar::ProgressPhase::ANALYZING;
    update.completionPercentage = 0.2f;
    update.statusMessage = "Analyzing " + std::to_string(tracks.size()) + " tracks...";
    progressBar.updateProgress(update);
    
    update.phase = TapeSquashProgressBar::ProgressPhase::RENDERING;
    update.completionPercentage = 0.6f;
    update.statusMessage = "Rendering optimized selection...";
    progressBar.updateProgress(update);
    
    update.phase = TapeSquashProgressBar::ProgressPhase::FINALIZING;
    update.completionPercentage = 0.9f;
    update.statusMessage = "Creating sample...";
    progressBar.updateProgress(update);
    
    progressBar.completeProgress("Tape squashing completed");
    
    assert(progressBar.isCompleted());
    
    // Test memory usage estimates across systems
    size_t totalMemory = rangeManager.getEstimatedMemoryUsage() +
                        layerSystem.getEstimatedMemoryUsage() +
                        progressBar.getEstimatedMemoryUsage() +
                        limiter.getEstimatedMemoryUsage();
    
    assert(totalMemory > 0);
    std::cout << "Total estimated memory usage: " << totalMemory << " bytes" << std::endl;
    
    std::cout << "✓ Cross-system integration test passed" << std::endl;
}

int main() {
    std::cout << "=== Fourth Batch Integration Test ===" << std::endl;
    std::cout << "Testing 4 advanced sampling and squashing systems..." << std::endl;
    
    try {
        testVelocityPitchRangeIntegration();
        testSampleLayeringIntegration();
        testTapeSquashProgressBarIntegration();
        testTapeSquashLimiterIntegration();
        testCrossSystemIntegration();
        
        std::cout << "\n🎉 All fourth batch integration tests passed!" << std::endl;
        std::cout << "\nCompleted systems:" << std::endl;
        std::cout << "✓ VelocityPitchRangeManager - Advanced velocity/pitch range assignment" << std::endl;
        std::cout << "✓ SampleLayeringSystem - Multi-layer sample playback with independent sequencing" << std::endl;
        std::cout << "✓ TapeSquashProgressBar - Interactive progress indication for tape squashing" << std::endl;
        std::cout << "✓ TapeSquashLimiter - Track count limits and performance optimization" << std::endl;
        
        std::cout << "\nKey capabilities demonstrated:" << std::endl;
        std::cout << "• Multi-dimensional sample mapping (velocity × pitch × round-robin)" << std::endl;
        std::cout << "• Advanced sample layering with Euclidean rhythm generation" << std::endl;
        std::cout << "• Comprehensive tape squashing workflow with progress tracking" << std::endl;
        std::cout << "• Intelligent performance optimization and resource management" << std::endl;
        std::cout << "• Cross-system integration for complex musical arrangements" << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}