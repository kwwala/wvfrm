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

    if (! nearlyEqual(wvfrm::mixForChannelView(wvfrm::ChannelView::mono, left, right), 0.3f))
    {
        std::cerr << "ChannelView: mono should be (L+R)/2." << std::endl;
        ok = false;
    }

    if (! nearlyEqual(wvfrm::mixForChannelView(wvfrm::ChannelView::mid, left, right), 0.3f))
    {
        std::cerr << "ChannelView: mid should be (L+R)/2." << std::endl;
        ok = false;
    }

    if (! nearlyEqual(wvfrm::mixForChannelView(wvfrm::ChannelView::side, left, right), 0.5f))
    {
        std::cerr << "ChannelView: side should be (L-R)/2." << std::endl;
        ok = false;
    }

    return ok;
}
