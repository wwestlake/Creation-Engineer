#include "EngineerPropertiesComponent.h"

namespace
{
void configureCaption(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffaebed0));
}

void configureEditor(juce::TextEditor& editor)
{
    editor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff111923));
    editor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff314155));
    editor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
}
}

EngineerPropertiesComponent::EngineerPropertiesComponent(EngineerSceneModel& model)
    : sceneModel(model)
{
    sceneModel.addListener(this);

    titleLabel.setText("Selected Object", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::Font(18.0f).boldened());
    addAndMakeVisible(titleLabel);

    hintLabel.setText("Edit primitive-backed object values and watch the viewport update.", juce::dontSendNotification);
    hintLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaebed0));
    addAndMakeVisible(hintLabel);

    configureCaption(nameLabel, "Name");
    configureCaption(primitiveLabel, "Primitive");
    configureCaption(positionLabel, "Position (normalized)");
    configureCaption(sizeLabel, "Size (normalized)");
    configureCaption(authoringStateLabel, "Authoring State");
    configureCaption(modifierLabel, "Modifier Stack");

    authoringStateValue.setColour(juce::Label::textColourId, juce::Colours::white);
    authoringStateValue.setFont(juce::Font(15.0f).boldened());

    addAndMakeVisible(nameLabel);
    addAndMakeVisible(primitiveLabel);
    addAndMakeVisible(positionLabel);
    addAndMakeVisible(sizeLabel);
    addAndMakeVisible(authoringStateLabel);
    addAndMakeVisible(authoringStateValue);
    addAndMakeVisible(modifierLabel);

    configureEditor(nameEditor);
    configureEditor(posXEditor);
    configureEditor(posYEditor);
    configureEditor(sizeXEditor);
    configureEditor(sizeYEditor);

    primitiveBox.addItem("Block", 1);
    primitiveBox.addItem("Cylinder", 2);
    primitiveBox.addItem("Plate", 3);

    nameEditor.onFocusLost = [this] { commitName(); };
    nameEditor.onReturnKey = [this] { commitName(); };
    primitiveBox.onChange = [this] { commitPrimitiveType(); };
    posXEditor.onFocusLost = [this] { commitPosition(); };
    posYEditor.onFocusLost = [this] { commitPosition(); };
    sizeXEditor.onFocusLost = [this] { commitSize(); };
    sizeYEditor.onFocusLost = [this] { commitSize(); };
    mirrorXToggle.onClick = [this] { commitMirrorState(); };
    commitGeometryButton.onClick = [this] { commitDirectGeometry(); };
    restorePrimitiveButton.onClick = [this] { restorePrimitiveWorkflow(); };

    mirrorXToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    commitGeometryButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a2533));
    commitGeometryButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    restorePrimitiveButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a2533));
    restorePrimitiveButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

    addAndMakeVisible(nameEditor);
    addAndMakeVisible(primitiveBox);
    addAndMakeVisible(posXEditor);
    addAndMakeVisible(posYEditor);
    addAndMakeVisible(sizeXEditor);
    addAndMakeVisible(sizeYEditor);
    addAndMakeVisible(mirrorXToggle);
    addAndMakeVisible(commitGeometryButton);
    addAndMakeVisible(restorePrimitiveButton);

    refreshFromScene();
}

EngineerPropertiesComponent::~EngineerPropertiesComponent()
{
    sceneModel.removeListener(this);
}

void EngineerPropertiesComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101722));
}

void EngineerPropertiesComponent::resized()
{
    auto area = getLocalBounds().reduced(12);
    titleLabel.setBounds(area.removeFromTop(24));
    hintLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(10);

    nameLabel.setBounds(area.removeFromTop(18));
    nameEditor.setBounds(area.removeFromTop(26));
    area.removeFromTop(8);

    primitiveLabel.setBounds(area.removeFromTop(18));
    primitiveBox.setBounds(area.removeFromTop(26));
    area.removeFromTop(8);

    positionLabel.setBounds(area.removeFromTop(18));
    auto posRow = area.removeFromTop(26);
    posXEditor.setBounds(posRow.removeFromLeft(posRow.getWidth() / 2 - 4));
    posRow.removeFromLeft(8);
    posYEditor.setBounds(posRow);
    area.removeFromTop(8);

    sizeLabel.setBounds(area.removeFromTop(18));
    auto sizeRow = area.removeFromTop(26);
    sizeXEditor.setBounds(sizeRow.removeFromLeft(sizeRow.getWidth() / 2 - 4));
    sizeRow.removeFromLeft(8);
    sizeYEditor.setBounds(sizeRow);
    area.removeFromTop(10);

    authoringStateLabel.setBounds(area.removeFromTop(18));
    authoringStateValue.setBounds(area.removeFromTop(22));
    area.removeFromTop(8);

    modifierLabel.setBounds(area.removeFromTop(18));
    mirrorXToggle.setBounds(area.removeFromTop(24));
    area.removeFromTop(12);

    commitGeometryButton.setBounds(area.removeFromTop(28));
    area.removeFromTop(8);
    restorePrimitiveButton.setBounds(area.removeFromTop(28));
}

void EngineerPropertiesComponent::engineerSceneModelChanged()
{
    refreshFromScene();
    repaint();
}

void EngineerPropertiesComponent::refreshFromScene()
{
    const auto& object = sceneModel.getSelectedObject();
    nameEditor.setText(object.name, juce::dontSendNotification);
    primitiveBox.setText(object.primitiveType, juce::dontSendNotification);
    posXEditor.setText(juce::String(object.normalizedPosition.x, 3), juce::dontSendNotification);
    posYEditor.setText(juce::String(object.normalizedPosition.y, 3), juce::dontSendNotification);
    sizeXEditor.setText(juce::String(object.normalizedSize.x, 3), juce::dontSendNotification);
    sizeYEditor.setText(juce::String(object.normalizedSize.y, 3), juce::dontSendNotification);
    authoringStateValue.setText(EngineerSceneModel::toDisplayString(object.authoringState), juce::dontSendNotification);
    mirrorXToggle.setToggleState(object.mirrorXEnabled, juce::dontSendNotification);
    mirrorXToggle.setEnabled(object.authoringState != EngineerSceneModel::AuthoringState::directGeometry);
    commitGeometryButton.setEnabled(object.authoringState != EngineerSceneModel::AuthoringState::directGeometry);
    restorePrimitiveButton.setEnabled(object.authoringState != EngineerSceneModel::AuthoringState::primitive);
}

void EngineerPropertiesComponent::commitName()
{
    sceneModel.setSelectedObjectName(nameEditor.getText().trim());
}

void EngineerPropertiesComponent::commitPrimitiveType()
{
    sceneModel.setSelectedObjectPrimitiveType(primitiveBox.getText());
}

void EngineerPropertiesComponent::commitPosition()
{
    sceneModel.setSelectedObjectPosition({ posXEditor.getText().getFloatValue(),
                                           posYEditor.getText().getFloatValue() });
}

void EngineerPropertiesComponent::commitSize()
{
    sceneModel.setSelectedObjectSize({ juce::jmax(0.05f, sizeXEditor.getText().getFloatValue()),
                                       juce::jmax(0.05f, sizeYEditor.getText().getFloatValue()) });
}

void EngineerPropertiesComponent::commitMirrorState()
{
    sceneModel.setSelectedObjectMirrorXEnabled(mirrorXToggle.getToggleState());
}

void EngineerPropertiesComponent::commitDirectGeometry()
{
    sceneModel.commitSelectedObjectToDirectGeometry();
}

void EngineerPropertiesComponent::restorePrimitiveWorkflow()
{
    sceneModel.restoreSelectedObjectPrimitiveWorkflow();
}
