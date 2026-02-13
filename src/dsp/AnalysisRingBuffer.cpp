#include "AnalysisRingBuffer.h"

namespace wvfrm
{

void AnalysisRingBuffer::prepare(int channels, int samplesPerChannel)
{
    const juce::SpinLock::ScopedLockType scopedLock(lock);
    storage.setSize(juce::jmax(1, channels), juce::jmax(1, samplesPerChannel), false, true, true);
    storage.clear();
    writeIndex = 0;
    totalWrittenSamples = 0;
}

void AnalysisRingBuffer::clear()
{
    const juce::SpinLock::ScopedLockType scopedLock(lock);
    storage.clear();
    writeIndex = 0;
    totalWrittenSamples = 0;
}

void AnalysisRingBuffer::pushBuffer(const juce::AudioBuffer<float>& buffer) noexcept
{
    const juce::SpinLock::ScopedTryLockType scopedLock(lock);

    if (! scopedLock.isLocked())
        return;

    const auto channels = juce::jmin(storage.getNumChannels(), buffer.getNumChannels());
    const auto capacity = storage.getNumSamples();

    if (channels <= 0 || capacity <= 0)
        return;

    const auto numSamples = buffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto ringIndex = (writeIndex + sample) % capacity;

        for (int channel = 0; channel < channels; ++channel)
            storage.setSample(channel, ringIndex, buffer.getSample(channel, sample));
    }

    writeIndex = (writeIndex + numSamples) % capacity;
    totalWrittenSamples += numSamples;
}

bool AnalysisRingBuffer::copyMostRecent(juce::AudioBuffer<float>& destination, int numSamples) const
{
    return copyWindowEndingAt(destination, numSamples, getTotalWrittenSamples());
}

bool AnalysisRingBuffer::copyWindowEndingAt(juce::AudioBuffer<float>& destination,
                                            int numSamples,
                                            int64_t endSampleExclusive) const
{
    const juce::SpinLock::ScopedLockType scopedLock(lock);

    const auto channels = safeChannelCount();
    const auto capacity = storage.getNumSamples();

    if (channels <= 0 || capacity <= 0)
        return false;

    const auto latestEnd = totalWrittenSamples;
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

    return true;
}

int AnalysisRingBuffer::getNumChannels() const noexcept
{
    const juce::SpinLock::ScopedLockType scopedLock(lock);
    return storage.getNumChannels();
}

int AnalysisRingBuffer::getCapacity() const noexcept
{
    const juce::SpinLock::ScopedLockType scopedLock(lock);
    return storage.getNumSamples();
}

int64_t AnalysisRingBuffer::getTotalWrittenSamples() const noexcept
{
    const juce::SpinLock::ScopedLockType scopedLock(lock);
    return totalWrittenSamples;
}

int AnalysisRingBuffer::safeChannelCount() const noexcept
{
    return juce::jmax(storage.getNumChannels(), 1);
}

} // namespace wvfrm
