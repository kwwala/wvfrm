#include "Parameters.h"

#include <cmath>

namespace wvfrm
{
namespace
{
constexpr auto defaultTimeMode = 0;
constexpr auto defaultTimeDivision = 4; // 1/4
constexpr auto defaultTimeMs = 1000.0f;
constexpr auto defaultChannelView = 0;
constexpr auto defaultColorMode = 1;
constexpr auto defaultThemePreset = 0;
constexpr auto defaultThemeIntensity = 100.0f;
constexpr auto defaultWaveGain = 0.0f;
constexpr auto defaultSmoothing = 35.0f;
constexpr auto defaultUiScale = 100.0f;
}

juce::StringArray getTimeModeChoices()
{
    return { "sync", "ms" };
}

juce::StringArray getTimeDivisionChoices()
{
    return { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1", "4/1" };
}

juce::StringArray getChannelViewChoices()
{
    return { "lr_split", "left", "right", "mono", "mid", "side" };
}

juce::StringArray getColorModeChoices()
{
    return { "flat_theme", "three_band" };
}

juce::StringArray getThemePresetChoices()
{
    return { "minimeters_3band", "rekordbox_inspired", "classic_amber", "ice_blue" };
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::timeMode,
        "Time Mode",
        getTimeModeChoices(),
        defaultTimeMode));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::timeSyncDivision,
        "Time Sync Division",
        getTimeDivisionChoices(),
        defaultTimeDivision));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::timeMs,
        "Time (ms)",
        juce::NormalisableRange<float>(10.0f, 5000.0f, 1.0f, 0.35f),
        defaultTimeMs));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::channelView,
        "Channel View",
        getChannelViewChoices(),
        defaultChannelView));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::colorMode,
        "Color Mode",
        getColorModeChoices(),
        defaultColorMode));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::themePreset,
        "Theme Preset",
        getThemePresetChoices(),
        defaultThemePreset));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::themeIntensity,
        "Theme Intensity",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        defaultThemeIntensity));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::waveGainVisual,
        "Wave Gain Visual",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f),
        defaultWaveGain));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::smoothing,
        "Smoothing",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.01f),
        defaultSmoothing));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::uiScale,
        "UI Scale",
        juce::NormalisableRange<float>(50.0f, 200.0f, 0.1f),
        defaultUiScale));

    return { params.begin(), params.end() };
}

int getChoiceIndex(const juce::AudioProcessorValueTreeState& state, const juce::String& paramId)
{
    if (const auto* value = state.getRawParameterValue(paramId))
        return juce::jmax(0, static_cast<int>(std::lround(value->load())));

    return 0;
}

float getFloatValue(const juce::AudioProcessorValueTreeState& state, const juce::String& paramId, float fallback) noexcept
{
    if (const auto* value = state.getRawParameterValue(paramId))
        return value->load();

    return fallback;
}

} // namespace wvfrm
