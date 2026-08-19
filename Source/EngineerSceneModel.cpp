#include "EngineerSceneModel.h"

#include <algorithm>

namespace
{
EngineerSceneModel::SceneObject makeSceneObject(const juce::String& name,
                                                const juce::String& primitiveType,
                                                juce::Point<float> position,
                                                juce::Point<float> size,
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
    objects = {
        makeSceneObject("Base Frame", "Block", { 0.36f, 0.62f }, { 0.26f, 0.18f }, false, {}, 0.0f, 0.0f, AuthoringState::primitive),
        makeSceneObject("Drive Housing",
                        "Cylinder",
                        { 0.56f, 0.48f },
                        { 0.15f, 0.22f },
                        true,
                        { { "Mirror X", true, 1.0f } },
                        0.0f,
                        0.0f,
                        AuthoringState::modifierStack),
        makeSceneObject("Top Plate", "Plate", { 0.49f, 0.34f }, { 0.28f, 0.10f }, false, {}, 0.07f, 0.01f, AuthoringState::directGeometry)
    };
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

void EngineerSceneModel::setSelectedObjectPosition(juce::Point<float> position)
{
    auto& object = getSelectedObjectMutable();
    object.normalizedPosition = position;
    syncDerivedState(object);
    notifyListeners();
}

void EngineerSceneModel::setSelectedObjectSize(juce::Point<float> size)
{
    auto& object = getSelectedObjectMutable();
    object.normalizedSize = size;
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
    const auto offset = static_cast<float>(objects.size()) * 0.04f;

    objects.push_back(makeSceneObject(nextName,
                                      primitiveType,
                                      { juce::jlimit(0.18f, 0.82f, 0.36f + offset),
                                        juce::jlimit(0.18f, 0.82f, 0.38f + offset * 0.55f) },
                                      { 0.18f, 0.14f },
                                      false,
                                      {},
                                      0.0f,
                                      0.0f,
                                      AuthoringState::primitive));
    selectedObjectIndex = static_cast<int>(objects.size()) - 1;
    notifyListeners();
}

void EngineerSceneModel::duplicateSelectedObject()
{
    auto duplicate = getSelectedObject();
    duplicate.name = makeUniqueObjectName(objects, duplicate.name);
    duplicate.normalizedPosition.x = juce::jlimit(0.14f, 0.86f, duplicate.normalizedPosition.x + 0.05f);
    duplicate.normalizedPosition.y = juce::jlimit(0.14f, 0.86f, duplicate.normalizedPosition.y + 0.05f);
    objects.push_back(duplicate);
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

void EngineerSceneModel::nudgeSelectedGeometryElement(juce::Point<float> delta)
{
    auto& object = getSelectedObjectMutable();
    if (object.authoringState != AuthoringState::directGeometry)
        return;

    const auto minSize = 0.05f;

    switch (selectedGeometryElementKind)
    {
        case GeometryElementKind::vertex:
        {
            auto left = object.normalizedPosition.x - object.normalizedSize.x * 0.5f;
            auto right = object.normalizedPosition.x + object.normalizedSize.x * 0.5f;
            auto top = object.normalizedPosition.y - object.normalizedSize.y * 0.5f;
            auto bottom = object.normalizedPosition.y + object.normalizedSize.y * 0.5f;

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

            object.normalizedPosition = { (left + right) * 0.5f, (top + bottom) * 0.5f };
            object.normalizedSize = { right - left, bottom - top };
            break;
        }

        case GeometryElementKind::edge:
        {
            switch (selectedGeometryElementIndex)
            {
                case 0:
                    object.normalizedPosition.y += delta.y * 0.5f;
                    object.normalizedSize.y -= delta.y;
                    break;
                case 1:
                    object.normalizedPosition.x += delta.x * 0.5f;
                    object.normalizedSize.x += delta.x;
                    break;
                case 2:
                    object.normalizedPosition.y += delta.y * 0.5f;
                    object.normalizedSize.y += delta.y;
                    break;
                case 3:
                    object.normalizedPosition.x += delta.x * 0.5f;
                    object.normalizedSize.x -= delta.x;
                    break;
                default:
                    break;
            }
            break;
        }

        case GeometryElementKind::face:
            object.normalizedPosition += delta;
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

    object.normalizedPosition += delta;
    clampDirectGeometryObject(object);
    notifyListeners();
}

void EngineerSceneModel::scaleSelectedDirectGeometry(juce::Point<float> delta)
{
    auto& object = getSelectedObjectMutable();
    if (object.authoringState != AuthoringState::directGeometry)
        return;

    object.normalizedSize += delta;
    clampDirectGeometryObject(object);
    notifyListeners();
}

void EngineerSceneModel::extrudeSelectedDirectGeometry(float amount)
{
    auto& object = getSelectedObjectMutable();
    if (object.authoringState != AuthoringState::directGeometry)
        return;

    object.geometryDepth = juce::jlimit(0.02f, 0.24f, object.geometryDepth + amount);
    notifyListeners();
}

void EngineerSceneModel::bevelSelectedDirectGeometry(float amount)
{
    auto& object = getSelectedObjectMutable();
    if (object.authoringState != AuthoringState::directGeometry)
        return;

    object.bevelAmount = juce::jlimit(0.0f, 0.08f, object.bevelAmount + amount);
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
    object.normalizedSize.x = juce::jlimit(0.05f, 0.55f, object.normalizedSize.x);
    object.normalizedSize.y = juce::jlimit(0.05f, 0.55f, object.normalizedSize.y);

    const auto halfWidth = object.normalizedSize.x * 0.5f;
    const auto halfHeight = object.normalizedSize.y * 0.5f;
    object.normalizedPosition.x = juce::jlimit(0.02f + halfWidth, 0.98f - halfWidth, object.normalizedPosition.x);
    object.normalizedPosition.y = juce::jlimit(0.02f + halfHeight, 0.98f - halfHeight, object.normalizedPosition.y);
}

void EngineerSceneModel::notifyListeners()
{
    listeners.call([](Listener& listener) { listener.engineerSceneModelChanged(); });
}
