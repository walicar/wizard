#include "MainComponent.h"

MainComponent::MainComponent() : audioSettingsComponent(audioDeviceManager), forwardFFT(fftOrder)
{
    // setup audio
    juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
                                      [this](bool granted)
                                      {
                                          int numInputChannels = granted ? 2 : 0;
                                          setAudioChannels(numInputChannels, 2);
                                      });
    startTimerHz(60);

    // setup display
    addAndMakeVisible(audioSettingsComponent);

    setOpaque(true);
    setSize(500, 500);
}

MainComponent::~MainComponent()
{
    shutDownAudio();
}

void MainComponent::paint(juce::Graphics &g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    g.setFont(juce::Font(16.0f));
    g.setColour(juce::Colours::white);
    g.drawText("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo &bufferToFill)
{
    if (bufferToFill.buffer->getNumChannels() > 0)
    {
        auto *channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);

        for (auto i = 0; i < bufferToFill.numSamples; ++i)
            pushNextSampleIntoFifo(channelData[i]);
    }
}

void MainComponent::timerCallback()
{
    if (nextFFTBlockReady)
    {
        processFFT();
        nextFFTBlockReady = false;
        repaint();
    }
}

void MainComponent::pushNextSampleIntoFifo(float sample)
{
    // if the fifo contains enough data, set a flag to say
    // that the next line should now be rendered..
    if (fifoIndex == fftSize)
    {
        if (!nextFFTBlockReady)
        {
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            std::copy(fifo.begin(), fifo.end(), fftData.begin());
            nextFFTBlockReady = true;
        }

        fifoIndex = 0;
    }

    fifo[(size_t)fifoIndex++] = sample;
}

void MainComponent::processFFT()
{
    forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());
    // create
    auto maxLevel = juce::FloatVectorOperations::findMinAndMax(fftData.data(), 256);

    for (auto y = 1; y < 256; ++y)
    {
        auto skewedProportionY = 1.0f - std::exp(std::log((float)y / (float)256) * 0.2f);
        auto fftDataIndex = (size_t)juce::jlimit(0, fftSize / 2, (int)(skewedProportionY * fftSize / 2));
        auto level = juce::jmap(fftData[fftDataIndex], 0.0f, juce::jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);

        if (y >= 100 and y <= 125)
        {
            printf("level[%d]> %f\n", y, level);
        }
    }
}

void MainComponent::releaseResources()
{
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double newSampleRate)
{
}

// Private Audio Stuff

void MainComponent::setAudioChannels(int numInputChannels, int numOutputChannels)
{
    audioDeviceManager.initialise(numInputChannels, 0, nullptr, true, {}, nullptr);
    audioDeviceManager.addAudioCallback(&audioSourcePlayer);
    audioSourcePlayer.setSource(this);
}

void MainComponent::shutDownAudio()
{
    audioSourcePlayer.setSource(nullptr);
    audioDeviceManager.removeAudioCallback(&audioSourcePlayer);
}
