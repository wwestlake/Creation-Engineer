#pragma once

#include <JuceHeader.h>
#include "EngineerSceneModel.h"

class EngineerViewportComponent final : public juce::Component
                                       , private EngineerSceneModel::Listener
{
public:
    enum class ViewMode
    {
        design,
        assemblyFloor
    };

    explicit EngineerViewportComponent(EngineerSceneModel& sceneModel);
    ~EngineerViewportComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    ViewMode getViewMode() const noexcept;

private:
    struct GeometryHit
    {
        bool valid = false;
        EngineerSceneModel::GeometryElementKind kind = EngineerSceneModel::GeometryElementKind::vertex;
        int index = 0;
    };

    void engineerSceneModelChanged() override;
    juce::Rectangle<float> getViewportBounds() const;
    juce::Rectangle<float> getSceneBounds() const;
    juce::Rectangle<float> getObjectBounds(const EngineerSceneModel::SceneObject& object) const;
    int hitTestObject(juce::Point<float> point) const;
    GeometryHit hitTestGeometryElement(juce::Point<float> point,
                                       const EngineerSceneModel::SceneObject& object,
                                       juce::Rectangle<float> rect) const;
    void configureButton(juce::TextButton& button);
    void updateModeButtons();
    void updatePrimitiveButtons();
    void updateCameraButtons();
    void updateObjectButtons();

    ViewMode viewMode = ViewMode::design;
    juce::String selectedPrimitive { "Block" };
    EngineerSceneModel& sceneModel;
    bool isDraggingObject = false;
    bool isDraggingGeometryElement = false;
    juce::Point<float> dragAnchor;
    juce::Point<float> dragStartPosition;

    juce::TextButton designViewButton { "Design View" };
    juce::TextButton assemblyFloorButton { "Assembly Floor" };
    juce::TextButton isoCameraButton { "ISO" };
    juce::TextButton topCameraButton { "Top" };
    juce::TextButton walkCameraButton { "Walk" };
    juce::TextButton blockButton { "Block" };
    juce::TextButton cylinderButton { "Cylinder" };
    juce::TextButton plateButton { "Plate" };
    juce::TextButton objectOneButton { "Base Frame" };
    juce::TextButton objectTwoButton { "Drive Housing" };
    juce::TextButton objectThreeButton { "Top Plate" };
    juce::TextButton refreshButton { "Rebuild View" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerViewportComponent)
};
