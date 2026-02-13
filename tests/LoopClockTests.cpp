#include "dsp/LoopClock.h"

#include <cmath>
#include <iostream>

namespace
{
bool movedBackwardWithoutWrap(float previous, float current)
{
    return (current + 1.0e-4f < previous) && ! (previous > 0.8f && current < 0.2f);
}

bool nearlyEqual(float a, float b, float tolerance)
{
    return std::abs(a - b) <= tolerance;
}
}

bool runLoopClockTests()
{
    bool ok = true;

    {
        wvfrm::SyncClockState state {};
        int64_t sample = 0;
        float previousPhase = 0.0f;

        for (int i = 0; i < 10000; ++i)
        {
            const auto blockSize = 32 + (i % 7) * 29;

            wvfrm::SyncClockInput input {};
            input.hostPhaseValid = true;
            input.hostPpq = (static_cast<double>(sample) * 120.0) / (60.0 * 48000.0);
            input.hostBpmValid = true;
            input.hostBpm = 120.0;
            input.blockStartSampleLocal = sample;
            input.blockNumSamples = blockSize;
            input.sampleRate = 48000.0;
            input.beatsInLoop = 4.0;
            input.isPlaying = true;
            input.hasHostTimeInSamples = true;
            input.hostTimeInSamples = sample;

            const auto out = wvfrm::updateSyncLoopClock(input, state);

            if (! out.phaseReliable)
            {
                std::cerr << "LoopClock: expected reliable phase under valid PPQ." << std::endl;
                ok = false;
                break;
            }

            if (i > 0 && movedBackwardWithoutWrap(previousPhase, out.phaseAtBlockStart))
            {
                std::cerr << "LoopClock: phase moved backward without wrap under host lock." << std::endl;
                ok = false;
                break;
            }

            previousPhase = out.phaseAtBlockStart;
            sample += blockSize;
        }
    }

    {
        wvfrm::SyncClockState state {};

        wvfrm::SyncClockInput seed {};
        seed.hostPhaseValid = true;
        seed.hostPpq = 8.0;
        seed.hostBpmValid = true;
        seed.hostBpm = 120.0;
        seed.blockStartSampleLocal = 0;
        seed.blockNumSamples = 128;
        seed.sampleRate = 48000.0;
        seed.beatsInLoop = 4.0;
        seed.isPlaying = true;
        const auto seeded = wvfrm::updateSyncLoopClock(seed, state);

        float previousPhase = seeded.phaseAtBlockStart;
        int64_t sample = 128;

        for (int i = 0; i < 2500; ++i)
        {
            const auto blockSize = 64 + (i % 5) * 37;

            wvfrm::SyncClockInput dropout {};
            dropout.hostPhaseValid = false;
            dropout.hostBpmValid = true;
            dropout.hostBpm = 120.0;
            dropout.blockStartSampleLocal = sample;
            dropout.blockNumSamples = blockSize;
            dropout.sampleRate = 48000.0;
            dropout.beatsInLoop = 4.0;
            dropout.isPlaying = true;

            const auto out = wvfrm::updateSyncLoopClock(dropout, state);

            if (out.phaseReliable)
            {
                std::cerr << "LoopClock: fallback phase should be marked unreliable." << std::endl;
                ok = false;
                break;
            }

            if (movedBackwardWithoutWrap(previousPhase, out.phaseAtBlockStart))
            {
                std::cerr << "LoopClock: fallback phase moved backward without wrap." << std::endl;
                ok = false;
                break;
            }

            previousPhase = out.phaseAtBlockStart;
            sample += blockSize;
        }

        wvfrm::SyncClockInput regained {};
        regained.hostPhaseValid = true;
        regained.hostPpq = 27.6;
        regained.hostBpmValid = true;
        regained.hostBpm = 120.0;
        regained.blockStartSampleLocal = sample;
        regained.blockNumSamples = 256;
        regained.sampleRate = 48000.0;
        regained.beatsInLoop = 4.0;
        regained.isPlaying = true;

        const auto regainedOut = wvfrm::updateSyncLoopClock(regained, state);
        if (! regainedOut.phaseReliable || ! regainedOut.resetSuggested)
        {
            std::cerr << "LoopClock: expected resetSuggested when PPQ returns with large discontinuity." << std::endl;
            ok = false;
        }
    }

    {
        wvfrm::SyncClockState state {};

        wvfrm::SyncClockInput first {};
        first.hostPhaseValid = true;
        first.hostPpq = 4.0;
        first.hostBpmValid = true;
        first.hostBpm = 120.0;
        first.blockStartSampleLocal = 0;
        first.blockNumSamples = 128;
        first.sampleRate = 48000.0;
        first.beatsInLoop = 4.0;
        first.isPlaying = true;

        const auto startOut = wvfrm::updateSyncLoopClock(first, state);

        float heldPhase = startOut.phaseAtBlockStart;
        int64_t sample = 128;
        for (int i = 0; i < 64; ++i)
        {
            wvfrm::SyncClockInput stopped {};
            stopped.hostPhaseValid = false;
            stopped.hostBpmValid = true;
            stopped.hostBpm = 120.0;
            stopped.blockStartSampleLocal = sample;
            stopped.blockNumSamples = 128;
            stopped.sampleRate = 48000.0;
            stopped.beatsInLoop = 4.0;
            stopped.isPlaying = false;

            const auto out = wvfrm::updateSyncLoopClock(stopped, state);
            if (! nearlyEqual(out.phaseAtBlockStart, heldPhase, 1.0e-6f))
            {
                std::cerr << "LoopClock: stop state should hold phase steady." << std::endl;
                ok = false;
                break;
            }

            sample += 128;
        }

        wvfrm::SyncClockInput resumed {};
        resumed.hostPhaseValid = true;
        resumed.hostPpq = 5.0;
        resumed.hostBpmValid = true;
        resumed.hostBpm = 120.0;
        resumed.blockStartSampleLocal = sample;
        resumed.blockNumSamples = 128;
        resumed.sampleRate = 48000.0;
        resumed.beatsInLoop = 4.0;
        resumed.isPlaying = true;

        const auto resumedOut = wvfrm::updateSyncLoopClock(resumed, state);
        if (! resumedOut.resetSuggested)
        {
            std::cerr << "LoopClock: expected resetSuggested on playback resume." << std::endl;
            ok = false;
        }
    }

    {
        wvfrm::SyncClockState state {};

        float previousPhase = 0.0f;
        auto sawMovement = false;
        int64_t sample = 0;

        for (int i = 0; i < 1200; ++i)
        {
            const auto blockSize = 48 + (i % 4) * 64;

            wvfrm::SyncClockInput bpmOnly {};
            bpmOnly.hostPhaseValid = false;
            bpmOnly.hostBpmValid = true;
            bpmOnly.hostBpm = 100.0;
            bpmOnly.blockStartSampleLocal = sample;
            bpmOnly.blockNumSamples = blockSize;
            bpmOnly.sampleRate = 48000.0;
            bpmOnly.beatsInLoop = 4.0;
            bpmOnly.isPlaying = true;

            const auto out = wvfrm::updateSyncLoopClock(bpmOnly, state);
            if (movedBackwardWithoutWrap(previousPhase, out.phaseAtBlockStart))
            {
                std::cerr << "LoopClock: BPM-only fallback phase moved backward without wrap." << std::endl;
                ok = false;
                break;
            }

            if (std::abs(out.phaseAtBlockStart - previousPhase) > 1.0e-6f)
                sawMovement = true;

            previousPhase = out.phaseAtBlockStart;
            sample += blockSize;
        }

        if (! sawMovement)
        {
            std::cerr << "LoopClock: BPM-only fallback did not advance phase." << std::endl;
            ok = false;
        }
    }

    return ok;
}
