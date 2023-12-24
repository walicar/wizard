#pragma once

#include <JuceHeader.h>
#include "AudioSettingsComponent.h"

class MainComponent  :  public juce::Component, public juce::AudioSource, private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void getNextAudioBlock(const juce::AudioSourceChannelInfo&) override;

    void timerCallback() override;

    void pushNextSampleIntoFifo (float);

    void processFFT();

    // Need to be implemented
    void prepareToPlay (int, double) override;
    void releaseResources() override;

    enum {
        fftOrder = 10,
        fftSize = 1 << fftOrder,
    };


private:
    // Audio
    juce::AudioDeviceManager audioDeviceManager;
    juce::AudioSourcePlayer audioSourcePlayer;
    AudioSettingsComponent audioSettingsComponent;
    
    void setAudioChannels(int, int);
    void shutDownAudio();

    // DSP Stuff
    juce::dsp::FFT forwardFFT;
    std::array<float, fftSize> fifo;
    std::array<float, fftSize * 2> fftData; // fftSize * 2 to account for real and complex components
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
