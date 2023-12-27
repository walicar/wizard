#pragma once

#include <JuceHeader.h>
#include "AudioSettingsComponent.h"
#include "OpenGLDS.h"

class MainComponent  :  public juce::Component, public juce::AudioSource, private juce::Timer, private juce::OpenGLRenderer, private juce::AsyncUpdater
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

    // Graphics
    void newOpenGLContextCreated() override;
    void openGLContextClosing() override;
    void renderOpenGL() override;
    juce::Matrix3D<float> getProjectionMatrix() const;
    juce::Matrix3D<float> getViewMatrix() const;
    void setShaderProgram(const juce::String&, const juce::String&);
    void setTexture (DemoTexture*);
    void freeAllContextObjects();

    float scale = 1.0f, rotationSpeed = 0.0f;
    juce::CriticalSection mutex;
    juce::Rectangle<int> bounds;
    BouncingNumber bouncingNumber;

    // DSP
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

    // Graphics
    float rotation = 0.0f;
    juce::OpenGLContext openGLContext;

    std::unique_ptr<juce::OpenGLShaderProgram> shader;
    std::unique_ptr<Shape> shape;
    std::unique_ptr<Attributes> attributes;
    std::unique_ptr<Uniforms> uniforms;

    juce::OpenGLTexture texture;
    DemoTexture* textureToUse = nullptr;
    DemoTexture* lastTexture  = nullptr;

    juce::CriticalSection shaderMutex;
    juce::String newVertexShader, newFragmentShader, statusText;

    void updateShader();
    void handleAsyncUpdate() override;

    // DSP Stuff
    juce::dsp::FFT forwardFFT;
    std::array<float, fftSize> fifo;
    std::array<float, fftSize * 2> fftData; // fftSize * 2 to account for real and complex components
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
