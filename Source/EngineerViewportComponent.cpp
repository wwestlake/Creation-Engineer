#include "EngineerViewportComponent.h"

#include <cmath>
#include <limits>

#include "Render/Scene/ProceduralMesh.h"

using namespace juce::gl;

namespace
{
struct Ray
{
    juce::Vector3D<float> origin;
    juce::Vector3D<float> direction;
};

// Combined translate(position) * rotate(rotationDegrees) * scale(size) model
// matrix -- boxMesh_/cylinderMesh_ are unit primitives (-0.5..0.5 extents),
// so `size` is directly the object's full extents, no extra 0.5 factor
// needed. This order (scale first in object space, then rotate about the
// object's own origin, then place in the world) is the standard, correct
// TRS composition -- get it backwards and rotation would happen around the
// world origin instead of the object's own center. Column-major layout,
// matching ce::Camera::SetLookAt's constructor convention.
juce::Matrix3D<float> makeModelMatrix(juce::Vector3D<float> position, juce::Vector3D<float> rotationDegrees,
                                      juce::Vector3D<float> size)
{
    const juce::Vector3D<float> rotationRadians { juce::degreesToRadians(rotationDegrees.x),
                                                   juce::degreesToRadians(rotationDegrees.y),
                                                   juce::degreesToRadians(rotationDegrees.z) };
    const juce::Matrix3D<float> scale(size.x, 0.0f, 0.0f, 0.0f,
                                      0.0f, size.y, 0.0f, 0.0f,
                                      0.0f, 0.0f, size.z, 0.0f,
                                      0.0f, 0.0f, 0.0f, 1.0f);
    return juce::Matrix3D<float>::fromTranslation(position) * juce::Matrix3D<float>::rotation(rotationRadians) * scale;
}

// translate(position) * rotate(rotationDegrees) only, no scale -- for
// LibraryPart/Connector/CustomPart objects, whose generated meshes are
// already real-meter-sized (see renderLibraryPartObject etc.).
juce::Matrix3D<float> makeTranslateRotateMatrix(juce::Vector3D<float> position, juce::Vector3D<float> rotationDegrees)
{
    const juce::Vector3D<float> rotationRadians { juce::degreesToRadians(rotationDegrees.x),
                                                   juce::degreesToRadians(rotationDegrees.y),
                                                   juce::degreesToRadians(rotationDegrees.z) };
    return juce::Matrix3D<float>::fromTranslation(position) * juce::Matrix3D<float>::rotation(rotationRadians);
}

// Component-wise tint/opacity multiply -- applied to every object's base
// colour right before it's uploaded to uBaseColour, regardless of primitive
// kind, so a layer's appearance is consistent across primitives, library
// parts, and connectors alike.
juce::Colour applyLayerTint(juce::Colour colour, const EngineerSceneModel::Layer* layer)
{
    if (layer == nullptr)
        return colour;

    return juce::Colour::fromFloatRGBA(colour.getFloatRed() * layer->tint.getFloatRed(),
                                       colour.getFloatGreen() * layer->tint.getFloatGreen(),
                                       colour.getFloatBlue() * layer->tint.getFloatBlue(),
                                       colour.getFloatAlpha() * layer->opacity);
}

juce::Colour objectColour(const EngineerSceneModel::SceneObject& object, bool selected, bool mirrored)
{
    auto colour = juce::Colour(0xff7ea0c7);
    if (object.primitiveType.equalsIgnoreCase("Cylinder"))
        colour = juce::Colour(0xff8e7c58);
    else if (object.primitiveType.equalsIgnoreCase("Plate"))
        colour = juce::Colour(0xff4da0cc);

    if (object.authoringState == EngineerSceneModel::AuthoringState::directGeometry)
        colour = juce::Colour(0xffd6a250);

    if (mirrored)
        colour = colour.withMultipliedAlpha(0.35f);
    else if (selected)
        colour = colour.brighter(0.55f);

    return colour;
}

// General 4x4 matrix inverse via cofactor expansion (column-major, same
// layout as juce::Matrix3D::mat) -- juce::Matrix3D has no inverse of its
// own (it only ever needed operator* for transform composition until now).
// Standard textbook cofactor/adjugate method; returns false for a singular
// matrix (never expected here -- a camera's view*projection is degenerate
// only if the viewport has zero area, already guarded by the caller).
bool invertMatrix4(const float* m, float* inv)
{
    inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
    inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
    inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
    inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
    inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
    inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
    inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
    inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
    inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
    inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
    inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
    inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
    inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
    inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
    inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
    inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

    float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
    if (std::abs(det) < 1.0e-8f)
        return false;

    det = 1.0f / det;
    for (int i = 0; i < 16; ++i)
        inv[i] *= det;

    return true;
}

// Transforms a world-space ray into an object's local space for hit-testing
// a rotated object against its own axis-aligned half-extent box, rather than
// needing a full oriented-bounding-box intersection routine -- inverts
// translate*rotate only, deliberately NEVER scale: including scale would
// change the local-space ray direction's length, and hitTestObject's
// nearest-pick loop compares hitDistance directly across objects of
// different sizes, so that length has to stay in real world units
// (rotation matrices alone preserve length; scale doesn't). Identity
// rotation reduces this to exactly today's plain translate-only behavior.
Ray rayToObjectLocalSpace(const Ray& worldRay, juce::Vector3D<float> position, juce::Vector3D<float> rotationDegrees)
{
    const juce::Vector3D<float> rotationRadians { juce::degreesToRadians(rotationDegrees.x),
                                                   juce::degreesToRadians(rotationDegrees.y),
                                                   juce::degreesToRadians(rotationDegrees.z) };
    const auto translateRotate = juce::Matrix3D<float>::fromTranslation(position) * juce::Matrix3D<float>::rotation(rotationRadians);
    std::array<float, 16> inverse{};
    if (!invertMatrix4(translateRotate.mat, inverse.data()))
        return worldRay;

    const auto& m = inverse;
    const auto transformPoint = [&](juce::Vector3D<float> p)
    {
        return juce::Vector3D<float>{ m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12],
                                      m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13],
                                      m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14] };
    };
    const auto transformVector = [&](juce::Vector3D<float> v)
    {
        return juce::Vector3D<float>{ m[0] * v.x + m[4] * v.y + m[8] * v.z,
                                      m[1] * v.x + m[5] * v.y + m[9] * v.z,
                                      m[2] * v.x + m[6] * v.y + m[10] * v.z };
    };

    return { transformPoint(worldRay.origin), transformVector(worldRay.direction) };
}

// Unprojects a clip-space point (ndcX/ndcY in [-1,1], ndcZ = -1 for the near
// plane / +1 for the far plane) through the inverse view-projection matrix.
// Working from a full inverse-VP rather than reconstructing camera basis
// vectors by hand makes picking identical for perspective and orthographic
// instances -- the old fake-2D viewport's rayFromScreenPoint had to branch
// on projection type; this doesn't need to.
juce::Vector3D<float> unprojectPoint(float ndcX, float ndcY, float ndcZ, const std::array<float, 16>& m)
{
    const float x = m[0] * ndcX + m[4] * ndcY + m[8] * ndcZ + m[12];
    const float y = m[1] * ndcX + m[5] * ndcY + m[9] * ndcZ + m[13];
    const float z = m[2] * ndcX + m[6] * ndcY + m[10] * ndcZ + m[14];
    const float w = m[3] * ndcX + m[7] * ndcY + m[11] * ndcZ + m[15];
    const float invW = std::abs(w) > 0.0001f ? 1.0f / w : 1.0f;
    return { x * invW, y * invW, z * invW };
}

// Forward projection (world -> screen), the inverse direction of
// unprojectPoint below -- used for vertex-handle hit-testing rather than a
// 3D ray-vs-point distance test, so pick tolerance is a constant screen-
// space radius regardless of distance from the camera.
juce::Point<float> projectToScreen(juce::Vector3D<float> worldPos, const juce::Matrix3D<float>& viewProjection,
                                   juce::Rectangle<float> area)
{
    const auto& m = viewProjection.mat;
    const float x = m[0] * worldPos.x + m[4] * worldPos.y + m[8] * worldPos.z + m[12];
    const float y = m[1] * worldPos.x + m[5] * worldPos.y + m[9] * worldPos.z + m[13];
    const float w = m[3] * worldPos.x + m[7] * worldPos.y + m[11] * worldPos.z + m[15];
    const float invW = std::abs(w) > 0.0001f ? 1.0f / w : 1.0f;
    const float ndcX = x * invW;
    const float ndcY = y * invW;
    return { area.getX() + (ndcX * 0.5f + 0.5f) * area.getWidth(),
            area.getY() + (1.0f - (ndcY * 0.5f + 0.5f)) * area.getHeight() };
}

// Extracts the camera's world-space forward direction straight from the
// view matrix's third column (SetLookAt stores -forward there, see
// Camera.cpp) -- works uniformly for FreeCamera or the ortho controller,
// without needing to ask either one directly.
juce::Vector3D<float> cameraForwardFromView(const juce::Matrix3D<float>& view)
{
    return { -view.mat[8], -view.mat[9], -view.mat[10] };
}

bool intersectRayWithAabb(Ray ray, juce::Vector3D<float> minimum, juce::Vector3D<float> maximum, float& distanceOut)
{
    float tMin = 0.0f;
    float tMax = 1.0e9f;

    const auto testAxis = [&](float origin, float direction, float minAxis, float maxAxis) -> bool
    {
        if (std::abs(direction) < 0.0001f)
            return origin >= minAxis && origin <= maxAxis;

        auto t0 = (minAxis - origin) / direction;
        auto t1 = (maxAxis - origin) / direction;
        if (t0 > t1)
            std::swap(t0, t1);

        tMin = juce::jmax(tMin, t0);
        tMax = juce::jmin(tMax, t1);
        return tMax >= tMin;
    };

    if (!testAxis(ray.origin.x, ray.direction.x, minimum.x, maximum.x))
        return false;
    if (!testAxis(ray.origin.y, ray.direction.y, minimum.y, maximum.y))
        return false;
    if (!testAxis(ray.origin.z, ray.direction.z, minimum.z, maximum.z))
        return false;

    distanceOut = tMin >= 0.0f ? tMin : tMax;
    return distanceOut >= 0.0f;
}

// The view-cube gizmo's six faces, one source of truth used identically by
// rendering, hit-testing, and label placement so index/label/direction can
// never drift out of sync with each other. FRONT is +Z specifically because
// that matches ce::FreeCamera's own default startup orientation (yaw=0,
// pitch=0 looks down -Z, i.e. the camera starts on the +Z side looking in)
// -- clicking FRONT is discoverable as "put me back where I started."
struct GizmoFace
{
    juce::Vector3D<float> normal;
    juce::Vector3D<float> tangentU;
    juce::Vector3D<float> tangentV;
    const char* label;
};

// tangentU x tangentV == normal for every entry (required for correct CCW
// winding in generateGizmoFaceQuad below) -- same right-handed convention
// GenerateCube's own per-face table uses.
const GizmoFace kGizmoFaces[6] = {
    { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, { 0.0f, 1.0f, 0.0f }, "RIGHT" },
    { { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, "LEFT" },
    { { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, -1.0f }, "TOP" },
    { { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, "BOTTOM" },
    { { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, "FRONT" },
    { { 0.0f, 0.0f, -1.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, "BACK" },
};

// Builds one flat quad for a gizmo cube face as its own standalone mesh --
// deliberately not a shared scaled unit primitive like boxMesh_, because the
// gizmo needs each face as an independently colourable draw call so hovering
// one face can highlight just that face (see renderViewCube). Corners wind
// BL,BR,TL,TR (index order 0,1,2,3); {0,1,2, 1,3,2} gives two CCW-outward
// triangles given tangentU x tangentV == normal.
void generateGizmoFaceQuad(const GizmoFace& face, std::vector<ce::Vertex>& outVertices, std::vector<GLuint>& outIndices)
{
    constexpr float halfExtent = 0.72f;
    const auto center = face.normal * halfExtent;
    const auto u = face.tangentU * halfExtent;
    const auto v = face.tangentV * halfExtent;

    const juce::Vector3D<float> corners[4] = { center - u - v, center + u - v, center - u + v, center + u + v };
    const float uvs[4][2] = { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } };

    outVertices.clear();
    for (int i = 0; i < 4; ++i)
    {
        ce::Vertex vertex{};
        vertex.position[0] = corners[i].x;
        vertex.position[1] = corners[i].y;
        vertex.position[2] = corners[i].z;
        vertex.normal[0] = face.normal.x;
        vertex.normal[1] = face.normal.y;
        vertex.normal[2] = face.normal.z;
        vertex.uv[0] = uvs[i][0];
        vertex.uv[1] = uvs[i][1];
        outVertices.push_back(vertex);
    }
    outIndices = { 0, 1, 2, 1, 3, 2 };
}

// Builds the small gizmo's own camera each frame: an orthographic camera
// that shares only the main camera's *rotation* (never its position), so
// the cube always shows "which way is the main viewport currently looking,"
// independent of where the FreeCamera has flown to.
ce::Camera makeGizmoCamera(const juce::Matrix3D<float>& mainViewMatrix)
{
    ce::Camera gizmoCamera;
    gizmoCamera.SetOrthographic(-1.6f, 1.6f, -1.6f, 1.6f, 0.1f, 10.0f);
    const auto forward = cameraForwardFromView(mainViewMatrix);
    gizmoCamera.SetLookAt(forward * -4.0f, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f });
    return gizmoCamera;
}
}

EngineerViewportComponent::EngineerViewportComponent(EngineerSceneModel& sceneModel, ViewMode mode,
                                                      const creation::engineering::SpecLibrary& specLibrary)
    : viewMode_(mode), sceneModel_(sceneModel), specLibrary_(specLibrary)
{
    sceneModel_.addListener(this);
    lastFrameTimeSeconds_ = juce::Time::getMillisecondCounterHiRes() / 1000.0;

    if (viewMode_ == ViewMode::planarElectronics)
    {
        orthoCamera_ = std::make_unique<EngineerOrthoCameraController>(*this);
    }
    else
    {
        // Orbit is the default for both perspective instances -- this is an
        // object-centric engineering tool -- but Fly mode is a real, easily
        // reachable alternative (see the CameraMode doc comment), not a
        // second-class option. Both controllers are constructed together,
        // right here, and kept alive for the component's whole lifetime;
        // setCameraMode (below) only ever flips SetEnabled on each -- never
        // destroys/recreates either one. Doing that from inside a click
        // handler was the actual bug: both classes attach/detach a
        // juce::MouseListener in their constructor/destructor, and mutating
        // that listener list synchronously from inside a listener callback
        // it's currently mid-dispatch of is what left the camera stuck.
        const auto initialTarget = sceneModel_.getSelectedObject().position;
        const auto initialDistance = viewMode_ == ViewMode::assemblyFloor ? 6.0f : 4.0f;
        orbitCamera_ = std::make_unique<EngineerOrbitCameraController>(*this, initialTarget, initialDistance);

        // Seed Fly mode's starting pose from Orbit's -- safe to read
        // orbitCamera_->Position()/Target() here specifically because
        // nothing is racing yet (the GL context/render thread doesn't exist
        // until openGLContext_.attachTo below), so this one-time copy at
        // construction isn't the same kind of cross-thread hazard a runtime
        // handoff between the two controllers would be.
        const auto orbitStartPosition = orbitCamera_->Position();
        freeCamera_ = std::make_unique<ce::FreeCamera>(*this, orbitStartPosition);
        const auto forward = (initialTarget - orbitStartPosition).normalised();
        const auto pitch = std::asin(juce::jlimit(-1.0f, 1.0f, forward.y));
        const auto cosPitch = std::cos(pitch);
        const auto yaw = cosPitch > 0.0001f ? std::atan2(forward.x, -forward.z) : 0.0f;
        freeCamera_->SetOrientation(yaw, pitch);
        freeCamera_->SetEnabled(false);

        addAndMakeVisible(cameraModeButton_);
        cameraModeButton_.setButtonText("Orbit Mode");
        cameraModeButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1c2533));
        cameraModeButton_.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe8edf4));
        cameraModeButton_.onClick = [this]
        {
            setCameraMode(cameraMode_ == CameraMode::orbit ? CameraMode::fly : CameraMode::orbit);
        };
    }

    openGLContext_.setOpenGLVersionRequired(juce::OpenGLContext::openGL4_1);
    openGLContext_.setRenderer(this);
    openGLContext_.setContinuousRepainting(true);
    // Only the two perspective instances get the gizmo/mode badge/control
    // hints (the planarElectronics instance is always a fixed top-down
    // ortho view, so there's no mode or orientation ambiguity to show
    // anything for there) -- component painting is only enabled where
    // paint() actually draws something, to keep the "no 2D compositing
    // unless it's real, deliberate annotation over an already-correct
    // frame" invariant explicit at every call site.
    if (viewMode_ != ViewMode::planarElectronics)
        openGLContext_.setComponentPaintingEnabled(true);
    openGLContext_.attachTo(*this);
}

EngineerViewportComponent::~EngineerViewportComponent()
{
    openGLContext_.detach();
    sceneModel_.removeListener(this);
}

void EngineerViewportComponent::engineerSceneModelChanged()
{
    // Render state lives entirely in GL buffers redrawn every frame under
    // setContinuousRepainting(true); nothing here needs an explicit
    // repaint() the way the old 2D-overlay viewport did.
}

void EngineerViewportComponent::resized()
{
    cameraModeButton_.setBounds(14, 14, 118, 26);
}

void EngineerViewportComponent::mouseDown(const juce::MouseEvent& event)
{
    if (!event.mods.isLeftButtonDown())
        return;

    const auto gizmoFace = hitTestViewCube(event.position);
    if (gizmoFace >= 0)
    {
        snapToViewCubeFace(gizmoFace);
        return;
    }

    if (sceneModel_.getEditMode() == EngineerSceneModel::EditMode::vertex)
    {
        const auto vertexIndex = hitTestVertex(event.position);
        if (vertexIndex < 0)
            return;

        sceneModel_.selectVertex(vertexIndex);
        isDraggingVertex_ = true;
        vertexDragPlanePoint_ = sceneModel_.getSelectedVertexPosition();
        vertexDragPlaneNormal_ = cameraForwardFromView(camera_.ViewMatrix());
        return;
    }

    if (sceneModel_.getEditMode() == EngineerSceneModel::EditMode::sketch)
    {
        const auto& sketch = sceneModel_.getActiveSketch();
        const juce::Vector3D<float> planeNormal = sketch.plane == EngineerSceneModel::SketchPlane::xy
                                                     ? juce::Vector3D<float>{ 0.0f, 0.0f, 1.0f }
                                                     : sketch.plane == EngineerSceneModel::SketchPlane::xz
                                                           ? juce::Vector3D<float>{ 0.0f, 1.0f, 0.0f }
                                                           : juce::Vector3D<float>{ 1.0f, 0.0f, 0.0f };
        juce::Vector3D<float> worldPoint;
        if (!unprojectScreenPointOntoPlane(event.position, sketch.origin, planeNormal, worldPoint))
            return;

        const auto uv = EngineerSceneModel::worldToSketchPlane(sketch.plane, sketch.origin, worldPoint);
        if (sketch.holePlacementActive)
            sceneModel_.addSketchHole(uv, 0.005f);
        else
            sceneModel_.addSketchBoundaryPoint(uv);
        return;
    }

    const auto clickedIndex = hitTestObject(event.position);
    if (clickedIndex >= 0)
    {
        // A click that hits an object both selects it and snaps the
        // placement cursor to it -- one action, immediately useful for
        // "place the next thing relative to this part" (see Part Q's
        // aligned-connector-placement workflow).
        sceneModel_.selectObject(clickedIndex);
        sceneModel_.setCursorPosition(sceneModel_.getObjects()[static_cast<size_t>(clickedIndex)].position);
        return;
    }

    // Empty-space click in Object mode moves the cursor to where the click
    // lands on the ground plane -- see EngineerSceneModel::getCursorPosition.
    juce::Vector3D<float> groundPoint;
    if (unprojectScreenPointOntoPlane(event.position, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, groundPoint))
        sceneModel_.setCursorPosition(groundPoint);
}

void EngineerViewportComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDraggingVertex_)
        return;

    juce::Vector3D<float> worldPoint;
    if (!unprojectScreenPointOntoPlane(event.position, vertexDragPlanePoint_, vertexDragPlaneNormal_, worldPoint))
        return;

    sceneModel_.setSelectedVertexPosition(worldPoint);
}

void EngineerViewportComponent::mouseUp(const juce::MouseEvent&)
{
    isDraggingVertex_ = false;
}

// Tracks which gizmo face (if any) is under the cursor, purely for the
// hover-highlight in renderViewCube -- reuses hitTestViewCube's existing
// visible-face-nearest-pick logic (already called the same way from
// mouseDown for the actual click), just on every move instead of on click.
void EngineerViewportComponent::mouseMove(const juce::MouseEvent& event)
{
    hoveredGizmoFace_.store(hitTestViewCube(event.position), std::memory_order_relaxed);
}

// Double-click an object to make it the orbit target -- the deliberate,
// discoverable way to say "I'm working on THIS now" (matches Blender/Maya's
// frame-selected convention), as opposed to every single click silently
// re-centering the view, which would feel just as disorienting as the
// gizmo bug this was built to fix.
void EngineerViewportComponent::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (!event.mods.isLeftButtonDown() || orbitCamera_ == nullptr)
        return;

    const auto clickedIndex = hitTestObject(event.position);
    if (clickedIndex < 0)
        return;

    sceneModel_.selectObject(clickedIndex);
    orbitCamera_->SetTarget(sceneModel_.getObjects()[static_cast<size_t>(clickedIndex)].position);
}

void EngineerViewportComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    // freeCamera_/orbitCamera_ are both always alive together now (see the
    // constructor), so which one the wheel should affect has to come from
    // cameraMode_, not pointer-nullness.
    if (cameraMode_ == CameraMode::fly && freeCamera_ != nullptr)
        freeCamera_->AdjustSpeed(wheel.deltaY);
    else if (orbitCamera_ != nullptr)
        orbitCamera_->AdjustZoom(wheel.deltaY);
    else if (orthoCamera_ != nullptr)
        orthoCamera_->AdjustZoom(wheel.deltaY);
}

void EngineerViewportComponent::newOpenGLContextCreated()
{
    shaderComposer_ = std::make_unique<ce::ShaderComposer>(juce::File(ENGINEER_SHADER_SOURCE_DIR));
    litProgram_ = shaderComposer_->GetProgram(openGLContext_, "programs/engineer_lit.vert", "programs/engineer_lit.frag");
    gridProgram_ = shaderComposer_->GetProgram(openGLContext_, "programs/grid.vert", "programs/grid.frag");
    unlitProgram_ = shaderComposer_->GetProgram(openGLContext_, "programs/unlit.vert", "programs/gizmo_unlit.frag");

    gridRenderer_.Build(10.0f, 1.0f);

    std::vector<ce::Vertex> vertices;
    std::vector<GLuint> indices;

    ce::GenerateCube(vertices, indices);
    boxMesh_.Upload(vertices, indices);

    ce::GenerateCylinder(24, vertices, indices);
    cylinderMesh_.Upload(vertices, indices);

    for (int i = 0; i < 6; ++i)
    {
        generateGizmoFaceQuad(kGizmoFaces[i], vertices, indices);
        gizmoFaceMeshes_[static_cast<size_t>(i)].Upload(vertices, indices);
    }

    lastFrameTimeSeconds_ = juce::Time::getMillisecondCounterHiRes() / 1000.0;
}

void EngineerViewportComponent::renderOpenGL()
{
    // Must be set every frame, not once at context creation -- JUCE's own
    // OpenGLGraphicsContext resets depth-test state after this callback
    // returns (see ce::ViewportComponent::renderOpenGL's identical comment,
    // the class this render loop is ported from).
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    const auto scale = static_cast<float>(openGLContext_.getRenderingScale());
    glViewport(0, 0, juce::roundToInt(scale * static_cast<float>(getWidth())),
               juce::roundToInt(scale * static_cast<float>(getHeight())));

    glClearColor(0.05f, 0.05f, 0.07f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (shaderComposer_ == nullptr)
        return;

    const double nowSeconds = juce::Time::getMillisecondCounterHiRes() / 1000.0;
    const float deltaSeconds = static_cast<float>(nowSeconds - lastFrameTimeSeconds_);
    lastFrameTimeSeconds_ = nowSeconds;

    const float aspect = getHeight() > 0 ? static_cast<float>(getWidth()) / static_cast<float>(getHeight()) : 1.0f;

    // freeCamera_/orbitCamera_ are both always alive together for the two
    // perspective view modes (see the constructor), so which one actually
    // drives camera_ this frame has to come from cameraMode_ -- pointer-
    // nullness alone can no longer distinguish them.
    if (cameraMode_ == CameraMode::fly && freeCamera_ != nullptr)
    {
        freeCamera_->Update(deltaSeconds);
        const auto fovDegrees = viewMode_ == ViewMode::assemblyFloor ? 65.0f : 50.0f;
        camera_.SetPerspective(juce::degreesToRadians(fovDegrees), aspect, 0.05f, 500.0f);
        camera_.SetLookAt(freeCamera_->Position(), freeCamera_->Target());
    }
    else if (orbitCamera_ != nullptr)
    {
        const auto fovDegrees = viewMode_ == ViewMode::assemblyFloor ? 65.0f : 50.0f;
        camera_.SetPerspective(juce::degreesToRadians(fovDegrees), aspect, 0.05f, 500.0f);
        camera_.SetLookAt(orbitCamera_->Position(), orbitCamera_->Target());
    }
    else if (orthoCamera_ != nullptr)
    {
        const auto pan = orthoCamera_->PanOffset();
        const auto halfHeight = orthoCamera_->ZoomHalfHeight();
        const auto halfWidth = halfHeight * aspect;
        camera_.SetOrthographic(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.05f, 500.0f);
        camera_.SetLookAt({ pan.x, 50.0f, pan.y }, { pan.x, 0.0f, pan.y }, { 0.0f, 0.0f, -1.0f });
    }

    {
        const auto viewProjection = camera_.ProjectionMatrix() * camera_.ViewMatrix();
        std::array<float, 16> inverse{};
        invertMatrix4(viewProjection.mat, inverse.data());
        const juce::ScopedLock lock(stateLock_);
        lastInverseViewProjection_ = inverse;
    }

    if (gridProgram_ != nullptr)
    {
        gridProgram_->use();
        gridProgram_->setUniformMat4("uView", camera_.ViewMatrix().mat, 1, GL_FALSE);
        gridProgram_->setUniformMat4("uProjection", camera_.ProjectionMatrix().mat, 1, GL_FALSE);
        gridProgram_->setUniform("uColor", 0.2f, 0.24f, 0.32f);
        gridRenderer_.Draw();
    }

    if (litProgram_ == nullptr)
        return;

    const auto selectedIndex = sceneModel_.getSelectedObjectIndex();
    const auto& objects = sceneModel_.getObjects();

    const auto dispatchRender = [&](const EngineerSceneModel::SceneObject& object, bool selected)
    {
        if (object.primitiveType == "LibraryPart")
        {
            renderLibraryPartObject(object, selected, camera_.ViewMatrix(), camera_.ProjectionMatrix());
            return;
        }

        if (object.primitiveType == "Connector")
        {
            renderConnectorObject(object, selected, camera_.ViewMatrix(), camera_.ProjectionMatrix());
            return;
        }

        if (object.primitiveType == "CustomPart")
        {
            renderCustomPartObject(object, selected, camera_.ViewMatrix(), camera_.ProjectionMatrix());
            return;
        }

        if (object.authoringState == EngineerSceneModel::AuthoringState::directGeometry && !object.editableVertices.empty())
        {
            renderDirectGeometryObject(object, selected, camera_.ViewMatrix(), camera_.ProjectionMatrix());
            return;
        }

        renderObject(object, selected, false, camera_.ViewMatrix(), camera_.ProjectionMatrix());
        if (object.mirrorXEnabled)
            renderObject(object, selected, true, camera_.ViewMatrix(), camera_.ProjectionMatrix());
    };

    // Two passes so translucent layers (opacity < 1) blend correctly: opaque
    // objects render first with the normal depth-write state already set up
    // above; translucent ones render second with blending enabled and depth
    // writes off (so they don't wrongly occlude anything drawn after them)
    // while still depth-*tested* against the opaque pass. GL_BLEND/
    // glDepthMask must be reset every frame, same discipline as GL_DEPTH_TEST/
    // GL_CULL_FACE above -- JUCE's own 2D compositor resets state after this
    // callback returns.
    for (size_t i = 0; i < objects.size(); ++i)
    {
        const auto& object = objects[i];
        const auto* layer = sceneModel_.findLayer(object.layerId);
        if (layer != nullptr && (!layer->visible || layer->opacity < 0.999f))
            continue;

        dispatchRender(object, static_cast<int>(i) == selectedIndex);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (size_t i = 0; i < objects.size(); ++i)
    {
        const auto& object = objects[i];
        const auto* layer = sceneModel_.findLayer(object.layerId);
        if (layer == nullptr || !layer->visible || layer->opacity >= 0.999f)
            continue;

        dispatchRender(object, static_cast<int>(i) == selectedIndex);
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    renderVertexHandles(camera_.ViewMatrix(), camera_.ProjectionMatrix());
    renderSketchPoints(camera_.ViewMatrix(), camera_.ProjectionMatrix());
    renderPlacementCursor();
    renderViewCube();
}

void EngineerViewportComponent::renderLibraryPartObject(const EngineerSceneModel::SceneObject& object, bool selected,
                                                         const juce::Matrix3D<float>& view,
                                                         const juce::Matrix3D<float>& projection)
{
    if (litProgram_ == nullptr || !object.libraryPart.has_value())
        return;

    const auto& part = *object.libraryPart;
    auto& cache = libraryPartMeshes_[object.objectId];
    if (cache.profileId != part.profileId || cache.lengthMeters != part.lengthMeters)
    {
        const auto* profile = specLibrary_.findProfile(part.profileId);
        if (profile == nullptr)
            return;

        std::vector<ce::Vertex> vertices;
        std::vector<GLuint> indices;
        if (profile->kind == creation::engineering::ProfileKind::dinRailTopHat)
            ce::GenerateDinRailTopHat(profile->outerWidthMm / 1000.0f, profile->outerHeightMm / 1000.0f,
                                      part.lengthMeters, vertices, indices);
        else
            ce::GenerateTSlotExtrusion(profile->outerWidthMm / 1000.0f, profile->slotOpeningWidthMm / 1000.0f,
                                       profile->slotChannelWidthMm / 1000.0f, profile->slotDepthMm / 1000.0f,
                                       part.lengthMeters, vertices, indices);
        cache.mesh.Upload(vertices, indices);
        cache.profileId = part.profileId;
        cache.lengthMeters = part.lengthMeters;
    }

    auto colour = juce::Colour(0xff6fae7c);
    if (selected)
        colour = colour.brighter(0.55f);
    colour = applyLayerTint(colour, sceneModel_.findLayer(object.layerId));

    const auto model = makeTranslateRotateMatrix(object.position, object.rotationDegrees);
    litProgram_->use();
    litProgram_->setUniformMat4("uModel", model.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uView", view.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uProjection", projection.mat, 1, GL_FALSE);
    litProgram_->setUniform("uBaseColour", colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha());
    litProgram_->setUniform("uLightDirection", 0.45f, -1.0f, 0.35f);
    cache.mesh.Draw();

    if (selected)
        drawSelectionOutline(cache.mesh, model, view, projection);
}

void EngineerViewportComponent::renderConnectorObject(const EngineerSceneModel::SceneObject& object, bool selected,
                                                       const juce::Matrix3D<float>& view,
                                                       const juce::Matrix3D<float>& projection)
{
    if (litProgram_ == nullptr || object.connectorId.isEmpty())
        return;

    auto& cache = connectorMeshes_[object.objectId];
    if (cache.connectorId != object.connectorId)
    {
        const auto* connector = specLibrary_.findConnector(object.connectorId);
        if (connector == nullptr)
            return;

        std::vector<ce::Vertex> vertices;
        std::vector<GLuint> indices;
        if (connector->kind == creation::engineering::ConnectorKind::tNut)
            ce::GenerateConnectorTNut(connector->sizeMmX / 1000.0f, connector->sizeMmY / 1000.0f,
                                      connector->sizeMmZ / 1000.0f, vertices, indices);
        else
            ce::GenerateConnectorCornerBracket(connector->sizeMmX / 1000.0f, connector->sizeMmY / 1000.0f,
                                               connector->sizeMmZ / 1000.0f, vertices, indices);

        cache.mesh.Upload(vertices, indices);
        cache.connectorId = object.connectorId;
    }

    auto colour = juce::Colour(0xffb0b6bd);
    if (selected)
        colour = colour.brighter(0.55f);
    colour = applyLayerTint(colour, sceneModel_.findLayer(object.layerId));

    const auto model = makeTranslateRotateMatrix(object.position, object.rotationDegrees);
    litProgram_->use();
    litProgram_->setUniformMat4("uModel", model.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uView", view.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uProjection", projection.mat, 1, GL_FALSE);
    litProgram_->setUniform("uBaseColour", colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha());
    litProgram_->setUniform("uLightDirection", 0.45f, -1.0f, 0.35f);
    cache.mesh.Draw();

    if (selected)
        drawSelectionOutline(cache.mesh, model, view, projection);
}

void EngineerViewportComponent::renderDirectGeometryObject(const EngineerSceneModel::SceneObject& object, bool selected,
                                                            const juce::Matrix3D<float>& view,
                                                            const juce::Matrix3D<float>& projection)
{
    if (litProgram_ == nullptr || object.editableVertices.size() != 8)
        return;

    auto& cache = directGeometryMeshes_[object.objectId];
    bool changed = cache.cachedVertices.size() != object.editableVertices.size();
    if (!changed)
    {
        for (size_t i = 0; i < object.editableVertices.size(); ++i)
        {
            const auto& a = cache.cachedVertices[i];
            const auto& b = object.editableVertices[i];
            if (a.x != b.x || a.y != b.y || a.z != b.z)
            {
                changed = true;
                break;
            }
        }
    }

    if (changed)
    {
        std::vector<ce::Vertex> vertices;
        std::vector<GLuint> indices;
        ce::BuildFlatShadedMeshFromCage(object.editableVertices, vertices, indices);
        cache.mesh.Upload(vertices, indices);
        cache.cachedVertices = object.editableVertices;
    }

    auto colour = juce::Colour(0xffd6a250);
    if (selected)
        colour = colour.brighter(0.55f);
    colour = applyLayerTint(colour, sceneModel_.findLayer(object.layerId));

    // editableVertices already store world-space positions, so the mesh
    // built from them needs no translate/scale -- identity model matrix.
    const juce::Matrix3D<float> model;
    litProgram_->use();
    litProgram_->setUniformMat4("uModel", model.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uView", view.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uProjection", projection.mat, 1, GL_FALSE);
    litProgram_->setUniform("uBaseColour", colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha());
    litProgram_->setUniform("uLightDirection", 0.45f, -1.0f, 0.35f);
    cache.mesh.Draw();

    if (selected)
        drawSelectionOutline(cache.mesh, model, view, projection);
}

void EngineerViewportComponent::renderCustomPartObject(const EngineerSceneModel::SceneObject& object, bool selected,
                                                        const juce::Matrix3D<float>& view,
                                                        const juce::Matrix3D<float>& projection)
{
    if (litProgram_ == nullptr || object.customPartId.isEmpty())
        return;

    auto& cache = customPartMeshes_[object.objectId];
    if (cache.customPartId != object.customPartId)
    {
        const auto* part = specLibrary_.findCustomPart(object.customPartId);
        if (part == nullptr)
            return;

        std::vector<ce::SketchHoleDefinition> holeDefs;
        holeDefs.reserve(part->holes.size());
        for (const auto& hole : part->holes)
            holeDefs.push_back({ hole.centerUV, hole.diameterMeters });

        std::vector<ce::Vertex> vertices;
        std::vector<GLuint> indices;
        ce::GenerateExtrudedPolygonWithHoles(part->boundaryUV, holeDefs, part->thicknessMeters, 24, vertices, indices);
        cache.mesh.Upload(vertices, indices);
        cache.customPartId = object.customPartId;
    }

    auto colour = juce::Colour(0xffb0937c);
    if (selected)
        colour = colour.brighter(0.55f);
    colour = applyLayerTint(colour, sceneModel_.findLayer(object.layerId));

    const auto model = makeTranslateRotateMatrix(object.position, object.rotationDegrees);
    litProgram_->use();
    litProgram_->setUniformMat4("uModel", model.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uView", view.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uProjection", projection.mat, 1, GL_FALSE);
    litProgram_->setUniform("uBaseColour", colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha());
    litProgram_->setUniform("uLightDirection", 0.45f, -1.0f, 0.35f);
    cache.mesh.Draw();

    if (selected)
        drawSelectionOutline(cache.mesh, model, view, projection);
}

void EngineerViewportComponent::renderVertexHandles(const juce::Matrix3D<float>& view, const juce::Matrix3D<float>& projection)
{
    if (litProgram_ == nullptr || sceneModel_.getEditMode() != EngineerSceneModel::EditMode::vertex)
        return;

    const auto& object = sceneModel_.getSelectedObject();
    if (object.editableVertices.empty())
        return;

    const juce::Vector3D<float> handleSize { 0.015f, 0.015f, 0.015f };
    for (size_t i = 0; i < object.editableVertices.size(); ++i)
    {
        const auto colour = static_cast<int>(i) == object.selectedVertexIndex ? juce::Colour(0xffffe08a)
                                                                               : juce::Colour(0xff59d0ff);
        const auto model = makeModelMatrix(object.editableVertices[i], { 0.0f, 0.0f, 0.0f }, handleSize);

        litProgram_->use();
        litProgram_->setUniformMat4("uModel", model.mat, 1, GL_FALSE);
        litProgram_->setUniformMat4("uView", view.mat, 1, GL_FALSE);
        litProgram_->setUniformMat4("uProjection", projection.mat, 1, GL_FALSE);
        litProgram_->setUniform("uBaseColour", colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), 1.0f);
        litProgram_->setUniform("uLightDirection", 0.45f, -1.0f, 0.35f);
        boxMesh_.Draw();
    }
}

// Draws the placement cursor (see EngineerSceneModel::getCursorPosition) as
// a small red/green/blue axis crosshair -- reuses the already-bound
// gridProgram_ (grid.vert bakes absolute world-space positions, same as
// GridRenderer/CursorGizmoRenderer's own vertex layout, so no uModel is
// needed), drawn right after the grid so it always sits under real scene
// geometry depth-wise. Rebuilds the underlying line geometry only when the
// cursor position actually changed since the last frame, not every frame.
void EngineerViewportComponent::renderPlacementCursor()
{
    if (gridProgram_ == nullptr)
        return;

    const auto cursorPosition = sceneModel_.getCursorPosition();
    if (cursorPosition.x != lastBuiltCursorPosition_.x || cursorPosition.y != lastBuiltCursorPosition_.y ||
        cursorPosition.z != lastBuiltCursorPosition_.z)
    {
        cursorGizmo_.Build(cursorPosition, 0.12f);
        lastBuiltCursorPosition_ = cursorPosition;
    }

    gridProgram_->use();
    gridProgram_->setUniformMat4("uView", camera_.ViewMatrix().mat, 1, GL_FALSE);
    gridProgram_->setUniformMat4("uProjection", camera_.ProjectionMatrix().mat, 1, GL_FALSE);

    gridProgram_->setUniform("uColor", 0.95f, 0.25f, 0.25f);
    cursorGizmo_.Draw(0);
    gridProgram_->setUniform("uColor", 0.25f, 0.9f, 0.3f);
    cursorGizmo_.Draw(1);
    gridProgram_->setUniform("uColor", 0.3f, 0.55f, 0.95f);
    cursorGizmo_.Draw(2);
}

// Small markers for an in-progress sketch's placed boundary points/holes --
// without this, Sketch Mode would give zero visual feedback about where a
// click landed until the sketch is finished, which the numeric panel alone
// doesn't fix (you need to see the shape forming to place a good boundary).
// Reuses renderVertexHandles' exact tiny-box/litProgram_ technique; boundary
// points and holes get distinct colours so the two aren't confused with each
// other or with the currently-selected one being edited numerically.
void EngineerViewportComponent::renderSketchPoints(const juce::Matrix3D<float>& view, const juce::Matrix3D<float>& projection)
{
    if (litProgram_ == nullptr || sceneModel_.getEditMode() != EngineerSceneModel::EditMode::sketch)
        return;

    const auto& sketch = sceneModel_.getActiveSketch();
    const juce::Vector3D<float> markerSize { 0.012f, 0.012f, 0.012f };

    const auto drawMarker = [&](juce::Vector3D<float> worldPos, juce::Colour colour)
    {
        const auto model = makeModelMatrix(worldPos, { 0.0f, 0.0f, 0.0f }, markerSize);
        litProgram_->use();
        litProgram_->setUniformMat4("uModel", model.mat, 1, GL_FALSE);
        litProgram_->setUniformMat4("uView", view.mat, 1, GL_FALSE);
        litProgram_->setUniformMat4("uProjection", projection.mat, 1, GL_FALSE);
        litProgram_->setUniform("uBaseColour", colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), 1.0f);
        litProgram_->setUniform("uLightDirection", 0.45f, -1.0f, 0.35f);
        boxMesh_.Draw();
    };

    for (int i = 0; i < static_cast<int>(sketch.boundaryPoints.size()); ++i)
    {
        const auto worldPos = EngineerSceneModel::sketchPlaneToWorld(sketch.plane, sketch.origin, sketch.boundaryPoints[static_cast<size_t>(i)]);
        drawMarker(worldPos, i == sketch.selectedBoundaryPointIndex ? juce::Colour(0xffffe08a) : juce::Colour(0xff59d0ff));
    }

    for (int i = 0; i < static_cast<int>(sketch.holes.size()); ++i)
    {
        const auto worldPos = EngineerSceneModel::sketchPlaneToWorld(sketch.plane, sketch.origin, sketch.holes[static_cast<size_t>(i)].centerUV);
        drawMarker(worldPos, i == sketch.selectedHoleIndex ? juce::Colour(0xffffb04a) : juce::Colour(0xffe8607a));
    }
}

juce::Rectangle<float> EngineerViewportComponent::getViewCubeScreenBounds() const
{
    constexpr float size = 84.0f;
    constexpr float margin = 14.0f;
    return { static_cast<float>(getWidth()) - size - margin, margin, size, size };
}

// Toggles which camera controller reacts to input -- both stay constructed
// and alive for the component's whole lifetime (see the constructor), so
// this is just two SetEnabled calls, never a destroy/recreate. That's the
// actual fix for the mode toggle getting stuck: the old version rebuilt
// ce::FreeCamera/EngineerOrbitCameraController from inside this same click
// handler, which meant detaching/reattaching a juce::MouseListener while
// JUCE was still mid-dispatch of the very mouseDown that triggered it.
void EngineerViewportComponent::setCameraMode(CameraMode mode)
{
    if (mode == cameraMode_)
        return;

    cameraMode_ = mode;
    if (freeCamera_ != nullptr)
        freeCamera_->SetEnabled(mode == CameraMode::fly);
    if (orbitCamera_ != nullptr)
        orbitCamera_->SetEnabled(mode == CameraMode::orbit);

    cameraModeButton_.setButtonText(mode == CameraMode::orbit ? "Orbit Mode" : "Fly Mode");
}

// Renders the view-cube into its own small scissored sub-viewport in the
// corner, after the main scene -- a real, distinct GL "widget," not a 2D
// drawing pretending to be 3D. Uses unlitProgram_, not litProgram_: running
// the gizmo through the scene's lit shader was the actual cause of it
// reading as "a black square" -- see gizmo_unlit.frag's doc comment. Each
// visible face is its own draw call so the hovered one can be highlighted
// independently, followed by a wireframe pass over the same geometry for a
// crisp edge outline so the shape reads as a cube at a glance. Text labels
// for each visible face are added afterward by paint() (see its own comment
// for why that's safe here).
void EngineerViewportComponent::renderViewCube()
{
    if ((freeCamera_ == nullptr && orbitCamera_ == nullptr) || unlitProgram_ == nullptr)
        return;

    const auto scale = static_cast<float>(openGLContext_.getRenderingScale());
    const auto bounds = getViewCubeScreenBounds();
    const auto vpX = juce::roundToInt(bounds.getX() * scale);
    const auto vpY = juce::roundToInt((static_cast<float>(getHeight()) - bounds.getBottom()) * scale);
    const auto vpW = juce::roundToInt(bounds.getWidth() * scale);
    const auto vpH = juce::roundToInt(bounds.getHeight() * scale);
    if (vpW <= 0 || vpH <= 0)
        return;

    glEnable(GL_SCISSOR_TEST);
    glScissor(vpX, vpY, vpW, vpH);
    glViewport(vpX, vpY, vpW, vpH);
    glClearColor(0.086f, 0.094f, 0.114f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const auto gizmoCamera = makeGizmoCamera(camera_.ViewMatrix());
    const auto forward = cameraForwardFromView(camera_.ViewMatrix());
    const auto hoveredFace = hoveredGizmoFace_.load(std::memory_order_relaxed);

    // Light, flat, high-contrast against the gizmo's own dark backing panel
    // -- unlike engineer_lit's ambient floor, this never depends on which
    // way a face happens to be facing the scene's light. Accent colour on
    // hover matches the Autodesk/Blender ViewCube convention of brightening
    // the face under the cursor as click feedback.
    const juce::Colour baseColour(0xffb7bfc9);
    const juce::Colour hoverColour(0xff5fa8e8);
    const juce::Colour edgeColour(0xff20262f);
    const juce::Matrix3D<float> identityModel;

    unlitProgram_->use();
    unlitProgram_->setUniformMat4("uModel", identityModel.mat, 1, GL_FALSE);
    unlitProgram_->setUniformMat4("uView", gizmoCamera.ViewMatrix().mat, 1, GL_FALSE);
    unlitProgram_->setUniformMat4("uProjection", gizmoCamera.ProjectionMatrix().mat, 1, GL_FALSE);

    for (int i = 0; i < 6; ++i)
    {
        if ((kGizmoFaces[i].normal * forward) >= -0.15f)
            continue;

        auto& faceMesh = gizmoFaceMeshes_[static_cast<size_t>(i)];
        const auto fillColour = i == hoveredFace ? hoverColour : baseColour;
        unlitProgram_->setUniform("uBaseColour", fillColour.getFloatRed(), fillColour.getFloatGreen(),
                                  fillColour.getFloatBlue(), 1.0f);
        faceMesh.Draw();

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glLineWidth(1.5f);
        unlitProgram_->setUniform("uBaseColour", edgeColour.getFloatRed(), edgeColour.getFloatGreen(),
                                  edgeColour.getFloatBlue(), 1.0f);
        faceMesh.Draw();
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glLineWidth(1.0f);
    }

    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, juce::roundToInt(scale * static_cast<float>(getWidth())),
               juce::roundToInt(scale * static_cast<float>(getHeight())));
}

void EngineerViewportComponent::drawSelectionOutline(ce::Mesh& mesh, const juce::Matrix3D<float>& model,
                                                      const juce::Matrix3D<float>& view,
                                                      const juce::Matrix3D<float>& projection) const
{
    if (litProgram_ == nullptr)
        return;

    // Bright, saturated orange -- unmistakable against any object's own
    // colour, unlike the old "20% brighter" tint which was easy to miss
    // entirely depending on the object's base colour and lighting.
    glDisable(GL_CULL_FACE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.5f);

    litProgram_->use();
    litProgram_->setUniformMat4("uModel", model.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uView", view.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uProjection", projection.mat, 1, GL_FALSE);
    litProgram_->setUniform("uBaseColour", 1.0f, 0.62f, 0.14f, 1.0f);
    litProgram_->setUniform("uLightDirection", 0.45f, -1.0f, 0.35f);
    mesh.Draw();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
    glEnable(GL_CULL_FACE);
}

void EngineerViewportComponent::renderObject(const EngineerSceneModel::SceneObject& object,
                                             bool selected,
                                             bool mirrored,
                                             const juce::Matrix3D<float>& view,
                                             const juce::Matrix3D<float>& projection) const
{
    auto position = object.position;
    if (mirrored)
        position.x = -position.x;

    auto size = object.size;
    if (object.authoringState == EngineerSceneModel::AuthoringState::directGeometry)
        size.y += object.geometryDepth;

    // Mirroring negates position.x but doesn't mirror rotationDegrees itself
    // -- a mirrored+rotated object's rotation applies unmirrored, an
    // accepted v1 limitation (see the rotation plan's Risks).
    const auto model = makeModelMatrix(position, object.rotationDegrees, size);
    const auto colour = applyLayerTint(objectColour(object, selected, mirrored), sceneModel_.findLayer(object.layerId));

    litProgram_->use();
    litProgram_->setUniformMat4("uModel", model.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uView", view.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uProjection", projection.mat, 1, GL_FALSE);
    litProgram_->setUniform("uBaseColour", colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha());
    litProgram_->setUniform("uLightDirection", 0.45f, -1.0f, 0.35f);

    const auto& mesh = object.primitiveType.equalsIgnoreCase("Cylinder") ? cylinderMesh_ : boxMesh_;
    const_cast<ce::Mesh&>(mesh).Draw();

    if (selected && !mirrored)
        drawSelectionOutline(const_cast<ce::Mesh&>(mesh), model, view, projection);
}

void EngineerViewportComponent::openGLContextClosing()
{
    litProgram_ = nullptr;
    gridProgram_ = nullptr;
    unlitProgram_ = nullptr;
    shaderComposer_.reset();
    libraryPartMeshes_.clear();
    connectorMeshes_.clear();
    directGeometryMeshes_.clear();
    customPartMeshes_.clear();
}

bool EngineerViewportComponent::unprojectScreenPointOntoPlane(juce::Point<float> screenPoint,
                                                               juce::Vector3D<float> planePoint,
                                                               juce::Vector3D<float> planeNormal,
                                                               juce::Vector3D<float>& outWorldPoint) const
{
    const auto area = getLocalBounds().toFloat();
    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
        return false;

    std::array<float, 16> inverseViewProjection{};
    {
        const juce::ScopedLock lock(stateLock_);
        inverseViewProjection = lastInverseViewProjection_;
    }

    const auto ndcX = (screenPoint.x / area.getWidth()) * 2.0f - 1.0f;
    const auto ndcY = 1.0f - (screenPoint.y / area.getHeight()) * 2.0f;
    const auto nearPoint = unprojectPoint(ndcX, ndcY, -1.0f, inverseViewProjection);
    const auto farPoint = unprojectPoint(ndcX, ndcY, 1.0f, inverseViewProjection);
    const auto rayDirection = (farPoint - nearPoint).normalised();

    const auto denom = rayDirection * planeNormal;
    if (std::abs(denom) < 1.0e-6f)
        return false;

    const auto t = ((planePoint - nearPoint) * planeNormal) / denom;
    outWorldPoint = nearPoint + rayDirection * t;
    return true;
}

int EngineerViewportComponent::hitTestObject(juce::Point<float> point) const
{
    const auto area = getLocalBounds().toFloat();
    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
        return -1;

    std::array<float, 16> inverseViewProjection{};
    {
        const juce::ScopedLock lock(stateLock_);
        inverseViewProjection = lastInverseViewProjection_;
    }

    const auto ndcX = (point.x / area.getWidth()) * 2.0f - 1.0f;
    const auto ndcY = 1.0f - (point.y / area.getHeight()) * 2.0f;

    const auto nearPoint = unprojectPoint(ndcX, ndcY, -1.0f, inverseViewProjection);
    const auto farPoint = unprojectPoint(ndcX, ndcY, 1.0f, inverseViewProjection);
    const Ray ray{ nearPoint, (farPoint - nearPoint).normalised() };

    const auto& objects = sceneModel_.getObjects();
    int bestIndex = -1;
    float bestDistance = std::numeric_limits<float>::max();

    for (size_t i = 0; i < objects.size(); ++i)
    {
        const auto& object = objects[i];
        auto size = object.size;
        if (object.authoringState == EngineerSceneModel::AuthoringState::directGeometry)
            size.y += object.geometryDepth;
        const auto halfExtent = size * 0.5f;

        // Transform the ray into the object's own local space (translate*
        // rotate inverse only, see rayToObjectLocalSpace) and test against a
        // local-origin-centered box -- correct for both an unrotated object
        // (reduces to exactly the old world-space test) and a rotated one,
        // without needing a full oriented-bounding-box routine.
        const auto testObjectAt = [&](juce::Vector3D<float> centre)
        {
            const auto localRay = rayToObjectLocalSpace(ray, centre, object.rotationDegrees);
            float hitDistance = 0.0f;
            if (intersectRayWithAabb(localRay, -halfExtent, halfExtent, hitDistance) && hitDistance < bestDistance)
            {
                bestDistance = hitDistance;
                bestIndex = static_cast<int>(i);
            }
        };

        testObjectAt(object.position);
        if (object.mirrorXEnabled)
            testObjectAt({ -object.position.x, object.position.y, object.position.z });
    }

    return bestIndex;
}

int EngineerViewportComponent::hitTestVertex(juce::Point<float> point) const
{
    const auto& object = sceneModel_.getSelectedObject();
    if (object.editableVertices.empty())
        return -1;

    const auto area = getLocalBounds().toFloat();
    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
        return -1;

    const auto viewProjection = camera_.ProjectionMatrix() * camera_.ViewMatrix();

    int bestIndex = -1;
    float bestDistanceSq = 144.0f; // 12px pick radius, squared.
    for (size_t i = 0; i < object.editableVertices.size(); ++i)
    {
        const auto screenPos = projectToScreen(object.editableVertices[i], viewProjection, area);
        const auto distanceSq = point.getDistanceSquaredFrom(screenPos);
        if (distanceSq < bestDistanceSq)
        {
            bestDistanceSq = distanceSq;
            bestIndex = static_cast<int>(i);
        }
    }

    return bestIndex;
}

// Returns an index into kGizmoFaces, or -1 if the point isn't within the
// gizmo's screen bounds or doesn't land near a currently-visible face.
// Picking works the same way hitTestVertex does -- project each candidate
// point to screen space and take the nearest within a fixed radius -- just
// restricted up front to faces actually facing the viewer (dot(normal,
// forward) sufficiently negative), since a click should only ever resolve
// to a face the user can actually see.
int EngineerViewportComponent::hitTestViewCube(juce::Point<float> point) const
{
    if (freeCamera_ == nullptr && orbitCamera_ == nullptr)
        return -1;

    const auto bounds = getViewCubeScreenBounds();
    if (!bounds.contains(point))
        return -1;

    const auto forward = cameraForwardFromView(camera_.ViewMatrix());
    const auto gizmoCamera = makeGizmoCamera(camera_.ViewMatrix());
    const auto viewProjection = gizmoCamera.ProjectionMatrix() * gizmoCamera.ViewMatrix();

    int bestFace = -1;
    float bestDistanceSq = std::numeric_limits<float>::max();
    for (int i = 0; i < 6; ++i)
    {
        if ((kGizmoFaces[i].normal * forward) >= -0.15f)
            continue;

        const auto screenPos = projectToScreen(kGizmoFaces[i].normal * 0.75f, viewProjection, bounds);
        const auto distanceSq = point.getDistanceSquaredFrom(screenPos);
        if (distanceSq < bestDistanceSq)
        {
            bestDistanceSq = distanceSq;
            bestFace = i;
        }
    }

    return bestFace;
}

// Clicking a face means "show me this face," i.e. look along its inward
// direction (-normal). In Orbit mode this reframes around the *current
// target* (EngineerOrbitCameraController::SnapToForward keeps target and
// distance fixed, only changing angle -- the target can never leave view,
// by construction). In Fly mode there is no target concept, so it's a pure
// re-orientation in place (ce::FreeCamera::SetOrientation), which is the
// correct behavior for a free-fly camera -- the yaw/pitch solve below comes
// directly from FreeCamera::Forward()'s own formula (sin(yaw)*cos(pitch),
// sin(pitch), -cos(yaw)*cos(pitch)), so it works for all six faces without
// a per-face lookup table.
void EngineerViewportComponent::snapToViewCubeFace(int faceIndex)
{
    if (faceIndex < 0 || faceIndex >= 6)
        return;

    const auto desiredForward = kGizmoFaces[faceIndex].normal * -1.0f;

    // Both controllers are always alive together now -- apply the snap to
    // whichever one is actually driving the view (cameraMode_), not
    // whichever pointer happens to be non-null first.
    if (cameraMode_ == CameraMode::fly && freeCamera_ != nullptr)
    {
        const auto pitch = std::asin(juce::jlimit(-1.0f, 1.0f, desiredForward.y));
        const auto cosPitch = std::cos(pitch);
        const auto yaw = cosPitch > 0.0001f ? std::atan2(desiredForward.x, -desiredForward.z) : 0.0f;
        freeCamera_->SetOrientation(yaw, pitch);
        return;
    }

    if (orbitCamera_ != nullptr)
        orbitCamera_->SnapToForward(desiredForward);
}

// Pure text-label annotation composited on top of the already-rendered GL
// frame -- safe now in a way the old viewport's gizmo wasn't, because this
// only ever draws *labels for a real, already-correctly-rendered cube*, it
// never substitutes for or hides broken 3D content. JUCE calls paint()
// synchronously right after renderOpenGL() finishes for this same frame
// (that's what setComponentPaintingEnabled(true) requests), so reading
// camera_ here -- normally render-thread-only state -- is safe: by the
// time this runs, the render thread is done touching it for this frame.
void EngineerViewportComponent::paint(juce::Graphics& g)
{
    if (freeCamera_ == nullptr && orbitCamera_ == nullptr)
        return;

    // Mode badge is a real cameraModeButton_ child component now (see the
    // constructor/resized) -- it paints and updates its own label, nothing
    // to hand-paint here. No on-screen control hints either: navigation is
    // explained in chat, not smeared onto the viewport.

    // View-cube gizmo labels.
    const auto bounds = getViewCubeScreenBounds();
    g.setColour(juce::Colour(0xff2a3244));
    g.drawRoundedRectangle(bounds.expanded(2.0f), 6.0f, 1.0f);

    const auto forward = cameraForwardFromView(camera_.ViewMatrix());
    const auto gizmoCamera = makeGizmoCamera(camera_.ViewMatrix());
    const auto viewProjection = gizmoCamera.ProjectionMatrix() * gizmoCamera.ViewMatrix();

    g.setColour(juce::Colour(0xffcfd8e3));
    g.setFont(juce::Font(juce::FontOptions(11.5f)).boldened());

    for (const auto& face : kGizmoFaces)
    {
        if ((face.normal * forward) >= -0.15f)
            continue;

        const auto screenPos = projectToScreen(face.normal * 0.75f, viewProjection, bounds);
        g.drawText(face.label, juce::Rectangle<float>(screenPos.x - 30.0f, screenPos.y - 8.0f, 60.0f, 16.0f),
                  juce::Justification::centred, false);
    }
}
