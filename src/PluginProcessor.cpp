#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace wvfrm
{

namespace
{
constexpr auto stateType = "wvfrm_state";
constexpr auto editorWidthProperty = "editor_width";
constexpr auto editorHeightProperty = "editor_height";
}

WaveformAudioProcessor::WaveformAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, stateType, createParameterLayout())
{
}

void WaveformAudioProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate.store(sampleRate);

    const auto capacity = juce::jlimit(65536, 2 * 1024 * 1024, static_cast<int>(std::ceil(sampleRate * 9.0)));
    analysisBuffer.prepare(2, capacity);
}

void WaveformAudioProcessor::releaseResources()
{
    analysisBuffer.clear();
}

bool WaveformAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void WaveformAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (auto* playHead = getPlayHead())
    {
        if (auto position = playHead->getPosition())
        {
            if (const auto bpm = position->getBpm())
            {
                const auto safeBpm = juce::jmax(1.0, *bpm);
                hostTempoBpm.store(safeBpm);
                lastKnownBpm.store(safeBpm);
                tempoReliable.store(true);
            }
            else
            {
                tempoReliable.store(false);
            }
        }
        else
        {
            tempoReliable.store(false);
        }
    }
    else
    {
        tempoReliable.store(false);
    }

    const auto totalInputChannels = getTotalNumInputChannels();
    const auto totalOutputChannels = getTotalNumOutputChannels();

    for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    analysisBuffer.pushBuffer(buffer);
}

juce::AudioProcessorEditor* WaveformAudioProcessor::createEditor()
{
    return new WaveformAudioProcessorEditor(*this);
}

bool WaveformAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String WaveformAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WaveformAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WaveformAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WaveformAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WaveformAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int WaveformAudioProcessor::getNumPrograms()
{
    return 1;
}

int WaveformAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WaveformAudioProcessor::setCurrentProgram(int)
{
}

const juce::String WaveformAudioProcessor::getProgramName(int)
{
    return {};
}

void WaveformAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void WaveformAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    state.setProperty(editorWidthProperty, editorWidth.load(), nullptr);
    state.setProperty(editorHeightProperty, editorHeight.load(), nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void WaveformAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
    {
        if (xml->hasTagName(parameters.state.getType()))
        {
            auto restoredState = juce::ValueTree::fromXml(*xml);

            if (restoredState.hasProperty(editorWidthProperty))
                editorWidth.store(static_cast<int>(restoredState.getProperty(editorWidthProperty)));

            if (restoredState.hasProperty(editorHeightProperty))
                editorHeight.store(static_cast<int>(restoredState.getProperty(editorHeightProperty)));

            parameters.replaceState(restoredState);
        }
    }
}

juce::AudioProcessorValueTreeState& WaveformAudioProcessor::getValueTreeState() noexcept
{
    return parameters;
}

const juce::AudioProcessorValueTreeState& WaveformAudioProcessor::getValueTreeState() const noexcept
{
    return parameters;
}

TimeWindowResolver::ResolvedWindow WaveformAudioProcessor::resolveCurrentWindow() const noexcept
{
    const auto mode = getChoiceIndex(parameters, ParamIDs::timeMode);
    const auto division = getChoiceIndex(parameters, ParamIDs::timeSyncDivision);
    const auto timeMs = static_cast<double>(getFloatValue(parameters, ParamIDs::timeMs, 1000.0f));

    const auto bpmFromHost = tempoReliable.load() ? std::optional<double> { hostTempoBpm.load() } : std::nullopt;

    return TimeWindowResolver::resolve(mode == static_cast<int>(TimeMode::sync),
                                       division,
                                       timeMs,
                                       bpmFromHost,
                                       lastKnownBpm.load());
}

bool WaveformAudioProcessor::copyRecentSamples(juce::AudioBuffer<float>& destination, int numSamples) const
{
    return analysisBuffer.copyMostRecent(destination, numSamples);
}

double WaveformAudioProcessor::getCurrentSampleRateHz() const noexcept
{
    return currentSampleRate.load();
}

int WaveformAudioProcessor::getAnalysisCapacity() const noexcept
{
    return analysisBuffer.getCapacity();
}

void WaveformAudioProcessor::setLastEditorSize(int width, int height) noexcept
{
    editorWidth.store(width);
    editorHeight.store(height);
}

juce::Rectangle<int> WaveformAudioProcessor::getLastEditorBounds() const noexcept
{
    return { 0, 0, editorWidth.load(), editorHeight.load() };
}

} // namespace wvfrm

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wvfrm::WaveformAudioProcessor();
}

