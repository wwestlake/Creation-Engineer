#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>
#include <vector>

#include "EngineerSceneModel.h"

class EngineerViewportComponent final : public juce::Component,
                                        private juce::OpenGLRenderer,
                                        private EngineerSceneModel::Listener
{
public:
    enum class ViewMode
    {
        design3D,
        assemblyFloor,
        planarLayer
    };

    explicit EngineerViewportComponent(EngineerSceneModel& sceneModel);
    ~EngineerViewportComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    ViewMode getViewMode() const noexcept;

private:
    struct ProjectedObjectBounds
    {
        int index = -1;
        juce::Rectangle<float> screenBounds;
        juce::Point<float> screenCentre;
    };

    struct MeshBuffer
    {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLsizei vertexCount = 0;
        unsigned int primitiveType = 0;
    };

    struct ShaderHandles
    {
        std::unique_ptr<juce::OpenGLShaderProgram> program;
        std::unique_ptr<juce::OpenGLShaderProgram::Attribute> position;
        std::unique_ptr<juce::OpenGLShaderProgram::Attribute> normal;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> mvp;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> model;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> baseColour;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> lightingMix;
        std::unique_ptr<juce::OpenGLShaderProgram::Uniform> lightDirection;
    };

    void engineerSceneModelChanged() override;

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void showContextMenu(const juce::MouseEvent& event);
    void handleContextMenuResult(int result, int clickedIndex);
    void applyCameraPreset(EngineerSceneModel::CameraPreset preset);
    void initialiseSceneBuffers();
    void releaseSceneBuffers();
    void buildMeshBuffer(MeshBuffer& mesh,
                         const std::vector<float>& vertices,
                         GLenum primitiveType);
    void renderScene();
    void renderGrid() const;
    void renderObject(const EngineerSceneModel::SceneObject& object,
                      bool selected,
                      bool mirrored) const;
    int hitTestObject(juce::Point<float> point) const;
    std::vector<ProjectedObjectBounds> buildProjectedObjectBounds() const;
    void updateViewMatrices();

    ViewMode viewMode = ViewMode::design3D;
    EngineerSceneModel& sceneModel;
    juce::OpenGLContext openGLContext;
    ShaderHandles shader;
    MeshBuffer boxMesh;
    MeshBuffer cylinderMesh;
    MeshBuffer gridMesh;

    juce::Point<float> dragAnchor;
    bool isNavigatingView = false;
    juce::Vector3D<float> orbitTarget { 0.0f, 0.0f, 0.0f };
    float orbitDistance = 18.0f;
    float yawRadians = 0.72f;
    float pitchRadians = -0.48f;
    bool useOrthographicProjection = false;

    std::array<float, 16> projectionMatrix {};
    std::array<float, 16> viewMatrix {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerViewportComponent)
};
