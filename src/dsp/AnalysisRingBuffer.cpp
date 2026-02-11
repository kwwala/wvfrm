#include "AnalysisRingBuffer.h"

namespace wvfrm
{

void AnalysisRingBuffer::prepare(int channels, int samplesPerChannel)
{
    const juce::SpinLock::ScopedLockType scopedLock(lock);
    storage.setSize(juce::jmax(1, channels), juce::jmax(1, samplesPerChannel), false, true, true);
    storage.clear();
    writeIndex = 0;
}

void AnalysisRingBuffer::clear()
{
    const juce::SpinLock::ScopedLockType scopedLock(lock);
    storage.clear();
    writeIndex = 0;
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
}

bool AnalysisRingBuffer::copyMostRecent(juce::AudioBuffer<float>& destination, int numSamples) const
{
    const juce::SpinLock::ScopedLockType scopedLock(lock);

    const auto channels = safeChannelCount();
    const auto capacity = storage.getNumSamples();

    if (channels <= 0 || capacity <= 0)
        return false;

    const auto samplesToCopy = juce::jlimit(1, capacity, numSamples);
    destination.setSize(channels, samplesToCopy, false, true, true);

    auto readIndex = writeIndex - samplesToCopy;
    if (readIndex < 0)
        readIndex += capacity;

    for (int sample = 0; sample < samplesToCopy; ++sample)
    {
        const auto ringIndex = (readIndex + sample) % capacity;

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

int AnalysisRingBuffer::safeChannelCount() const noexcept
{
    return juce::jmax(storage.getNumChannels(), 1);
}

} // namespace wvfrm
