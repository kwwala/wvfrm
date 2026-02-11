#pragma once

#include "../JuceIncludes.h"

namespace wvfrm
{

struct BandEnergies
{
    float low = 0.0f;
    float mid = 0.0f;
    float high = 0.0f;
};

class BandAnalyzer3
{
public:
    BandEnergies analyzeSegment(const float* samples,
                                int numSamples,
                                double sampleRate,
                                float smoothingAmount) const noexcept;

private:
    static float alphaForCutoff(double cutoffHz, double sampleRate) noexcept;
};

} // namespace wvfrm
