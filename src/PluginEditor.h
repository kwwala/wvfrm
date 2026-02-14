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
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    void configureCombo(juce::ComboBox& box, const juce::StringArray& choices);
    void configureSlider(juce::Slider& slider, const juce::String& suffix);
    void configureToggle(juce::ToggleButton& button);
    void refreshControlEnablement();
    void refreshThemeChoices();
    void timerCallback() override;

    WaveformAudioProcessor& processor;
    juce::AudioProcessorValueTreeState& state;

    juce::Label titleLabel;
    juce::Label themeLabel;
    juce::ComboBox themeCombo;

    juce::ComboBox scrollModeBox;
    juce::ComboBox syncDivisionBox;
    juce::Slider freeSpeedSlider;
    juce::ComboBox loopModeBox;

    juce::ComboBox channelABox;
    juce::ToggleButton channelBEnabledButton;
    juce::ComboBox channelBBox;
    juce::ComboBox colorModeBox;

    juce::ToggleButton historyEnabledButton;
    juce::ComboBox historyModeBox;
    juce::Slider historyAlphaSlider;
    juce::Slider delayCompSlider;

    juce::Slider lowMidSlider;
    juce::Slider midHighSlider;
    juce::Slider visualGainSlider;
    juce::ToggleButton showTimecodeButton;

    WaveformView waveformView;

    std::unique_ptr<ComboAttachment> scrollModeAttachment;
    std::unique_ptr<ComboAttachment> syncDivisionAttachment;
    std::unique_ptr<SliderAttachment> freeSpeedAttachment;
    std::unique_ptr<ComboAttachment> loopModeAttachment;

    std::unique_ptr<ComboAttachment> channelAAttachment;
    std::unique_ptr<ButtonAttachment> channelBEnabledAttachment;
    std::unique_ptr<ComboAttachment> channelBAttachment;
    std::unique_ptr<ComboAttachment> colorModeAttachment;

    std::unique_ptr<ButtonAttachment> historyEnabledAttachment;
    std::unique_ptr<ComboAttachment> historyModeAttachment;
    std::unique_ptr<SliderAttachment> historyAlphaAttachment;
    std::unique_ptr<SliderAttachment> delayCompAttachment;

    std::unique_ptr<SliderAttachment> lowMidAttachment;
    std::unique_ptr<SliderAttachment> midHighAttachment;
    std::unique_ptr<SliderAttachment> visualGainAttachment;
    std::unique_ptr<ButtonAttachment> showTimecodeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformAudioProcessorEditor)
};

} // namespace wvfrm
