#include "dsp/AnalysisRingBuffer.h"

#include <atomic>
#include <cmath>
#include <iostream>
#include <thread>

namespace
{
bool isContiguousAscending(const juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumSamples() <= 1)
        return true;

    for (int i = 1; i < buffer.getNumSamples(); ++i)
    {
        const auto expected = buffer.getSample(0, i - 1) + 1.0f;
        if (std::abs(buffer.getSample(0, i) - expected) > 1.0e-4f)
            return false;
    }

    return true;
}
}

bool runAnalysisRingBufferTests()
{
    bool ok = true;

    wvfrm::AnalysisRingBuffer ring;
    ring.prepare(1, 8);

    juce::AudioBuffer<float> block1(1, 5);
    for (int i = 0; i < 5; ++i)
        block1.setSample(0, i, static_cast<float>(i + 1));
    ring.pushBuffer(block1);

    juce::AudioBuffer<float> out;
    if (! ring.copyWindowEndingAt(out, 4, 5))
    {
        std::cerr << "AnalysisRingBuffer: expected copy at end sample 5." << std::endl;
        ok = false;
    }
    else if (out.getNumSamples() != 4
             || out.getSample(0, 0) != 2.0f
             || out.getSample(0, 1) != 3.0f
             || out.getSample(0, 2) != 4.0f
             || out.getSample(0, 3) != 5.0f)
    {
        std::cerr << "AnalysisRingBuffer: incorrect copy contents at end sample 5." << std::endl;
        ok = false;
    }

    juce::AudioBuffer<float> block2(1, 6);
    for (int i = 0; i < 6; ++i)
        block2.setSample(0, i, static_cast<float>(i + 6));
    ring.pushBuffer(block2);

    if (! ring.copyWindowEndingAt(out, 4, 11))
    {
        std::cerr << "AnalysisRingBuffer: expected copy at end sample 11." << std::endl;
        ok = false;
    }
    else if (out.getNumSamples() != 4
             || out.getSample(0, 0) != 8.0f
             || out.getSample(0, 1) != 9.0f
             || out.getSample(0, 2) != 10.0f
             || out.getSample(0, 3) != 11.0f)
    {
        std::cerr << "AnalysisRingBuffer: incorrect copy contents at end sample 11." << std::endl;
        ok = false;
    }

    if (! ring.copyWindowEndingAt(out, 4, 7))
    {
        std::cerr << "AnalysisRingBuffer: expected clamped copy at end sample 7." << std::endl;
        ok = false;
    }
    else if (out.getNumSamples() != 4
             || out.getSample(0, 0) != 4.0f
             || out.getSample(0, 1) != 5.0f
             || out.getSample(0, 2) != 6.0f
             || out.getSample(0, 3) != 7.0f)
    {
        std::cerr << "AnalysisRingBuffer: expected window ending at sample 7 to be [4..7]." << std::endl;
        ok = false;
    }

    {
        wvfrm::AnalysisRingBuffer concurrentRing;
        concurrentRing.prepare(1, 512);

        std::atomic<bool> writerDone { false };
        std::atomic<int64_t> samplesProduced { 0 };

        std::thread writer([&]()
        {
            juce::AudioBuffer<float> block(1, 16);
            float value = 1.0f;

            for (int chunk = 0; chunk < 2000; ++chunk)
            {
                for (int s = 0; s < 16; ++s)
                    block.setSample(0, s, value++);

                concurrentRing.pushBuffer(block);
                samplesProduced.store((chunk + 1) * 16, std::memory_order_release);
            }

            writerDone.store(true, std::memory_order_release);
        });

        juce::AudioBuffer<float> snapshot;
        for (int i = 0; i < 12000 && ! writerDone.load(std::memory_order_acquire); ++i)
        {
            const auto end = concurrentRing.getTotalWrittenSamples();
            if (end < 32)
                continue;

            if (concurrentRing.copyWindowEndingAt(snapshot, 32, end))
            {
                if (! isContiguousAscending(snapshot))
                {
                    std::cerr << "AnalysisRingBuffer: snapshot lost sample contiguity under concurrent read/write." << std::endl;
                    ok = false;
                    break;
                }
            }
        }

        writer.join();

        const auto expectedProduced = samplesProduced.load(std::memory_order_acquire);
        const auto actualProduced = concurrentRing.getTotalWrittenSamples();
        if (actualProduced != expectedProduced)
        {
            std::cerr << "AnalysisRingBuffer: writer dropped samples under contention." << std::endl;
            ok = false;
        }

        if (actualProduced >= 64)
        {
            if (! concurrentRing.copyWindowEndingAt(snapshot, 64, actualProduced))
            {
                std::cerr << "AnalysisRingBuffer: failed to copy final concurrent snapshot." << std::endl;
                ok = false;
            }
            else if (! isContiguousAscending(snapshot))
            {
                std::cerr << "AnalysisRingBuffer: final concurrent snapshot is not contiguous." << std::endl;
                ok = false;
            }
        }

    }

    return ok;
}
