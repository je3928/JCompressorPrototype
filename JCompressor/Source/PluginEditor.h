/*
  ==============================================================================
 JCompressor
 
 Author: Jordan Evans
 Date: 11/04/2022
 
 Simple compressor prototype using audio detector and mild sigmoid function for added harmonics.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class JCompressorAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    JCompressorAudioProcessorEditor (JCompressorAudioProcessor&);
    ~JCompressorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    JCompressorAudioProcessor& audioProcessor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JCompressorAudioProcessorEditor)
};
