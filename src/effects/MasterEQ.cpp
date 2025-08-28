#include "MasterEQ.h"
#include <algorithm>
#include <cstring>

namespace EtherSynth {

MasterEQ::MasterEQ() 
    : sampleRate_(48000.0f), inputLevel_(0.0f), outputLevel_(0.0f), 
      gainReduction_(0.0f), autoGainCompensation_(1.0f), 
      fftBufferIndex_(0), spectrumNeedsUpdate_(false) {
    
    // Initialize FFT window (Hann window)
    initializeFFTWindow();
    
    // Initialize smoothing parameters
    inputGainSmooth_.setSampleRate(sampleRate_);
    outputGainSmooth_.setSampleRate(sampleRate_);
    inputGainSmooth_.setSmoothing(10.0f); // 10ms smoothing
    outputGainSmooth_.setSmoothing(10.0f);
    
    for (auto& smooth : bandGainSmooth_) {
        smooth.setSampleRate(sampleRate_);
        smooth.setSmoothing(10.0f);
    }
    
    // Initialize peak followers
    inputPeak_.setSampleRate(sampleRate_);
    outputPeak_.setSampleRate(sampleRate_);
    inputPeak_.setAttackTime(0.001f);  // 1ms attack
    inputPeak_.setReleaseTime(0.100f); // 100ms release
    outputPeak_.setAttackTime(0.001f);
    outputPeak_.setReleaseTime(0.100f);
    
    // Calculate initial filter coefficients
    for (int i = 0; i < static_cast<int>(Band::COUNT); ++i) {
        updateCoefficients(static_cast<Band>(i));
    }
    
    // Initialize spectrum data frequencies
    for (int i = 0; i < SpectrumData::NUM_BINS; ++i) {
        float normalizedFreq = static_cast<float>(i) / SpectrumData::NUM_BINS;
        spectrumData_.frequency[i] = normalizedFreq * sampleRate_ * 0.5f;
    }
}

void MasterEQ::setSettings(const EQSettings& settings) {
    settings_ = settings;
    
    // Clamp values to valid ranges
    settings_.inputGain = std::clamp(settings_.inputGain, -24.0f, 24.0f);
    settings_.outputGain = std::clamp(settings_.outputGain, -24.0f, 24.0f);
    settings_.analysisSmooth = std::clamp(settings_.analysisSmooth, 0.0f, 0.99f);
    
    // Validate and clamp band settings
    for (auto& band : settings_.bands) {
        band.frequency = std::clamp(band.frequency, 20.0f, 20000.0f);
        band.gain = std::clamp(band.gain, -24.0f, 24.0f);
        band.q = std::clamp(band.q, 0.1f, 30.0f);
    }
    
    // Update all filter coefficients
    for (int i = 0; i < static_cast<int>(Band::COUNT); ++i) {
        updateCoefficients(static_cast<Band>(i));
    }
    
    updateAutoGain();
}

void MasterEQ::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    
    // Update smoothing parameters
    inputGainSmooth_.setSampleRate(sampleRate);
    outputGainSmooth_.setSampleRate(sampleRate);
    for (auto& smooth : bandGainSmooth_) {
        smooth.setSampleRate(sampleRate);
    }
    
    // Update peak followers
    inputPeak_.setSampleRate(sampleRate);
    outputPeak_.setSampleRate(sampleRate);
    
    // Recalculate all filter coefficients
    for (int i = 0; i < static_cast<int>(Band::COUNT); ++i) {
        updateCoefficients(static_cast<Band>(i));
    }
    
    // Update spectrum frequency array
    for (int i = 0; i < SpectrumData::NUM_BINS; ++i) {
        float normalizedFreq = static_cast<float>(i) / SpectrumData::NUM_BINS;
        spectrumData_.frequency[i] = normalizedFreq * sampleRate * 0.5f;
    }
}

void MasterEQ::setBandSettings(Band band, const BandSettings& bandSettings) {
    size_t bandIndex = static_cast<size_t>(band);
    settings_.bands[bandIndex] = bandSettings;
    
    // Clamp values
    settings_.bands[bandIndex].frequency = std::clamp(bandSettings.frequency, 20.0f, 20000.0f);
    settings_.bands[bandIndex].gain = std::clamp(bandSettings.gain, -24.0f, 24.0f);
    settings_.bands[bandIndex].q = std::clamp(bandSettings.q, 0.1f, 30.0f);
    
    updateCoefficients(band);
    updateAutoGain();
}

void MasterEQ::setBandGain(Band band, float gainDb) {
    size_t bandIndex = static_cast<size_t>(band);
    settings_.bands[bandIndex].gain = std::clamp(gainDb, -24.0f, 24.0f);
    updateCoefficients(band);
    updateAutoGain();
}

float MasterEQ::process(float input) {
    if (!settings_.enabled) {
        return input;
    }
    
    // Update level monitoring
    inputLevel_ = inputPeak_.process(std::abs(input));
    
    // Apply input gain
    inputGainSmooth_.setTarget(settings_.inputGain);
    float signal = input * dbToLinear(inputGainSmooth_.process());
    
    // Process through each enabled band
    for (int i = 0; i < static_cast<int>(Band::COUNT); ++i) {
        Band band = static_cast<Band>(i);
        const auto& bandSettings = settings_.bands[i];
        
        if (bandSettings.enabled) {
            // Check for solo mode
            bool shouldProcess = true;
            if (bandSettings.solo) {
                // If any band is soloed, only process soloed bands
                shouldProcess = true;
            } else {
                // Check if any band is soloed
                bool hasSolo = false;
                for (const auto& b : settings_.bands) {
                    if (b.solo) {
                        hasSolo = true;
                        break;
                    }
                }
                shouldProcess = !hasSolo;
            }
            
            if (shouldProcess) {
                signal = processBand(band, signal);
            }
        }
    }
    
    // Apply auto-gain compensation
    if (settings_.autoGain) {
        signal *= autoGainCompensation_;
    }
    
    // Apply output gain
    outputGainSmooth_.setTarget(settings_.outputGain);
    signal *= dbToLinear(outputGainSmooth_.process());
    
    // Update output level
    outputLevel_ = outputPeak_.process(std::abs(signal));
    
    // Calculate gain reduction for metering
    gainReduction_ = linearToDb(std::abs(signal) / std::max(std::abs(input), 1e-10f));
    
    // Add to spectrum analysis buffer
    if (settings_.spectrumEnabled && fftBufferIndex_ < SpectrumData::FFT_SIZE) {
        fftInputBuffer_[fftBufferIndex_++] = signal;
        if (fftBufferIndex_ >= SpectrumData::FFT_SIZE) {
            spectrumNeedsUpdate_ = true;
            fftBufferIndex_ = 0;
        }
    }
    
    return signal;
}

void MasterEQ::processBlock(const float* input, float* output, int blockSize) {
    if (!settings_.enabled) {
        if (input != output) {
            std::memcpy(output, input, blockSize * sizeof(float));
        }
        return;
    }
    
    // SIMD-optimized block processing where possible
    for (int i = 0; i < blockSize; ++i) {
        output[i] = process(input[i]);
    }
}

void MasterEQ::processStereo(const float* inputL, const float* inputR,
                           float* outputL, float* outputR, int blockSize) {
    // Process each channel independently
    processBlock(inputL, outputL, blockSize);
    processBlock(inputR, outputR, blockSize);
}

void MasterEQ::loadPreset(Preset preset) {
    switch (preset) {
        case Preset::FLAT:
            // Reset all bands to flat response
            for (auto& band : settings_.bands) {
                band.gain = 0.0f;
                band.enabled = false;
            }
            break;
            
        case Preset::WARM:
            setBandGain(Band::SUB, 1.5f);      // Subtle low end
            setBandGain(Band::LOW, 2.0f);      // Warm lows
            setBandGain(Band::HIGH_MID, -1.0f); // Reduce harshness
            setBandGain(Band::AIR, 1.0f);      // Gentle air
            break;
            
        case Preset::BRIGHT:
            setBandGain(Band::MID, 1.0f);      // Presence
            setBandGain(Band::HIGH_MID, 2.0f); // Brightness
            setBandGain(Band::HIGH, 3.0f);     // Crisp highs
            setBandGain(Band::AIR, 2.0f);      // Air
            break;
            
        case Preset::PUNCHY:
            setBandGain(Band::SUB, -2.0f);     // Tight low end
            setBandGain(Band::LOW, 3.0f);      // Punch
            setBandGain(Band::LOW_MID, 1.0f);  // Body
            setBandGain(Band::HIGH_MID, 2.0f); // Presence
            break;
            
        case Preset::VOCAL:
            setBandGain(Band::LOW, -2.0f);     // Remove mud
            setBandGain(Band::LOW_MID, -1.0f); // Reduce boxiness
            setBandGain(Band::MID, 2.0f);      // Vocal clarity
            setBandGain(Band::HIGH_MID, 3.0f); // Intelligibility
            setBandGain(Band::AIR, 1.5f);      // Breath
            break;
            
        case Preset::MASTER:
            setBandGain(Band::SUB, -1.0f);     // Control rumble
            setBandGain(Band::LOW, 1.0f);      // Foundation
            setBandGain(Band::HIGH_MID, 1.5f); // Definition
            setBandGain(Band::AIR, 2.0f);      // Polish
            break;
            
        case Preset::VINTAGE:
            setBandGain(Band::LOW, 2.0f);      // Warm lows
            setBandGain(Band::LOW_MID, 1.0f);  // Body
            setBandGain(Band::HIGH_MID, -2.0f); // Reduce digital harshness
            setBandGain(Band::HIGH, -1.0f);    // Vintage rolloff
            break;
            
        case Preset::MODERN:
            setBandGain(Band::SUB, -1.0f);     // Tight
            setBandGain(Band::LOW_MID, -0.5f); // Clean
            setBandGain(Band::HIGH_MID, 2.0f); // Definition
            setBandGain(Band::HIGH, 2.5f);     // Modern brightness
            setBandGain(Band::AIR, 3.0f);      // Digital polish
            break;
            
        default:
            break;
    }
    
    // Enable all modified bands
    for (auto& band : settings_.bands) {
        if (std::abs(band.gain) > 0.1f) {
            band.enabled = true;
        }
    }
    
    // Update coefficients and auto-gain
    for (int i = 0; i < static_cast<int>(Band::COUNT); ++i) {
        updateCoefficients(static_cast<Band>(i));
    }
    updateAutoGain();
}

void MasterEQ::updateCoefficients(Band band) {
    size_t bandIndex = static_cast<size_t>(band);
    const auto& bandSettings = settings_.bands[bandIndex];
    
    if (!bandSettings.enabled) {
        // Bypass filter - unity gain
        coeffs_[bandIndex] = BiquadCoeffs(); // Default unity response
        return;
    }
    
    switch (bandSettings.type) {
        case FilterType::BELL:
            calculateBellFilter(band, bandSettings.frequency, bandSettings.gain, bandSettings.q);
            break;
        case FilterType::HIGH_SHELF:
            calculateShelfFilter(band, bandSettings.frequency, bandSettings.gain, bandSettings.q, true);
            break;
        case FilterType::LOW_SHELF:
            calculateShelfFilter(band, bandSettings.frequency, bandSettings.gain, bandSettings.q, false);
            break;
        case FilterType::HIGH_PASS:
            calculateHighPassFilter(band, bandSettings.frequency, bandSettings.q);
            break;
        case FilterType::LOW_PASS:
            calculateLowPassFilter(band, bandSettings.frequency, bandSettings.q);
            break;
        case FilterType::NOTCH:
            calculateNotchFilter(band, bandSettings.frequency, bandSettings.q);
            break;
    }
}

void MasterEQ::calculateBellFilter(Band band, float freq, float gain, float q) {
    size_t bandIndex = static_cast<size_t>(band);
    
    float omega = 2.0f * M_PI * freq / sampleRate_;
    float cosOmega = std::cos(omega);
    float sinOmega = std::sin(omega);
    float A = std::pow(10.0f, gain / 40.0f); // sqrt of linear gain
    float alpha = sinOmega / (2.0f * q);
    
    float b0 = 1.0f + alpha * A;
    float b1 = -2.0f * cosOmega;
    float b2 = 1.0f - alpha * A;
    float a0 = 1.0f + alpha / A;
    float a1 = -2.0f * cosOmega;
    float a2 = 1.0f - alpha / A;
    
    // Normalize by a0
    coeffs_[bandIndex].b0 = b0 / a0;
    coeffs_[bandIndex].b1 = b1 / a0;
    coeffs_[bandIndex].b2 = b2 / a0;
    coeffs_[bandIndex].a1 = a1 / a0;
    coeffs_[bandIndex].a2 = a2 / a0;
}

void MasterEQ::calculateShelfFilter(Band band, float freq, float gain, float q, bool highShelf) {
    size_t bandIndex = static_cast<size_t>(band);
    
    float omega = 2.0f * M_PI * freq / sampleRate_;
    float cosOmega = std::cos(omega);
    float sinOmega = std::sin(omega);
    float A = std::pow(10.0f, gain / 40.0f);
    float S = 1.0f; // Shelf slope parameter
    float beta = std::sqrt(A) / q;
    
    float b0, b1, b2, a0, a1, a2;
    
    if (highShelf) {
        b0 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega + beta * sinOmega);
        b1 = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosOmega);
        b2 = A * ((A + 1.0f) + (A - 1.0f) * cosOmega - beta * sinOmega);
        a0 = (A + 1.0f) - (A - 1.0f) * cosOmega + beta * sinOmega;
        a1 = 2.0f * ((A - 1.0f) - (A + 1.0f) * cosOmega);
        a2 = (A + 1.0f) - (A - 1.0f) * cosOmega - beta * sinOmega;
    } else {
        b0 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega + beta * sinOmega);
        b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosOmega);
        b2 = A * ((A + 1.0f) - (A - 1.0f) * cosOmega - beta * sinOmega);
        a0 = (A + 1.0f) + (A - 1.0f) * cosOmega + beta * sinOmega;
        a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosOmega);
        a2 = (A + 1.0f) + (A - 1.0f) * cosOmega - beta * sinOmega;
    }
    
    // Normalize by a0
    coeffs_[bandIndex].b0 = b0 / a0;
    coeffs_[bandIndex].b1 = b1 / a0;
    coeffs_[bandIndex].b2 = b2 / a0;
    coeffs_[bandIndex].a1 = a1 / a0;
    coeffs_[bandIndex].a2 = a2 / a0;
}

void MasterEQ::calculateHighPassFilter(Band band, float freq, float q) {
    size_t bandIndex = static_cast<size_t>(band);
    
    float omega = 2.0f * M_PI * freq / sampleRate_;
    float cosOmega = std::cos(omega);
    float sinOmega = std::sin(omega);
    float alpha = sinOmega / (2.0f * q);
    
    float b0 = (1.0f + cosOmega) / 2.0f;
    float b1 = -(1.0f + cosOmega);
    float b2 = (1.0f + cosOmega) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosOmega;
    float a2 = 1.0f - alpha;
    
    // Normalize by a0
    coeffs_[bandIndex].b0 = b0 / a0;
    coeffs_[bandIndex].b1 = b1 / a0;
    coeffs_[bandIndex].b2 = b2 / a0;
    coeffs_[bandIndex].a1 = a1 / a0;
    coeffs_[bandIndex].a2 = a2 / a0;
}

void MasterEQ::calculateLowPassFilter(Band band, float freq, float q) {
    size_t bandIndex = static_cast<size_t>(band);
    
    float omega = 2.0f * M_PI * freq / sampleRate_;
    float cosOmega = std::cos(omega);
    float sinOmega = std::sin(omega);
    float alpha = sinOmega / (2.0f * q);
    
    float b0 = (1.0f - cosOmega) / 2.0f;
    float b1 = 1.0f - cosOmega;
    float b2 = (1.0f - cosOmega) / 2.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosOmega;
    float a2 = 1.0f - alpha;
    
    // Normalize by a0
    coeffs_[bandIndex].b0 = b0 / a0;
    coeffs_[bandIndex].b1 = b1 / a0;
    coeffs_[bandIndex].b2 = b2 / a0;
    coeffs_[bandIndex].a1 = a1 / a0;
    coeffs_[bandIndex].a2 = a2 / a0;
}

void MasterEQ::calculateNotchFilter(Band band, float freq, float q) {
    size_t bandIndex = static_cast<size_t>(band);
    
    float omega = 2.0f * M_PI * freq / sampleRate_;
    float cosOmega = std::cos(omega);
    float sinOmega = std::sin(omega);
    float alpha = sinOmega / (2.0f * q);
    
    float b0 = 1.0f;
    float b1 = -2.0f * cosOmega;
    float b2 = 1.0f;
    float a0 = 1.0f + alpha;
    float a1 = -2.0f * cosOmega;
    float a2 = 1.0f - alpha;
    
    // Normalize by a0
    coeffs_[bandIndex].b0 = b0 / a0;
    coeffs_[bandIndex].b1 = b1 / a0;
    coeffs_[bandIndex].b2 = b2 / a0;
    coeffs_[bandIndex].a1 = a1 / a0;
    coeffs_[bandIndex].a2 = a2 / a0;
}

void MasterEQ::updateAutoGain() {
    if (!settings_.autoGain) {
        autoGainCompensation_ = 1.0f;
        return;
    }
    
    // Calculate cumulative gain at key frequencies and compensate
    float totalGain = 0.0f;
    int sampleCount = 0;
    
    // Sample frequency response at key points
    std::array<float, 10> testFreqs = {
        100.0f, 250.0f, 500.0f, 1000.0f, 2000.0f,
        4000.0f, 8000.0f, 12000.0f, 16000.0f, 18000.0f
    };
    
    for (float freq : testFreqs) {
        float response = getFrequencyResponse(freq);
        totalGain += linearToDb(response);
        sampleCount++;
    }
    
    float averageGain = totalGain / sampleCount;
    autoGainCompensation_ = dbToLinear(-averageGain * 0.5f); // 50% compensation
}

float MasterEQ::getFrequencyResponse(float frequency) const {
    float response = 1.0f;
    
    for (int i = 0; i < static_cast<int>(Band::COUNT); ++i) {
        const auto& band = settings_.bands[i];
        if (!band.enabled) continue;
        
        const auto& c = coeffs_[i];
        
        // Calculate frequency response of biquad filter
        float omega = 2.0f * M_PI * frequency / sampleRate_;
        std::complex<float> z = std::exp(std::complex<float>(0.0f, omega));
        std::complex<float> z2 = z * z;
        
        std::complex<float> num = c.b0 + c.b1 * z + c.b2 * z2;
        std::complex<float> den = 1.0f + c.a1 * z + c.a2 * z2;
        
        std::complex<float> H = num / den;
        response *= std::abs(H);
    }
    
    return response;
}

void MasterEQ::initializeFFTWindow() {
    // Generate Hann window for spectral analysis
    for (int i = 0; i < SpectrumData::FFT_SIZE; ++i) {
        float phase = 2.0f * M_PI * i / (SpectrumData::FFT_SIZE - 1);
        fftWindow_[i] = 0.5f * (1.0f - std::cos(phase));
    }
}

void MasterEQ::updateSpectrum() {
    if (!spectrumNeedsUpdate_ || !settings_.spectrumEnabled) {
        return;
    }
    
    performFFTAnalysis();
    spectrumNeedsUpdate_ = false;
    spectrumData_.dataReady = true;
}

void MasterEQ::performFFTAnalysis() {
    // Apply window function
    std::array<float, SpectrumData::FFT_SIZE> windowedInput;
    for (int i = 0; i < SpectrumData::FFT_SIZE; ++i) {
        windowedInput[i] = fftInputBuffer_[i] * fftWindow_[i];
    }
    
    // Simple DFT for spectrum analysis (could be optimized with FFT library)
    for (int k = 0; k < SpectrumData::NUM_BINS; ++k) {
        float real = 0.0f, imag = 0.0f;
        
        for (int n = 0; n < SpectrumData::FFT_SIZE; ++n) {
            float angle = -2.0f * M_PI * k * n / SpectrumData::FFT_SIZE;
            real += windowedInput[n] * std::cos(angle);
            imag += windowedInput[n] * std::sin(angle);
        }
        
        float magnitude = std::sqrt(real * real + imag * imag);
        float phase = std::atan2(imag, real);
        
        // Apply smoothing
        float alpha = settings_.analysisSmooth;
        spectrumData_.magnitude[k] = alpha * spectrumData_.magnitude[k] + (1.0f - alpha) * magnitude;
        spectrumData_.phase[k] = phase;
    }
    
    // Find peak frequency and RMS level
    float maxMagnitude = 0.0f;
    int peakBin = 0;
    float rmsSum = 0.0f;
    
    for (int i = 1; i < SpectrumData::NUM_BINS; ++i) { // Skip DC bin
        if (spectrumData_.magnitude[i] > maxMagnitude) {
            maxMagnitude = spectrumData_.magnitude[i];
            peakBin = i;
        }
        rmsSum += spectrumData_.magnitude[i] * spectrumData_.magnitude[i];
    }
    
    spectrumData_.peakFrequency = spectrumData_.frequency[peakBin];
    spectrumData_.rmsLevel = std::sqrt(rmsSum / (SpectrumData::NUM_BINS - 1));
}

bool MasterEQ::hasActiveEQ() const {
    for (const auto& band : settings_.bands) {
        if (band.enabled && std::abs(band.gain) > 0.01f) {
            return true;
        }
    }
    return false;
}

void MasterEQ::reset() {
    // Reset all filter states
    for (auto& state : states_) {
        state.reset();
    }
    
    // Reset level monitoring
    inputLevel_ = outputLevel_ = gainReduction_ = 0.0f;
    inputPeak_.reset();
    outputPeak_.reset();
    
    // Reset spectrum analysis
    fftBufferIndex_ = 0;
    spectrumNeedsUpdate_ = false;
    spectrumData_.dataReady = false;
    
    std::fill(fftInputBuffer_.begin(), fftInputBuffer_.end(), 0.0f);
    std::fill(spectrumData_.magnitude.begin(), spectrumData_.magnitude.end(), 0.0f);
    std::fill(spectrumData_.phase.begin(), spectrumData_.phase.end(), 0.0f);
}

//=============================================================================
// Utility functions
//=============================================================================

const char* getBandName(MasterEQ::Band band) {
    switch (band) {
        case MasterEQ::Band::SUB: return "Sub";
        case MasterEQ::Band::LOW: return "Low";
        case MasterEQ::Band::LOW_MID: return "Low Mid";
        case MasterEQ::Band::MID: return "Mid";
        case MasterEQ::Band::HIGH_MID: return "High Mid";
        case MasterEQ::Band::HIGH: return "High";
        case MasterEQ::Band::AIR: return "Air";
        default: return "Unknown";
    }
}

const char* getFilterTypeName(MasterEQ::FilterType type) {
    switch (type) {
        case MasterEQ::FilterType::BELL: return "Bell";
        case MasterEQ::FilterType::HIGH_SHELF: return "High Shelf";
        case MasterEQ::FilterType::LOW_SHELF: return "Low Shelf";
        case MasterEQ::FilterType::HIGH_PASS: return "High Pass";
        case MasterEQ::FilterType::LOW_PASS: return "Low Pass";
        case MasterEQ::FilterType::NOTCH: return "Notch";
        default: return "Unknown";
    }
}

const char* getPresetName(MasterEQ::Preset preset) {
    switch (preset) {
        case MasterEQ::Preset::FLAT: return "Flat";
        case MasterEQ::Preset::WARM: return "Warm";
        case MasterEQ::Preset::BRIGHT: return "Bright";
        case MasterEQ::Preset::PUNCHY: return "Punchy";
        case MasterEQ::Preset::VOCAL: return "Vocal";
        case MasterEQ::Preset::MASTER: return "Master";
        case MasterEQ::Preset::VINTAGE: return "Vintage";
        case MasterEQ::Preset::MODERN: return "Modern";
        default: return "Unknown";
    }
}

float getBandCenterFrequency(MasterEQ::Band band) {
    switch (band) {
        case MasterEQ::Band::SUB: return 50.0f;
        case MasterEQ::Band::LOW: return 150.0f;
        case MasterEQ::Band::LOW_MID: return 500.0f;
        case MasterEQ::Band::MID: return 1500.0f;
        case MasterEQ::Band::HIGH_MID: return 4000.0f;
        case MasterEQ::Band::HIGH: return 10000.0f;
        case MasterEQ::Band::AIR: return 17000.0f;
        default: return 1000.0f;
    }
}

std::pair<float, float> getBandFrequencyRange(MasterEQ::Band band) {
    switch (band) {
        case MasterEQ::Band::SUB: return {20.0f, 80.0f};
        case MasterEQ::Band::LOW: return {80.0f, 250.0f};
        case MasterEQ::Band::LOW_MID: return {250.0f, 800.0f};
        case MasterEQ::Band::MID: return {800.0f, 2500.0f};
        case MasterEQ::Band::HIGH_MID: return {2500.0f, 8000.0f};
        case MasterEQ::Band::HIGH: return {8000.0f, 16000.0f};
        case MasterEQ::Band::AIR: return {16000.0f, 20000.0f};
        default: return {20.0f, 20000.0f};
    }
}

} // namespace EtherSynth