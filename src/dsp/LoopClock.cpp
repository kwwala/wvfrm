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
}

SyncClockOutput updateSyncLoopClock(const SyncClockInput& input,
                                    SyncClockState& state,
                                    double graceSeconds) noexcept
{
    SyncClockOutput out {};

    const auto safeRate = juce::jmax(1.0, input.sampleRate);
    const auto safeBeatsInLoop = juce::jmax(1.0e-9, input.beatsInLoop);
    const auto safeBpm = juce::jmax(1.0, input.hostBpm);

    if (input.hostPhaseValid)
    {
        auto phase = static_cast<float>(positiveFraction(input.hostPpq / safeBeatsInLoop));
        phase = juce::jlimit(0.0f, 1.0f, phase);
        auto jitterSuppressed = false;

        if (state.hasLastPhase)
        {
            const auto delta = phase - state.lastPhase;
            const auto wrapped = delta < -0.5f;

            if (! wrapped && delta < 0.0f)
            {
                // Ignore small backward host jitter to prevent visual creep.
                if (delta > -0.03f)
                {
                    phase = state.lastPhase;
                    jitterSuppressed = true;
                }
                else
                    out.resetSuggested = true;
            }

            if (! wrapped && std::abs(phase - state.lastPhase) > 0.35f)
                out.resetSuggested = true;

            if (! state.lastReliable && circularDistance(phase, state.lastPhase) > 0.06f)
                out.resetSuggested = true;
        }

        if (! jitterSuppressed)
        {
            state.hasReference = true;
            state.referencePpq = input.hostPpq;
            state.referenceEndSample = input.blockEndSample;
            state.referenceBpm = safeBpm;
        }

        state.hasLastPhase = true;
        state.lastPhase = phase;
        state.lastReliable = true;

        out.phaseNormalized = phase;
        out.phaseReliable = true;
        return out;
    }

    float phase = state.hasLastPhase ? state.lastPhase : 0.0f;
    auto reliable = false;

    if (state.hasReference)
    {
        const auto elapsedSamples = juce::jmax<int64_t>(0, input.blockEndSample - state.referenceEndSample);
        const auto graceSamples = static_cast<int64_t>(std::llround(juce::jmax(0.0, graceSeconds) * safeRate));

        if (elapsedSamples <= graceSamples)
        {
            const auto beatsAdvanced = (static_cast<double>(elapsedSamples) * state.referenceBpm) / (60.0 * safeRate);
            const auto predictedPpq = state.referencePpq + beatsAdvanced;
            phase = static_cast<float>(positiveFraction(predictedPpq / safeBeatsInLoop));
            phase = juce::jlimit(0.0f, 1.0f, phase);

            if (state.hasLastPhase)
            {
                const auto delta = phase - state.lastPhase;
                const auto wrapped = delta < -0.5f;
                if (! wrapped && delta < 0.0f)
                    phase = state.lastPhase;
            }

            reliable = true;
        }
    }

    state.hasLastPhase = true;
    state.lastPhase = phase;
    state.lastReliable = reliable;

    out.phaseNormalized = phase;
    out.phaseReliable = reliable;
    out.resetSuggested = false;
    return out;
}

} // namespace wvfrm
