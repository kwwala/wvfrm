#include "LoopClock.h"

#include <cmath>

namespace wvfrm
{
namespace
{
double positiveFraction(double value) noexcept
{
    const auto floored = std::floor(value);
    const auto fraction = value - floored;
    return fraction < 0.0 ? (fraction + 1.0) : fraction;
}
}

SyncClockOutput updateSyncLoopClock(const SyncClockInput& input,
                                    SyncClockState& state) noexcept
{
    SyncClockOutput out {};

    const auto safeRate = juce::jmax(1.0, input.sampleRate);
    const auto safeLoopBeats = juce::jmax(1.0e-9, input.beatsInLoop);
    const auto isPlaying = input.hostIsPlayingKnown ? input.hostIsPlaying : true;

    auto bpm = state.bpm;
    if (input.hostBpmValid)
        bpm = juce::jmax(1.0, input.hostBpm);

    const auto justStarted = isPlaying && ! state.wasPlaying;
    const auto bpmChanged = std::abs(bpm - state.bpm) > 1.0e-6;

    if (! state.initialized || justStarted || bpmChanged)
    {
        state.initialized = true;
        state.bpm = bpm;
        state.anchorSample = input.blockEndSample;

        if (bpmChanged)
        {
            // User-requested behavior: reset phase when BPM changes.
            state.anchorPhase = 0.0;
        }
        else if (justStarted && input.hostPpqValid)
        {
            state.anchorPhase = positiveFraction(input.hostPpq / safeLoopBeats);
        }
        else
        {
            state.anchorPhase = state.lastPhase;
        }

        state.lastPhase = state.anchorPhase;
        out.resetSuggested = true;
    }

    if (! isPlaying)
    {
        state.anchorSample = input.blockEndSample;
        state.anchorPhase = state.lastPhase;

        out.phaseNormalized = static_cast<float>(juce::jlimit(0.0, 1.0, state.lastPhase));
        out.phaseReliable = state.initialized;
        out.bpmUsed = state.bpm;
        state.wasPlaying = isPlaying;
        return out;
    }

    const auto cycleSamples = safeLoopBeats * safeRate * 60.0 / juce::jmax(1.0, state.bpm);
    const auto elapsedSamples = juce::jmax<int64_t>(0, input.blockEndSample - state.anchorSample);
    const auto phase = positiveFraction(state.anchorPhase + static_cast<double>(elapsedSamples) / juce::jmax(1.0, cycleSamples));

    state.lastPhase = phase;

    out.phaseNormalized = static_cast<float>(juce::jlimit(0.0, 1.0, phase));
    out.phaseReliable = state.initialized;
    out.bpmUsed = state.bpm;

    state.wasPlaying = isPlaying;
    return out;
}

} // namespace wvfrm
