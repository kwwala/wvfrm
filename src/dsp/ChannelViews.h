#pragma once

#include "../Parameters.h"

namespace wvfrm
{

inline float mixForChannelView(ChannelView view, float left, float right) noexcept
{
    switch (view)
    {
        case ChannelView::left: return left;
        case ChannelView::right: return right;
        case ChannelView::mono: return 0.5f * (left + right);
        case ChannelView::mid: return 0.5f * (left + right);
        case ChannelView::side: return 0.5f * (left - right);
        case ChannelView::lrSplit: return left;
        default: break;
    }

    return left;
}

} // namespace wvfrm
