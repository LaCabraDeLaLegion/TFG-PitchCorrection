#include "MainComponent.h"
#include "AutoTune.h"

//==============================================================================
MainComponent::MainComponent() : juce::AudioAppComponent(customDeviceManager),
openFileButton("Open File"), playAudioButton("Play"),
stopAudioButton("Stop"), transportSourceState(Stopped),
loadFileButton("Load File"),
correctPitchButton("Correct Pitch"),
saveResultsButton("Save Results"), 
LPF(juce::dsp::IIR::Coefficients<float>::makeLowPass(44100, 20000.0f, 0.1f))
{

    customDeviceManager.initialise(2, 2, nullptr, true);
    audioSettings.reset(new juce::AudioDeviceSelectorComponent(customDeviceManager,
        0, 2, 0, 2,
        true, true, true, true));
    addAndMakeVisible(audioSettings.get());

    // Some platforms require permissions to open input channels so request that here
    if (juce::RuntimePermissions::isRequired(juce::RuntimePermissions::recordAudio)
        && !juce::RuntimePermissions::isGranted(juce::RuntimePermissions::recordAudio))
    {
        juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
            [&](bool granted) { setAudioChannels(granted ? 2 : 0, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels(2, 2);
    }

    loadFileButton.onClick = [this] {loadFileButtonClicked(); };
    addAndMakeVisible(&loadFileButton);

    correctPitchButton.onClick = [this] {correctPitchButtonClicked(); };
    addAndMakeVisible(&correctPitchButton);

    saveResultsButton.onClick = [this] {saveResultsButtonClicked(); };
    addAndMakeVisible(&saveResultsButton);

    addAndMakeVisible(&pitchCorrectionFeedback);
    pitchCorrectionFeedback.setText("Pitch correction takes time. A message will be shown here when it's done.", juce::dontSendNotification);
    pitchCorrectionFeedback.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(&pitchParametersLabel);
    pitchParametersLabel.setText("Pitch Correction Parameters", juce::dontSendNotification);
    pitchParametersLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(&decaySlider);
    decaySlider.setRange(0, 1, 0.1);
    decaySlider.setValue(0.5);
    addAndMakeVisible(&decayLabel);
    decayLabel.setText("Decay", juce::dontSendNotification);
    decayLabel.attachToComponent(&decaySlider, true);

    addAndMakeVisible(&sensitivitySlider);
    sensitivitySlider.setRange(0, 0.4, 0.05);
    sensitivitySlider.setValue(0.2);
    addAndMakeVisible(&sensitivityLabel);
    sensitivityLabel.setText("Sensitivity", juce::dontSendNotification);
    sensitivityLabel.attachToComponent(&sensitivitySlider, true);

    addAndMakeVisible(&maxFrequencySlider);
    maxFrequencySlider.setRange(50, 2500, 1);
    maxFrequencySlider.setTextValueSuffix(" Hz");
    maxFrequencySlider.setValue(800.0);
    addAndMakeVisible(&maxFrequencyLabel);
    maxFrequencyLabel.setText("Max frequency ", juce::dontSendNotification);
    maxFrequencyLabel.attachToComponent(&maxFrequencySlider, true);

    addAndMakeVisible(&minFrequencySlider);
    minFrequencySlider.setRange(50, 2500, 1);
    minFrequencySlider.setTextValueSuffix(" Hz");
    minFrequencySlider.setValue(80.0);
    minFrequencySlider.setSkewFactorFromMidPoint(1250);
    addAndMakeVisible(&minFrequencyLabel);
    minFrequencyLabel.setText("Min frequency ", juce::dontSendNotification);
    minFrequencyLabel.attachToComponent(&minFrequencySlider, true);


    addAndMakeVisible(&sampleRateComboBox);
    sampleRateComboBox.addItem("48000", 1);
    sampleRateComboBox.addItem("44100", 2);
    sampleRateComboBox.setSelectedId(1);

    addAndMakeVisible(&sampleRateLabel);
    sampleRateLabel.setText("Sample rate", juce::dontSendNotification);
    sampleRateLabel.attachToComponent(&sampleRateComboBox, false); //With false it stays above
    sampleRateLabel.setJustificationType(juce::Justification::centred);



    //------------------------------------------------ Antiguo

    openFileButton.onClick = [this] {openTextButtonClicked(); };
    addAndMakeVisible(&openFileButton);

    playAudioButton.onClick = [this] {playAudioButtonClicked(); };
    playAudioButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    playAudioButton.setEnabled(true);
    addAndMakeVisible(&playAudioButton);

    stopAudioButton.onClick = [this] {stopAudioButtonClicked(); };
    stopAudioButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    stopAudioButton.setEnabled(false);
    addAndMakeVisible(&stopAudioButton);

    formatManager.registerBasicFormats();
    transportSource.addChangeListener(this);

    addAndMakeVisible(&titleLabel);
    titleLabel.setText("Wenasssss", juce::dontSendNotification);

    // Make sure you set the size of the component after
    // you add any child components.
    setSize(600, 400);
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::loadFileButtonClicked()
{
    juce::FileChooser chooser("Open a Wav file", juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav");

    pitchCorrectionFeedback.setText("Pitch correction takes time. A message will be shown here when it's done.", juce::dontSendNotification);

    if (chooser.browseForFileToOpen())
    {
        juce::File audioFile;
        audioFile = chooser.getResult();

        juce::AudioFormatReader* reader = formatManager.createReaderFor(audioFile);

        if (reader != nullptr)
        {
            input_audio_buffer.setSize(reader->numChannels, reader->lengthInSamples);
            output_audio_buffer.setSize(reader->numChannels, reader->lengthInSamples);

            reader->read(&input_audio_buffer, 0, reader->lengthInSamples, 0, true, true);
            reader->read(&output_audio_buffer, 0, reader->lengthInSamples, 0, true, true);

            //Instead of 40 I use 80 because I also have to write on the output audio buffer
            startTimer(80);

            std::unique_ptr<juce::AudioFormatReaderSource> tempSource(new juce::AudioFormatReaderSource(reader, true));
            transportSource.setSource(tempSource.get());
            transportSourceStateChanged(Stopped);
            playSource.reset(tempSource.release());
        }

    }
}

void MainComponent::correctPitchButtonClicked()
{
    double freq = 0;
    double Lmin = 0;

    auto* left_data = output_audio_buffer.getWritePointer(0);
    //auto* right_data = output_audio_buffer.getWritePointer(1);

    float sample_rate = 48000;
    int id = sampleRateComboBox.getSelectedId();
    if (id == 2) {
        sample_rate = 44100;
    }

    AutoTune autotune = AutoTune(sensitivitySlider.getValue(), minFrequencySlider.getValue(), maxFrequencySlider.getValue(), sample_rate, decaySlider.getValue());

    for (int index = 0; index < output_audio_buffer.getNumSamples(); index++) {

        autotune.add_sample(left_data[index]);
        if (index % 8 == 0) {
            autotune.add_decimated_sample();
            freq = autotune.get_fundamental_frequency();
            Lmin = sample_rate / freq;
        }

        if (freq != 0) {
            autotune.calculate_resample_rate(freq);
        }

        double output_sample = 0;

        autotune.update_output_addr(Lmin);

        output_sample = autotune.get_output_sample(Lmin);

        left_data[index] = output_sample;
        //right_data[index] = output_sample;

        if (index % 8 == 0) {
            autotune.reset_resample_rate();
        }

        if (index == 763039) {
            int hola = 0;
        }

    }

    pitchCorrectionFeedback.setText("Pitch correction finished", juce::dontSendNotification);
}

void MainComponent::saveResultsButtonClicked()
{
    juce::FileChooser chooser("Choose a file to save the results", juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav");

    //true = warns the user about overwriting files
    if (chooser.browseForFileToSave(true))
    {
        juce::File audioFile;
        audioFile = chooser.getResult();

        audioFile.deleteFile(); //We delete the file before writing so we warn the user about overwriting
        
        juce::WavAudioFormat format;
        std::unique_ptr<juce::AudioFormatWriter> writer;

        writer.reset(format.createWriterFor(new juce::FileOutputStream(audioFile),
            44100,
            output_audio_buffer.getNumChannels(),
            24,
            {},
            0));

        if (writer != nullptr)
            writer->writeFromAudioSampleBuffer(output_audio_buffer, 0, output_audio_buffer.getNumSamples());

        startTimer(40);

    }
}

void MainComponent::openTextButtonClicked()
{

    juce::FileChooser chooser("Open a Wav or AIFF file", juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.aiff;*.mp3");

    if (chooser.browseForFileToOpen())
    {
        juce::File audioFile;
        audioFile = chooser.getResult();

        juce::AudioFormatReader* reader = formatManager.createReaderFor(audioFile);

        if (reader != nullptr)
        {
            startTimer(40);
            std::unique_ptr<juce::AudioFormatReaderSource> tempSource(new juce::AudioFormatReaderSource(reader, true));
            transportSource.setSource(tempSource.get());
            transportSourceStateChanged(Stopped);
            playSource.reset(tempSource.release());
        }

    }

}

void MainComponent::playAudioButtonClicked()
{
    transportSourceStateChanged(Starting);
}

void MainComponent::stopAudioButtonClicked()
{
    transportSourceStateChanged(Stopping);
}

void MainComponent::transportSourceStateChanged(transportSourceState_t state)
{
    if (state != transportSourceState)
    {
        transportSourceState = state;

        switch (transportSourceState)
        {
        case Stopped:
            playAudioButton.setEnabled(true);
            transportSource.setPosition(0.0);
            break;
        case Playing:
            playAudioButton.setEnabled(true);
            break;
        case Starting:
            playAudioButton.setEnabled(false);
            stopAudioButton.setEnabled(true);
            transportSource.start();
            break;
        case Stopping:
            playAudioButton.setEnabled(true);
            stopAudioButton.setEnabled(false);
            transportSource.stop();
            break;
        }
    }
}


void MainComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &transportSource)
    {
        if (transportSource.isPlaying())
        {
            transportSourceStateChanged(Playing);
        }
        else
        {
            transportSourceStateChanged(Stopped);
        }

    }
}


void MainComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    transportSource.getNextAudioBlock(bufferToFill);
}

void MainComponent::releaseResources()
{
    // This will be called when the audio device stops, or when it is being
    // restarted due to a setting change.

    // For more details, see the help for AudioProcessor::releaseResources()
}

//==============================================================================
void MainComponent::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));


    g.setColour(juce::Colours::white);

    juce::Line<float> line_1(juce::Point<float>(10, 110), juce::Point<float>(getWidth() / 3 - 10, 110));

    g.drawLine(line_1, 1.0f);

    juce::Line<float> line_2(juce::Point<float>(getWidth() / 3 * 2 + 10, 110), juce::Point<float>(getWidth() - 10, 110));

    g.drawLine(line_2, 1.0f);
}

void MainComponent::resized()
{
    loadFileButton.setBounds(10, 10, getWidth()/3 - 10, 30);
    correctPitchButton.setBounds(getWidth() / 3 + 10, 10, getWidth() / 3 - 10, 30);
    saveResultsButton.setBounds(getWidth() / 3 * 2 + 10, 10, getWidth() / 3 - 20, 30);
    
    pitchCorrectionFeedback.setBounds(20, 50, getWidth() - 20, 30);

    pitchParametersLabel.setBounds(getWidth() / 3 + 10, 90, getWidth() / 3 - 10, 30);

    decaySlider.setBounds(getWidth() / 10 + 10, 130, getWidth() - getWidth() / 10 - 20, 30);
    sensitivitySlider.setBounds(getWidth() / 10 + 10, 160, getWidth() - getWidth() / 10 - 20, 30);
    maxFrequencySlider.setBounds(getWidth() / 10 + 10, 190, getWidth() - getWidth() / 10 - 20, 30);
    minFrequencySlider.setBounds(getWidth() / 10 + 10, 220, getWidth() - getWidth() / 10 - 20, 30);

    sampleRateComboBox.setBounds(20, 280, getWidth() / 2 - 10, 30);


    //openFileButton.setBounds(10, 90, getWidth() - 20, 30);
    //playAudioButton.setBounds(10, 130, getWidth() - 20, 30);
    //stopAudioButton.setBounds(10, 170, getWidth() - 20, 30);
    //audioSettings->setBounds(10, 130, getWidth() - 20, 100);
}
