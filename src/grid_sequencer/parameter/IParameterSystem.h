#pragma once
#include "../core/DataStructures.h"
#include "../core/DIContainer.h"
#include "../../core/Types.h"  // For ParameterID
#include <string>
#include <vector>

namespace GridSequencer {
namespace Parameter {

using Core::Result;
using Core::ParamRoute;

// Parameter system interface - manages parameter caching and routing
class IParameterSystem {
public:
    virtual ~IParameterSystem() = default;

    // Parameter value management
    virtual Result<bool> setParameter(int engine, ParameterID param, float value) = 0;
    virtual Result<float> getParameter(int engine, ParameterID param) = 0;
    virtual Result<bool> adjustParameter(int engine, ParameterID param, bool increment, bool useShift = false) = 0;

    // Parameter routing
    virtual ParamRoute getParameterRoute(int engine, ParameterID param) = 0;
    virtual bool isParameterSupported(int engine, ParameterID param) = 0;
    virtual std::string getRouteDisplayTag(ParamRoute route) = 0;

    // Parameter information
    virtual std::string getParameterName(ParameterID param) = 0;
    virtual std::string getParameterDisplayValue(int engine, ParameterID param) = 0;
    virtual float getParameterDisplayNormalized(int engine, ParameterID param) = 0;

    // Parameter lists for UI
    virtual std::vector<int> getVisibleParameters(int engine) = 0;
    virtual std::vector<int> getExtendedParameters(int engine) = 0;

    // Parameter validation
    virtual bool isValidParameterValue(ParameterID param, float value) = 0;
    virtual float clampParameterValue(ParameterID param, float value) = 0;

    // Cache management
    virtual void syncCacheToEngine(int engine) = 0;
    virtual void syncEngineToCache(int engine) = 0;
    virtual void clearCache() = 0;
    virtual void initializeDefaults(int engine) = 0;

    // Pseudo-parameters (octave/pitch offset)
    virtual Result<bool> setPseudoParameter(int paramId, float value) = 0;
    virtual Result<float> getPseudoParameter(int paramId) = 0;
    virtual bool isPseudoParameter(int paramId) = 0;

    // FM algorithm handling
    virtual int getFMAlgorithm(int engine) = 0;
    virtual Result<bool> setFMAlgorithm(int engine, int algorithm) = 0;
    virtual bool isFMEngine(int engine) = 0;
};

} // namespace Parameter
} // namespace GridSequencer