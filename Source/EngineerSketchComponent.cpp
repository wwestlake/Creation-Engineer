#include "EngineerSketchComponent.h"

#include "EngineerUnits.h"

namespace
{
void configureCaption(juce::Label& label, const juce::String& text)
{
    label.setText(text, juce::dontSendNotification);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffaebed0));
}

void configureEditor(juce::TextEditor& editor, const juce::String& placeholder = {})
{
    editor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff111923));
    editor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff314155));
    editor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    if (placeholder.isNotEmpty())
        editor.setTextToShowWhenEmpty(placeholder, juce::Colour(0xff5c6b7d));
}

void configureButton(juce::TextButton& button)
{
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a2533));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff25354a));
    button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}
}

EngineerSketchComponent::EngineerSketchComponent(EngineerSceneModel& sceneModel,
                                                 creation::engineering::SpecLibrary& userLibrary,
                                                 std::function<void()> onUserLibraryChanged)
    : sceneModel_(sceneModel), userLibrary_(userLibrary), onUserLibraryChanged_(std::move(onUserLibraryChanged))
{
    sceneModel_.addListener(this);

    titleLabel_.setText("Sketch", juce::dontSendNotification);
    titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel_.setFont(juce::Font(juce::FontOptions(18.0f)).boldened());
    addAndMakeVisible(titleLabel_);

    hintLabel_.setText("Choose a plane, begin, then click in a viewport to place points/holes.", juce::dontSendNotification);
    hintLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffaebed0));
    addAndMakeVisible(hintLabel_);

    configureCaption(planeLabel_, "Plane");
    addAndMakeVisible(planeLabel_);
    planeBox_.addItem("XY", 1);
    planeBox_.addItem("XZ (ground)", 2);
    planeBox_.addItem("YZ", 3);
    planeBox_.setSelectedId(2, juce::dontSendNotification);
    addAndMakeVisible(planeBox_);

    configureButton(beginSketchButton_);
    beginSketchButton_.onClick = [this] { beginSketch(); };
    addAndMakeVisible(beginSketchButton_);

    configureButton(cancelSketchButton_);
    cancelSketchButton_.onClick = [this] { cancelSketch(); };
    addChildComponent(cancelSketchButton_);

    configureCaption(boundaryCountLabel_, "Boundary Points: 0");
    addChildComponent(boundaryCountLabel_);
    configureCaption(boundaryPointLabel_, "Selected Point (u, v meters)");
    addChildComponent(boundaryPointLabel_);
    configureEditor(boundaryUEditor_, "u");
    configureEditor(boundaryVEditor_, "v");
    boundaryUEditor_.onFocusLost = [this] { commitSelectedBoundaryPoint(); };
    boundaryUEditor_.onReturnKey = [this] { commitSelectedBoundaryPoint(); };
    boundaryVEditor_.onFocusLost = [this] { commitSelectedBoundaryPoint(); };
    boundaryVEditor_.onReturnKey = [this] { commitSelectedBoundaryPoint(); };
    addChildComponent(boundaryUEditor_);
    addChildComponent(boundaryVEditor_);

    configureButton(removeBoundaryPointButton_);
    removeBoundaryPointButton_.onClick = [this] { removeSelectedBoundaryPoint(); };
    addChildComponent(removeBoundaryPointButton_);

    placeHoleToggle_.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    placeHoleToggle_.onClick = [this] { sceneModel_.setSketchHolePlacementActive(placeHoleToggle_.getToggleState()); };
    addChildComponent(placeHoleToggle_);

    configureCaption(holeCountLabel_, "Holes: 0");
    addChildComponent(holeCountLabel_);
    configureCaption(holeLabel_, "Selected Hole (center u,v + diameter, meters)");
    addChildComponent(holeLabel_);
    configureEditor(holeUEditor_, "u");
    configureEditor(holeVEditor_, "v");
    configureEditor(holeDiameterEditor_, "diameter");
    holeUEditor_.onFocusLost = [this] { commitSelectedHole(); };
    holeUEditor_.onReturnKey = [this] { commitSelectedHole(); };
    holeVEditor_.onFocusLost = [this] { commitSelectedHole(); };
    holeVEditor_.onReturnKey = [this] { commitSelectedHole(); };
    holeDiameterEditor_.onFocusLost = [this] { commitSelectedHole(); };
    holeDiameterEditor_.onReturnKey = [this] { commitSelectedHole(); };
    addChildComponent(holeUEditor_);
    addChildComponent(holeVEditor_);
    addChildComponent(holeDiameterEditor_);

    configureButton(removeHoleButton_);
    removeHoleButton_.onClick = [this] { removeSelectedHole(); };
    addChildComponent(removeHoleButton_);

    configureCaption(thicknessLabel_, "Thickness (m)");
    addChildComponent(thicknessLabel_);
    configureEditor(thicknessEditor_, "3mm");
    thicknessEditor_.onFocusLost = [this] { commitThickness(); };
    thicknessEditor_.onReturnKey = [this] { commitThickness(); };
    addChildComponent(thicknessEditor_);

    configureCaption(nameLabel_, "Part Name");
    addChildComponent(nameLabel_);
    configureEditor(nameEditor_, "Custom Part");
    addChildComponent(nameEditor_);

    configureButton(finishSketchButton_);
    finishSketchButton_.onClick = [this] { finishSketch(); };
    addChildComponent(finishSketchButton_);

    refreshFromScene();
}

EngineerSketchComponent::~EngineerSketchComponent()
{
    sceneModel_.removeListener(this);
}

void EngineerSketchComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101722));
}

void EngineerSketchComponent::resized()
{
    auto area = getLocalBounds().reduced(12);
    titleLabel_.setBounds(area.removeFromTop(24));
    hintLabel_.setBounds(area.removeFromTop(22));
    area.removeFromTop(8);

    planeLabel_.setBounds(area.removeFromTop(18));
    planeBox_.setBounds(area.removeFromTop(26));
    area.removeFromTop(8);
    beginSketchButton_.setBounds(area.removeFromTop(28));
    area.removeFromTop(8);

    if (!sceneModel_.getActiveSketch().active)
        return;

    cancelSketchButton_.setBounds(area.removeFromTop(28));
    area.removeFromTop(14);

    boundaryCountLabel_.setBounds(area.removeFromTop(18));
    boundaryPointLabel_.setBounds(area.removeFromTop(18));
    auto boundaryRow = area.removeFromTop(26);
    boundaryUEditor_.setBounds(boundaryRow.removeFromLeft(boundaryRow.getWidth() / 2 - 4));
    boundaryRow.removeFromLeft(8);
    boundaryVEditor_.setBounds(boundaryRow);
    area.removeFromTop(8);
    removeBoundaryPointButton_.setBounds(area.removeFromTop(26));
    area.removeFromTop(14);

    placeHoleToggle_.setBounds(area.removeFromTop(24));
    area.removeFromTop(8);
    holeCountLabel_.setBounds(area.removeFromTop(18));
    holeLabel_.setBounds(area.removeFromTop(18));
    auto holeRow = area.removeFromTop(26);
    const auto holeFieldWidth = (holeRow.getWidth() - 16) / 3;
    holeUEditor_.setBounds(holeRow.removeFromLeft(holeFieldWidth));
    holeRow.removeFromLeft(8);
    holeVEditor_.setBounds(holeRow.removeFromLeft(holeFieldWidth));
    holeRow.removeFromLeft(8);
    holeDiameterEditor_.setBounds(holeRow);
    area.removeFromTop(8);
    removeHoleButton_.setBounds(area.removeFromTop(26));
    area.removeFromTop(14);

    thicknessLabel_.setBounds(area.removeFromTop(18));
    thicknessEditor_.setBounds(area.removeFromTop(26));
    area.removeFromTop(14);

    nameLabel_.setBounds(area.removeFromTop(18));
    nameEditor_.setBounds(area.removeFromTop(26));
    area.removeFromTop(8);
    finishSketchButton_.setBounds(area.removeFromTop(28));
}

void EngineerSketchComponent::engineerSceneModelChanged()
{
    refreshFromScene();
}

void EngineerSketchComponent::refreshFromScene()
{
    const auto& sketch = sceneModel_.getActiveSketch();
    const auto active = sketch.active;

    planeBox_.setEnabled(!active);
    beginSketchButton_.setEnabled(!active);

    cancelSketchButton_.setVisible(active);
    boundaryCountLabel_.setVisible(active);
    boundaryPointLabel_.setVisible(active);
    boundaryUEditor_.setVisible(active);
    boundaryVEditor_.setVisible(active);
    removeBoundaryPointButton_.setVisible(active);
    placeHoleToggle_.setVisible(active);
    holeCountLabel_.setVisible(active);
    holeLabel_.setVisible(active);
    holeUEditor_.setVisible(active);
    holeVEditor_.setVisible(active);
    holeDiameterEditor_.setVisible(active);
    removeHoleButton_.setVisible(active);
    thicknessLabel_.setVisible(active);
    thicknessEditor_.setVisible(active);
    nameLabel_.setVisible(active);
    nameEditor_.setVisible(active);
    finishSketchButton_.setVisible(active);

    if (!active)
    {
        resized();
        return;
    }

    boundaryCountLabel_.setText("Boundary Points: " + juce::String(sketch.boundaryPoints.size())
                                  + (sceneModel_.canFinishActiveSketch() ? "" : "  (need at least 3)"),
                                juce::dontSendNotification);
    placeHoleToggle_.setToggleState(sketch.holePlacementActive, juce::dontSendNotification);

    const auto boundaryIndex = sketch.selectedBoundaryPointIndex;
    const auto hasBoundarySelection = boundaryIndex >= 0 && boundaryIndex < static_cast<int>(sketch.boundaryPoints.size());
    boundaryUEditor_.setEnabled(hasBoundarySelection);
    boundaryVEditor_.setEnabled(hasBoundarySelection);
    removeBoundaryPointButton_.setEnabled(hasBoundarySelection);
    if (hasBoundarySelection)
    {
        const auto& p = sketch.boundaryPoints[static_cast<size_t>(boundaryIndex)];
        boundaryUEditor_.setText(EngineerUnits::formatLengthMeters(p.x), juce::dontSendNotification);
        boundaryVEditor_.setText(EngineerUnits::formatLengthMeters(p.y), juce::dontSendNotification);
    }

    holeCountLabel_.setText("Holes: " + juce::String(sketch.holes.size()), juce::dontSendNotification);
    const auto holeIndex = sketch.selectedHoleIndex;
    const auto hasHoleSelection = holeIndex >= 0 && holeIndex < static_cast<int>(sketch.holes.size());
    holeUEditor_.setEnabled(hasHoleSelection);
    holeVEditor_.setEnabled(hasHoleSelection);
    holeDiameterEditor_.setEnabled(hasHoleSelection);
    removeHoleButton_.setEnabled(hasHoleSelection);
    if (hasHoleSelection)
    {
        const auto& hole = sketch.holes[static_cast<size_t>(holeIndex)];
        holeUEditor_.setText(EngineerUnits::formatLengthMeters(hole.centerUV.x), juce::dontSendNotification);
        holeVEditor_.setText(EngineerUnits::formatLengthMeters(hole.centerUV.y), juce::dontSendNotification);
        holeDiameterEditor_.setText(EngineerUnits::formatLengthMeters(hole.diameterMeters), juce::dontSendNotification);
    }

    thicknessEditor_.setText(EngineerUnits::formatLengthMeters(sketch.thicknessMeters), juce::dontSendNotification);
    finishSketchButton_.setEnabled(sceneModel_.canFinishActiveSketch());

    resized();
}

void EngineerSketchComponent::beginSketch()
{
    const auto selectedId = planeBox_.getSelectedId();
    const auto plane = selectedId == 1 ? EngineerSceneModel::SketchPlane::xy
                       : selectedId == 3 ? EngineerSceneModel::SketchPlane::yz
                                          : EngineerSceneModel::SketchPlane::xz;
    sceneModel_.beginSketch(plane);
}

void EngineerSketchComponent::cancelSketch()
{
    sceneModel_.cancelSketch();
}

void EngineerSketchComponent::commitSelectedBoundaryPoint()
{
    const auto& sketch = sceneModel_.getActiveSketch();
    const auto index = sketch.selectedBoundaryPointIndex;
    if (index < 0 || index >= static_cast<int>(sketch.boundaryPoints.size()))
        return;

    const auto& current = sketch.boundaryPoints[static_cast<size_t>(index)];
    sceneModel_.setSketchBoundaryPointPosition(index,
        { EngineerUnits::parseLengthMeters(boundaryUEditor_.getText(), current.x),
          EngineerUnits::parseLengthMeters(boundaryVEditor_.getText(), current.y) });
}

void EngineerSketchComponent::removeSelectedBoundaryPoint()
{
    sceneModel_.removeSketchBoundaryPoint(sceneModel_.getActiveSketch().selectedBoundaryPointIndex);
}

void EngineerSketchComponent::commitSelectedHole()
{
    const auto& sketch = sceneModel_.getActiveSketch();
    const auto index = sketch.selectedHoleIndex;
    if (index < 0 || index >= static_cast<int>(sketch.holes.size()))
        return;

    const auto& current = sketch.holes[static_cast<size_t>(index)];
    sceneModel_.setSketchHolePosition(index,
        { EngineerUnits::parseLengthMeters(holeUEditor_.getText(), current.centerUV.x),
          EngineerUnits::parseLengthMeters(holeVEditor_.getText(), current.centerUV.y) });
    sceneModel_.setSketchHoleDiameter(index, EngineerUnits::parseLengthMeters(holeDiameterEditor_.getText(), current.diameterMeters));
}

void EngineerSketchComponent::removeSelectedHole()
{
    sceneModel_.removeSketchHole(sceneModel_.getActiveSketch().selectedHoleIndex);
}

void EngineerSketchComponent::commitThickness()
{
    const auto current = sceneModel_.getActiveSketch().thicknessMeters;
    sceneModel_.setSketchThickness(EngineerUnits::parseLengthMeters(thicknessEditor_.getText(), current));
}

// Bridges Sketch Mode's in-progress ActiveSketch into a real, reusable
// library entry -- EngineerSceneModel is deliberately decoupled from
// shared/EngineeringSpecs (same reasoning as EngineerSceneModel::
// addLibraryPartObject's own decoupling comment), so this component plays
// the same role EngineerLibraryComponent::saveNewEntry() already plays for
// user-authored profiles/materials.
void EngineerSketchComponent::finishSketch()
{
    if (!sceneModel_.canFinishActiveSketch())
        return;

    const auto& sketch = sceneModel_.getActiveSketch();

    using namespace creation::engineering;
    CustomPartSpec spec;
    spec.id = juce::Uuid().toString();
    spec.displayName = nameEditor_.getText().trim().isNotEmpty() ? nameEditor_.getText().trim() : "Custom Part";
    spec.source.kind = SpecSourceKind::userAuthored;
    spec.plane = sketch.plane == EngineerSceneModel::SketchPlane::xy ? SketchPlaneToken::xy
               : sketch.plane == EngineerSceneModel::SketchPlane::yz ? SketchPlaneToken::yz
                                                                     : SketchPlaneToken::xz;
    spec.boundaryUV = sketch.boundaryPoints;
    spec.holes.reserve(sketch.holes.size());
    for (const auto& hole : sketch.holes)
        spec.holes.push_back({ hole.centerUV, hole.diameterMeters });
    spec.thicknessMeters = sketch.thicknessMeters;

    auto minimum = sketch.boundaryPoints.front();
    auto maximum = sketch.boundaryPoints.front();
    for (const auto& p : sketch.boundaryPoints)
    {
        minimum.x = juce::jmin(minimum.x, p.x);
        minimum.y = juce::jmin(minimum.y, p.y);
        maximum.x = juce::jmax(maximum.x, p.x);
        maximum.y = juce::jmax(maximum.y, p.y);
    }
    const juce::Vector3D<float> initialSize { maximum.x - minimum.x, maximum.y - minimum.y, sketch.thicknessMeters };

    userLibrary_.customParts.push_back(spec);
    if (onUserLibraryChanged_)
        onUserLibraryChanged_();

    sceneModel_.addCustomPartObject(spec.id, initialSize);
    sceneModel_.endSketch();

    nameEditor_.setText({}, juce::dontSendNotification);
}
