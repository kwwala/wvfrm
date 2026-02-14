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
constexpr auto themeNameProperty = "theme_name";

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

    const auto capacity = juce::jlimit(65536, 2 * 1024 * 1024, static_cast<int>(std::ceil(sampleRate * 12.0)));
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

    auto bpmValid = false;
    auto playKnown = false;
    auto playState = true;
    auto ppqValid = false;
    auto ppq = 0.0;
    auto timeValid = false;
    auto timeSeconds = 0.0;

    if (auto* currentPlayHead = getPlayHead())
    {
        if (auto position = currentPlayHead->getPosition())
        {
            if (const auto bpm = position->getBpm())
            {
                const auto safeBpm = juce::jmax(1.0, *bpm);
                hostTempoBpm.store(safeBpm);
                lastKnownBpm.store(safeBpm);
                tempoReliable.store(true);
                bpmValid = true;
            }
            else
            {
                tempoReliable.store(false);
            }

            playKnown = true;
            playState = position->getIsPlaying();

            if (const auto hostTime = position->getTimeInSeconds())
            {
                timeValid = true;
                timeSeconds = *hostTime;
            }

            if (const auto hostPpqValue = position->getPpqPosition())
            {
                ppqValid = true;
                ppq = *hostPpqValue;
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

    hostIsPlayingKnown.store(playKnown);
    hostIsPlaying.store(playState);
    hostTimeValid.store(timeValid);
    hostTimeSeconds.store(timeSeconds);
    hostPpqReliable.store(ppqValid);
    hostPpq.store(ppq);

    const auto totalInputChannels = getTotalNumInputChannels();
    const auto totalOutputChannels = getTotalNumOutputChannels();

    for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    analysisBuffer.pushBuffer(buffer);

    const auto blockSamples = buffer.getNumSamples();
    const auto blockEndSample = processedSamples.fetch_add(blockSamples) + static_cast<int64_t>(blockSamples);

    const auto syncSelected = getChoiceIndex(parameters, ParamIDs::scrollMode) == static_cast<int>(ScrollMode::syncBpm);
    const auto division = getChoiceIndex(parameters, ParamIDs::syncDivision);
    const auto freeSpeed = static_cast<double>(getFloatValue(parameters, ParamIDs::freeSpeedSeconds, 4.0f));

    float phaseNormalized = 0.0f;
    auto phaseReliable = false;
    auto resetSuggested = false;
    auto bpmUsed = juce::jmax(1.0, lastKnownBpm.load());

    if (syncSelected)
    {
        SyncClockInput input {};
        input.hostIsPlayingKnown = playKnown;
        input.hostIsPlaying = playState;
        input.hostPpqValid = ppqValid;
        input.hostPpq = ppq;
        input.hostBpmValid = bpmValid;
        input.hostBpm = bpmValid ? hostTempoBpm.load() : lastKnownBpm.load();
        input.blockEndSample = blockEndSample;
        input.sampleRate = currentSampleRate.load();
        input.beatsInLoop = TimeWindowResolver::divisionToBeats(division);

        const auto output = updateSyncLoopClock(input, syncClockState);
        phaseNormalized = output.phaseNormalized;
        phaseReliable = output.phaseReliable;
        resetSuggested = output.resetSuggested;
        bpmUsed = output.bpmUsed;
    }
    else
    {
        const auto resolved = TimeWindowResolver::resolve(false,
                                                          division,
                                                          freeSpeed,
                                                          bpmValid ? std::optional<double> { hostTempoBpm.load() } : std::nullopt,
                                                          lastKnownBpm.load());

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
    lastTransportTimeSeconds.store(timeSeconds, std::memory_order_relaxed);
    lastTransportTimeValid.store(timeValid, std::memory_order_relaxed);
    lastTransportIsPlaying.store(playState, std::memory_order_relaxed);
    lastTransportTempoReliable.store(bpmValid, std::memory_order_relaxed);
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
    state.setProperty(themeNameProperty, getThemeName(), nullptr);

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

            if (restoredState.hasProperty(themeNameProperty))
                setThemeName(restoredState.getProperty(themeNameProperty).toString());

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
    const auto syncMode = getChoiceIndex(parameters, ParamIDs::scrollMode) == static_cast<int>(ScrollMode::syncBpm);
    const auto division = getChoiceIndex(parameters, ParamIDs::syncDivision);
    const auto freeSpeed = static_cast<double>(getFloatValue(parameters, ParamIDs::freeSpeedSeconds, 4.0f));

    const auto bpmFromHost = tempoReliable.load() ? std::optional<double> { hostTempoBpm.load() } : std::nullopt;

    return TimeWindowResolver::resolve(syncMode, division, freeSpeed, bpmFromHost, lastKnownBpm.load());
}

bool WaveformAudioProcessor::getRenderFrame(RenderFrame& out, int requestedSamples, int delayCompensationSamples) const
{
    int64_t endSample = 0;
    float phase = 0.0f;
    auto reliable = false;
    auto resetSuggested = false;
    auto bpmUsed = 120.0;
    auto timeSeconds = 0.0;
    auto timeValid = false;
    auto isPlaying = true;
    auto bpmReliable = false;

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
        timeSeconds = lastTransportTimeSeconds.load(std::memory_order_relaxed);
        timeValid = lastTransportTimeValid.load(std::memory_order_relaxed);
        isPlaying = lastTransportIsPlaying.load(std::memory_order_relaxed);
        bpmReliable = lastTransportTempoReliable.load(std::memory_order_relaxed);

        const auto seqEnd = renderClockSeq.load(std::memory_order_acquire);
        if (seqBegin == seqEnd)
        {
            snapshotReadOk = true;
            break;
        }
    }

    if (! snapshotReadOk)
        return false;

    const auto shiftedEnd = juce::jmax<int64_t>(0, endSample - static_cast<int64_t>(delayCompensationSamples));

    if (! analysisBuffer.copyWindowEndingAt(out.samples, requestedSamples, shiftedEnd))
        return false;

    out.phaseNormalized = juce::jlimit(0.0f, 1.0f, phase);
    out.phaseReliable = reliable;
    out.resetSuggested = resetSuggested;
    out.bpmUsed = bpmUsed;
    out.transport.timeSeconds = timeSeconds;
    out.transport.timeSecondsValid = timeValid;
    out.transport.isPlaying = isPlaying;
    out.transport.tempoReliable = bpmReliable;
    out.transport.bpm = bpmUsed;
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

void WaveformAudioProcessor::setThemeName(const juce::String& name)
{
    const juce::SpinLock::ScopedLockType scopedLock(themeLock);
    themeName = name.trim();
    if (themeName.isEmpty())
        themeName = "Default Waveform";
}

juce::String WaveformAudioProcessor::getThemeName() const
{
    const juce::SpinLock::ScopedLockType scopedLock(themeLock);
    return themeName;
}

} // namespace wvfrm

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new wvfrm::WaveformAudioProcessor();
}
