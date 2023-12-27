// OpenGL Data Structures
// Code from JUCE OpenGLUtils.h

#include <JuceHeader.h>
#include "Utilities.h"
#include "WavefrontObjParser.h"

struct Vertex
{
    float position[3];
    float normal[3];
    float colour[4];
    float texCoord[2];
};

// Link vertex Attributes 5.2
struct Attributes
{
    explicit Attributes(juce::OpenGLShaderProgram &shader)
    {
        position.reset(createAttribute(shader, "position"));
        normal.reset(createAttribute(shader, "normal"));
        sourceColour.reset(createAttribute(shader, "sourceColour"));
        textureCoordIn.reset(createAttribute(shader, "textureCoordIn"));
    }

    void enable()
    {
        using namespace ::juce::gl;

        if (position.get() != nullptr)
        {
            glVertexAttribPointer(position->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
            glEnableVertexAttribArray(position->attributeID);
        }

        if (normal.get() != nullptr)
        {
            glVertexAttribPointer(normal->attributeID, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)(sizeof(float) * 3));
            glEnableVertexAttribArray(normal->attributeID);
        }

        if (sourceColour.get() != nullptr)
        {
            glVertexAttribPointer(sourceColour->attributeID, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)(sizeof(float) * 6));
            glEnableVertexAttribArray(sourceColour->attributeID);
        }

        if (textureCoordIn.get() != nullptr)
        {
            glVertexAttribPointer(textureCoordIn->attributeID, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid *)(sizeof(float) * 10));
            glEnableVertexAttribArray(textureCoordIn->attributeID);
        }
    }

    void disable()
    {
        using namespace ::juce::gl;

        if (position != nullptr)
            glDisableVertexAttribArray(position->attributeID);
        if (normal != nullptr)
            glDisableVertexAttribArray(normal->attributeID);
        if (sourceColour != nullptr)
            glDisableVertexAttribArray(sourceColour->attributeID);
        if (textureCoordIn != nullptr)
            glDisableVertexAttribArray(textureCoordIn->attributeID);
    }

    std::unique_ptr<juce::OpenGLShaderProgram::Attribute> position, normal, sourceColour, textureCoordIn;

private:
    static juce::OpenGLShaderProgram::Attribute *createAttribute(juce::OpenGLShaderProgram &shader,
                                                                 const char *attributeName)
    {
        using namespace ::juce::gl;
        if (glGetAttribLocation(shader.getProgramID(), attributeName) < 0)
            return nullptr;

        return new juce::OpenGLShaderProgram::Attribute(shader, attributeName);
    }
};

// Uniform Values 6.4
struct Uniforms
{
    explicit Uniforms(juce::OpenGLShaderProgram &shader)
    {
        projectionMatrix.reset(createUniform(shader, "projectionMatrix"));
        viewMatrix.reset(createUniform(shader, "viewMatrix"));
        texture.reset(createUniform(shader, "demoTexture"));
        lightPosition.reset(createUniform(shader, "lightPosition"));
        bouncingNumber.reset(createUniform(shader, "bouncingNumber"));
    }

    std::unique_ptr<juce::OpenGLShaderProgram::Uniform> projectionMatrix, viewMatrix, texture, lightPosition, bouncingNumber;

private:
    static juce::OpenGLShaderProgram::Uniform *createUniform(juce::OpenGLShaderProgram &shader,
                                                             const char *uniformName)
    {
        using namespace ::juce::gl;

        if (glGetUniformLocation(shader.getProgramID(), uniformName) < 0)
            return nullptr;

        return new juce::OpenGLShaderProgram::Uniform(shader, uniformName);
    }
};

struct Shape
{
    Shape()
    {
        if (shapeFile.load(getAsset("crate.obj")).wasOk()) // TODO: hardcoded
            for (auto *s : shapeFile.shapes)
                vertexBuffers.add(new VertexBuffer(*s));
    }

    void draw(Attributes &attributes)
    {
        using namespace ::juce::gl;

        for (auto *vertexBuffer : vertexBuffers)
        {
            vertexBuffer->bind();

            attributes.enable();
            glDrawElements(GL_TRIANGLES, vertexBuffer->numIndices, GL_UNSIGNED_INT, nullptr);
            attributes.disable();
        }
    }

private:
    struct VertexBuffer
    {
        explicit VertexBuffer(WavefrontObjFile::Shape &shape)
        {
            using namespace ::juce::gl;

            numIndices = shape.mesh.indices.size();

            juce::gl::glGenBuffers(1, &vertexBuffer);
            juce::gl::glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);

            Array<Vertex> vertices;
            createVertexListFromMesh(shape.mesh, vertices, Colours::green);

            glBufferData(GL_ARRAY_BUFFER, vertices.size() * (int)sizeof(Vertex),
                         vertices.getRawDataPointer(), GL_STATIC_DRAW);

            glGenBuffers(1, &indexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * (int)sizeof(juce::uint32),
                         shape.mesh.indices.getRawDataPointer(), GL_STATIC_DRAW);
        }

        ~VertexBuffer()
        {
            using namespace ::juce::gl;

            glDeleteBuffers(1, &vertexBuffer);
            glDeleteBuffers(1, &indexBuffer);
        }

        void bind()
        {
            using namespace ::juce::gl;

            glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        }

        GLuint vertexBuffer, indexBuffer;
        int numIndices;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VertexBuffer)
    };

    WavefrontObjFile shapeFile;
    juce::OwnedArray<VertexBuffer> vertexBuffers;

    static void createVertexListFromMesh(const WavefrontObjFile::Mesh &mesh, juce::Array<Vertex> &list, juce::Colour colour)
    {
        auto scale = 0.2f;
        WavefrontObjFile::TextureCoord defaultTexCoord = {0.5f, 0.5f};
        WavefrontObjFile::Vertex defaultNormal = {0.5f, 0.5f, 0.5f};

        for (int i = 0; i < mesh.vertices.size(); ++i)
        {
            auto &v = mesh.vertices.getReference(i);

            auto &n = (i < mesh.normals.size() ? mesh.normals.getReference(i)
                                               : defaultNormal);

            auto &tc = (i < mesh.textureCoords.size() ? mesh.textureCoords.getReference(i)
                                                      : defaultTexCoord);

            list.add({{
                          scale * v.x,
                          scale * v.y,
                          scale * v.z,
                      },
                      {
                          scale * n.x,
                          scale * n.y,
                          scale * n.z,
                      },
                      {colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha()},
                      {tc.x, tc.y}});
        }
    }
};

struct ShaderPreset
{
    const char *name;
    const char *vertexShader;
    const char *fragmentShader;
};

struct DemoTexture
{
    virtual ~DemoTexture() {}
    virtual bool applyTo(OpenGLTexture &) = 0;

    String name;
};

static Image resizeImageToPowerOfTwo(Image image)
{
    if (!(isPowerOfTwo(image.getWidth()) && isPowerOfTwo(image.getHeight())))
        return image.rescaled(jmin(1024, nextPowerOfTwo(image.getWidth())),
                              jmin(1024, nextPowerOfTwo(image.getHeight())));

    return image;
}

struct TextureFromAsset final : public DemoTexture
{
    // uh? ambigious error using GLBIND... if we remove juce:: prefixes?
    TextureFromAsset(juce::String assetName)
    {
        name = assetName;
        image = resizeImageToPowerOfTwo(getImage(assetName.toRawUTF8()));
    }

    juce::Image image;

    bool applyTo(OpenGLTexture &texture) override
    {
        texture.loadImage(image);
        return false;
    }
};

// Presets from JUCE OpenGLDemo example

static juce::Array<ShaderPreset> getPresets()
{
#define SHADER_DEMO_HEADER                                \
    "/*  This is a live OpenGL Shader demo.\n"            \
    "    Edit the shader program below and it will be \n" \
    "    compiled and applied to the model above!\n"      \
    "*/\n\n"

    ShaderPreset presets[] =
        {
            {"Texture + Lighting",

             SHADER_DEMO_HEADER
             "attribute vec4 position;\n"
             "attribute vec4 normal;\n"
             "attribute vec4 sourceColour;\n"
             "attribute vec2 textureCoordIn;\n"
             "\n"
             "uniform mat4 projectionMatrix;\n"
             "uniform mat4 viewMatrix;\n"
             "uniform vec4 lightPosition;\n"
             "\n"
             "varying vec4 destinationColour;\n"
             "varying vec2 textureCoordOut;\n"
             "varying float lightIntensity;\n"
             "\n"
             "void main()\n"
             "{\n"
             "    destinationColour = sourceColour;\n"
             "    textureCoordOut = textureCoordIn;\n"
             "\n"
             "    vec4 light = viewMatrix * lightPosition;\n"
             "    lightIntensity = dot (light, normal);\n"
             "\n"
             "    gl_Position = projectionMatrix * viewMatrix * position;\n"
             "}\n",

             SHADER_DEMO_HEADER
#if JUCE_OPENGL_ES
             "varying lowp vec4 destinationColour;\n"
             "varying lowp vec2 textureCoordOut;\n"
             "varying highp float lightIntensity;\n"
#else
             "varying vec4 destinationColour;\n"
             "varying vec2 textureCoordOut;\n"
             "varying float lightIntensity;\n"
#endif
             "\n"
             "uniform sampler2D demoTexture;\n"
             "\n"
             "void main()\n"
             "{\n"
#if JUCE_OPENGL_ES
             "   highp float l = max (0.3, lightIntensity * 0.3);\n"
             "   highp vec4 colour = vec4 (l, l, l, 1.0);\n"
#else
             "   float l = max (0.3, lightIntensity * 0.3);\n"
             "   vec4 colour = vec4 (l, l, l, 1.0);\n"
#endif
             "    gl_FragColor = colour * texture2D (demoTexture, textureCoordOut);\n"
             "}\n"},

            {"Textured",

             SHADER_DEMO_HEADER
             "attribute vec4 position;\n"
             "attribute vec4 sourceColour;\n"
             "attribute vec2 textureCoordIn;\n"
             "\n"
             "uniform mat4 projectionMatrix;\n"
             "uniform mat4 viewMatrix;\n"
             "\n"
             "varying vec4 destinationColour;\n"
             "varying vec2 textureCoordOut;\n"
             "\n"
             "void main()\n"
             "{\n"
             "    destinationColour = sourceColour;\n"
             "    textureCoordOut = textureCoordIn;\n"
             "    gl_Position = projectionMatrix * viewMatrix * position;\n"
             "}\n",

             SHADER_DEMO_HEADER
#if JUCE_OPENGL_ES
             "varying lowp vec4 destinationColour;\n"
             "varying lowp vec2 textureCoordOut;\n"
#else
             "varying vec4 destinationColour;\n"
             "varying vec2 textureCoordOut;\n"
#endif
             "\n"
             "uniform sampler2D demoTexture;\n"
             "\n"
             "void main()\n"
             "{\n"
             "    gl_FragColor = texture2D (demoTexture, textureCoordOut);\n"
             "}\n"},

            {"Flat Colour",

             SHADER_DEMO_HEADER
             "attribute vec4 position;\n"
             "attribute vec4 sourceColour;\n"
             "attribute vec2 textureCoordIn;\n"
             "\n"
             "uniform mat4 projectionMatrix;\n"
             "uniform mat4 viewMatrix;\n"
             "\n"
             "varying vec4 destinationColour;\n"
             "varying vec2 textureCoordOut;\n"
             "\n"
             "void main()\n"
             "{\n"
             "    destinationColour = sourceColour;\n"
             "    textureCoordOut = textureCoordIn;\n"
             "    gl_Position = projectionMatrix * viewMatrix * position;\n"
             "}\n",

             SHADER_DEMO_HEADER
#if JUCE_OPENGL_ES
             "varying lowp vec4 destinationColour;\n"
             "varying lowp vec2 textureCoordOut;\n"
#else
             "varying vec4 destinationColour;\n"
             "varying vec2 textureCoordOut;\n"
#endif
             "\n"
             "void main()\n"
             "{\n"
             "    gl_FragColor = destinationColour;\n"
             "}\n"},

            {"Rainbow",

             SHADER_DEMO_HEADER
             "attribute vec4 position;\n"
             "attribute vec4 sourceColour;\n"
             "attribute vec2 textureCoordIn;\n"
             "\n"
             "uniform mat4 projectionMatrix;\n"
             "uniform mat4 viewMatrix;\n"
             "\n"
             "varying vec4 destinationColour;\n"
             "varying vec2 textureCoordOut;\n"
             "\n"
             "varying float xPos;\n"
             "varying float yPos;\n"
             "varying float zPos;\n"
             "\n"
             "void main()\n"
             "{\n"
             "    vec4 v = vec4 (position);\n"
             "    xPos = clamp (v.x, 0.0, 1.0);\n"
             "    yPos = clamp (v.y, 0.0, 1.0);\n"
             "    zPos = clamp (v.z, 0.0, 1.0);\n"
             "    gl_Position = projectionMatrix * viewMatrix * position;\n"
             "}",

             SHADER_DEMO_HEADER
#if JUCE_OPENGL_ES
             "varying lowp vec4 destinationColour;\n"
             "varying lowp vec2 textureCoordOut;\n"
             "varying lowp float xPos;\n"
             "varying lowp float yPos;\n"
             "varying lowp float zPos;\n"
#else
             "varying vec4 destinationColour;\n"
             "varying vec2 textureCoordOut;\n"
             "varying float xPos;\n"
             "varying float yPos;\n"
             "varying float zPos;\n"
#endif
             "\n"
             "void main()\n"
             "{\n"
             "    gl_FragColor = vec4 (xPos, yPos, zPos, 1.0);\n"
             "}"},

            {"Changing Colour",

             SHADER_DEMO_HEADER
             "attribute vec4 position;\n"
             "attribute vec2 textureCoordIn;\n"
             "\n"
             "uniform mat4 projectionMatrix;\n"
             "uniform mat4 viewMatrix;\n"
             "\n"
             "varying vec2 textureCoordOut;\n"
             "\n"
             "void main()\n"
             "{\n"
             "    textureCoordOut = textureCoordIn;\n"
             "    gl_Position = projectionMatrix * viewMatrix * position;\n"
             "}\n",

             SHADER_DEMO_HEADER
             "#define PI 3.1415926535897932384626433832795\n"
             "\n"
#if JUCE_OPENGL_ES
             "precision mediump float;\n"
             "varying lowp vec2 textureCoordOut;\n"
#else
             "varying vec2 textureCoordOut;\n"
#endif
             "uniform float bouncingNumber;\n"
             "\n"
             "void main()\n"
             "{\n"
             "   float b = bouncingNumber;\n"
             "   float n = b * PI * 2.0;\n"
             "   float sn = (sin (n * textureCoordOut.x) * 0.5) + 0.5;\n"
             "   float cn = (sin (n * textureCoordOut.y) * 0.5) + 0.5;\n"
             "\n"
             "   vec4 col = vec4 (b, sn, cn, 1.0);\n"
             "   gl_FragColor = col;\n"
             "}\n"},

            {"Simple Light",

             SHADER_DEMO_HEADER
             "attribute vec4 position;\n"
             "attribute vec4 normal;\n"
             "\n"
             "uniform mat4 projectionMatrix;\n"
             "uniform mat4 viewMatrix;\n"
             "uniform vec4 lightPosition;\n"
             "\n"
             "varying float lightIntensity;\n"
             "\n"
             "void main()\n"
             "{\n"
             "    vec4 light = viewMatrix * lightPosition;\n"
             "    lightIntensity = dot (light, normal);\n"
             "\n"
             "    gl_Position = projectionMatrix * viewMatrix * position;\n"
             "}\n",

             SHADER_DEMO_HEADER
#if JUCE_OPENGL_ES
             "varying highp float lightIntensity;\n"
#else
             "varying float lightIntensity;\n"
#endif
             "\n"
             "void main()\n"
             "{\n"
#if JUCE_OPENGL_ES
             "   highp float l = lightIntensity * 0.25;\n"
             "   highp vec4 colour = vec4 (l, l, l, 1.0);\n"
#else
             "   float l = lightIntensity * 0.25;\n"
             "   vec4 colour = vec4 (l, l, l, 1.0);\n"
#endif
             "\n"
             "    gl_FragColor = colour;\n"
             "}\n"},

            {"Flattened",

             SHADER_DEMO_HEADER
             "attribute vec4 position;\n"
             "attribute vec4 normal;\n"
             "\n"
             "uniform mat4 projectionMatrix;\n"
             "uniform mat4 viewMatrix;\n"
             "uniform vec4 lightPosition;\n"
             "\n"
             "varying float lightIntensity;\n"
             "\n"
             "void main()\n"
             "{\n"
             "    vec4 light = viewMatrix * lightPosition;\n"
             "    lightIntensity = dot (light, normal);\n"
             "\n"
             "    vec4 v = vec4 (position);\n"
             "    v.z = v.z * 0.1;\n"
             "\n"
             "    gl_Position = projectionMatrix * viewMatrix * v;\n"
             "}\n",

             SHADER_DEMO_HEADER
#if JUCE_OPENGL_ES
             "varying highp float lightIntensity;\n"
#else
             "varying float lightIntensity;\n"
#endif
             "\n"
             "void main()\n"
             "{\n"
#if JUCE_OPENGL_ES
             "   highp float l = lightIntensity * 0.25;\n"
             "   highp vec4 colour = vec4 (l, l, l, 1.0);\n"
#else
             "   float l = lightIntensity * 0.25;\n"
             "   vec4 colour = vec4 (l, l, l, 1.0);\n"
#endif
             "\n"
             "    gl_FragColor = colour;\n"
             "}\n"},

            {"Toon Shader",

             SHADER_DEMO_HEADER
             "attribute vec4 position;\n"
             "attribute vec4 normal;\n"
             "\n"
             "uniform mat4 projectionMatrix;\n"
             "uniform mat4 viewMatrix;\n"
             "uniform vec4 lightPosition;\n"
             "\n"
             "varying float lightIntensity;\n"
             "\n"
             "void main()\n"
             "{\n"
             "    vec4 light = viewMatrix * lightPosition;\n"
             "    lightIntensity = dot (light, normal);\n"
             "\n"
             "    gl_Position = projectionMatrix * viewMatrix * position;\n"
             "}\n",

             SHADER_DEMO_HEADER
#if JUCE_OPENGL_ES
             "varying highp float lightIntensity;\n"
#else
             "varying float lightIntensity;\n"
#endif
             "\n"
             "void main()\n"
             "{\n"
#if JUCE_OPENGL_ES
             "    highp float intensity = lightIntensity * 0.5;\n"
             "    highp vec4 colour;\n"
#else
             "    float intensity = lightIntensity * 0.5;\n"
             "    vec4 colour;\n"
#endif
             "\n"
             "    if (intensity > 0.95)\n"
             "        colour = vec4 (1.0, 0.5, 0.5, 1.0);\n"
             "    else if (intensity > 0.5)\n"
             "        colour  = vec4 (0.6, 0.3, 0.3, 1.0);\n"
             "    else if (intensity > 0.25)\n"
             "        colour  = vec4 (0.4, 0.2, 0.2, 1.0);\n"
             "    else\n"
             "        colour  = vec4 (0.2, 0.1, 0.1, 1.0);\n"
             "\n"
             "    gl_FragColor = colour;\n"
             "}\n"}};

    return Array<ShaderPreset>(presets, numElementsInArray(presets));
};
