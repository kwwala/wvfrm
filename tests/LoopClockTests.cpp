#include "dsp/LoopClock.h"

#include <cmath>
#include <iostream>

namespace
{
bool nearlyEqual(float a, float b, float tolerance)
{
    return std::abs(a - b) <= tolerance;
}
}

bool runLoopClockTests()
{
    bool ok = true;

    wvfrm::SyncClockState state {};

    {
        wvfrm::SyncClockInput first {};
        first.hostPhaseValid = true;
        first.hostPpq = 4.0;
        first.hostBpm = 120.0;
        first.blockEndSample = 48000;
        first.sampleRate = 48000.0;
        first.beatsInLoop = 4.0;

        const auto out = wvfrm::updateSyncLoopClock(first, state);
        if (! out.phaseReliable || ! nearlyEqual(out.phaseNormalized, 0.0f, 1.0e-4f))
        {
            std::cerr << "LoopClock: expected reliable phase at 0.0 for aligned PPQ." << std::endl;
            ok = false;
        }
    }

    {
        wvfrm::SyncClockInput jitter {};
        jitter.hostPhaseValid = true;
        jitter.hostPpq = 5.0; // phase 0.25
        jitter.hostBpm = 120.0;
        jitter.blockEndSample = 96000;
        jitter.sampleRate = 48000.0;
        jitter.beatsInLoop = 4.0;
        auto out = wvfrm::updateSyncLoopClock(jitter, state);
        if (! nearlyEqual(out.phaseNormalized, 0.25f, 1.0e-4f))
        {
            std::cerr << "LoopClock: expected 0.25 phase for PPQ 5.0 on 4-beat loop." << std::endl;
            ok = false;
        }

        jitter.hostPpq = 4.995; // tiny negative jitter
        jitter.blockEndSample = 96500;
        out = wvfrm::updateSyncLoopClock(jitter, state);
        if (! nearlyEqual(out.phaseNormalized, 0.25f, 1.0e-4f))
        {
            std::cerr << "LoopClock: expected jitter suppression for tiny backward host step." << std::endl;
            ok = false;
        }
    }

    {
        wvfrm::SyncClockInput grace {};
        grace.hostPhaseValid = false;
        grace.hostBpm = 120.0;
        grace.blockEndSample = 102000; // 125 ms at 48k from last ref (96k)
        grace.sampleRate = 48000.0;
        grace.beatsInLoop = 4.0;

        const auto out = wvfrm::updateSyncLoopClock(grace, state);
        if (! out.phaseReliable || ! nearlyEqual(out.phaseNormalized, 0.3125f, 5.0e-3f))
        {
            std::cerr << "LoopClock: expected grace-phase prediction during short PPQ dropout." << std::endl;
            ok = false;
        }
    }

    {
        wvfrm::SyncClockInput dropout {};
        dropout.hostPhaseValid = false;
        dropout.hostBpm = 120.0;
        dropout.blockEndSample = 120500; // >250 ms from reference
        dropout.sampleRate = 48000.0;
        dropout.beatsInLoop = 4.0;

        const auto out = wvfrm::updateSyncLoopClock(dropout, state);
        if (out.phaseReliable)
        {
            std::cerr << "LoopClock: expected unreliable phase after grace timeout." << std::endl;
            ok = false;
        }
    }

    {
        wvfrm::SyncClockInput regained {};
        regained.hostPhaseValid = true;
        regained.hostPpq = 7.6; // phase 0.9
        regained.hostBpm = 120.0;
        regained.blockEndSample = 121000;
        regained.sampleRate = 48000.0;
        regained.beatsInLoop = 4.0;

        const auto out = wvfrm::updateSyncLoopClock(regained, state);
        if (! out.phaseReliable || ! out.resetSuggested)
        {
            std::cerr << "LoopClock: expected reset hint when host phase returns far from held phase." << std::endl;
            ok = false;
        }
    }

    return ok;
}
