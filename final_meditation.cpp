/* meditatron_v2_improved.cpp  (g++ -std=c++17 -O3 -lportaudio meditatron_v2_improved.cpp -o meditatron)  */

#include <portaudio.h>
#include <cmath>
#include <vector>
#include <random>
#include <iostream>
#include <thread>
#include <atomic>
#include <algorithm>
#include <string>
#include <cctype>

/* ---------- constants ---------- */
const int SAMPLE_RATE       = 44100;
const int FRAMES_PER_BUFFER = 512;
const int WAVETABLE_SIZE    = 2048;

/* ---------- RNG ---------- */
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> uniform(0.0f, 1.0f);

/* ---------- global controls ---------- */
std::atomic<float> masterVolume   (0.30f);
std::atomic<float> loopLength     (4.0f);
std::atomic<bool>  running        (true);
std::atomic<bool>  melodyEnabled  (false);
std::atomic<bool>  percussionEnabled(false);

/* ---------- ESP32 CONTROL MACROS ---------- */
std::atomic<float> gVol  (0.5f);    // v: Volume (0-1)
std::atomic<float> gTex  (0.0f);    // t: Texture (0=drones, 1=loops)
std::atomic<float> gMot  (0.5f);    // m: Motion (chorus/LFO depth)
std::atomic<float> gEvo  (0.5f);    // e: Evolve speed (0.2x to 3x)
std::atomic<float> gCmpx (0.5f);    // c: Complexity (melody/percussion)
std::atomic<int>   gVMax (3);       // n: Voice limit (1-5)
std::atomic<float> gTilt (0.5f);    // w: Warm/Bright tilt EQ
std::atomic<float> gSpc  (0.5f);    // s: Space (reverb wet + shimmer)
std::atomic<float> gLpf  (1.0f);    // l: Lowpass filter (was delay)
std::atomic<float> gBias (0.5f);    // b: Pitch bias (-1 to +1 octave)

// NEW A/B testing controls
std::atomic<bool>  gOldShimmer (false);  // f: toggle old/new shimmer
std::atomic<bool>  gOldChorus  (false);  // g: toggle old/new chorus

/* ---------- Smoothed parameter values ---------- */
struct SmoothedParam {
    float current = 0.0f;
    float target = 0.0f;
    float rate = 0.001f;
    
    void setTarget(float newTarget) { target = newTarget; }
    float update() {
        current += (target - current) * rate;
        return current;
    }
    void setImmediate(float val) { current = target = val; }
};

SmoothedParam smoothTex, smoothMot, smoothEvo, smoothCmpx, smoothTilt, smoothSpc, smoothLpf, smoothBias;

/* ---------- Filter states ---------- */
float loL = 0.0f, loR = 0.0f;  // Tilt EQ
float lpfL = 0.0f, lpfR = 0.0f;  // Lowpass filter

/* ---------- Motion LFOs ---------- */
float motionLfoPhase = 0.0f;
float panLfoPhase = 0.0f;

/* ---------- Cubic Hermite Interpolation ---------- */
float hermite(float frac, float xm1, float x0, float x1, float x2) {
    float c0 = x0;
    float c1 = 0.5f * (x1 - xm1);
    float c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
    float c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}

/* ---------- Smooth Shimmer Reverb ---------- */
struct SmoothShimmer {
    static constexpr int grainSize = 2048; // Smaller for smoother overlaps
    static constexpr int numGrains = 6;    // More grains for smoother texture
    static constexpr int bufSize = grainSize * 3;
    
    struct SmoothGrain {
        std::vector<float> bufL, bufR;
        int writeIdx = 0;
        float phase = 0.0f;
        float speed = 1.5f;  // +7 semitones instead of octave
        float amp = 0.0f;
        float phaseInc = 0.0f;
        
        SmoothGrain() : bufL(bufSize, 0.0f), bufR(bufSize, 0.0f) {}
        
        std::pair<float,float> process(float L, float R) {
            // Gentle pre-filter to tame HF
            static float preLP_L = 0.0f, preLP_R = 0.0f;
            const float lpCoef = 0.25f; // ~7kHz cutoff
            preLP_L += lpCoef * (L - preLP_L);
            preLP_R += lpCoef * (R - preLP_R);
            
            bufL[writeIdx] = preLP_L;
            bufR[writeIdx] = preLP_R;
            writeIdx = (writeIdx + 1) % bufSize;
            
            float readPos = writeIdx - grainSize + phase * grainSize;
            if(readPos < 0) readPos += bufSize;
            
            int r1 = int(readPos) % bufSize;
            int r2 = (r1 + 1) % bufSize;
            float frac = readPos - int(readPos);
            
            // Smoother Hann window
            float window = 0.5f * (1.0f - cos(2.0f * M_PI * phase));
            window = window * window; // Squared for even smoother transitions
            
            float outL = (bufL[r1] * (1-frac) + bufL[r2] * frac) * window * amp;
            float outR = (bufR[r1] * (1-frac) + bufR[r2] * frac) * window * amp;
            
            phase += phaseInc / SAMPLE_RATE;
            if(phase >= 1.0f) {
                phase = 0.0f;
                amp = 0.4f + uniform(gen) * 0.2f;
                // Very subtle detune ±3 cents
                speed = 1.5f + (uniform(gen) - 0.5f) * 0.006f;
                phaseInc = speed * 15.0f; // Slower grain rate
            }
            
            return {outL, outR};
        }
    };
    
    std::vector<SmoothGrain> grains;
    float shimmerAmt = 0.0f;
    
    SmoothShimmer() : grains(numGrains) {
        for(int i = 0; i < numGrains; ++i) {
            grains[i].phase = float(i) / numGrains;
            grains[i].amp = 0.3f + uniform(gen) * 0.2f;
            grains[i].speed = 1.5f + (uniform(gen) - 0.5f) * 0.006f;
            grains[i].phaseInc = grains[i].speed * 15.0f;
        }
    }
    
    std::pair<float,float> process(float L, float R, float amount) {
        shimmerAmt += (amount - shimmerAmt) * 0.0001f;
        
        float outL = 0.0f, outR = 0.0f;
        
        for(auto& grain : grains) {
            auto [gL, gR] = grain.process(L, R);
            outL += gL;
            outR += gR;
        }
        
        return {outL * shimmerAmt * 0.15f, outR * shimmerAmt * 0.15f};
    }
} smoothShimmer;

/* ---------- Original Shimmer (for A/B) ---------- */
struct ImprovedShimmer {
    static constexpr int grainSize = 4096; // Increased from 1024
    static constexpr int numGrains = 4;
    static constexpr int bufSize = grainSize * 2;
    
    struct Grain {
        std::vector<float> bufL, bufR;
        int writeIdx = 0;
        float phase = 0.0f;
        float speed = 1.2f;
        float amp = 0.0f;
        
        Grain() : bufL(bufSize, 0.0f), bufR(bufSize, 0.0f) {}
        
        std::pair<float,float> process(float L, float R) {
            // Pre-filter to tame HF before pitch shift
            static float preLP_L = 0.0f, preLP_R = 0.0f;
            const float lpCoef = 0.2f; // 5-8 kHz one-pole
            preLP_L += lpCoef * (L - preLP_L);
            preLP_R += lpCoef * (R - preLP_R);
            
            bufL[writeIdx] = preLP_L;
            bufR[writeIdx] = preLP_R;
            writeIdx = (writeIdx + 1) % bufSize;
            
            float readPos = writeIdx - grainSize + phase * grainSize;
            if(readPos < 0) readPos += bufSize;
            
            int r1 = int(readPos) % bufSize;
            int r2 = (r1 + 1) % bufSize;
            float frac = readPos - int(readPos);
            
            float window = 0.5f * (1.0f - cos(2.0f * M_PI * phase));
            
            float outL = (bufL[r1] * (1-frac) + bufL[r2] * frac) * window * amp;
            float outR = (bufR[r1] * (1-frac) + bufR[r2] * frac) * window * amp;
            
            phase += speed / SAMPLE_RATE * 20.0f;
            if(phase >= 1.0f) {
                phase = 0.0f;
                amp = 0.6f + uniform(gen) * 0.3f;
                // Reduced detune from ±30 to ±10 cents
                speed = 1.2f + (uniform(gen) - 0.5f) * 0.02f;
            }
            
            return {outL, outR};
        }
    };
    
    std::vector<Grain> grains;
    float shimmerAmt = 0.0f;
    
    ImprovedShimmer() : grains(numGrains) {
        for(int i = 0; i < numGrains; ++i) {
            grains[i].phase = float(i) / numGrains;
            grains[i].amp = 0.8f + uniform(gen) * 0.4f;
            // Reduced detune for smoother sound
            grains[i].speed = 1.2f + (uniform(gen) - 0.5f) * 0.02f;
        }
    }
    
    std::pair<float,float> process(float L, float R, float amount) {
        shimmerAmt += (amount - shimmerAmt) * 0.0001f;
        
        float outL = 0.0f, outR = 0.0f;
        
        for(auto& grain : grains) {
            auto [gL, gR] = grain.process(L, R);
            outL += gL;
            outR += gR;
        }
        
        return {outL * shimmerAmt * 0.12f, outR * shimmerAmt * 0.12f};
    }
} originalShimmer;

/* ---------- Gentle Chorus ---------- */
struct GentleChorus {
    static constexpr int maxDelay = 2205;
    std::vector<float> bufL, bufR;
    int idx = 0;
    float lfo1Phase = 0.0f, lfo2Phase = 0.0f;
    
    // Gentle high-cut filter states
    float hcL = 0.0f, hcR = 0.0f;
    
    GentleChorus() : bufL(maxDelay, 0.0f), bufR(maxDelay, 0.0f) {
        lfo2Phase = M_PI * 0.6f;
    }
    
    std::pair<float,float> process(float L, float R, float depth, float stereoWidth, float lpfAmount) {
        bufL[idx] = L;
        bufR[idx] = R;
        
        float dt = 1.0f / SAMPLE_RATE;
        
        // Slower, gentler LFOs
        lfo1Phase += 2.0f * M_PI * 0.2f * dt;  // Slower
        lfo2Phase += 2.0f * M_PI * 0.35f * dt; // Slower
        if(lfo1Phase > 2.0f * M_PI) lfo1Phase -= 2.0f * M_PI;
        if(lfo2Phase > 2.0f * M_PI) lfo2Phase -= 2.0f * M_PI;
        
        // Smoother LFO shapes
        float lfo1 = sin(lfo1Phase) * 0.7f + sin(lfo1Phase * 2.1f) * 0.3f;
        float lfo2 = sin(lfo2Phase) * 0.7f + sin(lfo2Phase * 1.8f) * 0.3f;
        
        // Much gentler depth scaling
        float gentleDepth = depth * depth * 0.5f; // Square and reduce
        
        float delayMsL = 12.0f + lfo1 * gentleDepth * 8.0f; // Smaller range
        float delayMsR = 12.0f + lfo2 * gentleDepth * 8.0f;
        
        // Subtle stereo width
        delayMsL += stereoWidth * 3.0f;
        delayMsR -= stereoWidth * 3.0f;
        
        float delaySampsL = delayMsL * SAMPLE_RATE / 1000.0f;
        float delaySampsR = delayMsR * SAMPLE_RATE / 1000.0f;
        
        // Simple linear interpolation (cubic was causing artifacts)
        auto readDelay = [&](float delaySamps, const std::vector<float>& buf) {
            int delay = int(delaySamps);
            float frac = delaySamps - delay;
            int r1 = (idx - delay + maxDelay) % maxDelay;
            int r2 = (idx - delay - 1 + maxDelay) % maxDelay;
            return buf[r1] * (1-frac) + buf[r2] * frac;
        };
        
        float chorusL = readDelay(delaySampsL, bufL);
        float chorusR = readDelay(delaySampsR, bufR);
        
        idx = (idx + 1) % maxDelay;
        
        // Much gentler mix
        float outL = L + chorusL * gentleDepth * 0.3f;
        float outR = R + chorusR * gentleDepth * 0.3f;
        
        // Gentle high-cut instead of shelf (simpler and safer)
        const float hcFreq = 8000.0f; // Higher cutoff
        float hcAlpha = dt / (1.0f / (2.0f * M_PI * hcFreq) + dt);
        hcL += hcAlpha * (outL - hcL);
        hcR += hcAlpha * (outR - hcR);
        
        // Blend in some filtering
        float filterMix = depth * 0.3f;
        outL = outL * (1.0f - filterMix) + hcL * filterMix;
        outR = outR * (1.0f - filterMix) + hcR * filterMix;
        
        return {outL, outR};
    }
} gentleChorus;

/* ---------- Original Chorus (for A/B) ---------- */
struct OriginalChorus {
    static constexpr int maxDelay = 2205;
    std::vector<float> bufL, bufR;
    int idx = 0;
    float lfo1Phase = 0.0f, lfo2Phase = 0.0f;
    
    OriginalChorus() : bufL(maxDelay, 0.0f), bufR(maxDelay, 0.0f) {
        lfo2Phase = M_PI * 0.7f;
    }
    
    std::pair<float,float> process(float L, float R, float depth, float stereoWidth) {
        bufL[idx] = L;
        bufR[idx] = R;
        
        float dt = 1.0f / SAMPLE_RATE;
        lfo1Phase += 2.0f * M_PI * 0.3f * dt;
        lfo2Phase += 2.0f * M_PI * 0.47f * dt;
        if(lfo1Phase > 2.0f * M_PI) lfo1Phase -= 2.0f * M_PI;
        if(lfo2Phase > 2.0f * M_PI) lfo2Phase -= 2.0f * M_PI;
        
        float lfo1 = sin(lfo1Phase);
        float lfo2 = sin(lfo2Phase);
        
        float delayMsL = 15.0f + (lfo1 + lfo2 * 0.3f) * depth * 15.0f;
        float delayMsR = 15.0f + (lfo2 + lfo1 * 0.3f) * depth * 15.0f;
        
        delayMsL += stereoWidth * 8.0f;
        delayMsR -= stereoWidth * 8.0f;
        
        float delaySampsL = delayMsL * SAMPLE_RATE / 1000.0f;
        float delaySampsR = delayMsR * SAMPLE_RATE / 1000.0f;
        
        auto readDelay = [&](float delaySamps, const std::vector<float>& buf) {
            int delay = int(delaySamps);
            float frac = delaySamps - delay;
            int r1 = (idx - delay + maxDelay) % maxDelay;
            int r2 = (idx - delay - 1 + maxDelay) % maxDelay;
            return buf[r1] * (1-frac) + buf[r2] * frac;
        };
        
        float chorusL = readDelay(delaySampsL, bufL);
        float chorusR = readDelay(delaySampsR, bufR);
        
        idx = (idx + 1) % maxDelay;
        
        return {L + chorusL * depth * 0.6f, R + chorusR * depth * 0.6f};
    }
} originalChorus;

/* ---------- Improved Reverb ---------- */
struct ImprovedReverb {
    struct AllPass {
        std::vector<float> buffer;
        int idx = 0;
        float feedback;
        
        AllPass(float delayMs, float fb) : feedback(fb) {
            int size = int(SAMPLE_RATE * delayMs / 1000.0f);
            buffer.resize(size, 0.0f);
        }
        
        float process(float input) {
            float delayed = buffer[idx];
            float output = -input + delayed;
            buffer[idx] = input + delayed * feedback;
            idx = (idx + 1) % buffer.size();
            return output;
        }
    };
    
    struct CombFilter {
        std::vector<float> buffer;
        int idx = 0;
        float feedback;
        float dampening = 0.0f;
        float filterState = 0.0f;
        
        CombFilter(float delayMs, float fb) : feedback(fb) {
            int size = int(SAMPLE_RATE * delayMs / 1000.0f);
            buffer.resize(size, 0.0f);
        }
        
        float process(float input) {
            float output = buffer[idx];
            filterState += (output - filterState) * (0.005f + dampening * 0.1f);
            buffer[idx] = input + filterState * feedback;
            idx = (idx + 1) % buffer.size();
            return output;
        }
    };
    
    std::vector<CombFilter> combsL, combsR;
    std::vector<AllPass> allpassL, allpassR;
    
    ImprovedReverb() {
        std::vector<float> combDelaysL = {29.7f, 37.1f, 41.1f, 43.7f};
        std::vector<float> combDelaysR = {30.5f, 36.4f, 40.8f, 42.9f};
        
        for(float delay : combDelaysL) {
            combsL.emplace_back(delay, 0.85f);
        }
        for(float delay : combDelaysR) {
            combsR.emplace_back(delay, 0.85f);
        }
        
        std::vector<float> allpassDelays = {5.0f, 1.7f};
        for(float delay : allpassDelays) {
            allpassL.emplace_back(delay, 0.7f);
            allpassR.emplace_back(delay, 0.7f);
        }
    }
    
    std::pair<float,float> process(float L, float R) {
        float combOutL = 0.0f, combOutR = 0.0f;
        for(auto& comb : combsL) {
            combOutL += comb.process(L) * 0.28f;
        }
        for(auto& comb : combsR) {
            combOutR += comb.process(R) * 0.28f;
        }
        
        float outL = combOutL, outR = combOutR;
        for(auto& ap : allpassL) {
            outL = ap.process(outL);
        }
        for(auto& ap : allpassR) {
            outR = ap.process(outR);
        }
        
        return {outL, outR};
    }
} reverb;

int activeLoopWithHarmony = -1;

/* ---------- wavetable class ---------- */
struct Wavetable {
    std::vector<float> data;
    Wavetable() : data(WAVETABLE_SIZE) {}

    float sample(float phase) const {
        float pos = phase * WAVETABLE_SIZE / (2.0f * M_PI);
        pos = fmod(pos, (float)WAVETABLE_SIZE);
        if (pos < 0) pos += WAVETABLE_SIZE;

        int i = int(pos);
        float frac = pos - i;
        float a = data[i];
        float b = data[(i + 1) % WAVETABLE_SIZE];
        return a + frac * (b - a);
    }

    void cello() {
        for (int i=0;i<WAVETABLE_SIZE;i++){
            float t = 2*M_PI*i/WAVETABLE_SIZE;
            float s = 0.8f*sin(t)+0.4f*sin(2*t)+0.25f*sin(3*t)+0.15f*sin(4*t)
                    +0.10f*sin(5*t)+0.08f*sin(6*t)+0.05f*sin(8*t);
            data[i] = s*0.25f;
        }
    }
    void warmBass() {
        for (int i=0;i<WAVETABLE_SIZE;i++){
            float t = 2*M_PI*i/WAVETABLE_SIZE;
            float s = 1.0f*sin(t)+0.6f*sin(2*t)+0.3f*sin(3*t)
                    +0.15f*sin(4*t)+0.08f*sin(5*t);
            s *= (0.7f+0.3f*exp(-t*0.3f));
            data[i] = s*0.2f;
        }
    }
    void softString() {
        for (int i=0;i<WAVETABLE_SIZE;i++){
            float t = 2*M_PI*i/WAVETABLE_SIZE;
            float s = 0.9f*sin(t)+0.3f*sin(2*t)+0.15f*sin(3*t)+0.08f*sin(4*t);
            s *= (1+cos(t))*0.5f;
            data[i] = s*0.35f;
        }
    }
    void cleanSine() {
        for (int i=0;i<WAVETABLE_SIZE;i++){
            float t = 2*M_PI*i/WAVETABLE_SIZE;
            data[i] = 0.55f*sin(t);
        }
    }
    void piano() {
        for (int i=0;i<WAVETABLE_SIZE;i++){
            float t = 2*M_PI*i/WAVETABLE_SIZE;
            float s = sin(t)+0.8f*sin(2*t)+0.6f*sin(3*t)+0.4f*sin(4*t)
                    +0.3f*sin(5*t)+0.2f*sin(6*t)+0.15f*sin(7*t)+0.1f*sin(8*t);
            data[i] = s*0.25f;
        }
    }
};

std::vector<Wavetable> wt(5);
struct WhiteNoise { float next(){return (uniform(gen)-0.5f)*2.0f;} } noise;

/* ---------- Voice declarations ---------- */
int countActiveVoices();

/* ---------- Drone voice ---------- */
struct Drone {
    float f0{}, f1{}, p0{}, p1{}, amp{}, targAmp{}, age{}, life{}, pan{}, panVel{};
    int   w=0;  bool active=false;

    void spawn() {
        const float pool[] = {55,65.4f,73.4f,82.4f,87.3f,98,110,123.5f,130.8f,146.8f,164.8f,174.6f,196};
        f0 = pool[gen()%13];
        
        float bias = smoothBias.current;
        float pitchMult = std::pow(2.0f, bias - 0.5f);
        f0 *= pitchMult;
        
        f1 = (uniform(gen)<0.7f)?f0*1.5f:f0*2.0f;
        targAmp=0.04f+uniform(gen)*0.05f; amp=0;
        life = 80+uniform(gen)*160; age=0;
        pan = 0.2f+uniform(gen)*0.6f;
        panVel=(uniform(gen)-0.5f)*0.000002f;
        w = gen()%3;
        p0 = uniform(gen)*2*M_PI; p1 = uniform(gen)*2*M_PI;
        active=true;
        std::cout<<"Spawn drone "<<f0<<"+"<<f1<<"\n";
    }
    
    std::pair<float,float> tick(){
        if(!active) return {0,0};
        age+=1.0f/SAMPLE_RATE;
        if(age>life){active=false; return {0,0};}
        
        float env=1.0f;
        const float A=25,R=30;
        if(age<A) env=sin(0.5f*M_PI*age/A);
        else if(age>life-R){ env=sin(0.5f*M_PI*(life-age)/R);}
        
        amp += (targAmp-amp)*0.0000005f;
        pan += panVel + 0.00001f*sin(age*0.03f); pan=std::clamp(pan,0.1f,0.9f);

        float s = (wt[w].sample(p0)+0.6f*wt[w].sample(p1))*amp*env*masterVolume;
        p0+=2*M_PI*f0/SAMPLE_RATE; if(p0>=2*M_PI) p0-=2*M_PI;
        p1+=2*M_PI*f1/SAMPLE_RATE; if(p1>=2*M_PI) p1-=2*M_PI;
        float gL = sqrtf(1-pan), gR = sqrtf(pan);
        return {s*gL,s*gR};
    }
};

/* ---------- Bell melody voice ---------- */
struct BellMelody {
    float frequency = 440.0f, phase = 0.0f, amp = 0.08f, fade = 0.0f;
    float age = 0.0f, noteAge = 0.0f, noteDur = 2.0f, restDur = 5.0f;
    int current = 0;
    std::vector<float> sequence;
    float pan = 0.5f;
    bool inNote = false, active = false;

    void spawn() {
        static const float scale[] = {
            349.2f, 392.0f, 440.0f, 523.3f, 587.3f,
            659.3f, 783.9f, 880.0f, 1046.5f
        };

        int len = 3 + (gen() % 3);
        sequence.clear();
        
        float bias = smoothBias.current;
        float pitchMult = std::pow(2.0f, bias - 0.5f);
        
        for (int i = 0; i < len; ++i)
            sequence.push_back(scale[gen() % 9] * pitchMult);

        frequency = sequence[0];
        amp = 0.05f + uniform(gen) * 0.03f;
        noteDur = 0.8f + uniform(gen) * 1.5f;
        restDur = 2.0f + uniform(gen) * 5.0f;
        phase = 0.0f; age = noteAge = 0.0f; current = 0;
        inNote = true; pan = 0.3f + uniform(gen) * 0.4f; fade = 0.0f;
        active = true;
        std::cout << "Spawn bell melody (" << len << " notes)\n";
    }

    std::pair<float, float> tick() {
        if (!active) return {0.0f, 0.0f};

        float targetFade = melodyEnabled.load() ? 1.0f : 0.0f;
        fade += (targetFade - fade) * 0.0002f;
        if (fade < 0.001f && !melodyEnabled) { active = false; return {0,0}; }

        const float dt = 1.0f / SAMPLE_RATE; age += dt; noteAge += dt;

        if (inNote && noteAge >= noteDur) {
            inNote = false; noteAge = 0.0f;
        } else if (!inNote && noteAge >= restDur) {
            inNote = true; current = (current + 1) % sequence.size();
            frequency = sequence[current]; noteAge = 0.0f;
        }

        if (age > 120.0f) { active = false; return {0,0}; }

        float env;
        const float ATTACK = 0.01f, RELEASE = 10.0f;
        if (noteAge < ATTACK) env = noteAge / ATTACK;
        else env = std::exp(-(noteAge - ATTACK) / RELEASE);

        float s = wt[3].sample(phase) * amp * env * fade * masterVolume.load();
        phase += 2.0f * M_PI * frequency * dt;
        if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;

        float gL = std::sqrt(1.0f - pan), gR = std::sqrt(pan);
        return { s * gL, s * gR };
    }
};

/* ---------- Loop voice ---------- */
struct Loop {
    std::vector<float> notes, vel;
    float phase{}, harmPhase{}, amp{}, curAmp{}, fade{}, age{}, noteAge{}, evoT{}, metro{};
    int pos{}; float pan{}; bool active=false, harmony=false; int self=-1; bool dying=false;

    void spawn(int index){
        self=index;
        const float pent[]={174.6f,196,220,261.6f,293.7f,329.6f,349.2f,392};
        int n = 4+(gen()%3); notes.resize(n); vel.resize(n);
        
        float bias = smoothBias.current;
        float pitchMult = std::pow(2.0f, bias - 0.5f);
        
        for(int i=0;i<n;i++){ 
            notes[i]=pent[gen()%8] * pitchMult; 
            vel[i]=0.7f+uniform(gen)*0.3f; 
        }
        amp=0.08f+uniform(gen)*0.06f; curAmp=0; fade=0;
        phase=harmPhase=0; age=noteAge=evoT=metro=0; pos=0;
        pan=0.3f+uniform(gen)*0.4f; dying=false;
        std::cout<<"Spawn loop "<<n<<" notes"<<(harmony?" +5th":"")<<"\n";
        active=true;
    }

    std::pair<float,float> tick(){
        if(!active) return {0,0};
        const float dt = 1.0f/SAMPLE_RATE;
        
        float evoSpeed = smoothEvo.current * 2.8f + 0.2f;
        age += dt * evoSpeed; noteAge += dt; metro += dt; evoT += dt * evoSpeed;

        float totalT=loopLength; float noteT=totalT/notes.size();
        if(metro>=noteT){ metro=noteAge=0; pos=(pos+1)%notes.size(); }

        if(evoT>900+uniform(gen)*300){
            const float pent[]={174.6f,196,220,261.6f,293.7f,329.6f,349.2f,392};
            float bias = smoothBias.current;
            float pitchMult = std::pow(2.0f, bias - 0.5f);
            notes[gen()%notes.size()]=pent[gen()%8] * pitchMult; 
            evoT=0;
        }

        if(!dying && age> totalT*(40+uniform(gen)*25)){ dying=true; }

        if(!dying) fade += (1.0f-fade)*0.00005f;
        else fade -= 0.000006f;
        if(fade<=0.0f){
            active=false;
            if(harmony && activeLoopWithHarmony==self) activeLoopWithHarmony=-1;
            return {0,0};
        }

        float targ = amp*vel[pos]; curAmp += (targ-curAmp)*0.002f;
        float f = notes[pos]; float s = wt[3].sample(phase);
        if(harmony) s += 0.6f*wt[3].sample(harmPhase);
        s *= curAmp * fade * masterVolume;

        phase += 2*M_PI*f*dt; if(phase>=2*M_PI) phase-=2*M_PI;
        if(harmony){ harmPhase+=2*M_PI*(f*1.5f)*dt; if(harmPhase>=2*M_PI) harmPhase-=2*M_PI;}

        float gL=sqrtf(1-pan), gR=sqrtf(pan);
        return {s*gL,s*gR};
    }
};

/* ---------- kick_deep with texture variations ---------- */
struct Perc {
    float age{}, tNext{}, pan{}, fade{};
    bool active=false;
    
    float kickAge = 0.0f, kickPhase = 0.0f;
    bool kickActive = false;
    float pitchStart = 80.0f, pitchEnd = 35.0f, pitchDecay = 3.0f, ampDecay = 1.2f;

    void spawn(){
        age=0; tNext=2+uniform(gen)*5; pan=0.4f+uniform(gen)*0.2f;
        fade=0; active=true;
        std::cout<<"Spawn kick generator\n";
    }
    
    void triggerKick() {
        kickAge = 0.0f; kickPhase = 0.0f; kickActive = true;
        
        float tex = smoothTex.current, rand = uniform(gen);
        
        if (tex > 0.6f && rand < 0.33f) {
            // Short kick - gentler
            pitchStart = 90.0f; pitchEnd = 45.0f; pitchDecay = 6.0f; ampDecay = 2.5f;
        } else if (tex < 0.4f && rand < 0.5f) {
            // Long kick - very gentle
            pitchStart = 70.0f; pitchEnd = 40.0f; pitchDecay = 1.5f; ampDecay = 0.8f;  
        } else {
            // Default deep kick - much gentler
            pitchStart = 60.0f; pitchEnd = 35.0f; pitchDecay = 2.0f; ampDecay = 1.0f;
        }
    }
    
    std::pair<float,float> tick(){
        if(!active) return {0,0};
        const float dt=1.0f/SAMPLE_RATE;
        
        fade += ((percussionEnabled?1.0f:0.0f)-fade)*0.0001f;
        if(fade<0.001f && !percussionEnabled){ active=false; return {0,0}; }
        
        float evoSpeed = smoothEvo.current * 2.8f + 0.2f;
        age += dt * evoSpeed; 
        if(age>180){active=false; return {0,0};}

        if(age >= tNext) {
            triggerKick();
            tNext = age + (12.0f + uniform(gen)*20.0f);  // Even less frequent
        }
        
        float out = 0.0f;
        
        if(kickActive) {
            kickAge += dt;
            
            if(kickAge > 2.5f) {  // Longer duration
                kickActive = false;
            } else {
                float t = kickAge;
                float pitchEnv = exp(-t * pitchDecay);
                float frequency = pitchEnd + (pitchStart - pitchEnd) * pitchEnv;
                
                // Much gentler click - shorter and much softer
                float clickDuration = 0.0003f;  // 0.3ms 
                float clickAmp = (kickAge < clickDuration) ? 
                    (1.0f - kickAge/clickDuration) * 0.03f : 0.0f;  // Much quieter
                float click = noise.next() * clickAmp;
                
                float sine = sin(kickPhase);
                kickPhase += 2.0f * M_PI * frequency * dt;
                if (kickPhase > 2.0f * M_PI) kickPhase -= 2.0f * M_PI;
                
                // Slower attack envelope
                float ampEnv = exp(-t * ampDecay);
                if (t < 0.01f) ampEnv *= (t / 0.01f); // 10ms attack
                
                out = (click + sine * 0.4f) * ampEnv * 0.15f;  // Much quieter overall
            }
        }
        
        out *= fade;
        float gL=sqrtf(1-pan), gR=sqrtf(pan);
        return {out*gL,out*gR};
    }
};

/* ---------- Voice containers ---------- */
std::vector<Drone> drones(4);
std::vector<BellMelody> bells(2);
std::vector<Loop> loops(3);
std::vector<Perc> percs(2);

int countActiveVoices() {
    int count = 0;
    for(auto &v:drones) if(v.active) count++;
    for(auto &v:loops) if(v.active) count++;
    for(auto &v:bells) if(v.active) count++;
    for(auto &v:percs) if(v.active) count++;
    return count;
}

/* ---------- Audio callback ---------- */
float droneSpawnTimer=0;

static int cb(const void*,void* output, unsigned long n, const PaStreamCallbackTimeInfo*,
              PaStreamCallbackFlags, void*) {
    float* out=(float*)output;

    float vol = gVol.load();
    smoothTex.setTarget(gTex.load()); smoothMot.setTarget(gMot.load());  
    smoothEvo.setTarget(gEvo.load()); smoothCmpx.setTarget(gCmpx.load());
    smoothTilt.setTarget(gTilt.load()); smoothSpc.setTarget(gSpc.load());
    smoothLpf.setTarget(gLpf.load()); smoothBias.setTarget(gBias.load());
    
    float tex = smoothTex.update(), mot = smoothMot.update();
    float evoSpeed = smoothEvo.update() * 2.8f + 0.2f, cmpx = smoothCmpx.update();
    int vmax = gVMax.load(); float tilt = smoothTilt.update();
    float spc = smoothSpc.update(), lpf = smoothLpf.update();
    
    // Get A/B test states
    bool useOldShimmer = gOldShimmer.load();
    bool useOldChorus = gOldChorus.load();
    
    melodyEnabled = (cmpx >= 0.3f); percussionEnabled = (cmpx >= 0.7f);

    for(unsigned long i=0;i<n;i++){
        float L=0,R=0;

        float dt = 1.0f / SAMPLE_RATE;
        motionLfoPhase += 2.0f * M_PI * 0.05f * dt;
        if(motionLfoPhase > 2.0f * M_PI) motionLfoPhase -= 2.0f * M_PI;
        panLfoPhase += 2.0f * M_PI * 0.03f * dt;
        if(panLfoPhase > 2.0f * M_PI) panLfoPhase -= 2.0f * M_PI;
        
        float panLfo = sin(panLfoPhase);
        int activeVoices = countActiveVoices();

        // Proportional voice spawning
        float droneRatio = 1.0f - tex, loopRatio = tex;
        float randomFactor = (uniform(gen) - 0.5f) * 0.3f;
        droneRatio = std::clamp(droneRatio + randomFactor, 0.1f, 0.9f);
        loopRatio = 1.0f - droneRatio;
        
        int targetDrones = int(vmax * droneRatio), targetLoops = vmax - targetDrones;
        int currentDrones = 0, currentLoops = 0;
        for(auto &v:drones) if(v.active) currentDrones++;
        for(auto &v:loops) if(v.active) currentLoops++;
        
        droneSpawnTimer += dt * evoSpeed;
        if(droneSpawnTimer > 30 && currentDrones < targetDrones && 
           uniform(gen) < 0.00002f && activeVoices < vmax) {
            for(auto &v:drones) if(!v.active){ v.spawn(); droneSpawnTimer=0; break; }
        }
        
        static float bellTimer=0; bellTimer += dt * evoSpeed;
        if(melodyEnabled && bellTimer > 45 && uniform(gen) < 0.00001f && activeVoices < vmax){
            for(auto &b:bells) if(!b.active){ b.spawn(); bellTimer=0; break;}
        }
        
        static float loopSpawnTimer=0; loopSpawnTimer += dt * evoSpeed;
        float interval = loopLength*6;
        if(loopSpawnTimer > interval && currentLoops < targetLoops && 
           uniform(gen) < 0.00002f && activeVoices < vmax){
            for(size_t j=0;j<loops.size();++j){
                auto &v=loops[j]; if(!v.active){
                    bool want5th = (activeLoopWithHarmony<0) && uniform(gen)<0.50f;
                    v.harmony = want5th;
                    if(want5th) activeLoopWithHarmony=j;
                    v.spawn((int)j); loopSpawnTimer=0; break;
                }
            }
        }
        
        static float pTimer=0; pTimer += dt * evoSpeed;
        if(percussionEnabled && pTimer > 20 && uniform(gen) < 0.0001f && activeVoices < vmax){
            for(auto &p:percs) if(!p.active){ p.spawn(); pTimer=0; break; }
        }
        
        // Mix all voices directly
        for(auto &v:drones){ auto [l,r] = v.tick(); L += l; R += r; }
        for(auto &v:loops){ auto [l,r] = v.tick(); L += l; R += r; }
        
        float melodyFade = (cmpx < 0.3f) ? 0.0f : (cmpx < 0.7f) ? (cmpx - 0.3f) / 0.4f : 1.0f;
        for(auto &b:bells) { auto [l,r] = b.tick(); L += l * melodyFade; R += r * melodyFade; }
        
        float percFade = (cmpx >= 0.7f) ? (cmpx - 0.7f) / 0.3f : 0.0f;
        for(auto &p:percs){ auto [l,r] = p.tick(); L += l * percFade; R += r * percFade; }

        // Motion effect (A/B test between old and new chorus)
        if(mot > 0.01f) {
            float stereoWidth = mot * mot * 0.5f; // Reduced stereo width
            
            if (useOldChorus) {
                auto [cL, cR] = originalChorus.process(L, R, mot * 0.8f, stereoWidth);
                L = cL; R = cR;
            } else {
                auto [cL, cR] = gentleChorus.process(L, R, mot, stereoWidth, lpf);
                L = cL; R = cR;
            }
            
            // Much gentler pan modulation
            float panDepth = mot * 0.15f; // Reduced from 0.4f
            float leftGain = 1.0f - panDepth * (0.5f - 0.5f * panLfo);
            float rightGain = 1.0f - panDepth * (0.5f + 0.5f * panLfo);
            
            L *= leftGain;
            R *= rightGain;
            
            // Stronger compression to prevent clipping
            float motionGain = 1.0f - mot * 0.25f; // Increased from 0.15f
            L *= motionGain;
            R *= motionGain;
        }

        // Reverb + shimmer (A/B test between old and new shimmer)
        auto [revL,revR] = reverb.process(L, R);
        float wetMix = spc, dryMix = 1.0f - wetMix;
        
        if(wetMix > 0.4f) {
            float shimmerAmt = (wetMix - 0.4f) / 0.6f;
            
            if (useOldShimmer) {
                auto [shimL, shimR] = originalShimmer.process(revL, revR, shimmerAmt);
                revL += shimL; revR += shimR;
            } else {
                auto [shimL, shimR] = smoothShimmer.process(revL, revR, shimmerAmt);
                revL += shimL; revR += shimR;
            }
        }
        
        L = L * dryMix + revL * wetMix; R = R * dryMix + revR * wetMix;

        // Lowpass filter with volume compensation
        if(lpf > 0.01f) {
            float cutoffHz = 20000.0f - lpf * 19800.0f;
            float rc = 1.0f / (2.0f * M_PI * cutoffHz), alpha = dt / (rc + dt);
            lpfL += alpha * (L - lpfL); lpfR += alpha * (R - lpfR);
            
            // Volume compensation - boost lows as highs are cut
            float compensation = 1.0f + lpf * 0.8f; // Up to 80% boost
            L = lpfL * compensation; 
            R = lpfR * compensation;
        }

        // Tilt EQ
        const float fc = 1600.0f, rc = 1.0f / (2.0f * M_PI * fc), alpha = dt / (rc + dt);
        loL += alpha * (L - loL); loR += alpha * (R - loR);
        float hiL = L - loL, hiR = R - loR;
        float bassGain = 1.0f + (0.5f - tilt) * 0.6f, trebGain = 1.0f + (tilt - 0.5f) * 0.6f;
        L = loL * bassGain + hiL * trebGain; R = loR * bassGain + hiR * trebGain;

        *out++ = std::clamp(L * vol * 0.9f, -0.99f, 0.99f);
        *out++ = std::clamp(R * vol * 0.9f, -0.99f, 0.99f);
    }
    return paContinue;
}

/* ---------- CLI control thread ---------- */
void controlLoop() {
    std::string tok;
    std::cout << "\nESP32 Controls: v t m e c n w s l b (0-100) | o (loop length) | q quit\n";
    std::cout << "A/B Tests: f (toggle shimmer) | g (toggle chorus) | ? (status)\n";
    std::cout << "Example: v70, t50, w30, f (toggle), g (toggle)\n> ";
    
    while (running && std::cin >> tok) {
        if(tok == "q") { running = false; break; }
        if(tok == "o") { 
            loopLength = (loopLength < 8 ? loopLength + 1 : 2);
            std::cout << "Loop length: " << loopLength.load() << "s\n> ";
            continue;
        }
        if(tok == "f") {
            gOldShimmer = !gOldShimmer.load();
            std::cout << "Shimmer: " << (gOldShimmer.load() ? "OLD (granular)" : "NEW (smooth)") << "\n> ";
            continue;
        }
        if(tok == "g") {
            gOldChorus = !gOldChorus.load();
            std::cout << "Chorus: " << (gOldChorus.load() ? "OLD (linear)" : "NEW (gentle)") << "\n> ";
            continue;
        }
        if(tok == "?") {
            std::cout << "Status:\n";
            std::cout << "  Shimmer: " << (gOldShimmer.load() ? "OLD (granular)" : "NEW (smooth)") << "\n";
            std::cout << "  Chorus: " << (gOldChorus.load() ? "OLD (linear)" : "NEW (gentle)") << "\n";
            std::cout << "  Volume: " << int(gVol.load() * 100) << "%\n";
            std::cout << "  Motion: " << int(gMot.load() * 100) << "%\n";
            std::cout << "  Space: " << int(gSpc.load() * 100) << "%\n> ";
            continue;
        }
        if(tok.size() < 2) { std::cout << "? > "; continue; }
        
        char cmd = std::tolower(tok[0]);
        int val;
        try { val = std::stoi(tok.substr(1)); } 
        catch(...) { std::cout << "Invalid number > "; continue; }
        
        val = std::clamp(val, 0, 100); float f = val / 100.0f;
        
        switch(cmd) {
            case 'v': gVol = f; std::cout << "Volume: " << val << "%"; break;
            case 't': gTex = f; std::cout << "Texture: " << val << "%"; break;
            case 'm': gMot = f; std::cout << "Motion: " << val << "%"; break;
            case 'e': gEvo = f; std::cout << "Evolve: " << val << "%"; break;
            case 'c': gCmpx = f; std::cout << "Complexity: " << val << "%"; break;
            case 'n': gVMax = 1 + int(f * 4); std::cout << "Voice limit: " << gVMax.load(); break;
            case 'w': gTilt = f; std::cout << "Warm/Bright: " << val << "%"; break;
            case 's': gSpc = f; std::cout << "Space: " << val << "%"; break;
            case 'l': gLpf = f; std::cout << "Lowpass: " << val << "%"; break;
            case 'b': gBias = f; std::cout << "Pitch bias: " << val << "%"; break;
            default: std::cout << "Unknown command"; break;
        }
        std::cout << "\n> ";
    }
}

/* ---------- Main ---------- */
int main(){
    smoothTex.setImmediate(gTex.load()); smoothMot.setImmediate(gMot.load());
    smoothEvo.setImmediate(gEvo.load()); smoothCmpx.setImmediate(gCmpx.load());
    smoothTilt.setImmediate(gTilt.load()); smoothSpc.setImmediate(gSpc.load());
    smoothLpf.setImmediate(gLpf.load()); smoothBias.setImmediate(gBias.load());
    
    wt[0].cello(); wt[1].warmBass(); wt[2].softString(); wt[3].cleanSine(); wt[4].piano();

    Pa_Initialize();
    std::cout<<"Device index? "; int dev; std::cin>>dev;

    PaStreamParameters outPar;
    outPar.device=dev; outPar.channelCount=2; outPar.sampleFormat=paFloat32;
    outPar.suggestedLatency=Pa_GetDeviceInfo(dev)->defaultLowOutputLatency;
    outPar.hostApiSpecificStreamInfo=nullptr;

    PaStream* s;
    Pa_OpenStream(&s,nullptr,&outPar,SAMPLE_RATE,FRAMES_PER_BUFFER, paClipOff,cb,nullptr);
    
    drones[0].spawn();
    Pa_StartStream(s);

    std::cout << "\n=== Meditatron v2 Improved ===\n";
    std::cout << "Starting with NEW algorithms (smooth shimmer + gentle chorus)\n";
    std::cout << "Use 'f' and 'g' to A/B test old vs new algorithms\n";

    std::thread t(controlLoop); t.join();
    Pa_StopStream(s); Pa_CloseStream(s); Pa_Terminate();
    return 0;
}