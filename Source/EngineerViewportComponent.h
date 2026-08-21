#pragma once

#include <JuceHeader.h>
#include <array>
#include <memory>
#include <unordered_map>

#include "creation/engineering/SpecLibrary.h"

#include "EngineerOrthoCameraController.h"
#include "EngineerSceneModel.h"
#include "Render/Scene/Camera.h"
#include "Render/Scene/FreeCamera.h"
#include "Render/Scene/GridRenderer.h"
#include "Render/Scene/Mesh.h"
#include "Render/Shaders/ShaderComposer.h"

// One real 3D/2D OpenGL design surface. Three independently-dockable
// instances of this class (one per ViewMode) make up CreationEngineer's
// workstation spine — see the docking rollout plan's Part C. Each instance
// owns its own OpenGLContext/camera/uploaded geometry and only shares the
// EngineerSceneModel with its siblings, so any of the three can be dragged
// into its own floating window via CreationDock for free.
//
// Replaces the old hand-rolled fake-2D viewport entirely: no gizmo, no
// InteractionOverlay, no setComponentPaintingEnabled(true) — paint() is a
// genuine no-op and the scene is real GL geometry driven by
// EngineerSceneModel's real juce::Vector3D<float> positions/sizes.
class EngineerViewportComponent final : public juce::Component,
                                        private juce::OpenGLRenderer,
                                        private EngineerSceneModel::Listener
{
public:
    // Fixed for the lifetime of the instance -- there is no runtime mode
    // switch; each mode is its own dockable panel instead.
    enum class ViewMode
    {
        design3D,
        assemblyFloor,
        planarElectronics
    };

    EngineerViewportComponent(EngineerSceneModel& sceneModel, ViewMode mode,
                              const creation::engineering::SpecLibrary& specLibrary);
    ~EngineerViewportComponent() override;

    void paint(juce::Graphics&) override {}
    void resized() override {}
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    ViewMode getViewMode() const noexcept { return viewMode_; }

private:
    void engineerSceneModelChanged() override;

    void newOpenGLContextCreated() override;
    void renderOpenGL() override;
    void openGLContextClosing() override;

    void renderObject(const EngineerSceneModel::SceneObject& object, bool selected, bool mirrored,
                      const juce::Matrix3D<float>& view, const juce::Matrix3D<float>& projection) const;
    void renderLibraryPartObject(const EngineerSceneModel::SceneObject& object, bool selected,
                                 const juce::Matrix3D<float>& view, const juce::Matrix3D<float>& projection);
    void renderConnectorObject(const EngineerSceneModel::SceneObject& object, bool selected,
                               const juce::Matrix3D<float>& view, const juce::Matrix3D<float>& projection);
    int hitTestObject(juce::Point<float> point) const;

    ViewMode viewMode_;
    EngineerSceneModel& sceneModel_;
    const creation::engineering::SpecLibrary& specLibrary_;

    juce::OpenGLContext openGLContext_;
    std::unique_ptr<ce::ShaderComposer> shaderComposer_;
    juce::OpenGLShaderProgram* litProgram_ = nullptr;
    juce::OpenGLShaderProgram* gridProgram_ = nullptr;

    ce::Camera camera_;
    std::unique_ptr<ce::FreeCamera> freeCamera_;                 // design3D / assemblyFloor only.
    std::unique_ptr<EngineerOrthoCameraController> orthoCamera_; // planarElectronics only.
    ce::GridRenderer gridRenderer_;
    ce::Mesh boxMesh_;
    ce::Mesh cylinderMesh_;

    // Keyed by SceneObject::objectId (stable across vector reindexing) so a
    // cached mesh survives unrelated add/remove/duplicate elsewhere in the
    // scene. Regenerated lazily only when the cached signature no longer
    // matches the object's current params -- see renderLibraryPartObject.
    struct LibraryPartMeshCache
    {
        ce::Mesh mesh;
        juce::String profileId;
        float lengthMeters = -1.0f;
    };
    std::unordered_map<int, LibraryPartMeshCache> libraryPartMeshes_;

    // Connectors have no live parameter to watch (v1 scope, see the
    // parametric-part-libraries plan's Part H) so this only ever needs to
    // build a connector's mesh once per objectId.
    struct ConnectorMeshCache
    {
        ce::Mesh mesh;
        juce::String connectorId;
    };
    std::unordered_map<int, ConnectorMeshCache> connectorMeshes_;

    double lastFrameTimeSeconds_ = 0.0;

    // Snapshot of the combined inverse(view*projection) matrix (column-
    // major, same layout as juce::Matrix3D::mat -- juce::Matrix3D itself has
    // no general 4x4 inverse, so this is computed by hand, see
    // invertMatrix4 in the .cpp), refreshed once per rendered frame under
    // stateLock_ and read from hitTestObject on the message thread
    // (mouseDown) -- same producer/consumer-across-threads pattern
    // CreationEngine's ViewportComponent uses for its own camera-position
    // snapshot (see that class's stateLock_ doc).
    mutable juce::CriticalSection stateLock_;
    std::array<float, 16> lastInverseViewProjection_ {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerViewportComponent)
};
