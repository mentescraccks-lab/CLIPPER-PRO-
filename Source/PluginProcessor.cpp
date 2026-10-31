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
    pDrive   = apvts.getRawParameterValue (ParamID::inputDrive);
    pAmount  = apvts.getRawParameterValue (ParamID::clipAmount);
    pCeiling = apvts.getRawParameterValue (ParamID::ceiling);
    pOutput  = apvts.getRawParameterValue (ParamID::output);
    pMode    = apvts.getRawParameterValue (ParamID::mode);
    pOs      = apvts.getRawParameterValue (ParamID::oversampling);
    pLink    = apvts.getRawParameterValue (ParamID::linkLR);
    pBypass  = apvts.getRawParameterValue (ParamID::bypass);
}

void ClipperAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    maxBlock = std::max (samplesPerBlock, 1);
    numChannelsPrepared = std::max (getTotalNumInputChannels(), 1);

    // Reserva de memoria aquí (nunca en processBlock).
    for (size_t f = 1; f < oversamplers.size(); ++f)
    {
        oversamplers[f] = std::make_unique<dsp::Oversampling<float>> (
            (size_t) numChannelsPrepared, f, dsp::Oversampling<float>::filterHalfBandFIREquiripple,
            true /*max quality*/, true /*latencia entera*/);
        oversamplers[f]->initProcessing ((size_t) maxBlock);
    }
    ceilBuf.assign ((size_t) maxBlock, 1.0f);
    outBuf.assign  ((size_t) maxBlock, 1.0f);

    preGain.reset  (sampleRate, 0.02);
    ceilGain.reset (sampleRate, 0.02);
    outGain.reset  (sampleRate, 0.02);
    preGain.setCurrentAndTargetValue  (clipper::dbToGain (pDrive->load() + pAmount->load()));
    ceilGain.setCurrentAndTargetValue (clipper::dbToGain (pCeiling->load()));
    outGain.setCurrentAndTargetValue  (clipper::dbToGain (pOutput->load()));

    activeOs = -1;
    applyOversamplingChoice ((int) pOs->load());
}

void ClipperAudioProcessor::applyOversamplingChoice (int index)
{
    index = juce::jlimit (0, 4, index);
    if (index == activeOs)
        return;
    activeOs = index;
    if (index == 0)
        setLatencySamples (0);
    else
    {
        oversamplers[(size_t) index]->reset();
        setLatencySamples (juce::roundToInt (oversamplers[(size_t) index]->getLatencyInSamples()));
    }
}

bool ClipperAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == AudioChannelSet::mono() || out == AudioChannelSet::stereo())
           && out == l.getMainInputChannelSet();
}

void ClipperAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    // Limitación conocida: en bypass no se compensa la latencia del oversampling.
    if (pBypass->load() > 0.5f)
        return;

    const int total = buffer.getNumSamples();
    for (int pos = 0; pos < total; pos += maxBlock)
        processChunk (buffer, pos, std::min (maxBlock, total - pos));
}

void ClipperAudioProcessor::processChunk (AudioBuffer<float>& buffer, int start, int num)
{
    const int numCh = std::min (buffer.getNumChannels(), numChannelsPrepared);
    const bool soft   = pMode->load() > 0.5f;
    const bool linked = pLink->load() > 0.5f && numCh == 2;

    applyOversamplingChoice ((int) pOs->load());
    const int osIdx = activeOs;

    preGain.setTargetValue  (clipper::dbToGain (pDrive->load() + pAmount->load()));
    ceilGain.setTargetValue (clipper::dbToGain (pCeiling->load()));
    outGain.setTargetValue  (clipper::dbToGain (pOutput->load()));

    // 1) Medidor de entrada + ganancia previa (a la tasa base, suavizada)
    float inPeak = 0.0f;
    for (int i = 0; i < num; ++i)
    {
        const float g = preGain.getNextValue();
        ceilBuf[(size_t) i] = ceilGain.getNextValue();
        outBuf[(size_t) i]  = outGain.getNextValue();
        for (int c = 0; c < numCh; ++c)
        {
            float* d = buffer.getWritePointer (c, start);
            d[i] = clipper::sanitize (d[i]);
            inPeak = std::max (inPeak, std::abs (d[i]));
            d[i] *= g;
        }
    }

    // 2) Clipping (a la tasa interna; el índice del ceiling se mapea con >> osIdx)
    float minRatio = 1.0f;
    int clipped = 0;
    auto clipBlock = [&] (dsp::AudioBlock<float>& b)
    {
        const int n = (int) b.getNumSamples();
        for (int j = 0; j < n; ++j)
        {
            const float t = ceilBuf[(size_t) (j >> osIdx)];
            if (linked)
            {
                float* l = b.getChannelPointer (0) + j;
                float* r = b.getChannelPointer (1) + j;
                const float ratio = clipper::clipLinked (*l, *r, t, soft);
                if (ratio < 0.99999f) { ++clipped; minRatio = std::min (minRatio, ratio); }
            }
            else
            {
                for (size_t c = 0; c < b.getNumChannels(); ++c)
                {
                    float* s = b.getChannelPointer (c) + j;
                    const float x = clipper::sanitize (*s);
                    const float y = clipper::clipSample (x, t, soft);
                    if (std::abs (x) > 1.0e-6f)
                    {
                        const float ratio = std::abs (y) / std::abs (x);
                        if (ratio < 0.99999f) { ++clipped; minRatio = std::min (minRatio, ratio); }
                    }
                    *s = y;
                }
            }
        }
    };

    dsp::AudioBlock<float> block (buffer);
    auto sub = block.getSubBlock ((size_t) start, (size_t) num).getSubsetChannelBlock (0, (size_t) numCh);

    if (osIdx == 0)
    {
        clipBlock (sub);                                   // ruta directa 1x
    }
    else
    {
        auto& os = *oversamplers[(size_t) osIdx];
        auto up = os.processSamplesUp (sub);
        clipBlock (up);
        os.processSamplesDown (sub);
    }

    // 3) Ganancia de salida + medidor de salida
    float outPeak = 0.0f;
    for (int c = 0; c < numCh; ++c)
    {
        float* d = buffer.getWritePointer (c, start);
        for (int i = 0; i < num; ++i)
        {
            d[i] = clipper::sanitize (d[i] * outBuf[(size_t) i]);
            outPeak = std::max (outPeak, std::abs (d[i]));
        }
    }

    meters.inputPeak.updateMax (inPeak);
    meters.outputPeak.updateMax (outPeak);
    meters.gainReduction.updateMax (1.0f - minRatio);
    meters.clipEvents.updateMax ((float) clipped);
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
