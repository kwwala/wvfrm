#pragma once

#include "JuceIncludes.h"

namespace wvfrm
{
namespace ParamIDs
{
static constexpr auto scrollMode = "scroll_mode";
static constexpr auto syncDivision = "sync_division";
static constexpr auto freeSpeedSeconds = "free_speed_seconds";
static constexpr auto loopMode = "loop_mode";

static constexpr auto channelA = "channel_a";
static constexpr auto channelBEnabled = "channel_b_enabled";
static constexpr auto channelB = "channel_b";

static constexpr auto colorMode = "color_mode";
static constexpr auto historyEnabled = "history_enabled";
static constexpr auto historyMode = "history_mode";
static constexpr auto historyAlpha = "history_alpha";

static constexpr auto delayCompMs = "delay_comp_ms";
static constexpr auto lowMidHz = "low_mid_hz";
static constexpr auto midHighHz = "mid_high_hz";
static constexpr auto visualGainDb = "visual_gain_db";
static constexpr auto showTimecode = "show_timecode";
}

enum class ChannelMode
{
    left = 0,
    right,
    mid,
    side
};

enum class ColorMode
{
    staticColour = 0,
    multiBand,
    colorMap
};

enum class HistoryMode
{
    fast1024 = 0,
    slow16384
};

enum class ScrollMode
{
    syncBpm = 0,
    freeSpeed
};

enum class LoopMode
{
    scrolling = 0,
    staticLoop
};

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

juce::StringArray getScrollModeChoices();
juce::StringArray getSyncDivisionChoices();
juce::StringArray getLoopModeChoices();
juce::StringArray getChannelChoices();
juce::StringArray getColorModeChoices();
juce::StringArray getHistoryModeChoices();

int getChoiceIndex(const juce::AudioProcessorValueTreeState& state, const juce::String& paramId);
float getFloatValue(const juce::AudioProcessorValueTreeState& state, const juce::String& paramId, float fallback) noexcept;
bool getBoolValue(const juce::AudioProcessorValueTreeState& state, const juce::String& paramId, bool fallback) noexcept;

} // namespace wvfrm
