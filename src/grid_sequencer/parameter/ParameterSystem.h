#pragma once
#include "IParameterSystem.h"
#include "../audio/IAudioEngine.h"
#include "../utils/Constants.h"
#include <map>
#include <array>
#include <atomic>
#include <memory>

namespace GridSequencer {
namespace Parameter {

using Audio::IAudioEngine;

class ParameterSystem : public IParameterSystem {
public:
    explicit ParameterSystem(std::shared_ptr<IAudioEngine> audioEngine);
    virtual ~ParameterSystem() = default;

    // Parameter value management
    Result<bool> setParameter(int engine, ParameterID param, float value) override;
    Result<float> getParameter(int engine, ParameterID param) override;
    Result<bool> adjustParameter(int engine, ParameterID param, bool increment, bool useShift = false) override;

    // Parameter routing
    ParamRoute getParameterRoute(int engine, ParameterID param) override;
    bool isParameterSupported(int engine, ParameterID param) override;
    std::string getRouteDisplayTag(ParamRoute route) override;

    // Parameter information
    std::string getParameterName(ParameterID param) override;
    std::string getParameterDisplayValue(int engine, ParameterID param) override;
    float getParameterDisplayNormalized(int engine, ParameterID param) override;

    // Parameter lists for UI
    std::vector<int> getVisibleParameters(int engine) override;
    std::vector<int> getExtendedParameters(int engine) override;

    // Parameter validation
    bool isValidParameterValue(ParameterID param, float value) override;
    float clampParameterValue(ParameterID param, float value) override;

    // Cache management
    void syncCacheToEngine(int engine) override;
    void syncEngineToCache(int engine) override;
    void clearCache() override;
    void initializeDefaults(int engine) override;

    // Pseudo-parameters (octave/pitch offset)
    Result<bool> setPseudoParameter(int paramId, float value) override;
    Result<float> getPseudoParameter(int paramId) override;
    bool isPseudoParameter(int paramId) override;

    // FM algorithm handling
    int getFMAlgorithm(int engine) override;
    Result<bool> setFMAlgorithm(int engine, int algorithm) override;
    bool isFMEngine(int engine) override;

    // Global state access (for migration period)
    void setCurrentEngine(int engine) { currentEngine_ = engine; }
    int getCurrentEngine() const { return currentEngine_; }
    void setShiftHeld(bool held) { shiftHeld_ = held; }

private:
    std::shared_ptr<IAudioEngine> audioEngine_;

    // Parameter cache - maintains UI-consistent values
    std::array<std::map<int, float>, MAX_ENGINES> parameterCache_;

    // Pseudo-parameters
    std::atomic<int> octaveOffset_{0};
    std::atomic<float> pitchOffset_{0.0f};

    // Current state (for UI coordination)
    int currentEngine_{0};
    std::atomic<bool> shiftHeld_{false};

    // Parameter name mapping
    std::map<int, std::string> parameterNames_;

    // Helper methods
    void initializeParameterNames();
    bool isValidEngine(int engine) const;
    float getParameterDelta(ParameterID param, bool useShift) const;
    void updateCache(int engine, ParameterID param, float value);

    // Pseudo-parameter constants
    static constexpr int PSEUDO_PARAM_OCTAVE = 1000;
    static constexpr int PSEUDO_PARAM_PITCH = 1001;
};

} // namespace Parameter
} // namespace GridSequencer