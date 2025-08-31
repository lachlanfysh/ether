#include "AudioEngine.h"
#include "VoiceManager.h"
#include "../instruments/InstrumentSlot.h"
#include "../sequencer/Timeline.h"
#include "../sequencer/EuclideanRhythm.h"
#include "../control/modulation/ModulationMatrix.h"
#include "../processing/effects/EffectsChain.h"
#include <iostream>
#include <chrono>

AudioEngine::AudioEngine() {
    std::cout << "Creating AudioEngine..." << std::endl;
    
    // Initialize parameter change system
    for (auto& change : parameterChanges_) {
        change.pending.store(false);
    }
}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::initialize(HardwareInterface* hardware) {
    std::cout << "Initializing AudioEngine..." << std::endl;
    
    hardware_ = hardware;
    if (!hardware_) {
        std::cerr << "Hardware interface is null!" << std::endl;
        return false;
    }
    
    try {
        // Create core components
        voiceManager_ = std::make_unique<VoiceManager>();
        timeline_ = std::make_unique<Timeline>();
        modMatrix_ = std::make_unique<ModulationMatrix>();
        masterEffects_ = std::make_unique<EffectsChain>();
        
        // Initialize instruments
        initializeInstruments();
        
        // Initialize sequencer
        initializeSequencer();
        
        // Set up audio callback
        hardware_->setAudioCallback([this](EtherAudioBuffer& buffer) {
            this->audioCallback(buffer);
        });
        
        std::cout << "AudioEngine initialized successfully" << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize AudioEngine: " << e.what() << std::endl;
        return false;
    }
}

void AudioEngine::shutdown() {
    std::cout << "Shutting down AudioEngine..." << std::endl;
    
    // Stop playback
    stop();
    
    // Reset components
    voiceManager_.reset();
    timeline_.reset();
    modMatrix_.reset();
    masterEffects_.reset();
    
    for (auto& instrument : instruments_) {
        instrument.reset();
    }
    
    hardware_ = nullptr;
}

void AudioEngine::processAudio(EtherAudioBuffer& outputBuffer) {
    audioCallback(outputBuffer);
}

void AudioEngine::audioCallback(EtherAudioBuffer& buffer) {
    // Clear the output buffer
    clearBuffer(buffer);
    
    // Apply any pending parameter changes (thread-safe)
    applyParameterChanges();
    
    // Update sequencer timing if playing
    if (isPlaying_) {
        updateSequencer();
    }
    
    // Process modulation matrix
    if (modMatrix_) {
        modMatrix_->process();
    }
    
    // Process all instruments
    EtherAudioBuffer instrumentBuffer;
    for (auto& instrument : instruments_) {
        if (instrument) {
            clearBuffer(instrumentBuffer);
            instrument->processAudio(instrumentBuffer);
            mixBuffers(buffer, instrumentBuffer);
        }
    }
    
    // Apply master effects
    applyMasterEffects(buffer);
    
    // Apply master volume
    float masterVol = masterVolume_.load();
    for (auto& frame : buffer) {
        frame = frame * masterVol;
    }
}

void AudioEngine::initializeInstruments() {
    std::cout << "Initializing instruments..." << std::endl;
    
    for (size_t i = 0; i < MAX_INSTRUMENTS; i++) {
        InstrumentColor color = static_cast<InstrumentColor>(i);
        instruments_[i] = std::make_unique<InstrumentSlot>(color);
        
        // Add a default engine to each instrument
        instruments_[i]->addEngine(EngineType::MACRO_VA);
        
        std::cout << "Instrument " << i << " (" << 
                     (color == InstrumentColor::CORAL ? "Coral" :
                      color == InstrumentColor::PEACH ? "Peach" :
                      color == InstrumentColor::CREAM ? "Cream" :
                      color == InstrumentColor::SAGE ? "Sage" :
                      color == InstrumentColor::TEAL ? "Teal" :
                      color == InstrumentColor::SLATE ? "Slate" :
                      color == InstrumentColor::PEARL ? "Pearl" : "Stone")
                     << ") initialized" << std::endl;
    }
}

void AudioEngine::initializeSequencer() {
    std::cout << "Initializing sequencer..." << std::endl;
    
    // Set default BPM
    setBPM(120.0f);
    
    // Initialize timeline with empty patterns
    if (timeline_) {
        // Timeline will be populated as user creates patterns
    }
}

void AudioEngine::calculateTiming() {
    float bpm = bpm_.load();
    float beatsPerSecond = bpm / 60.0f;
    float stepsPerSecond = beatsPerSecond * 4.0f; // 16th notes
    samplesPerStep_ = static_cast<uint32_t>(SAMPLE_RATE / stepsPerSecond);
}

void AudioEngine::updateSequencer() {
    sampleCounter_++;
    
    if (sampleCounter_ >= samplesPerStep_) {
        sampleCounter_ = 0;
        triggerStep(currentStep_);
        
        currentStep_++;
        if (currentStep_ >= 16) { // 16 steps per pattern
            currentStep_ = 0;
            currentBar_++;
        }
    }
}

void AudioEngine::triggerStep(uint8_t step) {
    // Trigger patterns for all active instruments
    for (auto& instrument : instruments_) {
        if (instrument && instrument->isPatternActive()) {
            auto* pattern = instrument->getPattern();
            if (pattern && pattern->shouldTrigger(step)) {
                // Trigger note - this is simplified, real implementation would
                // have more sophisticated note triggering logic
                instrument->noteOn(60, 0.8f); // Middle C with default velocity
            }
        }
    }
}

InstrumentSlot* AudioEngine::getInstrument(InstrumentColor color) {
    size_t index = static_cast<size_t>(color);
    if (index < MAX_INSTRUMENTS) {
        return instruments_[index].get();
    }
    return nullptr;
}

const InstrumentSlot* AudioEngine::getInstrument(InstrumentColor color) const {
    size_t index = static_cast<size_t>(color);
    if (index < MAX_INSTRUMENTS) {
        return instruments_[index].get();
    }
    return nullptr;
}

void AudioEngine::setActiveInstrument(InstrumentColor color) {
    activeInstrument_.store(color);
}

void AudioEngine::noteOn(uint8_t keyIndex, float velocity, float aftertouch) {
    // Map key index to MIDI note (simple mapping for now)
    uint8_t note = 60 + keyIndex; // Start from middle C
    
    InstrumentSlot* instrument = getInstrument(activeInstrument_.load());
    if (instrument) {
        instrument->noteOn(note, velocity, aftertouch);
    }
}

void AudioEngine::noteOff(uint8_t keyIndex) {
    uint8_t note = 60 + keyIndex;
    
    InstrumentSlot* instrument = getInstrument(activeInstrument_.load());
    if (instrument) {
        instrument->noteOff(note);
    }
}

void AudioEngine::setAftertouch(uint8_t keyIndex, float aftertouch) {
    uint8_t note = 60 + keyIndex;
    
    InstrumentSlot* instrument = getInstrument(activeInstrument_.load());
    if (instrument) {
        instrument->setAftertouch(note, aftertouch);
    }
}

void AudioEngine::allNotesOff() {
    for (auto& instrument : instruments_) {
        if (instrument) {
            instrument->allNotesOff();
        }
    }
}

void AudioEngine::setParameter(ParameterID param, float value) {
    InstrumentSlot* instrument = getInstrument(activeInstrument_.load());
    if (instrument) {
        queueParameterChange(activeInstrument_.load(), param, value);
    }
}

float AudioEngine::getParameter(ParameterID param) const {
    const InstrumentSlot* instrument = getInstrument(activeInstrument_.load());
    if (instrument) {
        return instrument->getParameter(param);
    }
    return 0.0f;
}

void AudioEngine::setInstrumentParameter(InstrumentColor instrument, ParameterID param, float value) {
    queueParameterChange(instrument, param, value);
}

float AudioEngine::getInstrumentParameter(InstrumentColor instrument, ParameterID param) const {
    const InstrumentSlot* instrumentSlot = getInstrument(instrument);
    if (instrumentSlot) {
        return instrumentSlot->getParameter(param);
    }
    return 0.0f;
}

void AudioEngine::play() {
    isPlaying_.store(true);
    std::cout << "Playback started" << std::endl;
}

void AudioEngine::stop() {
    isPlaying_.store(false);
    allNotesOff();
    
    // Reset sequencer position
    sampleCounter_ = 0;
    currentStep_ = 0;
    
    std::cout << "Playback stopped" << std::endl;
}

void AudioEngine::record(bool enable) {
    isRecording_.store(enable);
    std::cout << "Recording " << (enable ? "started" : "stopped") << std::endl;
}

void AudioEngine::setBPM(float bpm) {
    bpm_.store(std::clamp(bpm, 60.0f, 200.0f));
    calculateTiming();
    std::cout << "BPM set to " << bpm << std::endl;
}

size_t AudioEngine::getActiveVoiceCount() const {
    size_t totalVoices = 0;
    for (const auto& instrument : instruments_) {
        if (instrument) {
            totalVoices += instrument->getActiveVoiceCount();
        }
    }
    return totalVoices;
}

void AudioEngine::setMasterVolume(float volume) {
    masterVolume_.store(std::clamp(volume, 0.0f, 1.0f));
}

void AudioEngine::queueParameterChange(InstrumentColor instrument, ParameterID param, float value) {
    size_t index = parameterChangeIndex_.fetch_add(1) % MAX_PARAMETER_CHANGES;
    
    ParameterChange& change = parameterChanges_[index];
    change.instrument = instrument;
    change.parameter = param;
    change.value = value;
    change.pending.store(true);
}

void AudioEngine::applyParameterChanges() {
    for (auto& change : parameterChanges_) {
        if (change.pending.exchange(false)) { // Atomic read-and-clear
            InstrumentSlot* instrument = getInstrument(change.instrument);
            if (instrument) {
                instrument->setParameter(change.parameter, change.value);
            }
        }
    }
}

void AudioEngine::clearBuffer(EtherAudioBuffer& buffer) {
    for (auto& frame : buffer) {
        frame.left = 0.0f;
        frame.right = 0.0f;
    }
}

void AudioEngine::mixBuffers(EtherAudioBuffer& dest, const EtherAudioBuffer& src, float gain) {
    for (size_t i = 0; i < BUFFER_SIZE; i++) {
        dest[i] += src[i] * gain;
    }
}

void AudioEngine::applyMasterEffects(EtherAudioBuffer& buffer) {
    if (masterEffects_) {
        masterEffects_->process(buffer);
    }
}