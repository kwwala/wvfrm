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
    void setDebugOverlayEnabled(bool enabled) noexcept;

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

    struct LoopWriteRange
    {
        int start = 0;
        int end = 0;
    };

    struct LoopRenderState
    {
        int width = 0;
        int writeX = 0;
        int gapPixels = 0;
        int fadePixels = 0;
        int rangeCount = 0;
        LoopWriteRange ranges[2];
        bool resetCaches = false;
    };

    struct LoopCache
    {
        std::vector<BandEnergies> energies;
        std::vector<BandEnergies> mixed;
        std::vector<float> min;
        std::vector<float> max;
        std::vector<float> amp;
        std::vector<uint8_t> active;
        BandEnergies globalBands;
        bool globalValid = false;
    };

    void timerCallback() override;
    void ensureLoopCaches(size_t trackCount, int width) const;
    void clearLoopCaches() const;
    void updateLoopState(int width, float loopPhase) const;

    void drawTrack(juce::Graphics& g,
                   juce::Rectangle<int> bounds,
                   const juce::AudioBuffer<float>& source,
                   RenderMode mode,
                   const juce::String& label,
                   ThemePreset themePreset,
                   ColorMode colorMode,
                   float intensity,
                   float gainLinear,
                   float rmsSmoothing,
                   LoopCache& cache) const;

    float sampleForMode(RenderMode mode, const juce::AudioBuffer<float>& source, int sampleIndex) const noexcept;

    WaveformAudioProcessor& processor;
    BandAnalyzer3 bandAnalyzer;
    ThemeEngine themeEngine;

    mutable juce::AudioBuffer<float> scratch;
    mutable std::vector<LoopCache> loopCaches;
    mutable LoopRenderState loopState;
    mutable float lastLoopPhase = 0.0f;
    mutable bool hasLoopPhase = false;
    mutable int lastWidth = 0;
    mutable int lastTrackCount = 0;
    bool debugOverlayEnabled = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformView)
};

} // namespace wvfrm
