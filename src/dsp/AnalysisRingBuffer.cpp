#include "AnalysisRingBuffer.h"

namespace wvfrm
{

void AnalysisRingBuffer::prepare(int channels, int samplesPerChannel)
{
    sequence.fetch_add(1, std::memory_order_acq_rel); // begin write (odd)
    storage.setSize(juce::jmax(1, channels), juce::jmax(1, samplesPerChannel), false, true, true);
    storage.clear();
    writeIndex.store(0, std::memory_order_relaxed);
    totalWrittenSamples.store(0, std::memory_order_relaxed);
    sequence.fetch_add(1, std::memory_order_release); // end write (even)
}

void AnalysisRingBuffer::clear()
{
    sequence.fetch_add(1, std::memory_order_acq_rel); // begin write (odd)
    storage.clear();
    writeIndex.store(0, std::memory_order_relaxed);
    totalWrittenSamples.store(0, std::memory_order_relaxed);
    sequence.fetch_add(1, std::memory_order_release); // end write (even)
}

void AnalysisRingBuffer::pushBuffer(const juce::AudioBuffer<float>& buffer) noexcept
{
    const auto channels = juce::jmin(storage.getNumChannels(), buffer.getNumChannels());
    const auto capacity = storage.getNumSamples();

    if (channels <= 0 || capacity <= 0)
        return;

    const auto numSamples = buffer.getNumSamples();
    if (numSamples <= 0)
        return;

    sequence.fetch_add(1, std::memory_order_acq_rel); // begin write (odd)
    const auto localWriteIndex = writeIndex.load(std::memory_order_relaxed);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto ringIndex = (localWriteIndex + sample) % capacity;

        for (int channel = 0; channel < channels; ++channel)
            storage.setSample(channel, ringIndex, buffer.getSample(channel, sample));
    }

    writeIndex.store((localWriteIndex + numSamples) % capacity, std::memory_order_relaxed);
    const auto writtenSamples = totalWrittenSamples.load(std::memory_order_relaxed);
    totalWrittenSamples.store(writtenSamples + numSamples, std::memory_order_relaxed);
    sequence.fetch_add(1, std::memory_order_release); // end write (even)
}

bool AnalysisRingBuffer::copyMostRecent(juce::AudioBuffer<float>& destination, int numSamples) const
{
    return copyWindowEndingAt(destination, numSamples, getTotalWrittenSamples());
}

bool AnalysisRingBuffer::copyWindowEndingAt(juce::AudioBuffer<float>& destination,
                                            int numSamples,
                                            int64_t endSampleExclusive) const
{
    for (int attempt = 0; attempt < 16; ++attempt)
    {
        const auto seqBegin = sequence.load(std::memory_order_acquire);
        if ((seqBegin & 1u) != 0u)
            continue;

        const auto channels = safeChannelCount();
        const auto capacity = storage.getNumSamples();
        const auto latestEnd = totalWrittenSamples.load(std::memory_order_relaxed);

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
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        const auto seqBegin = sequence.load(std::memory_order_acquire);
        if ((seqBegin & 1u) != 0u)
            continue;

        const auto channels = storage.getNumChannels();
        const auto seqEnd = sequence.load(std::memory_order_acquire);

        if (seqBegin == seqEnd)
            return channels;
    }

    return storage.getNumChannels();
}

int AnalysisRingBuffer::getCapacity() const noexcept
{
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        const auto seqBegin = sequence.load(std::memory_order_acquire);
        if ((seqBegin & 1u) != 0u)
            continue;

        const auto capacity = storage.getNumSamples();
        const auto seqEnd = sequence.load(std::memory_order_acquire);

        if (seqBegin == seqEnd)
            return capacity;
    }

    return storage.getNumSamples();
}

int64_t AnalysisRingBuffer::getTotalWrittenSamples() const noexcept
{
    return totalWrittenSamples.load(std::memory_order_acquire);
}

int AnalysisRingBuffer::safeChannelCount() const noexcept
{
    return juce::jmax(storage.getNumChannels(), 1);
}

} // namespace wvfrm
