#pragma once
#include "PluginProcessor.h"

// FASE 1: editor genérico provisional. La interfaz analógica se construye en la Fase 3.
class ClipperAudioProcessorEditor : public juce::GenericAudioProcessorEditor
{
public:
    explicit ClipperAudioProcessorEditor (ClipperAudioProcessor& p);
};
