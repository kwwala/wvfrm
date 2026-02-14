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
        wvfrm::SyncClockInput start {};
        start.hostIsPlayingKnown = true;
        start.hostIsPlaying = true;
        start.hostPpqValid = true;
        start.hostPpq = 2.0; // 0.5 on a 4-beat loop
        start.hostBpmValid = true;
        start.hostBpm = 120.0;
        start.blockEndSample = 0;
        start.sampleRate = 48000.0;
        start.beatsInLoop = 4.0;

        const auto out = wvfrm::updateSyncLoopClock(start, state);
        if (! out.phaseReliable || ! out.resetSuggested || ! nearlyEqual(out.phaseNormalized, 0.5f, 1.0e-4f))
        {
            std::cerr << "LoopClock: expected play-start anchor at host phase 0.5." << std::endl;
            ok = false;
        }
    }

    {
        wvfrm::SyncClockInput running {};
        running.hostIsPlayingKnown = true;
        running.hostIsPlaying = true;
        running.hostPpqValid = false; // no host phase updates after start
        running.hostBpmValid = true;
        running.hostBpm = 120.0;
        running.blockEndSample = 24000; // +1 beat at 48k/120bpm
        running.sampleRate = 48000.0;
        running.beatsInLoop = 4.0;

        const auto out = wvfrm::updateSyncLoopClock(running, state);
        if (! out.phaseReliable || ! nearlyEqual(out.phaseNormalized, 0.75f, 1.0e-3f))
        {
            std::cerr << "LoopClock: expected internal clock advance by one beat to 0.75 phase." << std::endl;
            ok = false;
        }
    }

    {
        wvfrm::SyncClockInput bpmChange {};
        bpmChange.hostIsPlayingKnown = true;
        bpmChange.hostIsPlaying = true;
        bpmChange.hostPpqValid = false;
        bpmChange.hostBpmValid = true;
        bpmChange.hostBpm = 90.0; // should reset phase to 0
        bpmChange.blockEndSample = 30000;
        bpmChange.sampleRate = 48000.0;
        bpmChange.beatsInLoop = 4.0;

        const auto out = wvfrm::updateSyncLoopClock(bpmChange, state);
        if (! out.resetSuggested || ! nearlyEqual(out.phaseNormalized, 0.0f, 1.0e-4f))
        {
            std::cerr << "LoopClock: expected BPM change to reset phase to 0." << std::endl;
            ok = false;
        }
    }

    {
        wvfrm::SyncClockInput stopped {};
        stopped.hostIsPlayingKnown = true;
        stopped.hostIsPlaying = false;
        stopped.hostPpqValid = false;
        stopped.hostBpmValid = true;
        stopped.hostBpm = 90.0;
        stopped.blockEndSample = 32000;
        stopped.sampleRate = 48000.0;
        stopped.beatsInLoop = 4.0;

        const auto out = wvfrm::updateSyncLoopClock(stopped, state);
        if (! out.phaseReliable || ! nearlyEqual(out.phaseNormalized, state.lastPhase, 1.0e-4f))
        {
            std::cerr << "LoopClock: expected stopped transport to hold phase." << std::endl;
            ok = false;
        }
    }

    return ok;
}
