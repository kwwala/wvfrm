#include "Parameters.h"

#include <cmath>
#include <iostream>

namespace
{
class ParameterProbeProcessor : public juce::AudioProcessor
{
public:
    ParameterProbeProcessor()
        : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true)
                                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
          state(*this, nullptr, "parameter_probe_state", wvfrm::createParameterLayout())
    {
    }

    const juce::String getName() const override { return "parameter_probe"; }
    void prepareToPlay(double, int) override {}
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override { return true; }
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    juce::AudioProcessorValueTreeState state;
};
}

bool runParametersTests()
{
    bool ok = true;
    ParameterProbeProcessor probe;

    auto* colorMatchParameter = dynamic_cast<juce::AudioParameterFloat*>(probe.state.getParameter(wvfrm::ParamIDs::colorMatch));
    if (colorMatchParameter == nullptr)
    {
        std::cerr << "Parameters: missing color_match parameter." << std::endl;
        ok = false;
    }
    else
    {
        const auto& range = colorMatchParameter->range;
        if (std::abs(range.start - 0.0f) > 1.0e-6f
            || std::abs(range.end - 100.0f) > 1.0e-6f
            || std::abs(range.interval - 0.01f) > 1.0e-6f)
        {
            std::cerr << "Parameters: color_match range should be 0..100 step 0.01." << std::endl;
            ok = false;
        }

        if (std::abs(colorMatchParameter->get() - 100.0f) > 1.0e-4f)
        {
            std::cerr << "Parameters: color_match default should be 100." << std::endl;
            ok = false;
        }
    }

    if (probe.state.getParameter(wvfrm::ParamIDs::timeMode) == nullptr
        || probe.state.getParameter(wvfrm::ParamIDs::colorMode) == nullptr
        || probe.state.getParameter(wvfrm::ParamIDs::themePreset) == nullptr
        || probe.state.getParameter(wvfrm::ParamIDs::waveLoop) == nullptr)
    {
        std::cerr << "Parameters: one or more legacy parameters are missing after adding color_match." << std::endl;
        ok = false;
    }

    return ok;
}
