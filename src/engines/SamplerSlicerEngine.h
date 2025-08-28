#pragma once
#include "../synthesis/BaseEngine.h"
#include "../synthesis/DSPUtils.h"
#include "../SampleBuffer.h"
#include <array>
#include <memory>
#include <vector>

namespace SamplerSlicer {

/**
 * Slice detection modes
 */
enum class DetectMode {
    TRANSIENT = 0,  // Onset detection using energy + spectral flux
    GRID = 1,       // Fixed grid slicing (1/4 to 1/16 note divisions)
    MANUAL = 2      // User-defined slice points
};

/**
 * Playback modes per slice
 */
enum class PlayMode {
    ONESHOT = 0,    // Play slice once and stop
    GATE = 1,       // Play while key held
    THROUGH = 2     // Continue into next slice
};

/**
 * Time processing modes
 */
enum class TimeMode {
    REPITCH = 0,        // Change pitch when changing speed
    SLICE_STRETCH = 1,  // Time-stretch without pitch change
    OFFLINE_STRETCH = 2 // Pre-analyzed phase vocoder (future)
};

/**
 * Individual slice configuration
 */
struct Slice {
    size_t startFrame = 0;
    size_t endFrame = 0;
    float gain = 1.0f;          // -24dB to +12dB
    float pan = 0.0f;           // -1.0 to +1.0
    float pitch = 0.0f;         // ±24 semitones
    bool reverse = false;
    PlayMode playMode = PlayMode::ONESHOT;
    bool loop = false;
    float loopXFade = 0.0f;     // 0-10ms crossfade at loop points
    
    // Envelope (AHDR)
    float attack = 0.001f;
    float hold = 0.0f;
    float decay = 0.3f;
    float release = 0.1f;
    
    // Filter
    float lpfCutoff = 20000.0f;
    float lpfResonance = 0.0f;
    
    // FX sends
    float sendA = 0.0f;
    float sendB = 0.0f;
    float sendC = 0.0f;
    
    bool isValid() const {
        return endFrame > startFrame;
    }
    
    size_t lengthFrames() const {
        return endFrame - startFrame;
    }
    
    float lengthSeconds(float sampleRate) const {
        return static_cast<float>(lengthFrames()) / sampleRate;
    }
};

/**
 * Slice detection algorithms
 */
class SliceDetector {
public:
    static std::vector<size_t> detectTransients(const std::vector<int16_t>& buffer, 
                                               int channels, float sampleRate, 
                                               float sensitivity = 0.5f);
    
    static std::vector<size_t> detectGrid(size_t totalFrames, int divisions = 16);
    
    static std::vector<size_t> snapToZeroCrossings(const std::vector<int16_t>& buffer,
                                                   const std::vector<size_t>& roughSlices,
                                                   int channels, int windowSize = 64);
    
private:
    static float calculateSpectralFlux(const float* window, int size);
    static float calculateEnergy(const float* window, int size);
};

} // namespace SamplerSlicer

/**
 * SamplerSlicer Voice - handles individual slice playback
 */
class SamplerSlicerVoice : public BaseVoice {
public:
    SamplerSlicerVoice() : slice_(0), sliceConfig_(nullptr), 
                          sensitivity_(0.5f), xFade_(0.0f), followAction_(0.0f),
                          playPosition_(0), loopActive_(false) {}
    
    void setSampleRate(float sampleRate) override {
        BaseVoice::setSampleRate(sampleRate);
        // AHDR envelope handled by ampEnv_ from BaseVoice
    }
    
    void noteOn(float note, float velocity) override {
        BaseVoice::noteOn(note, velocity);
        
        // Map MIDI note to slice (C3 = slice 0, C#3 = slice 1, etc.)
        slice_ = static_cast<int>(note) % 25;
        
        if (slicesConfig_ && slice_ < static_cast<int>(slicesConfig_->size())) {
            sliceConfig_ = &(*slicesConfig_)[slice_];
            
            if (sliceConfig_->isValid() && sampleBuffer_) {
                // Configure envelope
                ampEnv_.setAttackTime(sliceConfig_->attack);
                ampEnv_.setDecayTime(sliceConfig_->decay);
                ampEnv_.setSustainLevel(0.0f);  // AHDR has no sustain
                ampEnv_.setReleaseTime(sliceConfig_->release);
                
                // Setup playback
                playPosition_ = sliceConfig_->startFrame;
                loopActive_ = sliceConfig_->loop;
                
                // Pre-compute pan multiplier for performance
                panMultiplier_ = (sliceConfig_->pan >= 0.0f) ? 
                               (1.0f - sliceConfig_->pan) : 1.0f;
                
                // Apply pitch if needed
                if (sliceConfig_->pitch != 0.0f && sampleBuffer_) {
                    sampleBuffer_->setPitch(sliceConfig_->pitch);
                }
            }
        }
    }
    
    void noteOff() override {
        BaseVoice::noteOff();
        
        // Handle different play modes
        if (sliceConfig_) {
            switch (sliceConfig_->playMode) {
                case SamplerSlicer::PlayMode::ONESHOT:
                    // Ignore note off, let slice finish naturally
                    break;
                case SamplerSlicer::PlayMode::GATE:
                    // Stop immediately on note off
                    break;
                case SamplerSlicer::PlayMode::THROUGH:
                    // Continue to next slice (TODO: implement follow actions)
                    break;
            }
        }
    }
    
    float renderSample(const RenderContext& ctx) override {
        if (!active_ || !sliceConfig_ || !sampleBuffer_) return 0.0f;
        
        float envelope = ampEnv_.process();
        
        if (envelope <= 0.001f && releasing_) {
            active_ = false;
            return 0.0f;
        }
        
        // Check if we're past the slice end
        if (playPosition_ >= sliceConfig_->endFrame) {
            if (loopActive_ && sliceConfig_->loop) {
                // Loop back to start
                playPosition_ = sliceConfig_->startFrame;
            } else {
                // Slice finished
                active_ = false;
                return 0.0f;
            }
        }
        
        // Render sample from current position - optimized path
        float sample = 0.0f;
        if (sampleBuffer_->isLoaded()) {
            int16_t sampleData;
            sampleBuffer_->renderSamples(&sampleData, 1, sliceConfig_->gain);
            sample = static_cast<float>(sampleData) * (1.0f / 32768.0f); // Avoid division
        }
        
        // Apply pan - pre-compute pan multipliers to avoid branches
        sample *= panMultiplier_;
        
        // Apply crossfade if near slice boundaries
        float xFadeGain = 1.0f;
        if (xFade_ > 0.0f) {
            size_t xFadeFrames = static_cast<size_t>(xFade_ * 0.010f * sampleRate_); // 10ms max
            size_t distFromStart = playPosition_ - sliceConfig_->startFrame;
            size_t distFromEnd = sliceConfig_->endFrame - playPosition_;
            
            if (distFromStart < xFadeFrames) {
                xFadeGain *= static_cast<float>(distFromStart) / xFadeFrames;
            } else if (distFromEnd < xFadeFrames) {
                xFadeGain *= static_cast<float>(distFromEnd) / xFadeFrames;
            }
        }
        
        sample *= xFadeGain;
        
        // Apply envelope and velocity
        sample *= envelope * velocity_;
        
        // Apply channel strip if present
        if (channelStrip_) {
            sample = channelStrip_->process(sample, note_);
        }
        
        playPosition_++;
        return sample;
    }
    
    void renderBlock(const RenderContext& ctx, float* output, int count) override {
        for (int i = 0; i < count; ++i) {
            output[i] = renderSample(ctx);
        }
    }
    
    // Parameter setters
    void setSampleBuffer(std::shared_ptr<Sample::SampleBuffer> buffer) { sampleBuffer_ = buffer; }
    void setSlicesConfig(std::vector<SamplerSlicer::Slice>* slices) { slicesConfig_ = slices; }
    void setSensitivity(float sensitivity) { sensitivity_ = sensitivity; }
    void setXFade(float xFade) { xFade_ = xFade; }
    void setFollowAction(float followAction) { followAction_ = followAction; }
    
    int getSlice() const { return slice_; }
    
    uint32_t getAge() const { return 0; }  // TODO: Implement proper age tracking
    
private:
    int slice_;
    SamplerSlicer::Slice* sliceConfig_;
    std::vector<SamplerSlicer::Slice>* slicesConfig_ = nullptr;
    std::shared_ptr<Sample::SampleBuffer> sampleBuffer_;
    
    // Macro values
    float sensitivity_;
    float xFade_;
    float followAction_;
    
    // Playback state
    size_t playPosition_;
    bool loopActive_;
    
    // Performance optimizations
    float panMultiplier_; // Pre-computed pan multiplier to avoid branches
};

/**
 * SamplerSlicer Engine - Loop slicing with up to 25 slices
 */
class SamplerSlicerEngine : public PolyphonicBaseEngine<SamplerSlicerVoice> {
public:
    SamplerSlicerEngine() 
        : PolyphonicBaseEngine("SamplerSlicer", "SLIC", 
                              static_cast<int>(EngineFactory::EngineType::SAMPLER_SLICER),
                              CPUClass::Medium, 32) {}  // 32 voices for slice polyphony
    
    // IEngine interface
    const char* getName() const override { return "SamplerSlicer"; }
    const char* getShortName() const override { return "SLIC"; }
    
    void setParam(int paramID, float v01) override {
        BaseEngine::setParam(paramID, v01);  // Handle common parameters
        
        switch (static_cast<EngineParamID>(paramID)) {
            case EngineParamID::HARMONICS:
                // HARMONICS controls slice detection sensitivity
                sensitivity_ = v01;  // 0.0 to 1.0
                if (autoDetect_) {
                    detectSlices();
                }
                for (auto& voice : voices_) {
                    voice->setSensitivity(sensitivity_);
                }
                break;
                
            case EngineParamID::TIMBRE:
                // TIMBRE controls slice crossfade amount
                xFade_ = v01;  // 0.0 to 1.0 (maps to 0-10ms)
                for (auto& voice : voices_) {
                    voice->setXFade(xFade_);
                }
                break;
                
            case EngineParamID::MORPH:
                // MORPH controls follow-action probability
                followAction_ = v01;  // 0.0 to 1.0
                for (auto& voice : voices_) {
                    voice->setFollowAction(followAction_);
                }
                break;
                
            default:
                break;
        }
    }
    
    // Sample and slice management
    bool loadLoop(const std::string& filePath) {
        // Pre-allocate sample buffer if not already done (avoids runtime allocation)
        if (!sampleBuffer_) {
            sampleBuffer_ = std::make_shared<Sample::SampleBuffer>();
        }
        
        if (sampleBuffer_ && sampleBuffer_->load(filePath)) {
            detectSlices();
            return true;
        }
        return false;
    }
    
    // Error-aware version using new error handling system
    EtherSynth::Result<void> loadLoopWithErrorHandling(const std::string& filePath) {
        if (filePath.empty()) {
            ETHER_ERROR_MSG(EtherSynth::ErrorCode::INVALID_PARAMETER, "Empty file path");
            return EtherSynth::ErrorCode::INVALID_PARAMETER;
        }
        
        try {
            sampleBuffer_ = std::make_shared<Sample::SampleBuffer>();
            if (!sampleBuffer_) {
                ETHER_ERROR(EtherSynth::ErrorCode::OUT_OF_MEMORY);
                return EtherSynth::ErrorCode::OUT_OF_MEMORY;
            }
            
            if (!sampleBuffer_->load(filePath)) {
                ETHER_ERROR_MSG(EtherSynth::ErrorCode::SAMPLE_LOAD_FAILED, filePath.c_str());
                return EtherSynth::ErrorCode::SAMPLE_LOAD_FAILED;
            }
            
            detectSlices();
            return EtherSynth::Result<void>(); // Success
            
        } catch (...) {
            ETHER_CRITICAL(EtherSynth::ErrorCode::UNKNOWN_ERROR);
            return EtherSynth::ErrorCode::UNKNOWN_ERROR;
        }
    }
    
    void setDetectMode(SamplerSlicer::DetectMode mode) {
        detectMode_ = mode;
        if (autoDetect_) {
            detectSlices();
        }
    }
    
    void detectSlices() {
        if (!sampleBuffer_ || !sampleBuffer_->isLoaded()) return;
        
        slices_.clear();
        
        // Get sample data for analysis (simplified - would need better access)
        const auto& info = sampleBuffer_->getInfo();
        std::vector<size_t> slicePoints;
        
        switch (detectMode_) {
            case SamplerSlicer::DetectMode::TRANSIENT:
                // Simplified transient detection - would need actual sample data access
                for (int i = 0; i < 25; ++i) {
                    size_t pos = (info.totalFrames * i) / 25;
                    slicePoints.push_back(pos);
                }
                break;
                
            case SamplerSlicer::DetectMode::GRID:
                slicePoints = SamplerSlicer::SliceDetector::detectGrid(info.totalFrames, 16);
                break;
                
            case SamplerSlicer::DetectMode::MANUAL:
                // Start with single slice - user can add more manually
                slicePoints.push_back(0);
                slicePoints.push_back(info.totalFrames);
                break;
        }
        
        // Create slices from detected points
        for (size_t i = 0; i < slicePoints.size() - 1 && i < 24; ++i) {
            SamplerSlicer::Slice slice;
            slice.startFrame = slicePoints[i];
            slice.endFrame = slicePoints[i + 1];
            slices_.push_back(slice);
        }
        
        // Update all voices with new slice configuration
        for (auto& voice : voices_) {
            voice->setSampleBuffer(sampleBuffer_);
            voice->setSlicesConfig(&slices_);
        }
    }
    
    void addSlice(size_t position) {
        // Manual slice addition at specified position
        if (slices_.size() >= 25) return;
        
        // Find insertion point and split existing slice
        for (auto& slice : slices_) {
            if (position > slice.startFrame && position < slice.endFrame) {
                size_t oldEnd = slice.endFrame;
                slice.endFrame = position;
                
                // Create new slice for the remainder
                SamplerSlicer::Slice newSlice;
                newSlice.startFrame = position;
                newSlice.endFrame = oldEnd;
                slices_.push_back(newSlice);
                break;
            }
        }
    }
    
    // Slice access
    SamplerSlicer::Slice& getSlice(int index) { return slices_[std::clamp(index, 0, static_cast<int>(slices_.size() - 1))]; }
    const SamplerSlicer::Slice& getSlice(int index) const { return slices_[std::clamp(index, 0, static_cast<int>(slices_.size() - 1))]; }
    size_t getSliceCount() const { return slices_.size(); }
    
    // Parameter metadata
    int getParameterCount() const override { return 6; }
    
    const ParameterInfo* getParameterInfo(int index) const override {
        static const ParameterInfo params[] = {
            {static_cast<int>(EngineParamID::HARMONICS), "Sensitivity", "", 0.5f, 0.0f, 1.0f, false, 0, "Detect"},
            {static_cast<int>(EngineParamID::TIMBRE), "X-Fade", "ms", 0.0f, 0.0f, 1.0f, false, 0, "Slice"},
            {static_cast<int>(EngineParamID::MORPH), "Follow", "", 0.0f, 0.0f, 1.0f, false, 0, "Action"},
            {static_cast<int>(EngineParamID::LPF_CUTOFF), "Filter", "Hz", 0.8f, 0.0f, 1.0f, false, 0, "Filter"},
            {static_cast<int>(EngineParamID::DRIVE), "Drive", "", 0.1f, 0.0f, 1.0f, false, 0, "Channel"},
            {static_cast<int>(EngineParamID::VOLUME), "Level", "dB", 0.8f, 0.0f, 1.0f, false, 0, "Output"}
        };
        
        return (index >= 0 && index < 6) ? &params[index] : nullptr;
    }
    
private:
    std::shared_ptr<Sample::SampleBuffer> sampleBuffer_;
    std::vector<SamplerSlicer::Slice> slices_;
    
    // Detection settings
    SamplerSlicer::DetectMode detectMode_ = SamplerSlicer::DetectMode::TRANSIENT;
    bool autoDetect_ = true;
    
    // Engine-specific parameters (hero macros)
    float sensitivity_ = 0.5f;   // Transient detection sensitivity
    float xFade_ = 0.0f;         // Slice crossfade amount (0-10ms)
    float followAction_ = 0.0f;  // Follow-action probability
};