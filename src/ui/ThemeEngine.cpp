#include "ThemeEngine.h"

#include <cmath>

namespace wvfrm
{

namespace
{
float lerpFloat(float from, float to, float amount) noexcept
{
    return from + (to - from) * amount;
}

BandEnergies normalizeEnergies(BandEnergies energies) noexcept
{
    energies.low = juce::jlimit(0.0f, 1.0f, energies.low);
    energies.mid = juce::jlimit(0.0f, 1.0f, energies.mid);
    energies.high = juce::jlimit(0.0f, 1.0f, energies.high);

    const auto sum = energies.low + energies.mid + energies.high;
    if (sum > 1.0e-6f)
    {
        energies.low /= sum;
        energies.mid /= sum;
        energies.high /= sum;
    }

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
    const auto effectiveMatch = 0.72f * std::pow(colorMatch, 0.9f);

    if (mode == ColorMode::threeBand)
    {
        // Increase chroma by emphasizing dominant bands before color blend.
        const auto legacyVibrancePower = theme == ThemePreset::minimeters3Band
            ? juce::jmap(intensity, 0.0f, 1.0f, 1.12f, 1.38f)
            : juce::jmap(intensity, 0.0f, 1.0f, 1.06f, 1.24f);
        const auto warmVibrancePower = juce::jmap(intensity, 0.0f, 1.0f, 1.16f, 1.44f);
        const auto vibrancePower = lerpFloat(legacyVibrancePower, warmVibrancePower, colorMatch);

        energies.low = std::pow(juce::jlimit(0.0f, 1.0f, energies.low), vibrancePower);
        energies.mid = std::pow(juce::jlimit(0.0f, 1.0f, energies.mid), vibrancePower);
        energies.high = std::pow(juce::jlimit(0.0f, 1.0f, energies.high), vibrancePower);

        energies = normalizeEnergies(energies);

        // Pull some weight from mids to lows/highs to preserve warm multicolor separation.
        energies.low *= 1.18f;
        energies.mid *= 0.88f;
        energies.high *= 1.26f;
        energies = normalizeEnergies(energies);
    }

    const auto legacyBase = (mode == ColorMode::threeBand)
        ? blendThreeBand(energies, theme)
        : colourFromPreset(theme);
    juce::Colour base = legacyBase;

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

    if (mode == ColorMode::threeBand)
    {
        const auto canonicalLegacyBase = blendThreeBand(energies, ThemePreset::minimeters3Band);
        const auto convergedLegacyBase = legacyBase.interpolatedWith(canonicalLegacyBase, colorMatch);
        const auto warmBase = blendMiniMetersWarm(energies);
        base = convergedLegacyBase.interpolatedWith(warmBase, effectiveMatch);

        const auto canonicalSaturation = juce::jlimit(0.0f, 1.45f, 1.02f + 0.36f * intensity);
        const auto canonicalBrightness = juce::jlimit(0.0f, 1.0f, 0.82f + 0.14f * intensity);
        const auto canonicalAlphaFloor = 0.2f;
        const auto canonicalAmpShaped = std::pow(amp, 0.75f);

        const auto warmSaturation = juce::jlimit(0.2f, 1.48f, 1.12f + 0.36f * intensity);
        const auto warmBrightness = juce::jlimit(0.3f, 1.0f, 0.84f + 0.13f * intensity);
        const auto warmAlphaFloor = 0.18f;
        const auto warmAmpShaped = std::pow(amp, 0.78f);

        saturation = lerpFloat(saturation, canonicalSaturation, colorMatch);
        brightness = lerpFloat(brightness, canonicalBrightness, colorMatch);
        alphaFloor = lerpFloat(alphaFloor, canonicalAlphaFloor, colorMatch);
        ampShaped = lerpFloat(ampShaped, canonicalAmpShaped, colorMatch);

        saturation = lerpFloat(saturation, warmSaturation, effectiveMatch);
        brightness = lerpFloat(brightness, warmBrightness, effectiveMatch);
        alphaFloor = lerpFloat(alphaFloor, warmAlphaFloor, effectiveMatch);
        ampShaped = lerpFloat(ampShaped, warmAmpShaped, effectiveMatch);
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

juce::Colour ThemeEngine::blendMiniMetersWarm(const BandEnergies& energies) noexcept
{
    auto weighted = normalizeEnergies(energies);
    auto low = weighted.low;
    auto mid = weighted.mid;
    auto high = weighted.high;

    if (low + mid + high <= 1.0e-6f)
    {
        low = 0.5f;
        mid = 0.4f;
        high = 0.1f;
    }

    // Keep highs visibly cyan/blue when they are strong in the source.
    if (high > 0.34f)
    {
        high *= 1.1f;
        const auto accentSum = low + mid + high;
        if (accentSum > 1.0e-6f)
        {
            low /= accentSum;
            mid /= accentSum;
            high /= accentSum;
        }
    }

    constexpr auto lowR = 1.00f;
    constexpr auto lowG = 0.22f;
    constexpr auto lowB = 0.05f;

    constexpr auto midR = 1.00f;
    constexpr auto midG = 0.32f;
    constexpr auto midB = 0.72f;

    constexpr auto highR = 0.20f;
    constexpr auto highG = 0.78f;
    constexpr auto highB = 1.00f;

    const auto r = juce::jlimit(0.0f, 1.0f, low * lowR + mid * midR + high * highR);
    const auto g = juce::jlimit(0.0f, 1.0f, low * lowG + mid * midG + high * highG);
    const auto b = juce::jlimit(0.0f, 1.0f, low * lowB + mid * midB + high * highB);

    return juce::Colour::fromFloatRGBA(r, g, b, 1.0f);
}

} // namespace wvfrm
