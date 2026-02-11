#include "dsp/TimeWindowResolver.h"

#include <cmath>
#include <iostream>

namespace
{
bool nearlyEqual(double a, double b, double tolerance)
{
    return std::abs(a - b) <= tolerance;
}
}

bool runTimeWindowResolverTests()
{
    bool ok = true;

    const auto quarterMs = wvfrm::TimeWindowResolver::divisionToMs(4, 120.0);
    if (! nearlyEqual(quarterMs, 500.0, 0.01))
    {
        std::cerr << "TimeWindowResolver: expected 1/4 at 120 BPM == 500 ms, got " << quarterMs << std::endl;
        ok = false;
    }

    const auto twoBarsMs = wvfrm::TimeWindowResolver::divisionToMs(7, 120.0);
    if (! nearlyEqual(twoBarsMs, 4000.0, 0.01))
    {
        std::cerr << "TimeWindowResolver: expected 2/1 at 120 BPM == 4000 ms, got " << twoBarsMs << std::endl;
        ok = false;
    }

    const auto fallback = wvfrm::TimeWindowResolver::resolve(true, 4, 250.0, std::nullopt, 120.0);
    if (fallback.tempoReliable)
    {
        std::cerr << "TimeWindowResolver: fallback should not be tempoReliable." << std::endl;
        ok = false;
    }

    if (! nearlyEqual(fallback.bpmUsed, 120.0, 0.001))
    {
        std::cerr << "TimeWindowResolver: fallback should use last BPM 120, got " << fallback.bpmUsed << std::endl;
        ok = false;
    }

    const auto msMode = wvfrm::TimeWindowResolver::resolve(false, 4, 50.0, std::nullopt, 120.0);
    if (! nearlyEqual(msMode.ms, 50.0, 0.001))
    {
        std::cerr << "TimeWindowResolver: ms mode expected 50 ms, got " << msMode.ms << std::endl;
        ok = false;
    }

    return ok;
}
