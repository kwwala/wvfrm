#include "PluginEditor.h"
#include "BinaryData.h"

namespace wvfrm
{

namespace
{
juce::Typeface::Ptr loadDmSansTypeface()
{
    return juce::Typeface::createSystemTypefaceFor(BinaryData::DMSans_Regular_ttf,
                                                   BinaryData::DMSans_Regular_ttfSize);
}

class MinimalLookAndFeel : public juce::LookAndFeel_V4
{
public:
    explicit MinimalLookAndFeel(juce::Typeface::Ptr typefaceIn)
        : typeface(typefaceIn)
    {
        setColour(juce::PopupMenu::backgroundColourId, juce::Colour::fromRGB(10, 12, 16));
        setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour::fromRGB(22, 28, 36));
        setColour(juce::PopupMenu::textColourId, juce::Colours::white.withAlpha(0.85f));
    }

    juce::Font getLabelFont(juce::Label&) override
    {
        return juce::Font(typeface).withHeight(12.5f);
    }

    juce::Font getComboBoxFont(juce::ComboBox&) override
    {
        return juce::Font(typeface).withHeight(12.5f);
    }

    juce::Font getPopupMenuFont() override
    {
        return juce::Font(typeface).withHeight(12.0f);
    }

    juce::Font getSliderPopupFont(juce::Slider&) override
    {
        return juce::Font(typeface).withHeight(12.0f);
    }

    void drawComboBox(juce::Graphics& g,
                      int width,
                      int height,
                      bool,
                      int,
                      int,
                      int,
                      int,
                      juce::ComboBox& box) override
    {
        const auto bounds = juce::Rectangle<int>(0, 0, width, height).toFloat();
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(bounds, 6.0f);

        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

        const auto arrowArea = bounds.removeFromRight(20.0f).reduced(6.0f);
        juce::Path arrow;
        arrow.addTriangle(arrowArea.getX(),
                          arrowArea.getCentreY() - 2.0f,
                          arrowArea.getRight(),
                          arrowArea.getCentreY() - 2.0f,
                          arrowArea.getCentreX(),
                          arrowArea.getCentreY() + 3.0f);
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.fillPath(arrow);
    }

    void drawLinearSlider(juce::Graphics& g,
                          int x,
                          int y,
                          int width,
                          int height,
                          float sliderPos,
                          float,
                          float,
                          const juce::Slider::SliderStyle,
                          juce::Slider& slider) override
    {
        const auto track = juce::Rectangle<float>(static_cast<float>(x),
                                                  static_cast<float>(y + height / 2 - 2),
                                                  static_cast<float>(width),
                                                  4.0f);

        g.setColour(slider.findColour(juce::Slider::backgroundColourId));
        g.fillRoundedRectangle(track, 2.0f);

        g.setColour(slider.findColour(juce::Slider::trackColourId));
        g.fillRoundedRectangle(track.withWidth(sliderPos - static_cast<float>(x)), 2.0f);

        const auto thumbX = sliderPos - 4.0f;
        const auto thumb = juce::Rectangle<float>(thumbX, track.getCentreY() - 6.0f, 8.0f, 12.0f);
        g.setColour(slider.findColour(juce::Slider::thumbColourId));
        g.fillRoundedRectangle(thumb, 3.0f);
    }

private:
    juce::Typeface::Ptr typeface;
};

void configureLabel(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
}
}

WaveformAudioProcessorEditor::WaveformAudioProcessorEditor(WaveformAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p),
      state(processor.getValueTreeState()),
      waveformView(processor)
{
    auto typeface = loadDmSansTypeface();
    lookAndFeel = std::make_unique<MinimalLookAndFeel>(typeface);
    setLookAndFeel(lookAndFeel.get());
    waveformView.setLookAndFeel(lookAndFeel.get());

    titleLabel.setText("wvfrm.", juce::dontSendNotification);
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.9f));
    titleLabel.setFont(juce::Font(typeface).withHeight(15.0f));
    addAndMakeVisible(titleLabel);

    configureLabel(timeModeLabel, "Time Mode");
    configureLabel(timeDivisionLabel, "Sync Div");
    configureLabel(timeMsLabel, "Time ms");
    configureLabel(channelViewLabel, "Channel");
    configureLabel(colorModeLabel, "Color");
    configureLabel(themePresetLabel, "Theme");

    addAndMakeVisible(timeModeLabel);
    addAndMakeVisible(timeDivisionLabel);
    addAndMakeVisible(timeMsLabel);
    addAndMakeVisible(channelViewLabel);
    addAndMakeVisible(colorModeLabel);
    addAndMakeVisible(themePresetLabel);

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
    addAndMakeVisible(timeMsSlider);

    addAndMakeVisible(waveformView);

    timeModeAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::timeMode, timeModeBox);
    timeDivisionAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::timeSyncDivision, timeDivisionBox);
    channelViewAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::channelView, channelViewBox);
    colorModeAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::colorMode, colorModeBox);
    themePresetAttachment = std::make_unique<ComboAttachment>(state, ParamIDs::themePreset, themePresetBox);

    timeMsAttachment = std::make_unique<SliderAttachment>(state, ParamIDs::timeMs, timeMsSlider);

    timeModeBox.onChange = [this] { updateTimeControls(); };

    setResizable(true, true);
    setResizeLimits(640, 360, 2048, 1400);

    const auto bounds = processor.getLastEditorBounds();
    setSize(juce::jmax(640, bounds.getWidth()), juce::jmax(360, bounds.getHeight()));

    updateTimeControls();
    setWantsKeyboardFocus(true);
    grabKeyboardFocus();
}

WaveformAudioProcessorEditor::~WaveformAudioProcessorEditor()
{
    waveformView.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void WaveformAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(5, 6, 8));
}

void WaveformAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced(14);

    auto header = bounds.removeFromTop(24);
    titleLabel.setBounds(header.removeFromLeft(200));

    bounds.removeFromTop(8);

    auto controls = bounds.removeFromTop(66);
    const auto cellWidth = controls.getWidth() / 6;
    const auto labelHeight = 18;

    auto row1 = controls.removeFromTop(labelHeight);
    timeModeLabel.setBounds(row1.removeFromLeft(cellWidth));
    timeDivisionLabel.setBounds(row1.removeFromLeft(cellWidth));
    timeMsLabel.setBounds(row1.removeFromLeft(cellWidth));
    channelViewLabel.setBounds(row1.removeFromLeft(cellWidth));
    colorModeLabel.setBounds(row1.removeFromLeft(cellWidth));
    themePresetLabel.setBounds(row1.removeFromLeft(cellWidth));

    auto row2 = controls.removeFromTop(30);
    timeModeBox.setBounds(row2.removeFromLeft(cellWidth).reduced(2, 0));
    timeDivisionBox.setBounds(row2.removeFromLeft(cellWidth).reduced(2, 0));
    timeMsSlider.setBounds(row2.removeFromLeft(cellWidth).reduced(2, 0));
    channelViewBox.setBounds(row2.removeFromLeft(cellWidth).reduced(2, 0));
    colorModeBox.setBounds(row2.removeFromLeft(cellWidth).reduced(2, 0));
    themePresetBox.setBounds(row2.removeFromLeft(cellWidth).reduced(2, 0));

    bounds.removeFromTop(8);
    waveformView.setBounds(bounds);

    processor.setLastEditorSize(getWidth(), getHeight());
}

void WaveformAudioProcessorEditor::configureCombo(juce::ComboBox& box, const juce::StringArray& choices)
{
    box.setColour(juce::ComboBox::backgroundColourId, juce::Colour::fromRGB(12, 14, 20));
    box.setColour(juce::ComboBox::textColourId, juce::Colours::white.withAlpha(0.85f));
    box.setColour(juce::ComboBox::arrowColourId, juce::Colours::white.withAlpha(0.6f));

    for (int i = 0; i < choices.size(); ++i)
        box.addItem(choices[i], i + 1);
}

void WaveformAudioProcessorEditor::configureKnob(juce::Slider& slider, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 20);
    slider.setTextValueSuffix(suffix);
    slider.setColour(juce::Slider::backgroundColourId, juce::Colour::fromRGB(14, 16, 22));
    slider.setColour(juce::Slider::trackColourId, juce::Colour::fromRGB(96, 135, 180));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour::fromRGB(215, 225, 235));
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
}

bool WaveformAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    if (key.getModifiers().isCtrlDown() && (key.getTextCharacter() == 'd' || key.getTextCharacter() == 'D'))
    {
        debugOverlayEnabled = ! debugOverlayEnabled;
        waveformView.setDebugOverlayEnabled(debugOverlayEnabled);
        repaint();
        return true;
    }

    return false;
}

} // namespace wvfrm
