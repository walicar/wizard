#include <JuceHeader.h>


inline juce::String getAsset(const char *assetName)
{
    // actually returning the data within the .obj file
    auto assetsDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Projects").getChildFile("juce_stuff");

    auto resourceFile = assetsDir.getChildFile(assetName);
    jassert(resourceFile.existsAsFile());

    std::unique_ptr<juce::InputStream> input(resourceFile.createInputStream());
    if (input == nullptr)
        return {};

    return input->readString();
}

inline juce::Image getImage(const char *assetName)
{
    auto assetsDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Projects").getChildFile("juce_stuff");

    auto resourceFile = assetsDir.getChildFile(assetName);
    jassert(resourceFile.existsAsFile());

    std::unique_ptr<juce::InputStream> input(resourceFile.createInputStream());
    auto image = juce::ImageFileFormat::loadFrom(*input);
    // could add it to imgcache, need hashcode though...
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
