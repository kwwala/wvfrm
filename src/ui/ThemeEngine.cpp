#include "ThemeEngine.h"

#include <cmath>

namespace wvfrm
{

juce::Colour ThemeEngine::colourFor(BandEnergies energies,
                                    ThemePreset theme,
                                    ColorMode mode,
                                    float intensityPercent,
                                    float amplitudeNorm) const noexcept
{
    const auto intensity = juce::jlimit(0.0f, 1.0f, intensityPercent / 100.0f);
    const auto amp = juce::jlimit(0.0f, 1.0f, amplitudeNorm);

    juce::Colour base = (mode == ColorMode::threeBand)
        ? blendThreeBand(energies, theme)
        : colourFromPreset(theme);

    auto saturationFloor = 0.35f;
    auto brightnessFloor = 0.45f;
    auto alphaFloor = (mode == ColorMode::threeBand) ? 0.35f : 0.2f;
    auto ampShaped = std::sqrt(amp);

    if (theme == ThemePreset::minimeters3Band)
    {
        saturationFloor = 0.6f;
        brightnessFloor = 0.55f;
        if (mode == ColorMode::threeBand)
            alphaFloor = 0.25f;
        ampShaped = std::pow(amp, 0.65f);
    }

    const auto saturation = juce::jlimit(0.05f, 1.0f, saturationFloor + (1.0f - saturationFloor) * intensity);
    const auto alpha = juce::jlimit(0.15f, 1.0f, alphaFloor + (1.0f - alphaFloor) * ampShaped);

    base = base.withMultipliedSaturation(saturation)
               .withMultipliedBrightness(brightnessFloor + (1.0f - brightnessFloor) * intensity)
               .withAlpha(alpha);

    return base;
}

juce::Colour ThemeEngine::colourFromPreset(ThemePreset theme) noexcept
{
    switch (theme)
    {
        case ThemePreset::minimeters3Band: return juce::Colour::fromRGB(255, 96, 96);
        case ThemePreset::rekordboxInspired: return juce::Colour::fromRGB(0, 240, 255);
        case ThemePreset::classicAmber: return juce::Colour::fromRGB(255, 190, 64);
        case ThemePreset::iceBlue: return juce::Colour::fromRGB(120, 210, 255);
        default: break;
    }

    return juce::Colours::white;
}

juce::Colour ThemeEngine::blendThreeBand(const BandEnergies& energies, ThemePreset theme) noexcept
{
    const auto low = juce::jlimit(0.0f, 1.0f, energies.low);
    const auto mid = juce::jlimit(0.0f, 1.0f, energies.mid);
    const auto high = juce::jlimit(0.0f, 1.0f, energies.high);

    if (theme == ThemePreset::minimeters3Band)
    {
        constexpr auto gamma = 1.7f;
        constexpr auto energyFloor = 0.03f;
        auto rEnergy = std::pow(low, gamma) + energyFloor;
        auto gEnergy = std::pow(mid, gamma) + energyFloor;
        auto bEnergy = std::pow(high, gamma) + energyFloor;
        const auto total = rEnergy + gEnergy + bEnergy;

        if (total > 1.0e-6f)
        {
            rEnergy /= total;
            gEnergy /= total;
            bEnergy /= total;
        }

        const auto r = juce::jlimit(0.0f, 1.0f, static_cast<float>(rEnergy));
        const auto g = juce::jlimit(0.0f, 1.0f, static_cast<float>(gEnergy));
        const auto b = juce::jlimit(0.0f, 1.0f, static_cast<float>(bEnergy));
        return juce::Colour::fromFloatRGBA(r, g, b, 1.0f);
    }

    if (theme == ThemePreset::rekordboxInspired)
    {
        const auto neonR = juce::jlimit(0, 255, static_cast<int>(40.0f + 180.0f * low + 80.0f * high));
        const auto neonG = juce::jlimit(0, 255, static_cast<int>(30.0f + 180.0f * mid + 70.0f * high));
        const auto neonB = juce::jlimit(0, 255, static_cast<int>(80.0f + 210.0f * high + 40.0f * mid));
        return juce::Colour::fromRGB(static_cast<juce::uint8>(neonR),
                                     static_cast<juce::uint8>(neonG),
                                     static_cast<juce::uint8>(neonB));
    }

    if (theme == ThemePreset::classicAmber)
    {
        const auto luminance = juce::jlimit(0.0f, 1.0f, 0.25f + low * 0.5f + mid * 0.25f);
        return juce::Colour::fromFloatRGBA(1.0f, 0.68f, 0.24f, 1.0f).withMultipliedBrightness(luminance);
    }

    const auto iceR = juce::jlimit(0, 255, static_cast<int>(80.0f + 80.0f * mid));
    const auto iceG = juce::jlimit(0, 255, static_cast<int>(150.0f + 90.0f * high));
    const auto iceB = juce::jlimit(0, 255, static_cast<int>(180.0f + 75.0f * high + 40.0f * low));
    return juce::Colour::fromRGB(static_cast<juce::uint8>(iceR),
                                 static_cast<juce::uint8>(iceG),
                                 static_cast<juce::uint8>(iceB));
}

} // namespace wvfrm
