#include "EffectsChain.h"
#include <iostream>

EffectsChain::EffectsChain() {
    initialized_ = true;
    std::cout << "EffectsChain created" << std::endl;
}

EffectsChain::~EffectsChain() {
    std::cout << "EffectsChain destroyed" << std::endl;
}

void EffectsChain::process(EtherAudioBuffer& buffer) {
    // Stub implementation - pass through for now
}

void EffectsChain::addEffect(int effectType) {
    std::cout << "EffectsChain: Added effect type " << effectType << std::endl;
}

void EffectsChain::removeEffect(size_t index) {
    std::cout << "EffectsChain: Removed effect " << index << std::endl;
}