#include "BandAnalyzer3.h"

#include <cmath>

namespace wvfrm
{

BandEnergies BandAnalyzer3::analyzeSegment(const float* samples,
                                           int numSamples,
                                           double sampleRate,
                                           float smoothingAmount) const noexcept
{
    BandEnergies output;

    if (samples == nullptr || numSamples <= 0 || sampleRate <= 0.0)
        return output;

    constexpr auto lowCut = 200.0;
    constexpr auto highCut = 2000.0;

    const auto lowAlpha = alphaForCutoff(lowCut, sampleRate);
    const auto highAlpha = alphaForCutoff(highCut, sampleRate);

    float lowState = 0.0f;
    float hiLpState = 0.0f;

    float lowEnergy = 0.0f;
    float midEnergy = 0.0f;
    float highEnergy = 0.0f;

    const auto smooth = juce::jlimit(0.0f, 1.0f, smoothingAmount);
    const auto oneMinusSmooth = 1.0f - smooth;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto x = samples[i];

        lowState += lowAlpha * (x - lowState);
        hiLpState += highAlpha * (x - hiLpState);

        const auto low = lowState;
        const auto high = x - hiLpState;
        const auto mid = x - low - high;

        const auto lowSq = low * low;
        const auto midSq = mid * mid;
        const auto highSq = high * high;

        lowEnergy = smooth * lowEnergy + oneMinusSmooth * lowSq;
        midEnergy = smooth * midEnergy + oneMinusSmooth * midSq;
        highEnergy = smooth * highEnergy + oneMinusSmooth * highSq;
    }

    const auto lowRms = std::sqrt(lowEnergy);
    const auto midRms = std::sqrt(midEnergy);
    const auto highRms = std::sqrt(highEnergy);

    const auto total = lowRms + midRms + highRms;

    if (total <= 1.0e-9f)
    {
        output.low = output.mid = output.high = 0.0f;
        return output;
    }

    output.low = lowRms / total;
    output.mid = midRms / total;
    output.high = highRms / total;
    return output;
}

float BandAnalyzer3::alphaForCutoff(double cutoffHz, double sampleRate) noexcept
{
    const auto clampedRate = juce::jmax(1.0, sampleRate);
    const auto omega = juce::MathConstants<double>::twoPi * cutoffHz / clampedRate;
    const auto alpha = 1.0 - std::exp(-omega);
    return static_cast<float>(juce::jlimit(0.0, 1.0, alpha));
}

} // namespace wvfrm

