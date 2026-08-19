#pragma once

#include <JuceHeader.h>
#include "EngineerSceneModel.h"

class EngineerGeometryToolsComponent final : public juce::Component,
                                             private EngineerSceneModel::Listener
{
public:
    explicit EngineerGeometryToolsComponent(EngineerSceneModel& sceneModel);
    ~EngineerGeometryToolsComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void engineerSceneModelChanged() override;
    void refreshFromScene();
    void selectTool(EngineerSceneModel::GeometryTool tool);
    void nudgeLeft();
    void nudgeRight();
    void nudgeUp();
    void nudgeDown();
    void scaleOut();
    void scaleIn();
    void extrudeMore();
    void extrudeLess();
    void bevelMore();
    void bevelLess();

    EngineerSceneModel& sceneModel;
    juce::Label titleLabel;
    juce::Label detailLabel;
    juce::Label modeLabel;
    juce::Label depthLabel;
    juce::Label bevelLabel;
    juce::TextButton translateButton { "Translate" };
    juce::TextButton scaleButton { "Scale" };
    juce::TextButton extrudeButton { "Extrude" };
    juce::TextButton bevelButton { "Bevel" };
    juce::TextButton leftButton { "Left" };
    juce::TextButton rightButton { "Right" };
    juce::TextButton upButton { "Up" };
    juce::TextButton downButton { "Down" };
    juce::TextButton scaleOutButton { "Scale +" };
    juce::TextButton scaleInButton { "Scale -" };
    juce::TextButton extrudeMoreButton { "Extrude +" };
    juce::TextButton extrudeLessButton { "Extrude -" };
    juce::TextButton bevelMoreButton { "Bevel +" };
    juce::TextButton bevelLessButton { "Bevel -" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerGeometryToolsComponent)
};
