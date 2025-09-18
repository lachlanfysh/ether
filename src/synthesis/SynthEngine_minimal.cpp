#include "SynthEngine.h"
#include "../core/CoreParameters.h"

using namespace EtherSynth;

void SynthEngine::setCoreParameter(CoreParameter param, float value) {
    float validated = validateCoreParameter(param, value);
    coreParams_[param] = validated;
    updatePostChain();
}

float SynthEngine::getCoreParameter(CoreParameter param) const {
    return coreParams_[param];
}

float SynthEngine::validateCoreParameter(CoreParameter param, float value) const {
    return ParameterUtils::validateParameter(param, value);
}

void SynthEngine::updatePostChain() {
    bool hasNativeFilter = hasNativeCoreSupport(PARAM_FILTER_CUTOFF) ||
                           hasNativeCoreSupport(PARAM_FILTER_RESONANCE);
    postProcessor_.updateParameters(coreParams_, hasNativeFilter);
    postProcessor_.setSampleRate(sampleRate_);
}

