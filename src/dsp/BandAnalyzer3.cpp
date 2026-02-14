#include "BandAnalyzer3.h"

#include <cmath>

namespace wvfrm
{

int BandAnalyzer3::rmsWindowForMode(HistoryMode mode) noexcept
{
    return mode == HistoryMode::slow16384 ? slowWindowSize : fastWindowSize;
}

BandEnergies BandAnalyzer3::analyzeSegment(const float* samples,
                                           int numSamples,
                                           double sampleRate,
                                           float lowMidHz,
                                           float midHighHz,
                                           int rmsWindowSamples) const noexcept
{
    BandEnergies output;

    if (samples == nullptr || numSamples <= 0 || sampleRate <= 0.0)
        return output;

    const auto lowCut = juce::jlimit(40.0f, 2000.0f, lowMidHz);
    const auto highCut = juce::jlimit(lowCut + 50.0f, 12000.0f, midHighHz);

    const auto lowAlpha = alphaForCutoff(lowCut, sampleRate);
    const auto highAlpha = alphaForCutoff(highCut, sampleRate);

    float lowState = 0.0f;
    float highLpState = 0.0f;

    const auto window = juce::jmax(8, rmsWindowSamples);
    const auto decay = std::exp(-1.0f / static_cast<float>(window));
    const auto attack = 1.0f - decay;

    float lowSq = 0.0f;
    float midSq = 0.0f;
    float highSq = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const auto x = samples[i];

        lowState += lowAlpha * (x - lowState);
        highLpState += highAlpha * (x - highLpState);

        const auto low = lowState;
        const auto high = x - highLpState;
        const auto mid = x - low - high;

        lowSq = decay * lowSq + attack * (low * low);
        midSq = decay * midSq + attack * (mid * mid);
        highSq = decay * highSq + attack * (high * high);
    }

    const auto lowRms = std::sqrt(juce::jmax(0.0f, lowSq));
    const auto midRms = std::sqrt(juce::jmax(0.0f, midSq));
    const auto highRms = std::sqrt(juce::jmax(0.0f, highSq));
    const auto total = lowRms + midRms + highRms;

    if (total > 1.0e-9f)
    {
        output.low = lowRms / total;
        output.mid = midRms / total;
        output.high = highRms / total;
    }

    const auto combined = std::sqrt(juce::jmax(0.0f, (lowSq + midSq + highSq) / 3.0f));
    output.combinedRmsDb = juce::Decibels::gainToDecibels(combined, -100.0f);
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
