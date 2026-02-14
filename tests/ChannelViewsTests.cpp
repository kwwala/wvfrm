#include "dsp/ChannelViews.h"

#include <cmath>
#include <iostream>

namespace
{
bool nearlyEqual(float a, float b, float tolerance = 1.0e-6f)
{
    return std::abs(a - b) <= tolerance;
}
}

bool runChannelViewsTests()
{
    bool ok = true;

    const auto left = 0.8f;
    const auto right = -0.2f;

    if (! nearlyEqual(wvfrm::sampleForChannelMode(wvfrm::ChannelMode::mid, left, right), 0.3f))
    {
        std::cerr << "ChannelMode: mid should be (L+R)/2." << std::endl;
        ok = false;
    }

    if (! nearlyEqual(wvfrm::sampleForChannelMode(wvfrm::ChannelMode::side, left, right), 0.5f))
    {
        std::cerr << "ChannelMode: side should be (L-R)/2." << std::endl;
        ok = false;
    }

    if (! nearlyEqual(wvfrm::sampleForChannelMode(wvfrm::ChannelMode::left, left, right), left))
    {
        std::cerr << "ChannelMode: left should return L." << std::endl;
        ok = false;
    }

    if (! nearlyEqual(wvfrm::sampleForChannelMode(wvfrm::ChannelMode::right, left, right), right))
    {
        std::cerr << "ChannelMode: right should return R." << std::endl;
        ok = false;
    }

    return ok;
}
