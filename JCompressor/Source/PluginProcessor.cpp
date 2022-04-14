/*
  ==============================================================================
 JCompressor
 
 Author: Jordan Evans
 Date: 11/04/2022
 
 Simple compressor prototype using audio detector and mild sigmoid function for added harmonics.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

//==============================================================================
JCompressorAudioProcessor::JCompressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), valuetree (*this, nullptr, "PARAMETERS",
                                { std::make_unique<AudioParameterFloat> ("Thresh", "THRESH", NormalisableRange<float> (-60.0f, 0.0f), -20), std::make_unique<AudioParameterFloat> ("Attack", "ATTACK", NormalisableRange<float> (3.0f, 500.0f), 3),std::make_unique<AudioParameterFloat> ("Release", "RELEASE", NormalisableRange<float> (20.0f, 1000.0f), 500),std::make_unique<AudioParameterFloat> ("Makeup", "MAKEUP", NormalisableRange<float> (0.0f, 12.0f), 0),
                                    std::make_unique<AudioParameterFloat> ("Ratio", "RATIO", NormalisableRange<float> (2.0f, 20.0f), 2)})

#endif
{
}

JCompressorAudioProcessor::~JCompressorAudioProcessor()
{
}

//==============================================================================
const juce::String JCompressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool JCompressorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool JCompressorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool JCompressorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double JCompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int JCompressorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int JCompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void JCompressorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String JCompressorAudioProcessor::getProgramName (int index)
{
    return {};
}

void JCompressorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void JCompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    envdetectl.reset(sampleRate);
    envdetectr.reset(sampleRate);
    
    
}

void JCompressorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool JCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void JCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    auto th = valuetree.getRawParameterValue("Thresh");
    auto at = valuetree.getRawParameterValue("Attack");
    auto rl = valuetree.getRawParameterValue("Release");
    auto mk = valuetree.getRawParameterValue("Makeup");
    auto rt = valuetree.getRawParameterValue("Ratio");
    
    double thres = th->load();
    double attack = at->load();
    double release = rl->load();
    double makeup = mk->load();
    double ratio = rt->load();
    
    AudioDetectorParameters envparams = envdetectl.getParameters();
    
    envparams.attackTime_mSec = attack;
    envparams.detect_dB = true;
    envparams.releaseTime_mSec = release;
    envparams.clampToUnityMax = true;
    envparams.detectMode = TLD_AUDIO_DETECT_MODE_PEAK;
    
    envdetectl.setParameters(envparams);
    envdetectr.setParameters(envparams);
    
    //juce::dsp::AudioBlock<float> Block;
    
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear();

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    
    auto* channelDataL = buffer.getWritePointer(0);
    auto* channelDataR = buffer.getWritePointer(1);
    
    for (int i = 0; i < buffer.getNumSamples(); i++){
        
        double xnl = channelDataL[i];
        double xnr = channelDataR[i];
        
        double ynl = 0.00000;
        double ynr = 0.00000;
        
        double detectorl = envdetectl.processAudioSample(xnl);
        double detectorr = envdetectr.processAudioSample(xnr);
        
        if(detectorl <= thres)
            ynl = detectorl;
        
        if(detectorr <= thres)
            ynr = detectorr;
        
        if (detectorl > thres)
            ynl = (thres + ((detectorl - thres)/ratio));
        
        if (detectorr > thres)
            ynr = (thres + ((detectorr - thres)/ratio));
        
        double gnl = dB2Raw(ynl - detectorl);
        double gnr = dB2Raw(ynr - detectorr);
    
        
        double makeupcooked = dB2Raw(makeup);
        
        xnl = xnl * gnl;
        xnr = xnr * gnr;
        
        ynl = xnl;
        ynr = xnr;
        
        ynl = ynl * makeupcooked;
        ynr = ynr * makeupcooked;
        
        channelDataL[i] = ((3 * ynl)/2) * (1 - ((ynl * ynl) /3));
        channelDataR[i] = ((3 * ynr)/2) * (1 - ((ynr * ynr) /3));
        
    
    }

    
}

//==============================================================================
bool JCompressorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* JCompressorAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void JCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void JCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JCompressorAudioProcessor();
}
