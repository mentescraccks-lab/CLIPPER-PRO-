#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipperAudioProcessor)
};
