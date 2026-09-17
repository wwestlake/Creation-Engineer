#include "EngineerGeometryToolsComponent.h"

namespace
{
void configureButton(juce::TextButton& button)
{
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a2533));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff25354a));
    button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}
}

EngineerGeometryToolsComponent::EngineerGeometryToolsComponent(EngineerSceneModel& model)
    : sceneModel(model)
{
    sceneModel.addListener(this);

    titleLabel.setText("Direct Geometry Tools", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::Font(18.0f).boldened());
    addAndMakeVisible(titleLabel);

    detailLabel.setText("Committed parts get engineering-friendly direct edit tools instead of stopping at the commit boundary.", juce::dontSendNotification);
    detailLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaebed0));
    addAndMakeVisible(detailLabel);

    modeLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    depthLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaebed0));
    bevelLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaebed0));
    snapLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaebed0));
    addAndMakeVisible(modeLabel);
    addAndMakeVisible(depthLabel);
    addAndMakeVisible(bevelLabel);
    addAndMakeVisible(snapLabel);

    auto wireButton = [this](juce::TextButton& button)
    {
        configureButton(button);
        addAndMakeVisible(button);
    };

    wireButton(translateButton);
    wireButton(scaleButton);
    wireButton(extrudeButton);
    wireButton(bevelButton);

    snapToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    snapToggle.onClick = [this] { toggleSnapping(); };
    addAndMakeVisible(snapToggle);

    snapStepBox.addItem("2.5 mm", 1);
    snapStepBox.addItem("5 mm", 2);
    snapStepBox.addItem("10 mm", 3);
    snapStepBox.addItem("25 mm", 4);
    snapStepBox.onChange = [this] { setSnapStep(); };
    addAndMakeVisible(snapStepBox);

    wireButton(leftButton);
    wireButton(rightButton);
    wireButton(upButton);
    wireButton(downButton);
    wireButton(scaleOutButton);
    wireButton(scaleInButton);
    wireButton(extrudeMoreButton);
    wireButton(extrudeLessButton);
    wireButton(bevelMoreButton);
    wireButton(bevelLessButton);

    translateButton.onClick = [this] { selectTool(EngineerSceneModel::GeometryTool::translate); };
    scaleButton.onClick = [this] { selectTool(EngineerSceneModel::GeometryTool::scale); };
    extrudeButton.onClick = [this] { selectTool(EngineerSceneModel::GeometryTool::extrude); };
    bevelButton.onClick = [this] { selectTool(EngineerSceneModel::GeometryTool::bevel); };
    leftButton.onClick = [this] { nudgeLeft(); };
    rightButton.onClick = [this] { nudgeRight(); };
    upButton.onClick = [this] { nudgeUp(); };
    downButton.onClick = [this] { nudgeDown(); };
    scaleOutButton.onClick = [this] { scaleOut(); };
    scaleInButton.onClick = [this] { scaleIn(); };
    extrudeMoreButton.onClick = [this] { extrudeMore(); };
    extrudeLessButton.onClick = [this] { extrudeLess(); };
    bevelMoreButton.onClick = [this] { bevelMore(); };
    bevelLessButton.onClick = [this] { bevelLess(); };

    refreshFromScene();
}

EngineerGeometryToolsComponent::~EngineerGeometryToolsComponent()
{
    sceneModel.removeListener(this);
}

void EngineerGeometryToolsComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101722));
}

void EngineerGeometryToolsComponent::resized()
{
    auto area = getLocalBounds().reduced(12);
    titleLabel.setBounds(area.removeFromTop(24));
    detailLabel.setBounds(area.removeFromTop(34));
    area.removeFromTop(8);
    modeLabel.setBounds(area.removeFromTop(22));
    depthLabel.setBounds(area.removeFromTop(20));
    bevelLabel.setBounds(area.removeFromTop(20));
    snapLabel.setBounds(area.removeFromTop(20));
    area.removeFromTop(10);

    auto toolRow = area.removeFromTop(28);
    translateButton.setBounds(toolRow.removeFromLeft(84));
    toolRow.removeFromLeft(6);
    scaleButton.setBounds(toolRow.removeFromLeft(72));
    toolRow.removeFromLeft(6);
    extrudeButton.setBounds(toolRow.removeFromLeft(78));
    toolRow.removeFromLeft(6);
    bevelButton.setBounds(toolRow.removeFromLeft(72));
    area.removeFromTop(10);

    auto snapRow = area.removeFromTop(28);
    snapToggle.setBounds(snapRow.removeFromLeft(118));
    snapRow.removeFromLeft(8);
    snapStepBox.setBounds(snapRow.removeFromLeft(110));
    area.removeFromTop(10);

    auto moveRowOne = area.removeFromTop(28);
    leftButton.setBounds(moveRowOne.removeFromLeft(72));
    moveRowOne.removeFromLeft(8);
    rightButton.setBounds(moveRowOne.removeFromLeft(72));
    moveRowOne.removeFromLeft(8);
    upButton.setBounds(moveRowOne.removeFromLeft(72));
    moveRowOne.removeFromLeft(8);
    downButton.setBounds(moveRowOne.removeFromLeft(72));
    area.removeFromTop(8);

    auto moveRowTwo = area.removeFromTop(28);
    scaleOutButton.setBounds(moveRowTwo.removeFromLeft(82));
    moveRowTwo.removeFromLeft(8);
    scaleInButton.setBounds(moveRowTwo.removeFromLeft(82));
    moveRowTwo.removeFromLeft(8);
    extrudeMoreButton.setBounds(moveRowTwo.removeFromLeft(92));
    moveRowTwo.removeFromLeft(8);
    extrudeLessButton.setBounds(moveRowTwo.removeFromLeft(92));
    area.removeFromTop(8);

    auto moveRowThree = area.removeFromTop(28);
    bevelMoreButton.setBounds(moveRowThree.removeFromLeft(82));
    moveRowThree.removeFromLeft(8);
    bevelLessButton.setBounds(moveRowThree.removeFromLeft(82));
}

void EngineerGeometryToolsComponent::engineerSceneModelChanged()
{
    refreshFromScene();
    repaint();
}

void EngineerGeometryToolsComponent::refreshFromScene()
{
    const auto& object = sceneModel.getSelectedObject();
    const auto geometryMode = sceneModel.isSelectedObjectDirectGeometry();

    modeLabel.setText("Selected tool: " + EngineerSceneModel::toDisplayString(sceneModel.getSelectedGeometryTool())
                      + "   |   Authoring state: " + EngineerSceneModel::toDisplayString(object.authoringState),
                      juce::dontSendNotification);
    depthLabel.setText("Geometry depth: " + juce::String(object.geometryDepth * 100.0f, 1) + " mm (normalized workspace proxy)",
                       juce::dontSendNotification);
    bevelLabel.setText("Bevel amount: " + juce::String(object.bevelAmount * 100.0f, 1) + " mm",
                       juce::dontSendNotification);
    snapLabel.setText("Snap step: " + juce::String(sceneModel.getGeometrySnapStep() * 1000.0f, 1) + " mm"
                      + "   |   Snapping: " + juce::String(sceneModel.isGeometrySnappingEnabled() ? "On" : "Off"),
                      juce::dontSendNotification);

    translateButton.setToggleState(sceneModel.getSelectedGeometryTool() == EngineerSceneModel::GeometryTool::translate, juce::dontSendNotification);
    scaleButton.setToggleState(sceneModel.getSelectedGeometryTool() == EngineerSceneModel::GeometryTool::scale, juce::dontSendNotification);
    extrudeButton.setToggleState(sceneModel.getSelectedGeometryTool() == EngineerSceneModel::GeometryTool::extrude, juce::dontSendNotification);
    bevelButton.setToggleState(sceneModel.getSelectedGeometryTool() == EngineerSceneModel::GeometryTool::bevel, juce::dontSendNotification);
    snapToggle.setToggleState(sceneModel.isGeometrySnappingEnabled(), juce::dontSendNotification);

    const auto step = sceneModel.getGeometrySnapStep();
    if (step <= 0.0026f) snapStepBox.setSelectedId(1, juce::dontSendNotification);
    else if (step <= 0.0051f) snapStepBox.setSelectedId(2, juce::dontSendNotification);
    else if (step <= 0.0101f) snapStepBox.setSelectedId(3, juce::dontSendNotification);
    else snapStepBox.setSelectedId(4, juce::dontSendNotification);

    leftButton.setEnabled(geometryMode);
    rightButton.setEnabled(geometryMode);
    upButton.setEnabled(geometryMode);
    downButton.setEnabled(geometryMode);
    scaleOutButton.setEnabled(geometryMode);
    scaleInButton.setEnabled(geometryMode);
    extrudeMoreButton.setEnabled(geometryMode);
    extrudeLessButton.setEnabled(geometryMode);
    bevelMoreButton.setEnabled(geometryMode);
    bevelLessButton.setEnabled(geometryMode);
    snapToggle.setEnabled(geometryMode);
    snapStepBox.setEnabled(geometryMode && sceneModel.isGeometrySnappingEnabled());
}

void EngineerGeometryToolsComponent::selectTool(EngineerSceneModel::GeometryTool tool)
{
    sceneModel.setSelectedGeometryTool(tool);
}

void EngineerGeometryToolsComponent::toggleSnapping()
{
    sceneModel.setGeometrySnappingEnabled(snapToggle.getToggleState());
}

void EngineerGeometryToolsComponent::setSnapStep()
{
    switch (snapStepBox.getSelectedId())
    {
        case 1: sceneModel.setGeometrySnapStep(0.0025f); break;
        case 2: sceneModel.setGeometrySnapStep(0.005f); break;
        case 3: sceneModel.setGeometrySnapStep(0.01f); break;
        case 4: sceneModel.setGeometrySnapStep(0.025f); break;
        default: break;
    }
}

void EngineerGeometryToolsComponent::nudgeLeft()
{
    sceneModel.nudgeSelectedDirectGeometryPosition({ -0.02f, 0.0f });
}

void EngineerGeometryToolsComponent::nudgeRight()
{
    sceneModel.nudgeSelectedDirectGeometryPosition({ 0.02f, 0.0f });
}

void EngineerGeometryToolsComponent::nudgeUp()
{
    sceneModel.nudgeSelectedDirectGeometryPosition({ 0.0f, -0.02f });
}

void EngineerGeometryToolsComponent::nudgeDown()
{
    sceneModel.nudgeSelectedDirectGeometryPosition({ 0.0f, 0.02f });
}

void EngineerGeometryToolsComponent::scaleOut()
{
    sceneModel.scaleSelectedDirectGeometry({ 0.02f, 0.015f });
}

void EngineerGeometryToolsComponent::scaleIn()
{
    sceneModel.scaleSelectedDirectGeometry({ -0.02f, -0.015f });
}

void EngineerGeometryToolsComponent::extrudeMore()
{
    sceneModel.extrudeSelectedDirectGeometry(0.01f);
}

void EngineerGeometryToolsComponent::extrudeLess()
{
    sceneModel.extrudeSelectedDirectGeometry(-0.01f);
}

void EngineerGeometryToolsComponent::bevelMore()
{
    sceneModel.bevelSelectedDirectGeometry(0.005f);
}

void EngineerGeometryToolsComponent::bevelLess()
{
    sceneModel.bevelSelectedDirectGeometry(-0.005f);
}
