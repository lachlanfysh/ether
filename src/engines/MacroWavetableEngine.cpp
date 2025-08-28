#include "MacroWavetableEngine.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <cmath>
#include <algorithm>

// Static wavetable data initialization
float MacroWavetableEngine::MacroWavetableVoice::WavetableOscillator::wavetables[NUM_WAVETABLES][WAVETABLE_SIZE];
bool MacroWavetableEngine::MacroWavetableVoice::WavetableOscillator::wavetablesInitialized = false;

MacroWavetableEngine::MacroWavetableEngine() {
    std::cout << "MacroWavetable engine created" << std::endl;
    
    // Initialize modulation array
    modulation_.fill(0.0f);
    
    // Initialize vector path with default diamond
    vectorPath_.clearWaypoints();
    vectorPath_.addWaypoint(0.5f, 0.0f); // Top
    vectorPath_.addWaypoint(1.0f, 0.5f); // Right
    vectorPath_.addWaypoint(0.5f, 1.0f); // Bottom
    vectorPath_.addWaypoint(0.0f, 0.5f); // Left
    vectorPath_.buildArcLengthLUT();
    
    // Calculate initial derived parameters
    calculateDerivedParams();
    
    // Set up default parameters for all voices
    updateAllVoices();
}

MacroWavetableEngine::~MacroWavetableEngine() {
    allNotesOff();
    std::cout << "MacroWavetable engine destroyed" << std::endl;
}

void MacroWavetableEngine::noteOn(uint8_t note, float velocity, float aftertouch) {
    MacroWavetableVoice* voice = findFreeVoice();
    if (!voice) {
        voice = stealVoice();
    }
    
    if (voice) {
        voice->noteOn(note, velocity, aftertouch, sampleRate_);
        voiceCounter_++;
        
        std::cout << "MacroWavetable: Note ON " << static_cast<int>(note) 
                  << " vel=" << velocity << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void MacroWavetableEngine::noteOff(uint8_t note) {
    MacroWavetableVoice* voice = findVoice(note);
    if (voice) {
        voice->noteOff();
        
        std::cout << "MacroWavetable: Note OFF " << static_cast<int>(note) 
                  << " voices=" << getActiveVoiceCount() << std::endl;
    }
}

void MacroWavetableEngine::setAftertouch(uint8_t note, float aftertouch) {
    MacroWavetableVoice* voice = findVoice(note);
    if (voice) {
        voice->setAftertouch(aftertouch);
    }
}

void MacroWavetableEngine::allNotesOff() {
    for (auto& voice : voices_) {
        voice.noteOff();
    }
}

void MacroWavetableEngine::processAudio(EtherAudioBuffer& outputBuffer) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Update vector path if latched
    if (vectorPath_.latched) {
        float deltaTime = BUFFER_SIZE / sampleRate_;
        updateVectorPath(deltaTime);
    }
    
    // Clear output buffer
    for (auto& frame : outputBuffer) {
        frame.left = 0.0f;
        frame.right = 0.0f;
    }
    
    // Process all active voices
    size_t activeVoices = 0;
    for (auto& voice : voices_) {
        if (voice.isActive()) {
            activeVoices++;
            
            for (size_t i = 0; i < BUFFER_SIZE; i++) {
                AudioFrame voiceFrame = voice.processSample();
                outputBuffer[i] += voiceFrame;
            }
        }
    }
    
    // Apply voice scaling to prevent clipping
    if (activeVoices > 1) {
        float scale = 0.8f / std::sqrt(static_cast<float>(activeVoices));
        for (auto& frame : outputBuffer) {
            frame = frame * scale;
        }
    }
    
    // Update CPU usage
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    float processingTime = duration.count() / 1000.0f;
    updateCPUUsage(processingTime);
}

void MacroWavetableEngine::setHarmonics(float harmonics) {
    harmonics_ = std::clamp(harmonics, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroWavetableEngine::setTimbre(float timbre) {
    timbre_ = std::clamp(timbre, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroWavetableEngine::setMorph(float morph) {
    morph_ = std::clamp(morph, 0.0f, 1.0f);
    
    if (vectorPath_.enabled) {
        vectorPath_.position = morph;
        calculateDerivedParams();
        updateAllVoices();
    }
}

void MacroWavetableEngine::calculateDerivedParams() {
    // HARMONICS: wavetable position scan (band-limited interpolation)
    wavetablePosition_ = mapWavetablePosition(harmonics_);
    
    // TIMBRE: formant shift −6 → +6 st + spectral tilt ±3 dB
    formantShift_ = mapFormantShift(timbre_);
    spectralTilt_ = mapSpectralTilt(timbre_);
    
    // MORPH: Vector Path position
    if (vectorPath_.enabled) {
        auto pathPoint = vectorPath_.interpolatePosition(vectorPath_.position);
        currentBlend_ = cornerSources_.calculateWeights(pathPoint.x, pathPoint.y);
    } else {
        // Default to center blend when Vector Path disabled
        currentBlend_ = cornerSources_.calculateWeights(0.5f, 0.5f);
    }
}

void MacroWavetableEngine::updateVectorPath(float deltaTime) {
    if (!vectorPath_.latched) return;
    
    // Update phase for automatic traversal
    vectorPath_.phase += vectorPath_.rate * deltaTime;
    if (vectorPath_.phase >= 1.0f) {
        vectorPath_.phase -= 1.0f;
    }
    
    // Update position along path
    vectorPath_.position = vectorPath_.phase;
    calculateDerivedParams();
    updateAllVoices();
}

float MacroWavetableEngine::mapWavetablePosition(float harmonics) const {
    // Direct mapping to wavetable position
    return harmonics;
}

float MacroWavetableEngine::mapFormantShift(float timbre) const {
    // Formant shift: -6 → +6 semitones
    return (timbre - 0.5f) * 12.0f; // -6 to +6 st
}

float MacroWavetableEngine::mapSpectralTilt(float timbre) const {
    // Spectral tilt: ±3 dB (separate from formant for more control)
    // Use upper half of timbre range for tilt
    return (timbre - 0.5f) * 6.0f; // ±3 dB
}

// Vector Path Implementation
void MacroWavetableEngine::VectorPath::addWaypoint(float x, float y) {
    VectorPathPoint point;
    point.x = std::clamp(x, 0.0f, 1.0f);
    point.y = std::clamp(y, 0.0f, 1.0f);
    waypoints.push_back(point);
    buildArcLengthLUT();
}

void MacroWavetableEngine::VectorPath::removeWaypoint(int index) {
    if (index >= 0 && index < static_cast<int>(waypoints.size()) && waypoints.size() > 1) {
        waypoints.erase(waypoints.begin() + index);
        buildArcLengthLUT();
    }
}

void MacroWavetableEngine::VectorPath::clearWaypoints() {
    waypoints.clear();
    arcLengthLUT.clear();
}

MacroWavetableEngine::VectorPathPoint MacroWavetableEngine::VectorPath::interpolatePosition(float t) const {
    if (waypoints.empty()) {
        return {0.5f, 0.5f};
    }
    
    if (waypoints.size() == 1) {
        return waypoints[0];
    }
    
    // Use uniform position if LUT is available
    float uniformT = getUniformPosition(t);
    
    // Catmull-Rom spline interpolation
    float scaledT = uniformT * static_cast<float>(waypoints.size());
    int segmentIndex = static_cast<int>(scaledT);
    float localT = scaledT - segmentIndex;
    
    // Get control points (with wrapping for closed path)
    int p0Index = (segmentIndex - 1 + waypoints.size()) % waypoints.size();
    int p1Index = segmentIndex % waypoints.size();
    int p2Index = (segmentIndex + 1) % waypoints.size();
    int p3Index = (segmentIndex + 2) % waypoints.size();
    
    return catmullRomInterp(waypoints[p0Index], waypoints[p1Index], 
                           waypoints[p2Index], waypoints[p3Index], localT);
}

MacroWavetableEngine::VectorPathPoint MacroWavetableEngine::VectorPath::catmullRomInterp(
    const VectorPathPoint& p0, const VectorPathPoint& p1,
    const VectorPathPoint& p2, const VectorPathPoint& p3, float t) const {
    
    float t2 = t * t;
    float t3 = t2 * t;
    
    VectorPathPoint result;
    result.x = 0.5f * ((2.0f * p1.x) +
                      (-p0.x + p2.x) * t +
                      (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
                      (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
    
    result.y = 0.5f * ((2.0f * p1.y) +
                      (-p0.y + p2.y) * t +
                      (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
                      (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
    
    return result;
}

void MacroWavetableEngine::VectorPath::buildArcLengthLUT() {
    arcLengthLUT.clear();
    
    if (waypoints.size() < 2) {
        return;
    }
    
    const int NUM_SAMPLES = 1000;
    arcLengthLUT.resize(NUM_SAMPLES + 1);
    
    arcLengthLUT[0] = 0.0f;
    VectorPathPoint prevPoint = interpolatePosition(0.0f);
    
    for (int i = 1; i <= NUM_SAMPLES; i++) {
        float t = static_cast<float>(i) / NUM_SAMPLES;
        VectorPathPoint currentPoint = interpolatePosition(t);
        
        float dx = currentPoint.x - prevPoint.x;
        float dy = currentPoint.y - prevPoint.y;
        float segmentLength = std::sqrt(dx * dx + dy * dy);
        
        arcLengthLUT[i] = arcLengthLUT[i - 1] + segmentLength;
        prevPoint = currentPoint;
    }
}

float MacroWavetableEngine::VectorPath::getUniformPosition(float t) const {
    if (arcLengthLUT.empty() || arcLengthLUT.size() < 2) {
        return t;
    }
    
    float totalLength = arcLengthLUT.back();
    float targetLength = t * totalLength;
    
    // Binary search in LUT
    int low = 0;
    int high = static_cast<int>(arcLengthLUT.size()) - 1;
    
    while (low < high) {
        int mid = (low + high) / 2;
        if (arcLengthLUT[mid] < targetLength) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    
    if (low == 0) return 0.0f;
    if (low >= static_cast<int>(arcLengthLUT.size()) - 1) return 1.0f;
    
    // Linear interpolation between LUT entries
    float alpha = (targetLength - arcLengthLUT[low - 1]) / 
                  (arcLengthLUT[low] - arcLengthLUT[low - 1]);
    
    return (static_cast<float>(low - 1) + alpha) / (arcLengthLUT.size() - 1);
}

// Corner Sources Implementation
MacroWavetableEngine::CornerSources::BlendWeights MacroWavetableEngine::CornerSources::calculateWeights(float x, float y) const {
    BlendWeights weights;
    
    // Equal-power bilinear blend
    weights.wA = (1.0f - x) * (1.0f - y);  // Top-left
    weights.wB = x * (1.0f - y);           // Top-right
    weights.wC = (1.0f - x) * y;           // Bottom-left
    weights.wD = x * y;                    // Bottom-right
    
    // Convert to gain coefficients (equal-power)
    float sum = weights.wA + weights.wB + weights.wC + weights.wD;
    if (sum > 0.0f) {
        weights.gA = std::sqrt(weights.wA / sum);
        weights.gB = std::sqrt(weights.wB / sum);
        weights.gC = std::sqrt(weights.wC / sum);
        weights.gD = std::sqrt(weights.wD / sum);
    } else {
        weights.gA = weights.gB = weights.gC = weights.gD = 0.25f;
    }
    
    return weights;
}

// Voice management and standard methods
MacroWavetableEngine::MacroWavetableVoice* MacroWavetableEngine::findFreeVoice() {
    for (auto& voice : voices_) {
        if (!voice.isActive()) {
            return &voice;
        }
    }
    return nullptr;
}

MacroWavetableEngine::MacroWavetableVoice* MacroWavetableEngine::findVoice(uint8_t note) {
    for (auto& voice : voices_) {
        if (voice.isActive() && voice.getNote() == note) {
            return &voice;
        }
    }
    return nullptr;
}

MacroWavetableEngine::MacroWavetableVoice* MacroWavetableEngine::stealVoice() {
    MacroWavetableVoice* oldest = nullptr;
    uint32_t oldestAge = 0;
    
    for (auto& voice : voices_) {
        if (voice.getAge() > oldestAge) {
            oldestAge = voice.getAge();
            oldest = &voice;
        }
    }
    
    return oldest;
}

void MacroWavetableEngine::updateAllVoices() {
    for (auto& voice : voices_) {
        voice.setWavetableParams(wavetablePosition_, currentBlend_);
        voice.setFormantParams(formantShift_, spectralTilt_);
        voice.setVolume(volume_);
        voice.setEnvelopeParams(attack_, decay_, sustain_, release_);
    }
}

// Parameter interface implementation
void MacroWavetableEngine::setParameter(ParameterID param, float value) {
    switch (param) {
        case ParameterID::HARMONICS:
            setHarmonics(value);
            break;
            
        case ParameterID::TIMBRE:
            setTimbre(value);
            break;
            
        case ParameterID::MORPH:
            setMorph(value);
            break;
            
        case ParameterID::VOLUME:
            volume_ = std::clamp(value, 0.0f, 1.0f);
            updateAllVoices();
            break;
            
        case ParameterID::ATTACK:
            attack_ = std::clamp(value, 0.0005f, 5.0f);
            updateAllVoices();
            break;
            
        case ParameterID::DECAY:
            decay_ = std::clamp(value, 0.001f, 5.0f);
            updateAllVoices();
            break;
            
        case ParameterID::SUSTAIN:
            sustain_ = std::clamp(value, 0.0f, 1.0f);
            updateAllVoices();
            break;
            
        case ParameterID::RELEASE:
            release_ = std::clamp(value, 0.001f, 5.0f);
            updateAllVoices();
            break;
            
        default:
            break;
    }
}

float MacroWavetableEngine::getParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::HARMONICS: return harmonics_;
        case ParameterID::TIMBRE: return timbre_;
        case ParameterID::MORPH: return morph_;
        case ParameterID::VOLUME: return volume_;
        case ParameterID::ATTACK: return attack_;
        case ParameterID::DECAY: return decay_;
        case ParameterID::SUSTAIN: return sustain_;
        case ParameterID::RELEASE: return release_;
        default: return 0.0f;
    }
}

bool MacroWavetableEngine::hasParameter(ParameterID param) const {
    switch (param) {
        case ParameterID::HARMONICS:
        case ParameterID::TIMBRE:
        case ParameterID::MORPH:
        case ParameterID::VOLUME:
        case ParameterID::ATTACK:
        case ParameterID::DECAY:
        case ParameterID::SUSTAIN:
        case ParameterID::RELEASE:
            return true;
        default:
            return false;
    }
}

// Vector Path controls
void MacroWavetableEngine::setVectorPathEnabled(bool enabled) {
    vectorPath_.enabled = enabled;
    calculateDerivedParams();
    updateAllVoices();
}

void MacroWavetableEngine::setVectorPathPosition(float position) {
    vectorPath_.position = std::clamp(position, 0.0f, 1.0f);
    calculateDerivedParams();
    updateAllVoices();
}

void MacroWavetableEngine::setVectorPathLatch(bool latched) {
    vectorPath_.latched = latched;
    if (latched) {
        vectorPath_.phase = vectorPath_.position;
    }
}

void MacroWavetableEngine::setVectorPathRate(float rate) {
    vectorPath_.rate = std::clamp(rate, 0.01f, 10.0f);
}

// Standard SynthEngine methods
size_t MacroWavetableEngine::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.isActive()) {
            count++;
        }
    }
    return count;
}

void MacroWavetableEngine::setVoiceCount(size_t maxVoices) {
    // Voice count is fixed for this implementation
}

void MacroWavetableEngine::savePreset(uint8_t* data, size_t maxSize, size_t& actualSize) const {
    struct PresetData {
        float harmonics, timbre, morph, volume;
        float attack, decay, sustain, release;
        bool vectorPathEnabled, vectorPathLatched;
        float vectorPathRate;
        int numWaypoints;
        // Waypoints would follow in real implementation
    };
    
    PresetData preset = {
        harmonics_, timbre_, morph_, volume_,
        attack_, decay_, sustain_, release_,
        vectorPath_.enabled, vectorPath_.latched, vectorPath_.rate,
        static_cast<int>(vectorPath_.waypoints.size())
    };
    
    actualSize = sizeof(PresetData);
    if (maxSize >= actualSize) {
        memcpy(data, &preset, actualSize);
    }
}

bool MacroWavetableEngine::loadPreset(const uint8_t* data, size_t size) {
    // Simplified preset loading
    return true;
}

void MacroWavetableEngine::setSampleRate(float sampleRate) {
    sampleRate_ = sampleRate;
    updateAllVoices();
}

void MacroWavetableEngine::setBufferSize(size_t bufferSize) {
    bufferSize_ = bufferSize;
}

bool MacroWavetableEngine::supportsModulation(ParameterID target) const {
    return hasParameter(target);
}

void MacroWavetableEngine::setModulation(ParameterID target, float amount) {
    size_t index = static_cast<size_t>(target);
    if (index < modulation_.size()) {
        modulation_[index] = amount;
    }
}

void MacroWavetableEngine::updateCPUUsage(float processingTime) {
    float maxTime = (BUFFER_SIZE / sampleRate_) * 1000.0f;
    cpuUsage_ = std::min(100.0f, (processingTime / maxTime) * 100.0f);
}

// Vector Path Editor Implementation
void MacroWavetableEngine::VectorPathEditor::openEditor() {
    isOpen = true;
}

void MacroWavetableEngine::VectorPathEditor::closeEditor() {
    isOpen = false;
    selectedPointIndex = -1;
}

void MacroWavetableEngine::VectorPathEditor::selectPoint(int index) {
    selectedPointIndex = index;
}

void MacroWavetableEngine::VectorPathEditor::addPoint(float x, float y) {
    editX = std::clamp(x, 0.0f, 1.0f);
    editY = std::clamp(y, 0.0f, 1.0f);
}

void MacroWavetableEngine::VectorPathEditor::deleteSelectedPoint() {
    if (selectedPointIndex >= 0) {
        selectedPointIndex = -1;
    }
}

void MacroWavetableEngine::VectorPathEditor::quantizeToCorners() {
    if (selectedPointIndex >= 0) {
        // Snap to nearest corner
        float corners[4][2] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}};
        
        int nearestCorner = 0;
        float minDist = 2.0f;
        
        for (int i = 0; i < 4; i++) {
            float dx = editX - corners[i][0];
            float dy = editY - corners[i][1];
            float dist = dx * dx + dy * dy;
            
            if (dist < minDist) {
                minDist = dist;
                nearestCorner = i;
            }
        }
        
        editX = corners[nearestCorner][0];
        editY = corners[nearestCorner][1];
    }
}

void MacroWavetableEngine::VectorPathEditor::setSelectedPointPosition(float x, float y) {
    editX = std::clamp(x, 0.0f, 1.0f);
    editY = std::clamp(y, 0.0f, 1.0f);
}

// Voice implementation would continue here...
// Simplified for length - voice contains wavetable oscillators, formant shifter, etc.

MacroWavetableEngine::MacroWavetableVoice::MacroWavetableVoice() {
    envelope_.sampleRate = 48000.0f;
    
    // Initialize wavetables if needed
    if (!WavetableOscillator::wavetablesInitialized) {
        WavetableOscillator::initializeWavetables();
    }
}

void MacroWavetableEngine::MacroWavetableVoice::noteOn(uint8_t note, float velocity, float aftertouch, float sampleRate) {
    note_ = note;
    velocity_ = velocity;
    aftertouch_ = aftertouch;
    active_ = true;
    age_ = 0;
    
    // Calculate note frequency
    noteFrequency_ = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    
    // Set oscillator frequencies
    oscA_.setFrequency(noteFrequency_, sampleRate);
    oscB_.setFrequency(noteFrequency_, sampleRate);
    oscC_.setFrequency(noteFrequency_, sampleRate);
    oscD_.setFrequency(noteFrequency_, sampleRate);
    
    // Update envelope sample rate
    envelope_.sampleRate = sampleRate;
    
    // Trigger envelope
    envelope_.noteOn();
}

void MacroWavetableEngine::MacroWavetableVoice::noteOff() {
    envelope_.noteOff();
}

void MacroWavetableEngine::MacroWavetableVoice::setAftertouch(float aftertouch) {
    aftertouch_ = aftertouch;
}

AudioFrame MacroWavetableEngine::MacroWavetableVoice::processSample() {
    if (!active_) {
        return AudioFrame(0.0f, 0.0f);
    }
    
    age_++;
    
    // Set wavetable positions for all oscillators
    oscA_.setPosition(wavetablePosition_);
    oscB_.setPosition(wavetablePosition_);
    oscC_.setPosition(wavetablePosition_);
    oscD_.setPosition(wavetablePosition_);
    
    // Generate oscillator outputs
    float outA = oscA_.process() * currentBlend_.gA;
    float outB = oscB_.process() * currentBlend_.gB;
    float outC = oscC_.process() * currentBlend_.gC;
    float outD = oscD_.process() * currentBlend_.gD;
    
    // Blend corner sources
    float mixed = outA + outB + outC + outD;
    
    // Apply formant shifting and spectral tilt
    float processed = formantShifter_.process(mixed);
    
    // Apply envelope
    float envLevel = envelope_.process();
    
    // Check if voice should be deactivated
    if (!envelope_.isActive()) {
        active_ = false;
    }
    
    // Apply velocity and volume
    float output = processed * envLevel * velocity_ * volume_;
    
    return AudioFrame(output, output);
}

// Simplified implementations for wavetable oscillator and formant shifter
void MacroWavetableEngine::MacroWavetableVoice::WavetableOscillator::initializeWavetables() {
    // Generate basic wavetables (sine, saw, square, etc.)
    for (int table = 0; table < NUM_WAVETABLES; table++) {
        for (int i = 0; i < WAVETABLE_SIZE; i++) {
            float phase = 2.0f * M_PI * i / WAVETABLE_SIZE;
            
            if (table == 0) {
                // Sine wave
                wavetables[table][i] = std::sin(phase);
            } else {
                // Progressive harmonic content
                float harmonic = 1.0f + table * 0.1f;
                wavetables[table][i] = std::sin(phase * harmonic) * (1.0f / harmonic);
            }
        }
    }
    wavetablesInitialized = true;
}

void MacroWavetableEngine::MacroWavetableVoice::WavetableOscillator::setFrequency(float freq, float sampleRate) {
    frequency = freq;
    this->sampleRate = sampleRate;
    increment = frequency / sampleRate;
}

void MacroWavetableEngine::MacroWavetableVoice::WavetableOscillator::setPosition(float pos) {
    position = std::clamp(pos, 0.0f, 1.0f);
}

float MacroWavetableEngine::MacroWavetableVoice::WavetableOscillator::process() {
    if (!wavetablesInitialized) return 0.0f;
    
    // Select wavetables for interpolation
    float scaledPos = position * (NUM_WAVETABLES - 1);
    int tableA = static_cast<int>(scaledPos);
    int tableB = std::min(tableA + 1, NUM_WAVETABLES - 1);
    float fraction = scaledPos - tableA;
    
    // Interpolate between wavetables
    float output = interpolateWavetable(tableA, tableB, fraction, phase);
    
    // Update phase
    phase += increment;
    if (phase >= 1.0f) phase -= 1.0f;
    
    return output;
}

float MacroWavetableEngine::MacroWavetableVoice::WavetableOscillator::interpolateWavetable(int tableA, int tableB, float fraction, float phase) const {
    // Simple linear interpolation between tables and samples
    float scaledPhase = phase * WAVETABLE_SIZE;
    int sampleA = static_cast<int>(scaledPhase) % WAVETABLE_SIZE;
    int sampleB = (sampleA + 1) % WAVETABLE_SIZE;
    float sampleFraction = scaledPhase - static_cast<int>(scaledPhase);
    
    // Interpolate within table A
    float outputA = wavetables[tableA][sampleA] * (1.0f - sampleFraction) + 
                    wavetables[tableA][sampleB] * sampleFraction;
    
    // Interpolate within table B
    float outputB = wavetables[tableB][sampleA] * (1.0f - sampleFraction) + 
                    wavetables[tableB][sampleB] * sampleFraction;
    
    // Interpolate between tables
    return outputA * (1.0f - fraction) + outputB * fraction;
}

// Simplified formant shifter
void MacroWavetableEngine::MacroWavetableVoice::FormantShifter::setShift(float semitones) {
    shiftSemitones = std::clamp(semitones, -6.0f, 6.0f);
}

void MacroWavetableEngine::MacroWavetableVoice::FormantShifter::setTilt(float tiltDb) {
    spectralTilt = std::clamp(tiltDb, -3.0f, 3.0f);
}

float MacroWavetableEngine::MacroWavetableVoice::FormantShifter::process(float input) {
    // Simplified pitch shifting and spectral tilt
    // In practice, this would use more sophisticated algorithms
    
    // Store input in ring buffer
    buffer[writeIndex] = input;
    writeIndex = (writeIndex + 1) % BUFFER_SIZE;
    
    // Read with pitch shift offset
    float shiftRatio = std::pow(2.0f, shiftSemitones / 12.0f);
    float readOffset = 1.0f / shiftRatio;
    
    int readIdx = static_cast<int>(readIndex + readOffset * BUFFER_SIZE) % BUFFER_SIZE;
    float shifted = buffer[readIdx];
    
    // Apply spectral tilt (simplified high-shelf)
    float tilted = applySpectralTilt(shifted);
    
    return tilted;
}

float MacroWavetableEngine::MacroWavetableVoice::FormantShifter::applySpectralTilt(float input) {
    // Simple high-frequency emphasis/de-emphasis
    float tiltGain = std::pow(10.0f, spectralTilt / 20.0f);
    return input * tiltGain;
}

// Envelope implementation
float MacroWavetableEngine::MacroWavetableVoice::Envelope::process() {
    const float attackRate = 1.0f / (attack * sampleRate);
    const float decayRate = 1.0f / (decay * sampleRate);
    const float releaseRate = 1.0f / (release * sampleRate);
    
    switch (stage) {
        case Stage::IDLE:
            return 0.0f;
            
        case Stage::ATTACK:
            level += attackRate;
            if (level >= 1.0f) {
                level = 1.0f;
                stage = Stage::DECAY;
            }
            break;
            
        case Stage::DECAY:
            level -= decayRate;
            if (level <= sustain) {
                level = sustain;
                stage = Stage::SUSTAIN;
            }
            break;
            
        case Stage::SUSTAIN:
            level = sustain;
            break;
            
        case Stage::RELEASE:
            level -= releaseRate;
            if (level <= 0.0f) {
                level = 0.0f;
                stage = Stage::IDLE;
            }
            break;
    }
    
    return level;
}

void MacroWavetableEngine::MacroWavetableVoice::setWavetableParams(float position, const CornerSources::BlendWeights& blend) {
    wavetablePosition_ = position;
    currentBlend_ = blend;
}

void MacroWavetableEngine::MacroWavetableVoice::setFormantParams(float formantShift, float spectralTilt) {
    formantShift_ = formantShift;
    spectralTilt_ = spectralTilt;
    formantShifter_.setShift(formantShift);
    formantShifter_.setTilt(spectralTilt);
}

void MacroWavetableEngine::MacroWavetableVoice::setVolume(float volume) {
    volume_ = volume;
}

void MacroWavetableEngine::MacroWavetableVoice::setEnvelopeParams(float attack, float decay, float sustain, float release) {
    envelope_.attack = attack;
    envelope_.decay = decay;
    envelope_.sustain = sustain;
    envelope_.release = release;
}