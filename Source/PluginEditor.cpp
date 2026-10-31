#include "PluginEditor.h"

ClipperAudioProcessorEditor::ClipperAudioProcessorEditor (ClipperAudioProcessor& p)
    : juce::GenericAudioProcessorEditor (p)
{
    setSize (500, 420);
}
