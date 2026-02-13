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

    const wvfrm::BandEnergies mixedBands { 0.42f, 0.43f, 0.15f };
    constexpr auto intensity = 100.0f;
    constexpr auto amplitude = 1.0f;

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
    const auto classicAtZero = engine.colourFor(mixedBands,
                                                wvfrm::ThemePreset::classicAmber,
                                                wvfrm::ColorMode::threeBand,
                                                intensity,
                                                amplitude,
                                                0.0f);

    if (rgbDistance(minimetersAtZero, rekordboxAtZero) < 0.02f
        || rgbDistance(minimetersAtZero, classicAtZero) < 0.02f)
    {
        std::cerr << "ThemeEngine: colorMatch=0 should preserve distinct preset colors." << std::endl;
        ok = false;
    }

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
    const auto classicAtHundred = engine.colourFor(mixedBands,
                                                   wvfrm::ThemePreset::classicAmber,
                                                   wvfrm::ColorMode::threeBand,
                                                   intensity,
                                                   amplitude,
                                                   100.0f);

    if (rgbDistance(minimetersAtHundred, rekordboxAtHundred) > 0.01f
        || rgbDistance(minimetersAtHundred, classicAtHundred) > 0.01f)
    {
        std::cerr << "ThemeEngine: colorMatch=100 should converge 3-band presets with small tolerance." << std::endl;
        ok = false;
    }

    const auto classicAtFifty = engine.colourFor(mixedBands,
                                                  wvfrm::ThemePreset::classicAmber,
                                                  wvfrm::ColorMode::threeBand,
                                                  intensity,
                                                  amplitude,
                                                  50.0f);
    const auto pinkTarget = juce::Colour::fromFloatRGBA(0.96f, 0.36f, 0.56f, 1.0f);
    const auto distZero = rgbDistance(classicAtZero, pinkTarget);
    const auto distFifty = rgbDistance(classicAtFifty, pinkTarget);
    const auto distHundred = rgbDistance(classicAtHundred, pinkTarget);

    if (! (distHundred < distZero && distFifty < distZero))
    {
        std::cerr << "ThemeEngine: increasing colorMatch should move colors toward warm pink/red target." << std::endl;
        ok = false;
    }

    const wvfrm::BandEnergies lowHeavy { 0.76f, 0.18f, 0.06f };
    const wvfrm::BandEnergies midHeavy { 0.14f, 0.74f, 0.12f };
    const wvfrm::BandEnergies highHeavy { 0.08f, 0.20f, 0.72f };

    const auto lowAtHundred = engine.colourFor(lowHeavy,
                                                wvfrm::ThemePreset::classicAmber,
                                                wvfrm::ColorMode::threeBand,
                                                intensity,
                                                amplitude,
                                                100.0f);
    const auto midAtHundred = engine.colourFor(midHeavy,
                                                wvfrm::ThemePreset::classicAmber,
                                                wvfrm::ColorMode::threeBand,
                                                intensity,
                                                amplitude,
                                                100.0f);
    const auto highAtHundred = engine.colourFor(highHeavy,
                                                 wvfrm::ThemePreset::classicAmber,
                                                 wvfrm::ColorMode::threeBand,
                                                 intensity,
                                                 amplitude,
                                                 100.0f);

    if (rgbDistance(lowAtHundred, midAtHundred) <= 0.12f
        || rgbDistance(lowAtHundred, highAtHundred) <= 0.12f
        || rgbDistance(midAtHundred, highAtHundred) <= 0.12f)
    {
        std::cerr << "ThemeEngine: colorMatch=100 should preserve spectral color diversity across low/mid/high inputs." << std::endl;
        ok = false;
    }

    const auto mixedAtHundred = engine.colourFor(mixedBands,
                                                  wvfrm::ThemePreset::classicAmber,
                                                  wvfrm::ColorMode::threeBand,
                                                  intensity,
                                                  amplitude,
                                                  100.0f);
    const auto blueTarget = juce::Colour::fromFloatRGBA(0.20f, 0.78f, 1.00f, 1.0f);
    const auto mixedDistPink = rgbDistance(mixedAtHundred, pinkTarget);
    const auto mixedDistBlue = rgbDistance(mixedAtHundred, blueTarget);

    if (! (mixedDistPink < mixedDistBlue && mixedDistPink > 0.08f))
    {
        std::cerr << "ThemeEngine: mixed 100% color should stay pink-dominant but not collapse to a single magenta tone." << std::endl;
        ok = false;
    }

    return ok;
}
