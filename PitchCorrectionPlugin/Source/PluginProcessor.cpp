/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PitchCorrectionPluginAudioProcessor::PitchCorrectionPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    addParameter(decay_parameter = new juce::AudioParameterFloat("decay_parameter", "Decay", 0.0f, 1.0f, 0.5f));
    addParameter(sensitivity_parameter = new juce::AudioParameterFloat("sensitivity_parameter", "Sensitivity", 0.0f, 0.4f, 0.2f));
    addParameter(min_frequency_parameter = new juce::AudioParameterFloat("min_frequency_parameter", "Minimum Frequency", 50.0f, 2500.0f, 80.0f));
    addParameter(max_frequency_parameter = new juce::AudioParameterFloat("max_frequency_parameter", "Maximum Frequency", 50.0f, 2500.4f, 800.0f));
}

PitchCorrectionPluginAudioProcessor::~PitchCorrectionPluginAudioProcessor()
{
}

//==============================================================================
const juce::String PitchCorrectionPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PitchCorrectionPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PitchCorrectionPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PitchCorrectionPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PitchCorrectionPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PitchCorrectionPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PitchCorrectionPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PitchCorrectionPluginAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PitchCorrectionPluginAudioProcessor::getProgramName (int index)
{
    return {};
}

void PitchCorrectionPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void PitchCorrectionPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{

    autotune = AutoTune(0.2, 80, 800, sampleRate, 0.5);
}

void PitchCorrectionPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PitchCorrectionPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void PitchCorrectionPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    autotune.set_parameters(sensitivity_parameter->get(), min_frequency_parameter->get(), max_frequency_parameter->get(), decay_parameter->get());

    double freq = 0;
    double Lmin = 0;

    auto* left_data = buffer.getWritePointer(0);
    auto* right_data = buffer.getWritePointer(1);

    for (int i = 0; i < buffer.getNumSamples(); i++) {

        autotune.add_sample(left_data[i]);
        if (i % 8 == 0) {
            autotune.add_decimated_sample();
            freq = autotune.get_fundamental_frequency();
            Lmin = getSampleRate() / freq;
        }

        if (freq != 0) {
            autotune.calculate_resample_rate(freq);
        }

        double output_sample = 0;

        autotune.update_output_addr(Lmin);

        output_sample = autotune.get_output_sample(Lmin);

        left_data[i] = output_sample;
        right_data[i] = output_sample;

        if (i % 8 == 0) {
            autotune.reset_resample_rate();
        }
    }
}

//==============================================================================
bool PitchCorrectionPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PitchCorrectionPluginAudioProcessor::createEditor()
{
    //return new PitchCorrectionPluginAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void PitchCorrectionPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void PitchCorrectionPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PitchCorrectionPluginAudioProcessor();
}
