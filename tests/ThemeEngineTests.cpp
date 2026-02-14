#include "ui/ThemeEngine.h"

#include <cmath>
#include <iostream>

namespace
{
float rgbDistance(const juce::Colour& a, const juce::Colour& b)
{
    const auto dr = a.getFloatRed() - b.getFloatRed();
    const auto dg = a.getFloatGreen() - b.getFloatGreen();
    const auto db = a.getFloatBlue() - b.getFloatBlue();
    return std::sqrt(dr * dr + dg * dg + db * db);
}
}

bool runThemeEngineTests()
{
    bool ok = true;
    wvfrm::ThemeEngine engine;

    constexpr auto intensity = 100.0f;
    constexpr auto amplitude = 1.0f;

    const wvfrm::BandEnergies lowHeavy { 0.95f, 0.20f, 0.10f };
    const wvfrm::BandEnergies midHeavy { 0.14f, 0.90f, 0.15f };
    const wvfrm::BandEnergies highHeavy { 0.10f, 0.24f, 0.92f };
    const wvfrm::BandEnergies lowMidMix { 0.95f, 0.92f, 0.08f };
    const wvfrm::BandEnergies midHighMix { 0.10f, 0.92f, 0.95f };
    const wvfrm::BandEnergies lowHighMix { 0.95f, 0.12f, 0.95f };
    const wvfrm::BandEnergies fullMix { 0.96f, 0.96f, 0.96f };
    const wvfrm::BandEnergies mixedBands { 0.42f, 0.43f, 0.15f };

    const auto lowColour = engine.colourFor(lowHeavy,
                                             wvfrm::ThemePreset::minimeters3Band,
                                             wvfrm::ColorMode::threeBand,
                                             intensity,
                                             amplitude,
                                             100.0f);
    const auto midColour = engine.colourFor(midHeavy,
                                             wvfrm::ThemePreset::minimeters3Band,
                                             wvfrm::ColorMode::threeBand,
                                             intensity,
                                             amplitude,
                                             100.0f);
    const auto highColour = engine.colourFor(highHeavy,
                                              wvfrm::ThemePreset::minimeters3Band,
                                              wvfrm::ColorMode::threeBand,
                                              intensity,
                                              amplitude,
                                              100.0f);

    if (! (lowColour.getFloatRed() > lowColour.getFloatGreen()
        && lowColour.getFloatRed() > lowColour.getFloatBlue()))
    {
        std::cerr << "ThemeEngine: low-heavy energy should be red-dominant." << std::endl;
        ok = false;
    }

    if (! (midColour.getFloatGreen() > midColour.getFloatRed()
        && midColour.getFloatGreen() > midColour.getFloatBlue()))
    {
        std::cerr << "ThemeEngine: mid-heavy energy should be green-dominant." << std::endl;
        ok = false;
    }

    if (! (highColour.getFloatBlue() > highColour.getFloatRed()
        && highColour.getFloatBlue() > highColour.getFloatGreen()))
    {
        std::cerr << "ThemeEngine: high-heavy energy should be blue-dominant." << std::endl;
        ok = false;
    }

    const auto yellowish = engine.colourFor(lowMidMix,
                                             wvfrm::ThemePreset::minimeters3Band,
                                             wvfrm::ColorMode::threeBand,
                                             intensity,
                                             amplitude,
                                             100.0f);
    if (! (yellowish.getFloatRed() > yellowish.getFloatBlue()
        && yellowish.getFloatGreen() > yellowish.getFloatBlue()))
    {
        std::cerr << "ThemeEngine: low+mid mix should move toward yellow (R+G)." << std::endl;
        ok = false;
    }

    const auto cyanish = engine.colourFor(midHighMix,
                                           wvfrm::ThemePreset::minimeters3Band,
                                           wvfrm::ColorMode::threeBand,
                                           intensity,
                                           amplitude,
                                           100.0f);
    if (! (cyanish.getFloatGreen() > cyanish.getFloatRed()
        && cyanish.getFloatBlue() > cyanish.getFloatRed()))
    {
        std::cerr << "ThemeEngine: mid+high mix should move toward cyan (G+B)." << std::endl;
        ok = false;
    }

    const auto magentaish = engine.colourFor(lowHighMix,
                                              wvfrm::ThemePreset::minimeters3Band,
                                              wvfrm::ColorMode::threeBand,
                                              intensity,
                                              amplitude,
                                              100.0f);
    if (! (magentaish.getFloatRed() > magentaish.getFloatGreen()
        && magentaish.getFloatBlue() > magentaish.getFloatGreen()))
    {
        std::cerr << "ThemeEngine: low+high mix should move toward magenta (R+B)." << std::endl;
        ok = false;
    }

    const auto whitish = engine.colourFor(fullMix,
                                           wvfrm::ThemePreset::minimeters3Band,
                                           wvfrm::ColorMode::threeBand,
                                           intensity,
                                           amplitude,
                                           100.0f);
    const auto maxDiff = juce::jmax(std::abs(whitish.getFloatRed() - whitish.getFloatGreen()),
                                    std::abs(whitish.getFloatRed() - whitish.getFloatBlue()));
    if (maxDiff > 0.12f)
    {
        std::cerr << "ThemeEngine: full-spectrum energy should approach white." << std::endl;
        ok = false;
    }

    const auto minimetersAtZero = engine.colourFor(mixedBands,
                                                    wvfrm::ThemePreset::minimeters3Band,
                                                    wvfrm::ColorMode::threeBand,
                                                    intensity,
                                                    amplitude,
                                                    0.0f);
    const auto rekordboxAtZero = engine.colourFor(mixedBands,
                                                   wvfrm::ThemePreset::rekordboxInspired,
                                                   wvfrm::ColorMode::threeBand,
                                                   intensity,
                                                   amplitude,
                                                   0.0f);
    const auto minimetersAtHundred = engine.colourFor(mixedBands,
                                                       wvfrm::ThemePreset::minimeters3Band,
                                                       wvfrm::ColorMode::threeBand,
                                                       intensity,
                                                       amplitude,
                                                       100.0f);
    const auto rekordboxAtHundred = engine.colourFor(mixedBands,
                                                      wvfrm::ThemePreset::rekordboxInspired,
                                                      wvfrm::ColorMode::threeBand,
                                                      intensity,
                                                      amplitude,
                                                      100.0f);

    if (rgbDistance(minimetersAtZero, rekordboxAtZero) < 0.02f)
    {
        std::cerr << "ThemeEngine: colorMatch=0 should keep preset styling distinct." << std::endl;
        ok = false;
    }

    if (rgbDistance(minimetersAtHundred, rekordboxAtHundred) > 0.015f)
    {
        std::cerr << "ThemeEngine: colorMatch=100 should converge presets to canonical RGB." << std::endl;
        ok = false;
    }

    const auto lowAmpColour = engine.colourFor(midHighMix,
                                                wvfrm::ThemePreset::minimeters3Band,
                                                wvfrm::ColorMode::threeBand,
                                                intensity,
                                                0.08f,
                                                100.0f);
    const auto highAmpColour = engine.colourFor(midHighMix,
                                                 wvfrm::ThemePreset::minimeters3Band,
                                                 wvfrm::ColorMode::threeBand,
                                                 intensity,
                                                 1.0f,
                                                 100.0f);

    if (! (highAmpColour.getFloatAlpha() > lowAmpColour.getFloatAlpha()))
    {
        std::cerr << "ThemeEngine: waveform amplitude should increase color alpha." << std::endl;
        ok = false;
    }

    if (! (highAmpColour.getBrightness() > lowAmpColour.getBrightness()))
    {
        std::cerr << "ThemeEngine: waveform amplitude should increase color brightness." << std::endl;
        ok = false;
    }

    return ok;
}
