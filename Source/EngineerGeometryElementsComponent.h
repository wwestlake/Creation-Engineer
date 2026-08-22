#pragma once

#include <JuceHeader.h>
#include "EngineerSceneModel.h"

// Object Mode / Vertex Edit Mode toggle (see EngineerSceneModel::EditMode)
// plus, in Vertex mode, navigation and a real-unit position readout/editor
// for the selected object's currently-selected vertex. Replaces the old
// fake GeometryElementKind vertex/edge/face proxy system entirely -- see
// the drawing/layers/vertex-editing plan's Part K.
class EngineerGeometryElementsComponent final : public juce::Component,
                                                private EngineerSceneModel::Listener
{
public:
    explicit EngineerGeometryElementsComponent(EngineerSceneModel& sceneModel);
    ~EngineerGeometryElementsComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void engineerSceneModelChanged() override;
    void refreshFromScene();
    void setObjectMode();
    void setVertexMode();
    void previousVertex();
    void nextVertex();
    void commitVertexPosition();

    EngineerSceneModel& sceneModel;
    juce::Label titleLabel;
    juce::Label detailLabel;
    juce::Label selectionLabel;
    juce::TextButton objectModeButton { "Object" };
    juce::TextButton vertexModeButton { "Vertex" };
    juce::TextButton previousButton { "Previous" };
    juce::TextButton nextButton { "Next" };

    juce::Label vertexPositionLabel;
    juce::TextEditor vertexXEditor;
    juce::TextEditor vertexYEditor;
    juce::TextEditor vertexZEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerGeometryElementsComponent)
};
