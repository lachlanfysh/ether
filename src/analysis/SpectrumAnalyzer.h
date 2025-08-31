#pragma once
#include <vector>
#include <array>
#include <complex>
#include <cmath>
#include <mutex>
#include <atomic>

/**
 * Real-time Spectrum Analyzer for EtherSynth V1.0
 * 
 * Professional FFT-based frequency analysis for:
 * - Real-time spectrum visualization
 * - Frequency-based parameter automation
 * - Intelligent mix analysis and suggestions
 * - Adaptive effects processing
 * - Hardware integration for 960×320 display
 */

namespace EtherSynth {

class SpectrumAnalyzer {
public:
    static constexpr int FFT_SIZE = 1024;
    static constexpr int BINS = FFT_SIZE / 2;
    static constexpr int BARK_BANDS = 24;      // Perceptual frequency bands
    static constexpr int DISPLAY_BARS = 32;    // For 960px display (30px per bar)
    static constexpr float SAMPLE_RATE = 48000.0f;
    
    // Analysis result structure
    struct SpectrumData {
        std::array<float, BINS> magnitudes;           // Raw FFT magnitudes
        std::array<float, BARK_BANDS> barkBands;      // Perceptual frequency bands
        std::array<float, DISPLAY_BARS> displayBars;  // Display-optimized bars
        
        // Analysis metrics
        float totalEnergy = 0.0f;           // Overall signal energy
        float spectralCentroid = 0.0f;      // Brightness metric
        float spectralSpread = 0.0f;        // Spectral width
        float spectralRolloff = 0.0f;       // Rolloff frequency
        float spectralFlux = 0.0f;          // Change rate
        float fundamentalFreq = 0.0f;       // Detected fundamental
        float rms = 0.0f;                   // RMS level
        float peak = 0.0f;                  // Peak level
        
        // Mix analysis
        float bassEnergy = 0.0f;            // 20-250 Hz
        float midEnergy = 0.0f;             // 250-4000 Hz  
        float highEnergy = 0.0f;            // 4000-20000 Hz
        float lowMidRatio = 0.0f;           // Bass/mid ratio
        float highMidRatio = 0.0f;          // High/mid ratio
        
        bool hasActivity = false;           // Signal present
        uint64_t timestamp = 0;             // Analysis timestamp
    };
    
    // Audio feature detection
    struct AudioFeatures {
        bool hasKick = false;               // Kick drum detected
        bool hasSnare = false;              // Snare detected
        bool hasHiHat = false;              // Hi-hat detected
        bool hasBass = false;               // Bass content detected
        bool hasVocals = false;             // Vocal content detected
        bool isPercussive = false;          // Percussive material
        bool isMelodic = false;             // Melodic content
        bool isNoisy = false;               // Noise/distortion
        
        float tempo = 0.0f;                 // Estimated BPM
        float key = 0.0f;                   // Estimated key (0-11)
        float rhythmStrength = 0.0f;        // Rhythmic regularity
        float harmonicity = 0.0f;           // Harmonic content
    };
    
    SpectrumAnalyzer();
    ~SpectrumAnalyzer();
    
    // Core Analysis
    void processAudioBuffer(const float* buffer, int bufferSize);
    const SpectrumData& getLatestSpectrum() const;
    const AudioFeatures& getAudioFeatures() const;
    
    // Configuration
    void setSampleRate(float sampleRate);
    void setWindowSize(int windowSize);
    void setOverlapRatio(float overlap);
    void setSmoothingFactor(float smoothing);
    
    // Frequency Analysis
    float getFrequencyForBin(int bin) const;
    int getBinForFrequency(float frequency) const;
    float getMagnitudeAtFrequency(float frequency) const;
    
    // Band Analysis  
    float getBandEnergy(float lowFreq, float highFreq) const;
    std::vector<float> getOctaveBands() const;
    std::vector<float> getThirdOctaveBands() const;
    
    // Musical Analysis
    float detectFundamental(float minFreq = 80.0f, float maxFreq = 800.0f) const;
    std::vector<float> detectHarmonics(float fundamental, int maxHarmonics = 8) const;
    float calculateInharmonicity() const;
    
    // Perceptual Analysis
    float calculateLoudness() const;        // Perceptual loudness (phons)
    float calculateSharpness() const;       // Psychoacoustic sharpness
    float calculateRoughness() const;       // Sensory dissonance
    float calculateBrightness() const;      // High-frequency content
    
    // Real-time Features
    void enableRealTimeProcessing(bool enabled);
    void setLatencyMode(bool lowLatency);
    float getProcessingLoad() const;
    
    // Display Integration
    void renderSpectrum(uint32_t* displayBuffer, int width, int height, 
                       uint32_t barColor = 0x00FF6B, uint32_t bgColor = 0x1A1A1A) const;
    void renderSpectrogram(uint32_t* displayBuffer, int width, int height) const;
    void renderBarkBands(uint32_t* displayBuffer, int width, int height) const;
    
    // Hardware Integration
    void mapToHardwareDisplay(uint8_t* ledBuffer, int ledCount) const;
    void triggerHardwareVisualization(int mode) const;
    
private:
    // FFT Implementation
    void performFFT(const std::vector<float>& input, std::vector<std::complex<float>>& output);
    void applyWindow(std::vector<float>& buffer) const;
    void calculateMagnitudes(const std::vector<std::complex<float>>& fftData);
    
    // Analysis Implementation
    void calculateSpectralFeatures();
    void calculateBarkBands();
    void calculateDisplayBars();
    void updateAudioFeatures();
    void detectPercussiveEvents();
    void estimateTempo();
    
    // Bark scale conversion
    float hzToBark(float hz) const;
    float barkToHz(float bark) const;
    std::vector<int> createBarkBandMaps();
    
    // Windowing functions
    std::vector<float> createHannWindow(int size) const;
    std::vector<float> createBlackmanWindow(int size) const;
    
    // Member variables
    mutable std::mutex analysisMutex_;
    std::atomic<bool> realTimeEnabled_;
    std::atomic<float> sampleRate_;
    
    // Analysis buffers
    std::vector<float> inputBuffer_;
    std::vector<float> windowBuffer_;
    std::vector<std::complex<float>> fftBuffer_;
    std::vector<float> magnitudeHistory_;
    
    // Analysis data
    SpectrumData currentSpectrum_;
    SpectrumData smoothedSpectrum_;
    AudioFeatures currentFeatures_;
    
    // Configuration
    int windowSize_ = FFT_SIZE;
    float overlapRatio_ = 0.5f;
    float smoothingFactor_ = 0.8f;
    bool lowLatency_ = false;
    
    // Bark band mapping
    std::vector<int> barkBandStart_;
    std::vector<int> barkBandEnd_;
    std::vector<float> barkBandWeights_;
    
    // Performance monitoring
    mutable std::atomic<float> processingLoad_;
    uint64_t lastProcessTime_;
    
    // Window function cache
    std::vector<float> windowFunction_;
    bool windowCacheValid_ = false;
};

// Utility functions for audio analysis
namespace AudioAnalysisUtils {
    // Frequency conversion
    float noteToFrequency(int midiNote);
    int frequencyToNote(float frequency);
    const char* noteToName(int midiNote);
    
    // Psychoacoustic functions
    float frequencyToMel(float frequency);
    float melToFrequency(float mel);
    float loudnessWeighting(float frequency);
    
    // Musical analysis
    bool isHarmonic(float frequency, float fundamental, float tolerance = 0.02f);
    float calculateDissonance(const std::vector<float>& frequencies, 
                            const std::vector<float>& amplitudes);
    
    // Signal classification
    bool isPercussive(const SpectrumAnalyzer::SpectrumData& spectrum);
    bool isTonal(const SpectrumAnalyzer::SpectrumData& spectrum);
    bool isNoise(const SpectrumAnalyzer::SpectrumData& spectrum);
}

} // namespace EtherSynth