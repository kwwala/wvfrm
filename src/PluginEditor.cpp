#include "PluginEditor.h"

#include <array>

namespace wvfrm
{
namespace
{
void styleLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.82f));
    label.setFont(juce::FontOptions(12.0f));
}
}

WaveformAudioProcessorEditor::WaveformAudioProcessorEditor(WaveformAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      state(processor.getValueTreeState()),
      waveformView(processor)
{
    titleLabel.setText("wvfrm waveform", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.95f));
    titleLabel.setFont(juce::FontOptions(15.0f));
    addAndMakeVisible(titleLabel);

    styleLabel(themeLabel, "Theme");
    addAndMakeVisible(themeLabel);
    addAndMakeVisible(themeCombo);
    themeCombo.onChange = [this]
    {
        const auto selected = themeCombo.getText().trim();
        if (! selected.isEmpty())
            waveformView.setSelectedTheme(selected);
    };

    configureCombo(scrollModeBox, getScrollModeChoices());
    configureCombo(syncDivisionBox, getSyncDivisionChoices());
    configureCombo(loopModeBox, getLoopModeChoices());
    configureCombo(channelABox, getChannelChoices());
    configureCombo(channelBBox, getChannelChoices());
    configureCombo(colorModeBox, getColorModeChoices());
    configureCombo(historyModeBox, getHistoryModeChoices());

    configureSlider(freeSpeedSlider, " s");
    configureSlider(historyAlphaSlider, "");
    configureSlider(delayCompSlider, " ms");
    configureSlider(lowMidSlider, " Hz");
    configureSlider(midHighSlider, " Hz");
    configureSlider(visualGainSlider, " dB");

    configureToggle(channelBEnabledButton);
    channelBEnabledButton.setButtonText("Channel B");
    configureToggle(historyEnabledButton);
    historyEnabledButton.setButtonText("History");
    configureToggle(showTimecodeButton);
    showTimecodeButton.setButtonText("Timecode");

    addAndMakeVisible(scrollModeBox);
    addAndMakeVisible(syncDivisionBox);
    addAndMakeVisible(freeSpeedSlider);
    addAndMakeVisible(loopModeBox);
    addAndMakeVisible(channelABox);
    addAndMakeVisible(channelBEnabledButton);
    addAndMakeVisible(channelBBox);
    addAndMakeVisible(colorModeBox);
    addAndMakeVisible(historyEnabledButton);
    addAndMakeVisible(historyModeBox);
    addAndMakeVisible(historyAlphaSlider);
    addAndMakeVisible(delayCompSlider);
    addAndMakeVisible(lowMidSlider);
    addAndMakeVisible(midHighSlider);
    addAndMakeVisible(visualGainSlider);
    addAndMakeVisible(showTimecodeButton);
    addAndMakeVisible(waveformView);

    scrollModeAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::scrollMode, scrollModeBox);
    syncDivisionAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::syncDivision, syncDivisionBox);
    freeSpeedAttachment = std::make_unique<SliderAttachment>(state, ParamIDs::freeSpeedSeconds, freeSpeedSlider);
    loopModeAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::loopMode, loopModeBox);

    channelAAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::channelA, channelABox);
    channelBEnabledAttachment = std::make_unique<ButtonAttachment>(state, ParamIDs::channelBEnabled, channelBEnabledButton);
    channelBAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::channelB, channelBBox);
    colorModeAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::colorMode, colorModeBox);

    historyEnabledAttachment = std::make_unique<ButtonAttachment>(state, ParamIDs::historyEnabled, historyEnabledButton);
    historyModeAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::historyMode, historyModeBox);
    historyAlphaAttachment = std::make_unique<SliderAttachment>(state, ParamIDs::historyAlpha, historyAlphaSlider);
    delayCompAttachment = std::make_unique<SliderAttachment>(state, ParamIDs::delayCompMs, delayCompSlider);

    lowMidAttachment = std::make_unique<SliderAttachment>(state, ParamIDs::lowMidHz, lowMidSlider);
    midHighAttachment = std::make_unique<SliderAttachment>(state, ParamIDs::midHighHz, midHighSlider);
    visualGainAttachment = std::make_unique<SliderAttachment>(state, ParamIDs::visualGainDb, visualGainSlider);
    showTimecodeAttachment = std::make_unique<ButtonAttachment>(state, ParamIDs::showTimecode, showTimecodeButton);

    scrollModeBox.onChange = [this] { refreshControlEnablement(); };
    channelBEnabledButton.onClick = [this] { refreshControlEnablement(); };
    historyEnabledButton.onClick = [this] { refreshControlEnablement(); };

    setResizable(true, true);
    setResizeLimits(860, 500, 2200, 1400);
    const auto bounds = processor.getLastEditorBounds();
    setSize(juce::jmax(860, bounds.getWidth()), juce::jmax(500, bounds.getHeight()));

    refreshThemeChoices();
    waveformView.setThemeHotReloadEnabled(true);
    refreshControlEnablement();
    startTimerHz(2);
}

void WaveformAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(8, 10, 14));
}

void WaveformAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(12);

    auto top = bounds.removeFromTop(24);
    titleLabel.setBounds(top.removeFromLeft(220));

    themeLabel.setBounds(top.removeFromLeft(54));
    themeCombo.setBounds(top.removeFromLeft(230).reduced(2, 0));

    bounds.removeFromTop(8);

    auto controls = bounds.removeFromTop(140);
    const auto rowHeight = 30;
    const auto rowGap = 4;
    const auto cells = 4;

    auto layoutRow = [&controls, rowHeight, cells] () mutable
    {
        auto row = controls.removeFromTop(rowHeight);
        controls.removeFromTop(4);
        std::array<juce::Rectangle<int>, 4> slots {};
        const auto cellWidth = row.getWidth() / cells;
        for (int i = 0; i < cells; ++i)
            slots[static_cast<size_t>(i)] = row.removeFromLeft(cellWidth).reduced(2, 0);
        return slots;
    };

    juce::ignoreUnused(rowGap);

    const auto row1 = layoutRow();
    scrollModeBox.setBounds(row1[0]);
    syncDivisionBox.setBounds(row1[1]);
    freeSpeedSlider.setBounds(row1[2]);
    loopModeBox.setBounds(row1[3]);

    const auto row2 = layoutRow();
    channelABox.setBounds(row2[0]);
    channelBEnabledButton.setBounds(row2[1]);
    channelBBox.setBounds(row2[2]);
    colorModeBox.setBounds(row2[3]);

    const auto row3 = layoutRow();
    historyEnabledButton.setBounds(row3[0]);
    historyModeBox.setBounds(row3[1]);
    historyAlphaSlider.setBounds(row3[2]);
    delayCompSlider.setBounds(row3[3]);

    const auto row4 = layoutRow();
    lowMidSlider.setBounds(row4[0]);
    midHighSlider.setBounds(row4[1]);
    visualGainSlider.setBounds(row4[2]);
    showTimecodeButton.setBounds(row4[3]);

    bounds.removeFromTop(6);
    waveformView.setBounds(bounds);

    processor.setLastEditorSize(getWidth(), getHeight());
}

void WaveformAudioProcessorEditor::configureCombo(juce::ComboBox& box, const juce::StringArray& choices)
{
    box.setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(13, 16, 22));
    box.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.9f));
    box.setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.1f));
    box.setColour(juce::ComboBox::arrowColourId, juce::Colours::white.withAlpha(0.7f));

    for (int i = 0; i < choices.size(); ++i)
        box.addItem(choices[i], i + 1);
}

void WaveformAudioProcessorEditor::configureSlider(juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    slider.setTextValueSuffix(suffix);
    slider.setColour(juce::Slider::backgroundColourId, juce::Colour::fromRGB(16, 20, 28));
    slider.setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(105, 151, 202));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(233, 238, 245));
}

void WaveformAudioProcessorEditor::configureToggle(juce::ToggleButton& button)
{
    button.setColour(juce::ToggleButton::textColourId, juce::Colours::white.withAlpha(0.9f));
    button.setColour(juce::ToggleButton::tickColourId, juce::Colour::fromRGB(105, 151, 202));
}

void WaveformAudioProcessorEditor::refreshControlEnablement()
{
    const auto syncSelected = scrollModeBox.getSelectedItemIndex() == static_cast<int>(ScrollMode::syncBpm);
    syncDivisionBox.setEnabled(syncSelected);
    freeSpeedSlider.setEnabled(! syncSelected);

    const auto showB = channelBEnabledButton.getToggleState();
    channelBBox.setEnabled(showB);

    const auto showHistory = historyEnabledButton.getToggleState();
    historyModeBox.setEnabled(showHistory);
    historyAlphaSlider.setEnabled(showHistory);
}

void WaveformAudioProcessorEditor::refreshThemeChoices()
{
    const auto selectedBefore = waveformView.getSelectedTheme();
    const auto themes = waveformView.getAvailableThemes();

    themeCombo.clear(juce::dontSendNotification);
    for (int i = 0; i < themes.size(); ++i)
        themeCombo.addItem(themes[i], i + 1);

    auto selectedIndex = themes.indexOf(selectedBefore);
    if (selectedIndex < 0)
        selectedIndex = 0;

    if (themes.size() > 0)
    {
        themeCombo.setSelectedItemIndex(selectedIndex, juce::dontSendNotification);
        waveformView.setSelectedTheme(themes[selectedIndex]);
    }
}

void WaveformAudioProcessorEditor::timerCallback()
{
    const auto previous = themeCombo.getText().trim();
    const auto themes = waveformView.getAvailableThemes();

    if (! themes.contains(previous) || themeCombo.getNumItems() != themes.size())
        refreshThemeChoices();
}

} // namespace wvfrm
