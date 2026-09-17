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

    // Object Mode: select/move whole objects (today's default behavior).
    // Vertex Mode: select/drag individual vertices of the selected object's
    // real editable vertex cage (only meaningful for directGeometry objects
    // -- see SceneObject::editableVertices). Sketch Mode: place boundary
    // points/holes for an in-progress ActiveSketch (see beginSketch). One
    // mode applies across all three viewports at once, not per-viewport.
    enum class EditMode
    {
        object,
        vertex,
        sketch
    };

    // One of the 3 principal planes through an ActiveSketch's origin point
    // -- a face-picked arbitrary plane is future work (see the sketch-
    // modeling plan's Part O). Purely a click-to-(u,v) mapping choice; the
    // generated solid is always built in a fixed local (X=u,Y=v,Z=thickness)
    // frame regardless of which plane was used to author it -- see
    // ce::GenerateExtrudedPolygonWithHoles's own doc comment.
    enum class SketchPlane
    {
        xy,
        xz,
        yz
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

    // A drawing-organization layer -- visibility/tint/opacity only, not
    // texturing (this is an engineering tool, not a game/image editor). Every
    // SceneObject belongs to exactly one layer via SceneObject::layerId;
    // "layer:default" always exists and can't be removed.
    struct Layer
    {
        juce::String id;
        juce::String name;
        bool visible = true;
        juce::Colour tint = juce::Colours::white;
        float opacity = 1.0f;
    };

    struct SketchHole
    {
        juce::Point<float> centerUV;   // meters, in the sketch plane's own 2D frame
        float diameterMeters = 0.005f; // 5mm default, immediately editable
    };

    // In-progress sketch state (Sketch Mode) -- lives here rather than on
    // EngineerViewportComponent because a user sketching in one docked
    // viewport instance needs their points visible identically if another
    // instance is docked open too, same reasoning as EditMode/cursorPosition_
    // itself. Boundary is straight-line-segments only (closed implicitly,
    // last point connects to first); holes are circular only -- arcs/
    // splines/non-circular holes are future work.
    struct ActiveSketch
    {
        bool active = false;
        SketchPlane plane = SketchPlane::xz;
        juce::Vector3D<float> origin;                   // world-space plane origin, snapshotted from cursorPosition_ at beginSketch()
        std::vector<juce::Point<float>> boundaryPoints;  // ordered u,v meters
        std::vector<SketchHole> holes;
        float thicknessMeters = 0.003f;                  // 3mm default
        bool holePlacementActive = false;                // toggles what a sketch-mode viewport click adds
        int selectedBoundaryPointIndex = -1;
        int selectedHoleIndex = -1;
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
        juce::String layerId { "layer:default" };

        // Real editable vertex cage -- populated only when authoringState ==
        // directGeometry (seeded by commitSelectedObjectToDirectGeometry,
        // cleared by restoreSelectedObjectPrimitiveWorkflow). Always exactly
        // 8 corners in a fixed binary-index order (bit0=+X, bit1=+Y, bit2=+Z)
        // that shared/Render's BuildFlatShadedMeshFromCage expects -- a real,
        // directly user-edited replacement for the old GeometryElementKind
        // proxy system.
        std::vector<juce::Vector3D<float>> editableVertices;
        int selectedVertexIndex = -1;

        // Euler XYZ, degrees. Inert (see isSelectedObjectRotatable) once
        // editableVertices is non-empty -- a directGeometry object's
        // orientation always comes from its vertex cage, never this field,
        // so the two can never disagree about which way the object faces.
        juce::Vector3D<float> rotationDegrees;
        juce::String customPartId; // non-empty only when primitiveType == "CustomPart"

        // objectId (SceneObject::objectId) of the DIN rail this object is
        // mounted on, or -1 if not mounted on anything -- same "-1 sentinel
        // for none" convention as selectedVertexIndex above. Set once at
        // placement time by addConnectorObjectMountedOnRail and never
        // updated afterward: if the rail is later moved/rotated, mounted
        // modules do NOT follow it (no parent-child transform propagation).
        int mountedOnObjectId = -1;
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
    // No-op when the selected object has a real editable vertex cage
    // (editableVertices non-empty) -- see isSelectedObjectRotatable's doc
    // comment and SceneObject::rotationDegrees.
    void setSelectedObjectRotationDegrees(juce::Vector3D<float> rotationDegrees);
    juce::Vector3D<float> getSelectedObjectRotationDegrees() const noexcept;
    // False once the selected object has a real editable vertex cage --
    // directGeometry objects always orient via their cage, never via
    // rotationDegrees, so the rotation UI/field is gated off for them
    // rather than risk two disagreeing sources of orientation truth.
    bool isSelectedObjectRotatable() const noexcept;
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

    // Same shape as addConnectorObject, except position/rotationDegrees are
    // supplied explicitly by the caller (EngineerLibraryComponent, via
    // snapSelectedConnectorToSelectedRail) rather than derived from
    // cursorPosition_ -- the whole point of snap placement is that position
    // is computed from the target rail + already-mounted modules, not the
    // cursor. mountedOnObjectId is recorded for round-tripping and for later
    // placements to find "what's already on this rail" via getObjects()
    // filtering; EngineerSceneModel itself never looks up a rail's
    // ProfileSpec or interprets mountedOnObjectId beyond storing it -- stays
    // decoupled from shared/EngineeringSpecs, same rule addLibraryPartObject
    // already follows.
    void addConnectorObjectMountedOnRail(const juce::String& connectorId, juce::Vector3D<float> size,
                                         int mountedOnObjectId, juce::Vector3D<float> position,
                                         juce::Vector3D<float> rotationDegrees);

    // size is computed by the caller from the CustomPartSpec's own boundary
    // extents/thickness -- same decoupling-from-shared/EngineeringSpecs
    // rationale as addLibraryPartObject/addConnectorObject above.
    void addCustomPartObject(const juce::String& customPartId, juce::Vector3D<float> size);

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
    EditMode getEditMode() const noexcept;
    void setEditMode(EditMode mode) noexcept;
    bool isSelectedObjectVertexEditable() const noexcept;
    int getSelectedVertexIndex() const noexcept;
    int getSelectedVertexCount() const noexcept;
    void selectVertex(int vertexIndex) noexcept;
    void selectPreviousVertex() noexcept;
    void selectNextVertex() noexcept;
    juce::Vector3D<float> getSelectedVertexPosition() const noexcept;
    void setSelectedVertexPosition(juce::Vector3D<float> worldPosition);
    // Scene-level (one per drawing, not per-object) placement cursor -- the
    // insertion point every add*Object entry point spawns at. See EngineerViewportComponent::mouseDown for how a viewport click places it.
    juce::Vector3D<float> getCursorPosition() const noexcept;
    void setCursorPosition(juce::Vector3D<float> position);

    // Sketch Mode (see EditMode::sketch). beginSketch snapshots
    // cursorPosition_ as the new sketch's plane origin. Every exit path
    // (cancelSketch, endSketch, or an unrelated setEditMode call) converges
    // on the same activeSketch_ = ActiveSketch{} reset, so a stale
    // abandoned sketch can never linger.
    void beginSketch(SketchPlane plane);
    void cancelSketch();
    void addSketchBoundaryPoint(juce::Point<float> uv);
    void removeSketchBoundaryPoint(int index);
    void setSketchBoundaryPointPosition(int index, juce::Point<float> uv);
    void setSketchHolePlacementActive(bool active);
    void addSketchHole(juce::Point<float> centerUV, float diameterMeters);
    void removeSketchHole(int index);
    void setSketchHolePosition(int index, juce::Point<float> centerUV);
    void setSketchHoleDiameter(int index, float diameterMeters);
    void setSketchThickness(float thicknessMeters);
    const ActiveSketch& getActiveSketch() const noexcept;
    bool canFinishActiveSketch() const noexcept; // >= 3 boundary points; self-intersection is not validated
    // Called by EngineerSketchComponent once it has read the final sketch
    // data and created/placed the CustomPartSpec -- this only clears
    // activeSketch_/editMode_, it never touches shared/EngineeringSpecs
    // itself (see addLibraryPartObject's decoupling comment).
    void endSketch();

    // Shared by viewport click-mapping and EngineerSketchComponent so
    // there's exactly one implementation of "how does a plane choice turn a
    // world point into (u,v)" -- static, no instance state needed.
    static juce::Vector3D<float> sketchPlaneToWorld(SketchPlane plane, juce::Vector3D<float> origin,
                                                     juce::Point<float> uv);
    static juce::Point<float> worldToSketchPlane(SketchPlane plane, juce::Vector3D<float> origin,
                                                  juce::Vector3D<float> world);

    void nudgeSelectedDirectGeometryPosition(juce::Point<float> delta);
    void scaleSelectedDirectGeometry(juce::Point<float> delta);
    void extrudeSelectedDirectGeometry(float amount);
    void bevelSelectedDirectGeometry(float amount);
    static juce::String toDisplayString(AuthoringState state);
    static juce::String toDisplayString(const ModifierEntry& modifier);
    static juce::String toDisplayString(GeometryTool tool);

    CameraPreset getCameraPreset() const noexcept;
    void setCameraPreset(CameraPreset preset) noexcept;

    const std::vector<Layer>& getLayers() const noexcept;
    void addLayer(const juce::String& name);
    bool canRemoveLayer(const juce::String& layerId) const noexcept;
    void removeLayer(const juce::String& layerId);
    void renameLayer(const juce::String& layerId, const juce::String& name);
    void setLayerVisible(const juce::String& layerId, bool visible);
    void setLayerTint(const juce::String& layerId, juce::Colour tint);
    void setLayerOpacity(const juce::String& layerId, float opacity);
    void moveObjectToLayer(int objectIndex, const juce::String& layerId);
    const Layer* findLayer(const juce::String& layerId) const noexcept;

    // Serialization support (see EngineerSceneSerialization.h/.cpp) --
    // exposed as a bulk mutation so toVar/fromVar stay plain free functions
    // operating only on this public API, matching the shared/EngineeringSpecs
    // toVar/fromVar convention, while EngineerSceneModel itself still fully
    // controls when/how its state changes and always notifies listeners
    // exactly once per load.
    int getNextObjectIdForSerialization() const noexcept;
    void loadDrawing(std::vector<SceneObject> loadedObjects, std::vector<Layer> loadedLayers,
                     int loadedSelectedObjectIndex, int loadedNextObjectId, CameraPreset loadedCameraPreset,
                     juce::Vector3D<float> loadedCursorPosition = {});

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
    EditMode editMode_ = EditMode::object;
    CameraPreset cameraPreset = CameraPreset::iso;
    std::vector<Layer> layers;
    juce::Vector3D<float> cursorPosition_ { 0.0f, 0.0f, 0.0f };
    ActiveSketch activeSketch_;
    juce::ListenerList<Listener> listeners;
};
