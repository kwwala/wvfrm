#include "ThemeEngine.h"

#include <cmath>

namespace wvfrm
{

namespace
{
float normalizeBand(float value) noexcept
{
    return juce::jlimit(0.0f, 1.0f, value);
}

BandEnergies normalizeEnergies(BandEnergies energies) noexcept
{
    energies.low = normalizeBand(energies.low);
    energies.mid = normalizeBand(energies.mid);
    energies.high = normalizeBand(energies.high);
    return energies;
}
}

juce::Colour ThemeEngine::colourFor(BandEnergies energies,
                                    ThemePreset theme,
                                    ColorMode mode,
                                    float intensityPercent,
                                    float amplitudeNorm,
                                    float colorMatchPercent) const noexcept
{
    const auto intensity = juce::jlimit(0.0f, 1.0f, intensityPercent / 100.0f);
    const auto amp = juce::jlimit(0.0f, 1.0f, amplitudeNorm);
    const auto colorMatch = juce::jlimit(0.0f, 1.0f, colorMatchPercent / 100.0f);
    const auto ampShaped = std::sqrt(amp);

    if (mode == ColorMode::threeBand)
    {
        energies = normalizeEnergies(energies);

        const auto canonical = blendThreeBand(energies, ThemePreset::minimeters3Band);
        const auto styled = blendThreeBand(energies, theme);
        auto base = styled.interpolatedWith(canonical, colorMatch);

        const auto saturation = juce::jlimit(0.7f, 1.4f, 0.9f + 0.45f * intensity);
        const auto brightness = juce::jlimit(0.05f, 1.0f, (0.62f + 0.38f * intensity) * (0.45f + 0.60f * ampShaped));
        const auto alpha = juce::jlimit(0.1f, 1.0f, 0.12f + 0.88f * ampShaped);

        return base.withMultipliedSaturation(saturation)
                   .withMultipliedBrightness(brightness)
                   .withAlpha(alpha);
    }

    auto base = colourFromPreset(theme);
    const auto saturation = juce::jlimit(0.1f, 1.0f, 0.35f + 0.65f * intensity);
    const auto brightness = juce::jlimit(0.05f, 1.0f, (0.45f + 0.55f * intensity) * (0.5f + 0.5f * ampShaped));
    const auto alpha = juce::jlimit(0.12f, 1.0f, 0.2f + 0.8f * ampShaped);

    return base.withMultipliedSaturation(saturation)
               .withMultipliedBrightness(brightness)
               .withAlpha(alpha);
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
    const auto low = normalizeBand(energies.low);
    const auto mid = normalizeBand(energies.mid);
    const auto high = normalizeBand(energies.high);

    if (theme == ThemePreset::minimeters3Band)
        return juce::Colour::fromFloatRGBA(low, mid, high, 1.0f);

    if (theme == ThemePreset::rekordboxInspired)
    {
        const auto neonR = juce::jlimit(0.0f, 1.0f, 0.14f + 0.68f * low + 0.22f * high);
        const auto neonG = juce::jlimit(0.0f, 1.0f, 0.10f + 0.70f * mid + 0.28f * high);
        const auto neonB = juce::jlimit(0.0f, 1.0f, 0.24f + 0.80f * high + 0.16f * mid);
        return juce::Colour::fromFloatRGBA(neonR, neonG, neonB, 1.0f);
    }

    if (theme == ThemePreset::classicAmber)
    {
        const auto amberR = juce::jlimit(0.0f, 1.0f, 0.45f + 0.52f * low + 0.16f * mid);
        const auto amberG = juce::jlimit(0.0f, 1.0f, 0.18f + 0.58f * mid + 0.15f * high);
        const auto amberB = juce::jlimit(0.0f, 1.0f, 0.05f + 0.34f * high);
        return juce::Colour::fromFloatRGBA(amberR, amberG, amberB, 1.0f);
    }

    const auto iceR = juce::jlimit(0.0f, 1.0f, 0.18f + 0.30f * low + 0.25f * mid);
    const auto iceG = juce::jlimit(0.0f, 1.0f, 0.38f + 0.42f * mid + 0.22f * high);
    const auto iceB = juce::jlimit(0.0f, 1.0f, 0.52f + 0.42f * high + 0.18f * low);
    return juce::Colour::fromFloatRGBA(iceR, iceG, iceB, 1.0f);
}

} // namespace wvfrm
