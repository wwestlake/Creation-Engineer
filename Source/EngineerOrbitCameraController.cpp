#include "EngineerOrbitCameraController.h"

#include <cmath>

EngineerOrbitCameraController::EngineerOrbitCameraController(juce::Component& viewport,
                                                              juce::Vector3D<float> initialTarget,
                                                              float initialDistance)
    : viewport_(viewport), target_(initialTarget), yaw_(0.6f), pitch_(-0.4f), distance_(initialDistance)
{
    viewport_.addMouseListener(this, false);
}

EngineerOrbitCameraController::~EngineerOrbitCameraController()
{
    viewport_.removeMouseListener(this);
}

juce::Vector3D<float> EngineerOrbitCameraController::Forward() const
{
    const auto yaw = yaw_.load(std::memory_order_relaxed);
    const auto pitch = pitch_.load(std::memory_order_relaxed);
    return { std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch) };
}

juce::Vector3D<float> EngineerOrbitCameraController::Position() const
{
    juce::Vector3D<float> target;
    {
        const juce::ScopedLock lock(targetLock_);
        target = target_;
    }
    return target - Forward() * distance_.load(std::memory_order_relaxed);
}

juce::Vector3D<float> EngineerOrbitCameraController::Target() const
{
    const juce::ScopedLock lock(targetLock_);
    return target_;
}

void EngineerOrbitCameraController::SetTarget(juce::Vector3D<float> target) noexcept
{
    const juce::ScopedLock lock(targetLock_);
    target_ = target;
}

void EngineerOrbitCameraController::SnapToForward(juce::Vector3D<float> forward) noexcept
{
    const auto pitch = std::asin(juce::jlimit(-1.0f, 1.0f, forward.y));
    const auto cosPitch = std::cos(pitch);
    const auto yaw = cosPitch > 0.0001f ? std::atan2(forward.x, -forward.z) : 0.0f;
    yaw_.store(yaw, std::memory_order_relaxed);
    pitch_.store(juce::jlimit(-1.5f, 1.5f, pitch), std::memory_order_relaxed);
}

void EngineerOrbitCameraController::AdjustZoom(float wheelDeltaY)
{
    const auto current = distance_.load(std::memory_order_relaxed);
    const auto updated = juce::jlimit(0.2f, 100.0f, current * (1.0f - wheelDeltaY * 0.15f));
    distance_.store(updated, std::memory_order_relaxed);
}

void EngineerOrbitCameraController::SetEnabled(bool enabled) noexcept
{
    enabled_ = enabled;
    if (!enabled_ && isOrbiting_)
    {
        isOrbiting_ = false;
        viewport_.setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void EngineerOrbitCameraController::mouseDown(const juce::MouseEvent& event)
{
    if (!enabled_ || !event.mods.isRightButtonDown())
        return;

    isOrbiting_ = true;
    lastDragScreenPos_ = event.getScreenPosition().toFloat();
    viewport_.setMouseCursor(juce::MouseCursor::NoCursor);
    // keepCursorVisibleUntilOffscreen=true -- same reasoning as FreeCamera's
    // own mouseDown (see shared/Render/Scene/FreeCamera.cpp): avoids the
    // cursor teleporting back to the drag-start point on release for an
    // ordinary orbit-drag that never reaches the screen edge.
    event.source.enableUnboundedMouseMovement(true, true);
}

void EngineerOrbitCameraController::mouseDrag(const juce::MouseEvent& event)
{
    if (!enabled_ || !isOrbiting_)
        return;

    const auto currentPos = event.getScreenPosition().toFloat();
    const auto delta = currentPos - lastDragScreenPos_;
    lastDragScreenPos_ = currentPos;

    constexpr float sensitivity = 0.006f;
    yaw_.store(yaw_.load(std::memory_order_relaxed) + delta.x * sensitivity, std::memory_order_relaxed);
    pitch_.store(juce::jlimit(-1.5f, 1.5f, pitch_.load(std::memory_order_relaxed) - delta.y * sensitivity),
                std::memory_order_relaxed);
}

void EngineerOrbitCameraController::mouseUp(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
        return;
    if (!isOrbiting_)
        return;

    isOrbiting_ = false;
    viewport_.setMouseCursor(juce::MouseCursor::NormalCursor);
    event.source.enableUnboundedMouseMovement(false);
}
