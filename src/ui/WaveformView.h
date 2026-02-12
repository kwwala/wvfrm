#pragma once

#include "../JuceIncludes.h"

#include <vector>

#include "../Parameters.h"
#include "../dsp/BandAnalyzer3.h"
#include "ThemeEngine.h"

namespace wvfrm
{

class WaveformAudioProcessor;

class WaveformView : public juce::Component,
                     private juce::Timer
{
public:
    explicit WaveformView(WaveformAudioProcessor& processorToUse);
    ~WaveformView() override = default;

    void paint(juce::Graphics& g) override;

private:
    enum class RenderMode
    {
        left,
        right,
        mono,
        mid,
        side
    };

    struct TrackDescriptor
    {
        RenderMode mode = RenderMode::left;
        juce::String label;
    };

    void timerCallback() override;

    void drawTrack(juce::Graphics& g,
                   juce::Rectangle<int> bounds,
                   const juce::AudioBuffer<float>& source,
                   RenderMode mode,
                   const juce::String& label,
                   ThemePreset themePreset,
                   ColorMode colorMode,
                   float intensity,
                   bool loopEnabled,
                   float loopPhase,
                   float gainLinear,
                   float smoothing) const;

    float sampleForMode(RenderMode mode, const juce::AudioBuffer<float>& source, int sampleIndex) const noexcept;

    WaveformAudioProcessor& processor;
    BandAnalyzer3 bandAnalyzer;
    ThemeEngine themeEngine;

    mutable juce::AudioBuffer<float> scratch;
    mutable std::vector<BandEnergies> energiesPerX;
    mutable std::vector<float> minPerX;
    mutable std::vector<float> maxPerX;
    mutable std::vector<float> ampPerX;
    mutable std::vector<uint8_t> activePerX;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformView)
};

} // namespace wvfrm
