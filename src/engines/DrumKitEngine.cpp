 #include "DrumKitEngine.h"
 #include <algorithm>
 
DrumKitEngine::DrumKitEngine() {
    for (auto &v : voices_) v.active = false;
    for (int i=0;i<16;++i){ padDecay_[i]=0.5f; padTune_[i]=0.0f; padLevel_[i]=0.85f; padPan_[i]=0.5f; }
}
 
static inline float fastExpMulFromMs(float ms, float sr) {
    // Per-sample exponential decay multiplier for time constant in milliseconds
    // A[n] = A0 * exp(-n / (tau * sr)), tau (seconds) = ms / 1000
    // => per-sample multiplier m = exp(-1 / (tau * sr)) = exp(-1000 / (ms * sr))
    if (ms < 0.1f) ms = 0.1f;
    return std::exp(-1000.0f / (ms * sr));
}

void DrumKitEngine::noteOn(uint8_t note, float velocity, float /*aftertouch*/) {
    float vel = std::clamp(velocity, 0.1f, 1.0f);
    int pad = mapNoteToPad(note);

    // Choke: CH/PH choke OH
    if (note == 42 || note == 44) {
        for (auto &v : voices_) if (v.active && v.type == DrumVoice::Type::HAT_O) v.active = false;
    }

    auto startVoice = [&](DrumVoice::Type t){
        DrumVoice* vp = nullptr;
        for (auto &v : voices_) { if (!v.active) { vp = &v; break; } }
        if (!vp) {
            // steal oldest/quietest
            vp = &voices_[0];
            for (auto &v : voices_) {
                if (v.lifeSamples > vp->lifeSamples) vp = &v;
            }
        }
        vp->active = true; vp->type = t; vp->padIndex = pad;
        // defaults
        vp->amp = vel * ((pad>=0&&pad<16)? padLevel_[pad] : 1.0f);
        float decMs = 40.0f + ((pad>=0&&pad<16)? padDecay_[pad] : 0.5f) * 600.0f; // 40..640ms
        vp->ampMul = fastExpMulFromMs(decMs, sampleRate_);
        vp->noise = vp->amp; vp->noiseMul = fastExpMulFromMs(decMs*0.7f, sampleRate_);
        vp->phase = 0.0f; vp->openHold = 1.0f; vp->clapTime = 0.0f;
        // HPF/LPF coef init
        float hpfCut = 40.0f; float rc = 1.0f / (2.0f * M_PI * hpfCut), dt = 1.0f / sampleRate_;
        vp->hpf_a = rc / (rc + dt); vp->hpf_y1 = vp->hpf_x1 = 0.0f;
        float lpCut = 8000.0f; float alpha = dt / (dt + 1.0f/(2.0f*M_PI*lpCut));
        vp->lp_a = alpha; vp->lp_y1 = 0.0f;
        // metallic partials
        for (int i=0;i<6;++i) { vp->metalPh[i]=0.0f; }
        // per-type init
        switch (t) {
            case DrumVoice::Type::KICK: {
                // Base frequency (808 ~55 Hz). Tune is in octaves (-1..1)
                float base = (kit_ == Kit::K808) ? 55.0f : 65.0f;
                float tune = (pad>=0&&pad<16)? padTune_[pad] : 0.0f;
                vp->freq = base * std::pow(2.0f, tune);
                // Two-stage downward pitch envelope: quick knock + slower tail
                // Fast ~8 ms to ~0, Slow ~120 ms to ~0
                auto mulMs = [&](float ms){ return fastExpMulFromMs(ms, sampleRate_); };
                vp->pitchFast = (kit_ == Kit::K808) ? 220.0f : 260.0f;  // initial extra Hz
                vp->pitchSlow = (kit_ == Kit::K808) ? 40.0f  : 60.0f;   // tail bend
                vp->pitchMulFast = mulMs(8.0f);
                vp->pitchMulSlow = mulMs(120.0f);
                // Transient drive for subtle harmonics (decays ~30 ms)
                vp->driveEnv = 1.0f;
                vp->driveMul = mulMs(30.0f);
                // Envelope/time limit
                vp->maxSamples = (int)(sampleRate_ * 1.6f);
            } break;
            case DrumVoice::Type::KICK_2: {
                // Punchier, shorter kick (909-ish flavor)
                float base = (kit_ == Kit::K808) ? 70.0f : 80.0f;
                float tune = (pad>=0&&pad<16)? padTune_[pad] : 0.0f;
                vp->freq = base * std::pow(2.0f, tune);
                auto mulMs = [&](float ms){ return fastExpMulFromMs(ms, sampleRate_); };
                vp->pitchFast = 360.0f;    // stronger initial knock
                vp->pitchSlow = 25.0f;     // subtle tail
                vp->pitchMulFast = mulMs(4.0f);
                vp->pitchMulSlow = mulMs(70.0f);
                vp->driveEnv = 1.0f; vp->driveMul = mulMs(80.0f);
                vp->maxSamples = (int)(sampleRate_ * 0.8f);
            } break;
            case DrumVoice::Type::TOM_L1: case DrumVoice::Type::TOM_L2:
            case DrumVoice::Type::TOM_M1: case DrumVoice::Type::TOM_M2:
            case DrumVoice::Type::TOM_H1: case DrumVoice::Type::TOM_H2: {
                float base = 110.0f;
                if (t == DrumVoice::Type::TOM_L2) base=130.0f;
                if (t == DrumVoice::Type::TOM_M1) base=160.0f;
                if (t == DrumVoice::Type::TOM_M2) base=190.0f;
                if (t == DrumVoice::Type::TOM_H1) base=220.0f;
                if (t == DrumVoice::Type::TOM_H2) base=260.0f;
                float tune = (pad>=0&&pad<16)? padTune_[pad] : 0.0f;
                vp->freq = base * std::pow(2.0f, tune);
                // Stronger initial bend and short noise burst for attack
                vp->pitch = 100.0f; vp->pitchMul = 0.996f;
                vp->noise = vp->amp * 0.8f; vp->noiseMul = fastExpMulFromMs(30.0f, sampleRate_);
                // Set a modest band for the click
                float dt2 = 1.0f / sampleRate_;
                float hpfCut2 = 600.0f; float rc2 = 1.0f / (2.0f * M_PI * hpfCut2);
                vp->hpf_a = rc2 / (rc2 + dt2); vp->hpf_y1 = vp->hpf_x1 = 0.0f;
                float lpCut2 = 4500.0f; float alpha2 = dt2 / (dt2 + 1.0f/(2.0f*M_PI*lpCut2));
                vp->lp_a = alpha2; vp->lp_y1 = 0.0f;
                vp->maxSamples = (int)(sampleRate_ * 1.2f);
            } break;
            case DrumVoice::Type::SNARE: {
                // 808-like: two detuned tones (~186 Hz and ~332 Hz) + bandpassed noise
                auto mulMs = [&](float ms){ return fastExpMulFromMs(ms, sampleRate_); };
                vp->freq = 186.0f; vp->phase = 0.0f;
                vp->freq2 = 332.0f; vp->phase2 = 0.0f;
                vp->toneEnv = 1.0f; vp->toneMul = mulMs(70.0f);   // short tone decay
                vp->noise = vp->amp * 1.0f; vp->noiseMul = mulMs(180.0f); // longer noise tail
                // Shape the noise with a simple bandpass (HP ~700 Hz, LP ~5 kHz)
                float dt = 1.0f / sampleRate_;
                float hpfCut = 700.0f; float rcH = 1.0f / (2.0f * M_PI * hpfCut);
                vp->hpf_a = rcH / (rcH + dt); vp->hpf_y1 = 0.0f; vp->hpf_x1 = 0.0f;
                float lpCut = 5000.0f; float alpha = dt / (dt + 1.0f/(2.0f*M_PI*lpCut));
                vp->lp_a = alpha; vp->lp_y1 = 0.0f;
                vp->maxSamples = (int)(sampleRate_ * 0.8f);
            } break;
            case DrumVoice::Type::SNARE_2: {
                // Brighter crack variant
                auto mulMs = [&](float ms){ return fastExpMulFromMs(ms, sampleRate_); };
                vp->freq = 210.0f; vp->phase = 0.0f;
                vp->freq2 = 380.0f; vp->phase2 = 0.0f;
                vp->toneEnv = 1.0f; vp->toneMul = mulMs(45.0f);
                vp->noise = vp->amp * 1.2f; vp->noiseMul = mulMs(150.0f);
                float dt = 1.0f / sampleRate_;
                float hpfCut = 1200.0f; float rcH = 1.0f / (2.0f * M_PI * hpfCut);
                vp->hpf_a = rcH / (rcH + dt); vp->hpf_y1 = vp->hpf_x1 = 0.0f;
                float lpCut = 6500.0f; float alpha = dt / (dt + 1.0f/(2.0f*M_PI*lpCut));
                vp->lp_a = alpha; vp->lp_y1 = 0.0f;
                vp->maxSamples = (int)(sampleRate_ * 0.7f);
            } break;
            case DrumVoice::Type::CLAP: {
                // Multi-slap envelope + bright noise tail
                vp->clapTime = 0.0f;
                // Re-tune filters for clap band (HP ~800 Hz, LP ~6 kHz)
                float dt = 1.0f / sampleRate_;
                float hpfCut = 650.0f; float rcH = 1.0f / (2.0f * M_PI * hpfCut);
                vp->hpf_a = rcH / (rcH + dt); vp->hpf_y1 = 0.0f; vp->hpf_x1 = 0.0f;
                float lpCut = 4500.0f; float alpha = dt / (dt + 1.0f/(2.0f*M_PI*lpCut));
                vp->lp_a = alpha; vp->lp_y1 = 0.0f;
                vp->maxSamples = (int)(sampleRate_ * 0.8f);
            } break;
            case DrumVoice::Type::HAT_C: case DrumVoice::Type::HAT_P: case DrumVoice::Type::HAT_O: {
                // 6 metallic square partials with band-limited brightness; tune adjusts base
                static const float ratios[6] = {2.0f, 3.0f, 4.16f, 5.43f, 6.79f, 8.21f};
                float base = 320.0f;
                float tune = (pad>=0&&pad<16)? padTune_[pad] : 0.0f; // -1..1 octaves
                base *= std::pow(2.0f, tune * 0.5f); // keep range reasonable
                for (int i=0;i<6;++i) { vp->metalFreq[i] = base * ratios[i]; vp->metalPh[i] = 0.0f; }
                // Per-type decay
                vp->ampMul = (t==DrumVoice::Type::HAT_O)? 0.9996f : 0.9945f;
                vp->maxSamples = (int)(sampleRate_ * ((t==DrumVoice::Type::HAT_O)? 2.4f : 0.25f));
                // Filters: HP around 3.5 kHz, LP around 9 kHz; closed is slightly darker
                float dt = 1.0f / sampleRate_;
                float hpfCut = (t==DrumVoice::Type::HAT_O) ? 2600.0f : (t==DrumVoice::Type::HAT_P ? 5200.0f : 6000.0f);
                float lpCut  = (t==DrumVoice::Type::HAT_O) ? 12000.0f : (t==DrumVoice::Type::HAT_P ? 9500.0f : 9000.0f);
                float rcH = 1.0f / (2.0f * M_PI * hpfCut);
                vp->hpf_a = rcH / (rcH + dt); vp->hpf_y1 = 0.0f; vp->hpf_x1 = 0.0f;
                float alpha = dt / (dt + 1.0f/(2.0f*M_PI*lpCut));
                vp->lp_a = alpha; vp->lp_y1 = 0.0f;
                // Add noise envelope for more noisy character
                if (t==DrumVoice::Type::HAT_C) {
                    vp->noise = vp->amp * 1.9f; vp->noiseMul = fastExpMulFromMs(90.0f, sampleRate_);
                } else if (t==DrumVoice::Type::HAT_P) {
                    vp->noise = vp->amp * 1.5f; vp->noiseMul = fastExpMulFromMs(140.0f, sampleRate_);
                } else { // HAT_O
                    vp->noise = vp->amp * 1.2f; vp->noiseMul = fastExpMulFromMs(650.0f, sampleRate_);
                }
            } break;
            case DrumVoice::Type::CRASH: case DrumVoice::Type::RIDE: {
                // Set metallic cluster freqs and filter bands
                static const float ratios[6] = {2.0f, 2.71f, 3.98f, 5.12f, 6.37f, 7.54f};
                float base = 420.0f;
                for (int i=0;i<6;++i) { vp->metalFreq[i] = base * ratios[i]; vp->metalPh[i] = 0.0f; }
                float dt = 1.0f / sampleRate_;
                float hpfCut = t==DrumVoice::Type::RIDE ? 2200.0f : 2600.0f;
                float lpCut  = t==DrumVoice::Type::RIDE ? 9000.0f : 11000.0f;
                float rcH = 1.0f / (2.0f * M_PI * hpfCut);
                vp->hpf_a = rcH / (rcH + dt); vp->hpf_y1 = vp->hpf_x1 = 0.0f;
                float alpha = dt / (dt + 1.0f/(2.0f*M_PI*lpCut));
                vp->lp_a = alpha; vp->lp_y1 = 0.0f;
                vp->ampMul = (t == DrumVoice::Type::RIDE)? 0.9997f : 0.9994f;
                vp->maxSamples = (int)(sampleRate_ * ((t==DrumVoice::Type::RIDE)? 7.0f : 5.0f));
            } break;
            case DrumVoice::Type::COWBELL: {
                // Woodblock-like cowbell: short bright click + pitched ping ~450 Hz (down an octave)
                float tune = (pad>=0&&pad<16)? padTune_[pad] : 0.0f; // -1..1 (reduced)
                float base = 450.0f * std::pow(2.0f, tune * 0.3f);
                vp->freq = base; vp->phase = 0.0f;
                // Noise burst for impact
                vp->noise = vp->amp * 1.0f; vp->noiseMul = fastExpMulFromMs(35.0f, sampleRate_);
                // Band-limit around 600 Hz .. 5.5 kHz
                float dt = 1.0f / sampleRate_;
                float hpfCut = 600.0f; float rcH = 1.0f / (2.0f * M_PI * hpfCut);
                vp->hpf_a = rcH / (rcH + dt); vp->hpf_y1 = vp->hpf_x1 = 0.0f;
                float lpCut = 5500.0f; float alpha = dt / (dt + 1.0f/(2.0f*M_PI*lpCut));
                vp->lp_a = alpha; vp->lp_y1 = 0.0f;
                vp->maxSamples = (int)(sampleRate_ * 0.45f);
            } break;
            case DrumVoice::Type::SHAKER: {
                // High-passed noise burst
                vp->noise = vp->amp * 1.0f; vp->noiseMul = fastExpMulFromMs(140.0f, sampleRate_);
                float dt = 1.0f / sampleRate_;
                float hpfCut = 2000.0f; float rcH = 1.0f / (2.0f * M_PI * hpfCut);
                vp->hpf_a = rcH / (rcH + dt); vp->hpf_y1 = vp->hpf_x1 = 0.0f;
                float lpCut = 10000.0f; float alpha = dt / (dt + 1.0f/(2.0f*M_PI*lpCut));
                vp->lp_a = alpha; vp->lp_y1 = 0.0f;
                vp->maxSamples = (int)(sampleRate_ * 0.35f);
            } break;
            case DrumVoice::Type::RIM: default: break;
        }
        vp->lifeSamples = 0;
    };

    switch (note) {
        case 36: startVoice(DrumVoice::Type::KICK); break;
        case 35: startVoice(DrumVoice::Type::KICK_2); break;
        case 38: startVoice(DrumVoice::Type::SNARE); break;
        case 40: startVoice(DrumVoice::Type::SNARE_2); break;
        case 37: startVoice(DrumVoice::Type::RIM); break;
        case 39: startVoice(DrumVoice::Type::CLAP); break;
        case 41: startVoice(DrumVoice::Type::TOM_L1); break;
        case 43: startVoice(DrumVoice::Type::TOM_L2); break;
        case 45: startVoice(DrumVoice::Type::TOM_M1); break;
        case 47: startVoice(DrumVoice::Type::TOM_M2); break;
        case 48: startVoice(DrumVoice::Type::TOM_H1); break;
        case 50: startVoice(DrumVoice::Type::TOM_H2); break;
        case 42: startVoice(DrumVoice::Type::HAT_C); break;
        case 44: startVoice(DrumVoice::Type::HAT_P); break;
        case 46: startVoice(DrumVoice::Type::HAT_O); break;
        case 49: startVoice(DrumVoice::Type::CRASH); break;
        case 51: startVoice(DrumVoice::Type::RIDE); break;
        case 56: startVoice(DrumVoice::Type::COWBELL); break;
        case 70: startVoice(DrumVoice::Type::SHAKER); break;
        default: startVoice(DrumVoice::Type::SNARE); break;
    }
}
 
 void DrumKitEngine::noteOff(uint8_t note) {
     // hats may close
     if (note == 42) {
         for (auto &v : voices_) if (v.active && v.type == DrumVoice::Type::HAT_O) v.openHold = 0.2f;
     }
 }
 
int DrumKitEngine::mapNoteToPad(int note) {
    switch (note) {
        case 36: return 0;   // Kick A
        case 38: return 1;   // Snare A
        case 49: return 2;   // Crash
        case 39: return 3;   // Clap
        case 41: return 4;   // Tom L
        case 45: return 5;   // Tom M
        case 48: return 6;   // Tom H
        case 37: return 7;   // Rim
        case 42: return 8;   // CH
        case 44: return 9;   // PH
        case 46: return 10;  // OH
        case 51: return 11;  // Ride
        case 56: return 12;  // Cowbell
        case 35: return 13;  // Kick B
        case 40: return 14;  // Snare B
        case 70: return 15;  // Shaker
        default: return 15;
    }
}

void DrumKitEngine::allNotesOff() {
    for (auto &v : voices_) v.active = false;
}
 
 void DrumKitEngine::setParameter(ParameterID pid, float value) {
     value = std::clamp(value, 0.0f, 1.0f);
     switch (pid) {
         case ParameterID::TIMBRE:
             kit_ = (value < 0.5f) ? Kit::K808 : Kit::K909;
             break;
         case ParameterID::VOLUME:
             volume_ = value;
             break;
         case ParameterID::PAN:
             pan_ = value;
             break;
         case ParameterID::DECAY:
             // Map 0..1 to a global decay time scaler 0.25x..2.0x
             decayScale_ = 0.25f + value * 1.75f;
             break;
         default: break;
     }
 }
 
 float DrumKitEngine::getParameter(ParameterID pid) const {
     switch (pid) {
         case ParameterID::TIMBRE: return kit_ == Kit::K808 ? 0.0f : 1.0f;
         case ParameterID::VOLUME: return volume_;
         case ParameterID::PAN: return pan_;
         case ParameterID::DECAY: return std::clamp((decayScale_ - 0.25f) / 1.75f, 0.0f, 1.0f);
         default: return 0.0f;
     }
 }
 
 size_t DrumKitEngine::getActiveVoiceCount() const {
     size_t c = 0; for (auto &v : voices_) if (v.active) ++c; return c;
 }
 
 void DrumKitEngine::processAudio(EtherAudioBuffer &buffer) {
     for (auto &f : buffer) { f.left = 0.0f; f.right = 0.0f; }
 
    auto kickSample = [&](DrumVoice &v){
        // 808-like: two-stage pitch sweep, short click, gentle drive/harmonics
        v.pitchFast *= v.pitchMulFast;
        v.pitchSlow *= v.pitchMulSlow;
        float f = v.freq + v.pitchFast + 0.35f * v.pitchSlow;
        // Integrate phase
        v.phase += f / sampleRate_;
        if (v.phase >= 1.0f) v.phase -= 1.0f;
        float ph = v.phase * 2.0f * M_PI;
        // Body with a touch of second harmonic for knock
        float body = std::sin(ph) + 0.12f * std::sin(2.0f * ph);
        // Short high-passed noise click (~3 ms)
        float click = 0.0f;
        const int clickSamples = (int)(0.003f * sampleRate_);
        if (v.lifeSamples < clickSamples) {
            float n = frand(v.noiseSeed);
            // simple 1-pole HP
            float hp = v.hpf_a * (v.hpf_y1 + n - v.hpf_x1); v.hpf_y1 = hp; v.hpf_x1 = n;
            // shape click envelope to bias earliest samples
            float e = 1.0f - (float)v.lifeSamples / (float)clickSamples;
            click = hp * (0.10f + 0.15f * e); // up to ~-16 dB
        }
        // Gentle transient drive that quickly reduces to near-linear
        v.driveEnv *= v.driveMul;
        float driveAmt = (kit_ == Kit::K909) ? 2.4f * v.driveEnv + 1.2f : 1.2f * v.driveEnv + 1.0f;
        float s = body + click;
        s = std::tanh(s * driveAmt);
        return s * v.amp;
    };
    auto snareSample = [&](DrumVoice &v){
        // Two detuned tone oscillators with short decay + bright bandpassed noise with transient crack
        v.phase += v.freq / sampleRate_; if (v.phase >= 1.0f) v.phase -= 1.0f;
        v.phase2 += v.freq2 / sampleRate_; if (v.phase2 >= 1.0f) v.phase2 -= 1.0f;
        v.toneEnv *= v.toneMul;
        float tone = (std::sin(2.0f * M_PI * v.phase) + 0.65f * std::sin(2.0f * M_PI * v.phase2)) * (0.35f * v.toneEnv);
        // Noise component with its own envelope and bandpass
        v.noise *= v.noiseMul;
        float n = frand(v.noiseSeed) * v.noise;
        float hp = v.hpf_a * (v.hpf_y1 + n - v.hpf_x1); v.hpf_y1 = hp; v.hpf_x1 = n;
        v.lp_y1 = v.lp_y1 + v.lp_a * (hp - v.lp_y1);
        float nz = v.lp_y1;
        // Add a very short crack at onset (extra bright noise)
        const int crackSamps = (int)(0.002f * sampleRate_);
        if (v.lifeSamples < crackSamps) {
            float e = 1.0f - (float)v.lifeSamples / (float)crackSamps;
            float crackAmt = (v.type == DrumVoice::Type::SNARE_2) ? 0.9f : 0.6f;
            float crack = (frand(v.noiseSeed) * crackAmt) * e; // brighter crack for SNARE_2
            // mix pre-LP to keep it bright
            nz = nz + crack;
        }
        float s = tone + nz * 0.9f;
        // Mild saturation to emphasize bite
        float drive = (v.type == DrumVoice::Type::SNARE_2) ? 1.9f : 1.6f;
        s = std::tanh(s * drive);
        return s * v.amp;
    };
    auto rimSample = [&](DrumVoice &v){
        // Woody side-stick: short 2 kHz ping + click
        float click = 0.0f;
        const int clickSamps = (int)(0.0015f * sampleRate_);
        if (v.lifeSamples < clickSamps) {
            float e = 1.0f - (float)v.lifeSamples / (float)clickSamps;
            float n = frand(v.noiseSeed);
            click = (n - 0.9f*n) * 0.9f * e;
        }
        // 2 kHz short sine
        float f = 2000.0f;
        v.phase += f / sampleRate_; if (v.phase >= 1.0f) v.phase -= 1.0f;
        float toneEnv = std::exp(-(float)v.lifeSamples / (sampleRate_ * 0.015f)); // ~15 ms
        float tone = std::sin(2.0f * M_PI * v.phase) * 0.4f * toneEnv;
        return (click + tone) * v.amp;
    };
    auto clapSample = [&](DrumVoice &v){
        // Four slaps around 0, 23ms, 47ms, 71ms + diffused tail
        v.clapTime += 1.0f / sampleRate_;
        float t = v.clapTime;
        auto pulse = [](float t, float center, float width){
            float x = (t - center) / width; // simple gaussian-ish
            return std::exp(-x * x * 8.0f);
        };
        float env = 0.0f;
        env += 1.00f * pulse(t, 0.000f, 0.004f);
        env += 0.85f * pulse(t, 0.023f, 0.004f);
        env += 0.70f * pulse(t, 0.047f, 0.004f);
        env += 0.55f * pulse(t, 0.071f, 0.004f);
        // Tail
        float tail = std::exp(-std::max(0.0f, t - 0.071f) * 18.0f);
        // Noise
        float n = frand(v.noiseSeed);
        // High-pass then low-pass to band-limit
        float hp = v.hpf_a * (v.hpf_y1 + n - v.hpf_x1); v.hpf_y1 = hp; v.hpf_x1 = n;
        v.lp_y1 = v.lp_y1 + v.lp_a * (hp - v.lp_y1);
        float s = (v.lp_y1 * env + v.lp_y1 * 0.6f * tail);
        // Subtle saturation
        s = std::tanh(s * 2.0f);
        return s * v.amp;
    };
    auto tomSample = [&](DrumVoice &v, float base){
        // Snappy toms: filtered noise impact + stronger bend + 2nd harmonic
        float tune = (v.padIndex>=0&&v.padIndex<16) ? (padTune_[v.padIndex]*0.5f) : 0.0f;
        v.freq = base * std::pow(2.0f, tune);
        // Band-passed noise impact using per-voice filters
        float impact = 0.0f;
        const int cS = (int)(0.0020f * sampleRate_);
        if (v.lifeSamples < cS) {
            float e = 1.0f - (float)v.lifeSamples / (float)cS;
            float n = frand(v.noiseSeed) * e * 1.2f;
            float hp = v.hpf_a * (v.hpf_y1 + n - v.hpf_x1); v.hpf_y1 = hp; v.hpf_x1 = n;
            v.lp_y1 = v.lp_y1 + v.lp_a * (hp - v.lp_y1);
            impact = v.lp_y1 * 0.85f;
        }
        // Stronger pitch bend using configured multiplier
        v.pitch *= v.pitchMul; float f = v.freq + v.pitch;
        v.phase += f / sampleRate_; if (v.phase >= 1.0f) v.phase -= 1.0f;
        float ph = v.phase * 2.0f * M_PI;
        float body = std::sin(ph) + 0.10f * std::sin(2.0f*ph);
        return (body + impact) * v.amp;
    };
    auto cymSample = [&](DrumVoice &v, bool ride){
        // 808-inspired cymbals: cluster + (for crash) noise wash with splash onset
        float cluster = 0.0f;
        for (int i=0;i<6;++i) {
            v.metalPh[i] += v.metalFreq[i] / sampleRate_;
            if (v.metalPh[i] >= 1.0f) v.metalPh[i] -= 1.0f;
            cluster += (v.metalPh[i] < 0.5f ? 1.0f : -1.0f);
        }
        cluster = (cluster / 6.0f) * 0.08f; // extremely small tonal component
        float s = 0.0f;
        if (ride) {
            // Mostly smoothed metallic with a touch of noise
            float hp = v.hpf_a * (v.hpf_y1 + cluster - v.hpf_x1); v.hpf_y1 = hp; v.hpf_x1 = cluster;
            v.lp_y1 = v.lp_y1 + v.lp_a * (hp - v.lp_y1);
            s = v.lp_y1 * (0.80f + 0.02f * frand(v.noiseSeed));
        } else {
            // Crash: heavily noise-based splash using snare-like band for the entire tail
            float t = (float)v.lifeSamples / sampleRate_;
            // Dynamic LP: open high then settle
            float lpStart = 12000.0f, lpEnd = 7000.0f;
            float lpCut = lpEnd + (lpStart - lpEnd) * std::exp(-t * 18.0f);
            float dt = 1.0f / sampleRate_;
            float alpha = dt / (dt + 1.0f/(2.0f*M_PI*lpCut));
            v.lp_a = alpha;
            float n = frand(v.noiseSeed);
            float hpN = v.hpf_a * (v.hpf_y1 + n - v.hpf_x1); v.hpf_y1 = hpN; v.hpf_x1 = n;
            v.lp_y1 = v.lp_y1 + v.lp_a * (hpN - v.lp_y1);
            float noiseBand = v.lp_y1;
            float splash = std::exp(-t * 70.0f); // fast noisy onset
            float tail   = std::exp(-t * 1.5f);  // steady tail
            float noisy  = noiseBand * (0.98f * splash + 0.78f * tail);
            // Metallic support: extremely small, fades quickly
            float wM = std::max(0.0f, std::min(0.05f, (t - 0.050f) * 0.4f));
            float metal = cluster * wM * std::exp(-t * 10.0f);
            s = (noisy + metal) * (0.98f + 0.04f * frand(v.noiseSeed));
        }
        return s * v.amp;
    };
    auto cowbellSample = [&](DrumVoice &v){
        // Woodblock-leaning cowbell: impact + short pitched ping
        float s = 0.0f;
        // Impact click
        const int clickSamps = (int)(0.0025f * sampleRate_);
        if (v.lifeSamples < clickSamps) {
            float e = 1.0f - (float)v.lifeSamples / (float)clickSamps;
            float n = frand(v.noiseSeed);
            float hp = v.hpf_a * (v.hpf_y1 + n - v.hpf_x1); v.hpf_y1 = hp; v.hpf_x1 = n;
            v.lp_y1 = v.lp_y1 + v.lp_a * (hp - v.lp_y1);
            s += v.lp_y1 * (0.8f * e);
        }
        // Pitched ping ~freq
        v.phase += v.freq / sampleRate_; if (v.phase >= 1.0f) v.phase -= 1.0f;
        float tEnv = std::exp(-(float)v.lifeSamples / (sampleRate_ * 0.12f));
        s += std::sin(2.0f * M_PI * v.phase) * 0.7f * tEnv;
        return s * v.amp;
    };
    auto shakerSample = [&](DrumVoice &v){
        // Bright, short noise burst with slight randomness
        v.noise *= v.noiseMul;
        float n = frand(v.noiseSeed) * v.noise;
        // add tiny random amplitude wobble
        n *= 0.9f + 0.1f * frand(v.noiseSeed);
        float hp = v.hpf_a * (v.hpf_y1 + n - v.hpf_x1); v.hpf_y1 = hp; v.hpf_x1 = n;
        v.lp_y1 = v.lp_y1 + v.lp_a * (hp - v.lp_y1);
        return v.lp_y1 * v.amp;
    };
    auto hatSample = [&](DrumVoice &v, bool open){
        // Metallic cluster
        float cluster = 0.0f;
        for (int i=0;i<6;++i) {
            v.metalPh[i] += v.metalFreq[i] / sampleRate_;
            if (v.metalPh[i] >= 1.0f) v.metalPh[i] -= 1.0f;
            cluster += (v.metalPh[i] < 0.5f ? 1.0f : -1.0f);
        }
        float t = (float)v.lifeSamples / sampleRate_;
        cluster = (cluster / 6.0f) * (open ? 0.05f : 0.08f) * std::exp(-t * (open ? 12.0f : 9.0f));
        // Noise band via per-voice HP/LP (dominant component)
        float n = frand(v.noiseSeed) * v.noise;
        float hpN = v.hpf_a * (v.hpf_y1 + n - v.hpf_x1); v.hpf_y1 = hpN; v.hpf_x1 = n;
        v.lp_y1 = v.lp_y1 + v.lp_a * (hpN - v.lp_y1);
        float bandNoise = v.lp_y1;
        // Bite envelope and tick for CH/PH only
        float bite = 0.0f, tick = 0.0f;
        if (!open) {
            const int biteS = (int)(0.0030f * sampleRate_);
            if (v.lifeSamples < biteS) {
                float e = 1.0f - (float)v.lifeSamples / (float)biteS;
                bite = e * e; // sharper
            }
            const int tickS = (int)(0.0010f * sampleRate_);
            if (v.lifeSamples < tickS) {
                float e2 = 1.0f - (float)v.lifeSamples / (float)tickS;
                tick = (frand(v.noiseSeed) * 0.65f) * e2;
            }
        }
        // Strong noise bias: closed > pedal > open
        float wNoise = open ? 0.95f : (v.type==DrumVoice::Type::HAT_P ? 0.94f : 0.96f);
        float s = wNoise * (bandNoise * (1.0f + 0.8f * bite) + tick) + (1.0f - wNoise) * cluster;
        // Slight randomization to avoid pure tone
        s *= (0.98f + 0.04f * frand(v.noiseSeed));
        return s * v.amp * (open?0.76f:0.65f);
    };
 
    for (size_t i = 0; i < buffer.size(); ++i) {
        float lmix = 0.0f, rmix = 0.0f;
        for (auto &v : voices_) if (v.active) {
            float s = 0.0f;
            switch (v.type) {
                case DrumVoice::Type::KICK: s = kickSample(v); break;
                case DrumVoice::Type::KICK_2: {
                    // Slightly stronger harmonic for Kick B
                    float tmp = v.amp; v.amp *= 1.1f; s = kickSample(v); v.amp = tmp;
                } break;
                case DrumVoice::Type::SNARE: s = snareSample(v); break;
                case DrumVoice::Type::SNARE_2: s = snareSample(v); break;
                case DrumVoice::Type::RIM: s = rimSample(v); break;
                case DrumVoice::Type::CLAP: s = clapSample(v); break;
                case DrumVoice::Type::HAT_C: s = hatSample(v, false); break;
                case DrumVoice::Type::HAT_P: s = hatSample(v, false) * 0.5f; break;
                case DrumVoice::Type::HAT_O: s = hatSample(v, true); v.openHold *= 0.9995f; break;
                case DrumVoice::Type::TOM_L1: s = tomSample(v, 110.0f); break;
                case DrumVoice::Type::TOM_L2: s = tomSample(v, 130.0f); break;
                case DrumVoice::Type::TOM_M1: s = tomSample(v, 160.0f); break;
                case DrumVoice::Type::TOM_M2: s = tomSample(v, 190.0f); break;
                case DrumVoice::Type::TOM_H1: s = tomSample(v, 220.0f); break;
                case DrumVoice::Type::TOM_H2: s = tomSample(v, 260.0f); break;
                case DrumVoice::Type::CRASH: s = cymSample(v, false); break;
                case DrumVoice::Type::RIDE: s = cymSample(v, true); break;
                case DrumVoice::Type::COWBELL: s = cowbellSample(v); break;
                case DrumVoice::Type::SHAKER: s = shakerSample(v); break;
            }
            int padIndex = v.padIndex;
            float lvl = (padIndex>=0&&padIndex<16)? padLevel_[padIndex] : 1.0f;
            auto baseDecayMs = [&](const DrumVoice& dv) -> float {
                switch (dv.type) {
                    case DrumVoice::Type::KICK: return 750.0f;
                    case DrumVoice::Type::KICK_2: return 380.0f;
                    case DrumVoice::Type::SNARE: return 700.0f;
                    case DrumVoice::Type::SNARE_2: return 550.0f;
                    case DrumVoice::Type::RIM: return 90.0f;
                    case DrumVoice::Type::CLAP: return 420.0f;
                    case DrumVoice::Type::HAT_C: return 130.0f;
                    case DrumVoice::Type::HAT_P: return 200.0f;
                    case DrumVoice::Type::HAT_O: return 900.0f;
                    case DrumVoice::Type::TOM_L1:
                    case DrumVoice::Type::TOM_L2:
                    case DrumVoice::Type::TOM_M1:
                    case DrumVoice::Type::TOM_M2:
                    case DrumVoice::Type::TOM_H1:
                    case DrumVoice::Type::TOM_H2: return 600.0f;
                    case DrumVoice::Type::CRASH: return 2200.0f;
                    case DrumVoice::Type::RIDE: return 5200.0f;
                    case DrumVoice::Type::COWBELL: return 380.0f;
                    case DrumVoice::Type::SHAKER: return 180.0f;
                    default: return 600.0f;
                }
            };
            float padScale = (padIndex>=0&&padIndex<16)? (0.5f + padDecay_[padIndex] * 1.5f) : 1.25f; // 0.5x..2.0x
            float decayMs = baseDecayMs(v) * padScale * decayScale_;
            v.ampMul = fastExpMulFromMs(decayMs, sampleRate_);
            // per-voice pan
            float p = (padIndex>=0&&padIndex<16)? padPan_[padIndex] : 0.5f;
            float thetaV = p * 1.5707963f; float lV = std::cos(thetaV), rV = std::sin(thetaV);
            float outV = s * lvl * headroom_;
            lmix += outV * lV;
            rmix += outV * rV;
             v.amp *= v.ampMul; v.noise *= v.noiseMul; v.lifeSamples++;
             if ((v.amp < 1e-5f) || v.lifeSamples > v.maxSamples) v.active = false;
         }
         // pan & volume
         float theta = pan_ * 1.5707963f; float l = std::cos(theta), r = std::sin(theta);
         float outL = std::tanh(lmix * 1.1f) * volume_ * l;
         float outR = std::tanh(rmix * 1.1f) * volume_ * r;
         buffer[i].left += outL;
         buffer[i].right += outR;
     }
}
