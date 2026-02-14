#include "dsp/AnalysisRingBuffer.h"

#include <iostream>

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

    return ok;
}
