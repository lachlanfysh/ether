#pragma once
#include <cstdint>
#include <string>
#include <functional>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/TextArea.hpp>
#include <touchgfx/widgets/Button.hpp>
#include <touchgfx/containers/Container.hpp>
#include <touchgfx/Color.hpp>

/**
 * TapeSquashProgressBar - Interactive loading bar for tape squashing operations
 * 
 * Provides comprehensive progress indication for tape squashing workflow:
 * - Real-time progress tracking with visual bar and percentage display
 * - Multi-phase progress indication (analysis, rendering, finalizing)
 * - Interactive cancellation with safety confirmation
 * - Integration with TouchGFX for embedded GUI display
 * - Memory-efficient progress visualization for STM32 H7 platform
 * 
 * Features:
 * - Animated progress bar with smooth transitions
 * - Phase-specific status messages and time estimation
 * - Cancel button with confirmation dialog for safety
 * - Error handling with informative user feedback
 * - Automatic cleanup after completion or cancellation
 * - Integration with pattern data replacer for seamless workflow
 * - Hardware-optimized graphics rendering for 600MHz CPU performance
 */
class TapeSquashProgressBar : public touchgfx::Container {
public:
    // Progress phases during tape squashing
    enum class ProgressPhase {
        INITIALIZING,       // Setting up audio rendering pipeline
        ANALYZING,          // Analyzing selected pattern region
        RENDERING,          // Real-time audio capture and processing
        FINALIZING,         // Sample creation and data replacement
        COMPLETED,          // Successfully finished
        CANCELLED,          // User cancelled operation
        ERROR              // Error occurred during processing
    };
    
    // Progress update information
    struct ProgressUpdate {
        ProgressPhase phase;            // Current processing phase
        float completionPercentage;     // Overall completion (0.0-1.0)
        uint32_t currentStep;           // Current step being processed
        uint32_t totalSteps;            // Total steps to process
        uint32_t elapsedTimeMs;         // Time elapsed since start
        uint32_t estimatedRemainingMs;  // Estimated time remaining
        std::string statusMessage;      // Human-readable status
        bool canCancel;                 // Whether cancellation is allowed
        
        ProgressUpdate() :
            phase(ProgressPhase::INITIALIZING),
            completionPercentage(0.0f),
            currentStep(0),
            totalSteps(0),
            elapsedTimeMs(0),
            estimatedRemainingMs(0),
            canCancel(true) {}
    };
    
    // Configuration for progress bar appearance
    struct ProgressConfig {
        uint16_t barWidth;              // Progress bar width in pixels
        uint16_t barHeight;             // Progress bar height in pixels
        touchgfx::colortype backgroundColor;    // Background color
        touchgfx::colortype progressColor;      // Progress fill color
        touchgfx::colortype textColor;          // Text color
        touchgfx::colortype errorColor;         // Error state color
        bool showPercentage;            // Display percentage text
        bool showTimeEstimate;          // Display time estimate
        bool showCancelButton;          // Show cancel button
        bool enableAnimation;           // Animate progress updates
        
        ProgressConfig() :
            barWidth(300),
            barHeight(20),
            backgroundColor(touchgfx::Color::getColorFromRGB(40, 40, 40)),
            progressColor(touchgfx::Color::getColorFromRGB(0, 150, 255)),
            textColor(touchgfx::Color::getColorFromRGB(255, 255, 255)),
            errorColor(touchgfx::Color::getColorFromRGB(255, 80, 80)),
            showPercentage(true),
            showTimeEstimate(true),
            showCancelButton(true),
            enableAnimation(true) {}
    };
    
    TapeSquashProgressBar();
    virtual ~TapeSquashProgressBar();
    
    // Configuration
    void setProgressConfig(const ProgressConfig& config);
    const ProgressConfig& getProgressConfig() const { return config_; }
    
    // Progress Control
    void startProgress(uint32_t totalSteps, const std::string& operation = "Tape Squashing");
    void updateProgress(const ProgressUpdate& update);
    void setProgress(float percentage, const std::string& message = "");
    void setPhase(ProgressPhase phase, const std::string& message = "");
    void completeProgress(const std::string& message = "Complete");
    void cancelProgress(const std::string& message = "Cancelled");
    void errorProgress(const std::string& message = "Error occurred");
    
    // State Management
    bool isActive() const { return isActive_; }
    bool isCompleted() const { return currentUpdate_.phase == ProgressPhase::COMPLETED; }
    bool isCancelled() const { return currentUpdate_.phase == ProgressPhase::CANCELLED; }
    bool hasError() const { return currentUpdate_.phase == ProgressPhase::ERROR; }
    ProgressPhase getCurrentPhase() const { return currentUpdate_.phase; }
    float getCurrentProgress() const { return currentUpdate_.completionPercentage; }
    
    // User Interaction
    void enableCancellation(bool enabled) { cancellationEnabled_ = enabled; }
    void showCancelConfirmation(bool show) { showCancelConfirmation_ = show; }
    void setCancelConfirmationMessage(const std::string& message) { cancelConfirmationMessage_ = message; }
    
    // Visual Appearance
    void setProgressColor(touchgfx::colortype color);
    void setBackgroundColor(touchgfx::colortype color);
    void setTextColor(touchgfx::colortype color);
    void setErrorColor(touchgfx::colortype color);
    
    // Animation Control
    void setAnimationEnabled(bool enabled) { animationEnabled_ = enabled; }
    void setAnimationSpeed(uint16_t durationMs) { animationDuration_ = durationMs; }
    
    // Callbacks
    using ProgressCallback = std::function<void(const ProgressUpdate&)>;
    using CancelCallback = std::function<void()>;
    using CompletedCallback = std::function<void(bool success, const std::string& message)>;
    
    void setProgressCallback(ProgressCallback callback) { progressCallback_ = callback; }
    void setCancelCallback(CancelCallback callback) { cancelCallback_ = callback; }
    void setCompletedCallback(CompletedCallback callback) { completedCallback_ = callback; }
    
    // Time Estimation
    void updateTimeEstimate();
    std::string formatTimeRemaining(uint32_t milliseconds) const;
    std::string formatElapsedTime(uint32_t milliseconds) const;
    
    // TouchGFX Integration
    virtual void setupScreen() override;
    virtual void tearDownScreen() override;
    virtual void handleTickEvent() override;
    virtual void handleClickEvent(const touchgfx::ClickEvent& evt) override;
    
    // Performance Analysis
    uint32_t getEstimatedMemoryUsage() const;
    float getUpdateRate() const;
    void resetPerformanceCounters();

protected:
    // TouchGFX Components
    touchgfx::Box backgroundBox_;
    touchgfx::Box progressBox_;
    touchgfx::TextArea statusText_;
    touchgfx::TextArea percentageText_;
    touchgfx::TextArea timeText_;
    touchgfx::Button cancelButton_;
    
    // Visual state
    ProgressConfig config_;
    ProgressUpdate currentUpdate_;
    bool isActive_;
    bool cancellationEnabled_;
    bool showCancelConfirmation_;
    std::string cancelConfirmationMessage_;
    
    // Animation state
    bool animationEnabled_;
    uint16_t animationDuration_;
    float targetProgress_;
    float displayedProgress_;
    uint32_t animationStartTime_;
    
    // Time tracking
    uint32_t startTime_;
    uint32_t lastUpdateTime_;
    std::vector<uint32_t> phaseTimings_;
    
    // Performance tracking
    uint32_t updateCount_;
    uint32_t totalUpdateTime_;
    
    // Callbacks
    ProgressCallback progressCallback_;
    CancelCallback cancelCallback_;
    CompletedCallback completedCallback_;
    
    // Internal methods
    void initializeComponents();
    void updateVisualState();
    void updateProgressBar();
    void updateStatusText();
    void updateTimeDisplay();
    void handleCancelButton();
    void showCancelDialog();
    void confirmCancel();
    
    // Animation helpers
    void startProgressAnimation(float targetProgress);
    void updateProgressAnimation();
    float easeInOutQuad(float t) const;
    
    // Layout helpers
    void layoutComponents();
    void calculateOptimalLayout();
    void adjustForScreenSize();
    
    // Text formatting helpers
    std::string getPhaseDisplayName(ProgressPhase phase) const;
    std::string formatPercentage(float percentage) const;
    std::string truncateStatusMessage(const std::string& message, uint16_t maxLength) const;
    
    // Validation helpers
    void validateProgressUpdate(ProgressUpdate& update) const;
    void sanitizeProgressValue(float& progress) const;
    
    // Utility methods
    uint32_t getCurrentTimeMs() const;
    float calculateAnimationProgress(uint32_t currentTime, uint32_t startTime, uint16_t duration) const;
    
    // Constants
    static constexpr uint16_t DEFAULT_ANIMATION_DURATION_MS = 300;
    static constexpr uint16_t CANCEL_CONFIRMATION_TIMEOUT_MS = 5000;
    static constexpr uint16_t MAX_STATUS_MESSAGE_LENGTH = 50;
    static constexpr uint16_t UPDATE_RATE_SAMPLES = 100;
    static constexpr float PROGRESS_SMOOTHING_FACTOR = 0.1f;
    static constexpr uint8_t MAX_PHASE_HISTORY = 16;
};