#include "PluginEditor.h"

namespace wvfrm
{

namespace
{
void configureLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.85f));
}
}

WaveformAudioProcessorEditor::WaveformAudioProcessorEditor(WaveformAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      state(processor.getValueTreeState()),
      waveformView(processor)
{
    titleLabel.setText("wvfrm - waveform visualizer", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    tempoLabel.setJustificationType(juce::Justification::centredRight);
    tempoLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    addAndMakeVisible(tempoLabel);

    configureLabel(timeModeLabel, "Time Mode");
    configureLabel(timeDivisionLabel, "Sync Div");
    configureLabel(timeMsLabel, "Time ms");
    configureLabel(channelViewLabel, "Channel");
    configureLabel(colorModeLabel, "Color");
    configureLabel(themePresetLabel, "Theme");
    configureLabel(themeIntensityLabel, "Intensity");
    configureLabel(waveGainLabel, "Gain dB");
    configureLabel(smoothingLabel, "Smoothing");
    configureLabel(loopLabel, "Loop");

    addAndMakeVisible(timeModeLabel);
    addAndMakeVisible(timeDivisionLabel);
    addAndMakeVisible(timeMsLabel);
    addAndMakeVisible(channelViewLabel);
    addAndMakeVisible(colorModeLabel);
    addAndMakeVisible(themePresetLabel);
    addAndMakeVisible(themeIntensityLabel);
    addAndMakeVisible(waveGainLabel);
    addAndMakeVisible(smoothingLabel);
    addAndMakeVisible(loopLabel);

    configureCombo(timeModeBox, getTimeModeChoices());
    configureCombo(timeDivisionBox, getTimeDivisionChoices());
    configureCombo(channelViewBox, getChannelViewChoices());
    configureCombo(colorModeBox, getColorModeChoices());
    configureCombo(themePresetBox, getThemePresetChoices());

    addAndMakeVisible(timeModeBox);
    addAndMakeVisible(timeDivisionBox);
    addAndMakeVisible(channelViewBox);
    addAndMakeVisible(colorModeBox);
    addAndMakeVisible(themePresetBox);

    configureKnob(timeMsSlider, " ms");
    configureKnob(themeIntensitySlider, " %");
    configureKnob(waveGainSlider, " dB");
    configureKnob(smoothingSlider, " %");

    addAndMakeVisible(timeMsSlider);
    addAndMakeVisible(themeIntensitySlider);
    addAndMakeVisible(waveGainSlider);
    addAndMakeVisible(smoothingSlider);

    loopButton.setButtonText("Enable");
    loopButton.setColour(juce::ToggleButton::textColourId, juce::Colours::white.withAlpha(0.9f));
    addAndMakeVisible(loopButton);

    addAndMakeVisible(waveformView);

    timeModeAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::timeMode, timeModeBox);
    timeDivisionAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::timeSyncDivision, timeDivisionBox);
    channelViewAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::channelView, channelViewBox);
    colorModeAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::colorMode, colorModeBox);
    themePresetAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::themePreset, themePresetBox);

    timeMsAttachment = std::make_unique<SliderAttachment>(state, ParamIDs::timeMs, timeMsSlider);
    themeIntensityAttachment = std::make_unique<SliderAttachment>(state, ParamIDs::themeIntensity, themeIntensitySlider);
    waveGainAttachment = std::make_unique<SliderAttachment>(state, ParamIDs::waveGainVisual, waveGainSlider);
    smoothingAttachment = std::make_unique<SliderAttachment>(state, ParamIDs::smoothing, smoothingSlider);
    loopAttachment = std::make_unique<ButtonAttachment>(state, ParamIDs::waveLoop, loopButton);

    timeModeBox.onChange = [this] { updateTimeControls(); };

    setResizable(true, true);
    setResizeLimits(720, 420, 2048, 1400);

    const auto bounds = processor.getLastEditorBounds();
    setSize(juce::jmax(720, bounds.getWidth()), juce::jmax(420, bounds.getHeight()));

    updateTimeControls();
    startTimerHz(8);
}

void WaveformAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(6, 8, 12));
}

void WaveformAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(12);

    auto header = bounds.removeFromTop(26);
    titleLabel.setBounds(header.removeFromLeft(320));
    tempoLabel.setBounds(header);

    bounds.removeFromTop(8);

    auto controls = bounds.removeFromTop(140);
    const auto cellWidth = controls.getWidth() / 5;
    const auto rowHeight = 28;

    auto row1 = controls.removeFromTop(rowHeight);
    timeModeLabel.setBounds(row1.removeFromLeft(cellWidth));
    timeDivisionLabel.setBounds(row1.removeFromLeft(cellWidth));
    timeMsLabel.setBounds(row1.removeFromLeft(cellWidth));
    channelViewLabel.setBounds(row1.removeFromLeft(cellWidth));
    colorModeLabel.setBounds(row1.removeFromLeft(cellWidth));

    auto row2 = controls.removeFromTop(30);
    timeModeBox.setBounds(row2.removeFromLeft(cellWidth).reduced(2));
    timeDivisionBox.setBounds(row2.removeFromLeft(cellWidth).reduced(2));
    timeMsSlider.setBounds(row2.removeFromLeft(cellWidth).reduced(2));
    channelViewBox.setBounds(row2.removeFromLeft(cellWidth).reduced(2));
    colorModeBox.setBounds(row2.removeFromLeft(cellWidth).reduced(2));

    controls.removeFromTop(8);

    auto row3 = controls.removeFromTop(rowHeight);
    themePresetLabel.setBounds(row3.removeFromLeft(cellWidth));
    themeIntensityLabel.setBounds(row3.removeFromLeft(cellWidth));
    waveGainLabel.setBounds(row3.removeFromLeft(cellWidth));
    smoothingLabel.setBounds(row3.removeFromLeft(cellWidth));
    loopLabel.setBounds(row3.removeFromLeft(cellWidth));

    auto row4 = controls.removeFromTop(30);
    themePresetBox.setBounds(row4.removeFromLeft(cellWidth).reduced(2));
    themeIntensitySlider.setBounds(row4.removeFromLeft(cellWidth).reduced(2));
    waveGainSlider.setBounds(row4.removeFromLeft(cellWidth).reduced(2));
    smoothingSlider.setBounds(row4.removeFromLeft(cellWidth).reduced(2));
    loopButton.setBounds(row4.removeFromLeft(cellWidth).reduced(2));

    bounds.removeFromTop(8);
    waveformView.setBounds(bounds);

    processor.setLastEditorSize(getWidth(), getHeight());
}

void WaveformAudioProcessorEditor::configureCombo(juce::ComboBox& box, const juce::StringArray& choices)
{
    box.setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(18, 22, 30));
    box.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.9f));

    for (int i = 0; i < choices.size(); ++i)
        box.addItem(choices[i], i + 1);
}

void WaveformAudioProcessorEditor::configureKnob(juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    slider.setTextValueSuffix(suffix);
    slider.setColour(juce::Slider::backgroundColourId, juce::Colour::fromRGB(20, 24, 34));
    slider.setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(70, 165, 240));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(230, 240, 250));
}

void WaveformAudioProcessorEditor::updateTimeControls()
{
    const auto syncSelected = timeModeBox.getSelectedItemIndex() == static_cast<int>(TimeMode::sync);

    timeDivisionBox.setEnabled(syncSelected);
    timeDivisionLabel.setEnabled(syncSelected);

    timeMsSlider.setEnabled(! syncSelected);
    timeMsLabel.setEnabled(! syncSelected);
}

void WaveformAudioProcessorEditor::timerCallback()
{
    const auto resolved = processor.resolveCurrentWindow();

    juce::String text = juce::String::formatted("Window %.1f ms", resolved.ms);

    if (getChoiceIndex(state, ParamIDs::timeMode) == static_cast<int>(TimeMode::sync))
    {
        if (resolved.tempoReliable)
            text << juce::String::formatted(" | Host BPM %.2f", resolved.bpmUsed);
        else
            text << juce::String::formatted(" | Fallback BPM %.2f", resolved.bpmUsed);
    }

    tempoLabel.setText(text, juce::dontSendNotification);
}

} // namespace wvfrm
