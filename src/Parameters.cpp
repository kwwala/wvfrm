#include "Parameters.h"

#include <cmath>

namespace wvfrm
{
namespace
{
constexpr auto defaultScrollMode = static_cast<int>(ScrollMode::syncBpm);
constexpr auto defaultSyncDivision = 6; // 1/1
constexpr auto defaultFreeSpeedSeconds = 4.0f;
constexpr auto defaultLoopMode = static_cast<int>(LoopMode::scrolling);

constexpr auto defaultChannelA = static_cast<int>(ChannelMode::mid);
constexpr auto defaultChannelBEnabled = false;
constexpr auto defaultChannelB = static_cast<int>(ChannelMode::side);

constexpr auto defaultColorMode = static_cast<int>(ColorMode::multiBand);
constexpr auto defaultHistoryEnabled = true;
constexpr auto defaultHistoryMode = static_cast<int>(HistoryMode::fast1024);
constexpr auto defaultHistoryAlpha = 0.45f;

constexpr auto defaultDelayCompMs = 0.0f;
constexpr auto defaultLowMidHz = 150.0f;
constexpr auto defaultMidHighHz = 2500.0f;
constexpr auto defaultVisualGainDb = 0.0f;
constexpr auto defaultShowTimecode = true;
}

juce::StringArray getScrollModeChoices()
{
    return { "sync_bpm", "free_speed" };
}

juce::StringArray getSyncDivisionChoices()
{
    return { "1/64", "1/32", "1/16", "1/8", "1/4", "1/2", "1/1", "2/1", "4/1" };
}

juce::StringArray getLoopModeChoices()
{
    return { "scrolling", "static_loop" };
}

juce::StringArray getChannelChoices()
{
    return { "left", "right", "mid", "side" };
}

juce::StringArray getColorModeChoices()
{
    return { "static", "multi_band", "color_map" };
}

juce::StringArray getHistoryModeChoices()
{
    return { "fast_1024", "slow_16384" };
}

juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::scrollMode,
        "Scroll Mode",
        getScrollModeChoices(),
        defaultScrollMode));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::syncDivision,
        "Sync Division",
        getSyncDivisionChoices(),
        defaultSyncDivision));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::freeSpeedSeconds,
        "Free Speed (Seconds)",
        juce::NormalisableRange<float>(0.25f, 12.0f, 0.01f, 0.35f),
        defaultFreeSpeedSeconds));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::loopMode,
        "Loop Mode",
        getLoopModeChoices(),
        defaultLoopMode));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::channelA,
        "Channel A",
        getChannelChoices(),
        defaultChannelA));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::channelBEnabled,
        "Channel B Enabled",
        defaultChannelBEnabled));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::channelB,
        "Channel B",
        getChannelChoices(),
        defaultChannelB));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::colorMode,
        "Color Mode",
        getColorModeChoices(),
        defaultColorMode));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::historyEnabled,
        "History Enabled",
        defaultHistoryEnabled));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        ParamIDs::historyMode,
        "History Mode",
        getHistoryModeChoices(),
        defaultHistoryMode));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::historyAlpha,
        "History Alpha",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f),
        defaultHistoryAlpha));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::delayCompMs,
        "Delay Compensation (ms)",
        juce::NormalisableRange<float>(-250.0f, 250.0f, 0.01f),
        defaultDelayCompMs));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::lowMidHz,
        "Low Mid Crossover (Hz)",
        juce::NormalisableRange<float>(60.0f, 400.0f, 0.1f, 0.45f),
        defaultLowMidHz));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::midHighHz,
        "Mid High Crossover (Hz)",
        juce::NormalisableRange<float>(800.0f, 6000.0f, 1.0f, 0.45f),
        defaultMidHighHz));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        ParamIDs::visualGainDb,
        "Visual Gain (dB)",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f),
        defaultVisualGainDb));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        ParamIDs::showTimecode,
        "Show Timecode",
        defaultShowTimecode));

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

bool getBoolValue(const juce::AudioProcessorValueTreeState& state, const juce::String& paramId, bool fallback) noexcept
{
    if (const auto* value = state.getRawParameterValue(paramId))
        return value->load() >= 0.5f;

    return fallback;
}

} // namespace wvfrm
