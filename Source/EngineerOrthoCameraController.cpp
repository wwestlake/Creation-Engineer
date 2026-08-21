#include "EngineerOrthoCameraController.h"

EngineerOrthoCameraController::EngineerOrthoCameraController(juce::Component& viewport)
    : viewport_(viewport)
{
    viewport_.addMouseListener(this, false);
}

EngineerOrthoCameraController::~EngineerOrthoCameraController()
{
    viewport_.removeMouseListener(this);
}

void EngineerOrthoCameraController::mouseDown(const juce::MouseEvent& event)
{
    if (!event.mods.isRightButtonDown())
        return;

    isPanning_ = true;
    lastDragScreenPos_ = event.getScreenPosition().toFloat();
    viewport_.setMouseCursor(juce::MouseCursor::DraggingHandCursor);
}

void EngineerOrthoCameraController::mouseDrag(const juce::MouseEvent& event)
{
    if (!isPanning_)
        return;

    const auto currentPos = event.getScreenPosition().toFloat();
    const auto screenDelta = currentPos - lastDragScreenPos_;
    lastDragScreenPos_ = currentPos;

    // World units per screen pixel at the current zoom level, so panning
    // feels the same regardless of how far zoomed in/out the view is.
    const auto viewportHeight = juce::jmax(1, viewport_.getHeight());
    const auto worldPerPixel = (zoomHalfHeight_ * 2.0f) / static_cast<float>(viewportHeight);

    // Drag right -> content should track the cursor, i.e. the view centre
    // moves left (negative X); drag down -> view centre moves toward -Z
    // (view looks straight down, so screen-down is world -Z here).
    panOffset_.x -= screenDelta.x * worldPerPixel;
    panOffset_.y -= screenDelta.y * worldPerPixel;
}

void EngineerOrthoCameraController::mouseUp(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
        return;

    if (!isPanning_)
        return;

    isPanning_ = false;
    viewport_.setMouseCursor(juce::MouseCursor::NormalCursor);
}

void EngineerOrthoCameraController::AdjustZoom(float wheelDeltaY)
{
    zoomHalfHeight_ = juce::jlimit(0.5f, 60.0f, zoomHalfHeight_ * (1.0f - wheelDeltaY * 0.15f));
}
