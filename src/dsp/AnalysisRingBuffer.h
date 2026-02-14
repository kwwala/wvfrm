#pragma once

#include "../JuceIncludes.h"

#include <atomic>

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
    struct Snapshot
    {
        int channels = 0;
        int capacity = 0;
        int writeIndex = 0;
        int64_t totalWritten = 0;
    };

    bool readSnapshot(Snapshot& out) const noexcept;

    juce::AudioBuffer<float> storage;
    int writeIndex = 0;
    int64_t totalWrittenSamples = 0;
    mutable std::atomic<uint64_t> sequence { 0 };
};

} // namespace wvfrm
