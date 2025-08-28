#include "PreviewSystem.h"
#include <algorithm>
#include <cmath>

namespace Preview {

//-----------------------------------------------------------------------------
// PreviewArbiter Implementation
//-----------------------------------------------------------------------------

PreviewArbiter::PreviewArbiter() 
    : sampleRate_(48000.0f), cardGrade_(CardGrade::OK), minTimeBetweenTriggers_(0.020f), // 20ms
      minDistance_(12.0f), lastTriggerTime_(0.0f), sensitivity_(0.5f), lastTriggeredId_(0),
      voronoiRadius_(25.0f), triggersThisSecond_(0) {
    
    motionHistory_.reserve(MAX_HISTORY);
    recentlyTriggered_.reserve(16);
}

PreviewArbiter::~PreviewArbiter() {}

void PreviewArbiter::init(float sampleRate) {
    sampleRate_ = sampleRate;
    setCardGrade(cardGrade_); // Refresh parameters
    lastSecondReset_ = std::chrono::steady_clock::now();
}

void PreviewArbiter::setCardGrade(CardGrade grade) {
    cardGrade_ = grade;
    
    // Adjust parameters based on card performance
    switch (grade) {
        case CardGrade::Gold:
            minTimeBetweenTriggers_ = 0.018f; // 18ms
            minDistance_ = 8.0f;
            break;
        case CardGrade::OK:
            minTimeBetweenTriggers_ = 0.021f; // 21ms  
            minDistance_ = 12.0f;
            break;
        case CardGrade::Slow:
            minTimeBetweenTriggers_ = 0.024f; // 24ms
            minDistance_ = 20.0f;
            break;
    }
}

void PreviewArbiter::tick(float x, float y, float timestamp) {
    if (!scatterBak_ || !previewPlayer_) return;
    
    // Update motion history
    updateMotionHistory(x, y, timestamp);
    
    // Reset triggers per second counter
    auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration<float>(now - lastSecondReset_).count() >= 1.0f) {
        triggersThisSecond_.store(0);
        lastSecondReset_ = now;
    }
    
    // Find closest sample to cursor position
    uint64_t closestId = findClosestSample(x, y);
    if (closestId == 0) return;
    
    // Calculate motion velocity
    float velocity = calculateMotionVelocity();
    
    // Apply gating rules
    if (!passesGating(closestId, velocity)) return;
    if (!passesVoronoiTest(closestId, x, y)) return;
    if (!passesSimilarityTest(closestId, velocity)) return;
    
    // Trigger preview
    previewPlayer_->playStub(closestId);
    lastTriggeredId_ = closestId;
    lastTriggerTime_ = timestamp;
    triggersThisSecond_.fetch_add(1);
    
    // Schedule body stream if conditions are met
    if (velocity <= 2.0f) { // Slow/dwelling motion
        schedulePrefetch(closestId);
        
        // Schedule bridge to body after dwell threshold
        static constexpr float DWELL_THRESHOLD = 0.060f; // 60ms
        // In a real implementation, this would be handled by a timer
        // For now, we'll prefetch immediately and let the cache handle it
        previewCache_->prefetch(closestId);
    }
    
    // Update recently triggered list
    recentlyTriggered_.push_back(closestId);
    if (recentlyTriggered_.size() > 16) {
        recentlyTriggered_.erase(recentlyTriggered_.begin());
    }
}

void PreviewArbiter::setMotionThresholds(float slowPx, float fastPx) {
    minDistance_ = slowPx;
    // Could use fastPx for different behavior at high speeds
}

uint64_t PreviewArbiter::findClosestSample(float x, float y) {
    if (!scatterBak_ || !scatterBak_->isLoaded()) return 0;
    
    // Convert screen coordinates to sample coordinate system
    // Assuming screen is normalized 0-1, sample coords are -32768 to 32767
    int16_t sampleX = static_cast<int16_t>((x - 0.5f) * 65536.0f);
    int16_t sampleY = static_cast<int16_t>((y - 0.5f) * 65536.0f);
    
    uint64_t closestId = 0;
    float closestDistance = std::numeric_limits<float>::max();
    
    // Simple brute force search - in reality would use spatial data structure
    size_t sampleCount = scatterBak_->getSampleCount();
    
    for (size_t i = 0; i < sampleCount; ++i) {
        // This is a simplified approach - would need access to sample IDs
        // In a real implementation, we'd iterate through the spatial bins
        
        // For now, create a mock search pattern
        uint64_t testId = i + 1; // Simple ID scheme
        int16_t testX, testY;
        
        if (scatterBak_->coords(testId, testX, testY)) {
            float dx = testX - sampleX;
            float dy = testY - sampleY;
            float distance = std::sqrt(dx * dx + dy * dy);
            
            if (distance < closestDistance) {
                closestDistance = distance;
                closestId = testId;
            }
        }
    }
    
    return closestId;
}

float PreviewArbiter::calculateMotionVelocity() {
    if (motionHistory_.size() < 2) return 0.0f;
    
    // Calculate velocity over last few samples
    float totalVelocity = 0.0f;
    int validSamples = 0;
    
    for (size_t i = 1; i < motionHistory_.size(); ++i) {
        const auto& prev = motionHistory_[i - 1];
        const auto& curr = motionHistory_[i];
        
        float dt = curr.timestamp - prev.timestamp;
        if (dt > 0.0f && dt < 0.1f) { // Valid time delta
            float dx = curr.x - prev.x;
            float dy = curr.y - prev.y;
            float distance = std::sqrt(dx * dx + dy * dy);
            float velocity = distance / dt;
            
            totalVelocity += velocity;
            validSamples++;
        }
    }
    
    return validSamples > 0 ? totalVelocity / validSamples : 0.0f;
}

bool PreviewArbiter::passesGating(uint64_t id, float velocity) {
    // Global trigger rate cap (~60 stubs/s max)
    if (triggersThisSecond_.load() >= 60) return false;
    
    // Minimum time between triggers
    float currentTime = std::chrono::duration<float>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    if (currentTime - lastTriggerTime_ < minTimeBetweenTriggers_) return false;
    
    // Same sample retrigger throttling
    if (id == lastTriggeredId_ && currentTime - lastTriggerTime_ < 0.100f) { // 100ms
        return false;
    }
    
    return true;
}

bool PreviewArbiter::passesVoronoiTest(uint64_t id, float x, float y) {
    // Only the closest point within radius can fire
    // This prevents multiple nearby samples from triggering simultaneously
    
    if (!scatterBak_) return true;
    
    int16_t targetX, targetY;
    if (!scatterBak_->coords(id, targetX, targetY)) return false;
    
    // Convert to screen coordinates  
    float targetScreenX = (targetX / 65536.0f) + 0.5f;
    float targetScreenY = (targetY / 65536.0f) + 0.5f;
    
    // Check distance to cursor
    float dx = x - targetScreenX;
    float dy = y - targetScreenY;
    float distance = std::sqrt(dx * dx + dy * dy) * 1000.0f; // Convert to pixels (assume 1000px canvas)
    
    return distance <= voronoiRadius_;
}

bool PreviewArbiter::passesSimilarityTest(uint64_t id, float velocity) {
    // Skip near-duplicates when moving fast
    if (velocity < 100.0f) return true; // Slow motion - allow all
    
    // Check if this sample is similar to recently triggered ones
    if (!scatterBak_) return true;
    
    uint16_t neighbors[16];
    uint8_t neighborCount = scatterBak_->knn(lastTriggeredId_, neighbors);
    
    // If current sample is in the k-NN of the last triggered sample, skip it
    for (uint8_t i = 0; i < neighborCount; ++i) {
        if (neighbors[i] == id) {
            return false; // Skip similar sample
        }
    }
    
    return true;
}

void PreviewArbiter::updateMotionHistory(float x, float y, float timestamp) {
    MotionHistory entry;
    entry.x = x;
    entry.y = y;
    entry.timestamp = timestamp;
    entry.velocity = calculateMotionVelocity(); // Calculate before adding current
    
    motionHistory_.push_back(entry);
    
    if (motionHistory_.size() > MAX_HISTORY) {
        motionHistory_.erase(motionHistory_.begin());
    }
}

void PreviewArbiter::schedulePrefetch(uint64_t id) {
    if (!scatterBak_ || !previewCache_) return;
    
    // Prefetch this sample's body
    previewCache_->prefetch(id);
    
    // Prefetch forward neighbors based on card grade
    int prefetchCount = 0;
    switch (cardGrade_) {
        case CardGrade::Gold: prefetchCount = 4; break;
        case CardGrade::OK: prefetchCount = 2; break;
        case CardGrade::Slow: prefetchCount = 0; break;
    }
    
    if (prefetchCount > 0) {
        uint16_t neighbors[16];
        uint8_t neighborCount = scatterBak_->knn(id, neighbors);
        
        int prefetched = 0;
        for (uint8_t i = 0; i < neighborCount && prefetched < prefetchCount; ++i) {
            previewCache_->prefetch(neighbors[i]);
            prefetched++;
        }
    }
}

size_t PreviewArbiter::getTriggersPerSecond() const {
    return triggersThisSecond_.load();
}

float PreviewArbiter::getPrefetchHitRate() const {
    // Would need to track prefetch success rate
    return 0.75f; // Placeholder
}

} // namespace Preview