#include "EngineerGeometryElementsComponent.h"

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

EngineerGeometryElementsComponent::EngineerGeometryElementsComponent(EngineerSceneModel& model)
    : sceneModel(model)
{
    sceneModel.addListener(this);

    titleLabel.setText("Geometry Elements", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::Font(18.0f).boldened());
    addAndMakeVisible(titleLabel);

    detailLabel.setText("Edit committed parts through engineering-friendly vertex, edge, and face proxies.", juce::dontSendNotification);
    detailLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaebed0));
    addAndMakeVisible(detailLabel);

    selectionLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(selectionLabel);

    auto wire = [this](juce::TextButton& button)
    {
        configureButton(button);
        addAndMakeVisible(button);
    };

    wire(vertexButton);
    wire(edgeButton);
    wire(faceButton);
    wire(previousButton);
    wire(nextButton);
    wire(leftButton);
    wire(rightButton);
    wire(upButton);
    wire(downButton);

    vertexButton.onClick = [this] { selectKind(EngineerSceneModel::GeometryElementKind::vertex); };
    edgeButton.onClick = [this] { selectKind(EngineerSceneModel::GeometryElementKind::edge); };
    faceButton.onClick = [this] { selectKind(EngineerSceneModel::GeometryElementKind::face); };
    previousButton.onClick = [this] { previousElement(); };
    nextButton.onClick = [this] { nextElement(); };
    leftButton.onClick = [this] { nudgeLeft(); };
    rightButton.onClick = [this] { nudgeRight(); };
    upButton.onClick = [this] { nudgeUp(); };
    downButton.onClick = [this] { nudgeDown(); };

    refreshFromScene();
}

EngineerGeometryElementsComponent::~EngineerGeometryElementsComponent()
{
    sceneModel.removeListener(this);
}

void EngineerGeometryElementsComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101722));
}

void EngineerGeometryElementsComponent::resized()
{
    auto area = getLocalBounds().reduced(12);
    titleLabel.setBounds(area.removeFromTop(24));
    detailLabel.setBounds(area.removeFromTop(34));
    area.removeFromTop(8);
    selectionLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(10);

    auto kindRow = area.removeFromTop(28);
    vertexButton.setBounds(kindRow.removeFromLeft(80));
    kindRow.removeFromLeft(8);
    edgeButton.setBounds(kindRow.removeFromLeft(72));
    kindRow.removeFromLeft(8);
    faceButton.setBounds(kindRow.removeFromLeft(72));
    area.removeFromTop(10);

    auto navRow = area.removeFromTop(28);
    previousButton.setBounds(navRow.removeFromLeft(88));
    navRow.removeFromLeft(8);
    nextButton.setBounds(navRow.removeFromLeft(72));
    area.removeFromTop(10);

    auto nudgeRow = area.removeFromTop(28);
    leftButton.setBounds(nudgeRow.removeFromLeft(72));
    nudgeRow.removeFromLeft(8);
    rightButton.setBounds(nudgeRow.removeFromLeft(72));
    nudgeRow.removeFromLeft(8);
    upButton.setBounds(nudgeRow.removeFromLeft(72));
    nudgeRow.removeFromLeft(8);
    downButton.setBounds(nudgeRow.removeFromLeft(72));
}

void EngineerGeometryElementsComponent::engineerSceneModelChanged()
{
    refreshFromScene();
    repaint();
}

void EngineerGeometryElementsComponent::refreshFromScene()
{
    const bool enabled = sceneModel.isSelectedObjectDirectGeometry();
    const auto kind = sceneModel.getSelectedGeometryElementKind();
    const auto index = sceneModel.getSelectedGeometryElementIndex();
    const auto count = sceneModel.getSelectedGeometryElementCount();

    selectionLabel.setText("Selected element: "
                           + EngineerSceneModel::toDisplayString(kind)
                           + " "
                           + juce::String(index + 1)
                           + " of "
                           + juce::String(count),
                           juce::dontSendNotification);

    vertexButton.setToggleState(kind == EngineerSceneModel::GeometryElementKind::vertex, juce::dontSendNotification);
    edgeButton.setToggleState(kind == EngineerSceneModel::GeometryElementKind::edge, juce::dontSendNotification);
    faceButton.setToggleState(kind == EngineerSceneModel::GeometryElementKind::face, juce::dontSendNotification);

    vertexButton.setEnabled(enabled);
    edgeButton.setEnabled(enabled);
    faceButton.setEnabled(enabled);
    previousButton.setEnabled(enabled && index > 0);
    nextButton.setEnabled(enabled && index < count - 1);
    leftButton.setEnabled(enabled);
    rightButton.setEnabled(enabled);
    upButton.setEnabled(enabled);
    downButton.setEnabled(enabled);
}

void EngineerGeometryElementsComponent::selectKind(EngineerSceneModel::GeometryElementKind kind)
{
    sceneModel.setSelectedGeometryElementKind(kind);
}

void EngineerGeometryElementsComponent::previousElement()
{
    sceneModel.selectPreviousGeometryElement();
}

void EngineerGeometryElementsComponent::nextElement()
{
    sceneModel.selectNextGeometryElement();
}

void EngineerGeometryElementsComponent::nudgeLeft()
{
    sceneModel.nudgeSelectedGeometryElement({ -0.015f, 0.0f });
}

void EngineerGeometryElementsComponent::nudgeRight()
{
    sceneModel.nudgeSelectedGeometryElement({ 0.015f, 0.0f });
}

void EngineerGeometryElementsComponent::nudgeUp()
{
    sceneModel.nudgeSelectedGeometryElement({ 0.0f, -0.015f });
}

void EngineerGeometryElementsComponent::nudgeDown()
{
    sceneModel.nudgeSelectedGeometryElement({ 0.0f, 0.015f });
}
