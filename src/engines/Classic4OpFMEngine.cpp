#include "Classic4OpFMEngine.h"
#include <cmath>

Classic4OpFMEngine::Classic4OpFMEngine() 
    : sampleRate_(44100.0f), initialized_(false),
      harmonics_(0.5f), timbre_(0.5f), morph_(0.5f), cpuUsage_(0.0f) {
    voices_.resize(maxVoices_);
}

Classic4OpFMEngine::~Classic4OpFMEngine() {
    shutdown();
}

bool Classic4OpFMEngine::initialize(float sampleRate) {
    if (initialized_) {
        return true;
    }
    
    sampleRate_ = sampleRate;
    initialized_ = true;
    for (auto& v : voices_) { v.active = false; v.lpfState = 0.0f; v.lastOp1 = 0.0f; v.age = 0; }
    return true;
}

void Classic4OpFMEngine::shutdown() {
    if (!initialized_) {
        return;
    }
    
    allNotesOff();
    initialized_ = false;
}

void Classic4OpFMEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    Voice* v = findFreeVoice();
    if (!v) v = stealVoice();
    if (!v) return;
    v->active = true;
    v->note = note;
    v->velocity = std::clamp(velocity, 0.0f, 1.0f);
    v->freq = 440.0f * std::pow(2.0f, (static_cast<int>(note) - 69) / 12.0f);
    v->p1 = v->p2 = v->p3 = v->p4 = 0.0f;
    v->lpfState = 0.0f; v->lastOp1 = 0.0f; v->age = 0;
    v->stage = Voice::EnvStage::ATTACK;
}

void Classic4OpFMEngine::noteOff(uint8_t note) {
    for (auto& v : voices_) if (v.active && v.note == note) v.stage = Voice::EnvStage::RELEASE;
}

void Classic4OpFMEngine::setAftertouch(uint8_t note, float aftertouch) {
    // Not supported
}

void Classic4OpFMEngine::allNotesOff() {
    for (auto& v : voices_) { v.active = false; v.stage = Voice::EnvStage::IDLE; }
}

void Classic4OpFMEngine::setParameter(ParameterID param, float value) {
    switch (param) {
        case ParameterID::HARMONICS:
            harmonics_ = value;
            break;
        case ParameterID::TIMBRE:
            timbre_ = value;
            break;
        case ParameterID::MORPH:
            morph_ = value;
            break;
        case ParameterID::ATTACK:
            envAttack_ = 0.001f + value * 0.2f;
            break;
        case ParameterID::DECAY:
            envDecay_ = 0.01f + value * 0.4f;
            break;
        case ParameterID::SUSTAIN:
            envSustain_ = value;
            break;
        case ParameterID::RELEASE:
            envRelease_ = 0.01f + value * 0.4f;
            break;
        case ParameterID::VOLUME:
            volume_ = value;
            break;
        case ParameterID::PAN:
            pan_ = value;
            break;
        case ParameterID::FILTER_CUTOFF:
            // Use FILTER_CUTOFF as brightness macro (0..1)
            brightness_ = value;
            break;
        default:
            break;
    }
}

float Classic4OpFMEngine::getParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::HARMONICS:
            return harmonics_;
        case ParameterID::TIMBRE:
            return timbre_;
        case ParameterID::MORPH:
            return morph_;
        case ParameterID::ATTACK: return envAttack_;
        case ParameterID::DECAY: return envDecay_;
        case ParameterID::SUSTAIN: return envSustain_;
        case ParameterID::RELEASE: return envRelease_;
        case ParameterID::VOLUME: return volume_;
        case ParameterID::PAN: return pan_;
        default:
            return 0.0f;
    }
}

bool Classic4OpFMEngine::hasParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::HARMONICS:
        case ParameterID::TIMBRE:
        case ParameterID::MORPH:
        case ParameterID::ATTACK:
        case ParameterID::DECAY:
        case ParameterID::SUSTAIN:
        case ParameterID::RELEASE:
        case ParameterID::VOLUME:
        case ParameterID::PAN:
            return true;
        default:
            return false;
    }
}

void Classic4OpFMEngine::processAudio(EtherAudioBuffer& outputBuffer) {
    if (!initialized_) { outputBuffer.fill(AudioFrame(0.0f, 0.0f)); return; }
    for (auto& f : outputBuffer) { f.left = 0.0f; f.right = 0.0f; }
    for (size_t i = 0; i < BUFFER_SIZE; ++i) {
        float sum = 0.0f;
        for (auto& v : voices_) { if (v.active) { v.age++; sum += processVoiceSample(v); } }
        float theta = pan_ * 1.5707963f; float l = std::cos(theta), r = std::sin(theta);
        outputBuffer[i].left  += sum * l;
        outputBuffer[i].right += sum * r;
    }
}

size_t Classic4OpFMEngine::getActiveVoiceCount() const { size_t n=0; for (auto& v: voices_) if (v.active) n++; return n; }
void Classic4OpFMEngine::setVoiceCount(size_t maxVoices) { if (maxVoices<1) maxVoices=1; if (maxVoices>16) maxVoices=16; maxVoices_=maxVoices; voices_.assign(maxVoices_, Voice{}); }

void Classic4OpFMEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
    actualSize = 0;
    if (maxSize < sizeof(float) * 3) {
        return;
    }
    
    float* floatData = reinterpret_cast<float*>(data);
    floatData[0] = harmonics_;
    floatData[1] = timbre_;
    floatData[2] = morph_;
    actualSize = sizeof(float) * 3;
}

bool Classic4OpFMEngine::loadPreset(const uint8_t* data, size_t size) {
    if (size < sizeof(float) * 3) {
        return false;
    }
    
    const float* floatData = reinterpret_cast<const float*>(data);
    harmonics_ = floatData[0];
    timbre_ = floatData[1];
    morph_ = floatData[2];
    return true;
}

void Classic4OpFMEngine::setSampleRate(float sampleRate) {
    if (sampleRate_ != sampleRate) {
        shutdown();
        initialize(sampleRate);
    }
}

void Classic4OpFMEngine::setBufferSize(size_t bufferSize) {
    // Buffer size changes handled automatically
}

float Classic4OpFMEngine::getCPUUsage() const { return cpuUsage_; }

float Classic4OpFMEngine::processVoiceSample(Voice& v) {
    if (!initialized_) return 0.0f;
    advanceEnvelope(v);
    if (v.stage == Voice::EnvStage::IDLE) { v.active = false; return 0.0f; }
    float baseFreq = v.freq;

    // Limit FM indices to musical ranges
    float idx = 0.2f + harmonics_ * 1.2f;  // gentler index 0.2..1.4
    // Map FILTER_RESONANCE (brightness_) to a subtle extra index lift
    idx *= (0.8f + 0.4f * brightness_);
    float fb = morph_ * 0.05f;       // tiny feedback for DX-style stability

    // Operator ratios influenced by timbre: 1,2,3,4 .. 1,2,2,3 etc.
    int algo = static_cast<int>(timbre_ * 7.99f); // 0..7
    float r1=1.0f, r2=2.0f, r3=3.0f, r4=4.0f;
    switch (algo) {
        case 0: r1=1; r2=2; r3=3; r4=4; break;            // classic stack
        case 1: r1=1; r2=2; r3=2; r4=3; break;            // tighter
        case 2: r1=1; r2=3; r3=2; r4=5; break;            // brighter
        case 3: r1=1; r2=1.5f; r3=2; r4=3; break;         // mellow
        case 4: r1=1; r2=2; r3=1; r4=2; break;            // feedback-friendly
        case 5: r1=0.5f; r2=1; r3=2; r4=3; break;         // sub richness
        case 6: r1=1; r2=2.5f; r3=3.5f; r4=5.0f; break;   // clang
        case 7: r1=1; r2=1; r3=1; r4=1; break;            // organ-ish
    }

    // Operator 4 (top of stack)
    float freq4 = baseFreq * r4;
    v.p4 += freq4 / sampleRate_; if (v.p4 >= 1.0f) v.p4 -= 1.0f; float op4 = std::sin(v.p4 * 2.0f * M_PI);
    
    // Operator 3 (modulated by op4)
    float freq3 = baseFreq * r3;
    v.p3 += freq3 / sampleRate_; if (v.p3 >= 1.0f) v.p3 -= 1.0f; float op3 = std::sin((v.p3 + op4 * idx) * 2.0f * M_PI);
    
    // Operator 2 (modulated by op3)
    float freq2 = baseFreq * r2;
    v.p2 += freq2 / sampleRate_; if (v.p2 >= 1.0f) v.p2 -= 1.0f; float op2 = std::sin((v.p2 + op3 * idx) * 2.0f * M_PI);
    
    // Operator 1 (carrier, modulated by op2, with feedback)
    float freq1 = baseFreq * r1;
    v.p1 += freq1 / sampleRate_; if (v.p1 >= 1.0f) v.p1 -= 1.0f; float op1 = std::sin((v.p1 + op2 * idx + v.lastOp1 * fb) * 2.0f * M_PI); v.lastOp1 = op1;
    
    // Post brightness: simple one-pole LPF (darken when brightness low)
    float cutoff = 0.05f + brightness_ * 0.45f; // normalized
    v.lpfState += cutoff * (op1 - v.lpfState);
    float s = 0.85f * v.lpfState + 0.15f * std::sin(v.p1 * 2.0f * M_PI);
    // Simple amplitude: velocity and envelope, with headroom
    float amp = 0.8f * v.velocity * v.env * volume_;
    return s * amp;
}

void Classic4OpFMEngine::advanceEnvelope(Voice& v) {
    float aRate = 1.0f / (envAttack_ * sampleRate_);
    float dRate = 1.0f / (envDecay_ * sampleRate_);
    float rRate = 1.0f / (envRelease_ * sampleRate_);
    switch (v.stage) {
        case Voice::EnvStage::IDLE: v.env = 0.0f; break;
        case Voice::EnvStage::ATTACK:
            v.env += aRate;
            if (v.env >= 1.0f) { v.env = 1.0f; v.stage = Voice::EnvStage::DECAY; }
            break;
        case Voice::EnvStage::DECAY:
            v.env -= dRate;
            if (v.env <= envSustain_) { v.env = envSustain_; v.stage = Voice::EnvStage::SUSTAIN; }
            break;
        case Voice::EnvStage::SUSTAIN:
            break;
        case Voice::EnvStage::RELEASE:
            v.env -= rRate;
            if (v.env <= 0.0f) { v.env = 0.0f; v.stage = Voice::EnvStage::IDLE; }
            break;
    }
}

Classic4OpFMEngine::Voice* Classic4OpFMEngine::findFreeVoice() { for (auto& v:voices_) if (!v.active || v.stage==Voice::EnvStage::IDLE) return &v; return nullptr; }
Classic4OpFMEngine::Voice* Classic4OpFMEngine::stealVoice() { Voice* oldest=nullptr; uint32_t best=0; for (auto& v:voices_) if (v.active && v.age>=best){best=v.age; oldest=&v;} if (oldest){ *oldest = Voice{}; return oldest;} return nullptr; }
