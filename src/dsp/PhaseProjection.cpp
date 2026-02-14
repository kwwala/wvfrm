#include "PhaseProjection.h"

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

double projectLoopPhase(double basePhaseNormalized,
                        bool phaseReliable,
                        bool isPlaying,
                        double bpmUsed,
                        double beatsInLoop,
                        double elapsedSeconds) noexcept
{
    auto phase = juce::jlimit(0.0, 1.0, basePhaseNormalized);

    if (! phaseReliable || ! isPlaying || bpmUsed <= 1.0 || beatsInLoop <= 1.0e-9)
        return phase;

    const auto clampedElapsed = juce::jlimit(0.0, 0.120, elapsedSeconds);
    const auto phaseAdvance = (clampedElapsed * bpmUsed) / (60.0 * beatsInLoop);
    return juce::jlimit(0.0, 1.0, positiveFraction(phase + phaseAdvance));
}

} // namespace wvfrm
