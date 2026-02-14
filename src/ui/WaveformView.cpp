#include "WaveformView.h"

#include "../PluginProcessor.h"
#include "../dsp/ChannelViews.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace wvfrm
{

WaveformView::WaveformView(WaveformAudioProcessor& processorToUse)
    : processor(processorToUse)
{
    themeEngine.setThemeName(processor.getThemeName());
    themeEngine.setHotReloadEnabled(true);

    openGLContext.setMultisamplingEnabled(true);
    openGLContext.setComponentPaintingEnabled(true);
    openGLContext.setContinuousRepainting(false);
    openGLContext.attachTo(*this);

    startTimerHz(60);
}

WaveformView::~WaveformView()
{
    openGLContext.detach();
}

void WaveformView::paint(juce::Graphics& g)
{
    const auto theme = themeEngine.getTheme();
    g.fillAll(theme.background);

    auto bounds = getLocalBounds().reduced(10);
    if (bounds.getWidth() < 2 || bounds.getHeight() < 2)
        return;

    lastPlotBounds = bounds;

    constexpr int gridRows = 6;
    constexpr int gridColumns = 8;
    g.setColour(theme.grid.withAlpha(0.32f));
    for (int row = 1; row < gridRows; ++row)
    {
        const auto y = bounds.getY() + bounds.getHeight() * row / gridRows;
        g.drawHorizontalLine(y, static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));
    }

    for (int column = 1; column < gridColumns; ++column)
    {
        const auto x = bounds.getX() + bounds.getWidth() * column / gridColumns;
        g.drawVerticalLine(x, static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));
    }

    const auto sampleRate = processor.getCurrentSampleRateHz();
    const auto resolved = processor.resolveCurrentWindow();
    const auto requestedSamples = juce::jlimit(128,
                                               juce::jmax(128, processor.getAnalysisCapacity()),
                                               static_cast<int>(std::round(resolved.ms * sampleRate / 1000.0)));

    const auto& state = processor.getValueTreeState();
    const auto delayCompMs = getFloatValue(state, ParamIDs::delayCompMs, 0.0f);
    const auto delayCompSamples = static_cast<int>(std::round(delayCompMs * static_cast<float>(sampleRate) / 1000.0f));

    WaveformAudioProcessor::RenderFrame renderFrame;
    if (! processor.getRenderFrame(renderFrame, requestedSamples, delayCompSamples))
    {
        g.setColour(theme.textTimecode.withAlpha(0.7f));
        g.setFont(juce::FontOptions(16.0f));
        g.drawText("Waiting for audio...", bounds, juce::Justification::centred);
        return;
    }

    const auto width = juce::jmax(1, bounds.getWidth());
    const auto numSamples = renderFrame.samples.getNumSamples();
    if (numSamples < 2)
        return;

    const auto maxSamplesPerPixel = juce::jmax(1, (numSamples + width - 1) / width);
    ensureBuffers(width, maxSamplesPerPixel);

    const auto channelA = static_cast<ChannelMode>(getChoiceIndex(state, ParamIDs::channelA));
    const auto channelB = static_cast<ChannelMode>(getChoiceIndex(state, ParamIDs::channelB));
    const auto channelBEnabled = getBoolValue(state, ParamIDs::channelBEnabled, false);
    const auto colorMode = static_cast<ColorMode>(getChoiceIndex(state, ParamIDs::colorMode));
    const auto historyEnabled = getBoolValue(state, ParamIDs::historyEnabled, true);
    const auto historyMode = static_cast<HistoryMode>(getChoiceIndex(state, ParamIDs::historyMode));
    const auto historyAlpha = getFloatValue(state, ParamIDs::historyAlpha, 0.45f);
    const auto loopMode = static_cast<LoopMode>(getChoiceIndex(state, ParamIDs::loopMode));
    const auto lowMidHz = getFloatValue(state, ParamIDs::lowMidHz, 150.0f);
    const auto midHighHz = getFloatValue(state, ParamIDs::midHighHz, 2500.0f);
    const auto visualGainDb = getFloatValue(state, ParamIDs::visualGainDb, 0.0f);
    const auto visualGain = juce::Decibels::decibelsToGain(visualGainDb);
    const auto showTimecode = getBoolValue(state, ParamIDs::showTimecode, true);
    const auto historyWindow = BandAnalyzer3::rmsWindowForMode(historyMode);

    const auto writeHeadX = juce::jlimit(0,
                                         width - 1,
                                         static_cast<int>(std::floor(renderFrame.phaseNormalized * static_cast<float>(width))));

    const auto rightChannel = renderFrame.samples.getNumChannels() > 1 ? 1 : 0;

    for (int x = 0; x < width; ++x)
    {
        auto& column = columns[static_cast<size_t>(x)];
        column = {};

        int start = 0;
        int end = 0;
        if (! resolveColumnSampleRange(loopMode, x, width, numSamples, writeHeadX, start, end))
            continue;

        auto minimumA = std::numeric_limits<float>::max();
        auto maximumA = -std::numeric_limits<float>::max();
        auto minimumB = std::numeric_limits<float>::max();
        auto maximumB = -std::numeric_limits<float>::max();

        auto monoCount = 0;
        for (int s = start; s < end; ++s)
        {
            const auto left = renderFrame.samples.getSample(0, s);
            const auto right = renderFrame.samples.getSample(rightChannel, s);

            const auto sampleA = sampleForChannelMode(channelA, left, right);
            minimumA = juce::jmin(minimumA, sampleA);
            maximumA = juce::jmax(maximumA, sampleA);

            if (channelBEnabled)
            {
                const auto sampleB = sampleForChannelMode(channelB, left, right);
                minimumB = juce::jmin(minimumB, sampleB);
                maximumB = juce::jmax(maximumB, sampleB);
            }

            monoSegmentScratch[static_cast<size_t>(monoCount++)] = 0.5f * (left + right);
        }

        if (monoCount <= 0)
            continue;

        column.minA = minimumA * visualGain;
        column.maxA = maximumA * visualGain;
        column.minB = channelBEnabled ? (minimumB * visualGain) : 0.0f;
        column.maxB = channelBEnabled ? (maximumB * visualGain) : 0.0f;

        const auto peakA = juce::jmax(std::abs(column.minA), std::abs(column.maxA));
        const auto peakB = channelBEnabled ? juce::jmax(std::abs(column.minB), std::abs(column.maxB)) : 0.0f;
        const auto peak = juce::jlimit(0.0f, 1.5f, juce::jmax(peakA, peakB));

        column.amplitude = juce::jlimit(0.0f, 1.0f, peak);
        column.peakDb = juce::Decibels::gainToDecibels(juce::jmax(1.0e-9f, peak), -100.0f);
        column.bands = bandAnalyzer.analyzeSegment(monoSegmentScratch.data(),
                                                   monoCount,
                                                   sampleRate,
                                                   lowMidHz,
                                                   midHighHz,
                                                   historyWindow);
        column.active = true;
    }

    const auto centerY = static_cast<float>(bounds.getCentreY());
    const auto halfHeight = static_cast<float>(bounds.getHeight()) * 0.47f;

    g.setColour(theme.grid.withAlpha(0.6f));
    g.drawHorizontalLine(bounds.getCentreY(), static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));

    if (historyEnabled)
    {
        const auto overlayAlpha = juce::jlimit(0.0f, 1.0f, historyAlpha) * juce::jlimit(0.0f, 1.0f, theme.peakHistoryAlpha);
        for (int x = 0; x < width; ++x)
        {
            const auto& column = columns[static_cast<size_t>(x)];
            if (! column.active)
                continue;

            const auto color = blendMultiBand(theme, column.bands, overlayAlpha * (0.25f + 0.75f * column.amplitude));
            const auto magnitude = juce::jlimit(0.0f, 1.0f, juce::Decibels::decibelsToGain(column.bands.combinedRmsDb));
            const auto extent = magnitude * halfHeight;
            const auto xPos = static_cast<float>(bounds.getX() + x) + 0.5f;

            g.setColour(color);
            g.drawLine(xPos, centerY - extent, xPos, centerY + extent, 1.0f);
        }
    }

    for (int x = 0; x < width; ++x)
    {
        const auto& column = columns[static_cast<size_t>(x)];
        if (! column.active)
            continue;

        const auto xPos = static_cast<float>(bounds.getX() + x) + 0.5f;

        const auto yMaxA = juce::jlimit(static_cast<float>(bounds.getY()),
                                        static_cast<float>(bounds.getBottom()),
                                        centerY - column.maxA * halfHeight);
        const auto yMinA = juce::jlimit(static_cast<float>(bounds.getY()),
                                        static_cast<float>(bounds.getBottom()),
                                        centerY - column.minA * halfHeight);

        g.setColour(waveformColourForColumn(theme, colorMode, true, column));
        g.drawLine(xPos, yMaxA, xPos, yMinA, 1.15f);

        if (channelBEnabled)
        {
            const auto yMaxB = juce::jlimit(static_cast<float>(bounds.getY()),
                                            static_cast<float>(bounds.getBottom()),
                                            centerY - column.maxB * halfHeight);
            const auto yMinB = juce::jlimit(static_cast<float>(bounds.getY()),
                                            static_cast<float>(bounds.getBottom()),
                                            centerY - column.minB * halfHeight);

            g.setColour(waveformColourForColumn(theme, colorMode, false, column));
            g.drawLine(xPos, yMaxB, xPos, yMinB, 1.0f);
        }
    }

    const auto writeX = loopMode == LoopMode::staticLoop ? (bounds.getX() + writeHeadX) : bounds.getRight() - 1;
    g.setColour(theme.textTimecode.withAlpha(0.22f));
    g.drawVerticalLine(writeX, static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));

    g.setFont(juce::FontOptions(12.0f));
    g.setColour(theme.textTimecode.withAlpha(0.9f));

    const auto syncMode = getChoiceIndex(state, ParamIDs::scrollMode) == static_cast<int>(ScrollMode::syncBpm);
    juce::String modeText;
    if (syncMode)
    {
        const auto divisions = getSyncDivisionChoices();
        const auto idx = juce::jlimit(0, juce::jmax(0, divisions.size() - 1), getChoiceIndex(state, ParamIDs::syncDivision));
        modeText = juce::String::formatted("SYNC %s | %.2f BPM", divisions[idx].toRawUTF8(), renderFrame.bpmUsed);
    }
    else
        modeText = juce::String::formatted("FREE %.2fs", getFloatValue(state, ParamIDs::freeSpeedSeconds, 4.0f));

    g.drawText(modeText, bounds.reduced(8).removeFromTop(18), juce::Justification::centredLeft);

    if (showTimecode && renderFrame.transport.timeSecondsValid)
    {
        auto timeText = formatTimecode(renderFrame.transport.timeSeconds);
        if (! renderFrame.transport.isPlaying)
            timeText << "  [stopped]";

        auto label = bounds.reduced(8).removeFromTop(18);
        g.drawText(timeText, label, juce::Justification::centredRight);
    }

    if (hasClickedDb && lastClickedX >= 0 && lastClickedX < static_cast<int>(columns.size()))
    {
        const auto drawX = bounds.getX() + lastClickedX;
        const auto markerY = bounds.getY() + 8;
        const auto text = juce::String::formatted("%.1f dBFS", lastClickedDb);

        g.setColour(theme.cursorReadout.withAlpha(0.92f));
        g.drawVerticalLine(drawX, static_cast<float>(bounds.getY()), static_cast<float>(bounds.getBottom()));

        auto bubble = juce::Rectangle<float>(static_cast<float>(drawX + 8), static_cast<float>(markerY), 92.0f, 20.0f);
        if (bubble.getRight() > static_cast<float>(bounds.getRight()))
            bubble.setX(static_cast<float>(drawX - bubble.getWidth() - 8.0f));

        g.setColour(theme.background.brighter(0.35f).withAlpha(0.9f));
        g.fillRoundedRectangle(bubble, 4.0f);
        g.setColour(theme.cursorReadout);
        g.drawRoundedRectangle(bubble, 4.0f, 1.0f);
        g.drawText(text, bubble.toNearestInt(), juce::Justification::centred);
    }
}

void WaveformView::mouseDown(const juce::MouseEvent& event)
{
    if (! lastPlotBounds.contains(event.getPosition()))
        return;

    const auto x = juce::jlimit(0,
                                juce::jmax(0, static_cast<int>(columns.size()) - 1),
                                event.x - lastPlotBounds.getX());

    if (x < 0 || x >= static_cast<int>(columns.size()))
        return;

    const auto& column = columns[static_cast<size_t>(x)];
    if (! column.active)
        return;

    lastClickedX = x;
    lastClickedDb = column.peakDb;
    hasClickedDb = true;
    repaint();
}

juce::StringArray WaveformView::getAvailableThemes() const
{
    return themeEngine.getAvailableThemeNames();
}

void WaveformView::setSelectedTheme(const juce::String& themeName)
{
    themeEngine.setThemeName(themeName);
    processor.setThemeName(themeEngine.getThemeName());
    repaint();
}

juce::String WaveformView::getSelectedTheme() const
{
    return themeEngine.getThemeName();
}

void WaveformView::setThemeHotReloadEnabled(bool enabled)
{
    themeEngine.setHotReloadEnabled(enabled);
}

void WaveformView::timerCallback()
{
    themeEngine.pollForChanges();

    if (isShowing() && isVisible())
        repaint();
}

void WaveformView::ensureBuffers(int width, int maxSamplesPerPixel)
{
    const auto widthSize = static_cast<size_t>(juce::jmax(1, width));
    if (columns.size() != widthSize)
        columns.assign(widthSize, {});

    const auto segmentSize = static_cast<size_t>(juce::jmax(1, maxSamplesPerPixel));
    if (monoSegmentScratch.size() < segmentSize)
        monoSegmentScratch.resize(segmentSize);
}

juce::Colour WaveformView::waveformColourForColumn(const ThemePalette& theme,
                                                   ColorMode colorMode,
                                                   bool primary,
                                                   const ColumnRenderData& column) const
{
    const auto baseAlpha = primary ? 0.95f : 0.78f;

    if (colorMode == ColorMode::staticColour)
    {
        return (primary ? theme.waveformPrimary : theme.waveformSecondary)
            .withAlpha(baseAlpha * (0.25f + 0.75f * column.amplitude));
    }

    if (colorMode == ColorMode::multiBand)
    {
        auto color = blendMultiBand(theme, column.bands, baseAlpha);
        if (! primary)
            color = color.interpolatedWith(theme.waveformSecondary, 0.35f);

        return color.withAlpha(baseAlpha * (0.3f + 0.7f * column.amplitude));
    }

    auto color = colorMapLookup(theme, column.amplitude);
    if (! primary)
        color = color.interpolatedWith(theme.waveformSecondary, 0.5f);

    return color.withAlpha(baseAlpha * (0.3f + 0.7f * column.amplitude));
}

juce::Colour WaveformView::colorMapLookup(const ThemePalette& theme, float normalized) const
{
    const auto x = juce::jlimit(0.0f, 1.0f, normalized);
    const auto& stops = theme.colorMapStops;

    if (stops.empty())
        return theme.waveformPrimary;

    if (x <= stops.front().first)
        return stops.front().second;

    for (size_t i = 1; i < stops.size(); ++i)
    {
        if (x <= stops[i].first)
        {
            const auto x0 = stops[i - 1].first;
            const auto x1 = stops[i].first;
            const auto t = (x - x0) / juce::jmax(1.0e-6f, x1 - x0);
            return stops[i - 1].second.interpolatedWith(stops[i].second, t);
        }
    }

    return stops.back().second;
}

juce::Colour WaveformView::blendMultiBand(const ThemePalette& theme, const BandEnergies& bands, float alpha) const
{
    const auto low = juce::jlimit(0.0f, 1.0f, bands.low);
    const auto mid = juce::jlimit(0.0f, 1.0f, bands.mid);
    const auto high = juce::jlimit(0.0f, 1.0f, bands.high);
    const auto norm = juce::jmax(1.0e-6f, low + mid + high);

    const auto lowWeight = low / norm;
    const auto midWeight = mid / norm;
    const auto highWeight = high / norm;

    const auto red = theme.bandLow.getFloatRed() * lowWeight
        + theme.bandMid.getFloatRed() * midWeight
        + theme.bandHigh.getFloatRed() * highWeight;
    const auto green = theme.bandLow.getFloatGreen() * lowWeight
        + theme.bandMid.getFloatGreen() * midWeight
        + theme.bandHigh.getFloatGreen() * highWeight;
    const auto blue = theme.bandLow.getFloatBlue() * lowWeight
        + theme.bandMid.getFloatBlue() * midWeight
        + theme.bandHigh.getFloatBlue() * highWeight;

    return juce::Colour::fromFloatRGBA(red, green, blue, juce::jlimit(0.0f, 1.0f, alpha));
}

juce::String WaveformView::formatTimecode(double seconds) const
{
    const auto totalMilliseconds = static_cast<int64_t>(std::round(juce::jmax(0.0, seconds) * 1000.0));
    const auto ms = static_cast<int>(totalMilliseconds % 1000);
    const auto totalSeconds = totalMilliseconds / 1000;
    const auto s = static_cast<int>(totalSeconds % 60);
    const auto totalMinutes = totalSeconds / 60;
    const auto m = static_cast<int>(totalMinutes % 60);
    const auto h = static_cast<int>(totalMinutes / 60);

    return juce::String::formatted("%02d:%02d:%02d.%03d", h, m, s, ms);
}

bool WaveformView::resolveColumnSampleRange(LoopMode loopMode,
                                            int x,
                                            int width,
                                            int numSamples,
                                            int writeHeadX,
                                            int& startSample,
                                            int& endSample) const noexcept
{
    if (loopMode == LoopMode::staticLoop)
    {
        const auto distanceBehind = (writeHeadX - x + width) % width;
        const auto startDistance = static_cast<double>(distanceBehind + 1);
        const auto endDistance = static_cast<double>(distanceBehind);

        startSample = numSamples - static_cast<int>(std::floor(startDistance * static_cast<double>(numSamples) / static_cast<double>(width)));
        endSample = numSamples - static_cast<int>(std::floor(endDistance * static_cast<double>(numSamples) / static_cast<double>(width)));
    }
    else
    {
        startSample = static_cast<int>(std::floor(static_cast<double>(x) * numSamples / static_cast<double>(width)));
        endSample = static_cast<int>(std::floor(static_cast<double>(x + 1) * numSamples / static_cast<double>(width)));
    }

    startSample = juce::jlimit(0, numSamples - 1, startSample);
    endSample = juce::jlimit(startSample + 1, numSamples, endSample);

    return endSample > startSample;
}

} // namespace wvfrm
