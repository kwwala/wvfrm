#pragma once

#include "../JuceIncludes.h"

namespace wvfrm
{

struct SyncClockInput
{
    bool hostIsPlayingKnown = false;
    bool hostIsPlaying = true;
    bool hostPpqValid = false;
    double hostPpq = 0.0;
    bool hostBpmValid = false;
    double hostBpm = 120.0;
    int64_t blockEndSample = 0;
    double sampleRate = 44100.0;
    double beatsInLoop = 4.0;
};

struct SyncClockOutput
{
    float phaseNormalized = 0.0f;
    bool phaseReliable = false;
    bool resetSuggested = false;
    double bpmUsed = 120.0;
};

struct SyncClockState
{
    bool initialized = false;
    bool wasPlaying = false;
    double bpm = 120.0;
    int64_t anchorSample = 0;
    double anchorPhase = 0.0;
    double lastPhase = 0.0;
};

SyncClockOutput updateSyncLoopClock(const SyncClockInput& input,
                                    SyncClockState& state) noexcept;

} // namespace wvfrm
