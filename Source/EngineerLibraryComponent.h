#pragma once

#include <JuceHeader.h>

#include "creation/engineering/SpecLibrary.h"

#include "EngineerSceneModel.h"

// Browse + place parts from the combined (builtin + user) part library, and
// author new user-library entries. Not an EngineerSceneModel::Listener --
// the library isn't scene state -- but it does call into sceneModel to place
// new library-backed/connector objects. See the parametric-part-libraries
// plan's Part F.
class EngineerLibraryComponent final : public juce::Component,
                                       private juce::ListBoxModel
{
public:
    // browseLibrary is the combined library (builtins + user), read-only
    // here -- placement/browsing never mutates it directly. userLibrary is
    // the user-authored-only subset MainComponent persists; onUserLibraryChanged
    // fires after a new entry is appended so MainComponent can persist it and
    // rebuild the combined library other components reference.
    EngineerLibraryComponent(EngineerSceneModel& sceneModel,
                             const creation::engineering::SpecLibrary& browseLibrary,
                             creation::engineering::SpecLibrary& userLibrary,
                             std::function<void()> onUserLibraryChanged);
    ~EngineerLibraryComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void selectedRowsChanged(int lastRowSelected) override;

    void refreshLists();
    void refreshMaterialBox();
    void placeSelectedProfile();
    void placeSelectedConnector();
    void toggleAddEntryForm();
    void saveNewEntry();

    EngineerSceneModel& sceneModel_;
    const creation::engineering::SpecLibrary& browseLibrary_;
    creation::engineering::SpecLibrary& userLibrary_;
    std::function<void()> onUserLibraryChanged_;

    juce::Label titleLabel_;
    juce::Label hintLabel_;

    juce::ListBox profileList_;
    int selectedProfileIndex_ = -1;
    juce::ComboBox materialBox_;
    juce::TextEditor lengthEditor_;
    juce::TextButton placeProfileButton_ { "Place in Scene" };

    juce::ComboBox connectorBox_;
    juce::TextButton placeConnectorButton_ { "Place Connector" };

    juce::TextButton addEntryToggleButton_ { "New Library Entry" };
    bool addEntryFormVisible_ = false;

    juce::Label newProfileLabel_;
    juce::TextEditor newDisplayNameEditor_;
    juce::TextEditor newFamilyNameEditor_;
    juce::TextEditor newOuterWidthEditor_;
    juce::TextEditor newOuterHeightEditor_;
    juce::TextEditor newSlotOpeningEditor_;
    juce::TextEditor newSlotChannelEditor_;
    juce::TextEditor newSlotDepthEditor_;
    juce::TextEditor newManufacturerEditor_;
    juce::TextEditor newPartNumberEditor_;

    juce::Label newMaterialLabel_;
    juce::TextEditor newAlloyDesignationEditor_;
    juce::TextEditor newDensityEditor_;
    juce::TextEditor newYieldStrengthEditor_;
    juce::TextEditor newUltimateStrengthEditor_;
    juce::TextEditor newElasticModulusEditor_;
    juce::TextEditor newShearModulusEditor_;
    juce::TextEditor newPoissonsRatioEditor_;

    juce::TextButton saveNewEntryButton_ { "Save Entry" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerLibraryComponent)
};
