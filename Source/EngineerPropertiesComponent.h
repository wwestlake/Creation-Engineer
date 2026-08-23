#pragma once

#include <JuceHeader.h>
#include "EngineerSceneModel.h"

class EngineerPropertiesComponent final : public juce::Component,
                                          private EngineerSceneModel::Listener
{
public:
    explicit EngineerPropertiesComponent(EngineerSceneModel& sceneModel);
    ~EngineerPropertiesComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void engineerSceneModelChanged() override;
    void refreshFromScene();
    void commitName();
    void commitPrimitiveType();
    void commitPosition();
    void commitSize();
    void commitMirrorState();
    void commitDirectGeometry();
    void restorePrimitiveWorkflow();
    void commitLibraryPartLength();
    void commitLayerSelection();
    void commitRotation();
    void commitCursorPosition();

    EngineerSceneModel& sceneModel;

    // Scene-level (not per-object) placement cursor readout -- always
    // visible regardless of selection, unlike everything else in this panel.
    juce::Label cursorLabel;
    juce::TextEditor cursorXEditor;
    juce::TextEditor cursorYEditor;
    juce::TextEditor cursorZEditor;

    juce::Label titleLabel;
    juce::Label hintLabel;
    juce::Label nameLabel;
    juce::Label primitiveLabel;
    juce::Label positionLabel;
    juce::Label sizeLabel;
    juce::Label rotationLabel;
    juce::Label authoringStateLabel;
    juce::Label authoringStateValue;
    juce::Label modifierLabel;
    juce::Label layerLabel;
    juce::ComboBox layerBox;
    juce::TextEditor nameEditor;
    juce::ComboBox primitiveBox;
    juce::TextEditor posXEditor;
    juce::TextEditor posYEditor;
    juce::TextEditor posZEditor;
    juce::TextEditor sizeXEditor;
    juce::TextEditor sizeYEditor;
    juce::TextEditor sizeZEditor;
    // Hidden/disabled for directGeometry (vertex-cage) objects -- see
    // EngineerSceneModel::isSelectedObjectRotatable's doc comment.
    juce::TextEditor rotXEditor;
    juce::TextEditor rotYEditor;
    juce::TextEditor rotZEditor;
    juce::ToggleButton mirrorXToggle { "Mirror Across X" };
    juce::TextButton commitGeometryButton { "Commit To Geometry" };
    juce::TextButton restorePrimitiveButton { "Restore Primitive" };

    juce::Label libraryPartLabel;
    juce::TextEditor libraryLengthEditor;
    juce::TextButton bakeButton { "Bake" };
    juce::TextButton unbakeButton { "Un-bake" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerPropertiesComponent)
};
