#pragma once

#include "../JuceIncludes.h"

#include "../Parameters.h"
#include "../dsp/BandAnalyzer3.h"
#include "ThemeEngine.h"

#include <vector>

namespace wvfrm
{

class WaveformAudioProcessor;

class WaveformView : public juce::Component,
                     private juce::Timer
{
public:
    explicit WaveformView(WaveformAudioProcessor& processorToUse);
    ~WaveformView() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;

    juce::StringArray getAvailableThemes() const;
    void setSelectedTheme(const juce::String& themeName);
    juce::String getSelectedTheme() const;
    void setThemeHotReloadEnabled(bool enabled);

private:
    struct ColumnRenderData
    {
        float minA = 0.0f;
        float maxA = 0.0f;
        float minB = 0.0f;
        float maxB = 0.0f;
        float amplitude = 0.0f;
        float peakDb = -100.0f;
        BandEnergies bands;
        bool active = false;
    };

    void timerCallback() override;
    void ensureBuffers(int width, int maxSamplesPerPixel);

    juce::Colour waveformColourForColumn(const ThemePalette& theme,
                                         ColorMode colorMode,
                                         bool primary,
                                         const ColumnRenderData& column) const;
    juce::Colour colorMapLookup(const ThemePalette& theme, float normalized) const;
    juce::Colour blendMultiBand(const ThemePalette& theme, const BandEnergies& bands, float alpha) const;
    juce::String formatTimecode(double seconds) const;
    bool resolveColumnSampleRange(LoopMode loopMode,
                                  int x,
                                  int width,
                                  int numSamples,
                                  int writeHeadX,
                                  int& startSample,
                                  int& endSample) const noexcept;

    WaveformAudioProcessor& processor;
    BandAnalyzer3 bandAnalyzer;
    ThemeEngine themeEngine;
    juce::OpenGLContext openGLContext;

    std::vector<ColumnRenderData> columns;
    std::vector<float> monoSegmentScratch;
    juce::Rectangle<int> lastPlotBounds;
    int lastClickedX = -1;
    float lastClickedDb = -100.0f;
    bool hasClickedDb = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformView)
};

} // namespace wvfrm
