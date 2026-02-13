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

    if (mode == ColorMode::threeBand)
    {
        // Increase chroma by emphasizing dominant bands before color blend.
        const auto vibrancePower = theme == ThemePreset::minimeters3Band
            ? juce::jmap(intensity, 0.0f, 1.0f, 1.12f, 1.38f)
            : juce::jmap(intensity, 0.0f, 1.0f, 1.06f, 1.24f);

        energies.low = std::pow(juce::jlimit(0.0f, 1.0f, energies.low), vibrancePower);
        energies.mid = std::pow(juce::jlimit(0.0f, 1.0f, energies.mid), vibrancePower);
        energies.high = std::pow(juce::jlimit(0.0f, 1.0f, energies.high), vibrancePower);

        const auto sum = energies.low + energies.mid + energies.high;
        if (sum > 1.0e-6f)
        {
            energies.low /= sum;
            energies.mid /= sum;
            energies.high /= sum;
        }
    }

    juce::Colour base = (mode == ColorMode::threeBand)
        ? blendThreeBand(energies, theme)
        : colourFromPreset(theme);

    auto alphaFloor = (mode == ColorMode::threeBand) ? 0.35f : 0.2f;
    auto ampShaped = std::sqrt(amp);
    float saturation = 0.0f;
    float brightness = 0.0f;

    if (theme == ThemePreset::minimeters3Band)
    {
        saturation = juce::jlimit(0.0f, 1.45f, 1.02f + 0.36f * intensity);
        brightness = juce::jlimit(0.0f, 1.0f, 0.82f + 0.14f * intensity);
        if (mode == ColorMode::threeBand)
            alphaFloor = 0.2f;
        ampShaped = std::pow(amp, 0.75f);
    }
    else
    {
        const auto saturationFloor = 0.35f;
        const auto brightnessFloor = 0.45f;
        if (mode == ColorMode::threeBand)
        {
            saturation = juce::jlimit(0.05f, 1.35f, 0.88f + 0.34f * intensity);
            brightness = juce::jlimit(0.0f, 1.0f, 0.74f + 0.20f * intensity);
        }
        else
        {
            saturation = juce::jlimit(0.05f, 1.0f, saturationFloor + (1.0f - saturationFloor) * intensity);
            brightness = juce::jlimit(0.0f, 1.0f, brightnessFloor + (1.0f - brightnessFloor) * intensity);
        }
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
        // Preserve the true low/mid/high proportion for a MiniMeters-like read.
        return juce::Colour::fromFloatRGBA(low, mid, high, 1.0f);
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
