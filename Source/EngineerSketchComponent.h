#pragma once

#include <JuceHeader.h>

#include "creation/engineering/SpecLibrary.h"

#include "EngineerSceneModel.h"

// Sketch Mode's panel: choose a plane and begin a sketch, place boundary
// points/holes by clicking in any viewport (see EngineerViewportComponent::
// mouseDown's sketch-mode branch) or by typing exact (u,v) values here, set
// a thickness, then extrude -- which creates a new reusable
// creation::engineering::CustomPartSpec (see finishSketch) and places its
// first instance, rather than one-off scene geometry. Follows
// EngineerLibraryComponent's exact shape (it also needs a mutable
// SpecLibrary& and an onUserLibraryChanged callback, since finishing a
// sketch appends a new library entry the same way EngineerLibraryComponent::
// saveNewEntry() does for user-authored profiles).
//
// v1 numeric editing only reaches whichever boundary point/hole was most
// recently placed/selected (EngineerSceneModel::ActiveSketch::
// selectedBoundaryPointIndex/selectedHoleIndex) -- reselecting an earlier
// point to numerically correct it isn't supported yet (see the sketch-
// modeling plan's Future work).
class EngineerSketchComponent final : public juce::Component,
                                      private EngineerSceneModel::Listener
{
public:
    EngineerSketchComponent(EngineerSceneModel& sceneModel, creation::engineering::SpecLibrary& userLibrary,
                            std::function<void()> onUserLibraryChanged);
    ~EngineerSketchComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void engineerSceneModelChanged() override;
    void refreshFromScene();

    void beginSketch();
    void cancelSketch();
    void commitSelectedBoundaryPoint();
    void removeSelectedBoundaryPoint();
    void commitSelectedHole();
    void removeSelectedHole();
    void commitThickness();
    void finishSketch();

    EngineerSceneModel& sceneModel_;
    creation::engineering::SpecLibrary& userLibrary_;
    std::function<void()> onUserLibraryChanged_;

    juce::Label titleLabel_;
    juce::Label hintLabel_;

    juce::Label planeLabel_;
    juce::ComboBox planeBox_;
    juce::TextButton beginSketchButton_ { "Begin Sketch" };
    juce::TextButton cancelSketchButton_ { "Cancel Sketch" };

    juce::Label boundaryCountLabel_;
    juce::Label boundaryPointLabel_;
    juce::TextEditor boundaryUEditor_;
    juce::TextEditor boundaryVEditor_;
    juce::TextButton removeBoundaryPointButton_ { "Remove Selected Point" };

    juce::ToggleButton placeHoleToggle_ { "Place Hole (instead of boundary point)" };
    juce::Label holeCountLabel_;
    juce::Label holeLabel_;
    juce::TextEditor holeUEditor_;
    juce::TextEditor holeVEditor_;
    juce::TextEditor holeDiameterEditor_;
    juce::TextButton removeHoleButton_ { "Remove Selected Hole" };

    juce::Label thicknessLabel_;
    juce::TextEditor thicknessEditor_;

    juce::Label nameLabel_;
    juce::TextEditor nameEditor_;
    juce::TextButton finishSketchButton_ { "Extrude && Save as Reusable Part" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerSketchComponent)
};
