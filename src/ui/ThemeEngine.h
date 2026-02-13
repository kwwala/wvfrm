#pragma once

#include "../JuceIncludes.h"
#include "../Parameters.h"
#include "../dsp/BandAnalyzer3.h"

namespace wvfrm
{

class ThemeEngine
{
public:
    juce::Colour colourFor(BandEnergies energies,
                           ThemePreset theme,
                           ColorMode mode,
                           float intensityPercent,
                           float amplitudeNorm,
                           float colorMatchPercent) const noexcept;

private:
    static juce::Colour colourFromPreset(ThemePreset theme) noexcept;
    static juce::Colour blendThreeBand(const BandEnergies& energies, ThemePreset theme) noexcept;
    static juce::Colour blendMiniMetersWarm(const BandEnergies& energies) noexcept;
};

} // namespace wvfrm
