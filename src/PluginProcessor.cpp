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

double getDivisionBeats(int divisionIndex) noexcept
{
    const auto clamped = juce::jlimit(0, static_cast<int>(TimeWindowResolver::divisions.size()) - 1, divisionIndex);
    const auto division = TimeWindowResolver::divisions[static_cast<size_t>(clamped)];
    return 4.0 * static_cast<double>(division.numerator) / static_cast<double>(division.denominator);
}

double positiveFraction(double value) noexcept
{
    const auto floored = std::floor(value);
    const auto fraction = value - floored;
    return fraction < 0.0 ? (fraction + 1.0) : fraction;
}
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
    processedSamples.store(0);
    syncClockState = {};

    renderClockSeq.store(0);
    lastClockEndSample.store(0);
    lastClockPhase.store(0.0f);
    lastClockReliable.store(false);
    lastClockBpm.store(juce::jmax(1.0, lastKnownBpm.load()));
    lastClockResetSuggested.store(false);

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

            if (const auto ppq = position->getPpqPosition())
            {
                hostPpqPosition.store(*ppq);
                ppqReliable.store(true);
            }
            else
            {
                ppqReliable.store(false);
            }
        }
        else
        {
            tempoReliable.store(false);
            ppqReliable.store(false);
        }
    }
    else
    {
        tempoReliable.store(false);
        ppqReliable.store(false);
    }

    const auto totalInputChannels = getTotalNumInputChannels();
    const auto totalOutputChannels = getTotalNumOutputChannels();

    for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    analysisBuffer.pushBuffer(buffer);

    const auto blockSamples = buffer.getNumSamples();
    const auto blockEndSample = processedSamples.fetch_add(blockSamples) + static_cast<int64_t>(blockSamples);

    const auto mode = getChoiceIndex(parameters, ParamIDs::timeMode);
    const auto division = getChoiceIndex(parameters, ParamIDs::timeSyncDivision);

    float phaseNormalized = 0.0f;
    auto phaseReliable = false;
    auto resetSuggested = false;
    auto bpmUsed = juce::jmax(1.0, lastKnownBpm.load());

    if (mode == static_cast<int>(TimeMode::sync))
    {
        const auto beatsInLoop = juce::jmax(1.0e-9, getDivisionBeats(division));
        const auto hostBpm = tempoReliable.load() ? hostTempoBpm.load() : lastKnownBpm.load();

        const SyncClockInput input {
            ppqReliable.load(),
            hostPpqPosition.load(),
            hostBpm,
            blockEndSample,
            currentSampleRate.load(),
            beatsInLoop
        };

        const auto output = updateSyncLoopClock(input, syncClockState, 0.25);
        phaseNormalized = output.phaseNormalized;
        phaseReliable = output.phaseReliable;
        resetSuggested = output.resetSuggested;
        bpmUsed = hostBpm;
    }
    else
    {
        const auto resolved = resolveCurrentWindow();
        const auto intervalSeconds = juce::jmax(1.0e-6, resolved.ms * 0.001);
        const auto intervalSamples = juce::jmax(1.0, intervalSeconds * currentSampleRate.load());
        phaseNormalized = static_cast<float>(positiveFraction(static_cast<double>(blockEndSample) / intervalSamples));
        phaseReliable = true;
        resetSuggested = false;
        bpmUsed = resolved.bpmUsed;
        syncClockState = {};
    }

    renderClockSeq.fetch_add(1, std::memory_order_acq_rel); // begin write (odd)
    lastClockEndSample.store(blockEndSample, std::memory_order_relaxed);
    lastClockPhase.store(juce::jlimit(0.0f, 1.0f, phaseNormalized), std::memory_order_relaxed);
    lastClockReliable.store(phaseReliable, std::memory_order_relaxed);
    lastClockBpm.store(bpmUsed, std::memory_order_relaxed);
    lastClockResetSuggested.store(resetSuggested, std::memory_order_relaxed);
    renderClockSeq.fetch_add(1, std::memory_order_release); // end write (even)
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

double WaveformAudioProcessor::getLoopPhaseNormalized() const noexcept
{
    return juce::jlimit(0.0, 1.0, static_cast<double>(lastClockPhase.load()));
}

bool WaveformAudioProcessor::getLoopRenderFrame(LoopRenderFrame& out, int requestedSamples) const
{
    int64_t endSample = 0;
    float phase = 0.0f;
    auto reliable = false;
    auto resetSuggested = false;
    auto bpmUsed = 120.0;

    auto snapshotReadOk = false;
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        const auto seqBegin = renderClockSeq.load(std::memory_order_acquire);
        if ((seqBegin & 1u) != 0u)
            continue;

        endSample = lastClockEndSample.load(std::memory_order_relaxed);
        phase = lastClockPhase.load(std::memory_order_relaxed);
        reliable = lastClockReliable.load(std::memory_order_relaxed);
        bpmUsed = lastClockBpm.load(std::memory_order_relaxed);
        resetSuggested = lastClockResetSuggested.load(std::memory_order_relaxed);

        const auto seqEnd = renderClockSeq.load(std::memory_order_acquire);
        if (seqBegin == seqEnd)
        {
            snapshotReadOk = true;
            break;
        }
    }

    if (! snapshotReadOk)
        return false;

    if (! analysisBuffer.copyWindowEndingAt(out.samples, requestedSamples, endSample))
        return false;

    out.phaseNormalized = juce::jlimit(0.0f, 1.0f, phase);
    out.phaseReliable = reliable;
    out.frameEndSample = endSample;
    out.bpmUsed = bpmUsed;
    out.resetSuggested = resetSuggested;
    return true;
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

