#include "EngineerSceneModel.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace
{
EngineerSceneModel::SceneObject makeSceneObject(const juce::String& name,
                                                const juce::String& primitiveType,
                                                juce::Vector3D<float> position,
                                                juce::Vector3D<float> size,
                                                bool mirrorXEnabled,
                                                std::vector<EngineerSceneModel::ModifierEntry> modifiers,
                                                float geometryDepth,
                                                float bevelAmount,
                                                EngineerSceneModel::AuthoringState authoringState)
{
    return { name, primitiveType, position, size, mirrorXEnabled, std::move(modifiers), geometryDepth, bevelAmount, authoringState };
}

juce::String baseNameForPrimitive(const juce::String& primitiveType)
{
    if (primitiveType.equalsIgnoreCase("Cylinder"))
        return "Cylinder Part";

    if (primitiveType.equalsIgnoreCase("Plate"))
        return "Plate Part";

    return "Block Part";
}

juce::String makeUniqueObjectName(const std::vector<EngineerSceneModel::SceneObject>& objects,
                                  const juce::String& desiredName)
{
    auto candidate = desiredName.trim();
    if (candidate.isEmpty())
        candidate = "Engineer Part";

    auto hasName = [&](const juce::String& name)
    {
        for (const auto& object : objects)
            if (object.name.equalsIgnoreCase(name))
                return true;

        return false;
    };

    if (!hasName(candidate))
        return candidate;

    for (int suffix = 2; suffix < 1000; ++suffix)
    {
        auto numbered = candidate + " " + juce::String(suffix);
        if (!hasName(numbered))
            return numbered;
    }

    return candidate + " Copy";
}

// Fixed 8-corner order every editable vertex cage uses (bit0=+X, bit1=+Y,
// bit2=+Z) -- matches shared/Render's BuildFlatShadedMeshFromCage exactly,
// so seeding here and expanding-for-rendering there never disagree.
std::vector<juce::Vector3D<float>> makeBoxCage(juce::Vector3D<float> position, juce::Vector3D<float> size)
{
    const auto half = size * 0.5f;
    std::vector<juce::Vector3D<float>> corners;
    corners.reserve(8);
    for (int i = 0; i < 8; ++i)
    {
        corners.push_back({ position.x + ((i & 1) ? half.x : -half.x),
                            position.y + ((i & 2) ? half.y : -half.y),
                            position.z + ((i & 4) ? half.z : -half.z) });
    }
    return corners;
}
}

EngineerSceneModel::EngineerSceneModel()
{
    // Real-unit (meters) demo scene, Y-up, objects resting on the y=0 grid
    // (position.y == size.y * 0.5). Replaces the old [0,1]-normalized-plane
    // mockup coordinates -- see the docking rollout plan's Part B.
    // A single default object -- 10cm cube, centered at the origin, metric
    // units -- rather than a demo assembly. Real content starts from the
    // library panel or the Navigator's "Add Primitive" actions.
    objects = {
        makeSceneObject("Cube", "Block", { 0.0f, 0.0f, 0.0f }, { 0.1f, 0.1f, 0.1f }, false, {}, 0.0f, 0.0f, AuthoringState::primitive)
    };

    for (auto& object : objects)
        object.objectId = nextObjectId_++;

    layers.push_back({ "layer:default", "Default", true, juce::Colours::white, 1.0f });
}

const std::vector<EngineerSceneModel::SceneObject>& EngineerSceneModel::getObjects() const noexcept
{
    return objects;
}

int EngineerSceneModel::getSelectedObjectIndex() const noexcept
{
    return selectedObjectIndex;
}

const EngineerSceneModel::SceneObject& EngineerSceneModel::getSelectedObject() const noexcept
{
    return objects[static_cast<size_t>(juce::jlimit(0, static_cast<int>(objects.size()) - 1, selectedObjectIndex))];
}

EngineerSceneModel::SceneObject& EngineerSceneModel::getSelectedObjectMutable() noexcept
{
    return objects[static_cast<size_t>(juce::jlimit(0, static_cast<int>(objects.size()) - 1, selectedObjectIndex))];
}

void EngineerSceneModel::selectObject(int index) noexcept
{
    selectedObjectIndex = juce::jlimit(0, static_cast<int>(objects.size()) - 1, index);
    notifyListeners();
}

void EngineerSceneModel::setSelectedObjectName(const juce::String& name)
{
    getSelectedObjectMutable().name = name;
    notifyListeners();
}

void EngineerSceneModel::setSelectedObjectPrimitiveType(const juce::String& primitiveType)
{
    auto& object = getSelectedObjectMutable();
    object.primitiveType = primitiveType;
    if (object.authoringState == AuthoringState::directGeometry)
        object.authoringState = AuthoringState::primitive;
    syncDerivedState(object);
    notifyListeners();
}

void EngineerSceneModel::setSelectedObjectPosition(juce::Vector3D<float> position)
{
    auto& object = getSelectedObjectMutable();
    object.position = position;
    syncDerivedState(object);
    notifyListeners();
}

void EngineerSceneModel::setSelectedObjectSize(juce::Vector3D<float> size)
{
    auto& object = getSelectedObjectMutable();
    object.size = size;
    syncDerivedState(object);
    notifyListeners();
}

void EngineerSceneModel::setSelectedObjectMirrorXEnabled(bool enabled)
{
    auto& object = getSelectedObjectMutable();
    bool found = false;
    for (auto& modifier : object.modifiers)
    {
        if (modifier.type == "Mirror X")
        {
            modifier.enabled = enabled;
            found = true;
        }
    }

    if (enabled && !found)
        object.modifiers.push_back({ "Mirror X", true, 1.0f });

    if (!enabled)
    {
        object.modifiers.erase(std::remove_if(object.modifiers.begin(),
                                              object.modifiers.end(),
                                              [](const ModifierEntry& modifier) { return modifier.type == "Mirror X"; }),
                               object.modifiers.end());
    }

    syncDerivedState(object);
    notifyListeners();
}

bool EngineerSceneModel::isSelectedObjectMirrorXEnabled() const noexcept
{
    return getSelectedObject().mirrorXEnabled;
}

EngineerSceneModel::AuthoringState EngineerSceneModel::getSelectedObjectAuthoringState() const noexcept
{
    return getSelectedObject().authoringState;
}

const std::vector<EngineerSceneModel::ModifierEntry>& EngineerSceneModel::getSelectedObjectModifiers() const noexcept
{
    return getSelectedObject().modifiers;
}

void EngineerSceneModel::addPrimitiveObject(const juce::String& primitiveType)
{
    const auto baseName = baseNameForPrimitive(primitiveType);
    const auto nextName = makeUniqueObjectName(objects, baseName);
    const auto offset = static_cast<float>(objects.size()) * 0.6f;
    constexpr float defaultHeight = 0.3f;

    objects.push_back(makeSceneObject(nextName,
                                      primitiveType,
                                      { offset, defaultHeight * 0.5f, offset * 0.55f },
                                      { 0.5f, defaultHeight, 0.5f },
                                      false,
                                      {},
                                      0.0f,
                                      0.0f,
                                      AuthoringState::primitive));
    objects.back().objectId = nextObjectId_++;
    selectedObjectIndex = static_cast<int>(objects.size()) - 1;
    notifyListeners();
}

void EngineerSceneModel::duplicateSelectedObject()
{
    auto duplicate = getSelectedObject();
    duplicate.name = makeUniqueObjectName(objects, duplicate.name);
    duplicate.position.x += 0.3f;
    duplicate.position.z += 0.3f;
    objects.push_back(duplicate);
    objects.back().objectId = nextObjectId_++;
    selectedObjectIndex = static_cast<int>(objects.size()) - 1;
    notifyListeners();
}

void EngineerSceneModel::addLibraryPartObject(const juce::String& profileId, const juce::String& materialId,
                                              float lengthMeters, juce::Vector3D<float> initialSize)
{
    SceneObject object;
    object.objectId = nextObjectId_++;
    object.name = makeUniqueObjectName(objects, "Library Part");
    object.primitiveType = "LibraryPart";
    object.position = { 0.0f, initialSize.y * 0.5f, 0.0f };
    object.size = initialSize;
    object.authoringState = AuthoringState::primitive;
    object.libraryPart = LibraryPartState{ profileId, materialId, lengthMeters, false };

    objects.push_back(object);
    selectedObjectIndex = static_cast<int>(objects.size()) - 1;
    notifyListeners();
}

void EngineerSceneModel::setSelectedLibraryPartLength(float lengthMeters)
{
    auto& object = getSelectedObjectMutable();
    if (!object.libraryPart.has_value() || object.libraryPart->baked)
        return;

    object.libraryPart->lengthMeters = juce::jmax(0.01f, lengthMeters);
    object.size.y = object.libraryPart->lengthMeters;
    notifyListeners();
}

void EngineerSceneModel::bakeSelectedLibraryPart()
{
    auto& object = getSelectedObjectMutable();
    if (!object.libraryPart.has_value())
        return;

    // Baking is purely this flag flip -- EngineerViewportComponent's
    // per-object mesh cache (keyed by objectId) stops regenerating once
    // baked == true, since its "does the cached profile/length still match"
    // check has nothing new to compare against. profileId/materialId/
    // lengthMeters are never cleared, which is what makes unbaking
    // reversible rather than a one-way conversion.
    object.libraryPart->baked = true;
    notifyListeners();
}

void EngineerSceneModel::unbakeSelectedLibraryPart()
{
    auto& object = getSelectedObjectMutable();
    if (!object.libraryPart.has_value())
        return;

    object.libraryPart->baked = false;
    notifyListeners();
}

bool EngineerSceneModel::isSelectedObjectLibraryPart() const noexcept
{
    return getSelectedObject().libraryPart.has_value();
}

bool EngineerSceneModel::isSelectedLibraryPartBaked() const noexcept
{
    const auto& object = getSelectedObject();
    return object.libraryPart.has_value() && object.libraryPart->baked;
}

void EngineerSceneModel::addConnectorObject(const juce::String& connectorId, juce::Vector3D<float> size)
{
    SceneObject object;
    object.objectId = nextObjectId_++;
    object.name = makeUniqueObjectName(objects, "Connector");
    object.primitiveType = "Connector";
    object.position = { 0.0f, size.y * 0.5f, 0.0f };
    object.size = size;
    object.authoringState = AuthoringState::primitive;
    object.connectorId = connectorId;

    objects.push_back(object);
    selectedObjectIndex = static_cast<int>(objects.size()) - 1;
    notifyListeners();
}

bool EngineerSceneModel::canRemoveSelectedObject() const noexcept
{
    return objects.size() > 1;
}

void EngineerSceneModel::removeSelectedObject()
{
    if (!canRemoveSelectedObject())
        return;

    objects.erase(objects.begin() + juce::jlimit(0, static_cast<int>(objects.size()) - 1, selectedObjectIndex));
    selectedObjectIndex = juce::jlimit(0, static_cast<int>(objects.size()) - 1, selectedObjectIndex);
    notifyListeners();
}

void EngineerSceneModel::addSelectedObjectMirrorModifier()
{
    auto& object = getSelectedObjectMutable();
    for (auto& modifier : object.modifiers)
    {
        if (modifier.type == "Mirror X")
        {
            modifier.enabled = true;
            syncDerivedState(object);
            notifyListeners();
            return;
        }
    }

    object.modifiers.push_back({ "Mirror X", true, 1.0f });
    syncDerivedState(object);
    notifyListeners();
}

void EngineerSceneModel::addSelectedObjectShellModifier()
{
    auto& object = getSelectedObjectMutable();
    object.modifiers.push_back({ "Shell", true, 2.5f });
    syncDerivedState(object);
    notifyListeners();
}

bool EngineerSceneModel::canRemoveSelectedModifier(int index) const noexcept
{
    return index >= 0 && index < static_cast<int>(getSelectedObject().modifiers.size());
}

void EngineerSceneModel::removeSelectedModifier(int index)
{
    auto& object = getSelectedObjectMutable();
    if (index < 0 || index >= static_cast<int>(object.modifiers.size()))
        return;

    object.modifiers.erase(object.modifiers.begin() + index);
    syncDerivedState(object);
    notifyListeners();
}

bool EngineerSceneModel::canMoveSelectedModifierUp(int index) const noexcept
{
    return index > 0 && index < static_cast<int>(getSelectedObject().modifiers.size());
}

bool EngineerSceneModel::canMoveSelectedModifierDown(int index) const noexcept
{
    return index >= 0 && index < static_cast<int>(getSelectedObject().modifiers.size()) - 1;
}

void EngineerSceneModel::moveSelectedModifierUp(int index)
{
    auto& object = getSelectedObjectMutable();
    if (!canMoveSelectedModifierUp(index))
        return;

    std::swap(object.modifiers[static_cast<size_t>(index)],
              object.modifiers[static_cast<size_t>(index - 1)]);
    syncDerivedState(object);
    notifyListeners();
}

void EngineerSceneModel::moveSelectedModifierDown(int index)
{
    auto& object = getSelectedObjectMutable();
    if (!canMoveSelectedModifierDown(index))
        return;

    std::swap(object.modifiers[static_cast<size_t>(index)],
              object.modifiers[static_cast<size_t>(index + 1)]);
    syncDerivedState(object);
    notifyListeners();
}

void EngineerSceneModel::setSelectedModifierEnabled(int index, bool enabled)
{
    auto& object = getSelectedObjectMutable();
    if (index < 0 || index >= static_cast<int>(object.modifiers.size()))
        return;

    object.modifiers[static_cast<size_t>(index)].enabled = enabled;
    syncDerivedState(object);
    notifyListeners();
}

void EngineerSceneModel::commitSelectedObjectToDirectGeometry()
{
    auto& object = getSelectedObjectMutable();
    object.authoringState = AuthoringState::directGeometry;
    object.mirrorXEnabled = false;
    object.geometryDepth = juce::jmax(object.geometryDepth, 0.05f);
    object.bevelAmount = juce::jlimit(0.0f, 0.08f, object.bevelAmount);
    object.editableVertices = makeBoxCage(object.position, object.size);
    object.selectedVertexIndex = 0;
    notifyListeners();
}

void EngineerSceneModel::restoreSelectedObjectPrimitiveWorkflow()
{
    auto& object = getSelectedObjectMutable();
    object.modifiers.clear();
    object.geometryDepth = 0.0f;
    object.bevelAmount = 0.0f;
    object.editableVertices.clear();
    object.selectedVertexIndex = -1;
    syncDerivedState(object);
    notifyListeners();
}

bool EngineerSceneModel::isSelectedObjectDirectGeometry() const noexcept
{
    return getSelectedObject().authoringState == AuthoringState::directGeometry;
}

EngineerSceneModel::GeometryTool EngineerSceneModel::getSelectedGeometryTool() const noexcept
{
    return selectedGeometryTool;
}

void EngineerSceneModel::setSelectedGeometryTool(GeometryTool tool) noexcept
{
    selectedGeometryTool = tool;
    notifyListeners();
}

bool EngineerSceneModel::isGeometrySnappingEnabled() const noexcept
{
    return geometrySnappingEnabled;
}

void EngineerSceneModel::setGeometrySnappingEnabled(bool enabled) noexcept
{
    geometrySnappingEnabled = enabled;
    notifyListeners();
}

float EngineerSceneModel::getGeometrySnapStep() const noexcept
{
    return geometrySnapStep;
}

void EngineerSceneModel::setGeometrySnapStep(float step) noexcept
{
    geometrySnapStep = juce::jlimit(0.0025f, 0.05f, step);
    notifyListeners();
}

EngineerSceneModel::EditMode EngineerSceneModel::getEditMode() const noexcept
{
    return editMode_;
}

void EngineerSceneModel::setEditMode(EditMode mode) noexcept
{
    editMode_ = mode;
    notifyListeners();
}

bool EngineerSceneModel::isSelectedObjectVertexEditable() const noexcept
{
    return !getSelectedObject().editableVertices.empty();
}

int EngineerSceneModel::getSelectedVertexIndex() const noexcept
{
    return getSelectedObject().selectedVertexIndex;
}

int EngineerSceneModel::getSelectedVertexCount() const noexcept
{
    return static_cast<int>(getSelectedObject().editableVertices.size());
}

void EngineerSceneModel::selectVertex(int vertexIndex) noexcept
{
    auto& object = getSelectedObjectMutable();
    if (object.editableVertices.empty())
        return;

    object.selectedVertexIndex = juce::jlimit(0, static_cast<int>(object.editableVertices.size()) - 1, vertexIndex);
    notifyListeners();
}

void EngineerSceneModel::selectPreviousVertex() noexcept
{
    auto& object = getSelectedObjectMutable();
    if (object.editableVertices.empty())
        return;

    const auto count = static_cast<int>(object.editableVertices.size());
    object.selectedVertexIndex = juce::jlimit(0, count - 1, object.selectedVertexIndex - 1);
    notifyListeners();
}

void EngineerSceneModel::selectNextVertex() noexcept
{
    auto& object = getSelectedObjectMutable();
    if (object.editableVertices.empty())
        return;

    const auto count = static_cast<int>(object.editableVertices.size());
    object.selectedVertexIndex = juce::jlimit(0, count - 1, object.selectedVertexIndex + 1);
    notifyListeners();
}

juce::Vector3D<float> EngineerSceneModel::getSelectedVertexPosition() const noexcept
{
    const auto& object = getSelectedObject();
    if (object.selectedVertexIndex < 0 || object.selectedVertexIndex >= static_cast<int>(object.editableVertices.size()))
        return {};

    return object.editableVertices[static_cast<size_t>(object.selectedVertexIndex)];
}

void EngineerSceneModel::setSelectedVertexPosition(juce::Vector3D<float> worldPosition)
{
    auto& object = getSelectedObjectMutable();
    if (object.selectedVertexIndex < 0 || object.selectedVertexIndex >= static_cast<int>(object.editableVertices.size()))
        return;

    object.editableVertices[static_cast<size_t>(object.selectedVertexIndex)] = worldPosition;

    // position/size stay a derived AABB of editableVertices once a cage
    // exists -- the ray-vs-AABB hit-test path and the layer-tint colour path
    // both still read position/size uniformly for every object kind, so this
    // keeps hit-testing correct even though the vertex cage (not the AABB)
    // is what's authoritative for rendering. See the drawing/layers/vertex-
    // editing plan's Risks section.
    juce::Vector3D<float> minimum = object.editableVertices.front();
    juce::Vector3D<float> maximum = object.editableVertices.front();
    for (const auto& vertex : object.editableVertices)
    {
        minimum.x = juce::jmin(minimum.x, vertex.x);
        minimum.y = juce::jmin(minimum.y, vertex.y);
        minimum.z = juce::jmin(minimum.z, vertex.z);
        maximum.x = juce::jmax(maximum.x, vertex.x);
        maximum.y = juce::jmax(maximum.y, vertex.y);
        maximum.z = juce::jmax(maximum.z, vertex.z);
    }
    object.position = (minimum + maximum) * 0.5f;
    object.size = maximum - minimum;

    notifyListeners();
}

void EngineerSceneModel::nudgeSelectedDirectGeometryPosition(juce::Point<float> delta)
{
    auto& object = getSelectedObjectMutable();
    if (object.authoringState != AuthoringState::directGeometry)
        return;

    const auto snapped = snapDelta(delta);
    object.position.x += snapped.x;
    object.position.z += snapped.y;
    clampDirectGeometryObject(object);
    notifyListeners();
}

void EngineerSceneModel::scaleSelectedDirectGeometry(juce::Point<float> delta)
{
    auto& object = getSelectedObjectMutable();
    if (object.authoringState != AuthoringState::directGeometry)
        return;

    const auto snapped = snapDelta(delta);
    object.size.x += snapped.x;
    object.size.z += snapped.y;
    clampDirectGeometryObject(object);
    notifyListeners();
}

void EngineerSceneModel::extrudeSelectedDirectGeometry(float amount)
{
    auto& object = getSelectedObjectMutable();
    if (object.authoringState != AuthoringState::directGeometry)
        return;

    object.geometryDepth = juce::jlimit(0.02f, 0.24f, object.geometryDepth + snapScalar(amount));
    notifyListeners();
}

void EngineerSceneModel::bevelSelectedDirectGeometry(float amount)
{
    auto& object = getSelectedObjectMutable();
    if (object.authoringState != AuthoringState::directGeometry)
        return;

    object.bevelAmount = juce::jlimit(0.0f, 0.08f, object.bevelAmount + snapScalar(amount));
    notifyListeners();
}

juce::String EngineerSceneModel::toDisplayString(AuthoringState state)
{
    switch (state)
    {
        case AuthoringState::primitive: return "Primitive";
        case AuthoringState::modifierStack: return "Modifier Stack";
        case AuthoringState::directGeometry: return "Direct Geometry";
    }

    return "Primitive";
}

juce::String EngineerSceneModel::toDisplayString(const ModifierEntry& modifier)
{
    juce::String text = modifier.type;

    if (modifier.type == "Shell")
        text << " (" << juce::String(modifier.amount, 1) << " mm)";

    text << (modifier.enabled ? "  [Enabled]" : "  [Disabled]");
    return text;
}

juce::String EngineerSceneModel::toDisplayString(GeometryTool tool)
{
    switch (tool)
    {
        case GeometryTool::translate: return "Translate";
        case GeometryTool::scale: return "Scale";
        case GeometryTool::extrude: return "Extrude";
        case GeometryTool::bevel: return "Bevel";
    }

    return "Translate";
}

EngineerSceneModel::CameraPreset EngineerSceneModel::getCameraPreset() const noexcept
{
    return cameraPreset;
}

void EngineerSceneModel::setCameraPreset(CameraPreset preset) noexcept
{
    cameraPreset = preset;
    notifyListeners();
}

const std::vector<EngineerSceneModel::Layer>& EngineerSceneModel::getLayers() const noexcept
{
    return layers;
}

const EngineerSceneModel::Layer* EngineerSceneModel::findLayer(const juce::String& layerId) const noexcept
{
    for (const auto& layer : layers)
        if (layer.id == layerId)
            return std::addressof(layer);

    return nullptr;
}

void EngineerSceneModel::addLayer(const juce::String& name)
{
    layers.push_back({ juce::Uuid().toString(), name, true, juce::Colours::white, 1.0f });
    notifyListeners();
}

bool EngineerSceneModel::canRemoveLayer(const juce::String& layerId) const noexcept
{
    return layerId != "layer:default" && findLayer(layerId) != nullptr;
}

void EngineerSceneModel::removeLayer(const juce::String& layerId)
{
    if (!canRemoveLayer(layerId))
        return;

    for (auto& object : objects)
        if (object.layerId == layerId)
            object.layerId = "layer:default";

    layers.erase(std::remove_if(layers.begin(), layers.end(),
                                [&layerId](const Layer& layer) { return layer.id == layerId; }),
                layers.end());
    notifyListeners();
}

void EngineerSceneModel::renameLayer(const juce::String& layerId, const juce::String& name)
{
    for (auto& layer : layers)
        if (layer.id == layerId)
            layer.name = name;

    notifyListeners();
}

void EngineerSceneModel::setLayerVisible(const juce::String& layerId, bool visible)
{
    for (auto& layer : layers)
        if (layer.id == layerId)
            layer.visible = visible;

    notifyListeners();
}

void EngineerSceneModel::setLayerTint(const juce::String& layerId, juce::Colour tint)
{
    for (auto& layer : layers)
        if (layer.id == layerId)
            layer.tint = tint;

    notifyListeners();
}

void EngineerSceneModel::setLayerOpacity(const juce::String& layerId, float opacity)
{
    for (auto& layer : layers)
        if (layer.id == layerId)
            layer.opacity = juce::jlimit(0.0f, 1.0f, opacity);

    notifyListeners();
}

void EngineerSceneModel::moveObjectToLayer(int objectIndex, const juce::String& layerId)
{
    if (objectIndex < 0 || objectIndex >= static_cast<int>(objects.size()))
        return;

    objects[static_cast<size_t>(objectIndex)].layerId = findLayer(layerId) != nullptr ? layerId : juce::String("layer:default");
    notifyListeners();
}

int EngineerSceneModel::getNextObjectIdForSerialization() const noexcept
{
    return nextObjectId_;
}

void EngineerSceneModel::loadDrawing(std::vector<SceneObject> loadedObjects, std::vector<Layer> loadedLayers,
                                     int loadedSelectedObjectIndex, int loadedNextObjectId,
                                     CameraPreset loadedCameraPreset)
{
    if (loadedObjects.empty())
        loadedObjects.push_back(makeSceneObject("Cube", "Block", { 0.0f, 0.0f, 0.0f }, { 0.1f, 0.1f, 0.1f },
                                                false, {}, 0.0f, 0.0f, AuthoringState::primitive));
    if (loadedLayers.empty())
        loadedLayers.push_back({ "layer:default", "Default", true, juce::Colours::white, 1.0f });

    objects = std::move(loadedObjects);
    layers = std::move(loadedLayers);
    selectedObjectIndex = juce::jlimit(0, static_cast<int>(objects.size()) - 1, loadedSelectedObjectIndex);
    nextObjectId_ = juce::jmax(1, loadedNextObjectId);
    cameraPreset = loadedCameraPreset;
    notifyListeners();
}

void EngineerSceneModel::addListener(Listener* listener)
{
    listeners.add(listener);
}

void EngineerSceneModel::removeListener(Listener* listener)
{
    listeners.remove(listener);
}

void EngineerSceneModel::syncDerivedState(SceneObject& object) noexcept
{
    object.mirrorXEnabled = false;
    for (const auto& modifier : object.modifiers)
    {
        if (modifier.type == "Mirror X" && modifier.enabled)
            object.mirrorXEnabled = true;
    }

    if (object.authoringState == AuthoringState::directGeometry)
        return;

    object.authoringState = object.modifiers.empty() ? AuthoringState::primitive
                                                     : AuthoringState::modifierStack;
}

void EngineerSceneModel::clampDirectGeometryObject(SceneObject& object) noexcept
{
    // Real-unit (meters) bounds, replacing the old [0,1]-normalized-canvas
    // clamp -- a generous footprint size range and a loose position bound
    // just to keep repeated nudges from drifting the object to infinity,
    // not a real "workspace canvas" the way the old normalized space was.
    object.size.x = juce::jlimit(0.05f, 5.0f, object.size.x);
    object.size.z = juce::jlimit(0.05f, 5.0f, object.size.z);

    object.position.x = juce::jlimit(-25.0f, 25.0f, object.position.x);
    object.position.z = juce::jlimit(-25.0f, 25.0f, object.position.z);
}

juce::Point<float> EngineerSceneModel::snapDelta(juce::Point<float> delta) const noexcept
{
    if (!geometrySnappingEnabled)
        return delta;

    return { snapScalar(delta.x), snapScalar(delta.y) };
}

float EngineerSceneModel::snapScalar(float value) const noexcept
{
    if (!geometrySnappingEnabled)
        return value;

    if (std::abs(value) < 0.0001f)
        return 0.0f;

    const auto scaled = value / geometrySnapStep;
    const auto snapped = std::round(scaled) * geometrySnapStep;
    if (std::abs(snapped) < geometrySnapStep)
        return value > 0.0f ? geometrySnapStep : -geometrySnapStep;

    return snapped;
}

void EngineerSceneModel::notifyListeners()
{
    listeners.call([](Listener& listener) { listener.engineerSceneModelChanged(); });
}
