#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout ClipperAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto dB = [] (const char* id, const char* name, float lo, float hi, float def)
    {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name,
                   NormalisableRange<float> (lo, hi, 0.1f), def,
                   AudioParameterFloatAttributes().withLabel ("dB"));
    };

    p.push_back (dB (ParamID::inputDrive, "Input Drive", -24.0f, 24.0f, 0.0f));
    p.push_back (dB (ParamID::clipAmount, "Clip Amount",   0.0f, 18.0f, 6.0f));
    p.push_back (dB (ParamID::ceiling,    "Ceiling",      -3.0f,  0.0f, -0.1f));
    p.push_back (dB (ParamID::output,     "Output",      -24.0f, 24.0f, 0.0f));

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::mode, 1 },
                     "Clip Mode", StringArray { "Hard", "Soft" }, 0));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamID::oversampling, 1 },
                     "Oversampling", StringArray { "1x", "2x", "4x", "8x", "16x" }, 0));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::linkLR, 1 }, "Link L/R", true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { ParamID::bypass, 1 }, "Bypass", false));

    return { p.begin(), p.end() };
}

ClipperAudioProcessor::ClipperAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CLIPPER_STATE", createLayout())
{
}

void ClipperAudioProcessor::prepareToPlay (double, int) {}

bool ClipperAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == AudioChannelSet::mono() || out == AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

// FASE 1: paso directo. El DSP real se implementa en la Fase 2.
void ClipperAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());
}

AudioProcessorEditor* ClipperAudioProcessor::createEditor()
{
    return new ClipperAudioProcessorEditor (*this);
}

void ClipperAudioProcessor::getStateInformation (MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void ClipperAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ClipperAudioProcessor(); }
