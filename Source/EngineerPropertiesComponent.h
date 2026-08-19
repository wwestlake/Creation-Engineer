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

    EngineerSceneModel& sceneModel;

    juce::Label titleLabel;
    juce::Label hintLabel;
    juce::Label nameLabel;
    juce::Label primitiveLabel;
    juce::Label positionLabel;
    juce::Label sizeLabel;
    juce::Label authoringStateLabel;
    juce::Label authoringStateValue;
    juce::Label modifierLabel;
    juce::TextEditor nameEditor;
    juce::ComboBox primitiveBox;
    juce::TextEditor posXEditor;
    juce::TextEditor posYEditor;
    juce::TextEditor sizeXEditor;
    juce::TextEditor sizeYEditor;
    juce::ToggleButton mirrorXToggle { "Mirror Across X" };
    juce::TextButton commitGeometryButton { "Commit To Geometry" };
    juce::TextButton restorePrimitiveButton { "Restore Primitive" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerPropertiesComponent)
};
