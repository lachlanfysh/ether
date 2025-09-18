#pragma once

#include <string>
#include "../../src/core/Types.h"

namespace param_utils {

inline std::string getParameterName(ParameterID pid) {
    switch (pid) {
        case ParameterID::HARMONICS: return "harmonics";
        case ParameterID::TIMBRE: return "timbre";
        case ParameterID::MORPH: return "morph";
        case ParameterID::OSC_MIX: return "oscmix";
        case ParameterID::DETUNE: return "detune";
        case ParameterID::SUB_LEVEL: return "sublevel";
        case ParameterID::SUB_ANCHOR: return "subanchor";
        case ParameterID::FILTER_CUTOFF: return "lpf";
        case ParameterID::FILTER_RESONANCE: return "resonance";
        case ParameterID::ATTACK: return "attack";
        case ParameterID::DECAY: return "decay";
        case ParameterID::SUSTAIN: return "sustain";
        case ParameterID::RELEASE: return "release";
        case ParameterID::REVERB_SIZE: return "reverb_size";
        case ParameterID::REVERB_DAMPING: return "reverb_damp";
        case ParameterID::REVERB_MIX: return "reverb";
        case ParameterID::DELAY_TIME: return "delay_time";
        case ParameterID::DELAY_FEEDBACK: return "delay_fb";
        case ParameterID::VOLUME: return "volume";
        case ParameterID::PAN: return "pan";
        case ParameterID::HPF: return "hpf";
        case ParameterID::ACCENT_AMOUNT: return "accent";
        case ParameterID::GLIDE_TIME: return "glide";
        case ParameterID::AMPLITUDE: return "amp";
        case ParameterID::CLIP: return "clip";
        default: return "unknown";
    }
}

} // namespace param_utils
