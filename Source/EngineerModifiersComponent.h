#pragma once

#include <JuceHeader.h>
#include "EngineerSceneModel.h"

class EngineerModifiersComponent final : public juce::Component,
                                         private juce::ListBoxModel,
                                         private EngineerSceneModel::Listener
{
public:
    explicit EngineerModifiersComponent(EngineerSceneModel& sceneModel);
    ~EngineerModifiersComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    int getNumRows() override;
    void paintListBoxItem(int rowNumber,
                          juce::Graphics& g,
                          int width,
                          int height,
                          bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;
    void engineerSceneModelChanged() override;
    void addMirrorModifier();
    void addShellModifier();
    void toggleSelectedModifierEnabled();
    void moveSelectedModifierUp();
    void moveSelectedModifierDown();
    void removeSelectedModifier();
    void refreshControls();

    EngineerSceneModel& sceneModel;
    int selectedModifierIndex = -1;
    juce::Label titleLabel;
    juce::Label detailLabel;
    juce::ListBox modifierList;
    juce::TextButton addMirrorButton { "+ Mirror" };
    juce::TextButton addShellButton { "+ Shell" };
    juce::TextButton toggleEnabledButton { "Enable / Disable" };
    juce::TextButton moveUpButton { "Move Up" };
    juce::TextButton moveDownButton { "Move Down" };
    juce::TextButton removeButton { "Remove" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerModifiersComponent)
};
