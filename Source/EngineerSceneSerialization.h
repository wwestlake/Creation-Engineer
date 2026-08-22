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
constexpr int kSchemaVersion = 1;

juce::var toVar(const EngineerSceneModel& model);

// Returns false (leaving model untouched) if the var isn't a recognizable
// drawing, or its schemaVersion is newer than kSchemaVersion -- this build
// doesn't know how to read it.
bool fromVar(const juce::var& value, EngineerSceneModel& model, juce::String& errorMessage);
}
