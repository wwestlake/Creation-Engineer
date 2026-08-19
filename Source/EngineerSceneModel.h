#pragma once

#include <JuceHeader.h>
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

    struct SceneObject
    {
        juce::String name;
        juce::String primitiveType;
        juce::Point<float> normalizedPosition;
        juce::Point<float> normalizedSize;
        bool mirrorXEnabled = false;
        std::vector<ModifierEntry> modifiers;
        float geometryDepth = 0.0f;
        float bevelAmount = 0.0f;
        AuthoringState authoringState = AuthoringState::primitive;
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
    void setSelectedObjectPosition(juce::Point<float> position);
    void setSelectedObjectSize(juce::Point<float> size);
    void setSelectedObjectMirrorXEnabled(bool enabled);
    bool isSelectedObjectMirrorXEnabled() const noexcept;
    AuthoringState getSelectedObjectAuthoringState() const noexcept;
    const std::vector<ModifierEntry>& getSelectedObjectModifiers() const noexcept;
    void addPrimitiveObject(const juce::String& primitiveType);
    void duplicateSelectedObject();
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
    int selectedObjectIndex = 0;
    GeometryTool selectedGeometryTool = GeometryTool::translate;
    bool geometrySnappingEnabled = true;
    float geometrySnapStep = 0.01f;
    GeometryElementKind selectedGeometryElementKind = GeometryElementKind::vertex;
    int selectedGeometryElementIndex = 0;
    CameraPreset cameraPreset = CameraPreset::iso;
    juce::ListenerList<Listener> listeners;
};
