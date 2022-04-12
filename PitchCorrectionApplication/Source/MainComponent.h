#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/

class MainComponent : public juce::AudioAppComponent,
    public juce::ChangeListener,
    private juce::Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    //==============================================================================
    // Your private member variables go here...

    //Processing
    juce::AudioBuffer<float> input_audio_buffer;
    juce::AudioBuffer<float> output_audio_buffer;

    //GUI

    juce::AudioDeviceManager customDeviceManager;
    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioSettings;

    typedef enum
    {
        Stopped,
        Playing,
        Starting,
        Stopping
    }transportSourceState_t;

    transportSourceState_t transportSourceState;

    void loadFileButtonClicked();
    void correctPitchButtonClicked();
    void saveResultsButtonClicked();

    juce::TextButton loadFileButton;
    juce::TextButton correctPitchButton;
    juce::TextButton saveResultsButton;

    juce::Label pitchCorrectionFeedback;

    juce::Label pitchParametersLabel;

    juce::Slider decaySlider;
    juce::Label decayLabel;
    juce::Slider sensitivitySlider;
    juce::Label sensitivityLabel;
    juce::Slider maxFrequencySlider;
    juce::Label maxFrequencyLabel;
    juce::Slider minFrequencySlider;
    juce::Label minFrequencyLabel;

    juce::Label sampleRateLabel;
    juce::ComboBox sampleRateComboBox;

    void openTextButtonClicked();
    void playAudioButtonClicked();
    void stopAudioButtonClicked();
    void transportSourceStateChanged(transportSourceState_t state);
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    juce::TextButton openFileButton;
    juce::TextButton playAudioButton;
    juce::TextButton stopAudioButton;

    juce::Label titleLabel;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> playSource;
    juce::AudioTransportSource transportSource;

    void timerCallback() override
    {
        repaint();
    }

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> LPF;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
