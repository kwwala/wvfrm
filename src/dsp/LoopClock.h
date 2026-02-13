#pragma once

#include "../JuceIncludes.h"

namespace wvfrm
{

struct SyncClockInput
{
    bool hostPhaseValid = false;
    double hostPpq = 0.0;
    bool hostBpmValid = false;
    double hostBpm = 120.0;
    int64_t blockStartSampleLocal = 0;
    int blockNumSamples = 0;
    double sampleRate = 44100.0;
    double beatsInLoop = 1.0;
    bool isPlaying = false;
    bool hasHostTimeInSamples = false;
    int64_t hostTimeInSamples = 0;
};

struct SyncClockOutput
{
    float phaseAtBlockStart = 0.0f;
    bool phaseReliable = false;
    bool resetSuggested = false;
};

struct SyncClockState
{
    bool hasLastPhase = false;
    float lastPhase = 0.0f;
    bool lastPhaseReliable = false;
    int64_t lastSampleLocal = 0;

    bool hasAnchor = false;
    int64_t anchorSampleLocal = 0;
    double anchorPpq = 0.0;

    bool hasLastHostTime = false;
    int64_t lastHostTimeInSamples = 0;

    bool hasLastBeatsInLoop = false;
    double lastBeatsInLoop = 1.0;
    double lastKnownBpm = 120.0;
    bool wasPlaying = false;
};

SyncClockOutput updateSyncLoopClock(const SyncClockInput& input,
                                    SyncClockState& state) noexcept;

} // namespace wvfrm
