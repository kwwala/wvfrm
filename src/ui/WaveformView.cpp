#include "WaveformView.h"

#include "../PluginProcessor.h"
#include "../dsp/ChannelViews.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace wvfrm
{

namespace
{
constexpr float globalBandAlpha = 0.15f;
constexpr float globalMix = 0.35f;
constexpr float rmsSmoothing = 0.7f;
constexpr float lineFatAlpha = 0.35f;
constexpr float lineFatThickness = 2.6f;
constexpr float lineMainThickness = 1.8f;
}

WaveformView::WaveformView(WaveformAudioProcessor& processorToUse)
    : processor(processorToUse)
{
    startTimerHz(60);
}

void WaveformView::setDebugOverlayEnabled(bool enabled) noexcept
{
    debugOverlayEnabled = enabled;
}

void WaveformView::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds();

    g.fillAll(juce::Colour::fromRGB(4, 4, 6));

    const auto resolved = processor.resolveCurrentWindow();
    const auto sampleRate = processor.getCurrentSampleRateHz();

    const auto requestedSamples = juce::jlimit(128,
                                               juce::jmax(128, processor.getAnalysisCapacity()),
                                               static_cast<int>(std::round(resolved.ms * sampleRate / 1000.0)));

    if (! processor.copyRecentSamples(scratch, requestedSamples))
    {
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(juce::FontOptions(16.0f, juce::Font::plain));
        g.drawText("Waiting for audio...", bounds, juce::Justification::centred);
        return;
    }

    const auto& state = processor.getValueTreeState();

    const auto channelMode = static_cast<ChannelView>(getChoiceIndex(state, ParamIDs::channelView));
    const auto colorMode = static_cast<ColorMode>(getChoiceIndex(state, ParamIDs::colorMode));
    const auto themePreset = static_cast<ThemePreset>(getChoiceIndex(state, ParamIDs::themePreset));
    const auto intensity = getFloatValue(state, ParamIDs::themeIntensity, 100.0f);

    const auto gainDb = getFloatValue(state, ParamIDs::waveGainVisual, 0.0f);
    const auto gainLinear = juce::Decibels::decibelsToGain(gainDb);

    const auto loopPhase = static_cast<float>(processor.getLoopPhaseNormalized());

    auto contentBounds = bounds.reduced(8);

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

    const auto width = contentBounds.getWidth();
    ensureLoopCaches(tracks.size(), width);
    updateLoopState(width, loopPhase);

    if (loopState.resetCaches)
        clearLoopCaches();

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
                  rmsSmoothing,
                  loopCaches[i]);
    }

    if (debugOverlayEnabled)
    {
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(juce::FontOptions(12.0f, juce::Font::plain));

        juce::String text = juce::String::formatted("Window %.1f ms", resolved.ms);
        if (getChoiceIndex(state, ParamIDs::timeMode) == static_cast<int>(TimeMode::sync))
        {
            if (resolved.tempoReliable)
                text << juce::String::formatted(" | Host BPM %.2f", resolved.bpmUsed);
            else
                text << juce::String::formatted(" | Fallback BPM %.2f", resolved.bpmUsed);
        }

        auto overlay = bounds.reduced(12);
        g.drawText(text, overlay.removeFromTop(16), juce::Justification::centredLeft);
    }

    lastLoopPhase = juce::jlimit(0.0f, 1.0f, loopPhase);
    hasLoopPhase = true;
}

void WaveformView::timerCallback()
{
    repaint();
}

void WaveformView::ensureLoopCaches(size_t trackCount, int width) const
{
    const auto trackCountInt = static_cast<int>(trackCount);
    if (width <= 0)
        return;

    const auto sizeChanged = (trackCountInt != lastTrackCount) || (width != lastWidth);

    if (sizeChanged)
    {
        loopCaches.clear();
        loopCaches.resize(trackCount);
        lastTrackCount = trackCountInt;
        lastWidth = width;
        hasLoopPhase = false;
    }

    for (auto& cache : loopCaches)
    {
        if (sizeChanged || cache.energies.size() != static_cast<size_t>(width))
        {
            cache.energies.assign(static_cast<size_t>(width), {});
            cache.mixed.assign(static_cast<size_t>(width), {});
            cache.min.assign(static_cast<size_t>(width), 0.0f);
            cache.max.assign(static_cast<size_t>(width), 0.0f);
            cache.amp.assign(static_cast<size_t>(width), 0.0f);
            cache.active.assign(static_cast<size_t>(width), 0);
            cache.globalBands = {};
            cache.globalValid = false;
        }
    }
}

void WaveformView::clearLoopCaches() const
{
    for (auto& cache : loopCaches)
    {
        std::fill(cache.active.begin(), cache.active.end(), 0);
        cache.globalBands = {};
        cache.globalValid = false;
    }
}

void WaveformView::updateLoopState(int width, float loopPhase) const
{
    loopState = {};
    if (width <= 0)
        return;

    const auto clampedLoopPhase = juce::jlimit(0.0f, 1.0f, loopPhase);

    loopState.width = width;
    loopState.writeX = juce::jlimit(0, width - 1, static_cast<int>(std::floor(clampedLoopPhase * static_cast<float>(width))));

    const auto gapGuess = juce::jmax(6, static_cast<int>(std::round(static_cast<float>(width) * 0.02f)));
    const auto maxGap = juce::jmax(1, width - 1);
    loopState.gapPixels = juce::jmin(gapGuess, maxGap);

    const auto fadeGuess = juce::jmax(3, loopState.gapPixels / 2);
    const auto maxFade = juce::jmax(1, width - 1 - loopState.gapPixels);
    loopState.fadePixels = juce::jmin(fadeGuess, maxFade);

    int prevX = loopState.writeX;

    if (hasLoopPhase)
    {
        prevX = juce::jlimit(0, width - 1, static_cast<int>(std::floor(lastLoopPhase * static_cast<float>(width))));
        const auto delta = clampedLoopPhase - lastLoopPhase;
        const auto wrap = delta < 0.0f && (lastLoopPhase - clampedLoopPhase) > 0.5f;
        const auto backward = delta < 0.0f && ! wrap;
        const auto jump = ! wrap && std::abs(delta) > 0.35f;

        if (jump)
        {
            loopState.resetCaches = true;
            loopState.rangeCount = 1;
            loopState.ranges[0] = { loopState.writeX, loopState.writeX };
            return;
        }

        if (wrap)
        {
            loopState.rangeCount = 2;
            loopState.ranges[0] = { prevX, width - 1 };
            loopState.ranges[1] = { 0, loopState.writeX };
            return;
        }

        if (backward || prevX > loopState.writeX)
        {
            loopState.rangeCount = 1;
            loopState.ranges[0] = { loopState.writeX, loopState.writeX };
            return;
        }

        loopState.rangeCount = 1;
        loopState.ranges[0] = { prevX, loopState.writeX };
        return;
    }

    loopState.rangeCount = 1;
    loopState.ranges[0] = { loopState.writeX, loopState.writeX };
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
                             float smoothing,
                             LoopCache& cache) const
{
    const auto width = juce::jmax(1, bounds.getWidth());
    const auto numSamples = source.getNumSamples();

    if (numSamples <= 0)
        return;

    g.setColour(juce::Colour::fromRGB(255, 255, 255).withAlpha(0.03f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 5.0f, 1.0f);

    const auto centerY = static_cast<float>(bounds.getCentreY());
    const auto halfHeight = static_cast<float>(bounds.getHeight()) * 0.5f;

    g.setColour(juce::Colours::white.withAlpha(0.04f));
    g.drawHorizontalLine(bounds.getCentreY(), static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));

    const auto samplesPerPixel = juce::jmax(1, numSamples / juce::jmax(1, width));
    std::vector<float> derived(static_cast<size_t>(samplesPerPixel));

    for (int r = 0; r < loopState.rangeCount; ++r)
    {
        const auto rangeStart = juce::jlimit(0, width - 1, loopState.ranges[r].start);
        const auto rangeEnd = juce::jlimit(0, width - 1, loopState.ranges[r].end);

        for (int x = rangeStart; x <= rangeEnd; ++x)
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

            const auto index = static_cast<size_t>(x);
            cache.min[index] = minimum;
            cache.max[index] = maximum;
            cache.amp[index] = amplitudeNorm;
            cache.energies[index] = energies;
            cache.active[index] = 1;
        }
    }

    const auto gapStart = (loopState.writeX + 1) % width;
    for (int i = 0; i < loopState.gapPixels; ++i)
    {
        const auto x = (gapStart + i) % width;
        cache.active[static_cast<size_t>(x)] = 0;
    }

    BandEnergies frameBands {};
    int activeCount = 0;

    for (int x = 0; x < width; ++x)
    {
        const auto index = static_cast<size_t>(x);
        if (cache.active[index] == 0)
            continue;

        frameBands.low += cache.energies[index].low;
        frameBands.mid += cache.energies[index].mid;
        frameBands.high += cache.energies[index].high;
        ++activeCount;
    }

    if (activeCount > 0)
    {
        const auto invCount = 1.0f / static_cast<float>(activeCount);
        frameBands.low *= invCount;
        frameBands.mid *= invCount;
        frameBands.high *= invCount;

        if (! cache.globalValid)
        {
            cache.globalBands = frameBands;
            cache.globalValid = true;
        }
        else
        {
            cache.globalBands.low += globalBandAlpha * (frameBands.low - cache.globalBands.low);
            cache.globalBands.mid += globalBandAlpha * (frameBands.mid - cache.globalBands.mid);
            cache.globalBands.high += globalBandAlpha * (frameBands.high - cache.globalBands.high);
        }
    }

    for (int x = 0; x < width; ++x)
    {
        const auto index = static_cast<size_t>(x);
        if (cache.active[index] == 0)
            continue;

        auto energies = cache.energies[index];
        energies.low += (cache.globalBands.low - energies.low) * globalMix;
        energies.mid += (cache.globalBands.mid - energies.mid) * globalMix;
        energies.high += (cache.globalBands.high - energies.high) * globalMix;
        cache.mixed[index] = energies;
    }

    const auto blurColors = (colorMode == ColorMode::threeBand);
    const int offsets[5] = { -2, -1, 0, 1, 2 };
    const float weights[5] = { 1.0f, 2.0f, 4.0f, 2.0f, 1.0f };

    for (int x = 0; x < width; ++x)
    {
        const auto index = static_cast<size_t>(x);
        if (cache.active[index] == 0)
            continue;

        int ahead = x - gapStart;
        if (ahead < 0)
            ahead += width;

        if (ahead < loopState.gapPixels)
            continue;

        float fade = 1.0f;
        if (ahead < loopState.gapPixels + loopState.fadePixels)
        {
            const auto fadeOffset = ahead - loopState.gapPixels;
            fade = loopState.fadePixels > 0
                ? static_cast<float>(fadeOffset) / static_cast<float>(loopState.fadePixels)
                : 1.0f;
        }

        BandEnergies energies = cache.mixed[index];

        if (blurColors)
        {
            float weightSum = 0.0f;
            float lowSum = 0.0f;
            float midSum = 0.0f;
            float highSum = 0.0f;

            for (int i = 0; i < 5; ++i)
            {
                const auto nx = x + offsets[i];
                if (nx < 0 || nx >= width)
                    continue;

                const auto nIndex = static_cast<size_t>(nx);
                if (cache.active[nIndex] == 0)
                    continue;

                int aheadNeighbor = nx - gapStart;
                if (aheadNeighbor < 0)
                    aheadNeighbor += width;
                if (aheadNeighbor < loopState.gapPixels)
                    continue;

                const auto w = weights[i];
                weightSum += w;
                lowSum += cache.mixed[nIndex].low * w;
                midSum += cache.mixed[nIndex].mid * w;
                highSum += cache.mixed[nIndex].high * w;
            }

            if (weightSum > 0.0f)
            {
                energies.low = lowSum / weightSum;
                energies.mid = midSum / weightSum;
                energies.high = highSum / weightSum;
            }
        }

        const auto amplitudeNorm = cache.amp[index];
        auto colour = themeEngine.colourFor(energies,
                                            themePreset,
                                            colorMode,
                                            intensity,
                                            amplitudeNorm);

        const auto baseAlpha = colour.getFloatAlpha();
        if (fade < 1.0f)
            colour = colour.withAlpha(baseAlpha * fade);

        const auto yMax = juce::jlimit(static_cast<float>(bounds.getY()),
                                       static_cast<float>(bounds.getBottom()),
                                       centerY - cache.max[index] * halfHeight);

        const auto yMin = juce::jlimit(static_cast<float>(bounds.getY()),
                                       static_cast<float>(bounds.getBottom()),
                                       centerY - cache.min[index] * halfHeight);

        const auto xPos = static_cast<float>(bounds.getX() + x) + 0.5f;

        g.setColour(colour.withAlpha(colour.getFloatAlpha() * lineFatAlpha));
        g.drawLine(xPos, yMax, xPos, yMin, lineFatThickness);

        g.setColour(colour);
        g.drawLine(xPos, yMax, xPos, yMin, lineMainThickness);
    }

    g.setColour(juce::Colours::white.withAlpha(0.45f));
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
