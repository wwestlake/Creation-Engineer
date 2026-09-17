#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>
#include <limits>
#include <memory>
#include <unordered_map>

#include "creation/engineering/SpecLibrary.h"

#include "EngineerOrbitCameraController.h"
#include "EngineerOrthoCameraController.h"
#include "EngineerSceneModel.h"
#include "Render/Scene/Camera.h"
#include "Render/Scene/CursorGizmoRenderer.h"
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
// Replaces the old hand-rolled fake-2D viewport entirely: no interactive
// gizmo/InteractionOverlay disconnected from real state, and the scene is
// real GL geometry driven by EngineerSceneModel's real juce::Vector3D<float>
// positions/sizes -- the bug that broke the old viewport. paint() IS used
// again, but only now that the GL layer underneath is proven solid: it
// draws pure text-label annotation (the view-cube gizmo's face labels) on
// top of an already-correct frame, never to paper over a broken one -- see
// renderViewCube/paint's own comments.
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

    // Two genuinely different navigation paradigms, both real tools per
    // established convention (Blender defaults to Orbit with an explicit
    // Fly/Walk mode; UE4 defaults to Fly but also offers orbit-around-
    // selection) -- design3D/assemblyFloor instances start in Orbit (this
    // is an object-centric engineering tool) but switch freely into Fly for
    // precise free camera positioning. planarElectronics never uses either
    // (fixed top-down ortho).
    enum class CameraMode
    {
        orbit,
        fly
    };

    EngineerViewportComponent(EngineerSceneModel& sceneModel, ViewMode mode,
                              const creation::engineering::SpecLibrary& specLibrary);
    ~EngineerViewportComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;
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
    void renderDirectGeometryObject(const EngineerSceneModel::SceneObject& object, bool selected,
                                    const juce::Matrix3D<float>& view, const juce::Matrix3D<float>& projection);
    void renderCustomPartObject(const EngineerSceneModel::SceneObject& object, bool selected,
                                const juce::Matrix3D<float>& view, const juce::Matrix3D<float>& projection);
    void renderVertexHandles(const juce::Matrix3D<float>& view, const juce::Matrix3D<float>& projection);
    void renderSketchPoints(const juce::Matrix3D<float>& view, const juce::Matrix3D<float>& projection);
    void renderViewCube();
    void renderPlacementCursor();
    // Draws a bright, unmistakable wireframe outline over an already-drawn
    // mesh -- the actual "this is selected" indicator (a subtle colour
    // brighten alone wasn't clear enough). Works for any mesh/model matrix,
    // so every render*Object function can call it the same way.
    void drawSelectionOutline(ce::Mesh& mesh, const juce::Matrix3D<float>& model, const juce::Matrix3D<float>& view,
                              const juce::Matrix3D<float>& projection) const;
    // Unprojects a screen point into a world-space ray (via the last rendered
    // frame's inverse view-projection, same producer/consumer pattern as
    // hitTestObject) and intersects it with the given world plane. Returns
    // false (leaving outWorldPoint untouched) if the ray is parallel to the
    // plane. Shared by vertex dragging, cursor placement, and sketch point/
    // hole placement -- one implementation of "screen point -> point on a
    // world plane" rather than near-identical copies at each call site.
    bool unprojectScreenPointOntoPlane(juce::Point<float> screenPoint, juce::Vector3D<float> planePoint,
                                       juce::Vector3D<float> planeNormal, juce::Vector3D<float>& outWorldPoint) const;
    int hitTestObject(juce::Point<float> point) const;
    int hitTestVertex(juce::Point<float> point) const;
    int hitTestViewCube(juce::Point<float> point) const;
    void snapToViewCubeFace(int faceIndex);
    juce::Rectangle<float> getViewCubeScreenBounds() const;
    void setCameraMode(CameraMode mode);

    ViewMode viewMode_;
    EngineerSceneModel& sceneModel_;
    const creation::engineering::SpecLibrary& specLibrary_;

    juce::OpenGLContext openGLContext_;
    std::unique_ptr<ce::ShaderComposer> shaderComposer_;
    juce::OpenGLShaderProgram* litProgram_ = nullptr;
    juce::OpenGLShaderProgram* gridProgram_ = nullptr;
    // Flat/unlit -- used only by the view-cube gizmo (renderViewCube), never
    // by scene geometry. See gizmo_unlit.frag's doc comment for why the
    // gizmo deliberately isn't lit like everything else.
    juce::OpenGLShaderProgram* unlitProgram_ = nullptr;

    ce::Camera camera_;
    CameraMode cameraMode_ = CameraMode::orbit;
    std::unique_ptr<ce::FreeCamera> freeCamera_;                       // Fly mode, design3D/assemblyFloor only.
    std::unique_ptr<EngineerOrbitCameraController> orbitCamera_;       // Orbit mode, design3D/assemblyFloor only.
    std::unique_ptr<EngineerOrthoCameraController> orthoCamera_;       // planarElectronics only.
    // Both freeCamera_ and orbitCamera_ are constructed together and kept
    // alive for the component's whole lifetime once viewMode_ is a
    // perspective mode; setCameraMode only ever toggles which one reacts to
    // input via SetEnabled. A real child button, not hand-painted text --
    // its own label is what shows the active mode, so there's nothing extra
    // to keep in sync by hand.
    juce::TextButton cameraModeButton_;
    ce::GridRenderer gridRenderer_;
    ce::Mesh boxMesh_;
    ce::Mesh cylinderMesh_;
    // Six independently-drawable quads for the view-cube gizmo (not the
    // shared unit boxMesh_) so a hovered face can be recoloured without
    // touching the other five -- see renderViewCube.
    std::array<ce::Mesh, 6> gizmoFaceMeshes_;
    // Written from mouseMove (message thread), read from renderViewCube
    // (render thread) -- a one-frame-stale hover highlight is imperceptible,
    // so a plain atomic is enough here (no lock needed).
    std::atomic<int> hoveredGizmoFace_ { -1 };

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

    // Custom (sketch-authored) parts regenerate from ce::GenerateExtrudedPolygonWithHoles
    // whenever the referenced CustomPartSpec changes identity -- same lazy,
    // per-objectId cache shape as LibraryPartMeshCache/ConnectorMeshCache above.
    struct CustomPartMeshCache
    {
        ce::Mesh mesh;
        juce::String customPartId;
    };
    std::unordered_map<int, CustomPartMeshCache> customPartMeshes_;

    // directGeometry objects render their own real vertex cage (Part K of
    // the drawing/layers/vertex-editing plan) instead of the shared scaled
    // boxMesh_. cachedVertices is compared element-wise each frame (a fixed
    // 8 entries, negligible cost) to detect "needs re-upload" -- simpler and
    // just as correct as a version counter for this vertex count.
    struct DirectGeometryMeshCache
    {
        ce::Mesh mesh;
        std::vector<juce::Vector3D<float>> cachedVertices;
    };
    std::unordered_map<int, DirectGeometryMeshCache> directGeometryMeshes_;

    // Vertex-drag state (Vertex Edit Mode only): a plane parallel to the
    // camera's view plane, fixed at the dragged vertex's position when the
    // drag started -- every subsequent mouseDrag intersects the new mouse
    // ray against this same fixed plane, giving a stable, unambiguous
    // "move within the screen plane" drag rather than something depth-
    // ambiguous like ray-vs-object-surface.
    bool isDraggingVertex_ = false;
    juce::Vector3D<float> vertexDragPlanePoint_;
    juce::Vector3D<float> vertexDragPlaneNormal_;

    // Placement cursor (see EngineerSceneModel::getCursorPosition) --
    // rebuilt only when the cursor actually moves (compared against
    // lastBuiltCursorPosition_ each frame), not every frame, same lazy-
    // rebuild discipline the mesh caches above already use.
    ce::CursorGizmoRenderer cursorGizmo_;
    juce::Vector3D<float> lastBuiltCursorPosition_ { std::numeric_limits<float>::max(), 0.0f, 0.0f };

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
