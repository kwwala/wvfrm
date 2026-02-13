#pragma once

#include "JuceIncludes.h"

#include "Parameters.h"
#include "dsp/AnalysisRingBuffer.h"
#include "dsp/LoopClock.h"
#include "dsp/TimeWindowResolver.h"

namespace wvfrm
{

class WaveformAudioProcessor : public juce::AudioProcessor
{
public:
    struct LoopRenderFrame
    {
        juce::AudioBuffer<float> samples;
        float phaseNormalized = 0.0f;
        bool phaseReliable = false;
        int64_t phaseSample = 0;
        bool isPlaying = false;
        double bpmUsed = 120.0;
        bool resetSuggested = false;
    };

    WaveformAudioProcessor();
    ~WaveformAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept;
    const juce::AudioProcessorValueTreeState& getValueTreeState() const noexcept;

    TimeWindowResolver::ResolvedWindow resolveCurrentWindow() const noexcept;
    bool copyRecentSamples(juce::AudioBuffer<float>& destination, int numSamples) const;
    double getLoopPhaseNormalized() const noexcept;
    bool getLoopRenderFrame(LoopRenderFrame& out, int requestedSamples) const;

    double getCurrentSampleRateHz() const noexcept;
    int getAnalysisCapacity() const noexcept;

    void setLastEditorSize(int width, int height) noexcept;
    juce::Rectangle<int> getLastEditorBounds() const noexcept;

private:
    juce::AudioProcessorValueTreeState parameters;
    AnalysisRingBuffer analysisBuffer;

    std::atomic<double> currentSampleRate { 44100.0 };
    std::atomic<double> hostTempoBpm { 120.0 };
    std::atomic<double> lastKnownBpm { 120.0 };
    std::atomic<bool> tempoReliable { false };
    std::atomic<double> hostPpqPosition { 0.0 };
    std::atomic<bool> ppqReliable { false };
    std::atomic<long long> processedSamples { 0 };

    std::atomic<uint64_t> renderClockSeq { 0 };
    std::atomic<int64_t> lastClockPhaseSample { 0 };
    std::atomic<float> lastClockPhase { 0.0f };
    std::atomic<bool> lastClockReliable { false };
    std::atomic<double> lastClockBpm { 120.0 };
    std::atomic<bool> lastClockResetSuggested { false };
    std::atomic<bool> lastClockIsPlaying { false };
    SyncClockState syncClockState;

    bool buildLoopRenderFrame(LoopRenderFrame& out, int requestedSamples) const;

    std::atomic<int> editorWidth { 960 };
    std::atomic<int> editorHeight { 540 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformAudioProcessor)
};

} // namespace wvfrm
