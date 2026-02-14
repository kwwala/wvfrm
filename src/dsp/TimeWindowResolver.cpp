#include "TimeWindowResolver.h"

namespace wvfrm
{

double TimeWindowResolver::divisionToBeats(int divisionIndex) noexcept
{
    const auto clamped = juce::jlimit(0, static_cast<int>(divisions.size()) - 1, divisionIndex);
    const auto division = divisions[static_cast<size_t>(clamped)];
    return 4.0 * static_cast<double>(division.numerator) / static_cast<double>(division.denominator);
}

double TimeWindowResolver::divisionToMs(int divisionIndex, double bpm) noexcept
{
    const auto safeBpm = juce::jmax(1.0, bpm);
    return (divisionToBeats(divisionIndex) * 60000.0) / safeBpm;
}

TimeWindowResolver::ResolvedWindow TimeWindowResolver::resolve(bool syncMode,
                                                               int divisionIndex,
                                                               double freeSpeedSeconds,
                                                               std::optional<double> hostBpm,
                                                               double lastKnownBpm) noexcept
{
    ResolvedWindow resolved;
    const auto fallbackBpm = juce::jmax(1.0, lastKnownBpm);

    if (! syncMode)
    {
        const auto seconds = juce::jlimit(0.25, 12.0, freeSpeedSeconds);
        resolved.ms = seconds * 1000.0;
        resolved.beats = 0.0;
        resolved.tempoReliable = hostBpm.has_value() && hostBpm.value() > 0.0;
        resolved.bpmUsed = resolved.tempoReliable ? hostBpm.value() : fallbackBpm;
        return resolved;
    }

    const auto hasReliableTempo = hostBpm.has_value() && hostBpm.value() > 0.0;
    const auto bpm = hasReliableTempo ? hostBpm.value() : fallbackBpm;

    resolved.ms = divisionToMs(divisionIndex, bpm);
    resolved.beats = divisionToBeats(divisionIndex);
    resolved.tempoReliable = hasReliableTempo;
    resolved.bpmUsed = bpm;
    return resolved;
}

} // namespace wvfrm
