#include "EngineerNavigatorComponent.h"

namespace
{
void configureActionButton(juce::TextButton& button)
{
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a2533));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff25354a));
    button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}
}

EngineerNavigatorComponent::EngineerNavigatorComponent(EngineerSceneModel& model)
    : sceneModel(model)
{
    sceneModel.addListener(this);

    titleLabel.setText("Scene Objects", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::Font(18.0f).boldened());
    addAndMakeVisible(titleLabel);

    detailLabel.setText("Add parts, duplicate selections, and manage the active engineering workspace.", juce::dontSendNotification);
    detailLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaebed0));
    addAndMakeVisible(detailLabel);

    configureActionButton(addBlockButton);
    configureActionButton(addCylinderButton);
    configureActionButton(addPlateButton);
    configureActionButton(duplicateButton);
    configureActionButton(removeButton);

    addBlockButton.onClick = [this] { addPrimitive("Block"); };
    addCylinderButton.onClick = [this] { addPrimitive("Cylinder"); };
    addPlateButton.onClick = [this] { addPrimitive("Plate"); };
    duplicateButton.onClick = [this] { duplicateSelection(); };
    removeButton.onClick = [this] { removeSelection(); };

    addAndMakeVisible(addBlockButton);
    addAndMakeVisible(addCylinderButton);
    addAndMakeVisible(addPlateButton);
    addAndMakeVisible(duplicateButton);
    addAndMakeVisible(removeButton);

    objectList.setModel(this);
    objectList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff111923));
    objectList.setOutlineThickness(0);
    addAndMakeVisible(objectList);

    objectList.selectRow(sceneModel.getSelectedObjectIndex());
    removeButton.setEnabled(sceneModel.canRemoveSelectedObject());
}

EngineerNavigatorComponent::~EngineerNavigatorComponent()
{
    sceneModel.removeListener(this);
}

void EngineerNavigatorComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101722));
}

void EngineerNavigatorComponent::resized()
{
    auto area = getLocalBounds().reduced(12);
    titleLabel.setBounds(area.removeFromTop(24));
    detailLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(8);
    auto addRow = area.removeFromTop(28);
    addBlockButton.setBounds(addRow.removeFromLeft(78));
    addRow.removeFromLeft(6);
    addCylinderButton.setBounds(addRow.removeFromLeft(88));
    addRow.removeFromLeft(6);
    addPlateButton.setBounds(addRow.removeFromLeft(72));
    area.removeFromTop(8);
    auto manageRow = area.removeFromTop(28);
    duplicateButton.setBounds(manageRow.removeFromLeft(96));
    manageRow.removeFromLeft(8);
    removeButton.setBounds(manageRow.removeFromLeft(78));
    area.removeFromTop(8);
    objectList.setBounds(area);
}

int EngineerNavigatorComponent::getNumRows()
{
    return static_cast<int>(sceneModel.getObjects().size());
}

void EngineerNavigatorComponent::paintListBoxItem(int rowNumber,
                                                  juce::Graphics& g,
                                                  int width,
                                                  int height,
                                                  bool rowIsSelected)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).reduced(4, 2);
    g.setColour(rowIsSelected ? juce::Colour(0xff1e3044) : juce::Colour(0xff15202c));
    g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

    if (rowNumber < 0 || rowNumber >= static_cast<int>(sceneModel.getObjects().size()))
        return;

    const auto& object = sceneModel.getObjects()[static_cast<size_t>(rowNumber)];
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(15.0f).boldened());
    g.drawText(object.name, bounds.removeFromTop(22), juce::Justification::centredLeft, true);

    g.setColour(juce::Colour(0xff9eb4c9));
    g.setFont(juce::Font(12.0f));
    g.drawText(object.primitiveType
                   + "  |  "
                   + EngineerSceneModel::toDisplayString(object.authoringState)
                   + "  |  Pos "
                   + juce::String(object.position.x, 2)
                   + ", "
                   + juce::String(object.position.y, 2)
                   + ", "
                   + juce::String(object.position.z, 2),
               bounds,
               juce::Justification::centredLeft,
               true);
}

void EngineerNavigatorComponent::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected >= 0)
        sceneModel.selectObject(lastRowSelected);
}

void EngineerNavigatorComponent::engineerSceneModelChanged()
{
    objectList.updateContent();
    objectList.selectRow(sceneModel.getSelectedObjectIndex(), juce::dontSendNotification);
    removeButton.setEnabled(sceneModel.canRemoveSelectedObject());
    repaint();
}

void EngineerNavigatorComponent::addPrimitive(const juce::String& primitiveType)
{
    sceneModel.addPrimitiveObject(primitiveType);
}

void EngineerNavigatorComponent::duplicateSelection()
{
    sceneModel.duplicateSelectedObject();
}

void EngineerNavigatorComponent::removeSelection()
{
    sceneModel.removeSelectedObject();
}
