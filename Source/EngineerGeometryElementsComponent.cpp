#include "EngineerGeometryElementsComponent.h"

#include "EngineerUnits.h"

namespace
{
void configureButton(juce::TextButton& button)
{
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a2533));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff25354a));
    button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}

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

EngineerGeometryElementsComponent::EngineerGeometryElementsComponent(EngineerSceneModel& model)
    : sceneModel(model)
{
    sceneModel.addListener(this);

    titleLabel.setText("Geometry Elements", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::Font(juce::FontOptions(18.0f)).boldened());
    addAndMakeVisible(titleLabel);

    detailLabel.setText("Object Mode moves whole parts. Vertex Mode edits a committed part's real geometry.",
                        juce::dontSendNotification);
    detailLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaebed0));
    addAndMakeVisible(detailLabel);

    selectionLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(selectionLabel);

    auto wire = [this](juce::TextButton& button)
    {
        configureButton(button);
        addAndMakeVisible(button);
    };

    wire(objectModeButton);
    wire(vertexModeButton);
    wire(previousButton);
    wire(nextButton);

    objectModeButton.onClick = [this] { setObjectMode(); };
    vertexModeButton.onClick = [this] { setVertexMode(); };
    previousButton.onClick = [this] { previousVertex(); };
    nextButton.onClick = [this] { nextVertex(); };

    configureCaption(vertexPositionLabel, "Selected Vertex Position (m)");
    addAndMakeVisible(vertexPositionLabel);

    configureEditor(vertexXEditor);
    configureEditor(vertexYEditor);
    configureEditor(vertexZEditor);
    vertexXEditor.onFocusLost = [this] { commitVertexPosition(); };
    vertexXEditor.onReturnKey = [this] { commitVertexPosition(); };
    vertexYEditor.onFocusLost = [this] { commitVertexPosition(); };
    vertexYEditor.onReturnKey = [this] { commitVertexPosition(); };
    vertexZEditor.onFocusLost = [this] { commitVertexPosition(); };
    vertexZEditor.onReturnKey = [this] { commitVertexPosition(); };
    addAndMakeVisible(vertexXEditor);
    addAndMakeVisible(vertexYEditor);
    addAndMakeVisible(vertexZEditor);

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

    auto modeRow = area.removeFromTop(28);
    objectModeButton.setBounds(modeRow.removeFromLeft(80));
    modeRow.removeFromLeft(8);
    vertexModeButton.setBounds(modeRow.removeFromLeft(80));
    area.removeFromTop(10);

    auto navRow = area.removeFromTop(28);
    previousButton.setBounds(navRow.removeFromLeft(88));
    navRow.removeFromLeft(8);
    nextButton.setBounds(navRow.removeFromLeft(72));
    area.removeFromTop(12);

    vertexPositionLabel.setBounds(area.removeFromTop(18));
    auto vertexRow = area.removeFromTop(26);
    const auto fieldWidth = (vertexRow.getWidth() - 16) / 3;
    vertexXEditor.setBounds(vertexRow.removeFromLeft(fieldWidth));
    vertexRow.removeFromLeft(8);
    vertexYEditor.setBounds(vertexRow.removeFromLeft(fieldWidth));
    vertexRow.removeFromLeft(8);
    vertexZEditor.setBounds(vertexRow);
}

void EngineerGeometryElementsComponent::engineerSceneModelChanged()
{
    refreshFromScene();
    repaint();
}

void EngineerGeometryElementsComponent::refreshFromScene()
{
    const auto mode = sceneModel.getEditMode();
    const auto vertexEditable = sceneModel.isSelectedObjectVertexEditable();
    const auto index = sceneModel.getSelectedVertexIndex();
    const auto count = sceneModel.getSelectedVertexCount();

    objectModeButton.setToggleState(mode == EngineerSceneModel::EditMode::object, juce::dontSendNotification);
    vertexModeButton.setToggleState(mode == EngineerSceneModel::EditMode::vertex, juce::dontSendNotification);
    vertexModeButton.setEnabled(vertexEditable);

    const auto showVertexControls = mode == EngineerSceneModel::EditMode::vertex && vertexEditable;
    previousButton.setVisible(showVertexControls);
    nextButton.setVisible(showVertexControls);
    vertexPositionLabel.setVisible(showVertexControls);
    vertexXEditor.setVisible(showVertexControls);
    vertexYEditor.setVisible(showVertexControls);
    vertexZEditor.setVisible(showVertexControls);

    if (!vertexEditable)
    {
        selectionLabel.setText("Commit the selected object to direct geometry to edit its vertices.",
                               juce::dontSendNotification);
        return;
    }

    if (mode != EngineerSceneModel::EditMode::vertex)
    {
        selectionLabel.setText("Switch to Vertex Mode to edit this object's geometry.", juce::dontSendNotification);
        return;
    }

    selectionLabel.setText("Selected vertex: " + juce::String(index + 1) + " of " + juce::String(count),
                           juce::dontSendNotification);
    previousButton.setEnabled(index > 0);
    nextButton.setEnabled(index < count - 1);

    const auto position = sceneModel.getSelectedVertexPosition();
    vertexXEditor.setText(EngineerUnits::formatLengthMeters(position.x), juce::dontSendNotification);
    vertexYEditor.setText(EngineerUnits::formatLengthMeters(position.y), juce::dontSendNotification);
    vertexZEditor.setText(EngineerUnits::formatLengthMeters(position.z), juce::dontSendNotification);
}

void EngineerGeometryElementsComponent::setObjectMode()
{
    sceneModel.setEditMode(EngineerSceneModel::EditMode::object);
}

void EngineerGeometryElementsComponent::setVertexMode()
{
    sceneModel.setEditMode(EngineerSceneModel::EditMode::vertex);
}

void EngineerGeometryElementsComponent::previousVertex()
{
    sceneModel.selectPreviousVertex();
}

void EngineerGeometryElementsComponent::nextVertex()
{
    sceneModel.selectNextVertex();
}

void EngineerGeometryElementsComponent::commitVertexPosition()
{
    const auto current = sceneModel.getSelectedVertexPosition();
    sceneModel.setSelectedVertexPosition({ EngineerUnits::parseLengthMeters(vertexXEditor.getText(), current.x),
                                           EngineerUnits::parseLengthMeters(vertexYEditor.getText(), current.y),
                                           EngineerUnits::parseLengthMeters(vertexZEditor.getText(), current.z) });
}
