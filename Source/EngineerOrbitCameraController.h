#pragma once

#include <atomic>

#include <JuceHeader.h>

// Object-centric orbit navigation for Creation Engineer's perspective
// viewports (3D Design / Assembly Floor). Deliberately NOT shared/Render's
// ce::FreeCamera: that class's FPS-style free-fly is the right model for
// CreationEngine (a game-dev tool) but the wrong one for an engineering/CAD
// tool, where the fundamental expectation is "orbit around the thing I'm
// working on, and nothing I do should lose it off-screen" -- shared/Render
// stays untouched so CreationEngine's own proven navigation is unaffected.
//
// Right-drag orbits around a fixed target point; the wheel dollies toward/
// away from that same target; double-clicking an object in the viewport
// re-targets onto it. Position() is always derived as
// target - Forward()*distance, so the target is in view by construction --
// there is no operation (including the view-cube gizmo's SnapToForward)
// that can point the camera away from it.
class EngineerOrbitCameraController final : private juce::MouseListener
{
public:
    EngineerOrbitCameraController(juce::Component& viewport, juce::Vector3D<float> initialTarget,
                                  float initialDistance);
    ~EngineerOrbitCameraController() override;

    juce::Vector3D<float> Position() const;
    juce::Vector3D<float> Target() const;

    void SetTarget(juce::Vector3D<float> target) noexcept;
    void AdjustZoom(float wheelDeltaY);

    // See ce::FreeCamera::SetEnabled's doc comment -- same rationale:
    // EngineerViewportComponent keeps both controllers alive permanently
    // and toggles which one reacts to input, instead of destroying/
    // recreating them from inside a click handler.
    void SetEnabled(bool enabled) noexcept;

    // Reframes to look along `forward` (world-space, normalized) while
    // keeping the current target and distance -- used by the view-cube
    // gizmo. Never changes what's being looked at, only the angle it's
    // viewed from.
    void SnapToForward(juce::Vector3D<float> forward) noexcept;

private:
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

    juce::Vector3D<float> Forward() const;

    juce::Component& viewport_;

    mutable juce::CriticalSection targetLock_;
    juce::Vector3D<float> target_; // guarded by targetLock_ -- written from the message thread
                                   // (SetTarget), read from the render thread (Position/Target).

    std::atomic<float> yaw_;
    std::atomic<float> pitch_;
    std::atomic<float> distance_;

    bool isOrbiting_ = false;
    bool enabled_ = true;
    juce::Point<float> lastDragScreenPos_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EngineerOrbitCameraController)
};
