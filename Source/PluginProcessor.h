#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <memory>
#include "DspUtils.h"

namespace ParamIDs
{
    inline constexpr const char* inputGain    = "inputGain";
    inline constexpr const char* ceiling      = "ceiling";
    inline constexpr const char* knee         = "knee";
    inline constexpr const char* gold1        = "gold1";
    inline constexpr const char* gold2        = "gold2";
    inline constexpr const char* alchemy      = "alchemy";
    inline constexpr const char* boxTone      = "boxTone";
    inline constexpr const char* mix          = "mix";
    inline constexpr const char* trueParallel = "trueParallel";
    inline constexpr const char* shimmer      = "shimmer";
    inline constexpr const char* ceilTrim     = "ceilTrim";
    inline constexpr const char* output       = "output";
    inline constexpr const char* trimIn       = "trimIn";
    inline constexpr const char* trimOut      = "trimOut";
    inline constexpr const char* trimLink     = "trimLink";
    inline constexpr const char* unityGain    = "unityGain";
    inline constexpr const char* power        = "power";
    inline constexpr const char* oversampling = "oversampling";
    inline constexpr const char* clipMode     = "clipMode";
}

class AndresBeatsClipAudioProcessor : public juce::AudioProcessor
{
public:
    AndresBeatsClipAudioProcessor();
    ~AndresBeatsClipAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AndresBeats Clip"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::AudioProcessorValueTreeState apvts;

    //==========================================================================
    // Datos de medición (el editor los lee con exchange(0) en su timer)
    std::atomic<float> meterRawIn[2];     // entrada cruda (antes de ganancias)
    std::atomic<float> meterClipIn[2];    // nivel que llega al clipper (después de Input Gain + Trim In)
    std::atomic<float> meterOut[2];       // salida final
    std::atomic<float> meterGR[2];        // reducción de pico en dB (valor positivo)

    // Historial para la pantalla central (anillo)
    static constexpr int scopeSize = 600;
    std::array<std::atomic<float>, scopeSize> scopePre;
    std::array<std::atomic<float>, scopeSize> scopePost;
    std::array<std::atomic<bool>, scopeSize>  scopeClip;
    std::atomic<int> scopeWrite { 0 };

    std::atomic<juce::uint32> lastClipMs { 0 };

private:
    static constexpr int kChunk = 256;   // tamaño interno de procesamiento

    struct Params
    {
        float inputGainDb, ceilingDb, kneePct, gold1, gold2, alchemy, boxTone, mixPct, shimmer;
        float ceilTrimDb, outputDb, trimInDb, trimOutDb;
        bool  trueParallel, trimLink, unity, power;
        int   osIndex, mode;
    };

    Params readParams() const;
    void   applyOversamplingChoice (int osIndex);
    void   processChunk (juce::AudioBuffer<float>& buffer, int numCh, int start, int n, const Params& p);
    void   clipBlock (juce::dsp::AudioBlock<float>& block, const Params& p, float ceilLin,
                      float* preMax, float* postMax) const;

    std::array<std::atomic<float>*, 19> rawParams {};
    std::atomic<float>* P (const char* id) const;

    double sr = 44100.0;
    int currentOs = -1;

    std::array<std::unique_ptr<juce::dsp::Oversampling<float>>, 2> oversamplers; // [0]=2x, [1]=4x

    abdsp::Biquad boxLow[2], boxBell[2], shimHP[2];
    abdsp::DcBlocker dc[2];
    abdsp::SimpleDelay dryDelay[2], bypassDelay[2];

    juce::SmoothedValue<float> inGainSm, outGainSm, mixSm, powerSm, ceilSm;

    juce::AudioBuffer<float> dryBuf, origBuf;

    float lastBoxDb = -999.0f, lastShimOn = -1.0f;

    int scopeCounter = 0, scopeInterval = 600;
    float scopeAccPre = 0.0f, scopeAccPost = 0.0f;
    bool scopeAccClip = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AndresBeatsClipAudioProcessor)
};
