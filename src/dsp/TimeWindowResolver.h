#pragma once

#include "../JuceIncludes.h"
#include <array>
#include <optional>

namespace wvfrm
{

class TimeWindowResolver
{
public:
    struct SyncDivision
    {
        int numerator;
        int denominator;
        const char* label;
    };

    struct ResolvedWindow
    {
        double ms = 1000.0;
        bool tempoReliable = false;
        double bpmUsed = 120.0;
    };

    static constexpr std::array<SyncDivision, 9> divisions {
        SyncDivision { 1, 64, "1/64" },
        SyncDivision { 1, 32, "1/32" },
        SyncDivision { 1, 16, "1/16" },
        SyncDivision { 1, 8,  "1/8"  },
        SyncDivision { 1, 4,  "1/4"  },
        SyncDivision { 1, 2,  "1/2"  },
        SyncDivision { 1, 1,  "1/1"  },
        SyncDivision { 2, 1,  "2/1"  },
        SyncDivision { 4, 1,  "4/1"  }
    };

    static double divisionToMs(int divisionIndex, double bpm) noexcept;

    static ResolvedWindow resolve(bool syncMode,
                                  int divisionIndex,
                                  double milliseconds,
                                  std::optional<double> hostBpm,
                                  double lastKnownBpm) noexcept;
};

} // namespace wvfrm
