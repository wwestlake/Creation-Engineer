#include "EngineerSceneModel.h"

#include <algorithm>
#include <cmath>

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

int geometryElementCountForKind(EngineerSceneModel::GeometryElementKind kind)
{
    switch (kind)
    {
        case EngineerSceneModel::GeometryElementKind::vertex: return 4;
        case EngineerSceneModel::GeometryElementKind::edge: return 4;
        case EngineerSceneModel::GeometryElementKind::face: return 1;
    }

    return 1;
}
}

EngineerSceneModel::EngineerSceneModel()
{
    // Real-unit (meters) demo scene, Y-up, objects resting on the y=0 grid
    // (position.y == size.y * 0.5). Replaces the old [0,1]-normalized-plane
    // mockup coordinates -- see the docking rollout plan's Part B.
    objects = {
        makeSceneObject("Base Frame", "Block", { 0.0f, 0.15f, 0.0f }, { 1.6f, 0.3f, 1.1f }, false, {}, 0.0f, 0.0f, AuthoringState::primitive),
        makeSceneObject("Drive Housing",
                        "Cylinder",
                        { 1.2f, 0.35f, -0.4f },
                        { 0.5f, 0.7f, 0.5f },
                        true,
                        { { "Mirror X", true, 1.0f } },
                        0.0f,
                        0.0f,
                        AuthoringState::modifierStack),
        makeSceneObject("Top Plate", "Plate", { 0.6f, 0.65f, 0.3f }, { 1.7f, 0.15f, 0.7f }, false, {}, 0.07f, 0.01f, AuthoringState::directGeometry)
    };

    for (auto& object : objects)
        object.objectId = nextObjectId_++;
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
    selectedGeometryElementIndex = juce::jlimit(0, getSelectedGeometryElementCount() - 1, selectedGeometryElementIndex);
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
    notifyListeners();
}

void EngineerSceneModel::restoreSelectedObjectPrimitiveWorkflow()
{
    auto& object = getSelectedObjectMutable();
    object.modifiers.clear();
    object.geometryDepth = 0.0f;
    object.bevelAmount = 0.0f;
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

EngineerSceneModel::GeometryElementKind EngineerSceneModel::getSelectedGeometryElementKind() const noexcept
{
    return selectedGeometryElementKind;
}

void EngineerSceneModel::setSelectedGeometryElementKind(GeometryElementKind kind) noexcept
{
    selectedGeometryElementKind = kind;
    selectedGeometryElementIndex = juce::jlimit(0, getSelectedGeometryElementCount() - 1, 0);
    notifyListeners();
}

int EngineerSceneModel::getSelectedGeometryElementIndex() const noexcept
{
    return selectedGeometryElementIndex;
}

void EngineerSceneModel::setSelectedGeometryElementIndex(int index) noexcept
{
    selectedGeometryElementIndex = juce::jlimit(0, getSelectedGeometryElementCount() - 1, index);
    notifyListeners();
}

int EngineerSceneModel::getSelectedGeometryElementCount() const noexcept
{
    return geometryElementCountForKind(selectedGeometryElementKind);
}

void EngineerSceneModel::selectPreviousGeometryElement() noexcept
{
    selectedGeometryElementIndex = juce::jlimit(0, getSelectedGeometryElementCount() - 1, selectedGeometryElementIndex - 1);
    notifyListeners();
}

void EngineerSceneModel::selectNextGeometryElement() noexcept
{
    selectedGeometryElementIndex = juce::jlimit(0, getSelectedGeometryElementCount() - 1, selectedGeometryElementIndex + 1);
    notifyListeners();
}

// delta stays a 2D juce::Point<float> (callers are fixed-magnitude panel
// buttons, see EngineerGeometryToolsComponent/EngineerGeometryElementsComponent)
// -- it always meant "footprint plane" motion, which was X/Y in the old
// normalized-2D model and is X/Z now that position/size are real 3D. Height
// (Y) was never touched by these calls before and still isn't.
void EngineerSceneModel::nudgeSelectedGeometryElement(juce::Point<float> delta)
{
    auto& object = getSelectedObjectMutable();
    if (object.authoringState != AuthoringState::directGeometry)
        return;

    delta = snapDelta(delta);

    const auto minSize = 0.05f;

    switch (selectedGeometryElementKind)
    {
        case GeometryElementKind::vertex:
        {
            auto left = object.position.x - object.size.x * 0.5f;
            auto right = object.position.x + object.size.x * 0.5f;
            auto top = object.position.z - object.size.z * 0.5f;
            auto bottom = object.position.z + object.size.z * 0.5f;

            switch (selectedGeometryElementIndex)
            {
                case 0: left += delta.x; top += delta.y; break;
                case 1: right += delta.x; top += delta.y; break;
                case 2: right += delta.x; bottom += delta.y; break;
                case 3: left += delta.x; bottom += delta.y; break;
                default: break;
            }

            if (right - left < minSize)
                right = left + minSize;
            if (bottom - top < minSize)
                bottom = top + minSize;

            object.position.x = (left + right) * 0.5f;
            object.position.z = (top + bottom) * 0.5f;
            object.size.x = right - left;
            object.size.z = bottom - top;
            break;
        }

        case GeometryElementKind::edge:
        {
            switch (selectedGeometryElementIndex)
            {
                case 0:
                    object.position.z += delta.y * 0.5f;
                    object.size.z -= delta.y;
                    break;
                case 1:
                    object.position.x += delta.x * 0.5f;
                    object.size.x += delta.x;
                    break;
                case 2:
                    object.position.z += delta.y * 0.5f;
                    object.size.z += delta.y;
                    break;
                case 3:
                    object.position.x += delta.x * 0.5f;
                    object.size.x -= delta.x;
                    break;
                default:
                    break;
            }
            break;
        }

        case GeometryElementKind::face:
            object.position.x += delta.x;
            object.position.z += delta.y;
            break;
    }

    clampDirectGeometryObject(object);
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

juce::String EngineerSceneModel::toDisplayString(GeometryElementKind kind)
{
    switch (kind)
    {
        case GeometryElementKind::vertex: return "Vertex";
        case GeometryElementKind::edge: return "Edge";
        case GeometryElementKind::face: return "Face";
    }

    return "Vertex";
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
