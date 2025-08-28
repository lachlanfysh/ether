#pragma once

/**
 * ADSREnvelope - Attack, Decay, Sustain, Release envelope generator
 */
class ADSREnvelope {
public:
    enum class Stage {
        IDLE,
        ATTACK,
        DECAY,
        SUSTAIN,
        RELEASE
    };
    
    ADSREnvelope() = default;
    ~ADSREnvelope() = default;
    
    bool initialize(float sampleRate) {
        sampleRate_ = sampleRate;
        return true;
    }
    
    void shutdown() {}
    
    void setADSR(float attack, float decay, float sustain, float release) {
        attack_ = attack;
        decay_ = decay;
        sustain_ = sustain;
        release_ = release;
    }
    
    void setDepth(float depth) { depth_ = depth; }
    void setExponential(bool exponential) { exponential_ = exponential; }
    void setRelease(float release) { release_ = release; }
    
    void trigger() {
        stage_ = Stage::ATTACK;
        level_ = 0.0f;
        active_ = true;
    }
    
    void release() {
        if (active_) {
            stage_ = Stage::RELEASE;
        }
    }
    
    void reset() {
        stage_ = Stage::IDLE;
        level_ = 0.0f;
        active_ = false;
    }
    
    float processSample() {
        if (!active_) return 0.0f;
        
        float increment = 1.0f / (sampleRate_ * 0.1f); // Simple increment
        
        switch (stage_) {
            case Stage::ATTACK:
                level_ += increment / attack_;
                if (level_ >= 1.0f) {
                    level_ = 1.0f;
                    stage_ = Stage::DECAY;
                }
                break;
            case Stage::DECAY:
                level_ -= increment / decay_;
                if (level_ <= sustain_) {
                    level_ = sustain_;
                    stage_ = Stage::SUSTAIN;
                }
                break;
            case Stage::SUSTAIN:
                level_ = sustain_;
                break;
            case Stage::RELEASE:
                level_ -= increment / release_;
                if (level_ <= 0.0f) {
                    level_ = 0.0f;
                    stage_ = Stage::IDLE;
                    active_ = false;
                }
                break;
            case Stage::IDLE:
                level_ = 0.0f;
                break;
        }
        
        return level_ * depth_;
    }
    
    bool isActive() const { return active_; }
    bool isComplete() const { return stage_ == Stage::IDLE; }
    float getCurrentLevel() const { return level_ * depth_; }
    
private:
    float sampleRate_ = 44100.0f;
    float attack_ = 0.001f;
    float decay_ = 0.1f;
    float sustain_ = 0.7f;
    float release_ = 0.5f;
    float depth_ = 1.0f;
    bool exponential_ = true;
    
    Stage stage_ = Stage::IDLE;
    float level_ = 0.0f;
    bool active_ = false;
};