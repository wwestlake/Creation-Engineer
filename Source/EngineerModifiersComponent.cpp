#include "EngineerModifiersComponent.h"

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

EngineerModifiersComponent::EngineerModifiersComponent(EngineerSceneModel& model)
    : sceneModel(model)
{
    sceneModel.addListener(this);

    titleLabel.setText("Modifier Stack", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::Font(18.0f).boldened());
    addAndMakeVisible(titleLabel);

    detailLabel.setText("Build non-destructive part transforms before committing to direct geometry.", juce::dontSendNotification);
    detailLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaebed0));
    addAndMakeVisible(detailLabel);

    configureActionButton(addMirrorButton);
    configureActionButton(addShellButton);
    configureActionButton(toggleEnabledButton);
    configureActionButton(moveUpButton);
    configureActionButton(moveDownButton);
    configureActionButton(removeButton);

    addMirrorButton.onClick = [this] { addMirrorModifier(); };
    addShellButton.onClick = [this] { addShellModifier(); };
    toggleEnabledButton.onClick = [this] { toggleSelectedModifierEnabled(); };
    moveUpButton.onClick = [this] { moveSelectedModifierUp(); };
    moveDownButton.onClick = [this] { moveSelectedModifierDown(); };
    removeButton.onClick = [this] { removeSelectedModifier(); };

    addAndMakeVisible(addMirrorButton);
    addAndMakeVisible(addShellButton);
    addAndMakeVisible(toggleEnabledButton);
    addAndMakeVisible(moveUpButton);
    addAndMakeVisible(moveDownButton);
    addAndMakeVisible(removeButton);

    modifierList.setModel(this);
    modifierList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff111923));
    modifierList.setOutlineThickness(0);
    addAndMakeVisible(modifierList);

    refreshControls();
}

EngineerModifiersComponent::~EngineerModifiersComponent()
{
    sceneModel.removeListener(this);
}

void EngineerModifiersComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101722));
}

void EngineerModifiersComponent::resized()
{
    auto area = getLocalBounds().reduced(12);
    titleLabel.setBounds(area.removeFromTop(24));
    detailLabel.setBounds(area.removeFromTop(34));
    area.removeFromTop(8);

    auto addRow = area.removeFromTop(28);
    addMirrorButton.setBounds(addRow.removeFromLeft(90));
    addRow.removeFromLeft(8);
    addShellButton.setBounds(addRow.removeFromLeft(82));
    area.removeFromTop(8);

    auto actionRow = area.removeFromTop(28);
    toggleEnabledButton.setBounds(actionRow.removeFromLeft(128));
    actionRow.removeFromLeft(8);
    moveUpButton.setBounds(actionRow.removeFromLeft(82));
    actionRow.removeFromLeft(8);
    moveDownButton.setBounds(actionRow.removeFromLeft(94));
    actionRow.removeFromLeft(8);
    removeButton.setBounds(actionRow.removeFromLeft(82));
    area.removeFromTop(8);

    modifierList.setBounds(area);
}

int EngineerModifiersComponent::getNumRows()
{
    return static_cast<int>(sceneModel.getSelectedObjectModifiers().size());
}

void EngineerModifiersComponent::paintListBoxItem(int rowNumber,
                                                  juce::Graphics& g,
                                                  int width,
                                                  int height,
                                                  bool rowIsSelected)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).reduced(4, 2);
    g.setColour(rowIsSelected ? juce::Colour(0xff1e3044) : juce::Colour(0xff15202c));
    g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

    const auto& modifiers = sceneModel.getSelectedObjectModifiers();
    if (rowNumber < 0 || rowNumber >= static_cast<int>(modifiers.size()))
        return;

    const auto& modifier = modifiers[static_cast<size_t>(rowNumber)];

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(15.0f).boldened());
    g.drawText(modifier.type, bounds.removeFromTop(22), juce::Justification::centredLeft, true);

    g.setColour(juce::Colour(0xff9eb4c9));
    g.setFont(juce::Font(12.0f));
    g.drawText(EngineerSceneModel::toDisplayString(modifier),
               bounds,
               juce::Justification::centredLeft,
               true);
}

void EngineerModifiersComponent::selectedRowsChanged(int lastRowSelected)
{
    selectedModifierIndex = lastRowSelected;
    refreshControls();
}

void EngineerModifiersComponent::engineerSceneModelChanged()
{
    const auto modifierCount = static_cast<int>(sceneModel.getSelectedObjectModifiers().size());
    if (modifierCount == 0)
        selectedModifierIndex = -1;
    else
        selectedModifierIndex = juce::jlimit(0, modifierCount - 1, selectedModifierIndex);

    modifierList.updateContent();
    modifierList.selectRow(selectedModifierIndex, juce::dontSendNotification);
    refreshControls();
    repaint();
}

void EngineerModifiersComponent::addMirrorModifier()
{
    sceneModel.addSelectedObjectMirrorModifier();
    selectedModifierIndex = static_cast<int>(sceneModel.getSelectedObjectModifiers().size()) - 1;
}

void EngineerModifiersComponent::addShellModifier()
{
    sceneModel.addSelectedObjectShellModifier();
    selectedModifierIndex = static_cast<int>(sceneModel.getSelectedObjectModifiers().size()) - 1;
}

void EngineerModifiersComponent::toggleSelectedModifierEnabled()
{
    if (selectedModifierIndex < 0)
        return;

    const auto& modifiers = sceneModel.getSelectedObjectModifiers();
    if (selectedModifierIndex >= static_cast<int>(modifiers.size()))
        return;

    sceneModel.setSelectedModifierEnabled(selectedModifierIndex,
                                          !modifiers[static_cast<size_t>(selectedModifierIndex)].enabled);
}

void EngineerModifiersComponent::moveSelectedModifierUp()
{
    if (selectedModifierIndex < 0)
        return;

    sceneModel.moveSelectedModifierUp(selectedModifierIndex);
    selectedModifierIndex = juce::jmax(0, selectedModifierIndex - 1);
}

void EngineerModifiersComponent::moveSelectedModifierDown()
{
    if (selectedModifierIndex < 0)
        return;

    sceneModel.moveSelectedModifierDown(selectedModifierIndex);
    selectedModifierIndex += 1;
}

void EngineerModifiersComponent::removeSelectedModifier()
{
    if (selectedModifierIndex < 0)
        return;

    sceneModel.removeSelectedModifier(selectedModifierIndex);
}

void EngineerModifiersComponent::refreshControls()
{
    const auto modifierCount = static_cast<int>(sceneModel.getSelectedObjectModifiers().size());
    const bool hasSelection = selectedModifierIndex >= 0 && selectedModifierIndex < modifierCount;

    toggleEnabledButton.setEnabled(hasSelection);
    moveUpButton.setEnabled(sceneModel.canMoveSelectedModifierUp(selectedModifierIndex));
    moveDownButton.setEnabled(sceneModel.canMoveSelectedModifierDown(selectedModifierIndex));
    removeButton.setEnabled(sceneModel.canRemoveSelectedModifier(selectedModifierIndex));
}
