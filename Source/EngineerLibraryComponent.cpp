#include "EngineerLibraryComponent.h"

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

juce::String sourceTagFor(const creation::engineering::SpecSource& source)
{
    if (source.kind == creation::engineering::SpecSourceKind::userAuthored)
    {
        juce::String tag = source.manufacturer.isNotEmpty() ? source.manufacturer : "User";
        if (source.partNumber.isNotEmpty())
            tag << " " << source.partNumber;
        return tag;
    }

    return "Generic";
}
}

EngineerLibraryComponent::EngineerLibraryComponent(EngineerSceneModel& sceneModel,
                                                   const creation::engineering::SpecLibrary& browseLibrary,
                                                   creation::engineering::SpecLibrary& userLibrary,
                                                   std::function<void()> onUserLibraryChanged)
    : sceneModel_(sceneModel), browseLibrary_(browseLibrary), userLibrary_(userLibrary),
      onUserLibraryChanged_(std::move(onUserLibraryChanged))
{
    titleLabel_.setText("Part Library", juce::dontSendNotification);
    titleLabel_.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel_.setFont(juce::Font(juce::FontOptions(18.0f)).boldened());
    addAndMakeVisible(titleLabel_);

    hintLabel_.setText("Browse profiles, place a part, or add your own spec from a catalog.", juce::dontSendNotification);
    hintLabel_.setColour(juce::Label::textColourId, juce::Colour(0xffaebed0));
    addAndMakeVisible(hintLabel_);

    profileList_.setModel(this);
    profileList_.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xff111923));
    profileList_.setOutlineThickness(0);
    addAndMakeVisible(profileList_);

    materialBox_.setTextWhenNoChoicesAvailable("No materials");
    addAndMakeVisible(materialBox_);

    configureEditor(lengthEditor_, "Length (m)");
    lengthEditor_.setText("0.5", juce::dontSendNotification);
    addAndMakeVisible(lengthEditor_);

    configureButton(placeProfileButton_);
    placeProfileButton_.onClick = [this] { placeSelectedProfile(); };
    addAndMakeVisible(placeProfileButton_);

    connectorBox_.setTextWhenNoChoicesAvailable("No connectors");
    addAndMakeVisible(connectorBox_);

    configureButton(placeConnectorButton_);
    placeConnectorButton_.onClick = [this] { placeSelectedConnector(); };
    addAndMakeVisible(placeConnectorButton_);

    configureButton(placeConnectorAlignedButton_);
    placeConnectorAlignedButton_.onClick = [this] { placeSelectedConnectorAlignedToSelection(); };
    addAndMakeVisible(placeConnectorAlignedButton_);

    configureButton(snapToRailButton_);
    snapToRailButton_.onClick = [this] { snapSelectedConnectorToSelectedRail(); };
    addAndMakeVisible(snapToRailButton_);

    customPartBox_.setTextWhenNoChoicesAvailable("No custom parts yet");
    addAndMakeVisible(customPartBox_);

    configureButton(placeCustomPartButton_);
    placeCustomPartButton_.onClick = [this] { placeSelectedCustomPart(); };
    addAndMakeVisible(placeCustomPartButton_);

    configureButton(addEntryToggleButton_);
    addEntryToggleButton_.onClick = [this] { toggleAddEntryForm(); };
    addAndMakeVisible(addEntryToggleButton_);

    configureCaption(newProfileLabel_, "New Profile (from a catalog page)");
    addAndMakeVisible(newProfileLabel_);
    configureEditor(newDisplayNameEditor_, "Display name");
    configureEditor(newFamilyNameEditor_, "Family (e.g. 2020)");
    configureEditor(newOuterWidthEditor_, "Outer width (mm)");
    configureEditor(newOuterHeightEditor_, "Outer height (mm)");
    configureEditor(newSlotOpeningEditor_, "Slot opening (mm)");
    configureEditor(newSlotChannelEditor_, "Slot channel (mm)");
    configureEditor(newSlotDepthEditor_, "Slot depth (mm)");
    configureEditor(newManufacturerEditor_, "Manufacturer");
    configureEditor(newPartNumberEditor_, "Part number");
    for (auto* editor : { &newDisplayNameEditor_, &newFamilyNameEditor_, &newOuterWidthEditor_,
                          &newOuterHeightEditor_, &newSlotOpeningEditor_, &newSlotChannelEditor_,
                          &newSlotDepthEditor_, &newManufacturerEditor_, &newPartNumberEditor_ })
        addChildComponent(*editor);

    configureCaption(newMaterialLabel_, "New Material");
    addAndMakeVisible(newMaterialLabel_);
    newMaterialLabel_.setVisible(false);
    configureEditor(newAlloyDesignationEditor_, "Alloy designation");
    configureEditor(newDensityEditor_, "Density (kg/m^3)");
    configureEditor(newYieldStrengthEditor_, "Yield strength (MPa)");
    configureEditor(newUltimateStrengthEditor_, "Ultimate strength (MPa)");
    configureEditor(newElasticModulusEditor_, "Elastic modulus (GPa)");
    configureEditor(newShearModulusEditor_, "Shear modulus (GPa)");
    configureEditor(newPoissonsRatioEditor_, "Poisson's ratio");
    for (auto* editor : { &newAlloyDesignationEditor_, &newDensityEditor_, &newYieldStrengthEditor_,
                          &newUltimateStrengthEditor_, &newElasticModulusEditor_, &newShearModulusEditor_,
                          &newPoissonsRatioEditor_ })
        addChildComponent(*editor);

    configureButton(saveNewEntryButton_);
    saveNewEntryButton_.onClick = [this] { saveNewEntry(); };
    addChildComponent(saveNewEntryButton_);

    refreshLists();
}

EngineerLibraryComponent::~EngineerLibraryComponent() = default;

void EngineerLibraryComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101722));
}

void EngineerLibraryComponent::resized()
{
    auto area = getLocalBounds().reduced(12);
    titleLabel_.setBounds(area.removeFromTop(24));
    hintLabel_.setBounds(area.removeFromTop(22));
    area.removeFromTop(8);

    profileList_.setBounds(area.removeFromTop(140));
    area.removeFromTop(8);

    auto materialRow = area.removeFromTop(26);
    materialBox_.setBounds(materialRow.removeFromLeft(materialRow.getWidth() * 2 / 3));
    materialRow.removeFromLeft(8);
    lengthEditor_.setBounds(materialRow);
    area.removeFromTop(8);
    placeProfileButton_.setBounds(area.removeFromTop(28));
    area.removeFromTop(14);

    auto connectorRow = area.removeFromTop(26);
    connectorBox_.setBounds(connectorRow.removeFromLeft(connectorRow.getWidth() * 2 / 3));
    area.removeFromTop(8);
    placeConnectorButton_.setBounds(area.removeFromTop(28));
    area.removeFromTop(8);
    placeConnectorAlignedButton_.setBounds(area.removeFromTop(28));
    area.removeFromTop(8);
    snapToRailButton_.setBounds(area.removeFromTop(28));
    area.removeFromTop(14);

    auto customPartRow = area.removeFromTop(26);
    customPartBox_.setBounds(customPartRow.removeFromLeft(customPartRow.getWidth() * 2 / 3));
    area.removeFromTop(8);
    placeCustomPartButton_.setBounds(area.removeFromTop(28));
    area.removeFromTop(14);

    addEntryToggleButton_.setBounds(area.removeFromTop(28));
    area.removeFromTop(8);

    if (!addEntryFormVisible_)
        return;

    newProfileLabel_.setBounds(area.removeFromTop(18));
    newDisplayNameEditor_.setBounds(area.removeFromTop(24));
    area.removeFromTop(4);
    newFamilyNameEditor_.setBounds(area.removeFromTop(24));
    area.removeFromTop(4);

    auto dimRow1 = area.removeFromTop(24);
    newOuterWidthEditor_.setBounds(dimRow1.removeFromLeft(dimRow1.getWidth() / 2 - 4));
    dimRow1.removeFromLeft(8);
    newOuterHeightEditor_.setBounds(dimRow1);
    area.removeFromTop(4);

    auto dimRow2 = area.removeFromTop(24);
    newSlotOpeningEditor_.setBounds(dimRow2.removeFromLeft(dimRow2.getWidth() / 2 - 4));
    dimRow2.removeFromLeft(8);
    newSlotChannelEditor_.setBounds(dimRow2);
    area.removeFromTop(4);

    newSlotDepthEditor_.setBounds(area.removeFromTop(24));
    area.removeFromTop(4);
    newManufacturerEditor_.setBounds(area.removeFromTop(24));
    area.removeFromTop(4);
    newPartNumberEditor_.setBounds(area.removeFromTop(24));
    area.removeFromTop(10);

    newMaterialLabel_.setBounds(area.removeFromTop(18));
    newAlloyDesignationEditor_.setBounds(area.removeFromTop(24));
    area.removeFromTop(4);

    auto matRow1 = area.removeFromTop(24);
    newDensityEditor_.setBounds(matRow1.removeFromLeft(matRow1.getWidth() / 2 - 4));
    matRow1.removeFromLeft(8);
    newYieldStrengthEditor_.setBounds(matRow1);
    area.removeFromTop(4);

    auto matRow2 = area.removeFromTop(24);
    newUltimateStrengthEditor_.setBounds(matRow2.removeFromLeft(matRow2.getWidth() / 2 - 4));
    matRow2.removeFromLeft(8);
    newElasticModulusEditor_.setBounds(matRow2);
    area.removeFromTop(4);

    auto matRow3 = area.removeFromTop(24);
    newShearModulusEditor_.setBounds(matRow3.removeFromLeft(matRow3.getWidth() / 2 - 4));
    matRow3.removeFromLeft(8);
    newPoissonsRatioEditor_.setBounds(matRow3);
    area.removeFromTop(8);

    saveNewEntryButton_.setBounds(area.removeFromTop(28));
}

int EngineerLibraryComponent::getNumRows()
{
    return static_cast<int>(browseLibrary_.profiles.size());
}

void EngineerLibraryComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    auto bounds = juce::Rectangle<int>(0, 0, width, height).reduced(4, 2);
    g.setColour(rowIsSelected ? juce::Colour(0xff1e3044) : juce::Colour(0xff15202c));
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);

    if (rowNumber < 0 || rowNumber >= static_cast<int>(browseLibrary_.profiles.size()))
        return;

    const auto& profile = browseLibrary_.profiles[static_cast<size_t>(rowNumber)];
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(14.0f)).boldened());
    g.drawText(profile.displayName, bounds.removeFromTop(18), juce::Justification::centredLeft, true);

    g.setColour(juce::Colour(0xff9eb4c9));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText(profile.familyName + "  |  " + sourceTagFor(profile.source), bounds, juce::Justification::centredLeft, true);
}

void EngineerLibraryComponent::selectedRowsChanged(int lastRowSelected)
{
    selectedProfileIndex_ = lastRowSelected;
}

void EngineerLibraryComponent::refreshLists()
{
    profileList_.updateContent();
    if (selectedProfileIndex_ < 0 && !browseLibrary_.profiles.empty())
        selectedProfileIndex_ = 0;
    profileList_.selectRow(selectedProfileIndex_, juce::dontSendNotification);

    refreshMaterialBox();

    connectorBox_.clear(juce::dontSendNotification);
    for (int i = 0; i < static_cast<int>(browseLibrary_.connectors.size()); ++i)
    {
        const auto& connector = browseLibrary_.connectors[static_cast<size_t>(i)];
        connectorBox_.addItem(connector.displayName, i + 1);
    }
    if (connectorBox_.getNumItems() > 0)
        connectorBox_.setSelectedItemIndex(0, juce::dontSendNotification);

    customPartBox_.clear(juce::dontSendNotification);
    for (int i = 0; i < static_cast<int>(browseLibrary_.customParts.size()); ++i)
    {
        const auto& part = browseLibrary_.customParts[static_cast<size_t>(i)];
        customPartBox_.addItem(part.displayName, i + 1);
    }
    if (customPartBox_.getNumItems() > 0)
        customPartBox_.setSelectedItemIndex(0, juce::dontSendNotification);
}

void EngineerLibraryComponent::refreshMaterialBox()
{
    materialBox_.clear(juce::dontSendNotification);
    for (int i = 0; i < static_cast<int>(browseLibrary_.materials.size()); ++i)
    {
        const auto& material = browseLibrary_.materials[static_cast<size_t>(i)];
        materialBox_.addItem(material.displayName, i + 1);
    }
    if (materialBox_.getNumItems() > 0)
        materialBox_.setSelectedItemIndex(0, juce::dontSendNotification);
}

void EngineerLibraryComponent::placeSelectedProfile()
{
    if (selectedProfileIndex_ < 0 || selectedProfileIndex_ >= static_cast<int>(browseLibrary_.profiles.size()))
        return;

    const auto materialIndex = materialBox_.getSelectedItemIndex();
    if (materialIndex < 0 || materialIndex >= static_cast<int>(browseLibrary_.materials.size()))
        return;

    const auto& profile = browseLibrary_.profiles[static_cast<size_t>(selectedProfileIndex_)];
    const auto& material = browseLibrary_.materials[static_cast<size_t>(materialIndex)];
    const auto lengthMeters = juce::jmax(0.01f, EngineerUnits::parseLengthMeters(lengthEditor_.getText(), 0.5f));

    const juce::Vector3D<float> initialSize { profile.outerWidthMm / 1000.0f, lengthMeters,
                                              profile.outerHeightMm / 1000.0f };
    sceneModel_.addLibraryPartObject(profile.id, material.id, lengthMeters, initialSize);
}

void EngineerLibraryComponent::placeSelectedConnector()
{
    const auto connectorIndex = connectorBox_.getSelectedItemIndex();
    if (connectorIndex < 0 || connectorIndex >= static_cast<int>(browseLibrary_.connectors.size()))
        return;

    const auto& connector = browseLibrary_.connectors[static_cast<size_t>(connectorIndex)];
    const juce::Vector3D<float> size { connector.sizeMmX / 1000.0f, connector.sizeMmY / 1000.0f,
                                       connector.sizeMmZ / 1000.0f };
    sceneModel_.addConnectorObject(connector.id, size);
}

void EngineerLibraryComponent::placeSelectedConnectorAlignedToSelection()
{
    // Read before placing -- addConnectorObject below re-selects the new
    // connector (see EngineerSceneModel::addConnectorObject), so the
    // rotation to copy has to be captured first.
    const auto rotationToCopy = sceneModel_.getSelectedObjectRotationDegrees();
    placeSelectedConnector();
    sceneModel_.setSelectedObjectRotationDegrees(rotationToCopy);
}

// Places the connectorBox_ selection as a new DIN module flush against the
// current far end of whatever's already mounted on the selected rail --
// finds that far end by scanning already-mounted modules and taking the max
// of their actual current positions (projected onto the rail's world-space
// length axis), not by summing nominal widths in insertion order: a running
// sum silently breaks once any middle module is deleted (an accepted v1 gap
// -- gaps are left, not auto-closed), since the sum would under-count and
// the next placement would land before the true end of the strip, overlapping
// a module sitting beyond the gap. Reading ground-truth positions is
// self-correcting regardless of deletion/insertion order.
void EngineerLibraryComponent::snapSelectedConnectorToSelectedRail()
{
    const auto connectorIndex = connectorBox_.getSelectedItemIndex();
    if (connectorIndex < 0 || connectorIndex >= static_cast<int>(browseLibrary_.connectors.size()))
        return;
    const auto& connector = browseLibrary_.connectors[static_cast<size_t>(connectorIndex)];

    const auto& rail = sceneModel_.getSelectedObject();
    if (!rail.libraryPart.has_value())
        return;
    const auto* railProfile = browseLibrary_.findProfile(rail.libraryPart->profileId);
    if (railProfile == nullptr || railProfile->kind != creation::engineering::ProfileKind::dinRailTopHat)
        return;

    // Rail's world-space length axis: Matrix3D<float>::rotation's column-
    // major layout means "R applied to local +Y" is exactly its 2nd column.
    const juce::Vector3D<float> rotationRadians { juce::degreesToRadians(rail.rotationDegrees.x),
                                                  juce::degreesToRadians(rail.rotationDegrees.y),
                                                  juce::degreesToRadians(rail.rotationDegrees.z) };
    const auto railRotation = juce::Matrix3D<float>::rotation(rotationRadians);
    const juce::Vector3D<float> lengthAxis { railRotation.mat[4], railRotation.mat[5], railRotation.mat[6] };

    // rail.size.y stays synced to the live LibraryPartState::lengthMeters
    // (see EngineerSceneModel::setSelectedLibraryPartLength), so no
    // ProfileSpec re-lookup needed for the rail's own length.
    const auto railHalfLength = rail.size.y * 0.5f;
    float currentStripEndLocalY = -railHalfLength;
    for (const auto& object : sceneModel_.getObjects())
    {
        if (object.mountedOnObjectId != rail.objectId)
            continue;

        const float dx = object.position.x - rail.position.x;
        const float dy = object.position.y - rail.position.y;
        const float dz = object.position.z - rail.position.z;
        const float localY = dx * lengthAxis.x + dy * lengthAxis.y + dz * lengthAxis.z; // projection onto a unit axis
        currentStripEndLocalY = juce::jmax(currentStripEndLocalY, localY + object.size.y * 0.5f);
    }

    const auto newModuleWidthMeters = connector.sizeMmY / 1000.0f; // along-rail axis -- see ConnectorSpec.h
    const auto newLocalY = currentStripEndLocalY + newModuleWidthMeters * 0.5f;
    const juce::Vector3D<float> newPosition { rail.position.x + lengthAxis.x * newLocalY,
                                              rail.position.y + lengthAxis.y * newLocalY,
                                              rail.position.z + lengthAxis.z * newLocalY };

    const juce::Vector3D<float> size { connector.sizeMmX / 1000.0f, connector.sizeMmY / 1000.0f,
                                       connector.sizeMmZ / 1000.0f };
    sceneModel_.addConnectorObjectMountedOnRail(connector.id, size, rail.objectId, newPosition, rail.rotationDegrees);
}

void EngineerLibraryComponent::placeSelectedCustomPart()
{
    const auto partIndex = customPartBox_.getSelectedItemIndex();
    if (partIndex < 0 || partIndex >= static_cast<int>(browseLibrary_.customParts.size()))
        return;

    const auto& part = browseLibrary_.customParts[static_cast<size_t>(partIndex)];
    if (part.boundaryUV.empty())
        return;

    auto minimum = part.boundaryUV.front();
    auto maximum = part.boundaryUV.front();
    for (const auto& p : part.boundaryUV)
    {
        minimum.x = juce::jmin(minimum.x, p.x);
        minimum.y = juce::jmin(minimum.y, p.y);
        maximum.x = juce::jmax(maximum.x, p.x);
        maximum.y = juce::jmax(maximum.y, p.y);
    }

    const juce::Vector3D<float> size { maximum.x - minimum.x, maximum.y - minimum.y, part.thicknessMeters };
    sceneModel_.addCustomPartObject(part.id, size);
}

void EngineerLibraryComponent::toggleAddEntryForm()
{
    addEntryFormVisible_ = !addEntryFormVisible_;

    for (auto* editor : { &newDisplayNameEditor_, &newFamilyNameEditor_, &newOuterWidthEditor_,
                          &newOuterHeightEditor_, &newSlotOpeningEditor_, &newSlotChannelEditor_,
                          &newSlotDepthEditor_, &newManufacturerEditor_, &newPartNumberEditor_,
                          &newAlloyDesignationEditor_, &newDensityEditor_, &newYieldStrengthEditor_,
                          &newUltimateStrengthEditor_, &newElasticModulusEditor_, &newShearModulusEditor_,
                          &newPoissonsRatioEditor_ })
        editor->setVisible(addEntryFormVisible_);

    newProfileLabel_.setVisible(addEntryFormVisible_);
    newMaterialLabel_.setVisible(addEntryFormVisible_);
    saveNewEntryButton_.setVisible(addEntryFormVisible_);

    resized();
}

void EngineerLibraryComponent::saveNewEntry()
{
    if (newDisplayNameEditor_.getText().trim().isEmpty() || newFamilyNameEditor_.getText().trim().isEmpty())
        return;

    using namespace creation::engineering;

    MaterialSpec material;
    material.id = juce::Uuid().toString();
    material.displayName = newAlloyDesignationEditor_.getText().trim().isNotEmpty()
                              ? newAlloyDesignationEditor_.getText().trim()
                              : (newDisplayNameEditor_.getText().trim() + " Material");
    material.alloyDesignation = newAlloyDesignationEditor_.getText().trim();
    material.densityKgM3 = newDensityEditor_.getText().getFloatValue();
    material.yieldStrengthMPa = newYieldStrengthEditor_.getText().getFloatValue();
    material.ultimateTensileStrengthMPa = newUltimateStrengthEditor_.getText().getFloatValue();
    material.elasticModulusGPa = newElasticModulusEditor_.getText().getFloatValue();
    material.shearModulusGPa = newShearModulusEditor_.getText().getFloatValue();
    material.poissonsRatio = newPoissonsRatioEditor_.getText().getFloatValue();
    material.source.kind = SpecSourceKind::userAuthored;
    material.source.manufacturer = newManufacturerEditor_.getText().trim();
    material.source.partNumber = newPartNumberEditor_.getText().trim();
    userLibrary_.materials.push_back(material);

    ProfileSpec profile;
    profile.id = juce::Uuid().toString();
    profile.displayName = newDisplayNameEditor_.getText().trim();
    profile.kind = ProfileKind::tSlotExtrusion;
    profile.familyName = newFamilyNameEditor_.getText().trim();
    profile.outerWidthMm = newOuterWidthEditor_.getText().getFloatValue();
    profile.outerHeightMm = newOuterHeightEditor_.getText().getFloatValue();
    profile.slotOpeningWidthMm = newSlotOpeningEditor_.getText().getFloatValue();
    profile.slotChannelWidthMm = newSlotChannelEditor_.getText().getFloatValue();
    profile.slotDepthMm = newSlotDepthEditor_.getText().getFloatValue();
    profile.wallThicknessMm = 1.5f;
    profile.centerBoreDiameterMm = 0.0f;
    profile.slotsPerSide = 1;
    profile.defaultMaterialId = material.id;
    profile.source.kind = SpecSourceKind::userAuthored;
    profile.source.manufacturer = newManufacturerEditor_.getText().trim();
    profile.source.partNumber = newPartNumberEditor_.getText().trim();
    userLibrary_.profiles.push_back(profile);

    if (onUserLibraryChanged_)
        onUserLibraryChanged_();

    toggleAddEntryForm();
    refreshLists();
}
