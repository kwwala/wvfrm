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

    auto alphaFloor = (mode == ColorMode::threeBand) ? 0.35f : 0.2f;
    auto ampShaped = std::sqrt(amp);
    float saturation = 0.0f;
    float brightness = 0.0f;

    if (theme == ThemePreset::minimeters3Band)
    {
        saturation = juce::jlimit(0.0f, 1.0f, 0.85f + 0.15f * intensity);
        brightness = juce::jlimit(0.0f, 1.0f, 0.9f + 0.1f * intensity);
        if (mode == ColorMode::threeBand)
            alphaFloor = 0.25f;
        ampShaped = std::pow(amp, 0.65f);
    }
    else
    {
        const auto saturationFloor = 0.35f;
        const auto brightnessFloor = 0.45f;
        saturation = juce::jlimit(0.05f, 1.0f, saturationFloor + (1.0f - saturationFloor) * intensity);
        brightness = juce::jlimit(0.0f, 1.0f, brightnessFloor + (1.0f - brightnessFloor) * intensity);
    }

    const auto alpha = juce::jlimit(0.15f, 1.0f, alphaFloor + (1.0f - alphaFloor) * ampShaped);

    base = base.withMultipliedSaturation(saturation)
               .withMultipliedBrightness(brightness)
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
        auto r = juce::jlimit(0.0f, 1.0f, low);
        auto g = juce::jlimit(0.0f, 1.0f, mid);
        auto b = juce::jlimit(0.0f, 1.0f, high);

        const auto maxComp = juce::jmax(r, juce::jmax(g, b));
        if (maxComp > 1.0e-6f)
        {
            constexpr auto targetBrightness = 0.95f;
            const auto scale = targetBrightness / maxComp;
            r = juce::jlimit(0.0f, 1.0f, r * scale);
            g = juce::jlimit(0.0f, 1.0f, g * scale);
            b = juce::jlimit(0.0f, 1.0f, b * scale);
        }

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
