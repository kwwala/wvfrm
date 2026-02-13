#pragma once

#include "JuceIncludes.h"

namespace wvfrm
{
namespace ParamIDs
{
static constexpr auto timeMode = "time_mode";
static constexpr auto timeSyncDivision = "time_sync_division";
static constexpr auto timeMs = "time_ms";
static constexpr auto channelView = "channel_view";
static constexpr auto colorMode = "color_mode";
static constexpr auto themePreset = "theme_preset";
static constexpr auto themeIntensity = "theme_intensity";
static constexpr auto waveGainVisual = "wave_gain_visual";
static constexpr auto smoothing = "smoothing";
static constexpr auto uiScale = "ui_scale";
static constexpr auto waveLoop = "wave_loop";
static constexpr auto colorMatch = "color_match";
}

enum class TimeMode
{
    sync = 0,
    milliseconds
};

enum class ChannelView
{
    lrSplit = 0,
    left,
    right,
    mono,
    mid,
    side
};

enum class ColorMode
{
    flatTheme = 0,
    threeBand
};

enum class ThemePreset
{
    minimeters3Band = 0,
    rekordboxInspired,
    classicAmber,
    iceBlue
};

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

juce::StringArray getTimeModeChoices();
juce::StringArray getTimeDivisionChoices();
juce::StringArray getChannelViewChoices();
juce::StringArray getColorModeChoices();
juce::StringArray getThemePresetChoices();

int getChoiceIndex(const juce::AudioProcessorValueTreeState& state, const juce::String& paramId);
float getFloatValue(const juce::AudioProcessorValueTreeState& state, const juce::String& paramId, float fallback) noexcept;

} // namespace wvfrm
