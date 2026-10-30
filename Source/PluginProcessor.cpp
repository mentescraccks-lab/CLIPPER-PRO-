#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace abdsp;

namespace
{
    inline void pushMax (std::atomic<float>& a, float v) noexcept
    {
        float cur = a.load (std::memory_order_relaxed);
        while (v > cur && ! a.compare_exchange_weak (cur, v, std::memory_order_relaxed)) {}
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AndresBeatsClipAudioProcessor::createLayout()
{
    using namespace juce;
    using Range = NormalisableRange<float>;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    auto addFloat = [&p] (const char* id, const char* name, float lo, float hi, float def)
    {
        p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, Range (lo, hi, 0.1f), def));
    };
    auto addBool = [&p] (const char* id, const char* name, bool def)
    {
        p.push_back (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def));
    };

    addFloat (ParamIDs::inputGain, "Input Gain",  -24.0f, 24.0f,   0.0f);
    addFloat (ParamIDs::ceiling,   "Ceiling",     -12.0f,  0.0f,  -0.8f);
    addFloat (ParamIDs::knee,      "Knee",          0.0f, 100.0f, 15.0f);
    addFloat (ParamIDs::gold1,     "Gold I",        0.0f, 100.0f, 42.5f);
    addFloat (ParamIDs::gold2,     "Gold II",       0.0f, 100.0f, 38.7f);
    addFloat (ParamIDs::alchemy,   "Alchemy",       0.0f, 100.0f, 35.2f);
    addFloat (ParamIDs::boxTone,   "Box Tone",      0.0f, 100.0f, 28.6f);
    addFloat (ParamIDs::mix,       "Dry/Wet",       0.0f, 100.0f, 62.5f);
    addBool  (ParamIDs::trueParallel, "True Parallel", false);
    addFloat (ParamIDs::shimmer,   "High Air Shimmer", 0.0f, 100.0f, 18.0f);
    addFloat (ParamIDs::ceilTrim,  "Ceil Trim",   -12.0f, 12.0f,   0.0f);
    addFloat (ParamIDs::output,    "Output",      -24.0f, 24.0f,   0.0f);
    addFloat (ParamIDs::trimIn,    "Trim In",     -12.0f, 12.0f,   0.0f);
    addFloat (ParamIDs::trimOut,   "Trim Out",    -12.0f, 12.0f,   0.0f);
    addBool  (ParamIDs::trimLink,  "Trim Link",   false);
    addBool  (ParamIDs::unityGain, "Unity Gain",  false);
    addBool  (ParamIDs::power,     "Power",       true);

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::oversampling, 1 },
                                                         "Oversampling", StringArray { "1x", "2x", "4x" }, 1));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { ParamIDs::clipMode, 1 },
                                                         "Clip Mode", StringArray { "Modern", "Classic", "Hard" }, 0));

    return { p.begin(), p.end() };
}

//==============================================================================
AndresBeatsClipAudioProcessor::AndresBeatsClipAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < 2; ++i)
    {
        meterRawIn[i] = 0.0f;
        meterClipIn[i] = 0.0f;
        meterOut[i] = 0.0f;
        meterGR[i] = 0.0f;
    }
    for (int i = 0; i < scopeSize; ++i)
    {
        scopePre[(size_t) i] = 0.0f;
        scopePost[(size_t) i] = 0.0f;
        scopeClip[(size_t) i] = false;
    }
}

std::atomic<float>* AndresBeatsClipAudioProcessor::P (const char* id) const
{
    return apvts.getRawParameterValue (id);
}

AndresBeatsClipAudioProcessor::Params AndresBeatsClipAudioProcessor::readParams() const
{
    Params p;
    p.inputGainDb  = P (ParamIDs::inputGain)->load();
    p.ceilingDb    = P (ParamIDs::ceiling)->load();
    p.kneePct      = P (ParamIDs::knee)->load();
    p.gold1        = P (ParamIDs::gold1)->load() * 0.01f;
    p.gold2        = P (ParamIDs::gold2)->load() * 0.01f;
    p.alchemy      = P (ParamIDs::alchemy)->load() * 0.01f;
    p.boxTone      = P (ParamIDs::boxTone)->load() * 0.01f;
    p.mixPct       = P (ParamIDs::mix)->load();
    p.shimmer      = P (ParamIDs::shimmer)->load() * 0.01f;
    p.ceilTrimDb   = P (ParamIDs::ceilTrim)->load();
    p.outputDb     = P (ParamIDs::output)->load();
    p.trimInDb     = P (ParamIDs::trimIn)->load();
    p.trimOutDb    = P (ParamIDs::trimOut)->load();
    p.trueParallel = P (ParamIDs::trueParallel)->load() > 0.5f;
    p.trimLink     = P (ParamIDs::trimLink)->load() > 0.5f;
    p.unity        = P (ParamIDs::unityGain)->load() > 0.5f;
    p.power        = P (ParamIDs::power)->load() > 0.5f;
    p.osIndex      = juce::jlimit (0, 2, (int) std::lround (P (ParamIDs::oversampling)->load()));
    p.mode         = juce::jlimit (0, 2, (int) std::lround (P (ParamIDs::clipMode)->load()));
    return p;
}

bool AndresBeatsClipAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

//==============================================================================
void AndresBeatsClipAudioProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;

    for (size_t i = 0; i < oversamplers.size(); ++i)
    {
        oversamplers[i] = std::make_unique<juce::dsp::Oversampling<float>> (
            2, i + 1, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
        oversamplers[i]->initProcessing ((size_t) kChunk);
    }

    dryBuf.setSize (2, kChunk);
    origBuf.setSize (2, kChunk);

    const double ramp = 0.02;
    inGainSm.reset (sr, ramp);   inGainSm.setCurrentAndTargetValue (1.0f);
    outGainSm.reset (sr, ramp);  outGainSm.setCurrentAndTargetValue (1.0f);
    mixSm.reset (sr, ramp);      mixSm.setCurrentAndTargetValue (1.0f);
    powerSm.reset (sr, 0.01);    powerSm.setCurrentAndTargetValue (1.0f);
    ceilSm.reset (sr, ramp);     ceilSm.setCurrentAndTargetValue (1.0f);

    const double shimFreq = juce::jmin (7000.0, sr * 0.4);
    for (int ch = 0; ch < 2; ++ch)
    {
        boxLow[ch].reset();
        boxBell[ch].reset();
        shimHP[ch].reset();
        shimHP[ch].setHighPass (sr, shimFreq);
        dc[ch].prepare (sr);
        dryDelay[ch].prepare (2048);
        bypassDelay[ch].prepare (2048);
    }

    lastBoxDb = -999.0f;
    scopeInterval = juce::jmax (1, (int) (sr / 80.0));
    scopeCounter = 0;
    scopeAccPre = scopeAccPost = 0.0f;
    scopeAccClip = false;

    currentOs = -1;
    applyOversamplingChoice (readParams().osIndex);
}

void AndresBeatsClipAudioProcessor::applyOversamplingChoice (int osIndex)
{
    currentOs = osIndex;
    int latency = 0;

    if (osIndex > 0)
    {
        auto& os = *oversamplers[(size_t) osIndex - 1];
        os.reset();
        latency = (int) std::lround (os.getLatencyInSamples());
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        dryDelay[ch].reset();
        bypassDelay[ch].reset();
        dryDelay[ch].setDelay (latency);
        bypassDelay[ch].setDelay (latency);
    }

    setLatencySamples (latency);
}

//==============================================================================
void AndresBeatsClipAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (2, buffer.getNumChannels());
    const int numSamples = buffer.getNumSamples();

    for (int ch = numCh; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    const Params p = readParams();

    if (p.osIndex != currentOs)
        applyOversamplingChoice (p.osIndex);

    // Objetivos de ganancia
    const float trimOutEff = p.trimLink ? -p.trimInDb : p.trimOutDb;
    const float unityComp  = p.unity ? -p.inputGainDb : 0.0f;
    inGainSm.setTargetValue  (dbToGain (p.inputGainDb + p.trimInDb));
    outGainSm.setTargetValue (dbToGain (p.outputDb + trimOutEff + unityComp));
    mixSm.setTargetValue     (p.trueParallel ? p.mixPct * 0.01f : 1.0f);
    powerSm.setTargetValue   (p.power ? 1.0f : 0.0f);
    ceilSm.setTargetValue    (dbToGain (juce::jlimit (-36.0f, 0.0f, p.ceilingDb + p.ceilTrimDb)));

    // Filtros que dependen de parámetros
    if (std::abs (p.boxTone - lastBoxDb) > 1.0e-4f)
    {
        lastBoxDb = p.boxTone;
        for (int ch = 0; ch < 2; ++ch)
        {
            boxLow[ch].setLowShelf (sr, 120.0, 6.0 * p.boxTone);
            boxBell[ch].setPeak (sr, 320.0, 0.8, 3.0 * p.boxTone);
        }
    }

    for (int start = 0; start < numSamples; start += kChunk)
        processChunk (buffer, numCh, start, juce::jmin (kChunk, numSamples - start), p);
}

//==============================================================================
void AndresBeatsClipAudioProcessor::clipBlock (juce::dsp::AudioBlock<float>& block, const Params& p,
                                               float ceilLin, float* preMax, float* postMax) const
{
    const float invCeil = 1.0f / ceilLin;
    const float t = 1.0f - 0.85f * (1.0f - p.kneePct * 0.01f);
    const float g1 = p.gold1, g2 = p.gold2, al = p.alchemy;
    const float halfPi = juce::MathConstants<float>::halfPi;
    const int mode = p.mode;

    for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
    {
        float* d = block.getChannelPointer (ch);
        float pre = 0.0f, post = 0.0f;

        for (size_t i = 0; i < block.getNumSamples(); ++i)
        {
            float u = d[i] * invCeil;

            // Gold I : armónicos pares (calidez), acotado
            u += g1 * 0.25f * (u * u) / (1.0f + std::abs (u));

            // Gold II: saturación tipo cinta (armónicos impares)
            u += g2 * (std::tanh (u) - u);

            // Alchemy: curva senoidal que añade densidad armónica
            const float uc = juce::jlimit (-halfPi, halfPi, u);
            u += al * (std::sin (uc) - u);

            const float y = clipSample (u, mode, t);

            pre  = juce::jmax (pre,  std::abs (u));
            post = juce::jmax (post, std::abs (y));

            d[i] = y * ceilLin;
        }

        preMax[ch]  = juce::jmax (preMax[ch],  pre);
        postMax[ch] = juce::jmax (postMax[ch], post);
    }
}

void AndresBeatsClipAudioProcessor::processChunk (juce::AudioBuffer<float>& buffer, int numCh,
                                                  int start, int n, const Params& p)
{
    float* io[2]   = { buffer.getWritePointer (0) + start, numCh > 1 ? buffer.getWritePointer (1) + start : nullptr };
    float* dry[2]  = { dryBuf.getWritePointer (0),  dryBuf.getWritePointer (1) };
    float* orig[2] = { origBuf.getWritePointer (0), origBuf.getWritePointer (1) };

    float rawPk[2] = { 0.0f, 0.0f }, gainedPk[2] = { 0.0f, 0.0f };

    // ---- 1) Ganancia de entrada + Box Tone (a la frecuencia base) ----
    for (int i = 0; i < n; ++i)
    {
        const float g = inGainSm.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
        {
            const float in = io[ch][i];
            rawPk[ch] = juce::jmax (rawPk[ch], std::abs (in));
            orig[ch][i] = in;

            float x = in * g;
            gainedPk[ch] = juce::jmax (gainedPk[ch], std::abs (x));
            dry[ch][i] = x;

            x = boxLow[ch].process (x);
            x = boxBell[ch].process (x);
            io[ch][i] = x;
        }
    }

    // ---- 2) Clipper (con oversampling opcional) ----
    const float ceilLin = ceilSm.skip (n);
    float preMax[2] = { 0.0f, 0.0f }, postMax[2] = { 0.0f, 0.0f };

    juce::dsp::AudioBlock<float> full (buffer.getArrayOfWritePointers(), (size_t) numCh, (size_t) start, (size_t) n);

    if (currentOs == 0)
    {
        clipBlock (full, p, ceilLin, preMax, postMax);
    }
    else
    {
        auto& os = *oversamplers[(size_t) currentOs - 1];
        auto up = os.processSamplesUp (full);
        clipBlock (up, p, ceilLin, preMax, postMax);
        os.processSamplesDown (full);
    }

    bool clipped = false;
    for (int ch = 0; ch < numCh; ++ch)
    {
        const float grDb = juce::jmax (0.0f, -gainToDb (postMax[ch] / juce::jmax (preMax[ch], 1.0e-6f)));
        pushMax (meterGR[ch], grDb);
        if (grDb > 0.1f)
            clipped = true;
    }
    if (clipped)
    {
        lastClipMs.store (juce::Time::getMillisecondCounter());
        scopeAccClip = true;
    }

    // ---- 3) Post: DC, Shimmer, mezcla, salida, bypass ----
    const float shim = p.shimmer;
    float outPk[2] = { 0.0f, 0.0f };

    for (int i = 0; i < n; ++i)
    {
        const float outG = outGainSm.getNextValue();
        const float mx   = mixSm.getNextValue();
        const float pw   = powerSm.getNextValue();

        float pre = 0.0f, post = 0.0f;

        for (int ch = 0; ch < numCh; ++ch)
        {
            float wet = dc[ch].process (io[ch][i]);

            const float hp = shimHP[ch].process (wet);
            wet += shim * (0.3f * hp + 0.2f * std::tanh (3.0f * hp));

            const float d = dryDelay[ch].process (dry[ch][i]);
            const float mixed = d + mx * (wet - d);
            const float processed = mixed * outG;

            const float byp = bypassDelay[ch].process (orig[ch][i]);
            const float y = byp + pw * (processed - byp);

            io[ch][i] = y;
            outPk[ch] = juce::jmax (outPk[ch], std::abs (y));
            pre  = juce::jmax (pre,  std::abs (dry[ch][i]));
            post = juce::jmax (post, std::abs (y));
        }

        scopeAccPre  = juce::jmax (scopeAccPre,  pre);
        scopeAccPost = juce::jmax (scopeAccPost, post);

        if (++scopeCounter >= scopeInterval)
        {
            const int w = scopeWrite.load (std::memory_order_relaxed);
            scopePre[(size_t) w].store (scopeAccPre, std::memory_order_relaxed);
            scopePost[(size_t) w].store (scopeAccPost, std::memory_order_relaxed);
            scopeClip[(size_t) w].store (scopeAccClip, std::memory_order_relaxed);
            scopeWrite.store ((w + 1) % scopeSize, std::memory_order_release);

            scopeCounter = 0;
            scopeAccPre = scopeAccPost = 0.0f;
            scopeAccClip = false;
        }
    }

    for (int ch = 0; ch < numCh; ++ch)
    {
        pushMax (meterRawIn[ch],  rawPk[ch]);
        pushMax (meterClipIn[ch], gainedPk[ch]);
        pushMax (meterOut[ch],    outPk[ch]);
    }
    if (numCh == 1)
    {
        pushMax (meterRawIn[1],  rawPk[0]);
        pushMax (meterClipIn[1], gainedPk[0]);
        pushMax (meterOut[1],    outPk[0]);
        pushMax (meterGR[1],     meterGR[0].load());
    }
}

//==============================================================================
juce::AudioProcessorEditor* AndresBeatsClipAudioProcessor::createEditor()
{
    return new AndresBeatsClipAudioProcessorEditor (*this);
}

void AndresBeatsClipAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void AndresBeatsClipAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AndresBeatsClipAudioProcessor();
}
