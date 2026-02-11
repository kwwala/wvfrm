#pragma once

#include "JuceIncludes.h"

#include "PluginProcessor.h"
#include "ui/WaveformView.h"

namespace wvfrm
{

class WaveformAudioProcessorEditor : public juce::AudioProcessorEditor,
                                     private juce::Timer
{
public:
    explicit WaveformAudioProcessorEditor(WaveformAudioProcessor&);
    ~WaveformAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void configureCombo(juce::ComboBox& box, const juce::StringArray& choices);
    void configureKnob(juce::Slider& slider, const juce::String& suffix);
    void updateTimeControls();
    void timerCallback() override;

    WaveformAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& state;

    juce::Label titleLabel;
    juce::Label tempoLabel;

    juce::ComboBox timeModeBox;
    juce::ComboBox timeDivisionBox;
    juce::ComboBox channelViewBox;
    juce::ComboBox colorModeBox;
    juce::ComboBox themePresetBox;

    juce::Slider timeMsSlider;
    juce::Slider themeIntensitySlider;
    juce::Slider waveGainSlider;
    juce::Slider smoothingSlider;

    juce::Label timeModeLabel;
    juce::Label timeDivisionLabel;
    juce::Label timeMsLabel;
    juce::Label channelViewLabel;
    juce::Label colorModeLabel;
    juce::Label themePresetLabel;
    juce::Label themeIntensityLabel;
    juce::Label waveGainLabel;
    juce::Label smoothingLabel;

    WaveformView waveformView;

    std::unique_ptr<ComboAttachment> timeModeAttachment;
    std::unique_ptr<ComboAttachment> timeDivisionAttachment;
    std::unique_ptr<ComboAttachment> channelViewAttachment;
    std::unique_ptr<ComboAttachment> colorModeAttachment;
    std::unique_ptr<ComboAttachment> themePresetAttachment;

    std::unique_ptr<SliderAttachment> timeMsAttachment;
    std::unique_ptr<SliderAttachment> themeIntensityAttachment;
    std::unique_ptr<SliderAttachment> waveGainAttachment;
    std::unique_ptr<SliderAttachment> smoothingAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformAudioProcessorEditor)
};

} // namespace wvfrm
