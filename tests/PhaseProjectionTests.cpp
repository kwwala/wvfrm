#include "dsp/PhaseProjection.h"

#include <cmath>
#include <iostream>

namespace
{
bool nearlyEqual(double a, double b, double tolerance = 1.0e-9)
{
    return std::abs(a - b) <= tolerance;
}
}

bool runPhaseProjectionTests()
{
    bool ok = true;

    {
        // Monotonic increase for growing elapsed time without wrap.
        const auto base = 0.2;
        auto previous = base;
        for (int i = 1; i <= 20; ++i)
        {
            const auto elapsed = 0.002 * static_cast<double>(i);
            const auto projected = wvfrm::projectLoopPhase(base, true, true, 120.0, 4.0, elapsed);
            if (projected + 1.0e-8 < previous)
            {
                std::cerr << "PhaseProjection: projected phase moved backward in monotonic test." << std::endl;
                ok = false;
                break;
            }
            previous = projected;
        }
    }

    {
        // Wrap remains in [0, 1).
        const auto projected = wvfrm::projectLoopPhase(0.98, true, true, 150.0, 1.0, 0.05);
        if (projected < 0.0 || projected > 1.0)
        {
            std::cerr << "PhaseProjection: wrap projected out of [0,1]." << std::endl;
            ok = false;
        }
    }

    {
        // No advance when not playing.
        const auto base = 0.37;
        const auto projected = wvfrm::projectLoopPhase(base, true, false, 128.0, 4.0, 0.09);
        if (! nearlyEqual(projected, base))
        {
            std::cerr << "PhaseProjection: phase advanced while transport stopped." << std::endl;
            ok = false;
        }
    }

    return ok;
}
