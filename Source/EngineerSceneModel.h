#pragma once

#include <JuceHeader.h>
#include <optional>
#include <vector>

class EngineerSceneModel final
{
public:
    struct ModifierEntry
    {
        juce::String type;
        bool enabled = true;
        float amount = 0.0f;
    };

    enum class GeometryTool
    {
        translate,
        scale,
        extrude,
        bevel
    };

    enum class GeometryElementKind
    {
        vertex,
        edge,
        face
    };

    enum class AuthoringState
    {
        primitive,
        modifierStack,
        directGeometry
    };

    // Payload for a scene object backed by a shared/EngineeringSpecs library
    // profile -- a parallel authoring path alongside AuthoringState's
    // primitive/modifierStack/directGeometry, not a variant of it.
    struct LibraryPartState
    {
        juce::String profileId;
        juce::String materialId;
        float lengthMeters = 0.1f;
        bool baked = false;
        // profileId/materialId/lengthMeters are retained even when baked ==
        // true, so unbakeSelectedLibraryPart() can regenerate an identical
        // mesh with no data loss/re-prompt -- see bakeSelectedLibraryPart's
        // implementation comment.
    };

    // Real 3D, Y-up, scene units (meters) -- matches shared/Render's Camera/
    // FreeCamera convention exactly, so viewport code never needs to convert.
    // position = object center; size = full extents (X/Y/Z), direct scale
    // factors for a unit box/cylinder mesh (or, for a LibraryPart/Connector
    // object, the already-real-sized extents its generated mesh occupies).
    // objectId/libraryPart/connectorId are appended AFTER authoringState
    // (rather than at the front) deliberately -- makeSceneObject's positional
    // aggregate-init (EngineerSceneModel.cpp) still provides exactly the
    // original 9 values (name..authoringState); putting new fields after
    // those lets C++'s aggregate-init rule for "fewer initializers than
    // members" default-initialize the new trailing fields, instead of
    // silently shifting every existing positional value onto the wrong
    // member.
    struct SceneObject
    {
        juce::String name;
        juce::String primitiveType;
        juce::Vector3D<float> position;
        juce::Vector3D<float> size;
        bool mirrorXEnabled = false;
        std::vector<ModifierEntry> modifiers;
        float geometryDepth = 0.0f;
        float bevelAmount = 0.0f;
        AuthoringState authoringState = AuthoringState::primitive;
        int objectId = 0;
        std::optional<LibraryPartState> libraryPart;
        juce::String connectorId; // non-empty only when primitiveType == "Connector"
    };

    enum class CameraPreset
    {
        iso,
        top,
        walk
    };

    struct Listener
    {
        virtual ~Listener() = default;
        virtual void engineerSceneModelChanged() = 0;
    };

    EngineerSceneModel();

    const std::vector<SceneObject>& getObjects() const noexcept;
    int getSelectedObjectIndex() const noexcept;
    const SceneObject& getSelectedObject() const noexcept;
    void selectObject(int index) noexcept;
    void setSelectedObjectName(const juce::String& name);
    void setSelectedObjectPrimitiveType(const juce::String& primitiveType);
    void setSelectedObjectPosition(juce::Vector3D<float> position);
    void setSelectedObjectSize(juce::Vector3D<float> size);
    void setSelectedObjectMirrorXEnabled(bool enabled);
    bool isSelectedObjectMirrorXEnabled() const noexcept;
    AuthoringState getSelectedObjectAuthoringState() const noexcept;
    const std::vector<ModifierEntry>& getSelectedObjectModifiers() const noexcept;
    void addPrimitiveObject(const juce::String& primitiveType);
    void duplicateSelectedObject();

    // initialSize is computed by the caller (EngineerLibraryComponent, which
    // has the ProfileSpec) rather than looked up here, so the scene model
    // stays decoupled from shared/EngineeringSpecs -- see the parametric-
    // part-libraries plan's Part D.
    void addLibraryPartObject(const juce::String& profileId, const juce::String& materialId,
                              float lengthMeters, juce::Vector3D<float> initialSize);
    void setSelectedLibraryPartLength(float lengthMeters);
    void bakeSelectedLibraryPart();
    void unbakeSelectedLibraryPart();
    bool isSelectedObjectLibraryPart() const noexcept;
    bool isSelectedLibraryPartBaked() const noexcept;

    // size is likewise computed by the caller (from ConnectorSpec) -- v1
    // connectors are placed as-is, not length-parametric or bakeable.
    void addConnectorObject(const juce::String& connectorId, juce::Vector3D<float> size);
    bool canRemoveSelectedObject() const noexcept;
    void removeSelectedObject();
    void addSelectedObjectMirrorModifier();
    void addSelectedObjectShellModifier();
    bool canRemoveSelectedModifier(int index) const noexcept;
    void removeSelectedModifier(int index);
    bool canMoveSelectedModifierUp(int index) const noexcept;
    bool canMoveSelectedModifierDown(int index) const noexcept;
    void moveSelectedModifierUp(int index);
    void moveSelectedModifierDown(int index);
    void setSelectedModifierEnabled(int index, bool enabled);
    void commitSelectedObjectToDirectGeometry();
    void restoreSelectedObjectPrimitiveWorkflow();
    bool isSelectedObjectDirectGeometry() const noexcept;
    GeometryTool getSelectedGeometryTool() const noexcept;
    void setSelectedGeometryTool(GeometryTool tool) noexcept;
    bool isGeometrySnappingEnabled() const noexcept;
    void setGeometrySnappingEnabled(bool enabled) noexcept;
    float getGeometrySnapStep() const noexcept;
    void setGeometrySnapStep(float step) noexcept;
    GeometryElementKind getSelectedGeometryElementKind() const noexcept;
    void setSelectedGeometryElementKind(GeometryElementKind kind) noexcept;
    int getSelectedGeometryElementIndex() const noexcept;
    void setSelectedGeometryElementIndex(int index) noexcept;
    int getSelectedGeometryElementCount() const noexcept;
    void selectPreviousGeometryElement() noexcept;
    void selectNextGeometryElement() noexcept;
    void nudgeSelectedGeometryElement(juce::Point<float> delta);
    void nudgeSelectedDirectGeometryPosition(juce::Point<float> delta);
    void scaleSelectedDirectGeometry(juce::Point<float> delta);
    void extrudeSelectedDirectGeometry(float amount);
    void bevelSelectedDirectGeometry(float amount);
    static juce::String toDisplayString(AuthoringState state);
    static juce::String toDisplayString(const ModifierEntry& modifier);
    static juce::String toDisplayString(GeometryTool tool);
    static juce::String toDisplayString(GeometryElementKind kind);

    CameraPreset getCameraPreset() const noexcept;
    void setCameraPreset(CameraPreset preset) noexcept;

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

private:
    SceneObject& getSelectedObjectMutable() noexcept;
    void syncDerivedState(SceneObject& object) noexcept;
    void clampDirectGeometryObject(SceneObject& object) noexcept;
    juce::Point<float> snapDelta(juce::Point<float> delta) const noexcept;
    float snapScalar(float value) const noexcept;
    void notifyListeners();

    std::vector<SceneObject> objects;
    int nextObjectId_ = 1;
    int selectedObjectIndex = 0;
    GeometryTool selectedGeometryTool = GeometryTool::translate;
    bool geometrySnappingEnabled = true;
    float geometrySnapStep = 0.01f;
    GeometryElementKind selectedGeometryElementKind = GeometryElementKind::vertex;
    int selectedGeometryElementIndex = 0;
    CameraPreset cameraPreset = CameraPreset::iso;
    juce::ListenerList<Listener> listeners;
};
