#include "WaveformView.h"

#include "../PluginProcessor.h"
#include "../dsp/ChannelViews.h"

#include <cmath>
#include <limits>
#include <vector>

namespace wvfrm
{

WaveformView::WaveformView(WaveformAudioProcessor& processorToUse)
    : processor(processorToUse)
{
    startTimerHz(60);
}

void WaveformView::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds();

    juce::ColourGradient background(juce::Colour::fromRGB(7, 10, 16),
                                    static_cast<float>(bounds.getX()),
                                    static_cast<float>(bounds.getY()),
                                    juce::Colour::fromRGB(15, 26, 32),
                                    static_cast<float>(bounds.getRight()),
                                    static_cast<float>(bounds.getBottom()),
                                    false);
    background.addColour(0.55, juce::Colour::fromRGB(12, 22, 35));
    g.setGradientFill(background);
    g.fillRect(bounds);

    const auto resolved = processor.resolveCurrentWindow();
    const auto sampleRate = processor.getCurrentSampleRateHz();

    const auto requestedSamples = juce::jlimit(128,
                                               juce::jmax(128, processor.getAnalysisCapacity()),
                                               static_cast<int>(std::round(resolved.ms * sampleRate / 1000.0)));

    if (! processor.copyRecentSamples(scratch, requestedSamples))
    {
        g.setColour(juce::Colours::white.withAlpha(0.75f));
        g.setFont(juce::FontOptions(16.0f, juce::Font::plain));
        g.drawText("Waiting for audio...", bounds, juce::Justification::centred);
        return;
    }

    const auto& state = processor.getValueTreeState();

    const auto channelMode = static_cast<ChannelView>(getChoiceIndex(state, ParamIDs::channelView));
    const auto colorMode = static_cast<ColorMode>(getChoiceIndex(state, ParamIDs::colorMode));
    const auto themePreset = static_cast<ThemePreset>(getChoiceIndex(state, ParamIDs::themePreset));
    const auto intensity = getFloatValue(state, ParamIDs::themeIntensity, 100.0f);
    const auto smoothing = getFloatValue(state, ParamIDs::smoothing, 35.0f) / 100.0f;

    const auto gainDb = getFloatValue(state, ParamIDs::waveGainVisual, 0.0f);
    const auto gainLinear = juce::Decibels::decibelsToGain(gainDb);

    auto contentBounds = bounds.reduced(8);
    auto topLabel = contentBounds.removeFromTop(24);

    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.setFont(juce::FontOptions(14.0f, juce::Font::plain));

    const juce::String modeText = (getChoiceIndex(state, ParamIDs::timeMode) == static_cast<int>(TimeMode::sync))
        ? "SYNC"
        : "MS";

    const auto windowText = juce::String::formatted("Window: %.1f ms (%s)", resolved.ms, modeText.toRawUTF8());
    g.drawText(windowText, topLabel.removeFromLeft(300), juce::Justification::centredLeft);

    if (! resolved.tempoReliable && getChoiceIndex(state, ParamIDs::timeMode) == static_cast<int>(TimeMode::sync))
    {
        g.setColour(juce::Colour::fromRGB(230, 170, 60));
        g.drawText("Tempo indisponivel: fallback ativo", topLabel, juce::Justification::centredRight);
    }

    std::vector<TrackDescriptor> tracks;

    switch (channelMode)
    {
        case ChannelView::lrSplit:
            tracks.push_back({ RenderMode::left, "L" });
            tracks.push_back({ RenderMode::right, "R" });
            break;
        case ChannelView::left:
            tracks.push_back({ RenderMode::left, "LEFT" });
            break;
        case ChannelView::right:
            tracks.push_back({ RenderMode::right, "RIGHT" });
            break;
        case ChannelView::mono:
            tracks.push_back({ RenderMode::mono, "MONO" });
            break;
        case ChannelView::mid:
            tracks.push_back({ RenderMode::mid, "MID" });
            break;
        case ChannelView::side:
            tracks.push_back({ RenderMode::side, "SIDE" });
            break;
        default:
            tracks.push_back({ RenderMode::left, "L" });
            tracks.push_back({ RenderMode::right, "R" });
            break;
    }

    if (tracks.empty())
        return;

    const auto trackHeight = contentBounds.getHeight() / static_cast<int>(tracks.size());

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        auto trackBounds = contentBounds.removeFromTop(trackHeight);

        if (i == tracks.size() - 1 && ! contentBounds.isEmpty())
            trackBounds = trackBounds.withHeight(trackBounds.getHeight() + contentBounds.getHeight());

        drawTrack(g,
                  trackBounds,
                  scratch,
                  tracks[i].mode,
                  tracks[i].label,
                  themePreset,
                  colorMode,
                  intensity,
                  gainLinear,
                  smoothing);
    }
}

void WaveformView::timerCallback()
{
    repaint();
}

void WaveformView::drawTrack(juce::Graphics& g,
                             juce::Rectangle<int> bounds,
                             const juce::AudioBuffer<float>& source,
                             RenderMode mode,
                             const juce::String& label,
                             ThemePreset themePreset,
                             ColorMode colorMode,
                             float intensity,
                             float gainLinear,
                             float smoothing) const
{
    const auto width = juce::jmax(1, bounds.getWidth());
    const auto numSamples = source.getNumSamples();

    if (numSamples <= 0)
        return;

    g.setColour(juce::Colour::fromRGB(255, 255, 255).withAlpha(0.08f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 5.0f, 1.0f);

    const auto centerY = static_cast<float>(bounds.getCentreY());
    const auto halfHeight = static_cast<float>(bounds.getHeight()) * 0.46f;

    g.setColour(juce::Colours::white.withAlpha(0.13f));
    g.drawHorizontalLine(bounds.getCentreY(), static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));

    const auto samplesPerPixel = juce::jmax(1, numSamples / width);

    std::vector<float> derived;
    derived.resize(static_cast<size_t>(samplesPerPixel));

    for (int x = 0; x < width; ++x)
    {
        const auto start = x * samplesPerPixel;
        const auto end = juce::jmin(numSamples, start + samplesPerPixel);

        if (start >= end)
            continue;

        float minimum = std::numeric_limits<float>::max();
        float maximum = -std::numeric_limits<float>::max();

        const float* segmentData = nullptr;
        const auto segmentLength = end - start;

        if (mode == RenderMode::left)
        {
            segmentData = source.getReadPointer(0, start);

            for (int s = start; s < end; ++s)
            {
                const auto value = source.getSample(0, s);
                minimum = juce::jmin(minimum, value);
                maximum = juce::jmax(maximum, value);
            }
        }
        else if (mode == RenderMode::right)
        {
            const auto rightChannel = source.getNumChannels() > 1 ? 1 : 0;
            segmentData = source.getReadPointer(rightChannel, start);

            for (int s = start; s < end; ++s)
            {
                const auto value = source.getSample(rightChannel, s);
                minimum = juce::jmin(minimum, value);
                maximum = juce::jmax(maximum, value);
            }
        }
        else
        {
            for (int i = 0; i < segmentLength; ++i)
            {
                const auto sample = sampleForMode(mode, source, start + i);
                derived[static_cast<size_t>(i)] = sample;
                minimum = juce::jmin(minimum, sample);
                maximum = juce::jmax(maximum, sample);
            }

            segmentData = derived.data();
        }

        minimum *= gainLinear;
        maximum *= gainLinear;

        const auto amplitudeNorm = juce::jlimit(0.0f, 1.0f, (std::abs(maximum) + std::abs(minimum)) * 0.5f);
        const auto energies = bandAnalyzer.analyzeSegment(segmentData,
                                                          segmentLength,
                                                          processor.getCurrentSampleRateHz(),
                                                          smoothing);

        g.setColour(themeEngine.colourFor(energies,
                                          themePreset,
                                          colorMode,
                                          intensity,
                                          amplitudeNorm));

        const auto yMax = juce::jlimit(static_cast<float>(bounds.getY()),
                                       static_cast<float>(bounds.getBottom()),
                                       centerY - maximum * halfHeight);

        const auto yMin = juce::jlimit(static_cast<float>(bounds.getY()),
                                       static_cast<float>(bounds.getBottom()),
                                       centerY - minimum * halfHeight);

        g.drawVerticalLine(bounds.getX() + x, yMax, yMin);
    }

    g.setColour(juce::Colours::white.withAlpha(0.65f));
    g.setFont(juce::FontOptions(12.0f, juce::Font::plain));
    g.drawText(label, bounds.reduced(8), juce::Justification::topLeft);
}

float WaveformView::sampleForMode(RenderMode mode, const juce::AudioBuffer<float>& source, int sampleIndex) const noexcept
{
    const auto left = source.getSample(0, sampleIndex);
    const auto right = source.getNumChannels() > 1 ? source.getSample(1, sampleIndex) : left;

    switch (mode)
    {
        case RenderMode::left: return left;
        case RenderMode::right: return right;
        case RenderMode::mono: return mixForChannelView(ChannelView::mono, left, right);
        case RenderMode::mid: return mixForChannelView(ChannelView::mid, left, right);
        case RenderMode::side: return mixForChannelView(ChannelView::side, left, right);
        default: break;
    }

    return left;
}

} // namespace wvfrm
