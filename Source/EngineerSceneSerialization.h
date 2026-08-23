#pragma once

#include <JuceHeader.h>

#include "EngineerSceneModel.h"

// Serializes the whole drawing (objects + layers + selection + camera
// preset) to/from a single juce::var tree, following the toVar/fromVar +
// per-record juce::DynamicObject convention already used by
// shared/AssetSystem's AssetDescriptor and shared/EngineeringSpecs. Free
// functions operating only on EngineerSceneModel's public API (see
// loadDrawing) so the model itself still controls every state change.
namespace EngineerSceneSerialization
{
// v2 (Phase 4) adds SceneObject::rotationDegrees/customPartId and the
// drawing-root cursorPosition -- all three default cleanly when absent (a
// missing juce::var property reads as 0/empty), so schema-1 files load with
// no explicit migration code, just this version bump.
// v3 (Phase 5) adds SceneObject::mountedOnObjectId -- unlike every other
// field added in the v1->v2 bump, this one must NOT rely on "missing var
// reads as 0" (0 is a real, legitimate objectId); see sceneObjectFromVar's
// explicit isVoid() check.
constexpr int kSchemaVersion = 3;

juce::var toVar(const EngineerSceneModel& model);

// Returns false (leaving model untouched) if the var isn't a recognizable
// drawing, or its schemaVersion is newer than kSchemaVersion -- this build
// doesn't know how to read it.
bool fromVar(const juce::var& value, EngineerSceneModel& model, juce::String& errorMessage);
}
