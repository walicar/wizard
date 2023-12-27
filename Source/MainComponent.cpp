#include "MainComponent.h"

/**
 * TODO: Box is not spinning
 */

MainComponent::MainComponent() : audioSettingsComponent(audioDeviceManager), forwardFFT(fftOrder)
{
    if (auto *peer = getPeer())
        peer->setCurrentRenderingEngine(0);

    setOpaque(true);
    // set up openGL

    ShaderPreset shaderPreset = getPresets()[0];
    setShaderProgram(shaderPreset.vertexShader, shaderPreset.fragmentShader);
    setTexture(new TextureFromAsset("port.jpg"));

    openGLContext.setRenderer(this);
    openGLContext.attachTo(*this);
    openGLContext.setContinuousRepainting(true);

    // setup audio
    juce::RuntimePermissions::request(juce::RuntimePermissions::recordAudio,
                                      [this](bool granted)
                                      {
                                          int numInputChannels = granted ? 2 : 0;
                                          setAudioChannels(numInputChannels, 2);
                                      });
    startTimerHz(60);

    // setup display

    // TODO: temporary removal
    // addAndMakeVisible(audioSettingsComponent);

    setSize(500, 500);
}

MainComponent::~MainComponent()
{

    openGLContext.detach();
    shutDownAudio();
}

void MainComponent::paint(juce::Graphics &g)
{
    // uncomment me? after we done it?

    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    // g.setFont(juce::Font(16.0f));
    // g.setColour(juce::Colours::white);
    // g.drawText("Hello World!", getLocalBounds(), juce::Justification::centred, true);
}

void MainComponent::resized()
{
    // This is called when the MainComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    const ScopedLock lock(mutex);
    bounds = getLocalBounds();
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
        // repaint(); // TODO: ? should we replace this?
        // TODO: add setShaderProgram
        setShaderProgram(newVertexShader, newFragmentShader);
    }
}

// Public DSP
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

        // if (y >= 100 and y <= 110)
        // {
        //     printf("level[%d]> %f\n", y, level);
        // }
    }
}

// Public Audio
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double newSampleRate)
{
}

void MainComponent::releaseResources()
{
}

// Public Graphics
void MainComponent::newOpenGLContextCreated()
{
    freeAllContextObjects();
}

void MainComponent::renderOpenGL()
{
    using namespace ::juce::gl;

    const ScopedLock lock(mutex);

    jassert(OpenGLHelpers::isContextActive());

    auto desktopScale = (float)openGLContext.getRenderingScale();

    OpenGLHelpers::clear(Colour(0xff4d4d4d));

    if (textureToUse != nullptr)
        if (!textureToUse->applyTo(texture))
            textureToUse = nullptr;

    // First draw our background graphics to demonstrate the OpenGLGraphicsContext
    //     class, add this later?

    // if (doBackgroundDrawing)
    //     drawBackground2DStuff(desktopScale);

    updateShader(); // Check whether we need to compile a new shader

    if (shader.get() == nullptr)
        return;

    // Having used the juce 2D renderer, it will have messed-up a whole load of GL state, so
    // we need to initialise some important settings before doing our normal GL 3D drawing..
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glActiveTexture(GL_TEXTURE0);

    if (!openGLContext.isCoreProfile())
        glEnable(GL_TEXTURE_2D);

    glViewport(0, 0,
               roundToInt(desktopScale * (float)bounds.getWidth()),
               roundToInt(desktopScale * (float)bounds.getHeight()));

    texture.bind();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    shader->use();

    if (uniforms->projectionMatrix != nullptr)
    {
        uniforms->projectionMatrix->setMatrix4(getProjectionMatrix().mat, 1, false);
        printf("erm?");
    }

    if (uniforms->viewMatrix != nullptr)
        uniforms->viewMatrix->setMatrix4(getViewMatrix().mat, 1, false);

    if (uniforms->texture != nullptr)
        uniforms->texture->set((GLint)0);

    if (uniforms->lightPosition != nullptr)
        uniforms->lightPosition->set(-15.0f, 10.0f, 15.0f, 0.0f);

    if (uniforms->bouncingNumber != nullptr)
        uniforms->bouncingNumber->set(bouncingNumber.getValue());

    shape->draw(*attributes);

    // Reset the element buffers so child Components draw correctly
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    rotation += (float)rotationSpeed;
}

void MainComponent::openGLContextClosing()
{
    // When the context is about to close, you must use this callback to delete
    // any GPU resources while the context is still current.
    freeAllContextObjects();

    if (lastTexture != nullptr)
        delete lastTexture;
}

Matrix3D<float> MainComponent::getProjectionMatrix() const
{
    const ScopedLock lock(mutex);

    auto w = 1.0f / (scale + 0.1f);
    auto h = w * bounds.toFloat().getAspectRatio(false);

    return Matrix3D<float>::fromFrustum(-w, w, -h, h, 4.0f, 30.0f);
}

Matrix3D<float> MainComponent::getViewMatrix() const
{
    const ScopedLock lock(mutex);

    // if we had controls
    // auto viewMatrix = Matrix3D<float>::fromTranslation({0.0f, 1.0f, -10.0f}) * draggableOrientation.getRotationMatrix();
    auto viewMatrix = Matrix3D<float>::fromTranslation({0.0f, 1.0f, -10.0f});
    auto rotationMatrix = Matrix3D<float>::rotation({rotation, rotation, -0.3f});

    return viewMatrix * rotationMatrix;
}

void MainComponent::setShaderProgram(const String &vertexShader, const String &fragmentShader)
{
    const ScopedLock lock(shaderMutex); // Prevent concurrent access to shader strings and status
    newVertexShader = vertexShader;
    newFragmentShader = fragmentShader;
}

void MainComponent::setTexture(DemoTexture *t)
{
    // cool C++ stuff
    lastTexture = textureToUse = t;
}

void MainComponent::freeAllContextObjects()
{
    shape.reset();
    shader.reset();
    attributes.reset();
    uniforms.reset();
    texture.release();
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

// Private Graphics
void MainComponent::updateShader()
{
    const ScopedLock lock(shaderMutex); // Prevent concurrent access to shader strings and status

    if (newVertexShader.isNotEmpty() || newFragmentShader.isNotEmpty())
    {
        std::unique_ptr<OpenGLShaderProgram> newShader(new OpenGLShaderProgram(openGLContext));

        if (newShader->addVertexShader(OpenGLHelpers::translateVertexShaderToV3(newVertexShader)) && newShader->addFragmentShader(OpenGLHelpers::translateFragmentShaderToV3(newFragmentShader)) && newShader->link())
        {
            shape.reset();
            attributes.reset();
            uniforms.reset();

            shader.reset(newShader.release());
            shader->use();

            shape.reset(new Shape());
            attributes.reset(new Attributes(*shader));
            uniforms.reset(new Uniforms(*shader));

            statusText = "GLSL: v" + String(OpenGLShaderProgram::getLanguageVersion(), 2);
        }
        else
        {
            statusText = newShader->getLastError();
        }

        triggerAsyncUpdate();

        // in the example we use CodeDocument vertexShader to make sure we have a single truth

        newVertexShader = {};
        newFragmentShader = {};

        // I don't think we have to update this if the program doesn't use a code document?
    }
}

void MainComponent::handleAsyncUpdate() // might want to keep this function for reference
{
    const ScopedLock lock(shaderMutex); // Prevent concurrent access to shader strings and status
    // controlsOverlay->statusLabel.setText(statusText, dontSendNotification);
}