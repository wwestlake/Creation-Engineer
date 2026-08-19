#include "EngineerViewportComponent.h"

#include <array>
#include <cmath>

using namespace juce::gl;

namespace
{
constexpr float kPi = 3.1415926535f;

struct Vec3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Vec4
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;
};

using Mat4 = std::array<float, 16>;

Vec3 operator+(Vec3 a, Vec3 b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vec3 operator-(Vec3 a, Vec3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
Vec3 operator*(Vec3 a, float s) { return { a.x * s, a.y * s, a.z * s }; }

float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 cross(Vec3 a, Vec3 b)
{
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

Vec3 normalise(Vec3 v)
{
    const auto length = std::sqrt(dot(v, v));
    if (length <= 0.0001f)
        return { 0.0f, 0.0f, 0.0f };

    return { v.x / length, v.y / length, v.z / length };
}

Mat4 identityMatrix()
{
    return {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
}

Mat4 multiply(Mat4 a, Mat4 b)
{
    Mat4 result {};
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            result[static_cast<size_t>(column * 4 + row)] =
                a[static_cast<size_t>(0 * 4 + row)] * b[static_cast<size_t>(column * 4 + 0)] +
                a[static_cast<size_t>(1 * 4 + row)] * b[static_cast<size_t>(column * 4 + 1)] +
                a[static_cast<size_t>(2 * 4 + row)] * b[static_cast<size_t>(column * 4 + 2)] +
                a[static_cast<size_t>(3 * 4 + row)] * b[static_cast<size_t>(column * 4 + 3)];

    return result;
}

Vec4 multiply(Mat4 m, Vec4 v)
{
    return {
        m[0] * v.x + m[4] * v.y + m[8] * v.z + m[12] * v.w,
        m[1] * v.x + m[5] * v.y + m[9] * v.z + m[13] * v.w,
        m[2] * v.x + m[6] * v.y + m[10] * v.z + m[14] * v.w,
        m[3] * v.x + m[7] * v.y + m[11] * v.z + m[15] * v.w
    };
}

Mat4 translationMatrix(Vec3 t)
{
    auto m = identityMatrix();
    m[12] = t.x;
    m[13] = t.y;
    m[14] = t.z;
    return m;
}

Mat4 scaleMatrix(Vec3 s)
{
    return {
        s.x, 0, 0, 0,
        0, s.y, 0, 0,
        0, 0, s.z, 0,
        0, 0, 0, 1
    };
}

Mat4 perspectiveMatrix(float fovRadians, float aspect, float nearPlane, float farPlane)
{
    const auto f = 1.0f / std::tan(fovRadians * 0.5f);
    Mat4 m {};
    m[0] = f / aspect;
    m[5] = f;
    m[10] = (farPlane + nearPlane) / (nearPlane - farPlane);
    m[11] = -1.0f;
    m[14] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane);
    return m;
}

Mat4 orthographicMatrix(float left, float right, float bottom, float top, float nearPlane, float farPlane)
{
    Mat4 m = identityMatrix();
    m[0] = 2.0f / (right - left);
    m[5] = 2.0f / (top - bottom);
    m[10] = -2.0f / (farPlane - nearPlane);
    m[12] = -(right + left) / (right - left);
    m[13] = -(top + bottom) / (top - bottom);
    m[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    return m;
}

Mat4 lookAtMatrix(Vec3 eye, Vec3 target, Vec3 up)
{
    const auto forward = normalise(target - eye);
    const auto side = normalise(cross(forward, up));
    const auto correctedUp = cross(side, forward);

    Mat4 m = identityMatrix();
    m[0] = side.x;
    m[1] = correctedUp.x;
    m[2] = -forward.x;
    m[4] = side.y;
    m[5] = correctedUp.y;
    m[6] = -forward.y;
    m[8] = side.z;
    m[9] = correctedUp.z;
    m[10] = -forward.z;
    m[12] = -dot(side, eye);
    m[13] = -dot(correctedUp, eye);
    m[14] = dot(forward, eye);
    return m;
}

Vec3 objectCentreFor(const EngineerSceneModel::SceneObject& object, bool mirrored)
{
    auto x = (object.normalizedPosition.x - 0.5f) * 22.0f;
    const auto z = (object.normalizedPosition.y - 0.5f) * 16.0f;
    if (mirrored)
        x = -x;

    float height = 1.0f;
    if (object.primitiveType.equalsIgnoreCase("Plate"))
        height = 0.30f + object.geometryDepth * 8.0f;
    else if (object.primitiveType.equalsIgnoreCase("Cylinder"))
        height = 1.2f + object.normalizedSize.y * 4.5f;
    else
        height = 1.4f + object.geometryDepth * 5.0f;

    return { x, height * 0.5f, z };
}

Vec3 objectScaleFor(const EngineerSceneModel::SceneObject& object)
{
    if (object.primitiveType.equalsIgnoreCase("Cylinder"))
    {
        const auto radius = 0.55f + object.normalizedSize.x * 4.8f;
        const auto height = 1.2f + object.normalizedSize.y * 4.5f;
        return { radius, height * 0.5f, radius };
    }

    if (object.primitiveType.equalsIgnoreCase("Plate"))
        return { 1.1f + object.normalizedSize.x * 8.5f, 0.20f + object.geometryDepth * 4.0f, 0.8f + object.normalizedSize.y * 5.0f };

    return { 1.0f + object.normalizedSize.x * 8.0f, 0.9f + object.geometryDepth * 3.5f, 0.8f + object.normalizedSize.y * 5.5f };
}

Mat4 objectModelMatrix(const EngineerSceneModel::SceneObject& object, bool mirrored)
{
    return multiply(translationMatrix(objectCentreFor(object, mirrored)), scaleMatrix(objectScaleFor(object)));
}

std::vector<float> makeBoxVertices()
{
    std::vector<float> vertices;

    auto pushVertex = [&vertices](Vec3 p, Vec3 n)
    {
        vertices.push_back(p.x);
        vertices.push_back(p.y);
        vertices.push_back(p.z);
        vertices.push_back(n.x);
        vertices.push_back(n.y);
        vertices.push_back(n.z);
    };

    auto pushFace = [&pushVertex](Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 n)
    {
        pushVertex(a, n);
        pushVertex(b, n);
        pushVertex(c, n);
        pushVertex(a, n);
        pushVertex(c, n);
        pushVertex(d, n);
    };

    pushFace({ -1, -1,  1 }, {  1, -1,  1 }, {  1,  1,  1 }, { -1,  1,  1 }, {  0,  0,  1 });
    pushFace({ -1, -1, -1 }, { -1,  1, -1 }, {  1,  1, -1 }, {  1, -1, -1 }, {  0,  0, -1 });
    pushFace({ -1, -1, -1 }, { -1, -1,  1 }, { -1,  1,  1 }, { -1,  1, -1 }, { -1,  0,  0 });
    pushFace({  1, -1, -1 }, {  1,  1, -1 }, {  1,  1,  1 }, {  1, -1,  1 }, {  1,  0,  0 });
    pushFace({ -1,  1, -1 }, { -1,  1,  1 }, {  1,  1,  1 }, {  1,  1, -1 }, {  0,  1,  0 });
    pushFace({ -1, -1, -1 }, {  1, -1, -1 }, {  1, -1,  1 }, { -1, -1,  1 }, {  0, -1,  0 });

    return vertices;
}

std::vector<float> makeCylinderVertices()
{
    std::vector<float> vertices;
    constexpr int segments = 24;
    for (int i = 0; i < segments; ++i)
    {
        const auto a0 = (static_cast<float>(i) / static_cast<float>(segments)) * 2.0f * kPi;
        const auto a1 = (static_cast<float>(i + 1) / static_cast<float>(segments)) * 2.0f * kPi;
        const Vec3 p0 { std::cos(a0), -1.0f, std::sin(a0) };
        const Vec3 p1 { std::cos(a1), -1.0f, std::sin(a1) };
        const Vec3 p2 { std::cos(a1),  1.0f, std::sin(a1) };
        const Vec3 p3 { std::cos(a0),  1.0f, std::sin(a0) };
        const Vec3 n0 { std::cos(a0), 0.0f, std::sin(a0) };
        const Vec3 n1 { std::cos(a1), 0.0f, std::sin(a1) };

        const std::array<float, 72> side {
            p0.x, p0.y, p0.z, n0.x, n0.y, n0.z,  p1.x, p1.y, p1.z, n1.x, n1.y, n1.z,  p2.x, p2.y, p2.z, n1.x, n1.y, n1.z,
            p0.x, p0.y, p0.z, n0.x, n0.y, n0.z,  p2.x, p2.y, p2.z, n1.x, n1.y, n1.z,  p3.x, p3.y, p3.z, n0.x, n0.y, n0.z
        };
        vertices.insert(vertices.end(), side.begin(), side.end());

        const std::array<float, 36> top {
            0.0f,  1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  p3.x, p3.y, p3.z, 0.0f, 1.0f, 0.0f,  p2.x, p2.y, p2.z, 0.0f, 1.0f, 0.0f
        };
        vertices.insert(vertices.end(), top.begin(), top.end());

        const std::array<float, 36> bottom {
            0.0f, -1.0f, 0.0f, 0.0f, -1.0f, 0.0f,  p1.x, p1.y, p1.z, 0.0f, -1.0f, 0.0f,  p0.x, p0.y, p0.z, 0.0f, -1.0f, 0.0f
        };
        vertices.insert(vertices.end(), bottom.begin(), bottom.end());
    }

    return vertices;
}

std::vector<float> makeGridVertices()
{
    std::vector<float> vertices;
    for (int line = -20; line <= 20; ++line)
    {
        const auto axis = line == 0 ? 1.0f : 0.0f;
        const auto d = static_cast<float>(line);
        const std::array<float, 24> a {
            -20.0f, 0.0f, d, 0.0f, 1.0f, axis,  20.0f, 0.0f, d, 0.0f, 1.0f, axis,
             d, 0.0f, -20.0f, axis, 1.0f, 0.0f, d, 0.0f, 20.0f, axis, 1.0f, 0.0f
        };
        vertices.insert(vertices.end(), a.begin(), a.end());
    }

    return vertices;
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
        colour = colour.withMultipliedAlpha(0.28f);
    else if (selected)
        colour = colour.brighter(0.55f);

    return colour;
}

juce::Rectangle<float> viewportRectFor(const juce::Component& component)
{
    return component.getLocalBounds().toFloat();
}

juce::Point<float> toScreenPoint(Vec4 clip, juce::Rectangle<float> area)
{
    const auto invW = 1.0f / clip.w;
    const auto ndcX = clip.x * invW;
    const auto ndcY = clip.y * invW;
    return {
        area.getX() + (ndcX * 0.5f + 0.5f) * area.getWidth(),
        area.getY() + (1.0f - (ndcY * 0.5f + 0.5f)) * area.getHeight()
    };
}

juce::String viewModeLabel(EngineerViewportComponent::ViewMode mode)
{
    switch (mode)
    {
        case EngineerViewportComponent::ViewMode::design3D: return "3D Design";
        case EngineerViewportComponent::ViewMode::assemblyFloor: return "Assembly Floor";
        case EngineerViewportComponent::ViewMode::planarLayer: return "Planar Layer";
    }

    return "3D Design";
}
}

EngineerViewportComponent::EngineerViewportComponent(EngineerSceneModel& model)
    : sceneModel(model)
{
    sceneModel.addListener(this);
    applyCameraPreset(sceneModel.getCameraPreset());

    openGLContext.setOpenGLVersionRequired(juce::OpenGLContext::openGL4_1);
    openGLContext.setRenderer(this);
    openGLContext.setContinuousRepainting(true);
    openGLContext.attachTo(*this);
}

EngineerViewportComponent::~EngineerViewportComponent()
{
    openGLContext.detach();
    sceneModel.removeListener(this);
}

void EngineerViewportComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(16.0f)).boldened());
    g.drawText(viewModeLabel(viewMode), 12, 10, 200, 22, juce::Justification::left, true);

    g.setColour(juce::Colour(0xffc2cfde));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawText("Orbit: middle drag   Pan: shift+middle drag   Zoom: wheel   Menu: right-click",
               12, 34, getWidth() - 24, 18, juce::Justification::left, true);

    const auto& selected = sceneModel.getSelectedObject();
    g.drawText("Selected: " + selected.name
               + "   State: " + EngineerSceneModel::toDisplayString(selected.authoringState)
               + "   Tool: " + EngineerSceneModel::toDisplayString(sceneModel.getSelectedGeometryTool()),
               12, 54, getWidth() - 24, 18, juce::Justification::left, true);
}

void EngineerViewportComponent::resized()
{
    updateViewMatrices();
}

void EngineerViewportComponent::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isPopupMenu())
    {
        showContextMenu(event);
        return;
    }

    if (event.mods.isMiddleButtonDown() || (event.mods.isLeftButtonDown() && event.mods.isAltDown()))
    {
        dragAnchor = event.position;
        isNavigatingView = true;
        return;
    }

    if (event.mods.isLeftButtonDown())
    {
        const auto clickedIndex = hitTestObject(event.position);
        if (clickedIndex >= 0)
            sceneModel.selectObject(clickedIndex);
    }
}

void EngineerViewportComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (!isNavigatingView)
        return;

    const auto delta = event.position - dragAnchor;
    dragAnchor = event.position;

    if (event.mods.isShiftDown())
    {
        const auto right = Vec3 { std::cos(yawRadians), 0.0f, -std::sin(yawRadians) };
        const auto forward = Vec3 { std::sin(yawRadians), 0.0f, std::cos(yawRadians) };
        orbitTarget.x -= right.x * delta.x * 0.03f;
        orbitTarget.z -= right.z * delta.x * 0.03f;
        orbitTarget.x += forward.x * delta.y * 0.03f;
        orbitTarget.z += forward.z * delta.y * 0.03f;
    }
    else
    {
        yawRadians += delta.x * 0.010f;
        pitchRadians = juce::jlimit(-1.42f, -0.08f, pitchRadians - delta.y * 0.008f);
    }

    updateViewMatrices();
}

void EngineerViewportComponent::mouseUp(const juce::MouseEvent&)
{
    isNavigatingView = false;
}

void EngineerViewportComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    orbitDistance = juce::jlimit(4.0f, 70.0f, orbitDistance - wheel.deltaY * 3.5f);
    updateViewMatrices();
}

EngineerViewportComponent::ViewMode EngineerViewportComponent::getViewMode() const noexcept
{
    return viewMode;
}

void EngineerViewportComponent::engineerSceneModelChanged()
{
    if (!isNavigatingView)
        applyCameraPreset(sceneModel.getCameraPreset());

    repaint();
}

void EngineerViewportComponent::newOpenGLContextCreated()
{
    const char* vertexShader = R"(
        #version 150 core
        in vec3 position;
        in vec3 normal;
        uniform mat4 uMvp;
        uniform mat4 uModel;
        out vec3 vNormal;
        out vec3 vWorldPosition;
        void main()
        {
            vec4 world = uModel * vec4(position, 1.0);
            vWorldPosition = world.xyz;
            vNormal = mat3(uModel) * normal;
            gl_Position = uMvp * vec4(position, 1.0);
        }
    )";

    const char* fragmentShader = R"(
        #version 150 core
        uniform vec4 uBaseColour;
        uniform float uLightingMix;
        uniform vec3 uLightDirection;
        in vec3 vNormal;
        out vec4 fragColour;
        void main()
        {
            vec3 n = normalize(vNormal);
            float diffuse = max(dot(n, normalize(-uLightDirection)), 0.0);
            vec3 shaded = uBaseColour.rgb * (0.20 + diffuse * 0.80);
            vec3 colour = mix(uBaseColour.rgb, shaded, uLightingMix);
            fragColour = vec4(colour, uBaseColour.a);
        }
    )";

    shader.program = std::make_unique<juce::OpenGLShaderProgram>(openGLContext);
    if (!shader.program->addVertexShader(juce::OpenGLHelpers::translateVertexShaderToV3(vertexShader))
        || !shader.program->addFragmentShader(juce::OpenGLHelpers::translateFragmentShaderToV3(fragmentShader))
        || !shader.program->link())
    {
        shader.program.reset();
        return;
    }

    shader.position = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader.program, "position");
    shader.normal = std::make_unique<juce::OpenGLShaderProgram::Attribute>(*shader.program, "normal");
    shader.mvp = std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shader.program, "uMvp");
    shader.model = std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shader.program, "uModel");
    shader.baseColour = std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shader.program, "uBaseColour");
    shader.lightingMix = std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shader.program, "uLightingMix");
    shader.lightDirection = std::make_unique<juce::OpenGLShaderProgram::Uniform>(*shader.program, "uLightDirection");

    initialiseSceneBuffers();
    updateViewMatrices();
}

void EngineerViewportComponent::renderOpenGL()
{
    juce::OpenGLHelpers::clear(juce::Colour(0xff0d131b));
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE);

    const auto scale = static_cast<float>(openGLContext.getRenderingScale());
    glViewport(0, 0, juce::roundToInt(scale * static_cast<float>(getWidth())),
               juce::roundToInt(scale * static_cast<float>(getHeight())));

    updateViewMatrices();
    renderScene();
}

void EngineerViewportComponent::openGLContextClosing()
{
    releaseSceneBuffers();
    shader = {};
}

void EngineerViewportComponent::showContextMenu(const juce::MouseEvent& event)
{
    juce::PopupMenu menu;
    juce::PopupMenu viewMenu;
    viewMenu.addItem(101, "3D Design View", true, viewMode == ViewMode::design3D);
    viewMenu.addItem(102, "Assembly Floor View", true, viewMode == ViewMode::assemblyFloor);
    viewMenu.addItem(103, "Planar Layer View", true, viewMode == ViewMode::planarLayer);
    menu.addSubMenu("View Mode", viewMenu);

    juce::PopupMenu cameraMenu;
    cameraMenu.addItem(111, "ISO", true, sceneModel.getCameraPreset() == EngineerSceneModel::CameraPreset::iso);
    cameraMenu.addItem(112, "Top", true, sceneModel.getCameraPreset() == EngineerSceneModel::CameraPreset::top);
    cameraMenu.addItem(113, "Walk", true, sceneModel.getCameraPreset() == EngineerSceneModel::CameraPreset::walk);
    menu.addSubMenu("Camera", cameraMenu);

    juce::PopupMenu primitiveMenu;
    primitiveMenu.addItem(121, "Block");
    primitiveMenu.addItem(122, "Cylinder");
    primitiveMenu.addItem(123, "Plate");
    menu.addSubMenu("Add Primitive", primitiveMenu);

    if (sceneModel.isSelectedObjectDirectGeometry())
    {
        juce::PopupMenu geometryMenu;
        geometryMenu.addItem(131, "Translate", true, sceneModel.getSelectedGeometryTool() == EngineerSceneModel::GeometryTool::translate);
        geometryMenu.addItem(132, "Scale", true, sceneModel.getSelectedGeometryTool() == EngineerSceneModel::GeometryTool::scale);
        geometryMenu.addItem(133, "Extrude", true, sceneModel.getSelectedGeometryTool() == EngineerSceneModel::GeometryTool::extrude);
        geometryMenu.addItem(134, "Bevel", true, sceneModel.getSelectedGeometryTool() == EngineerSceneModel::GeometryTool::bevel);
        menu.addSubMenu("Direct Geometry", geometryMenu);
    }

    const auto clickedIndex = hitTestObject(event.position);
    juce::Component::SafePointer<EngineerViewportComponent> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({ event.getScreenPosition(), { 1, 1 } }),
                       [safeThis, clickedIndex](int result)
                       {
                           if (safeThis != nullptr)
                               safeThis->handleContextMenuResult(result, clickedIndex);
                       });
}

void EngineerViewportComponent::handleContextMenuResult(int result, int clickedIndex)
{
    switch (result)
    {
        case 101: viewMode = ViewMode::design3D; break;
        case 102: viewMode = ViewMode::assemblyFloor; break;
        case 103: viewMode = ViewMode::planarLayer; break;
        case 111: sceneModel.setCameraPreset(EngineerSceneModel::CameraPreset::iso); break;
        case 112: sceneModel.setCameraPreset(EngineerSceneModel::CameraPreset::top); break;
        case 113: sceneModel.setCameraPreset(EngineerSceneModel::CameraPreset::walk); break;
        case 121: sceneModel.addPrimitiveObject("Block"); break;
        case 122: sceneModel.addPrimitiveObject("Cylinder"); break;
        case 123: sceneModel.addPrimitiveObject("Plate"); break;
        case 131: sceneModel.setSelectedGeometryTool(EngineerSceneModel::GeometryTool::translate); break;
        case 132: sceneModel.setSelectedGeometryTool(EngineerSceneModel::GeometryTool::scale); break;
        case 133: sceneModel.setSelectedGeometryTool(EngineerSceneModel::GeometryTool::extrude); break;
        case 134: sceneModel.setSelectedGeometryTool(EngineerSceneModel::GeometryTool::bevel); break;
        default: break;
    }

    if (clickedIndex >= 0)
        sceneModel.selectObject(clickedIndex);

    applyCameraPreset(sceneModel.getCameraPreset());
}

void EngineerViewportComponent::applyCameraPreset(EngineerSceneModel::CameraPreset preset)
{
    switch (preset)
    {
        case EngineerSceneModel::CameraPreset::iso:
            yawRadians = 0.78f;
            pitchRadians = -0.55f;
            orbitDistance = 19.0f;
            useOrthographicProjection = false;
            break;
        case EngineerSceneModel::CameraPreset::top:
            yawRadians = 0.0f;
            pitchRadians = -1.40f;
            orbitDistance = 24.0f;
            useOrthographicProjection = true;
            break;
        case EngineerSceneModel::CameraPreset::walk:
            yawRadians = 0.58f;
            pitchRadians = -0.22f;
            orbitDistance = 11.0f;
            useOrthographicProjection = false;
            break;
    }

    if (viewMode == ViewMode::planarLayer)
        useOrthographicProjection = true;

    updateViewMatrices();
}

void EngineerViewportComponent::initialiseSceneBuffers()
{
    buildMeshBuffer(boxMesh, makeBoxVertices(), GL_TRIANGLES);
    buildMeshBuffer(cylinderMesh, makeCylinderVertices(), GL_TRIANGLES);
    buildMeshBuffer(gridMesh, makeGridVertices(), GL_LINES);
}

void EngineerViewportComponent::releaseSceneBuffers()
{
    auto releaseBuffer = [this](MeshBuffer& mesh)
    {
        if (mesh.vbo != 0)
            glDeleteBuffers(1, &mesh.vbo);
        if (mesh.vao != 0)
            openGLContext.extensions.glDeleteVertexArrays(1, &mesh.vao);
        mesh = {};
    };

    releaseBuffer(boxMesh);
    releaseBuffer(cylinderMesh);
    releaseBuffer(gridMesh);
}

void EngineerViewportComponent::buildMeshBuffer(MeshBuffer& mesh,
                                                const std::vector<float>& vertices,
                                                GLenum primitiveType)
{
    mesh.primitiveType = primitiveType;
    mesh.vertexCount = static_cast<GLsizei>(vertices.size() / 6);

    openGLContext.extensions.glGenVertexArrays(1, &mesh.vao);
    openGLContext.extensions.glBindVertexArray(mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(static_cast<GLuint>(shader.position->attributeID));
    glVertexAttribPointer(static_cast<GLuint>(shader.position->attributeID), 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, nullptr);
    glEnableVertexAttribArray(static_cast<GLuint>(shader.normal->attributeID));
    glVertexAttribPointer(static_cast<GLuint>(shader.normal->attributeID), 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, reinterpret_cast<const void*>(sizeof(float) * 3));

    openGLContext.extensions.glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void EngineerViewportComponent::renderScene()
{
    if (shader.program == nullptr)
        return;

    shader.program->use();
    shader.lightDirection->set(0.45f, -1.0f, 0.35f);

    renderGrid();

    const auto selectedIndex = sceneModel.getSelectedObjectIndex();
    const auto& objects = sceneModel.getObjects();
    for (size_t i = 0; i < objects.size(); ++i)
    {
        const auto& object = objects[i];
        const bool selected = static_cast<int>(i) == selectedIndex;
        renderObject(object, selected, false);
        if (object.mirrorXEnabled)
            renderObject(object, selected, true);
    }

    openGLContext.extensions.glBindVertexArray(0);
}

void EngineerViewportComponent::renderGrid() const
{
    if (gridMesh.vao == 0)
        return;

    const auto vp = multiply(projectionMatrix, viewMatrix);
    shader.mvp->setMatrix4(vp.data(), 1, false);
    shader.model->setMatrix4(identityMatrix().data(), 1, false);
    shader.baseColour->set(0.19f, 0.31f, 0.42f, 1.0f);
    shader.lightingMix->set(0.0f);

    openGLContext.extensions.glBindVertexArray(gridMesh.vao);
    glDrawArrays(gridMesh.primitiveType, 0, gridMesh.vertexCount);
}

void EngineerViewportComponent::renderObject(const EngineerSceneModel::SceneObject& object,
                                             bool selected,
                                             bool mirrored) const
{
    const auto model = objectModelMatrix(object, mirrored);
    const auto vp = multiply(projectionMatrix, viewMatrix);
    const auto mvp = multiply(vp, model);
    const auto colour = objectColour(object, selected, mirrored);

    shader.mvp->setMatrix4(mvp.data(), 1, false);
    shader.model->setMatrix4(model.data(), 1, false);
    shader.baseColour->set(colour.getFloatRed(),
                           colour.getFloatGreen(),
                           colour.getFloatBlue(),
                           colour.getFloatAlpha());
    shader.lightingMix->set(mirrored ? 0.35f : 1.0f);

    const auto& mesh = object.primitiveType.equalsIgnoreCase("Cylinder") ? cylinderMesh : boxMesh;
    openGLContext.extensions.glBindVertexArray(mesh.vao);
    glDrawArrays(mesh.primitiveType, 0, mesh.vertexCount);

    if (!mirrored)
    {
        glDisable(GL_CULL_FACE);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        shader.baseColour->set(selected ? 0.35f : 0.82f,
                               selected ? 0.82f : 0.63f,
                               1.0f,
                               selected ? 1.0f : 0.42f);
        shader.lightingMix->set(0.0f);
        glDrawArrays(mesh.primitiveType, 0, mesh.vertexCount);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_CULL_FACE);
    }
}

int EngineerViewportComponent::hitTestObject(juce::Point<float> point) const
{
    const auto projectedBounds = buildProjectedObjectBounds();
    for (auto it = projectedBounds.rbegin(); it != projectedBounds.rend(); ++it)
    {
        if (it->screenBounds.contains(point))
            return it->index;
    }

    return -1;
}

std::vector<EngineerViewportComponent::ProjectedObjectBounds> EngineerViewportComponent::buildProjectedObjectBounds() const
{
    std::vector<ProjectedObjectBounds> bounds;
    const auto vp = multiply(projectionMatrix, viewMatrix);
    const auto area = viewportRectFor(*this).reduced(2.0f);
    const auto& objects = sceneModel.getObjects();

    for (size_t i = 0; i < objects.size(); ++i)
    {
        const auto& object = objects[i];
        const auto scale = objectScaleFor(object);
        const std::array<Vec3, 8> corners {
            Vec3 { -scale.x, -scale.y, -scale.z },
            Vec3 {  scale.x, -scale.y, -scale.z },
            Vec3 { -scale.x,  scale.y, -scale.z },
            Vec3 {  scale.x,  scale.y, -scale.z },
            Vec3 { -scale.x, -scale.y,  scale.z },
            Vec3 {  scale.x, -scale.y,  scale.z },
            Vec3 { -scale.x,  scale.y,  scale.z },
            Vec3 {  scale.x,  scale.y,  scale.z }
        };

        const auto model = objectModelMatrix(object, false);
        juce::Rectangle<float> rect;
        bool started = false;
        for (const auto& corner : corners)
        {
            const auto worldCorner = multiply(model, Vec4 { corner.x, corner.y, corner.z, 1.0f });
            const auto clip = multiply(vp, worldCorner);
            if (std::abs(clip.w) < 0.0001f)
                continue;

            const auto point2d = toScreenPoint(clip, area);
            if (!started)
            {
                rect = { point2d.x, point2d.y, 1.0f, 1.0f };
                started = true;
            }
            else
            {
                rect = rect.getUnion({ point2d.x, point2d.y, 1.0f, 1.0f });
            }
        }

        if (started)
            bounds.push_back({ static_cast<int>(i), rect.expanded(4.0f), rect.getCentre() });
    }

    return bounds;
}

void EngineerViewportComponent::updateViewMatrices()
{
    const auto area = viewportRectFor(*this);
    if (area.getWidth() <= 0.0f || area.getHeight() <= 0.0f)
        return;

    if (viewMode == ViewMode::planarLayer)
        useOrthographicProjection = true;

    const auto cameraOffset = Vec3 {
        std::cos(pitchRadians) * std::sin(yawRadians) * orbitDistance,
        std::sin(-pitchRadians) * orbitDistance,
        std::cos(pitchRadians) * std::cos(yawRadians) * orbitDistance
    };
    const auto eye = Vec3 { orbitTarget.x + cameraOffset.x, orbitTarget.y + cameraOffset.y, orbitTarget.z + cameraOffset.z };
    viewMatrix = lookAtMatrix(eye, { orbitTarget.x, orbitTarget.y, orbitTarget.z }, { 0.0f, 1.0f, 0.0f });

    const auto aspect = area.getWidth() / area.getHeight();
    if (useOrthographicProjection)
    {
        const auto halfHeight = orbitDistance * 0.45f;
        const auto halfWidth = halfHeight * aspect;
        projectionMatrix = orthographicMatrix(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.1f, 200.0f);
    }
    else
    {
        const auto fov = viewMode == ViewMode::assemblyFloor ? 58.0f : 42.0f;
        projectionMatrix = perspectiveMatrix(juce::degreesToRadians(fov), aspect, 0.1f, 250.0f);
    }
}
