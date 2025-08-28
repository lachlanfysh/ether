#include "SequencerPattern.h"
#include <algorithm>
#include <random>
#include <cstring>

SequencerPattern::SequencerPattern() {
    numSteps_ = DEFAULT_STEPS;
    numTracks_ = 1;
    name_ = "Pattern";
    tempo_ = 120.0f;
    timing_ = TimingConfig();
    selection_ = Selection{};
    hasValidClipboard_ = false;
    
    initializeSteps();
}

SequencerPattern::SequencerPattern(int numSteps, int numTracks) {
    numSteps_ = std::max(MIN_STEPS, std::min(numSteps, MAX_STEPS));
    numTracks_ = std::max(1, std::min(numTracks, MAX_TRACKS));
    name_ = "Pattern";
    tempo_ = 120.0f;
    timing_ = TimingConfig();
    selection_ = Selection{};
    hasValidClipboard_ = false;
    
    initializeSteps();
}

// Pattern structure
void SequencerPattern::setLength(int numSteps) {
    int newLength = std::max(MIN_STEPS, std::min(numSteps, MAX_STEPS));
    
    // If shortening, clear steps beyond new length
    if (newLength < numSteps_) {
        for (int track = 0; track < numTracks_; track++) {
            for (int step = newLength; step < numSteps_; step++) {
                steps_[track][step].reset();
            }
        }
    }
    
    numSteps_ = newLength;
    
    // Validate selection after length change
    if (hasSelection()) {
        Selection newSelection = selection_;
        validateSelection(newSelection);
        selection_ = newSelection;
    }
}

void SequencerPattern::setNumTracks(int numTracks) {
    int newNumTracks = std::max(1, std::min(numTracks, MAX_TRACKS));
    
    // If reducing tracks, clear the removed tracks
    if (newNumTracks < numTracks_) {
        for (int track = newNumTracks; track < numTracks_; track++) {
            clearTrack(track);
        }
    }
    
    numTracks_ = newNumTracks;
    
    // Validate selection after track change
    if (hasSelection()) {
        Selection newSelection = selection_;
        validateSelection(newSelection);
        selection_ = newSelection;
    }
}

// Step access
SequencerStep* SequencerPattern::getStep(int track, int step) {
    if (!isValidPosition(track, step)) {
        return nullptr;
    }
    return &steps_[track][step];
}

const SequencerStep* SequencerPattern::getStep(int track, int step) const {
    if (!isValidPosition(track, step)) {
        return nullptr;
    }
    return &steps_[track][step];
}

void SequencerPattern::setStep(int track, int step, const SequencerStep& stepData) {
    if (isValidPosition(track, step)) {
        steps_[track][step] = stepData;
    }
}

void SequencerPattern::clearStep(int track, int step) {
    if (isValidPosition(track, step)) {
        steps_[track][step].reset();
    }
}

void SequencerPattern::copyStep(int fromTrack, int fromStep, int toTrack, int toStep) {
    if (isValidPosition(fromTrack, fromStep) && isValidPosition(toTrack, toStep)) {
        steps_[toTrack][toStep] = steps_[fromTrack][fromStep];
    }
}

// Step convenience methods
void SequencerPattern::setStepNote(int track, int step, uint8_t note, uint8_t velocity) {
    if (SequencerStep* stepPtr = getStep(track, step)) {
        stepPtr->setNote(note);
        stepPtr->setVelocity(velocity);
        stepPtr->setEnabled(true);
    }
}

void SequencerPattern::setStepAccent(int track, int step, bool accent, uint8_t amount) {
    if (SequencerStep* stepPtr = getStep(track, step)) {
        stepPtr->setAccent(accent);
        if (accent) {
            stepPtr->setAccentAmount(amount);
        }
    }
}

void SequencerPattern::setStepSlide(int track, int step, bool slide, uint8_t timeMs) {
    if (SequencerStep* stepPtr = getStep(track, step)) {
        stepPtr->setSlide(slide);
        if (slide) {
            stepPtr->setSlideTime(timeMs);
        }
    }
}

void SequencerPattern::toggleStepAccent(int track, int step) {
    if (SequencerStep* stepPtr = getStep(track, step)) {
        stepPtr->toggleFlag(SequencerStep::StepFlags::ACCENT);
    }
}

void SequencerPattern::toggleStepSlide(int track, int step) {
    if (SequencerStep* stepPtr = getStep(track, step)) {
        stepPtr->toggleFlag(SequencerStep::StepFlags::SLIDE);
    }
}

// Track configuration
void SequencerPattern::setTrackConfig(int track, const TrackConfig& config) {
    if (validateTrackRange(track)) {
        trackConfigs_[track] = config;
    }
}

const SequencerPattern::TrackConfig& SequencerPattern::getTrackConfig(int track) const {
    if (validateTrackRange(track)) {
        return trackConfigs_[track];
    }
    static TrackConfig defaultConfig;
    return defaultConfig;
}

void SequencerPattern::setTrackType(int track, TrackType type) {
    if (validateTrackRange(track)) {
        trackConfigs_[track].type = type;
    }
}

void SequencerPattern::setTrackMute(int track, bool muted) {
    if (validateTrackRange(track)) {
        trackConfigs_[track].muted = muted;
    }
}

void SequencerPattern::setTrackSolo(int track, bool solo) {
    if (validateTrackRange(track)) {
        trackConfigs_[track].solo = solo;
    }
}

void SequencerPattern::setTrackLevel(int track, float level) {
    if (validateTrackRange(track)) {
        trackConfigs_[track].level = std::max(MIN_LEVEL, std::min(level, MAX_LEVEL));
    }
}

void SequencerPattern::setTrackTranspose(int track, int8_t semitones) {
    if (validateTrackRange(track)) {
        trackConfigs_[track].transpose = std::max(MIN_TRANSPOSE, std::min(semitones, MAX_TRANSPOSE));
    }
}

// Track state queries
bool SequencerPattern::isTrackMuted(int track) const {
    if (validateTrackRange(track)) {
        return trackConfigs_[track].muted;
    }
    return true; // Invalid tracks are considered muted
}

bool SequencerPattern::isTrackSolo(int track) const {
    if (validateTrackRange(track)) {
        return trackConfigs_[track].solo;
    }
    return false;
}

bool SequencerPattern::isTrackAudible(int track) const {
    if (!validateTrackRange(track) || !trackConfigs_[track].enabled) {
        return false;
    }
    
    // If any track is solo'd, only solo tracks are audible
    bool anySolo = false;
    for (int i = 0; i < numTracks_; i++) {
        if (trackConfigs_[i].solo) {
            anySolo = true;
            break;
        }
    }
    
    if (anySolo) {
        return trackConfigs_[track].solo;
    } else {
        return !trackConfigs_[track].muted;
    }
}

float SequencerPattern::getTrackLevel(int track) const {
    if (validateTrackRange(track)) {
        return trackConfigs_[track].level;
    }
    return 0.0f;
}

// Pattern operations
void SequencerPattern::clear() {
    for (int track = 0; track < numTracks_; track++) {
        clearTrack(track);
    }
}

void SequencerPattern::clearTrack(int track) {
    if (validateTrackRange(track)) {
        for (int step = 0; step < numSteps_; step++) {
            steps_[track][step].reset();
        }
    }
}

void SequencerPattern::clearRange(int track, int startStep, int endStep) {
    if (!validateTrackRange(track)) return;
    
    int start = std::max(0, startStep);
    int end = std::min(numSteps_ - 1, endStep);
    
    for (int step = start; step <= end; step++) {
        steps_[track][step].reset();
    }
}

void SequencerPattern::shiftPattern(int steps) {
    if (steps == 0) return;
    
    for (int track = 0; track < numTracks_; track++) {
        shiftTrack(track, steps);
    }
}

void SequencerPattern::shiftTrack(int track, int steps) {
    if (!validateTrackRange(track) || steps == 0) return;
    
    // Normalize steps to pattern length
    int normalizedSteps = steps % numSteps_;
    if (normalizedSteps == 0) return;
    
    // Create temporary copy
    std::array<SequencerStep, MAX_STEPS> temp;
    for (int i = 0; i < numSteps_; i++) {
        temp[i] = steps_[track][i];
    }
    
    // Shift steps
    for (int i = 0; i < numSteps_; i++) {
        int sourceIndex = (i - normalizedSteps + numSteps_) % numSteps_;
        steps_[track][i] = temp[sourceIndex];
    }
}

void SequencerPattern::reversePattern() {
    for (int track = 0; track < numTracks_; track++) {
        reverseTrack(track);
    }
}

void SequencerPattern::reverseTrack(int track) {
    if (!validateTrackRange(track)) return;
    
    for (int i = 0; i < numSteps_ / 2; i++) {
        int j = numSteps_ - 1 - i;
        std::swap(steps_[track][i], steps_[track][j]);
    }
}

void SequencerPattern::randomizeVelocities(int track, uint8_t minVel, uint8_t maxVel) {
    if (!validateTrackRange(track)) return;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint8_t> dist(minVel, maxVel);
    
    for (int step = 0; step < numSteps_; step++) {
        if (steps_[track][step].isEnabled()) {
            steps_[track][step].setVelocity(dist(gen));
        }
    }
}

void SequencerPattern::randomizeAccents(int track, float probability) {
    if (!validateTrackRange(track)) return;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::bernoulli_distribution dist(probability);
    std::uniform_int_distribution<uint8_t> amountDist(40, 100);
    
    for (int step = 0; step < numSteps_; step++) {
        if (steps_[track][step].isEnabled() && dist(gen)) {
            steps_[track][step].setAccent(true);
            steps_[track][step].setAccentAmount(amountDist(gen));
        }
    }
}

// Selection support
bool SequencerPattern::Selection::isValid() const {
    return (startTrack >= 0 && endTrack >= startTrack &&
            startStep >= 0 && endStep >= startStep);
}

void SequencerPattern::setSelection(const Selection& selection) {
    selection_ = selection;
    validateSelection(selection_);
}

void SequencerPattern::clearSelection() {
    selection_ = Selection{};
}

bool SequencerPattern::hasSelection() const {
    return selection_.isValid();
}

// Timing configuration
void SequencerPattern::setTimingConfig(const TimingConfig& config) {
    timing_ = config;
    timing_.swing = std::max(MIN_SWING, std::min(timing_.swing, MAX_SWING));
    timing_.shuffle = std::max(MIN_SWING, std::min(timing_.shuffle, MAX_SWING));
}

void SequencerPattern::setSwing(float swing) {
    timing_.swing = std::max(MIN_SWING, std::min(swing, MAX_SWING));
}

void SequencerPattern::setShuffle(float shuffle) {
    timing_.shuffle = std::max(MIN_SWING, std::min(shuffle, MAX_SWING));
}

void SequencerPattern::setHumanize(int8_t humanize) {
    timing_.humanize = std::max(static_cast<int8_t>(-64), std::min(humanize, static_cast<int8_t>(63)));
}

void SequencerPattern::setGateTime(float gateTime) {
    timing_.gateTime = std::max(0.1f, std::min(gateTime, 2.0f));
}

// Pattern analysis
int SequencerPattern::countActiveSteps(int track) const {
    if (!validateTrackRange(track)) return 0;
    
    int count = 0;
    for (int step = 0; step < numSteps_; step++) {
        if (steps_[track][step].isActive()) {
            count++;
        }
    }
    return count;
}

int SequencerPattern::countAccentSteps(int track) const {
    if (!validateTrackRange(track)) return 0;
    
    int count = 0;
    for (int step = 0; step < numSteps_; step++) {
        if (steps_[track][step].isAccent()) {
            count++;
        }
    }
    return count;
}

int SequencerPattern::countSlideSteps(int track) const {
    if (!validateTrackRange(track)) return 0;
    
    int count = 0;
    for (int step = 0; step < numSteps_; step++) {
        if (steps_[track][step].isSlide()) {
            count++;
        }
    }
    return count;
}

bool SequencerPattern::isEmpty() const {
    for (int track = 0; track < numTracks_; track++) {
        if (!isTrackEmpty(track)) {
            return false;
        }
    }
    return true;
}

bool SequencerPattern::isTrackEmpty(int track) const {
    return countActiveSteps(track) == 0;
}

// Validation
bool SequencerPattern::isValidTrack(int track) const {
    return validateTrackRange(track);
}

bool SequencerPattern::isValidStep(int step) const {
    return validateStepRange(step);
}

bool SequencerPattern::isValidPosition(int track, int step) const {
    return validateTrackRange(track) && validateStepRange(step);
}

// Private methods
void SequencerPattern::initializeSteps() {
    for (int track = 0; track < MAX_TRACKS; track++) {
        for (int step = 0; step < MAX_STEPS; step++) {
            steps_[track][step].reset();
        }
    }
}

void SequencerPattern::validateSelection(Selection& selection) const {
    selection.startTrack = std::max(0, std::min(selection.startTrack, numTracks_ - 1));
    selection.endTrack = std::max(selection.startTrack, std::min(selection.endTrack, numTracks_ - 1));
    selection.startStep = std::max(0, std::min(selection.startStep, numSteps_ - 1));
    selection.endStep = std::max(selection.startStep, std::min(selection.endStep, numSteps_ - 1));
}

bool SequencerPattern::validateTrackRange(int track) const {
    return track >= 0 && track < numTracks_;
}

bool SequencerPattern::validateStepRange(int step) const {
    return step >= 0 && step < numSteps_;
}