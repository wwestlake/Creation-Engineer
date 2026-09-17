#pragma once

#include <JuceHeader.h>
#include "EngineerSceneModel.h"

class EngineerNavigatorComponent final : public juce::Component,
                                         private juce::ListBoxModel,
                                         private EngineerSceneModel::Listener
{
public:
    explicit EngineerNavigatorComponent(EngineerSceneModel& sceneModel);
    ~EngineerNavigatorComponent() override;

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
    void addPrimitive(const juce::String& primitiveType);
    void duplicateSelection();
    void removeSelection();

    EngineerSceneModel& sceneModel;
    juce::Label titleLabel;
    juce::Label detailLabel;
    juce::ListBox objectList;
    juce::TextButton addBlockButton { "+ Block" };
    juce::TextButton addCylinderButton { "+ Cylinder" };
    juce::TextButton addPlateButton { "+ Plate" };
    juce::TextButton duplicateButton { "Duplicate" };
    juce::TextButton removeButton { "Delete" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerNavigatorComponent)
};
