#pragma once

#include <JuceHeader.h>
#include "EngineerSceneModel.h"

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
    void selectKind(EngineerSceneModel::GeometryElementKind kind);
    void previousElement();
    void nextElement();
    void nudgeLeft();
    void nudgeRight();
    void nudgeUp();
    void nudgeDown();

    EngineerSceneModel& sceneModel;
    juce::Label titleLabel;
    juce::Label detailLabel;
    juce::Label selectionLabel;
    juce::TextButton vertexButton { "Vertex" };
    juce::TextButton edgeButton { "Edge" };
    juce::TextButton faceButton { "Face" };
    juce::TextButton previousButton { "Previous" };
    juce::TextButton nextButton { "Next" };
    juce::TextButton leftButton { "Left" };
    juce::TextButton rightButton { "Right" };
    juce::TextButton upButton { "Up" };
    juce::TextButton downButton { "Down" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerGeometryElementsComponent)
};
