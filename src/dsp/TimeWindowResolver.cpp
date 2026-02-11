#include "TimeWindowResolver.h"

namespace wvfrm
{

double TimeWindowResolver::divisionToMs(int divisionIndex, double bpm) noexcept
{
    const auto clamped = juce::jlimit(0, static_cast<int>(divisions.size()) - 1, divisionIndex);
    const auto safeBpm = juce::jmax(1.0, bpm);

    const auto division = divisions[static_cast<size_t>(clamped)];
    const auto beats = 4.0 * static_cast<double>(division.numerator) / static_cast<double>(division.denominator);
    return (beats * 60000.0) / safeBpm;
}

TimeWindowResolver::ResolvedWindow TimeWindowResolver::resolve(bool syncMode,
                                                               int divisionIndex,
                                                               double milliseconds,
                                                               std::optional<double> hostBpm,
                                                               double lastKnownBpm) noexcept
{
    ResolvedWindow resolved;

    const auto fallbackBpm = juce::jmax(1.0, lastKnownBpm);

    if (! syncMode)
    {
        resolved.ms = juce::jlimit(10.0, 5000.0, milliseconds);
        resolved.tempoReliable = hostBpm.has_value() && hostBpm.value() > 0.0;
        resolved.bpmUsed = resolved.tempoReliable ? hostBpm.value() : fallbackBpm;
        return resolved;
    }

    const auto hasReliableTempo = hostBpm.has_value() && hostBpm.value() > 0.0;
    const auto bpm = hasReliableTempo ? hostBpm.value() : fallbackBpm;

    resolved.ms = divisionToMs(divisionIndex, bpm);
    resolved.tempoReliable = hasReliableTempo;
    resolved.bpmUsed = bpm;

    return resolved;
}

} // namespace wvfrm
