#pragma once

#include "../JuceIncludes.h"

namespace wvfrm
{

double projectLoopPhase(double basePhaseNormalized,
                        bool phaseReliable,
                        bool isPlaying,
                        double bpmUsed,
                        double beatsInLoop,
                        double elapsedSeconds) noexcept;

} // namespace wvfrm
