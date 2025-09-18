#include "Sources/CEtherSynth/include/EtherSynthBridge.h"
#include "src/core/Types.h"
#include "src/synthesis/SynthEngine.h"

// All 15 engines now using unified SynthEngine interface
#include "src/engines/MacroVAEngine.h"
#include "src/engines/MacroFMEngine.h"
#include "src/engines/MacroWaveshaperEngine.h"
#include "src/engines/MacroWavetableEngine.h"
#include "src/engines/MacroChordEngine.h"
#include "src/engines/MacroHarmonicsEngine.h"
#include "src/engines/FormantEngine.h"
#include "src/engines/NoiseEngine.h"  
#include "src/engines/TidesOscEngine.h"
#include "src/engines/RingsVoiceEngine.h"
#include "src/engines/ElementsVoiceEngine.h"
#include "src/engines/SlideAccentBassEngine.h"
#include "src/engines/Classic4OpFMEngine.h"
#include "src/engines/SamplerSlicerEngine.h"
#include "src/engines/SerialHPLPEngine.h"
#include "src/engines/DrumKitEngine.h"
#include "src/engines/GranularEngine.h"

#include <iostream>
#include <map>
#include <string>
#include <cstring>
#include <memory>
#include <chrono>
#include <cmath>

#if defined(__APPLE__)
#include <mach/mach.h>
#endif

extern "C" {

// Harmonized bridge with expanded 16 instrument slots
static constexpr int SLOT_COUNT = 16;

struct Harmonized15EngineEtherSynthInstance {
    float bpm = 120.0f;
    float masterVolume = 0.8f;
    int activeInstrument = 0; // 0..15
    bool playing = false;
    bool recording = false;
    float cpuUsage = 15.0f;
    float cycles480_buf = 0.0f;
    float cycles480_samp = 0.0f;
    // Per-slot CPU estimates
    std::array<float, SLOT_COUNT> slotCpuPct{ };
    std::array<float, SLOT_COUNT> slotCyclesBuf{ };
    std::array<float, SLOT_COUNT> slotCyclesSamp{ };

    // Per-slot FX sends
    std::array<float, SLOT_COUNT> sendReverb{ };
    std::array<float, SLOT_COUNT> sendDelay{ };
    // Global FX params
    struct DelayFX { float timeMs = 350.0f; float feedback = 0.35f; float mix = 0.2f; } delayFX;
    struct ReverbFX { float time = 0.9f; float damp = 0.3f; float mix = 0.2f; } reverbFX;
    
    // Simple delay state
    struct SimpleDelay {
        std::vector<float> bufL, bufR; size_t idx = 0; float sr = 48000.0f;
        void setSR(float s){ sr = s; if (bufL.empty()) { bufL.resize((size_t)(sr*2),0.0f); bufR=bufL; }}
        void process(float* l, float* r, size_t n, float timeMs, float fb, float mix){
            size_t dl = std::min((size_t)bufL.size()-1, (size_t)std::max(1.0f, timeMs*sr*0.001f));
            for (size_t i=0;i<n;i++){
                float di = (float)bufL[(idx+bufL.size()-dl)%bufL.size()];
                float dj = (float)bufR[(idx+bufR.size()-dl)%bufR.size()];
                float inL = l[i], inR = r[i];
                float outL = inL + di * mix; float outR = inR + dj * mix;
                bufL[idx] = inL + di * fb; bufR[idx] = inR + dj * fb;
                l[i] = outL; r[i] = outR; idx++; if (idx>=bufL.size()) idx=0;
            }
        }
    } delayState;
    
    // Simple reverb stub (multi-tap diffuser)
    struct SimpleReverb {
        std::array<size_t,4> taps{ 149, 263, 457, 631 };
        std::vector<float> bufL, bufR; size_t idx=0; float sr=48000.0f;
        void setSR(float s){ sr=s; size_t N=(size_t)sr*2; bufL.assign(N,0.0f); bufR.assign(N,0.0f); }
        void process(float* l, float* r, size_t n, float time, float damp, float mix){
            size_t N=bufL.size(); if (N<8) return; float fb = std::clamp(time,0.1f,0.98f);
            for (size_t i=0;i<n;i++){
                float accL=0.0f, accR=0.0f;
                for (auto t: taps){ size_t p=(idx+N-t)%N; accL+=bufL[p]; accR+=bufR[p]; }
                accL /= taps.size(); accR /= taps.size();
                // damp
                accL = accL*(1.0f-damp); accR = accR*(1.0f-damp);
                float inL=l[i], inR=r[i];
                float fbL=inL + accL*fb; float fbR=inR + accR*fb;
                bufL[idx]=fbL; bufR[idx]=fbR;
                l[i]=inL + accL*mix; r[i]=inR + accR*mix; idx++; if (idx>=N) idx=0;
            }
        }
    } reverbState;
    int activeVoices = 0;
    
    // Real synthesis engines per slot
    std::array<std::unique_ptr<SynthEngine>, SLOT_COUNT> engines;
    
    // Engine type per slot
    std::array<EngineType, SLOT_COUNT> engineTypes;

    // Simple per-slot post processing (global controls apply to all engines)
    struct PostFilter {
        // One-pole HPF
        float hpfCut = 20.0f; // Hz
        float hpf_a = 0.0f, hpf_y1L = 0.0f, hpf_x1L = 0.0f, hpf_y1R = 0.0f, hpf_x1R = 0.0f;
        // Biquad LPF (RBJ)
        float lpfCut = 20000.0f; // Hz
        float lpfQ = 0.707f;
        float b0L=1, b1L=0, b2L=0, a1L=0, a2L=0, z1L=0, z2L=0;
        float b0R=1, b1R=0, b2R=0, a1R=0, a2R=0, z1R=0, z2R=0;
        float sampleRate = 48000.0f;
        // Pre/post controls
        float preGain = 1.0f;   // amplitude gain
        float drive = 0.0f;     // soft clip amount 0..1
        float pan = 0.0f;       // -1..1 equal-power
        
        void setHPF(float hz) {
            hpfCut = std::max(10.0f, std::min(hz, sampleRate * 0.45f));
            float rc = 1.0f / (2.0f * M_PI * hpfCut);
            float dt = 1.0f / sampleRate;
            hpf_a = rc / (rc + dt);
        }
        void setLPF(float hz, float q) {
            lpfCut = std::max(20.0f, std::min(hz, sampleRate * 0.45f));
            lpfQ = std::max(0.1f, q);
            float w0 = 2.0f * M_PI * (lpfCut / sampleRate);
            float alpha = std::sin(w0) / (2.0f * lpfQ);
            float cosw0 = std::cos(w0);
            float b0 = (1 - cosw0) * 0.5f;
            float b1 = 1 - cosw0;
            float b2 = (1 - cosw0) * 0.5f;
            float a0 = 1 + alpha;
            float a1 = -2 * cosw0;
            float a2 = 1 - alpha;
            // Normalize
            b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
            b0L=b0; b1L=b1; b2L=b2; a1L=a1; a2L=a2;
            b0R=b0; b1R=b1; b2R=b2; a1R=a1; a2R=a2;
        }
        void setSampleRate(float sr) {
            sampleRate = sr;
            setHPF(hpfCut);
            setLPF(lpfCut, lpfQ);
        }
        void setPreGain(float g) { preGain = std::max(0.0f, g); }
        void setDrive(float d) { drive = std::max(0.0f, std::min(d, 1.0f)); }
        void setPan(float p) { pan = std::max(-1.0f, std::min(p, 1.0f)); }
        inline void applyPan(float& l, float& r) {
            // Equal-power pan
            float t = (pan + 1.0f) * 0.25f * M_PI; // [-1,1] -> [0, pi/2]
            float gL = std::cos(t);
            float gR = std::sin(t);
            l *= gL; r *= gR;
        }
        inline float softClip(float x) const {
            if (drive <= 0.001f) return x;
            float k = 1.0f + drive * 5.0f; // up to ~tanh(6x)
            return std::tanh(k * x);
        }
        inline float procHPF(float x, float& y1, float& x1) {
            float y = hpf_a * (y1 + x - x1);
            y1 = y; x1 = x; return y;
        }
        inline float procLPFL(float x) {
            float y = b0L*x + z1L; z1L = b1L*x - a1L*y + z2L; z2L = b2L*x - a2L*y; return y;
        }
        inline float procLPFR(float x) {
            float y = b0R*x + z1R; z1R = b1R*x - a1R*y + z2R; z2R = b2R*x - a2R*y; return y;
        }
        inline void processFrame(float& l, float& r) {
            // Pre gain
            l *= preGain; r *= preGain;
            // Drive (soft clip)
            l = softClip(l); r = softClip(r);
            // Pan
            applyPan(l, r);
            l = procHPF(l, hpf_y1L, hpf_x1L);
            r = procHPF(r, hpf_y1R, hpf_x1R);
            l = procLPFL(l);
            r = procLPFR(r);
        }
    };
    std::array<PostFilter, SLOT_COUNT> postFX;

    // ===== Global LFO System (8 per slot, shared) =====
    static constexpr int MAX_LFOS = 8;
    struct LFO {
        int waveform = 0;     // 0=SINE, 1=TRI, 2=SAW_UP, 3=SAW_DOWN, 4=SQUARE, 5=PULSE, etc.
        float rateHz = 1.0f;  // 0.01..50 Hz
        float depth = 0.0f;   // 0..1 global depth
        int syncMode = 0;     // 0=FREE,1=TEMPO,2=KEY,3=ONESHOT,4=ENV
        float phase = 0.0f;   // 0..2π
        float lastValue = 0.0f; // -1..+1
        bool active = false;
    };
    // LFOs per slot
    std::array<std::array<LFO, MAX_LFOS>, SLOT_COUNT> slotLFOs{};

    // Per-parameter assignment: 8-bit mask + per-assignment depths
    static constexpr int PARAM_COUNT = static_cast<int>(ParameterID::COUNT);
    struct ParamLFOAssign {
        uint8_t mask = 0;                                  // bit i set => LFO i assigned
        std::array<float, MAX_LFOS> depths{};              // per-assignment depth 0..1
    };
    // For each slot: per-ParameterID mapping
    std::array<std::array<ParamLFOAssign, PARAM_COUNT>, SLOT_COUNT> lfoAssign{};
    
    Harmonized15EngineEtherSynthInstance() {
        // Initialize all slots with no engines (nullptr)
        for (size_t i = 0; i < engines.size(); i++) {
            engines[i] = nullptr;
            engineTypes[i] = EngineType::MACRO_VA; // Default type
        }
        for (auto& pf : postFX) { pf.setSampleRate(48000.0f); pf.setHPF(20.0f); pf.setLPF(20000.0f, 0.707f); }
        std::cout << "Harmonized 15-Engine Bridge: Created EtherSynth instance with 15 unified engines" << std::endl;
    }
    
    ~Harmonized15EngineEtherSynthInstance() {
        std::cout << "Harmonized 15-Engine Bridge: Destroyed EtherSynth instance" << std::endl;
    }
    
    // Create a real synthesis engine of the specified type (all using SynthEngine interface)
    std::unique_ptr<SynthEngine> createEngine(EngineType type) {
        switch (type) {
            case EngineType::MACRO_VA:
                return std::make_unique<MacroVAEngine>();
            case EngineType::MACRO_FM:
                return std::make_unique<MacroFMEngine>();
            case EngineType::MACRO_WAVESHAPER:
                return std::make_unique<MacroWaveshaperEngine>();
            case EngineType::MACRO_WAVETABLE:
                return std::make_unique<MacroWavetableEngine>();
            case EngineType::MACRO_CHORD:
                return std::make_unique<MacroChordEngine>();
            case EngineType::MACRO_HARMONICS:
                return std::make_unique<MacroHarmonicsEngine>();
            case EngineType::FORMANT_VOCAL:
                return std::make_unique<FormantEngine>();
            case EngineType::NOISE_PARTICLES:
                return std::make_unique<NoiseEngine>();
            case EngineType::TIDES_OSC:
                return std::make_unique<TidesOscEngine>();
            case EngineType::RINGS_VOICE:
                return std::make_unique<RingsVoiceEngine>();
            case EngineType::ELEMENTS_VOICE:
                return std::make_unique<ElementsVoiceEngine>();
            case EngineType::SLIDE_ACCENT_BASS:
                return std::make_unique<SlideAccentBassEngine>();
            case EngineType::CLASSIC_4OP_FM:
                return std::make_unique<Classic4OpFMEngine>();
            case EngineType::GRANULAR:
                return std::make_unique<GranularEngine>();
            case EngineType::SAMPLER_SLICER:
                return std::make_unique<SamplerSlicerEngine>();
            case EngineType::SERIAL_HPLP:
                return std::make_unique<SerialHPLPEngine>();
                
            // For engines that still have issues, fall back to working ones
            case EngineType::DRUM_KIT:
                return std::make_unique<DrumKitEngine>();
            case EngineType::SAMPLER_KIT:
                return std::make_unique<DrumKitEngine>();
                
            default:
                return std::make_unique<MacroVAEngine>(); // Safe fallback
        }
    }
    
    void setEngineType(int slot, EngineType type) {
        size_t index = static_cast<size_t>(slot);
        if (index >= engines.size()) return;
        
        // Destroy old engine
        engines[index] = nullptr;
        
        // Create new engine
        engines[index] = createEngine(type);
        engineTypes[index] = type;
        
        if (engines[index]) {
            engines[index]->setSampleRate(48000.0f);
            engines[index]->setBufferSize(128);
            postFX[index].setSampleRate(48000.0f);
            std::cout << "Harmonized 13-Engine Bridge: Created REAL " << getEngineTypeName(type) << " engine for slot " << index << std::endl;
        }
    }
    
    const char* getEngineTypeName(EngineType type) {
        switch (type) {
            case EngineType::MACRO_VA: return "MacroVA";
            case EngineType::MACRO_FM: return "MacroFM";
            case EngineType::MACRO_WAVESHAPER: return "MacroWaveshaper";
            case EngineType::MACRO_WAVETABLE: return "MacroWavetable";
            case EngineType::MACRO_CHORD: return "MacroChord";
            case EngineType::MACRO_HARMONICS: return "MacroHarmonics";
            case EngineType::FORMANT_VOCAL: return "FormantVocal";
            case EngineType::NOISE_PARTICLES: return "NoiseParticles";
            case EngineType::TIDES_OSC: return "TidesOsc";
            case EngineType::RINGS_VOICE: return "RingsVoice";
            case EngineType::ELEMENTS_VOICE: return "ElementsVoice";
            case EngineType::SLIDE_ACCENT_BASS: return "SlideAccentBass";
            case EngineType::CLASSIC_4OP_FM: return "Classic4OpFM";
            case EngineType::GRANULAR: return "Granular";
            case EngineType::DRUM_KIT: return "DrumKit(fallback)";
            case EngineType::SAMPLER_KIT: return "SamplerKit(fallback)";
            case EngineType::SAMPLER_SLICER: return "SamplerSlicer(fallback)";
            case EngineType::SERIAL_HPLP: return "SerialHPLP(fallback)";
            default: return "Unknown";
        }
    }
    const char* getEngineCategory(EngineType type) {
        switch (type) {
            case EngineType::MACRO_VA:
            case EngineType::MACRO_FM:
            case EngineType::MACRO_WAVESHAPER:
            case EngineType::MACRO_WAVETABLE:
            case EngineType::MACRO_HARMONICS:
                return "Synthesizers";
            case EngineType::MACRO_CHORD:
                return "Multi-Voice";
            case EngineType::FORMANT_VOCAL:
            case EngineType::NOISE_PARTICLES:
                return "Textures";
            case EngineType::TIDES_OSC:
            case EngineType::RINGS_VOICE:
            case EngineType::ELEMENTS_VOICE:
                return "Physical Models";
            case EngineType::DRUM_KIT:
                return "Drums";
            case EngineType::SAMPLER_KIT:
            case EngineType::SAMPLER_SLICER:
                return "Sampler";
            case EngineType::GRANULAR:
                return "Granular";
            case EngineType::SERIAL_HPLP:
                return "Filter";
            default:
                return "Other";
        }
    }
};

void* ether_create(void) {
    return new Harmonized15EngineEtherSynthInstance();
}

void ether_set_bpm(void* synth, float bpm) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    if (!instance) return;
    instance->bpm = std::max(20.0f, std::min(bpm, 300.0f));
}

float ether_get_bpm(void* synth) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    if (!instance) return 120.0f;
    return instance->bpm;
}

void ether_destroy(void* synth) {
    delete static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
}

int ether_initialize(void* synth) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    
    // Initialize with MacroVA engine on instrument 0
    instance->setEngineType(0, EngineType::MACRO_VA);
    
    instance->delayState.setSR(48000.0f);
    instance->reverbState.setSR(48000.0f);
    std::cout << "Harmonized 13-Engine Bridge: Initialized with unified synthesis engines" << std::endl;
    return 1;
}

void ether_process_audio(void* synth, float* outputBuffer, size_t bufferSize) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    auto t0 = std::chrono::high_resolution_clock::now();
    
    // Clear output buffer
    for (size_t i = 0; i < bufferSize * 2; i++) {
        outputBuffer[i] = 0.0f;
    }
    
    // Mix all engine slots
    EtherAudioBuffer temp;
    std::vector<float> sendL(bufferSize,0.0f), sendR(bufferSize,0.0f);
    double frameMs = (double)bufferSize / 48000.0 * 1000.0;
    for (size_t slot = 0; slot < instance->engines.size(); ++slot) {
        if (!instance->engines[slot]) continue;
        // --- LFO update & apply per-slot modulations (once per block)
        // Step LFOs and compute per-parameter combined value (snapshot)
        // Basic stepping: phase += 2π f / sr * bufferSize
        for (int i=0;i<instance->MAX_LFOS;i++) {
            auto &l = instance->slotLFOs[slot][i];
            float inc = 2.0f * (float)M_PI * std::max(0.01f, l.rateHz) / 48000.0f;
            l.phase += inc * (float)bufferSize; // block step
            // wrap
            if (l.phase > 2.0f * (float)M_PI) l.phase -= 2.0f * (float)M_PI;
            // simple waveforms
            float v = 0.0f;
            switch (l.waveform) {
                case 0: v = std::sinf(l.phase); break;                      // sine
                case 1: {                                                   // tri
                    float p = fmodf(l.phase/(2.0f*(float)M_PI),1.0f);
                    v = 4.0f * fabsf(p - 0.5f) - 1.0f;
                } break;
                case 4: v = (std::sinf(l.phase) >= 0.0f) ? 1.0f : -1.0f; break; // square
                default: v = std::sinf(l.phase); break;
            }
            l.lastValue = v * std::clamp(l.depth, 0.0f, 1.0f);
        }
        // For this slot, derive modulation deltas for post-chain fields
        float modHPF = 0.0f, modLPFCut = 0.0f, modLPFQ = 0.0f, modPreGain = 0.0f, modDrive = 0.0f, modPan = 0.0f;
        auto accumulate = [&](ParameterID pid, float &target){
            int pidIdx = static_cast<int>(pid);
            const auto &assign = instance->lfoAssign[slot][pidIdx];
            if (assign.mask==0) return;
            float sum = 0.0f;
            for (int i=0;i<instance->MAX_LFOS;i++) {
                if (assign.mask & (1u<<i)) {
                    sum += instance->slotLFOs[slot][i].lastValue * std::clamp(assign.depths[i], 0.0f, 1.0f);
                }
            }
            target += sum; // accumulate
        };
        accumulate(ParameterID::HPF, modHPF);
        accumulate(ParameterID::FILTER_CUTOFF, modLPFCut);
        accumulate(ParameterID::FILTER_RESONANCE, modLPFQ);
        accumulate(ParameterID::HARMONICS, modHPF);     // tilt contributes to HPF
        accumulate(ParameterID::TIMBRE, modLPFCut);     // brightness contributes to LPF cutoff
        accumulate(ParameterID::MORPH, modLPFQ);        // focus contributes to Q
        accumulate(ParameterID::AMPLITUDE, modPreGain);
        accumulate(ParameterID::VOLUME, modPreGain);    // mirror volume
        accumulate(ParameterID::CLIP, modDrive);
        accumulate(ParameterID::PAN, modPan);
        // Apply deltas with safe scales
        auto &pf = instance->postFX[slot];
        // hpf tilt: 10..600 Hz scaled by ~±20%
        float hpfBase = pf.hpfCut;
        float hpf = std::max(10.0f, hpfBase * (1.0f + 0.2f * modHPF));
        pf.setHPF(hpf);
        // lpf cutoff: scale exponentially by ± one octave per |mod|≈1
        float lpfBase = pf.lpfCut;
        float lpf = std::max(100.0f, lpfBase * std::pow(2.0f, 0.8f * modLPFCut));
        pf.setLPF(lpf, pf.lpfQ);
        // q: ±2.0 around base
        float qBase = pf.lpfQ;
        float q = std::clamp(qBase + 2.0f * modLPFQ, 0.5f, 10.0f);
        pf.setLPF(pf.lpfCut, q);
        // pre-gain: ±50%
        pf.setPreGain(std::max(0.1f, pf.preGain * (1.0f + 0.5f * modPreGain)));
        // drive: ±0.5
        pf.setDrive(std::clamp(pf.drive + 0.5f * modDrive, 0.0f, 1.0f));
        // pan: ±0.5
        pf.setPan(std::clamp(pf.pan + 0.5f * modPan, -1.0f, 1.0f));
        auto ts = std::chrono::high_resolution_clock::now();
        // Process into temp and accumulate
        for (auto &f : temp) { f.left = 0.0f; f.right = 0.0f; }
        instance->engines[slot]->processAudio(temp);
        // Apply per-slot post filters (pre-gain/drive/pan/HPF/LPF)
        for (size_t i = 0; i < bufferSize; ++i) {
            float l = temp[i].left;
            float r = temp[i].right;
            instance->postFX[slot].processFrame(l, r);
            temp[i].left = l; temp[i].right = r;
        }
        for (size_t i = 0; i < bufferSize; ++i) {
            outputBuffer[i*2]   += temp[i].left  * instance->masterVolume;
            outputBuffer[i*2+1] += temp[i].right * instance->masterVolume;
            // accumulate sends
            float sR = instance->sendReverb[slot]; float sD = instance->sendDelay[slot];
            if (sR>0.0001f || sD>0.0001f){
                sendL[i] += temp[i].left * (sR + sD);
                sendR[i] += temp[i].right* (sR + sD);
            }
        }
        auto te = std::chrono::high_resolution_clock::now();
        double msSlot = std::chrono::duration<double, std::milli>(te - ts).count();
        float pct = (float)std::clamp(msSlot / frameMs * 100.0, 0.0, 400.0);
        instance->slotCpuPct[slot] = 0.85f * instance->slotCpuPct[slot] + 0.15f * pct;
        double cyclesAvail = 480000000.0 * ((double)bufferSize / 48000.0);
        double cyc = (pct / 100.0) * cyclesAvail;
        instance->slotCyclesBuf[slot] = 0.85f * instance->slotCyclesBuf[slot] + 0.15f * (float)cyc;
        instance->slotCyclesSamp[slot] = (bufferSize>0) ? (instance->slotCyclesBuf[slot] / (float)bufferSize) : 0.0f;
    }
    // Process FX returns
    instance->delayState.process(sendL.data(), sendR.data(), bufferSize, instance->delayFX.timeMs, instance->delayFX.feedback, instance->delayFX.mix);
    instance->reverbState.process(sendL.data(), sendR.data(), bufferSize, instance->reverbFX.time, instance->reverbFX.damp, instance->reverbFX.mix);
    for (size_t i=0;i<bufferSize;i++){ outputBuffer[i*2]+=sendL[i]; outputBuffer[i*2+1]+=sendR[i]; }
    // Gentle soft clip on mixed output
    auto softclip = [](float x) {
        const float drive = 1.5f;
        return std::tanh(x * drive);
    };
    for (size_t i = 0; i < bufferSize; ++i) {
        outputBuffer[i*2]   = softclip(outputBuffer[i*2]);
        outputBuffer[i*2+1] = softclip(outputBuffer[i*2+1]);
    }
    
    // CPU usage estimation: processing time vs buffer duration
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    double frameMs2 = (double)bufferSize / 48000.0 * 1000.0; // sampleRate hardcoded 48k
    float inst = (float)std::clamp(ms / frameMs2 * 100.0, 0.0, 400.0);
    // Exponential moving average for stability
    instance->cpuUsage = 0.85f * instance->cpuUsage + 0.15f * inst;
    // Estimate cycles/buffer @480 MHz
    double cyclesAvail = 480000000.0 * ((double)bufferSize / 48000.0);
    double cyc = (inst / 100.0) * cyclesAvail;
    instance->cycles480_buf = 0.85f * instance->cycles480_buf + 0.15f * (float)cyc;
    instance->cycles480_samp = (bufferSize > 0) ? (instance->cycles480_buf / (float)bufferSize) : 0.0f;
}

void ether_play(void* synth) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    instance->playing = true;
    std::cout << "Harmonized 13-Engine Bridge: Play" << std::endl;
}

void ether_stop(void* synth) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    instance->playing = false;
    std::cout << "Harmonized 13-Engine Bridge: Stop" << std::endl;
}

void ether_note_on(void* synth, int key_index, float velocity, float aftertouch) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    size_t activeIndex = static_cast<size_t>(instance->activeInstrument);
    
    if (activeIndex < instance->engines.size() && instance->engines[activeIndex]) {
        instance->engines[activeIndex]->noteOn(key_index, velocity, aftertouch);
        instance->activeVoices++;
    }
}

void ether_note_off(void* synth, int key_index) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    size_t activeIndex = static_cast<size_t>(instance->activeInstrument);
    
    if (activeIndex < instance->engines.size() && instance->engines[activeIndex]) {
        instance->engines[activeIndex]->noteOff(key_index);
        if (instance->activeVoices > 0) {
            instance->activeVoices--;
        }
    }
}

void ether_all_notes_off(void* synth) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    size_t activeIndex = static_cast<size_t>(instance->activeInstrument);
    
    if (activeIndex < instance->engines.size() && instance->engines[activeIndex]) {
        instance->engines[activeIndex]->allNotesOff();
    }
    instance->activeVoices = 0;
}

void ether_set_instrument_engine_type(void* synth, int instrument, int engine_type) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    
    if (engine_type >= 0 && engine_type < static_cast<int>(EngineType::COUNT)) {
        EngineType type = static_cast<EngineType>(engine_type);
        instance->setEngineType(instrument, type);
        std::cout << "Harmonized 13-Engine Bridge: Set slot " << instrument << " to REAL engine type " << engine_type << std::endl;
    }
}

int ether_get_instrument_engine_type(void* synth, int instrument) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    size_t index = static_cast<size_t>(instrument);
    
    if (index < instance->engineTypes.size()) {
        return static_cast<int>(instance->engineTypes[index]);
    }
    return 0;
}

const char* ether_get_engine_type_name(int engine_type) {
    if (engine_type >= 0 && engine_type < static_cast<int>(EngineType::COUNT)) {
        Harmonized15EngineEtherSynthInstance dummy;
        return dummy.getEngineTypeName(static_cast<EngineType>(engine_type));
    }
    return "Unknown";
}

int ether_get_engine_type_count() {
    return static_cast<int>(EngineType::COUNT);
}

void ether_set_active_instrument(void* synth, int color_index) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    if (color_index >= 0 && color_index < SLOT_COUNT) {
        instance->activeInstrument = color_index;
    }
}

int ether_get_active_instrument(void* synth) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    return instance->activeInstrument;
}

int ether_get_active_voice_count(void* synth) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    return instance->activeVoices;
}

float ether_get_cpu_usage(void* synth) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    return instance->cpuUsage;
}

float ether_get_cycles_480_per_buffer(void* synth) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    return instance->cycles480_buf;
}

float ether_get_cycles_480_per_sample(void* synth) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    return instance->cycles480_samp;
}

// Get resident set size in kilobytes (platform specific)
static inline float get_rss_kb() {
#if defined(__APPLE__)
    mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS) {
        return (float)info.resident_size / 1024.0f;
    }
    return 0.0f;
#else
    return 0.0f;
#endif
}

float ether_get_memory_usage_kb(void* /*synth*/) {
    return get_rss_kb();
}

void ether_set_master_volume(void* synth, float volume) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    instance->masterVolume = volume;
}

float ether_get_master_volume(void* synth) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    return instance->masterVolume;
}

void ether_set_instrument_parameter(void* synth, int instrument, int param_id, float value) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    size_t idx = static_cast<size_t>(instrument);
    
    if (idx < instance->engines.size() && instance->engines[idx]) {
        // Convert int parameter ID to ParameterID enum
        if (param_id >= 0 && param_id < static_cast<int>(ParameterID::COUNT)) {
            ParameterID paramEnum = static_cast<ParameterID>(param_id);
            instance->engines[idx]->setParameter(paramEnum, value);
            // Also drive per-slot post filters for universal LPF/HPF
            switch (paramEnum) {
                case ParameterID::HPF: {
                    // Map 0..1 to 20..200 Hz
                    float hz = 20.0f + std::clamp(value, 0.0f, 1.0f) * 180.0f;
                    instance->postFX[idx].setHPF(hz);
                } break;
                case ParameterID::FILTER_CUTOFF: {
                    // Map 0..1 exponentially 100Hz..18kHz for global LPF
                    float norm = std::clamp(value, 0.0f, 1.0f);
                    float hz = 100.0f * std::pow(2.0f, norm * 7.5f); // ~100..18100 Hz
                    instance->postFX[idx].setLPF(hz, instance->postFX[idx].lpfQ);
                } break;
                case ParameterID::FILTER_RESONANCE: {
                    float q = 0.5f + std::clamp(value, 0.0f, 1.0f) * 9.5f;
                    instance->postFX[idx].setLPF(instance->postFX[idx].lpfCut, q);
                } break;
                case ParameterID::VOLUME: {
                    // Also scale post pre-gain so engines lacking VOLUME respond
                    float amp = std::clamp(value, 0.0f, 1.0f);
                    instance->postFX[idx].setPreGain(amp * 2.0f);
                } break;
                case ParameterID::PAN: {
                    instance->postFX[idx].setPan(std::clamp(value, -1.0f, 1.0f));
                } break;
                case ParameterID::AMPLITUDE: {
                    float amp = std::clamp(value, 0.0f, 1.0f);
                    instance->postFX[idx].setPreGain(amp * 2.0f); // up to +6 dB
                } break;
                case ParameterID::CLIP: {
                    instance->postFX[idx].setDrive(std::clamp(value, 0.0f, 1.0f));
                } break;
                case ParameterID::HARMONICS: {
                    // Map to HPF tilt: 10..600 Hz
                    float hz = 10.0f + std::clamp(value, 0.0f, 1.0f) * 590.0f;
                    instance->postFX[idx].setHPF(hz);
                } break;
                case ParameterID::TIMBRE: {
                    // Map to LPF cutoff 300..18kHz
                    float hz = 300.0f * std::pow(2.0f, std::clamp(value, 0.0f, 1.0f) * 6.5f);
                    instance->postFX[idx].setLPF(hz, instance->postFX[idx].lpfQ);
                } break;
                case ParameterID::MORPH: {
                    // Map to LPF Q 0.5..10 for tone emphasis
                    float q = 0.5f + std::clamp(value, 0.0f, 1.0f) * 9.5f;
                    instance->postFX[idx].setLPF(instance->postFX[idx].lpfCut, q);
                } break;
                default: break;
            }
        }
    }
}

float ether_get_instrument_parameter(void* synth, int instrument, int param_id) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    size_t idx = static_cast<size_t>(instrument);
    
    if (idx < instance->engines.size() && instance->engines[idx]) {
        // Convert int parameter ID to ParameterID enum
        if (param_id >= 0 && param_id < static_cast<int>(ParameterID::COUNT)) {
            ParameterID paramEnum = static_cast<ParameterID>(param_id);
            return instance->engines[idx]->getParameter(paramEnum);
        }
    }
    return 0.0f;
}

void ether_shutdown(void* synth) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    
    // All engines now use SynthEngine interface, no shutdown() method needed
    for (auto& engine : instance->engines) {
        engine = nullptr; // Destructor handles cleanup
    }
    
    std::cout << "Harmonized 15-Engine Bridge: Shutdown" << std::endl;
}

extern "C" {
// which: 0=decay,1=tune,2=level,3=pan
void ether_drum_set_param(void* synth, int instrument, int pad, int which, float value) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    size_t index = static_cast<size_t>(instrument);
    if (index < instance->engines.size() && instance->engines[index]) {
        auto* dk = dynamic_cast<DrumKitEngine*>(instance->engines[index].get());
        if (!dk) return;
        // Treat value as delta for convenience
        switch (which) {
            case 0: dk->setPadDecay(pad, dk->getPadDecay(pad) + value); break;
            case 1: dk->setPadTune(pad,  dk->getPadTune(pad)  + value); break;
            case 2: dk->setPadLevel(pad, dk->getPadLevel(pad) + value); break;
            case 3: dk->setPadPan(pad,   dk->getPadPan(pad)   + value); break;
        }
    }
}

// Expose whether an engine claims it handles a parameter (UI can choose to hide)
bool ether_engine_has_parameter(void* synth, int instrument, int param_id) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    size_t idx = static_cast<size_t>(instrument);
    if (idx < instance->engines.size() && instance->engines[idx]) {
        if (param_id >= 0 && param_id < static_cast<int>(ParameterID::COUNT)) {
            ParameterID pid = static_cast<ParameterID>(param_id);
            // Universal post filters mean HPF/LPF/RES always take effect
            if (pid == ParameterID::HPF || pid == ParameterID::FILTER_CUTOFF || pid == ParameterID::FILTER_RESONANCE) return true;
            // Provide a consistent core footprint across engines so UI can expose common controls
            switch (pid) {
                case ParameterID::HARMONICS:
                case ParameterID::TIMBRE:
                case ParameterID::MORPH:
                case ParameterID::OSC_MIX:
                case ParameterID::DETUNE:
                case ParameterID::SUB_LEVEL:
                case ParameterID::ATTACK:
                case ParameterID::DECAY:
                case ParameterID::SUSTAIN:
                case ParameterID::RELEASE:
                case ParameterID::VOLUME:
                case ParameterID::PAN:
                case ParameterID::AMPLITUDE:
                case ParameterID::CLIP:
                    return true; // Treat as supported; engines that ignore will safely no-op
                default: break;
            }
            return instance->engines[idx]->hasParameter(pid);
        }
    }
    return false;
}
}

// FX sends
void ether_set_engine_fx_send(void* synth, int instrument, int which, float value) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    size_t idx = static_cast<size_t>(instrument);
    value = std::clamp(value, 0.0f, 1.0f);
    if (idx < instance->sendReverb.size()) {
        if (which == 0) instance->sendReverb[idx] = value;
        else if (which == 1) instance->sendDelay[idx] = value;
    }
}

// ===== Global LFO C API (8 global LFOs per instrument slot) =====

// Bridge compatibility: match minimal C header (no instrument param)
void ether_set_lfo_waveform(void* synth, unsigned char lfo_id, unsigned char waveform) {
    auto* in = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    if (!in) return;
    int instrument = in->activeInstrument;
    int lfoIndex = static_cast<int>(lfo_id);
    if (instrument < 0 || instrument >= (int)in->engines.size()) return;
    if (lfoIndex < 0 || lfoIndex >= Harmonized15EngineEtherSynthInstance::MAX_LFOS) return;
    in->slotLFOs[instrument][lfoIndex].waveform = (int)waveform;
    in->slotLFOs[instrument][lfoIndex].active = true;
}

void ether_set_lfo_rate(void* synth, unsigned char lfo_id, float rate) {
    auto* in = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    if (!in) return;
    int instrument = in->activeInstrument;
    int lfoIndex = static_cast<int>(lfo_id);
    if (instrument < 0 || instrument >= (int)in->engines.size()) return;
    if (lfoIndex < 0 || lfoIndex >= Harmonized15EngineEtherSynthInstance::MAX_LFOS) return;
    in->slotLFOs[instrument][lfoIndex].rateHz = std::clamp(rate, 0.01f, 50.0f);
    in->slotLFOs[instrument][lfoIndex].active = true;
}

void ether_set_lfo_depth(void* synth, unsigned char lfo_id, float depth) {
    auto* in = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    if (!in) return;
    int instrument = in->activeInstrument;
    int lfoIndex = static_cast<int>(lfo_id);
    if (instrument < 0 || instrument >= (int)in->engines.size()) return;
    if (lfoIndex < 0 || lfoIndex >= Harmonized15EngineEtherSynthInstance::MAX_LFOS) return;
    in->slotLFOs[instrument][lfoIndex].depth = std::clamp(depth, 0.0f, 1.0f);
    in->slotLFOs[instrument][lfoIndex].active = true;
}

void ether_set_lfo_sync(void* synth, int instrument, int lfoIndex, int syncMode) {
    auto* in = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    if (!in) return;
    if (instrument < 0 || instrument >= (int)in->engines.size()) return;
    if (lfoIndex < 0 || lfoIndex >= Harmonized15EngineEtherSynthInstance::MAX_LFOS) return;
    in->slotLFOs[instrument][lfoIndex].syncMode = std::clamp(syncMode, 0, 4);
}

void ether_assign_lfo_to_param_id(void* synth, int instrument, int lfoIndex, int paramId, float depth) {
    auto* in = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    if (!in) return;
    if (instrument < 0 || instrument >= (int)in->engines.size()) return;
    if (lfoIndex < 0 || lfoIndex >= Harmonized15EngineEtherSynthInstance::MAX_LFOS) return;
    if (paramId < 0 || paramId >= (int)Harmonized15EngineEtherSynthInstance::PARAM_COUNT) return;
    auto &assign = in->lfoAssign[instrument][paramId];
    assign.mask |= (1u << lfoIndex);
    assign.depths[lfoIndex] = std::clamp(depth, 0.0f, 1.0f);
    in->slotLFOs[instrument][lfoIndex].active = true;
}

void ether_remove_lfo_assignment_by_param(void* synth, int instrument, int lfoIndex, int paramId) {
    auto* in = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    if (!in) return;
    if (instrument < 0 || instrument >= (int)in->engines.size()) return;
    if (lfoIndex < 0 || lfoIndex >= Harmonized15EngineEtherSynthInstance::MAX_LFOS) return;
    if (paramId < 0 || paramId >= (int)Harmonized15EngineEtherSynthInstance::PARAM_COUNT) return;
    auto &assign = in->lfoAssign[instrument][paramId];
    assign.mask &= ~(1u << lfoIndex);
    assign.depths[lfoIndex] = 0.0f;
}

int ether_get_parameter_lfo_info(void* synth, int instrument, int keyIndex, int* activeLFOs, float* currentValue) {
    auto* in = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    if (!in || !activeLFOs || !currentValue) return 0;
    if (instrument < 0 || instrument >= (int)in->engines.size()) return 0;
    if (keyIndex < 0 || keyIndex >= (int)Harmonized15EngineEtherSynthInstance::PARAM_COUNT) return 0;
    const auto &assign = in->lfoAssign[instrument][keyIndex];
    *activeLFOs = (int)assign.mask;
    float sum = 0.0f;
    for (int i=0;i<Harmonized15EngineEtherSynthInstance::MAX_LFOS;i++) {
        if (assign.mask & (1u<<i)) {
            sum += in->slotLFOs[instrument][i].lastValue * std::clamp(assign.depths[i], 0.0f, 1.0f);
        }
    }
    *currentValue = sum; // -? .. +?
    return assign.mask ? 1 : 0;
}

void ether_trigger_instrument_lfos(void* synth, int instrument) {
    auto* in = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    if (!in) return;
    if (instrument < 0 || instrument >= (int)in->engines.size()) return;
    for (int i=0;i<Harmonized15EngineEtherSynthInstance::MAX_LFOS;i++) {
        auto &l = in->slotLFOs[instrument][i];
        if (l.syncMode==2 /*KEY*/ || l.syncMode==3 /*ONESHOT*/ || l.syncMode==4 /*ENV*/) {
            l.phase = 0.0f;
        }
    }
}
float ether_get_engine_fx_send(void* synth, int instrument, int which) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    size_t idx = static_cast<size_t>(instrument);
    if (idx < instance->sendReverb.size()) {
        if (which == 0) return instance->sendReverb[idx];
        if (which == 1) return instance->sendDelay[idx];
    }
    return 0.0f;
}
void ether_set_fx_global(void* synth, int which, int param, float value) {
    auto* in = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    value = std::clamp(value, 0.0f, 1.0f);
    if (which==0){ // reverb
        if (param==0) in->reverbFX.time = 0.2f + value*0.8f; // decay
        else if (param==1) in->reverbFX.damp = value; 
        else if (param==2) in->reverbFX.mix = value*0.5f; 
    } else if (which==1) { // delay
        if (param==0) in->delayFX.timeMs = 40.0f + value*960.0f;
        else if (param==1) in->delayFX.feedback = value*0.9f;
        else if (param==2) in->delayFX.mix = value*0.5f;
    }
}
float ether_get_fx_global(void* synth, int which, int param) {
    auto* in = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    if (which==0){ // reverb
        if (param==0) return std::clamp((in->reverbFX.time - 0.2f)/0.8f, 0.0f, 1.0f);
        if (param==1) return in->reverbFX.damp;
        if (param==2) return std::clamp(in->reverbFX.mix/0.5f, 0.0f, 1.0f);
    } else if (which==1) { // delay
        if (param==0) return std::clamp((in->delayFX.timeMs - 40.0f)/960.0f, 0.0f, 1.0f);
        if (param==1) return std::clamp(in->delayFX.feedback/0.9f, 0.0f, 1.0f);
        if (param==2) return std::clamp(in->delayFX.mix/0.5f, 0.0f, 1.0f);
    }
    return 0.0f;
}
float ether_get_engine_cpu_pct(void* synth, int instrument){
    auto* in = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    size_t idx = static_cast<size_t>(instrument);
    if (idx<in->slotCpuPct.size()) return in->slotCpuPct[idx];
    return 0.0f;
}
float ether_get_engine_cycles_480_buf(void* synth, int instrument){ auto* in = static_cast<Harmonized15EngineEtherSynthInstance*>(synth); size_t idx=(size_t)instrument; if (idx<in->slotCyclesBuf.size()) return in->slotCyclesBuf[idx]; return 0.0f; }
float ether_get_engine_cycles_480_smp(void* synth, int instrument){ auto* in = static_cast<Harmonized15EngineEtherSynthInstance*>(synth); size_t idx=(size_t)instrument; if (idx<in->slotCyclesSamp.size()) return in->slotCyclesSamp[idx]; return 0.0f; }
void ether_set_engine_voice_count(void* synth, int instrument, int voices) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    size_t index = static_cast<size_t>(instrument);
    if (index < instance->engines.size() && instance->engines[index]) {
        instance->engines[index]->setVoiceCount(std::max(1, std::min(voices, (int)MAX_VOICES)));
    }
}

int ether_get_engine_voice_count(void* synth, int instrument) {
    auto* instance = static_cast<Harmonized15EngineEtherSynthInstance*>(synth);
    size_t index = static_cast<size_t>(instrument);
    if (index < instance->engines.size() && instance->engines[index]) {
        return (int)instance->engines[index]->getMaxVoiceCount();
    }
    return (int)MAX_VOICES;
}

} // extern "C"
const char* ether_get_engine_category_name(int engine_type) {
    Harmonized15EngineEtherSynthInstance dummy;
    if (engine_type < 0 || engine_type >= static_cast<int>(EngineType::COUNT)) return "Other";
    return dummy.getEngineCategory(static_cast<EngineType>(engine_type));
}
