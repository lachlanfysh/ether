#include "GranularEngine.h"
#include <cmath>

GranularEngine::GranularEngine() {
    std::random_device rd; rng_ = std::mt19937(rd());
}

void GranularEngine::noteOn(uint8_t /*note*/, float velocity, float /*aftertouch*/) {
    volume_ = std::clamp(velocity, 0.0f, 1.0f);
    active_ = true;
}

void GranularEngine::noteOff(uint8_t /*note*/) {
    active_ = false;
}

void GranularEngine::setParameter(ParameterID param, float value) {
    float v = std::clamp(value, 0.0f, 1.0f);
    switch (param) {
        case ParameterID::HARMONICS: texture_ = v; break;
        case ParameterID::TIMBRE: size_ = v; break;
        case ParameterID::MORPH: jitter_ = v; break;
        case ParameterID::VOLUME: volume_ = v; break;
        default: break; // Others handled by post-chain
    }
}

float GranularEngine::getParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::HARMONICS: return texture_;
        case ParameterID::TIMBRE: return size_;
        case ParameterID::MORPH: return jitter_;
        case ParameterID::VOLUME: return volume_;
        default: return 0.5f;
    }
}

void GranularEngine::processAudio(EtherAudioBuffer& output) {
    // Minimal grain cloud: sum a few short windowed sines with random phase/offset
    const float baseHz = 220.0f * std::pow(2.0f, (pitch_ - 0.5f) * 2.0f); // ~110..440
    const float winDur = 0.02f + size_ * 0.18f; // 20..200 ms
    const int grains = 1 + int(2 + density_ * 6); // 3..8 grains per block
    for (auto &f : output) { f.left = 0.0f; f.right = 0.0f; }
    for (int g=0; g<grains; ++g) {
        float freq = baseHz * (0.8f + 0.4f * (uni_(rng_) - 0.5f) * jitter_);
        float start = uni_(rng_) * (float)output.size();
        for (size_t i=0;i<output.size();++i) {
            float t = (float)i / sampleRate_;
            float p = 2.0f * (float)M_PI * freq * t + (float)M_PI * 2.0f * uni_(rng_);
            // Hann window over winDur seconds centered at start
            float rel = ((float)i - start) / (winDur * sampleRate_);
            float w = 0.0f;
            if (fabsf(rel) < 0.5f) {
                float x = (rel + 0.5f) * (float)M_PI; // 0..pi
                w = 0.5f - 0.5f * cosf(x);
                // Texture: harder window reduces tails
                w = std::pow(w, 0.5f + texture_);
            }
            float s = std::sinf(p) * w * volume_ * 0.2f;
            float pan = (spread_ - 0.5f) * 2.0f; // -1..+1
            float ang = (pan + 1.0f) * 0.25f * (float)M_PI;
            float gL = cosf(ang), gR = sinf(ang);
            output[i].left  += s * gL;
            output[i].right += s * gR;
        }
    }
}

