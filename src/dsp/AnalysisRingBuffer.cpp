#include "AnalysisRingBuffer.h"

namespace wvfrm
{

void AnalysisRingBuffer::prepare(int channels, int samplesPerChannel)
{
    sequence.fetch_add(1, std::memory_order_acq_rel);
    storage.setSize(juce::jmax(1, channels), juce::jmax(1, samplesPerChannel), false, true, true);
    storage.clear();
    writeIndex = 0;
    totalWrittenSamples = 0;
    sequence.fetch_add(1, std::memory_order_release);
}

void AnalysisRingBuffer::clear()
{
    sequence.fetch_add(1, std::memory_order_acq_rel);
    storage.clear();
    writeIndex = 0;
    totalWrittenSamples = 0;
    sequence.fetch_add(1, std::memory_order_release);
}

void AnalysisRingBuffer::pushBuffer(const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channels = juce::jmin(storage.getNumChannels(), buffer.getNumChannels());
    const auto capacity = storage.getNumSamples();

    if (channels <= 0 || capacity <= 0)
        return;

    sequence.fetch_add(1, std::memory_order_acq_rel);

    const auto numSamples = buffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto ringIndex = (writeIndex + sample) % capacity;

        for (int channel = 0; channel < channels; ++channel)
            storage.setSample(channel, ringIndex, buffer.getSample(channel, sample));
    }

    writeIndex = (writeIndex + numSamples) % capacity;
    totalWrittenSamples += numSamples;

    sequence.fetch_add(1, std::memory_order_release);
}

bool AnalysisRingBuffer::copyMostRecent(juce::AudioBuffer<float>& destination, int numSamples) const
{
    Snapshot snapshot;
    if (! readSnapshot(snapshot))
        return false;

    return copyWindowEndingAt(destination, numSamples, snapshot.totalWritten);
}

bool AnalysisRingBuffer::copyWindowEndingAt(juce::AudioBuffer<float>& destination,
                                            int numSamples,
                                            int64_t endSampleExclusive) const
{
    for (int attempt = 0; attempt < 12; ++attempt)
    {
        const auto seqBegin = sequence.load(std::memory_order_acquire);
        if ((seqBegin & 1u) != 0u)
            continue;

        const auto channels = storage.getNumChannels();
        const auto capacity = storage.getNumSamples();
        const auto latestEnd = totalWrittenSamples;
        const auto localWriteIndex = writeIndex;
        juce::ignoreUnused(localWriteIndex);

        if (channels <= 0 || capacity <= 0)
            return false;

        const auto earliestAvailable = juce::jmax<int64_t>(0, latestEnd - capacity);
        const auto requestedEnd = juce::jlimit<int64_t>(earliestAvailable, latestEnd, endSampleExclusive);

        if (requestedEnd <= earliestAvailable)
            return false;

        auto samplesToCopy = juce::jlimit(1, capacity, numSamples);
        auto absoluteStart = requestedEnd - samplesToCopy;

        if (absoluteStart < earliestAvailable)
        {
            absoluteStart = earliestAvailable;
            samplesToCopy = static_cast<int>(requestedEnd - absoluteStart);
        }

        if (samplesToCopy <= 0)
            return false;

        destination.setSize(channels, samplesToCopy, false, true, true);

        for (int sample = 0; sample < samplesToCopy; ++sample)
        {
            const auto absoluteIndex = absoluteStart + sample;
            const auto ringIndex = static_cast<int>(absoluteIndex % static_cast<int64_t>(capacity));

            for (int channel = 0; channel < channels; ++channel)
                destination.setSample(channel, sample, storage.getSample(channel, ringIndex));
        }

        const auto seqEnd = sequence.load(std::memory_order_acquire);
        if (seqBegin == seqEnd)
            return true;
    }

    return false;
}

int AnalysisRingBuffer::getNumChannels() const noexcept
{
    Snapshot snapshot;
    if (! readSnapshot(snapshot))
        return 0;

    return snapshot.channels;
}

int AnalysisRingBuffer::getCapacity() const noexcept
{
    Snapshot snapshot;
    if (! readSnapshot(snapshot))
        return 0;

    return snapshot.capacity;
}

int64_t AnalysisRingBuffer::getTotalWrittenSamples() const noexcept
{
    Snapshot snapshot;
    if (! readSnapshot(snapshot))
        return 0;

    return snapshot.totalWritten;
}

bool AnalysisRingBuffer::readSnapshot(Snapshot& out) const noexcept
{
    for (int attempt = 0; attempt < 12; ++attempt)
    {
        const auto seqBegin = sequence.load(std::memory_order_acquire);
        if ((seqBegin & 1u) != 0u)
            continue;

        out.channels = storage.getNumChannels();
        out.capacity = storage.getNumSamples();
        out.writeIndex = writeIndex;
        out.totalWritten = totalWrittenSamples;

        const auto seqEnd = sequence.load(std::memory_order_acquire);
        if (seqBegin == seqEnd)
            return true;
    }

    return false;
}

} // namespace wvfrm
