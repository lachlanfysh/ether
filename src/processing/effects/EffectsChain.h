#pragma once
#include "../core/Types.h"

/**
 * Effects chain processor
 * Stub implementation for initial testing
 */
class EffectsChain {
public:
    EffectsChain();
    ~EffectsChain();
    
    // Processing
    void process(EtherAudioBuffer& buffer);
    
    // Effect management
    void addEffect(int effectType);
    void removeEffect(size_t index);
    
private:
    bool initialized_ = false;
};