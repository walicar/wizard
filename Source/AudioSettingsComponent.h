#include <JuceHeader.h>

class AudioSettingsComponent final : public juce::Component, public juce::ChangeListener
{
public:
    AudioSettingsComponent(juce::AudioDeviceManager &);
    ~AudioSettingsComponent();

    void resized() override;

    juce::AudioDeviceManager &audioDeviceManager;

private:
    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioDeviceSelector;

    void changeListenerCallback(juce::ChangeBroadcaster *) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSettingsComponent);
};
