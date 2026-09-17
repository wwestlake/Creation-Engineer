#include "EngineerLayersComponent.h"

namespace
{
// One layer's interactive row: visibility toggle, colour swatch (opens a
// juce::ColourSelector in a CallOutBox -- the suite's first use of that
// component), name editor, opacity slider, remove button. Recycled by
// juce::ListBox via refreshComponentForRow, so it's rebound to a different
// layer id on every call rather than constructed fresh per row.
class LayerRowComponent final : public juce::Component,
                                private juce::ChangeListener
{
public:
    explicit LayerRowComponent(EngineerSceneModel& model) : sceneModel(model)
    {
        nameEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff111923));
        nameEditor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff314155));
        nameEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
        nameEditor.onFocusLost = [this] { sceneModel.renameLayer(layerId, nameEditor.getText().trim()); };
        nameEditor.onReturnKey = [this] { sceneModel.renameLayer(layerId, nameEditor.getText().trim()); };
        addAndMakeVisible(nameEditor);

        visibleToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        visibleToggle.onClick = [this] { sceneModel.setLayerVisible(layerId, visibleToggle.getToggleState()); };
        addAndMakeVisible(visibleToggle);

        swatchButton.onClick = [this] { showColourPicker(); };
        addAndMakeVisible(swatchButton);

        opacitySlider.setRange(0.0, 1.0, 0.01);
        opacitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
        opacitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 44, 20);
        opacitySlider.onValueChange = [this] { sceneModel.setLayerOpacity(layerId, static_cast<float>(opacitySlider.getValue())); };
        addAndMakeVisible(opacitySlider);

        removeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a2533));
        removeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        removeButton.onClick = [this] { sceneModel.removeLayer(layerId); };
        addAndMakeVisible(removeButton);
    }

    void updateForLayer(const EngineerSceneModel::Layer& layer)
    {
        layerId = layer.id;
        nameEditor.setText(layer.name, juce::dontSendNotification);
        visibleToggle.setToggleState(layer.visible, juce::dontSendNotification);
        swatchButton.setColour(juce::TextButton::buttonColourId, layer.tint);
        opacitySlider.setValue(layer.opacity, juce::dontSendNotification);
        removeButton.setEnabled(sceneModel.canRemoveLayer(layerId));
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(4, 2);
        visibleToggle.setBounds(area.removeFromLeft(24));
        area.removeFromLeft(4);
        swatchButton.setBounds(area.removeFromLeft(24));
        area.removeFromLeft(6);
        removeButton.setBounds(area.removeFromRight(24));
        area.removeFromRight(4);
        opacitySlider.setBounds(area.removeFromRight(110));
        area.removeFromRight(6);
        nameEditor.setBounds(area);
    }

private:
    void showColourPicker()
    {
        auto selector = std::make_unique<juce::ColourSelector>(juce::ColourSelector::showColourAtTop
                                                                | juce::ColourSelector::editableColour);
        selector->setSize(300, 300);
        selector->setCurrentColour(swatchButton.findColour(juce::TextButton::buttonColourId));
        selector->addChangeListener(this);
        juce::CallOutBox::launchAsynchronously(std::move(selector), swatchButton.getScreenBounds(), nullptr);
    }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override
    {
        if (auto* selector = dynamic_cast<juce::ColourSelector*>(source))
        {
            swatchButton.setColour(juce::TextButton::buttonColourId, selector->getCurrentColour());
            sceneModel.setLayerTint(layerId, selector->getCurrentColour());
        }
    }

    EngineerSceneModel& sceneModel;
    juce::String layerId;
    juce::TextEditor nameEditor;
    juce::ToggleButton visibleToggle;
    juce::TextButton swatchButton;
    juce::Slider opacitySlider;
    juce::TextButton removeButton { "X" };
};
}

EngineerLayersComponent::EngineerLayersComponent(EngineerSceneModel& model)
    : sceneModel(model)
{
    sceneModel.addListener(this);

    titleLabel.setText("Layers", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::Font(juce::FontOptions(18.0f)).boldened());
    addAndMakeVisible(titleLabel);

    layerList.setModel(this);
    layerList.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff111923));
    layerList.setOutlineThickness(0);
    layerList.setRowHeight(30);
    addAndMakeVisible(layerList);

    addLayerButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a2533));
    addLayerButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addLayerButton.onClick = [this] { addLayerClicked(); };
    addAndMakeVisible(addLayerButton);
}

EngineerLayersComponent::~EngineerLayersComponent()
{
    sceneModel.removeListener(this);
}

void EngineerLayersComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101722));
}

void EngineerLayersComponent::resized()
{
    auto area = getLocalBounds().reduced(12);
    titleLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);
    addLayerButton.setBounds(area.removeFromBottom(28));
    area.removeFromBottom(8);
    layerList.setBounds(area);
}

void EngineerLayersComponent::engineerSceneModelChanged()
{
    layerList.updateContent();
    repaint();
}

int EngineerLayersComponent::getNumRows()
{
    return static_cast<int>(sceneModel.getLayers().size());
}

juce::Component* EngineerLayersComponent::refreshComponentForRow(int rowNumber, bool,
                                                                  juce::Component* existingComponentToUpdate)
{
    auto* row = dynamic_cast<LayerRowComponent*>(existingComponentToUpdate);
    if (row == nullptr)
    {
        delete existingComponentToUpdate;
        row = new LayerRowComponent(sceneModel);
    }

    const auto& layers = sceneModel.getLayers();
    if (rowNumber >= 0 && rowNumber < static_cast<int>(layers.size()))
        row->updateForLayer(layers[static_cast<size_t>(rowNumber)]);

    return row;
}

void EngineerLayersComponent::addLayerClicked()
{
    sceneModel.addLayer("Layer " + juce::String(sceneModel.getLayers().size() + 1));
}
