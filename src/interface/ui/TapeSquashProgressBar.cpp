#include "TapeSquashProgressBar.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <sstream>

TapeSquashProgressBar::TapeSquashProgressBar() :
    Container(),
    config_(ProgressConfig()),
    isActive_(false),
    cancellationEnabled_(true),
    showCancelConfirmation_(true),
    cancelConfirmationMessage_("Cancel tape squashing operation?"),
    animationEnabled_(true),
    animationDuration_(DEFAULT_ANIMATION_DURATION_MS),
    targetProgress_(0.0f),
    displayedProgress_(0.0f),
    animationStartTime_(0),
    startTime_(0),
    lastUpdateTime_(0),
    updateCount_(0),
    totalUpdateTime_(0) {
    
    initializeComponents();
    phaseTimings_.reserve(MAX_PHASE_HISTORY);
}

TapeSquashProgressBar::~TapeSquashProgressBar() {
    // Cleanup handled by TouchGFX framework
}

// Configuration
void TapeSquashProgressBar::setProgressConfig(const ProgressConfig& config) {
    config_ = config;
    updateVisualState();
}

// Progress Control
void TapeSquashProgressBar::startProgress(uint32_t totalSteps, const std::string& operation) {
    isActive_ = true;
    startTime_ = getCurrentTimeMs();
    lastUpdateTime_ = startTime_;
    updateCount_ = 0;
    totalUpdateTime_ = 0;
    phaseTimings_.clear();
    
    currentUpdate_ = ProgressUpdate();
    currentUpdate_.phase = ProgressPhase::INITIALIZING;
    currentUpdate_.totalSteps = totalSteps;
    currentUpdate_.statusMessage = "Starting " + operation + "...";
    currentUpdate_.canCancel = true;
    
    targetProgress_ = 0.0f;
    displayedProgress_ = 0.0f;
    
    updateVisualState();
    
    if (progressCallback_) {
        progressCallback_(currentUpdate_);
    }
}

void TapeSquashProgressBar::updateProgress(const ProgressUpdate& update) {
    if (!isActive_) return;
    
    uint32_t currentTime = getCurrentTimeMs();
    uint32_t updateStartTime = currentTime;
    
    // Validate and sanitize the update
    ProgressUpdate sanitizedUpdate = update;
    validateProgressUpdate(sanitizedUpdate);
    
    // Update timing information
    sanitizedUpdate.elapsedTimeMs = currentTime - startTime_;
    
    // Calculate time estimate if we have enough data
    if (sanitizedUpdate.completionPercentage > 0.01f) {  // At least 1% complete
        float remainingPercentage = 1.0f - sanitizedUpdate.completionPercentage;
        float timePerPercent = sanitizedUpdate.elapsedTimeMs / sanitizedUpdate.completionPercentage;
        sanitizedUpdate.estimatedRemainingMs = static_cast<uint32_t>(remainingPercentage * timePerPercent);
    }
    
    // Store phase timing if phase changed
    if (sanitizedUpdate.phase != currentUpdate_.phase) {
        if (phaseTimings_.size() >= MAX_PHASE_HISTORY) {
            phaseTimings_.erase(phaseTimings_.begin());
        }
        phaseTimings_.push_back(sanitizedUpdate.elapsedTimeMs);
    }
    
    currentUpdate_ = sanitizedUpdate;
    lastUpdateTime_ = currentTime;
    
    // Start animation to new progress value
    if (animationEnabled_ && sanitizedUpdate.completionPercentage != targetProgress_) {
        startProgressAnimation(sanitizedUpdate.completionPercentage);
    } else {
        displayedProgress_ = sanitizedUpdate.completionPercentage;
    }
    
    updateVisualState();
    
    // Update performance counters
    uint32_t updateTime = getCurrentTimeMs() - updateStartTime;
    updateCount_++;
    totalUpdateTime_ += updateTime;
    
    if (progressCallback_) {
        progressCallback_(currentUpdate_);
    }
}

void TapeSquashProgressBar::setProgress(float percentage, const std::string& message) {
    ProgressUpdate update = currentUpdate_;
    update.completionPercentage = percentage;
    if (!message.empty()) {
        update.statusMessage = message;
    }
    updateProgress(update);
}

void TapeSquashProgressBar::setPhase(ProgressPhase phase, const std::string& message) {
    ProgressUpdate update = currentUpdate_;
    update.phase = phase;
    if (!message.empty()) {
        update.statusMessage = message;
    }
    updateProgress(update);
}

void TapeSquashProgressBar::completeProgress(const std::string& message) {
    ProgressUpdate update = currentUpdate_;
    update.phase = ProgressPhase::COMPLETED;
    update.completionPercentage = 1.0f;
    update.statusMessage = message;
    update.canCancel = false;
    
    updateProgress(update);
    isActive_ = false;
    
    if (completedCallback_) {
        completedCallback_(true, message);
    }
}

void TapeSquashProgressBar::cancelProgress(const std::string& message) {
    ProgressUpdate update = currentUpdate_;
    update.phase = ProgressPhase::CANCELLED;
    update.statusMessage = message;
    update.canCancel = false;
    
    updateProgress(update);
    isActive_ = false;
    
    if (completedCallback_) {
        completedCallback_(false, message);
    }
}

void TapeSquashProgressBar::errorProgress(const std::string& message) {
    ProgressUpdate update = currentUpdate_;
    update.phase = ProgressPhase::ERROR;
    update.statusMessage = message;
    update.canCancel = false;
    
    updateProgress(update);
    isActive_ = false;
    
    if (completedCallback_) {
        completedCallback_(false, message);
    }
}

// Visual Appearance
void TapeSquashProgressBar::setProgressColor(touchgfx::colortype color) {
    config_.progressColor = color;
    updateVisualState();
}

void TapeSquashProgressBar::setBackgroundColor(touchgfx::colortype color) {
    config_.backgroundColor = color;
    updateVisualState();
}

void TapeSquashProgressBar::setTextColor(touchgfx::colortype color) {
    config_.textColor = color;
    updateVisualState();
}

void TapeSquashProgressBar::setErrorColor(touchgfx::colortype color) {
    config_.errorColor = color;
    updateVisualState();
}

// Time Estimation
void TapeSquashProgressBar::updateTimeEstimate() {
    if (!isActive_ || currentUpdate_.completionPercentage <= 0.0f) {
        return;
    }
    
    uint32_t currentTime = getCurrentTimeMs();
    currentUpdate_.elapsedTimeMs = currentTime - startTime_;
    
    // Use exponential moving average for smoother estimation
    if (currentUpdate_.completionPercentage > 0.01f) {
        float totalEstimatedTime = currentUpdate_.elapsedTimeMs / currentUpdate_.completionPercentage;
        currentUpdate_.estimatedRemainingMs = static_cast<uint32_t>(
            totalEstimatedTime - currentUpdate_.elapsedTimeMs);
    }
}

std::string TapeSquashProgressBar::formatTimeRemaining(uint32_t milliseconds) const {
    uint32_t seconds = milliseconds / 1000;
    uint32_t minutes = seconds / 60;
    seconds = seconds % 60;
    
    if (minutes > 0) {
        return std::to_string(minutes) + "m " + std::to_string(seconds) + "s";
    } else {
        return std::to_string(seconds) + "s";
    }
}

std::string TapeSquashProgressBar::formatElapsedTime(uint32_t milliseconds) const {
    return formatTimeRemaining(milliseconds);
}

// TouchGFX Integration
void TapeSquashProgressBar::setupScreen() {
    initializeComponents();
    layoutComponents();
    updateVisualState();
    setVisible(true);
}

void TapeSquashProgressBar::tearDownScreen() {
    setVisible(false);
}

void TapeSquashProgressBar::handleTickEvent() {
    Container::handleTickEvent();
    
    if (animationEnabled_ && targetProgress_ != displayedProgress_) {
        updateProgressAnimation();
        updateProgressBar();
    }
    
    // Update time display if active
    if (isActive_) {
        updateTimeEstimate();
        updateTimeDisplay();
    }
}

void TapeSquashProgressBar::handleClickEvent(const touchgfx::ClickEvent& evt) {
    Container::handleClickEvent(evt);
    
    // Check if cancel button was clicked
    if (config_.showCancelButton && cancellationEnabled_ && currentUpdate_.canCancel) {
        if (cancelButton_.getAbsoluteRect().intersect(evt.getX(), evt.getY())) {
            handleCancelButton();
        }
    }
}

// Performance Analysis
uint32_t TapeSquashProgressBar::getEstimatedMemoryUsage() const {
    return sizeof(TapeSquashProgressBar) + 
           phaseTimings_.capacity() * sizeof(uint32_t) +
           currentUpdate_.statusMessage.capacity();
}

float TapeSquashProgressBar::getUpdateRate() const {
    return (updateCount_ > 0) ? static_cast<float>(totalUpdateTime_) / updateCount_ : 0.0f;
}

void TapeSquashProgressBar::resetPerformanceCounters() {
    updateCount_ = 0;
    totalUpdateTime_ = 0;
}

// Internal methods
void TapeSquashProgressBar::initializeComponents() {
    // Initialize background box
    backgroundBox_.setPosition(0, 0, config_.barWidth, config_.barHeight);
    backgroundBox_.setColor(config_.backgroundColor);
    add(backgroundBox_);
    
    // Initialize progress box
    progressBox_.setPosition(0, 0, 0, config_.barHeight);
    progressBox_.setColor(config_.progressColor);
    add(progressBox_);
    
    // Initialize status text
    statusText_.setPosition(0, config_.barHeight + 5, config_.barWidth, 20);
    statusText_.setColor(config_.textColor);
    statusText_.setTypedText(touchgfx::TypedText(0));  // Would be configured with actual text ID
    add(statusText_);
    
    // Initialize percentage text
    if (config_.showPercentage) {
        percentageText_.setPosition(config_.barWidth - 60, 0, 60, config_.barHeight);
        percentageText_.setColor(config_.textColor);
        percentageText_.setTypedText(touchgfx::TypedText(1));  // Would be configured with actual text ID
        add(percentageText_);
    }
    
    // Initialize time text
    if (config_.showTimeEstimate) {
        timeText_.setPosition(0, config_.barHeight + 25, config_.barWidth, 20);
        timeText_.setColor(config_.textColor);
        timeText_.setTypedText(touchgfx::TypedText(2));  // Would be configured with actual text ID
        add(timeText_);
    }
    
    // Initialize cancel button
    if (config_.showCancelButton) {
        cancelButton_.setPosition(config_.barWidth + 10, 0, 60, config_.barHeight);
        cancelButton_.setLabelText(touchgfx::TypedText(3));  // "Cancel"
        cancelButton_.setAction(touchgfx::AbstractButtonContainer::Action([this]() { handleCancelButton(); }));
        add(cancelButton_);
    }
}

void TapeSquashProgressBar::updateVisualState() {
    updateProgressBar();
    updateStatusText();
    updateTimeDisplay();
    
    // Update colors based on phase
    touchgfx::colortype currentProgressColor = config_.progressColor;
    if (currentUpdate_.phase == ProgressPhase::ERROR) {
        currentProgressColor = config_.errorColor;
    }
    progressBox_.setColor(currentProgressColor);
    
    // Update cancel button visibility
    if (config_.showCancelButton) {
        cancelButton_.setVisible(cancellationEnabled_ && currentUpdate_.canCancel);
    }
    
    invalidate();
}

void TapeSquashProgressBar::updateProgressBar() {
    uint16_t progressWidth = static_cast<uint16_t>(displayedProgress_ * config_.barWidth);
    progressBox_.setWidth(progressWidth);
    progressBox_.invalidate();
}

void TapeSquashProgressBar::updateStatusText() {
    std::string truncatedMessage = truncateStatusMessage(
        currentUpdate_.statusMessage, MAX_STATUS_MESSAGE_LENGTH);
    
    // In a real implementation, would set text content through TouchGFX text system
    // statusText_.setWildcard(truncatedMessage.c_str());
    statusText_.invalidate();
}

void TapeSquashProgressBar::updateTimeDisplay() {
    if (!config_.showTimeEstimate) return;
    
    std::string timeInfo = "";
    if (isActive_) {
        timeInfo = "Elapsed: " + formatElapsedTime(currentUpdate_.elapsedTimeMs);
        if (currentUpdate_.estimatedRemainingMs > 0) {
            timeInfo += " | Remaining: " + formatTimeRemaining(currentUpdate_.estimatedRemainingMs);
        }
    }
    
    // In a real implementation, would set text content through TouchGFX text system
    // timeText_.setWildcard(timeInfo.c_str());
    timeText_.invalidate();
}

void TapeSquashProgressBar::handleCancelButton() {
    if (!cancellationEnabled_ || !currentUpdate_.canCancel) return;
    
    if (showCancelConfirmation_) {
        showCancelDialog();
    } else {
        confirmCancel();
    }
}

void TapeSquashProgressBar::showCancelDialog() {
    // In a real implementation, would show TouchGFX modal dialog
    // For now, directly call confirm cancel after a brief delay to simulate dialog
    confirmCancel();
}

void TapeSquashProgressBar::confirmCancel() {
    if (cancelCallback_) {
        cancelCallback_();
    }
    cancelProgress("Operation cancelled by user");
}

// Animation helpers
void TapeSquashProgressBar::startProgressAnimation(float targetProgress) {
    targetProgress_ = targetProgress;
    animationStartTime_ = getCurrentTimeMs();
}

void TapeSquashProgressBar::updateProgressAnimation() {
    uint32_t currentTime = getCurrentTimeMs();
    float animationTime = calculateAnimationProgress(currentTime, animationStartTime_, animationDuration_);
    
    if (animationTime >= 1.0f) {
        displayedProgress_ = targetProgress_;
    } else {
        float easedTime = easeInOutQuad(animationTime);
        float startProgress = displayedProgress_;
        displayedProgress_ = startProgress + (targetProgress_ - startProgress) * easedTime;
    }
}

float TapeSquashProgressBar::easeInOutQuad(float t) const {
    return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
}

// Layout helpers
void TapeSquashProgressBar::layoutComponents() {
    calculateOptimalLayout();
    adjustForScreenSize();
}

void TapeSquashProgressBar::calculateOptimalLayout() {
    // Ensure components fit within screen bounds
    uint16_t totalWidth = config_.barWidth;
    if (config_.showCancelButton) {
        totalWidth += 70;  // Cancel button width + margin
    }
    
    uint16_t totalHeight = config_.barHeight;
    if (config_.showTimeEstimate) {
        totalHeight += 45;  // Status text + time text + margins
    } else {
        totalHeight += 25;  // Just status text + margin
    }
    
    setWidth(totalWidth);
    setHeight(totalHeight);
}

void TapeSquashProgressBar::adjustForScreenSize() {
    // Could implement screen size adjustments here
    // For STM32 H7 with fixed display, likely not needed
}

// Text formatting helpers
std::string TapeSquashProgressBar::getPhaseDisplayName(ProgressPhase phase) const {
    switch (phase) {
        case ProgressPhase::INITIALIZING:   return "Initializing";
        case ProgressPhase::ANALYZING:      return "Analyzing";
        case ProgressPhase::RENDERING:      return "Rendering";
        case ProgressPhase::FINALIZING:     return "Finalizing";
        case ProgressPhase::COMPLETED:      return "Completed";
        case ProgressPhase::CANCELLED:      return "Cancelled";
        case ProgressPhase::ERROR:          return "Error";
        default:                           return "Processing";
    }
}

std::string TapeSquashProgressBar::formatPercentage(float percentage) const {
    uint8_t percent = static_cast<uint8_t>(percentage * 100.0f);
    return std::to_string(percent) + "%";
}

std::string TapeSquashProgressBar::truncateStatusMessage(const std::string& message, uint16_t maxLength) const {
    if (message.length() <= maxLength) {
        return message;
    }
    return message.substr(0, maxLength - 3) + "...";
}

// Validation helpers
void TapeSquashProgressBar::validateProgressUpdate(ProgressUpdate& update) const {
    sanitizeProgressValue(update.completionPercentage);
    
    if (update.currentStep > update.totalSteps) {
        update.currentStep = update.totalSteps;
    }
    
    if (update.statusMessage.empty()) {
        update.statusMessage = getPhaseDisplayName(update.phase);
    }
}

void TapeSquashProgressBar::sanitizeProgressValue(float& progress) const {
    progress = std::max(0.0f, std::min(1.0f, progress));
}

// Utility methods
uint32_t TapeSquashProgressBar::getCurrentTimeMs() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

float TapeSquashProgressBar::calculateAnimationProgress(uint32_t currentTime, uint32_t startTime, uint16_t duration) const {
    if (duration == 0) return 1.0f;
    
    uint32_t elapsed = currentTime - startTime;
    return static_cast<float>(elapsed) / static_cast<float>(duration);
}