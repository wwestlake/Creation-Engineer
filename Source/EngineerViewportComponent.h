#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
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

    enum class GizmoDragMode
    {
        none,
        planar,
        axisX,
        axisY,
        axisZ
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
    class InteractionOverlay;

    struct ProjectedObjectBounds
    {
        int index = -1;
        juce::Rectangle<float> screenBounds;
        juce::Point<float> screenCentre;
    };

    struct GizmoProjection
    {
        juce::Point<float> centre;
        juce::Point<float> axisX;
        juce::Point<float> axisY;
        juce::Point<float> axisZ;
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
    void renderReferenceScene() const;
    void renderObject(const EngineerSceneModel::SceneObject& object,
                      bool selected,
                      bool mirrored) const;
    void updateFlyCamera(float deltaSeconds);
    juce::Vector3D<float> flyCameraForward() const;
    juce::Vector3D<float> flyCameraFlatForward() const;
    void paintOverlay(juce::Graphics& g) const;
    void drawOverlayGizmo(juce::Graphics& g) const;
    int hitTestObject(juce::Point<float> point) const;
    GizmoDragMode hitTestGizmo(juce::Point<float> point) const;
    GizmoProjection buildSelectedGizmoProjection() const;
    std::vector<ProjectedObjectBounds> buildProjectedObjectBounds() const;
    juce::Vector3D<float> getCameraPosition() const noexcept;
    void updateViewMatrices();

    ViewMode viewMode = ViewMode::design3D;
    EngineerSceneModel& sceneModel;
    juce::OpenGLContext openGLContext;
    std::unique_ptr<InteractionOverlay> interactionOverlay;
    ShaderHandles shader;
    MeshBuffer boxMesh;
    MeshBuffer cylinderMesh;
    MeshBuffer gridMesh;
    MeshBuffer axisMesh;

    juce::Point<float> dragAnchor;
    juce::Point<float> mouseDownPoint;
    juce::Point<float> lastLookScreenPos;
    bool isNavigatingView = false;
    bool isDraggingGizmo = false;
    std::atomic<bool> isLooking { false };
    bool lookDragMoved = false;
    bool pendingBackgroundNavigation = false;
    bool popupMenuTriggered = false;
    GizmoDragMode gizmoDragMode = GizmoDragMode::none;
    juce::Vector3D<float> orbitTarget { 0.0f, 0.0f, 0.0f };
    juce::Vector3D<float> flyCameraPosition { 0.0f, 1.6f, 5.0f };
    float orbitDistance = 18.0f;
    std::atomic<float> yawRadians { 0.72f };
    std::atomic<float> pitchRadians { -0.48f };
    bool useOrthographicProjection = false;
    std::atomic<float> flySpeedMultiplier { 1.0f };
    double lastFrameTimeSeconds = 0.0;

    std::array<float, 16> projectionMatrix {};
    std::array<float, 16> viewMatrix {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerViewportComponent)
};
