#pragma once

#include "../Parameters.h"

namespace wvfrm
{

inline float sampleForChannelMode(ChannelMode mode, float left, float right) noexcept
{
    switch (mode)
    {
        case ChannelMode::left: return left;
        case ChannelMode::right: return right;
        case ChannelMode::mid: return 0.5f * (left + right);
        case ChannelMode::side: return 0.5f * (left - right);
        default: break;
    }

    return left;
}

} // namespace wvfrm
