#pragma once

#include "../JuceIncludes.h"

namespace wvfrm
{

class AnalysisRingBuffer
{
public:
    void prepare(int channels, int samplesPerChannel);
    void clear();

    void pushBuffer(const juce::AudioBuffer<float>& buffer) noexcept;
    bool copyMostRecent(juce::AudioBuffer<float>& destination, int numSamples) const;
    bool copyWindowEndingAt(juce::AudioBuffer<float>& destination, int numSamples, int64_t endSampleExclusive) const;

    int getNumChannels() const noexcept;
    int getCapacity() const noexcept;
    int64_t getTotalWrittenSamples() const noexcept;

private:
    int safeChannelCount() const noexcept;

    mutable juce::SpinLock lock;
    juce::AudioBuffer<float> storage;
    int writeIndex = 0;
    int64_t totalWrittenSamples = 0;
};

} // namespace wvfrm
