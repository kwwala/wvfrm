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
constexpr float minimetersLineThickness = 1.15f;
constexpr float defaultLineThickness = 1.35f;
constexpr float minGlowExtraThickness = 0.3f;
constexpr float maxGlowExtraThickness = 2.6f;
constexpr double colourAnalysisWindowSeconds = 0.012;
constexpr float wrapGateAmplitudeThreshold = 0.08f;
constexpr float wrapGateDeltaThreshold = 0.35f;
constexpr float peakFloor = 1.0e-4f;

float smoothToward(float previous, float target, double dtSeconds, double attackTauSeconds, double releaseTauSeconds)
{
    const auto tau = target > previous ? attackTauSeconds : releaseTauSeconds;
    const auto alpha = std::exp(-dtSeconds / tau);
    return static_cast<float>(alpha * static_cast<double>(previous)
                              + (1.0 - alpha) * static_cast<double>(target));
}

float mapBandIntensity(float energy, float normalizationPeak, float intensityPercent)
{
    const auto peak = juce::jmax(peakFloor, normalizationPeak);
    const auto normalized = juce::jmax(0.0f, energy) / peak;
    const auto clamped = juce::jlimit(0.0f, 1.0f, normalized);

    // Log compression keeps tails visible without clipping strong peaks.
    constexpr auto compressionAmount = 8.0f;
    const auto compressed = std::log1p(compressionAmount * clamped) / std::log1p(compressionAmount);

    const auto intensity = juce::jlimit(0.0f, 1.0f, intensityPercent / 100.0f);
    const auto gamma = juce::jmap(intensity, 0.0f, 1.0f, 1.05f, 0.78f);
    return std::pow(juce::jlimit(0.0f, 1.0f, compressed), gamma);
}
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

    WaveformAudioProcessor::LoopRenderFrame renderFrame;
    if (! processor.getLoopRenderFrame(renderFrame, requestedSamples))
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
    const auto smoothing = getFloatValue(state, ParamIDs::smoothing, 35.0f) / 100.0f;
    const auto colorMatch = getFloatValue(state, ParamIDs::colorMatch, 100.0f) / 100.0f;
    const auto gainDb = getFloatValue(state, ParamIDs::waveGainVisual, 0.0f);
    const auto gainLinear = juce::Decibels::decibelsToGain(gainDb);
    const auto loopPhase = juce::jlimit(0.0f, 1.0f, renderFrame.phaseNormalized);
    const auto threeBandEnabled = colorMode == ColorMode::threeBand;

    const auto nowSeconds = juce::Time::getMillisecondCounterHiRes() * 0.001;
    auto dtSeconds = 1.0 / 60.0;
    if (lastColourFrameTimeSec > 0.0)
        dtSeconds = juce::jlimit(1.0 / 240.0, 1.0 / 15.0, nowSeconds - lastColourFrameTimeSec);

    lastColourFrameTimeSec = nowSeconds;

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

    auto resetAllTemporalState = renderFrame.resetSuggested
        || ! wasVisibleForTemporalState
        || (threeBandEnabled != lastThreeBandTemporalEnabled);

    if (temporalEnergiesByTrack.size() != tracks.size()
        || temporalInitByTrack.size() != tracks.size()
        || normalizationPeakByTrack.size() != tracks.size()
        || normalizationPeakInitByTrack.size() != tracks.size()
        || temporalTrackModes.size() != tracks.size())
    {
        temporalEnergiesByTrack.assign(tracks.size(), {});
        temporalInitByTrack.assign(tracks.size(), {});
        normalizationPeakByTrack.assign(tracks.size(), {});
        normalizationPeakInitByTrack.assign(tracks.size(), static_cast<uint8_t>(0));
        temporalTrackModes.assign(tracks.size(), RenderMode::left);
        resetAllTemporalState = true;
    }

    const auto trackRenderWidth = juce::jmax(1, contentBounds.getWidth());
    std::vector<uint8_t> resetTemporalByTrack(tracks.size(), static_cast<uint8_t>(resetAllTemporalState ? 1 : 0));

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        if (temporalTrackModes[i] != tracks[i].mode)
        {
            temporalTrackModes[i] = tracks[i].mode;
            resetTemporalByTrack[i] = static_cast<uint8_t>(1);
        }

        if (temporalEnergiesByTrack[i].size() != static_cast<size_t>(trackRenderWidth)
            || temporalInitByTrack[i].size() != static_cast<size_t>(trackRenderWidth))
        {
            temporalEnergiesByTrack[i].assign(static_cast<size_t>(trackRenderWidth), {});
            temporalInitByTrack[i].assign(static_cast<size_t>(trackRenderWidth), static_cast<uint8_t>(0));
            resetTemporalByTrack[i] = static_cast<uint8_t>(1);
        }
    }

    const auto trackHeight = contentBounds.getHeight() / static_cast<int>(tracks.size());

    for (size_t i = 0; i < tracks.size(); ++i)
    {
        auto trackBounds = contentBounds.removeFromTop(trackHeight);

        if (i == tracks.size() - 1 && ! contentBounds.isEmpty())
            trackBounds = trackBounds.withHeight(trackBounds.getHeight() + contentBounds.getHeight());

        drawTrack(g,
                  trackBounds,
                  renderFrame.samples,
                  static_cast<int>(i),
                  tracks[i].mode,
                  tracks[i].label,
                  themePreset,
                  colorMode,
                  intensity,
                  colorMatch,
                  loopPhase,
                  renderFrame.phaseReliable,
                  renderFrame.resetSuggested,
                  resetTemporalByTrack[i] != 0,
                  dtSeconds,
                  gainLinear,
                  smoothing);
    }

    wasVisibleForTemporalState = true;
    lastThreeBandTemporalEnabled = threeBandEnabled;

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
}

void WaveformView::timerCallback()
{
    if (isShowing() && isVisible())
        repaint();
    else
    {
        wasVisibleForTemporalState = false;
        lastColourFrameTimeSec = 0.0;
    }
}

void WaveformView::ensureRenderBuffers(int width) const
{
    const auto requiredSize = static_cast<size_t>(juce::jmax(1, width));

    if (energiesPerX.size() != requiredSize)
    {
        energiesPerX.assign(requiredSize, {});
        minPerX.assign(requiredSize, 0.0f);
        maxPerX.assign(requiredSize, 0.0f);
        ampPerX.assign(requiredSize, 0.0f);
        activePerX.assign(requiredSize, static_cast<uint8_t>(0));
    }
}

void WaveformView::drawTrack(juce::Graphics& g,
                             juce::Rectangle<int> bounds,
                             const juce::AudioBuffer<float>& source,
                             int trackIndex,
                             RenderMode mode,
                             const juce::String& label,
                             ThemePreset themePreset,
                             ColorMode colorMode,
                             float intensity,
                             float colorMatch,
                             float loopPhase,
                             bool phaseReliable,
                             bool resetSuggested,
                             bool resetTemporalState,
                             double dtSeconds,
                             float gainLinear,
                             float smoothing) const
{
    const auto width = juce::jmax(1, bounds.getWidth());
    const auto numSamples = source.getNumSamples();

    if (numSamples <= 0)
        return;

    g.setColour(juce::Colour::fromRGB(255, 255, 255).withAlpha(0.05f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 5.0f, 1.0f);

    const auto centerY = static_cast<float>(bounds.getCentreY());
    const auto halfHeight = static_cast<float>(bounds.getHeight()) * 0.46f;

    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawHorizontalLine(bounds.getCentreY(), static_cast<float>(bounds.getX()), static_cast<float>(bounds.getRight()));

    ensureRenderBuffers(width);
    std::fill(activePerX.begin(), activePerX.end(), static_cast<uint8_t>(0));

    const auto clampedLoopPhase = juce::jlimit(0.0f, 1.0f, loopPhase);
    const auto writeX = juce::jlimit(0, width - 1, static_cast<int>(std::floor(clampedLoopPhase * static_cast<float>(width))));
    BandEnergies framePeak {};

    if (width > 0 && numSamples > 0)
    {
        const auto maxSamplesPerPixel = juce::jmax(1, (numSamples + width - 1) / juce::jmax(1, width));
        std::vector<float> derived(static_cast<size_t>(maxSamplesPerPixel));
        const auto colourWindowSamples = juce::jlimit(64,
                                                      juce::jmin(2048, numSamples),
                                                      static_cast<int>(std::round(processor.getCurrentSampleRateHz()
                                                                                   * colourAnalysisWindowSeconds)));
        std::vector<float> colourDerived(static_cast<size_t>(juce::jmax(1, colourWindowSamples)));

        for (int x = 0; x < width; ++x)
        {
            // Map the most recent window to a circular write-head to keep a full-width loop.
            const auto distanceBehind = (writeX - x + width) % width;
            const auto startDistance = static_cast<double>(distanceBehind + 1);
            const auto endDistance = static_cast<double>(distanceBehind);
            const auto widthAsDouble = static_cast<double>(width);

            auto start = numSamples - static_cast<int>(std::floor((startDistance * static_cast<double>(numSamples)) / widthAsDouble));
            auto end = numSamples - static_cast<int>(std::floor((endDistance * static_cast<double>(numSamples)) / widthAsDouble));

            start = juce::jlimit(0, numSamples - 1, start);
            end = juce::jlimit(1, numSamples, end);
            end = juce::jmax(end, start + 1);

            if (start >= end)
                continue;

            float minimum = std::numeric_limits<float>::max();
            float maximum = -std::numeric_limits<float>::max();

            const auto segmentLength = end - start;

            if (mode == RenderMode::left)
            {
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
            }

            minimum *= gainLinear;
            maximum *= gainLinear;

            const auto amplitudeNorm = juce::jlimit(0.0f,
                                                    1.0f,
                                                    juce::jmax(std::abs(maximum), std::abs(minimum)));

            const auto colourEnd = juce::jlimit(1, numSamples, end);
            const auto colourStart = juce::jmax(0, colourEnd - colourWindowSamples);
            const auto colourLength = juce::jmax(1, colourEnd - colourStart);

            const float* colourData = nullptr;
            if (mode == RenderMode::left)
            {
                colourData = source.getReadPointer(0, colourStart);
            }
            else if (mode == RenderMode::right)
            {
                const auto rightChannel = source.getNumChannels() > 1 ? 1 : 0;
                colourData = source.getReadPointer(rightChannel, colourStart);
            }
            else
            {
                for (int i = 0; i < colourLength; ++i)
                    colourDerived[static_cast<size_t>(i)] = sampleForMode(mode, source, colourStart + i);

                colourData = colourDerived.data();
            }

            const auto energies = bandAnalyzer.analyzeSegment(colourData,
                                                              colourLength,
                                                              processor.getCurrentSampleRateHz(),
                                                              smoothing);

            const auto index = static_cast<size_t>(x);
            minPerX[index] = minimum;
            maxPerX[index] = maximum;
            ampPerX[index] = amplitudeNorm;
            energiesPerX[index] = energies;
            activePerX[index] = static_cast<uint8_t>(1);
            framePeak.low = juce::jmax(framePeak.low, energies.low);
            framePeak.mid = juce::jmax(framePeak.mid, energies.mid);
            framePeak.high = juce::jmax(framePeak.high, energies.high);
        }
    }

    const auto blurColors = phaseReliable
        && ! resetSuggested
        && (colorMode == ColorMode::threeBand)
        && (colorMatch > 0.0f);

    const int lowOffsets[3] = { -1, 0, 1 };
    const float lowWeights[3] = { 1.0f, 2.0f, 1.0f };
    const int wideOffsets[5] = { -2, -1, 0, 1, 2 };
    const float wideWeights[5] = { 1.0f, 2.0f, 3.0f, 2.0f, 1.0f };
    const auto applyTemporalSmoothing = colorMode == ColorMode::threeBand
        && trackIndex >= 0
        && trackIndex < static_cast<int>(temporalEnergiesByTrack.size())
        && trackIndex < static_cast<int>(temporalInitByTrack.size())
        && temporalEnergiesByTrack[static_cast<size_t>(trackIndex)].size() == static_cast<size_t>(width)
        && temporalInitByTrack[static_cast<size_t>(trackIndex)].size() == static_cast<size_t>(width);
    const auto applyDynamicNormalization = colorMode == ColorMode::threeBand
        && trackIndex >= 0
        && trackIndex < static_cast<int>(normalizationPeakByTrack.size())
        && trackIndex < static_cast<int>(normalizationPeakInitByTrack.size());

    const auto baseAttackMs = juce::jmap(smoothing, 0.0f, 1.0f, 18.0f, 60.0f);
    const auto baseReleaseMs = juce::jmap(smoothing, 0.0f, 1.0f, 120.0f, 360.0f);
    const auto attackMs = baseAttackMs + juce::jmap(colorMatch, 0.0f, 1.0f, 0.0f, 42.0f);
    const auto releaseMs = baseReleaseMs + juce::jmap(colorMatch, 0.0f, 1.0f, 0.0f, 220.0f);
    const auto attackTauSeconds = juce::jmax(1.0e-4, static_cast<double>(attackMs) * 0.001);
    const auto releaseTauSeconds = juce::jmax(1.0e-4, static_cast<double>(releaseMs) * 0.001);
    auto* temporalTrack = applyTemporalSmoothing ? &temporalEnergiesByTrack[static_cast<size_t>(trackIndex)] : nullptr;
    auto* temporalInit = applyTemporalSmoothing ? &temporalInitByTrack[static_cast<size_t>(trackIndex)] : nullptr;
    auto* normalizationPeak = applyDynamicNormalization ? &normalizationPeakByTrack[static_cast<size_t>(trackIndex)] : nullptr;
    auto* normalizationPeakInit = applyDynamicNormalization ? &normalizationPeakInitByTrack[static_cast<size_t>(trackIndex)] : nullptr;

    const auto peakAttackTauSeconds = juce::jmax(1.0e-4, static_cast<double>(juce::jmap(smoothing, 0.0f, 1.0f, 0.010f, 0.028f)));
    const auto peakReleaseTauSeconds = juce::jmax(1.0e-4, static_cast<double>(juce::jmap(smoothing, 0.0f, 1.0f, 0.26f, 0.56f)));

    if (applyDynamicNormalization && normalizationPeak != nullptr && normalizationPeakInit != nullptr)
    {
        const auto safeFramePeak = BandEnergies {
            juce::jmax(peakFloor, framePeak.low),
            juce::jmax(peakFloor, framePeak.mid),
            juce::jmax(peakFloor, framePeak.high)
        };

        if (resetTemporalState || *normalizationPeakInit == 0)
        {
            *normalizationPeak = safeFramePeak;
            *normalizationPeakInit = static_cast<uint8_t>(1);
        }
        else
        {
            normalizationPeak->low = smoothToward(normalizationPeak->low,
                                                  safeFramePeak.low,
                                                  dtSeconds,
                                                  peakAttackTauSeconds,
                                                  peakReleaseTauSeconds);
            normalizationPeak->mid = smoothToward(normalizationPeak->mid,
                                                  safeFramePeak.mid,
                                                  dtSeconds,
                                                  peakAttackTauSeconds,
                                                  peakReleaseTauSeconds);
            normalizationPeak->high = smoothToward(normalizationPeak->high,
                                                   safeFramePeak.high,
                                                   dtSeconds,
                                                   peakAttackTauSeconds,
                                                   peakReleaseTauSeconds);
        }

        normalizationPeak->low = juce::jmax(peakFloor, normalizationPeak->low);
        normalizationPeak->mid = juce::jmax(peakFloor, normalizationPeak->mid);
        normalizationPeak->high = juce::jmax(peakFloor, normalizationPeak->high);
    }

    for (int x = 0; x < width; ++x)
    {
        const auto index = static_cast<size_t>(x);
        if (activePerX[index] == 0)
            continue;

        BandEnergies energies = energiesPerX[index];

        if (blurColors)
        {
            const auto useWideKernel = colorMatch >= 0.55f;
            const auto* offsets = useWideKernel ? wideOffsets : lowOffsets;
            const auto* weights = useWideKernel ? wideWeights : lowWeights;
            const auto kernelSize = useWideKernel ? 5 : 3;
            const auto blurMix = juce::jmap(colorMatch, 0.0f, 1.0f, 0.2f, 1.0f);

            float weightSum = 0.0f;
            float lowSum = 0.0f;
            float midSum = 0.0f;
            float highSum = 0.0f;

            for (int i = 0; i < kernelSize; ++i)
            {
                auto nx = x + offsets[i];
                if (nx < 0)
                    nx += width;
                else if (nx >= width)
                    nx -= width;

                const auto nIndex = static_cast<size_t>(nx);
                if (activePerX[nIndex] == 0)
                    continue;

                const auto wrappedNeighbour = (x == 0 && nx == width - 1)
                    || (x == width - 1 && nx == 0);

                if (wrappedNeighbour)
                {
                    const auto ampA = ampPerX[index];
                    const auto ampB = ampPerX[nIndex];
                    const auto maxAmp = juce::jmax(ampA, ampB);

                    const auto delta = std::abs(energiesPerX[index].low - energiesPerX[nIndex].low)
                        + std::abs(energiesPerX[index].mid - energiesPerX[nIndex].mid)
                        + std::abs(energiesPerX[index].high - energiesPerX[nIndex].high);

                    if (maxAmp >= wrapGateAmplitudeThreshold && delta > wrapGateDeltaThreshold)
                        continue;
                }

                const auto w = weights[i];
                weightSum += w;
                lowSum += energiesPerX[nIndex].low * w;
                midSum += energiesPerX[nIndex].mid * w;
                highSum += energiesPerX[nIndex].high * w;
            }

            if (weightSum > 0.0f)
            {
                const auto blurredLow = lowSum / weightSum;
                const auto blurredMid = midSum / weightSum;
                const auto blurredHigh = highSum / weightSum;

                energies.low = juce::jmap(blurMix, energies.low, blurredLow);
                energies.mid = juce::jmap(blurMix, energies.mid, blurredMid);
                energies.high = juce::jmap(blurMix, energies.high, blurredHigh);
            }
        }

        if (applyTemporalSmoothing && temporalTrack != nullptr && temporalInit != nullptr)
        {
            auto& smoothed = (*temporalTrack)[index];
            auto& initialized = (*temporalInit)[index];

            if (resetTemporalState || initialized == 0)
            {
                smoothed = energies;
                initialized = static_cast<uint8_t>(1);
            }
            else
            {
                smoothed.low = smoothToward(smoothed.low, energies.low, dtSeconds, attackTauSeconds, releaseTauSeconds);
                smoothed.mid = smoothToward(smoothed.mid, energies.mid, dtSeconds, attackTauSeconds, releaseTauSeconds);
                smoothed.high = smoothToward(smoothed.high, energies.high, dtSeconds, attackTauSeconds, releaseTauSeconds);
            }

            energies = smoothed;
        }

        if (colorMode == ColorMode::threeBand)
        {
            BandEnergies mapped {};
            if (applyDynamicNormalization && normalizationPeak != nullptr && *normalizationPeakInit != 0)
            {
                mapped.low = mapBandIntensity(energies.low, normalizationPeak->low, intensity);
                mapped.mid = mapBandIntensity(energies.mid, normalizationPeak->mid, intensity);
                mapped.high = mapBandIntensity(energies.high, normalizationPeak->high, intensity);
            }
            else
            {
                mapped.low = juce::jlimit(0.0f, 1.0f, energies.low);
                mapped.mid = juce::jlimit(0.0f, 1.0f, energies.mid);
                mapped.high = juce::jlimit(0.0f, 1.0f, energies.high);
            }

            energies = mapped;
        }

        auto colour = themeEngine.colourFor(energies,
                                            themePreset,
                                            colorMode,
                                            intensity,
                                            ampPerX[index],
                                            colorMatch * 100.0f);

        const auto yMax = juce::jlimit(static_cast<float>(bounds.getY()),
                                       static_cast<float>(bounds.getBottom()),
                                       centerY - maxPerX[index] * halfHeight);

        const auto yMin = juce::jlimit(static_cast<float>(bounds.getY()),
                                       static_cast<float>(bounds.getBottom()),
                                       centerY - minPerX[index] * halfHeight);

        const auto xPos = static_cast<float>(bounds.getX() + x) + 0.5f;
        const auto coreThickness = colorMode == ColorMode::threeBand
            ? juce::jmap(colorMatch, defaultLineThickness, minimetersLineThickness)
            : defaultLineThickness;

        if (colorMode == ColorMode::threeBand && colorMatch > 0.0f)
        {
            const auto glowAlpha = colour.getFloatAlpha() * juce::jmap(colorMatch, 0.0f, 1.0f, 0.10f, 0.42f);
            const auto glowColour = colour.withAlpha(glowAlpha);
            const auto glowThickness = coreThickness + juce::jmap(colorMatch,
                                                                  0.0f,
                                                                  1.0f,
                                                                  minGlowExtraThickness,
                                                                  maxGlowExtraThickness);
            g.setColour(glowColour);
            g.drawLine(xPos, yMax, xPos, yMin, glowThickness);
        }

        g.setColour(colour);
        g.drawLine(xPos, yMax, xPos, yMin, coreThickness);
    }

    const auto cursorX = bounds.getX() + writeX;
    g.setColour(juce::Colours::white.withAlpha(0.16f));
    g.drawVerticalLine(cursorX,
                       static_cast<float>(bounds.getY() + 2),
                       static_cast<float>(bounds.getBottom() - 2));

    g.setColour(juce::Colours::white.withAlpha(0.6f));
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
