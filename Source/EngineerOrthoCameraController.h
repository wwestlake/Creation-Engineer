#pragma once

#include <atomic>

#include <JuceHeader.h>

// Pan/zoom-only navigation for EngineerViewportComponent's planarElectronics
// instance — a top-down orthographic view with no fly-camera concept, so it
// gets its own small controller rather than reusing ce::FreeCamera. Lives in
// CreationEngineer, not shared/Render: this interaction model (right-drag to
// pan, wheel to zoom, no look/rotate at all) is specific to Engineer's
// planar/schematic authoring mode, not a general-purpose camera.
//
// Self-attaches as a juce::MouseListener on the owning viewport component,
// the same pattern ce::FreeCamera uses — the viewport doesn't need to route
// mouse events to it manually.
class EngineerOrthoCameraController final : private juce::MouseListener
{
public:
    explicit EngineerOrthoCameraController(juce::Component& viewport);
    ~EngineerOrthoCameraController() override;

    void AdjustZoom(float wheelDeltaY);

    // World-space X/Z the view is centred on (used as juce::Point<float>
    // since this mode never touches Y/height).
    juce::Point<float> PanOffset() const { return panOffset_; }

    // Half the visible world-space height at the current zoom level; the
    // viewport multiplies this by aspect ratio for the ortho half-width.
    float ZoomHalfHeight() const { return zoomHalfHeight_; }

private:
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    juce::Component& viewport_;

    juce::Point<float> panOffset_{ 0.0f, 0.0f };
    float zoomHalfHeight_ = 4.0f;

    bool isPanning_ = false;
    juce::Point<float> lastDragScreenPos_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerOrthoCameraController)
};
