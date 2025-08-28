#include "SmartKnob.h"
#include <cmath>
#include <algorithm>
#include <cstring>

// For STM32 H7 hardware abstraction
#ifdef STM32H7
#include "stm32h7xx_hal.h"
#include "stm32h7xx_ll_tim.h"
#include "stm32h7xx_ll_spi.h"
#include "stm32h7xx_ll_dma.h"
#else
// Simulation mode for development
#include <chrono>
static uint32_t simulationTime = 0;
#endif

SmartKnob* SmartKnob::instance_ = nullptr;

SmartKnob::SmartKnob() {
    instance_ = this;
    
    // Initialize data structures
    velocity_.velocityHistory.fill(0.0f);
    velocity_.timeHistory.fill(0);
    
    // Set default configurations
    detentConfig_.mode = DetentMode::MEDIUM;
    detentConfig_.customSpacing = 1365; // 12 detents per revolution
    detentConfig_.snapStrength = 0.7f;
    detentConfig_.deadZone = 0.1f;
    
    hapticConfig_.pattern = HapticPattern::TICK;
    hapticConfig_.strength = 0.5f;
    hapticConfig_.frequency = 50.0f;
    hapticConfig_.decay = 0.1f;
    hapticConfig_.velocityScaling = true;
    
    gestureConfig_.detentDwellEnabled = true;
    gestureConfig_.dwellTimeMs = 500;
    gestureConfig_.doubleFlickEnabled = true;
    gestureConfig_.flickThresholdMs = 150;
    gestureConfig_.fineModeEnabled = true;
    gestureConfig_.fineModeThreshold = 0.1f;
    gestureConfig_.coarseModeThreshold = 2.0f;
}

SmartKnob::~SmartKnob() {
    shutdown();
    instance_ = nullptr;
}

bool SmartKnob::initialize() {
    if (initialized_) {
        return true;
    }
    
    // Initialize encoder hardware
    if (!initializeEncoder()) {
        return false;
    }
    
    // Initialize haptic feedback hardware
    if (!initializeHaptic()) {
        shutdownEncoder();
        return false;
    }
    
    // Reset state
    encoder_.rawPosition = readEncoderRaw();
    encoder_.lastRawPosition = encoder_.rawPosition;
    encoder_.position = 0;
    encoder_.lastPosition = 0;
    encoder_.timestamp = getTimeMs();
    encoder_.lastTimestamp = encoder_.timestamp;
    
    position_ = 0;
    detentPosition_ = 0;
    velocity_ = 0.0f;
    inDetent_ = true;
    
    // Reset gesture state
    gesture_.current = GestureType::NONE;
    gesture_.active = false;
    
    // Reset haptic state
    haptic_.currentForce = 0.0f;
    haptic_.targetForce = 0.0f;
    haptic_.activePattern = HapticPattern::NONE;
    
    initialized_ = true;
    calibrated_ = false;
    
    // Perform initial calibration
    calibrate();
    
    return true;
}

void SmartKnob::shutdown() {
    if (!initialized_) {
        return;
    }
    
    // Stop haptic feedback
    setHapticForce(0.0f);
    
    shutdownHaptic();
    shutdownEncoder();
    
    initialized_ = false;
    calibrated_ = false;
}

void SmartKnob::setDetentConfig(const DetentConfig& config) {
    detentConfig_ = config;
    
    // Recalculate detent position
    updateDetentState();
}

void SmartKnob::setHapticConfig(const HapticConfig& config) {
    hapticConfig_ = config;
}

void SmartKnob::setGestureConfig(const GestureConfig& config) {
    gestureConfig_ = config;
}

void SmartKnob::setPosition(int32_t position) {
    position_ = position;
    encoder_.position = position;
    updateDetentState();
}

void SmartKnob::processEncoderUpdate(uint16_t rawPosition) {
    encoder_.rawPosition = rawPosition;
    encoder_.timestamp = getTimeMs();
    encoder_.dataReady = true;
}

void SmartKnob::processHapticUpdate() {
    updateHapticFeedback();
    
    float force = calculateHapticForce();
    writeHapticForce(force);
}

void SmartKnob::update() {
    if (!initialized_) {
        return;
    }
    
    // Process encoder data if available
    if (encoder_.dataReady) {
        updatePosition();
        updateVelocity();
        updateDetentState();
        updateGestureDetection();
        
        encoder_.dataReady = false;
        
        // Call rotation callback if registered
        if (rotationCallback_) {
            int32_t delta = position_ - encoder_.lastPosition;
            if (delta != 0) {
                rotationCallback_(delta, velocity_, inDetent_);
                encoder_.lastPosition = position_;
            }
        }
    }
    
    // Update haptic feedback
    updateHapticFeedback();
}

void SmartKnob::triggerHaptic(HapticPattern pattern, float strength) {
    haptic_.activePattern = pattern;
    haptic_.targetForce = strength * hapticConfig_.strength;
    haptic_.patternStartTime = getTimeMs();
    haptic_.patternPhase = 0.0f;
    haptic_.decay = hapticConfig_.decay;
    
    if (hapticCallback_) {
        hapticCallback_(pattern, strength);
    }
}

void SmartKnob::setHapticForce(float force) {
    haptic_.currentForce = std::clamp(force, -1.0f, 1.0f);
    writeHapticForce(haptic_.currentForce);
}

void SmartKnob::calibrate() {
    if (!initialized_) {
        return;
    }
    
    // Perform encoder calibration
    encoderHealth_ = 1.0f;
    
    // Sample encoder over multiple positions
    const int numSamples = 64;
    float totalError = 0.0f;
    
    for (int i = 0; i < numSamples; i++) {
        uint16_t raw1 = readEncoderRaw();
        
        // Small delay
        for (volatile int j = 0; j < 1000; j++) {}
        
        uint16_t raw2 = readEncoderRaw();
        
        // Calculate noise/jitter
        float error = std::abs(static_cast<int16_t>(raw2 - raw1));
        totalError += error;
    }
    
    float averageError = totalError / numSamples;
    encoderHealth_ = std::max(0.0f, 1.0f - averageError / 100.0f);
    
    calibrated_ = (encoderHealth_ > 0.8f);
}

void SmartKnob::updatePosition() {
    uint16_t currentRaw = encoder_.rawPosition;
    uint16_t lastRaw = encoder_.lastRawPosition;
    
    // Handle 16-bit wraparound
    int32_t delta = static_cast<int16_t>(currentRaw - lastRaw);
    
    // Update absolute position
    encoder_.position += delta;
    position_ = encoder_.position;
    
    encoder_.lastRawPosition = currentRaw;
}

void SmartKnob::updateVelocity() {
    uint32_t currentTime = encoder_.timestamp;
    uint32_t lastTime = encoder_.lastTimestamp;
    
    if (currentTime == lastTime) {
        return; // No time elapsed
    }
    
    // Calculate instantaneous velocity
    int32_t deltaPos = encoder_.position - encoder_.lastPosition;
    uint32_t deltaTime = currentTime - lastTime;
    
    float instantVelocity = static_cast<float>(deltaPos) / static_cast<float>(deltaTime) * 1000.0f; // per second
    
    // Add to history for smoothing
    velocity_.velocityHistory[velocity_.historyIndex] = instantVelocity;
    velocity_.timeHistory[velocity_.historyIndex] = currentTime;
    velocity_.historyIndex = (velocity_.historyIndex + 1) % VelocityTracker::HISTORY_SIZE;
    
    // Calculate smoothed velocity using weighted average
    float totalWeight = 0.0f;
    float weightedSum = 0.0f;
    
    for (int i = 0; i < VelocityTracker::HISTORY_SIZE; i++) {
        uint32_t age = currentTime - velocity_.timeHistory[i];
        if (age < 100) { // Only use samples from last 100ms
            float weight = std::exp(-static_cast<float>(age) * 0.01f); // Exponential decay
            weightedSum += velocity_.velocityHistory[i] * weight;
            totalWeight += weight;
        }
    }
    
    if (totalWeight > 0.0f) {
        velocity_.smoothedVelocity = weightedSum / totalWeight;
    } else {
        velocity_.smoothedVelocity = 0.0f;
    }
    
    velocity_ = velocity_.smoothedVelocity;
    
    // Calculate acceleration
    static float lastVelocity = 0.0f;
    velocity_.acceleration = (velocity_ - lastVelocity) * sampleRate_;
    lastVelocity = velocity_;
    
    encoder_.lastTimestamp = currentTime;
}

void SmartKnob::updateDetentState() {
    if (detentConfig_.mode == DetentMode::NONE) {
        inDetent_ = false;
        detentPosition_ = position_;
        return;
    }
    
    int32_t nearestDetent = calculateDetentPosition(position_);
    int32_t detentDistance = std::abs(position_ - nearestDetent);
    
    // Determine detent spacing
    uint16_t spacing = detentConfig_.customSpacing;
    switch (detentConfig_.mode) {
        case DetentMode::LIGHT:
            spacing = 683; // 24 detents
            break;
        case DetentMode::MEDIUM:
            spacing = 1365; // 12 detents
            break;
        case DetentMode::HEAVY:
            spacing = 2731; // 6 detents
            break;
        default:
            break;
    }
    
    uint16_t deadZone = static_cast<uint16_t>(spacing * detentConfig_.deadZone);
    
    bool wasInDetent = inDetent_;
    inDetent_ = (detentDistance <= deadZone);
    
    if (inDetent_) {
        detentPosition_ = nearestDetent;
        
        // Trigger haptic feedback on detent entry
        if (!wasInDetent && hapticConfig_.pattern != HapticPattern::NONE) {
            float strength = hapticConfig_.strength;
            
            // Scale with velocity if enabled
            if (hapticConfig_.velocityScaling) {
                strength *= std::clamp(std::abs(velocity_) / 5.0f, 0.1f, 1.0f);
            }
            
            triggerHaptic(hapticConfig_.pattern, strength);
        }
    }
}

void SmartKnob::updateGestureDetection() {
    if (!gestureConfig_.detentDwellEnabled && !gestureConfig_.doubleFlickEnabled && 
        !gestureConfig_.fineModeEnabled) {
        return;
    }
    
    uint32_t currentTime = getTimeMs();
    
    // Update motion tracking
    if (std::abs(velocity_) > 0.1f) {
        gesture_.lastMotionTime = currentTime;
        gesture_.peakVelocity = std::max(gesture_.peakVelocity, std::abs(velocity_));
        
        if (velocity_ > 0.1f) {
            gesture_.motionDirection = 1;
        } else if (velocity_ < -0.1f) {
            gesture_.motionDirection = -1;
        }
    }
    
    GestureType previousGesture = currentGesture_;
    
    // Detect specific gestures
    detectDetentDwell();
    detectDoubleFlick();
    detectFineCoarseMode();
    
    // Trigger gesture callback if gesture changed
    if (currentGesture_ != previousGesture && gestureCallback_) {
        gestureCallback_(currentGesture_, static_cast<float>(position_));
    }
}

void SmartKnob::detectDetentDwell() {
    if (!gestureConfig_.detentDwellEnabled) {
        return;
    }
    
    uint32_t currentTime = getTimeMs();
    
    if (inDetent_ && std::abs(velocity_) < 0.05f) {
        if (!gesture_.detentDwellActive) {
            gesture_.detentDwellActive = true;
            gesture_.detentDwellStart = currentTime;
        } else if (currentTime - gesture_.detentDwellStart >= gestureConfig_.dwellTimeMs) {
            currentGesture_ = GestureType::DETENT_DWELL;
            gestureActive_ = true;
        }
    } else {
        gesture_.detentDwellActive = false;
        if (currentGesture_ == GestureType::DETENT_DWELL) {
            currentGesture_ = GestureType::NONE;
            gestureActive_ = false;
        }
    }
}

void SmartKnob::detectDoubleFlick() {
    if (!gestureConfig_.doubleFlickEnabled) {
        return;
    }
    
    uint32_t currentTime = getTimeMs();
    
    // Detect quick direction changes
    static int lastDirection = 0;
    static uint32_t lastDirectionTime = 0;
    
    if (gesture_.motionDirection != 0 && gesture_.motionDirection != lastDirection) {
        if (currentTime - lastDirectionTime < gestureConfig_.flickThresholdMs && 
            gesture_.peakVelocity > 3.0f) {
            
            currentGesture_ = GestureType::DOUBLE_FLICK;
            gestureActive_ = true;
            gesture_.lastFlickTime = currentTime;
        }
        
        lastDirection = gesture_.motionDirection;
        lastDirectionTime = currentTime;
    }
    
    // Reset double flick after timeout
    if (currentGesture_ == GestureType::DOUBLE_FLICK && 
        currentTime - gesture_.lastFlickTime > 200) {
        currentGesture_ = GestureType::NONE;
        gestureActive_ = false;
    }
    
    // Reset peak velocity periodically
    if (currentTime - gesture_.lastMotionTime > 50) {
        gesture_.peakVelocity *= 0.9f;
    }
}

void SmartKnob::detectFineCoarseMode() {
    if (!gestureConfig_.fineModeEnabled) {
        return;
    }
    
    float absVelocity = std::abs(velocity_);
    
    if (absVelocity < gestureConfig_.fineModeThreshold && absVelocity > 0.01f) {
        if (currentGesture_ != GestureType::FINE_MODE) {
            currentGesture_ = GestureType::FINE_MODE;
            gestureActive_ = true;
        }
    } else if (absVelocity > gestureConfig_.coarseModeThreshold) {
        if (currentGesture_ != GestureType::COARSE_MODE) {
            currentGesture_ = GestureType::COARSE_MODE;
            gestureActive_ = true;
        }
    } else if (absVelocity < 0.01f) {
        if (currentGesture_ == GestureType::FINE_MODE || currentGesture_ == GestureType::COARSE_MODE) {
            currentGesture_ = GestureType::NONE;
            gestureActive_ = false;
        }
    }
}

void SmartKnob::updateHapticFeedback() {
    uint32_t currentTime = getTimeMs();
    
    // Calculate base haptic force from detent system
    float detentForce = 0.0f;
    if (detentConfig_.mode != DetentMode::NONE && !inDetent_) {
        detentForce = calculateDetentForce(position_, detentPosition_);
    }
    
    // Calculate pattern-based haptic force
    float patternForce = 0.0f;
    if (haptic_.activePattern != HapticPattern::NONE) {
        uint32_t patternAge = currentTime - haptic_.patternStartTime;
        float patternTime = static_cast<float>(patternAge) * 0.001f; // Convert to seconds
        
        // Apply decay
        float decay = std::exp(-patternTime / haptic_.decay);
        
        switch (haptic_.activePattern) {
            case HapticPattern::TICK:
                patternForce = haptic_.targetForce * decay * std::exp(-patternTime * 20.0f);
                break;
                
            case HapticPattern::BUMP:
                patternForce = haptic_.targetForce * decay * (1.0f - patternTime * 10.0f);
                if (patternForce < 0.0f) patternForce = 0.0f;
                break;
                
            case HapticPattern::THUD:
                patternForce = haptic_.targetForce * decay * std::exp(-patternTime * 5.0f);
                break;
                
            case HapticPattern::RAMP_UP:
                patternForce = haptic_.targetForce * std::min(1.0f, patternTime * 2.0f);
                break;
                
            case HapticPattern::RAMP_DOWN:
                patternForce = haptic_.targetForce * std::max(0.0f, 1.0f - patternTime * 2.0f);
                break;
                
            case HapticPattern::SPRING: {
                float springForce = -static_cast<float>(position_ - detentPosition_) * 0.0001f;
                patternForce = springForce * haptic_.targetForce * decay;
                break;
            }
                
            case HapticPattern::FRICTION:
                patternForce = -velocity_ * 0.1f * haptic_.targetForce * decay;
                break;
                
            default:
                break;
        }
        
        // Clear pattern if decayed
        if (decay < 0.01f) {
            haptic_.activePattern = HapticPattern::NONE;
        }
    }
    
    // Combine forces
    float totalForce = detentForce + patternForce;
    totalForce = std::clamp(totalForce, -1.0f, 1.0f);
    
    // Apply exponential smoothing to prevent abrupt changes
    haptic_.currentForce = exponentialSmooth(haptic_.currentForce, totalForce, 0.3f);
    
    setHapticForce(haptic_.currentForce);
}

int32_t SmartKnob::calculateDetentPosition(int32_t rawPosition) const {
    if (detentConfig_.mode == DetentMode::NONE) {
        return rawPosition;
    }
    
    uint16_t spacing = detentConfig_.customSpacing;
    switch (detentConfig_.mode) {
        case DetentMode::LIGHT:
            spacing = 683; // 24 detents
            break;
        case DetentMode::MEDIUM:
            spacing = 1365; // 12 detents
            break;
        case DetentMode::HEAVY:
            spacing = 2731; // 6 detents
            break;
        default:
            break;
    }
    
    // Round to nearest detent
    int32_t detentIndex = (rawPosition + spacing / 2) / spacing;
    return detentIndex * spacing;
}

float SmartKnob::calculateDetentForce(int32_t position, int32_t detentPos) const {
    int32_t distance = position - detentPos;
    float normalizedDistance = static_cast<float>(distance) / detentConfig_.customSpacing;
    
    // Calculate spring force toward detent
    float force = -normalizedDistance * detentConfig_.snapStrength;
    
    // Apply asymmetric force if configured
    if (detentConfig_.asymmetric) {
        if (distance > 0) {
            force *= 1.2f; // Stronger resistance in positive direction
        } else {
            force *= 0.8f; // Weaker resistance in negative direction
        }
    }
    
    return std::clamp(force, -1.0f, 1.0f);
}

float SmartKnob::calculateHapticForce() const {
    return haptic_.currentForce;
}

uint32_t SmartKnob::getTimeMs() const {
#ifdef STM32H7
    return HAL_GetTick();
#else
    // Simulation mode
    auto now = std::chrono::steady_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
#endif
}

float SmartKnob::exponentialSmooth(float current, float target, float alpha) const {
    return current + alpha * (target - current);
}

// Hardware abstraction layer implementation
bool SmartKnob::initializeEncoder() {
#ifdef STM32H7
    // Initialize SPI for magnetic encoder (e.g., AS5047P)
    // Configure SPI3 with DMA for high-speed reading
    // Set up timer interrupt for regular sampling
    
    // This would contain actual STM32 H7 peripheral initialization
    // For now, return success for simulation
    return true;
#else
    // Simulation mode
    return true;
#endif
}

bool SmartKnob::initializeHaptic() {
#ifdef STM32H7
    // Initialize PWM timer for haptic motor control
    // Configure complementary PWM outputs for H-bridge
    // Set up current sensing for force feedback
    
    // This would contain actual STM32 H7 PWM/motor control setup
    return true;
#else
    // Simulation mode
    return true;
#endif
}

void SmartKnob::shutdownEncoder() {
#ifdef STM32H7
    // Disable SPI and DMA
    // Stop timer interrupts
#endif
}

void SmartKnob::shutdownHaptic() {
#ifdef STM32H7
    // Disable PWM outputs
    // Power down motor driver
#endif
}

uint16_t SmartKnob::readEncoderRaw() {
#ifdef STM32H7
    // Read from SPI register of magnetic encoder
    // Return 14-bit position scaled to 16-bit
    return 0; // Placeholder
#else
    // Simulation: generate smooth rotation
    static uint16_t simPosition = 0;
    simPosition += 10; // Simulate slow rotation
    return simPosition;
#endif
}

void SmartKnob::writeHapticForce(float force) {
#ifdef STM32H7
    // Convert force to PWM duty cycle
    // Set complementary PWM outputs for H-bridge
    // Handle current limiting and protection
#else
    // Simulation mode - just store the value
    (void)force;
#endif
}

// Interrupt handlers
void SmartKnob::encoderInterruptHandler() {
    if (instance_) {
        uint16_t rawPosition = instance_->readEncoderRaw();
        instance_->processEncoderUpdate(rawPosition);
    }
}

void SmartKnob::hapticTimerHandler() {
    if (instance_) {
        instance_->processHapticUpdate();
    }
}