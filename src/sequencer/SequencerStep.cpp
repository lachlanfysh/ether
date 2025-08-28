#include "SequencerStep.h"
#include <algorithm>
#include <cmath>

SequencerStep::SequencerStep() {
    data_ = StepData(); // Use default constructor
}

SequencerStep::SequencerStep(const StepData& data) {
    setData(data);
}

SequencerStep::SequencerStep(uint8_t note, uint8_t velocity) {
    data_ = StepData();
    setNote(note);
    setVelocity(velocity);
}

void SequencerStep::setNote(uint8_t note) {
    data_.note = clampNote(note);
}

void SequencerStep::setVelocity(uint8_t velocity) {
    data_.velocity = clampVelocity(velocity);
}

void SequencerStep::setSlideTime(uint8_t slideTimeMs) {
    data_.slideTimeMs = clampSlideTime(slideTimeMs);
}

void SequencerStep::setAccentAmount(uint8_t accentAmount) {
    data_.accentAmount = std::min(accentAmount, MAX_ACCENT_AMOUNT);
}

float SequencerStep::getSlideTimeSeconds() const {
    return static_cast<float>(data_.slideTimeMs) * 0.001f; // ms to seconds
}

void SequencerStep::setSlideTimeSeconds(float timeSeconds) {
    uint8_t timeMs = static_cast<uint8_t>(std::round(timeSeconds * 1000.0f));
    setSlideTime(timeMs);
}

float SequencerStep::getAccentGainDB() const {
    // Map 0-127 to 0-8dB
    return (static_cast<float>(data_.accentAmount) / 127.0f) * MAX_ACCENT_GAIN_DB;
}

float SequencerStep::getAccentCutoffBoost() const {
    // Map 0-127 to 0-25% boost
    return (static_cast<float>(data_.accentAmount) / 127.0f) * MAX_ACCENT_CUTOFF_BOOST;
}

void SequencerStep::setAccentGainDB(float gainDB) {
    float clamped = std::max(0.0f, std::min(gainDB, MAX_ACCENT_GAIN_DB));
    uint8_t amount = static_cast<uint8_t>((clamped / MAX_ACCENT_GAIN_DB) * 127.0f);
    setAccentAmount(amount);
}

// Flag management
void SequencerStep::setFlag(StepFlags flag, bool enabled) {
    setFlagBit(static_cast<uint16_t>(flag), enabled);
}

void SequencerStep::clearFlag(StepFlags flag) {
    setFlagBit(static_cast<uint16_t>(flag), false);
}

void SequencerStep::toggleFlag(StepFlags flag) {
    uint16_t mask = static_cast<uint16_t>(flag);
    setFlagBit(mask, !getFlagBit(mask));
}

bool SequencerStep::hasFlag(StepFlags flag) const {
    return getFlagBit(static_cast<uint16_t>(flag));
}

void SequencerStep::clearAllFlags() {
    data_.flags = 0;
}

// Convenience flag methods
void SequencerStep::setEnabled(bool enabled) {
    setFlag(StepFlags::ENABLED, enabled);
}

void SequencerStep::setAccent(bool accent) {
    setFlag(StepFlags::ACCENT, accent);
}

void SequencerStep::setSlide(bool slide) {
    setFlag(StepFlags::SLIDE, slide);
}

void SequencerStep::setTie(bool tie) {
    setFlag(StepFlags::TIE, tie);
}

void SequencerStep::setVelocityLatch(bool latch) {
    setFlag(StepFlags::VELOCITY_LATCH, latch);
}

void SequencerStep::setMute(bool mute) {
    setFlag(StepFlags::MUTE, mute);
}

bool SequencerStep::isEnabled() const {
    return hasFlag(StepFlags::ENABLED);
}

bool SequencerStep::isAccent() const {
    return hasFlag(StepFlags::ACCENT);
}

bool SequencerStep::isSlide() const {
    return hasFlag(StepFlags::SLIDE);
}

bool SequencerStep::isTie() const {
    return hasFlag(StepFlags::TIE);
}

bool SequencerStep::isVelocityLatch() const {
    return hasFlag(StepFlags::VELOCITY_LATCH);
}

bool SequencerStep::isMute() const {
    return hasFlag(StepFlags::MUTE);
}

// Advanced parameters
void SequencerStep::setProbability(uint8_t probability) {
    data_.probability = std::min(probability, static_cast<uint8_t>(127));
}

void SequencerStep::setMicroTiming(int8_t offset) {
    // Clamp to -64 to +63 range, store as 0-127
    int8_t clamped = std::max(static_cast<int8_t>(-64), std::min(offset, static_cast<int8_t>(63)));
    data_.microTiming = static_cast<uint8_t>(clamped + 64);
}

// Step data access
void SequencerStep::setData(const StepData& data) {
    data_.note = clampNote(data.note);
    data_.velocity = clampVelocity(data.velocity);
    data_.slideTimeMs = clampSlideTime(data.slideTimeMs);
    data_.accentAmount = std::min(data.accentAmount, MAX_ACCENT_AMOUNT);
    data_.flags = data.flags;
    data_.probability = std::min(data.probability, static_cast<uint8_t>(127));
    data_.microTiming = data.microTiming; // Already validated in setter
}

// Serialization
uint64_t SequencerStep::serialize() const {
    uint64_t packed = 0;
    
    // Pack all data into 64-bit value
    packed |= static_cast<uint64_t>(data_.note);                    // Bits 0-7
    packed |= static_cast<uint64_t>(data_.velocity) << 8;           // Bits 8-15
    packed |= static_cast<uint64_t>(data_.slideTimeMs) << 16;       // Bits 16-23
    packed |= static_cast<uint64_t>(data_.accentAmount) << 24;      // Bits 24-31
    packed |= static_cast<uint64_t>(data_.flags) << 32;             // Bits 32-47
    packed |= static_cast<uint64_t>(data_.probability) << 48;       // Bits 48-55
    packed |= static_cast<uint64_t>(data_.microTiming) << 56;       // Bits 56-63
    
    return packed;
}

void SequencerStep::deserialize(uint64_t packed) {
    StepData newData;
    
    newData.note = static_cast<uint8_t>(packed & 0xFF);
    newData.velocity = static_cast<uint8_t>((packed >> 8) & 0xFF);
    newData.slideTimeMs = static_cast<uint8_t>((packed >> 16) & 0xFF);
    newData.accentAmount = static_cast<uint8_t>((packed >> 24) & 0xFF);
    newData.flags = static_cast<uint16_t>((packed >> 32) & 0xFFFF);
    newData.probability = static_cast<uint8_t>((packed >> 48) & 0xFF);
    newData.microTiming = static_cast<uint8_t>((packed >> 56) & 0xFF);
    
    setData(newData); // This will validate all values
}

// Utility methods
void SequencerStep::reset() {
    data_ = StepData(); // Reset to defaults
}

void SequencerStep::copyFrom(const SequencerStep& other) {
    data_ = other.data_;
}

bool SequencerStep::isActive() const {
    return isEnabled() && !isMute();
}

// Comparison operators
bool SequencerStep::operator==(const SequencerStep& other) const {
    return (data_.note == other.data_.note &&
            data_.velocity == other.data_.velocity &&
            data_.slideTimeMs == other.data_.slideTimeMs &&
            data_.accentAmount == other.data_.accentAmount &&
            data_.flags == other.data_.flags &&
            data_.probability == other.data_.probability &&
            data_.microTiming == other.data_.microTiming);
}

bool SequencerStep::operator!=(const SequencerStep& other) const {
    return !(*this == other);
}

// Private utility functions
uint8_t SequencerStep::clampSlideTime(uint8_t slideTimeMs) const {
    return std::max(MIN_SLIDE_TIME_MS, std::min(slideTimeMs, MAX_SLIDE_TIME_MS));
}

uint8_t SequencerStep::clampVelocity(uint8_t velocity) const {
    return std::min(velocity, static_cast<uint8_t>(127));
}

uint8_t SequencerStep::clampNote(uint8_t note) const {
    return std::min(note, static_cast<uint8_t>(127));
}

void SequencerStep::setFlagBit(uint16_t mask, bool value) {
    if (value) {
        data_.flags |= mask;
    } else {
        data_.flags &= ~mask;
    }
}

bool SequencerStep::getFlagBit(uint16_t mask) const {
    return (data_.flags & mask) != 0;
}