#pragma once

#include "../JuceIncludes.h"

namespace wvfrm
{

struct SyncClockInput
{
    bool hostPhaseValid = false;
    double hostPpq = 0.0;
    double hostBpm = 120.0;
    int64_t blockEndSample = 0;
    double sampleRate = 44100.0;
    double beatsInLoop = 1.0;
};

struct SyncClockOutput
{
    float phaseNormalized = 0.0f;
    bool phaseReliable = false;
    bool resetSuggested = false;
};

struct SyncClockState
{
    bool hasReference = false;
    double referencePpq = 0.0;
    int64_t referenceEndSample = 0;
    double referenceBpm = 120.0;
    bool hasLastPhase = false;
    float lastPhase = 0.0f;
    bool lastReliable = false;
};

SyncClockOutput updateSyncLoopClock(const SyncClockInput& input,
                                    SyncClockState& state,
                                    double graceSeconds = 0.25) noexcept;

} // namespace wvfrm
