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

// Combined translate(position) * scale(size) model matrix -- boxMesh_/
// cylinderMesh_ are unit primitives (-0.5..0.5 extents), so `size` is
// directly the object's full extents, no extra 0.5 factor needed. Column-
// major layout, matching ce::Camera::SetLookAt's constructor convention.
juce::Matrix3D<float> makeModelMatrix(juce::Vector3D<float> position, juce::Vector3D<float> size)
{
    return juce::Matrix3D<float>(size.x, 0.0f, 0.0f, 0.0f,
                                  0.0f, size.y, 0.0f, 0.0f,
                                  0.0f, 0.0f, size.z, 0.0f,
                                  position.x, position.y, position.z, 1.0f);
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
        const auto startPosition = viewMode_ == ViewMode::assemblyFloor
                                      ? juce::Vector3D<float>{ 0.0f, 1.7f, 6.0f }
                                      : juce::Vector3D<float>{ 2.5f, 1.8f, 4.0f };
        freeCamera_ = std::make_unique<ce::FreeCamera>(*this, startPosition);
    }

    openGLContext_.setOpenGLVersionRequired(juce::OpenGLContext::openGL4_1);
    openGLContext_.setRenderer(this);
    openGLContext_.setContinuousRepainting(true);
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

void EngineerViewportComponent::mouseDown(const juce::MouseEvent& event)
{
    if (!event.mods.isLeftButtonDown())
        return;

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

    const auto clickedIndex = hitTestObject(event.position);
    if (clickedIndex >= 0)
        sceneModel_.selectObject(clickedIndex);
}

void EngineerViewportComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDraggingVertex_)
        return;

    const auto area = getLocalBounds().toFloat();
    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
        return;

    std::array<float, 16> inverseViewProjection{};
    {
        const juce::ScopedLock lock(stateLock_);
        inverseViewProjection = lastInverseViewProjection_;
    }

    const auto ndcX = (event.position.x / area.getWidth()) * 2.0f - 1.0f;
    const auto ndcY = 1.0f - (event.position.y / area.getHeight()) * 2.0f;
    const auto nearPoint = unprojectPoint(ndcX, ndcY, -1.0f, inverseViewProjection);
    const auto farPoint = unprojectPoint(ndcX, ndcY, 1.0f, inverseViewProjection);
    const auto rayDirection = (farPoint - nearPoint).normalised();

    const auto denom = rayDirection * vertexDragPlaneNormal_;
    if (std::abs(denom) < 1.0e-6f)
        return;

    const auto t = ((vertexDragPlanePoint_ - nearPoint) * vertexDragPlaneNormal_) / denom;
    sceneModel_.setSelectedVertexPosition(nearPoint + rayDirection * t);
}

void EngineerViewportComponent::mouseUp(const juce::MouseEvent&)
{
    isDraggingVertex_ = false;
}

void EngineerViewportComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    if (freeCamera_ != nullptr)
        freeCamera_->AdjustSpeed(wheel.deltaY);
    else if (orthoCamera_ != nullptr)
        orthoCamera_->AdjustZoom(wheel.deltaY);
}

void EngineerViewportComponent::newOpenGLContextCreated()
{
    shaderComposer_ = std::make_unique<ce::ShaderComposer>(juce::File(ENGINEER_SHADER_SOURCE_DIR));
    litProgram_ = shaderComposer_->GetProgram(openGLContext_, "programs/engineer_lit.vert", "programs/engineer_lit.frag");
    gridProgram_ = shaderComposer_->GetProgram(openGLContext_, "programs/grid.vert", "programs/grid.frag");

    gridRenderer_.Build(10.0f, 1.0f);

    std::vector<ce::Vertex> vertices;
    std::vector<GLuint> indices;

    ce::GenerateCube(vertices, indices);
    boxMesh_.Upload(vertices, indices);

    ce::GenerateCylinder(24, vertices, indices);
    cylinderMesh_.Upload(vertices, indices);

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

    if (freeCamera_ != nullptr)
    {
        freeCamera_->Update(deltaSeconds);
        const auto fovDegrees = viewMode_ == ViewMode::assemblyFloor ? 65.0f : 50.0f;
        camera_.SetPerspective(juce::degreesToRadians(fovDegrees), aspect, 0.05f, 500.0f);
        camera_.SetLookAt(freeCamera_->Position(), freeCamera_->Target());
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

    const auto model = juce::Matrix3D<float>::fromTranslation(object.position);
    litProgram_->use();
    litProgram_->setUniformMat4("uModel", model.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uView", view.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uProjection", projection.mat, 1, GL_FALSE);
    litProgram_->setUniform("uBaseColour", colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha());
    litProgram_->setUniform("uLightDirection", 0.45f, -1.0f, 0.35f);
    cache.mesh.Draw();
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

    const auto model = juce::Matrix3D<float>::fromTranslation(object.position);
    litProgram_->use();
    litProgram_->setUniformMat4("uModel", model.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uView", view.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uProjection", projection.mat, 1, GL_FALSE);
    litProgram_->setUniform("uBaseColour", colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha());
    litProgram_->setUniform("uLightDirection", 0.45f, -1.0f, 0.35f);
    cache.mesh.Draw();
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
        const auto model = makeModelMatrix(object.editableVertices[i], handleSize);

        litProgram_->use();
        litProgram_->setUniformMat4("uModel", model.mat, 1, GL_FALSE);
        litProgram_->setUniformMat4("uView", view.mat, 1, GL_FALSE);
        litProgram_->setUniformMat4("uProjection", projection.mat, 1, GL_FALSE);
        litProgram_->setUniform("uBaseColour", colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), 1.0f);
        litProgram_->setUniform("uLightDirection", 0.45f, -1.0f, 0.35f);
        boxMesh_.Draw();
    }
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

    const auto model = makeModelMatrix(position, size);
    const auto colour = applyLayerTint(objectColour(object, selected, mirrored), sceneModel_.findLayer(object.layerId));

    litProgram_->use();
    litProgram_->setUniformMat4("uModel", model.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uView", view.mat, 1, GL_FALSE);
    litProgram_->setUniformMat4("uProjection", projection.mat, 1, GL_FALSE);
    litProgram_->setUniform("uBaseColour", colour.getFloatRed(), colour.getFloatGreen(), colour.getFloatBlue(), colour.getFloatAlpha());
    litProgram_->setUniform("uLightDirection", 0.45f, -1.0f, 0.35f);

    const auto& mesh = object.primitiveType.equalsIgnoreCase("Cylinder") ? cylinderMesh_ : boxMesh_;
    const_cast<ce::Mesh&>(mesh).Draw();
}

void EngineerViewportComponent::openGLContextClosing()
{
    litProgram_ = nullptr;
    gridProgram_ = nullptr;
    shaderComposer_.reset();
    libraryPartMeshes_.clear();
    connectorMeshes_.clear();
    directGeometryMeshes_.clear();
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

        const auto testObjectAt = [&](juce::Vector3D<float> centre)
        {
            const auto minimum = centre - halfExtent;
            const auto maximum = centre + halfExtent;
            float hitDistance = 0.0f;
            if (intersectRayWithAabb(ray, minimum, maximum, hitDistance) && hitDistance < bestDistance)
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
