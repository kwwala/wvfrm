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

float circularDistance(float a, float b) noexcept
{
    const auto direct = std::abs(a - b);
    return juce::jmin(direct, 1.0f - direct);
}

bool isForwardWrap(float previous, float current) noexcept
{
    return previous > 0.8f && current < 0.2f;
}
}

SyncClockOutput updateSyncLoopClock(const SyncClockInput& input,
                                    SyncClockState& state) noexcept
{
    SyncClockOutput out {};

    const auto safeRate = juce::jmax(1.0, input.sampleRate);
    const auto safeBeatsInLoop = juce::jmax(1.0e-9, input.beatsInLoop);

    const auto fallbackBpm = input.hostBpmValid
        ? juce::jmax(1.0, input.hostBpm)
        : juce::jmax(1.0, state.lastKnownBpm);

    if (input.hostBpmValid)
        state.lastKnownBpm = fallbackBpm;

    const auto restartedPlayback = input.isPlaying && ! state.wasPlaying;

    auto hostTimeDiscontinuity = false;
    if (input.isPlaying
        && state.wasPlaying
        && input.hasHostTimeInSamples
        && state.hasLastHostTime)
    {
        const auto hostDelta = input.hostTimeInSamples - state.lastHostTimeInSamples;
        const auto expectedDelta = static_cast<int64_t>(juce::jmax(0, input.blockNumSamples));
        const auto tolerance = juce::jmax<int64_t>(expectedDelta * 2,
                                                   static_cast<int64_t>(safeRate * 0.02));
        hostTimeDiscontinuity = (hostDelta < 0)
            || (std::llabs(hostDelta - expectedDelta) > tolerance);
    }

    const auto beatsChanged = state.hasLastBeatsInLoop
        && (std::abs(state.lastBeatsInLoop - safeBeatsInLoop) > 1.0e-9);

    if (! input.isPlaying)
    {
        out.phaseAtBlockStart = state.hasLastPhase ? state.lastPhase : 0.0f;
        out.phaseReliable = false;
        out.resetSuggested = false;

        if (input.hostPhaseValid)
        {
            state.hasAnchor = true;
            state.anchorSampleLocal = input.blockStartSampleLocal;
            state.anchorPpq = input.hostPpq;
        }

        state.hasLastPhase = true;
        state.lastPhase = out.phaseAtBlockStart;
        state.lastPhaseReliable = false;
        state.lastSampleLocal = input.blockStartSampleLocal;

        state.hasLastHostTime = input.hasHostTimeInSamples;
        if (input.hasHostTimeInSamples)
            state.lastHostTimeInSamples = input.hostTimeInSamples;

        state.hasLastBeatsInLoop = true;
        state.lastBeatsInLoop = safeBeatsInLoop;
        state.wasPlaying = false;
        return out;
    }

    out.resetSuggested = restartedPlayback || hostTimeDiscontinuity || beatsChanged;

    if (input.hostPhaseValid)
    {
        auto phase = static_cast<float>(positiveFraction(input.hostPpq / safeBeatsInLoop));
        phase = juce::jlimit(0.0f, 1.0f, phase);

        if (state.hasLastPhase)
        {
            const auto wrapped = isForwardWrap(state.lastPhase, phase);
            const auto largeJump = circularDistance(state.lastPhase, phase) > 0.35f;

            if (! wrapped && largeJump)
                out.resetSuggested = true;

            if (! state.lastPhaseReliable && circularDistance(state.lastPhase, phase) > 0.06f)
                out.resetSuggested = true;
        }

        out.phaseAtBlockStart = phase;
        out.phaseReliable = true;

        state.hasAnchor = true;
        state.anchorSampleLocal = input.blockStartSampleLocal;
        state.anchorPpq = input.hostPpq;
    }
    else
    {
        auto phase = state.hasLastPhase ? state.lastPhase : 0.0f;

        if (beatsChanged || hostTimeDiscontinuity || restartedPlayback)
            state.hasAnchor = false;

        if (state.hasAnchor)
        {
            const auto samplesFromAnchor = juce::jmax<int64_t>(0, input.blockStartSampleLocal - state.anchorSampleLocal);
            const auto beatsAdvanced = (static_cast<double>(samplesFromAnchor) * fallbackBpm) / (60.0 * safeRate);
            phase = static_cast<float>(positiveFraction((state.anchorPpq + beatsAdvanced) / safeBeatsInLoop));
        }
        else
        {
            const auto sampleDelta = state.hasLastPhase
                ? juce::jmax<int64_t>(0, input.blockStartSampleLocal - state.lastSampleLocal)
                : static_cast<int64_t>(juce::jmax(0, input.blockNumSamples));
            const auto phaseAdvance = (static_cast<double>(sampleDelta) * fallbackBpm)
                / (60.0 * safeRate * safeBeatsInLoop);
            phase = static_cast<float>(positiveFraction(static_cast<double>(phase) + phaseAdvance));
        }

        out.phaseAtBlockStart = juce::jlimit(0.0f, 1.0f, phase);
        out.phaseReliable = false;
    }

    state.hasLastPhase = true;
    state.lastPhase = out.phaseAtBlockStart;
    state.lastPhaseReliable = out.phaseReliable;
    state.lastSampleLocal = input.blockStartSampleLocal;

    state.hasLastHostTime = input.hasHostTimeInSamples;
    if (input.hasHostTimeInSamples)
        state.lastHostTimeInSamples = input.hostTimeInSamples;

    state.hasLastBeatsInLoop = true;
    state.lastBeatsInLoop = safeBeatsInLoop;
    state.wasPlaying = true;

    return out;
}

} // namespace wvfrm
