#include "EngineerSceneSerialization.h"

namespace
{
juce::var vector3ToVar(juce::Vector3D<float> v)
{
    auto* object = new juce::DynamicObject();
    object->setProperty("x", v.x);
    object->setProperty("y", v.y);
    object->setProperty("z", v.z);
    return juce::var(object);
}

bool vector3FromVar(const juce::var& value, juce::Vector3D<float>& outVector)
{
    const auto* object = value.getDynamicObject();
    if (object == nullptr)
        return false;

    outVector.x = static_cast<float>(static_cast<double>(object->getProperty("x")));
    outVector.y = static_cast<float>(static_cast<double>(object->getProperty("y")));
    outVector.z = static_cast<float>(static_cast<double>(object->getProperty("z")));
    return true;
}

juce::String authoringStateToken(EngineerSceneModel::AuthoringState state)
{
    switch (state)
    {
        case EngineerSceneModel::AuthoringState::modifierStack: return "modifierStack";
        case EngineerSceneModel::AuthoringState::directGeometry: return "directGeometry";
        case EngineerSceneModel::AuthoringState::primitive: break;
    }

    return "primitive";
}

EngineerSceneModel::AuthoringState authoringStateFromToken(const juce::String& token)
{
    if (token.equalsIgnoreCase("modifierStack"))
        return EngineerSceneModel::AuthoringState::modifierStack;
    if (token.equalsIgnoreCase("directGeometry"))
        return EngineerSceneModel::AuthoringState::directGeometry;

    return EngineerSceneModel::AuthoringState::primitive;
}

juce::String cameraPresetToken(EngineerSceneModel::CameraPreset preset)
{
    switch (preset)
    {
        case EngineerSceneModel::CameraPreset::top: return "top";
        case EngineerSceneModel::CameraPreset::walk: return "walk";
        case EngineerSceneModel::CameraPreset::iso: break;
    }

    return "iso";
}

EngineerSceneModel::CameraPreset cameraPresetFromToken(const juce::String& token)
{
    if (token.equalsIgnoreCase("top"))
        return EngineerSceneModel::CameraPreset::top;
    if (token.equalsIgnoreCase("walk"))
        return EngineerSceneModel::CameraPreset::walk;

    return EngineerSceneModel::CameraPreset::iso;
}

juce::var modifierToVar(const EngineerSceneModel::ModifierEntry& modifier)
{
    auto* object = new juce::DynamicObject();
    object->setProperty("type", modifier.type);
    object->setProperty("enabled", modifier.enabled);
    object->setProperty("amount", modifier.amount);
    return juce::var(object);
}

bool modifierFromVar(const juce::var& value, EngineerSceneModel::ModifierEntry& outModifier)
{
    const auto* object = value.getDynamicObject();
    if (object == nullptr)
        return false;

    outModifier.type = object->getProperty("type").toString();
    outModifier.enabled = static_cast<bool>(object->getProperty("enabled"));
    outModifier.amount = static_cast<float>(static_cast<double>(object->getProperty("amount")));
    return true;
}

juce::var libraryPartToVar(const EngineerSceneModel::LibraryPartState& part)
{
    auto* object = new juce::DynamicObject();
    object->setProperty("profileId", part.profileId);
    object->setProperty("materialId", part.materialId);
    object->setProperty("lengthMeters", part.lengthMeters);
    object->setProperty("baked", part.baked);
    return juce::var(object);
}

bool libraryPartFromVar(const juce::var& value, EngineerSceneModel::LibraryPartState& outPart)
{
    const auto* object = value.getDynamicObject();
    if (object == nullptr)
        return false;

    outPart.profileId = object->getProperty("profileId").toString();
    outPart.materialId = object->getProperty("materialId").toString();
    outPart.lengthMeters = static_cast<float>(static_cast<double>(object->getProperty("lengthMeters")));
    outPart.baked = static_cast<bool>(object->getProperty("baked"));
    return true;
}

juce::var sceneObjectToVar(const EngineerSceneModel::SceneObject& object)
{
    auto* root = new juce::DynamicObject();
    root->setProperty("name", object.name);
    root->setProperty("primitiveType", object.primitiveType);
    root->setProperty("position", vector3ToVar(object.position));
    root->setProperty("size", vector3ToVar(object.size));
    root->setProperty("mirrorXEnabled", object.mirrorXEnabled);
    root->setProperty("geometryDepth", object.geometryDepth);
    root->setProperty("bevelAmount", object.bevelAmount);
    root->setProperty("authoringState", authoringStateToken(object.authoringState));
    root->setProperty("objectId", object.objectId);
    root->setProperty("connectorId", object.connectorId);
    root->setProperty("customPartId", object.customPartId);
    root->setProperty("layerId", object.layerId);
    root->setProperty("selectedVertexIndex", object.selectedVertexIndex);
    root->setProperty("rotation", vector3ToVar(object.rotationDegrees));
    root->setProperty("mountedOnObjectId", object.mountedOnObjectId);

    juce::Array<juce::var> modifiersVar;
    for (const auto& modifier : object.modifiers)
        modifiersVar.add(modifierToVar(modifier));
    root->setProperty("modifiers", modifiersVar);

    root->setProperty("libraryPart", object.libraryPart.has_value() ? libraryPartToVar(*object.libraryPart) : juce::var());

    juce::Array<juce::var> verticesVar;
    for (const auto& vertex : object.editableVertices)
        verticesVar.add(vector3ToVar(vertex));
    root->setProperty("editableVertices", verticesVar);

    return juce::var(root);
}

bool sceneObjectFromVar(const juce::var& value, EngineerSceneModel::SceneObject& outObject)
{
    const auto* root = value.getDynamicObject();
    if (root == nullptr)
        return false;

    outObject.name = root->getProperty("name").toString();
    outObject.primitiveType = root->getProperty("primitiveType").toString();
    vector3FromVar(root->getProperty("position"), outObject.position);
    vector3FromVar(root->getProperty("size"), outObject.size);
    outObject.mirrorXEnabled = static_cast<bool>(root->getProperty("mirrorXEnabled"));
    outObject.geometryDepth = static_cast<float>(static_cast<double>(root->getProperty("geometryDepth")));
    outObject.bevelAmount = static_cast<float>(static_cast<double>(root->getProperty("bevelAmount")));
    outObject.authoringState = authoringStateFromToken(root->getProperty("authoringState").toString());
    outObject.objectId = static_cast<int>(root->getProperty("objectId"));
    outObject.connectorId = root->getProperty("connectorId").toString();
    outObject.customPartId = root->getProperty("customPartId").toString();
    outObject.layerId = root->getProperty("layerId").toString();
    if (outObject.layerId.isEmpty())
        outObject.layerId = "layer:default";
    outObject.selectedVertexIndex = static_cast<int>(root->getProperty("selectedVertexIndex"));
    // Missing on a schema-1 file -- vector3FromVar returns false and leaves
    // rotationDegrees at its default-constructed {0,0,0}, exactly the
    // desired "old files have no rotation" behavior.
    vector3FromVar(root->getProperty("rotation"), outObject.rotationDegrees);

    // Deliberately NOT the bare-cast pattern selectedVertexIndex uses above
    // -- a missing property there defaults to 0 harmlessly, but 0 is a real,
    // legitimate objectId here, so a schema-<3 file (which never wrote this
    // property at all) must explicitly default to the -1 "unmounted"
    // sentinel, not silently read as "mounted on object 0."
    if (const auto mountedVar = root->getProperty("mountedOnObjectId"); !mountedVar.isVoid())
        outObject.mountedOnObjectId = static_cast<int>(mountedVar);
    else
        outObject.mountedOnObjectId = -1;

    outObject.modifiers.clear();
    if (const auto* modifiersArray = root->getProperty("modifiers").getArray())
        for (const auto& entry : *modifiersArray)
        {
            EngineerSceneModel::ModifierEntry modifier;
            if (modifierFromVar(entry, modifier))
                outObject.modifiers.push_back(modifier);
        }

    outObject.libraryPart.reset();
    if (const auto libraryPartVar = root->getProperty("libraryPart"); !libraryPartVar.isVoid())
    {
        EngineerSceneModel::LibraryPartState part;
        if (libraryPartFromVar(libraryPartVar, part))
            outObject.libraryPart = part;
    }

    outObject.editableVertices.clear();
    if (const auto* verticesArray = root->getProperty("editableVertices").getArray())
        for (const auto& entry : *verticesArray)
        {
            juce::Vector3D<float> vertex;
            if (vector3FromVar(entry, vertex))
                outObject.editableVertices.push_back(vertex);
        }

    return true;
}

juce::var layerToVar(const EngineerSceneModel::Layer& layer)
{
    auto* object = new juce::DynamicObject();
    object->setProperty("id", layer.id);
    object->setProperty("name", layer.name);
    object->setProperty("visible", layer.visible);
    object->setProperty("tint", layer.tint.toString());
    object->setProperty("opacity", layer.opacity);
    return juce::var(object);
}

bool layerFromVar(const juce::var& value, EngineerSceneModel::Layer& outLayer)
{
    const auto* object = value.getDynamicObject();
    if (object == nullptr)
        return false;

    outLayer.id = object->getProperty("id").toString();
    outLayer.name = object->getProperty("name").toString();
    outLayer.visible = static_cast<bool>(object->getProperty("visible"));
    outLayer.tint = juce::Colour::fromString(object->getProperty("tint").toString());
    outLayer.opacity = static_cast<float>(static_cast<double>(object->getProperty("opacity")));
    return true;
}
}

namespace EngineerSceneSerialization
{
juce::var toVar(const EngineerSceneModel& model)
{
    auto* root = new juce::DynamicObject();
    root->setProperty("schemaVersion", kSchemaVersion);
    root->setProperty("selectedObjectIndex", model.getSelectedObjectIndex());
    root->setProperty("nextObjectId", model.getNextObjectIdForSerialization());
    root->setProperty("cameraPreset", cameraPresetToken(model.getCameraPreset()));
    root->setProperty("cursorPosition", vector3ToVar(model.getCursorPosition()));

    juce::Array<juce::var> objectsVar;
    for (const auto& object : model.getObjects())
        objectsVar.add(sceneObjectToVar(object));
    root->setProperty("objects", objectsVar);

    juce::Array<juce::var> layersVar;
    for (const auto& layer : model.getLayers())
        layersVar.add(layerToVar(layer));
    root->setProperty("layers", layersVar);

    return juce::var(root);
}

bool fromVar(const juce::var& value, EngineerSceneModel& model, juce::String& errorMessage)
{
    const auto* root = value.getDynamicObject();
    if (root == nullptr)
    {
        errorMessage = "Drawing data is not a JSON object.";
        return false;
    }

    const auto schemaVersion = static_cast<int>(root->getProperty("schemaVersion"));
    if (schemaVersion > kSchemaVersion)
    {
        errorMessage = "This drawing was saved by a newer version of Creation Engineer (schema "
                      + juce::String(schemaVersion) + ", this build understands up to " + juce::String(kSchemaVersion) + ").";
        return false;
    }

    std::vector<EngineerSceneModel::SceneObject> objects;
    if (const auto* objectsArray = root->getProperty("objects").getArray())
        for (const auto& entry : *objectsArray)
        {
            EngineerSceneModel::SceneObject object;
            if (sceneObjectFromVar(entry, object))
                objects.push_back(std::move(object));
        }

    std::vector<EngineerSceneModel::Layer> layers;
    if (const auto* layersArray = root->getProperty("layers").getArray())
        for (const auto& entry : *layersArray)
        {
            EngineerSceneModel::Layer layer;
            if (layerFromVar(entry, layer))
                layers.push_back(layer);
        }

    const auto selectedObjectIndex = static_cast<int>(root->getProperty("selectedObjectIndex"));
    const auto nextObjectId = static_cast<int>(root->getProperty("nextObjectId"));
    const auto cameraPreset = cameraPresetFromToken(root->getProperty("cameraPreset").toString());

    juce::Vector3D<float> cursorPosition; // missing on a schema-1 file -> stays {0,0,0}
    vector3FromVar(root->getProperty("cursorPosition"), cursorPosition);

    model.loadDrawing(std::move(objects), std::move(layers), selectedObjectIndex, nextObjectId, cameraPreset,
                      cursorPosition);
    return true;
}
}
