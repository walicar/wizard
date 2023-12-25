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