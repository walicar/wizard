#include "AudioSettingsComponent.h"

AudioSettingsComponent::AudioSettingsComponent(juce::AudioDeviceManager &adm) : audioDeviceManager(adm)
{
    audioDeviceSelector.reset(new juce::AudioDeviceSelectorComponent(audioDeviceManager,
                                                                     0, 256, 0, 256, false, false, false, true));
    addAndMakeVisible(audioDeviceSelector.get());
    audioDeviceManager.addChangeListener(this);
    setSize(500, 250);
}

AudioSettingsComponent::~AudioSettingsComponent()
{
    audioDeviceManager.removeChangeListener(this);
}

void AudioSettingsComponent::resized()
{
    juce::Rectangle<int> bounds = getLocalBounds().reduced(5);
    audioDeviceSelector->setBounds(bounds.removeFromBottom(proportionOfHeight(0.5f)));
}

void AudioSettingsComponent::changeListenerCallback(juce::ChangeBroadcaster *)
{
    printf("ChangeBroadcaster event listened");
}
