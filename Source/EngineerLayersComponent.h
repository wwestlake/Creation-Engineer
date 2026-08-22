#pragma once

#include <JuceHeader.h>

#include "EngineerSceneModel.h"

// Browse/edit the drawing's layers: name, visibility, colour tint, opacity.
// Each row is a real interactive juce::Component (via refreshComponentForRow),
// not painted controls -- see EngineerLayersComponent.cpp's anonymous-
// namespace LayerRowComponent.
class EngineerLayersComponent final : public juce::Component,
                                      private EngineerSceneModel::Listener,
                                      private juce::ListBoxModel
{
public:
    explicit EngineerLayersComponent(EngineerSceneModel& sceneModel);
    ~EngineerLayersComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void engineerSceneModelChanged() override;

    int getNumRows() override;
    void paintListBoxItem(int, juce::Graphics&, int, int, bool) override {}
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected,
                                            juce::Component* existingComponentToUpdate) override;

    void addLayerClicked();

    EngineerSceneModel& sceneModel;
    juce::Label titleLabel;
    juce::ListBox layerList;
    juce::TextButton addLayerButton { "Add Layer" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerLayersComponent)
};
