#include <JuceHeader.h>
#include <BinaryData.h>

inline juce::String getAsset(const char *assetName)
{
    juce::String input(BinaryData::crate_obj);
    return input;
}

inline juce::Image getImage(const char *assetName)
{
    auto image = juce::ImageFileFormat::loadFrom(BinaryData::port_jpg, BinaryData::port_jpgSize);
    return image;
}

struct BouncingNumber
{
    virtual ~BouncingNumber() = default;

    float getValue() const
    {
        double v = fmod(phase + speed * juce::Time::getMillisecondCounterHiRes(), 2.0);
        return (float)(v >= 1.0 ? (2.0 - v) : v);
    }

protected:
    double speed = 0.0004 + 0.0007 * juce::Random::getSystemRandom().nextDouble(),
           phase = juce::Random::getSystemRandom().nextDouble();
};
