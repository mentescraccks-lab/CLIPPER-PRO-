#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>
#include <vector>
#include "DSP/ClipperDSP.h"
#include "DSP/Metering.h"

namespace ParamID
{
    inline constexpr const char* inputDrive   = "inputDrive";
    inline constexpr const char* clipAmount   = "clipAmount";
    inline constexpr const char* ceiling      = "ceiling";
    inline constexpr const char* output       = "output";
    inline constexpr const char* mode         = "mode";
    inline constexpr const char* oversampling = "oversampling";
    inline constexpr const char* linkLR       = "linkLR";
    inline constexpr const char* bypass       = "bypass";
}

class ClipperAudioProcessor : public juce::AudioProcessor
{
public:
    ClipperAudioProcessor();
    ~ClipperAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CLIPPER"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default Preset"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    ClipperMeters meters;

private:
    void processChunk (juce::AudioBuffer<float>& buffer, int start, int num);
    void applyOversamplingChoice (int index);

    std::atomic<float> *pDrive {}, *pAmount {}, *pCeiling {}, *pOutput {}, *pMode {}, *pOs {}, *pLink {}, *pBypass {};

    juce::SmoothedValue<float> preGain, ceilGain, outGain;
    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 5> oversamplers; // [0] = 1x (no se usa)
    std::vector<float> ceilBuf, outBuf;
    int maxBlock = 512, activeOs = 0, numChannelsPrepared = 2;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipperAudioProcessor)
};
