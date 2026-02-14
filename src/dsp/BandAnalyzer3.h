#pragma once

#include "../JuceIncludes.h"
#include "../Parameters.h"

namespace wvfrm
{

struct BandEnergies
{
    float low = 0.0f;
    float mid = 0.0f;
    float high = 0.0f;
    float combinedRmsDb = -100.0f;
};

class BandAnalyzer3
{
public:
    static constexpr int fastWindowSize = 1024;
    static constexpr int slowWindowSize = 16384;

    static int rmsWindowForMode(HistoryMode mode) noexcept;

    BandEnergies analyzeSegment(const float* samples,
                                int numSamples,
                                double sampleRate,
                                float lowMidHz,
                                float midHighHz,
                                int rmsWindowSamples) const noexcept;

private:
    static float alphaForCutoff(double cutoffHz, double sampleRate) noexcept;
};

} // namespace wvfrm
