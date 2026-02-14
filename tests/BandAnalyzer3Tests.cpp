#include "dsp/BandAnalyzer3.h"

#include <cmath>
#include <iostream>
#include <numbers>
#include <vector>

namespace
{
std::vector<float> makeSine(double frequency, double sampleRate, int numSamples)
{
    std::vector<float> signal(static_cast<size_t>(numSamples), 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto phase = 2.0 * std::numbers::pi * frequency * static_cast<double>(i) / sampleRate;
        signal[static_cast<size_t>(i)] = static_cast<float>(std::sin(phase));
    }

    return signal;
}
}

bool runBandAnalyzerTests()
{
    constexpr auto sampleRate = 48000.0;
    constexpr auto numSamples = 4096;

    wvfrm::BandAnalyzer3 analyzer;

    bool ok = true;

    const auto lowSignal = makeSine(80.0, sampleRate, numSamples);
    const auto lowBands = analyzer.analyzeSegment(lowSignal.data(),
                                                  static_cast<int>(lowSignal.size()),
                                                  sampleRate,
                                                  150.0f,
                                                  2500.0f,
                                                  wvfrm::BandAnalyzer3::fastWindowSize);

    if (! (lowBands.low > lowBands.mid && lowBands.low > lowBands.high))
    {
        std::cerr << "BandAnalyzer3: 80 Hz should favor low band." << std::endl;
        ok = false;
    }

    const auto midSignal = makeSine(1000.0, sampleRate, numSamples);
    const auto midBands = analyzer.analyzeSegment(midSignal.data(),
                                                  static_cast<int>(midSignal.size()),
                                                  sampleRate,
                                                  150.0f,
                                                  2500.0f,
                                                  wvfrm::BandAnalyzer3::fastWindowSize);

    if (! (midBands.mid > midBands.low && midBands.mid > midBands.high))
    {
        std::cerr << "BandAnalyzer3: 1 kHz should favor mid band." << std::endl;
        ok = false;
    }

    const auto highSignal = makeSine(8000.0, sampleRate, numSamples);
    const auto highBands = analyzer.analyzeSegment(highSignal.data(),
                                                   static_cast<int>(highSignal.size()),
                                                   sampleRate,
                                                   150.0f,
                                                   2500.0f,
                                                   wvfrm::BandAnalyzer3::fastWindowSize);

    if (! (highBands.high > highBands.low && highBands.high > highBands.mid))
    {
        std::cerr << "BandAnalyzer3: 8 kHz should favor high band." << std::endl;
        ok = false;
    }

    return ok;
}
