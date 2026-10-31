#pragma once
#include "PluginProcessor.h"
#include "GUI/AnalogKnob.h"
#include "GUI/LedMeter.h"
#include "GUI/Controls.h"

class ClipperAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ClipperAudioProcessorEditor (ClipperAudioProcessor&);
    ~ClipperAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Todo se coloca en coordenadas de diseño (1672x941) y el conjunto se escala a la ventana.
    struct Content : public juce::Component
    {
        juce::Image composite;
        void paint (juce::Graphics& g) override
        {
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImageAt (composite, 0, 0);
        }
    };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    ClipperAudioProcessor& proc;
    Content content;

    AnalogKnob driveKnob, clipKnob, ceilKnob, outKnob;
    ValueReadout driveVal, clipVal, ceilVal, outVal;
    ImageStateButton hardBtn, softBtn, os1, os2, os4, os8, os16;
    LedPushButton linkBtn, bypassBtn;
    LedMeter inMeter, outMeter, grMeter;

    float inDb = -100.0f, outDb = -100.0f, grDb = 0.0f;

    // Los attachments se declaran al final: se destruyen antes que los controles.
    std::unique_ptr<SliderAttachment> driveAtt, clipAtt, ceilAtt, outAtt;
    std::unique_ptr<ButtonAttachment> linkAtt, bypassAtt;
    std::unique_ptr<juce::ParameterAttachment> modeAtt, osAtt;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipperAudioProcessorEditor)
};
