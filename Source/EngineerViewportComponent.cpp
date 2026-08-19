#include "EngineerViewportComponent.h"

#include <array>

namespace
{
juce::Colour panelFill() noexcept { return juce::Colour(0xff101722); }
juce::Colour frameColour() noexcept { return juce::Colour(0xff2d3e55); }
juce::Colour accentColour() noexcept { return juce::Colour(0xffffa247); }
juce::Colour cyanAccent() noexcept { return juce::Colour(0xff59d0ff); }

void drawSceneGrid(juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour(juce::Colour(0x223d556f));

    const float step = 24.0f;
    for (float x = area.getX(); x <= area.getRight(); x += step)
        g.drawVerticalLine(juce::roundToInt(x), area.getY(), area.getBottom());

    for (float y = area.getY(); y <= area.getBottom(); y += step)
        g.drawHorizontalLine(juce::roundToInt(y), area.getX(), area.getRight());
}

juce::String cameraLabel(EngineerSceneModel::CameraPreset preset)
{
    switch (preset)
    {
        case EngineerSceneModel::CameraPreset::iso: return "ISO";
        case EngineerSceneModel::CameraPreset::top: return "Top";
        case EngineerSceneModel::CameraPreset::walk: return "Walk";
    }

    return "ISO";
}

juce::Rectangle<float> normalizedRectToBounds(juce::Rectangle<float> bounds,
                                              juce::Point<float> center,
                                              juce::Point<float> size)
{
    auto width = bounds.getWidth() * size.x;
    auto height = bounds.getHeight() * size.y;
    auto x = bounds.getX() + bounds.getWidth() * center.x - width * 0.5f;
    auto y = bounds.getY() + bounds.getHeight() * center.y - height * 0.5f;
    return { x, y, width, height };
}

juce::Rectangle<float> projectForAssemblyFloor(juce::Rectangle<float> rect,
                                               juce::Point<float> center)
{
    return rect.translated((center.x - 0.5f) * 30.0f,
                           (center.y - 0.5f) * 24.0f);
}

juce::Rectangle<float> mirroredRectX(juce::Rectangle<float> sceneBounds,
                                     juce::Rectangle<float> rect)
{
    const auto mirroredX = sceneBounds.getCentreX() - (rect.getCentreX() - sceneBounds.getCentreX());
    auto mirrored = rect;
    mirrored.setCentre(mirroredX, rect.getCentreY());
    return mirrored;
}

void drawDirectGeometryOverlay(juce::Graphics& g,
                               juce::Rectangle<float> rect,
                               float depth,
                               float bevelAmount,
                               bool selected)
{
    auto offset = juce::Point<float>(depth * 180.0f, -depth * 120.0f);
    auto rear = rect.translated(offset.x, offset.y);

    g.setColour(juce::Colour(0x2259d0ff));
    g.fillRoundedRectangle(rear, 10.0f);
    g.setColour(juce::Colour(0xff59d0ff).withAlpha(selected ? 0.75f : 0.45f));
    g.drawRoundedRectangle(rear, 10.0f, 1.0f);

    g.setColour(juce::Colour(0xff59d0ff).withAlpha(selected ? 0.7f : 0.4f));
    g.drawLine(juce::Line<float>(rect.getTopLeft(), rear.getTopLeft()), selected ? 1.8f : 1.0f);
    g.drawLine(juce::Line<float>(rect.getTopRight(), rear.getTopRight()), selected ? 1.8f : 1.0f);
    g.drawLine(juce::Line<float>(rect.getBottomLeft(), rear.getBottomLeft()), selected ? 1.8f : 1.0f);
    g.drawLine(juce::Line<float>(rect.getBottomRight(), rear.getBottomRight()), selected ? 1.8f : 1.0f);

    if (bevelAmount > 0.0f)
    {
        auto inset = rect.reduced(bevelAmount * rect.getWidth(), bevelAmount * rect.getHeight());
        g.setColour(juce::Colour(0xffffd27a).withAlpha(selected ? 0.85f : 0.50f));
        g.drawRoundedRectangle(inset, 8.0f, selected ? 1.8f : 1.0f);
    }
}

std::array<juce::Point<float>, 4> rectangleVertices(const juce::Rectangle<float>& rect)
{
    return {
        rect.getTopLeft(),
        rect.getTopRight(),
        rect.getBottomRight(),
        rect.getBottomLeft()
    };
}

std::array<juce::Point<float>, 4> rectangleEdgeMidpoints(const juce::Rectangle<float>& rect)
{
    return {
        juce::Point<float>(rect.getCentreX(), rect.getY()),
        juce::Point<float>(rect.getRight(), rect.getCentreY()),
        juce::Point<float>(rect.getCentreX(), rect.getBottom()),
        juce::Point<float>(rect.getX(), rect.getCentreY())
    };
}

void drawGeometryElementSelection(juce::Graphics& g,
                                  juce::Rectangle<float> rect,
                                  EngineerSceneModel::GeometryElementKind kind,
                                  int index)
{
    g.setColour(juce::Colour(0xffffd27a));

    switch (kind)
    {
        case EngineerSceneModel::GeometryElementKind::vertex:
        {
            const auto vertices = rectangleVertices(rect);
            const auto point = vertices[static_cast<size_t>(juce::jlimit(0, 3, index))];
            g.fillEllipse(point.x - 5.0f, point.y - 5.0f, 10.0f, 10.0f);
            break;
        }

        case EngineerSceneModel::GeometryElementKind::edge:
        {
            const auto midpoints = rectangleEdgeMidpoints(rect);
            const auto point = midpoints[static_cast<size_t>(juce::jlimit(0, 3, index))];
            juce::Rectangle<float> handle(point.x - 8.0f, point.y - 4.0f, 16.0f, 8.0f);
            g.fillRoundedRectangle(handle, 3.0f);
            break;
        }

        case EngineerSceneModel::GeometryElementKind::face:
        {
            g.drawRoundedRectangle(rect.reduced(8.0f), 8.0f, 2.0f);
            break;
        }
    }
}
}

EngineerViewportComponent::EngineerViewportComponent(EngineerSceneModel& model)
    : sceneModel(model)
{
    sceneModel.addListener(this);
    configureButton(designViewButton);
    configureButton(assemblyFloorButton);
    configureButton(isoCameraButton);
    configureButton(topCameraButton);
    configureButton(walkCameraButton);
    configureButton(blockButton);
    configureButton(cylinderButton);
    configureButton(plateButton);
    configureButton(objectOneButton);
    configureButton(objectTwoButton);
    configureButton(objectThreeButton);
    configureButton(refreshButton);

    designViewButton.onClick = [this]
    {
        viewMode = ViewMode::design;
        updateModeButtons();
        repaint();
    };

    assemblyFloorButton.onClick = [this]
    {
        viewMode = ViewMode::assemblyFloor;
        updateModeButtons();
        repaint();
    };

    blockButton.onClick = [this]
    {
        selectedPrimitive = "Block";
        sceneModel.setSelectedObjectPrimitiveType(selectedPrimitive);
        updatePrimitiveButtons();
        repaint();
    };

    cylinderButton.onClick = [this]
    {
        selectedPrimitive = "Cylinder";
        sceneModel.setSelectedObjectPrimitiveType(selectedPrimitive);
        updatePrimitiveButtons();
        repaint();
    };

    plateButton.onClick = [this]
    {
        selectedPrimitive = "Plate";
        sceneModel.setSelectedObjectPrimitiveType(selectedPrimitive);
        updatePrimitiveButtons();
        repaint();
    };

    isoCameraButton.onClick = [this]
    {
        sceneModel.setCameraPreset(EngineerSceneModel::CameraPreset::iso);
        updateCameraButtons();
        repaint();
    };

    topCameraButton.onClick = [this]
    {
        sceneModel.setCameraPreset(EngineerSceneModel::CameraPreset::top);
        updateCameraButtons();
        repaint();
    };

    walkCameraButton.onClick = [this]
    {
        sceneModel.setCameraPreset(EngineerSceneModel::CameraPreset::walk);
        updateCameraButtons();
        repaint();
    };

    objectOneButton.onClick = [this]
    {
        sceneModel.selectObject(0);
        updateObjectButtons();
        repaint();
    };

    objectTwoButton.onClick = [this]
    {
        sceneModel.selectObject(1);
        updateObjectButtons();
        repaint();
    };

    objectThreeButton.onClick = [this]
    {
        sceneModel.selectObject(2);
        updateObjectButtons();
        repaint();
    };

    refreshButton.onClick = [this] { repaint(); };

    addAndMakeVisible(designViewButton);
    addAndMakeVisible(assemblyFloorButton);
    addAndMakeVisible(isoCameraButton);
    addAndMakeVisible(topCameraButton);
    addAndMakeVisible(walkCameraButton);
    addAndMakeVisible(blockButton);
    addAndMakeVisible(cylinderButton);
    addAndMakeVisible(plateButton);
    addAndMakeVisible(objectOneButton);
    addAndMakeVisible(objectTwoButton);
    addAndMakeVisible(objectThreeButton);
    addAndMakeVisible(refreshButton);

    updateModeButtons();
    updatePrimitiveButtons();
    updateCameraButtons();
    updateObjectButtons();
}

EngineerViewportComponent::~EngineerViewportComponent()
{
    sceneModel.removeListener(this);
}

void EngineerViewportComponent::engineerSceneModelChanged()
{
    selectedPrimitive = sceneModel.getSelectedObject().primitiveType;
    updateCameraButtons();
    updatePrimitiveButtons();
    updateObjectButtons();
    repaint();
}

void EngineerViewportComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(panelFill());
    g.fillRoundedRectangle(bounds, 18.0f);
    g.setColour(frameColour());
    g.drawRoundedRectangle(bounds, 18.0f, 1.0f);

    auto toolbar = bounds.removeFromTop(72.0f);
    auto viewport = getViewportBounds();

    g.setColour(juce::Colour(0xff0c131d));
    g.fillRoundedRectangle(viewport, 18.0f);
    g.setColour(frameColour().brighter(0.2f));
    g.drawRoundedRectangle(viewport, 18.0f, 1.0f);

    drawSceneGrid(g, viewport.reduced(18.0f));

    auto headerArea = viewport.reduced(24.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(22.0f).boldened());
    g.drawText(viewMode == ViewMode::design ? "Technical Design View" : "Assembly Floor View",
               headerArea.removeFromTop(28.0f).toNearestInt(),
               juce::Justification::centredLeft,
               true);

    g.setColour(juce::Colour(0xffb9c7d9));
    g.setFont(juce::Font(14.0f));
    g.drawText("Primitive seed: " + selectedPrimitive + "   |   Grid: 10 mm   |   Camera: " + cameraLabel(sceneModel.getCameraPreset()),
               headerArea.removeFromTop(22.0f).toNearestInt(),
               juce::Justification::centredLeft,
               true);

    const auto& selectedObject = sceneModel.getSelectedObject();
    g.drawText("Selected object: " + selectedObject.name + "   |   Source: " + selectedObject.primitiveType,
               headerArea.removeFromTop(22.0f).toNearestInt(),
               juce::Justification::centredLeft,
               true);

    g.drawText("Workflow: " + EngineerSceneModel::toDisplayString(selectedObject.authoringState)
                + "   |   Mirror X: " + juce::String(selectedObject.mirrorXEnabled ? "On" : "Off"),
               headerArea.removeFromTop(22.0f).toNearestInt(),
               juce::Justification::centredLeft,
               true);

    if (selectedObject.authoringState == EngineerSceneModel::AuthoringState::directGeometry)
    {
        g.drawText("Direct geometry tool: " + EngineerSceneModel::toDisplayString(sceneModel.getSelectedGeometryTool())
                    + "   |   Element: " + EngineerSceneModel::toDisplayString(sceneModel.getSelectedGeometryElementKind())
                    + " " + juce::String(sceneModel.getSelectedGeometryElementIndex() + 1)
                    + "   |   Depth: " + juce::String(selectedObject.geometryDepth * 100.0f, 1)
                    + " mm   |   Bevel: " + juce::String(selectedObject.bevelAmount * 100.0f, 1) + " mm",
                   headerArea.removeFromTop(22.0f).toNearestInt(),
                   juce::Justification::centredLeft,
                   true);
    }

    auto sceneArea = getSceneBounds();

    if (viewMode == ViewMode::design)
    {
        const auto selectedIndex = sceneModel.getSelectedObjectIndex();
        const auto& objects = sceneModel.getObjects();
        for (size_t i = 0; i < objects.size(); ++i)
        {
            const auto& object = objects[i];
            auto rect = getObjectBounds(object);
            const bool selected = static_cast<int>(i) == selectedIndex;

            if (object.authoringState == EngineerSceneModel::AuthoringState::directGeometry)
            {
                drawDirectGeometryOverlay(g, rect, object.geometryDepth, object.bevelAmount, selected);
                if (selected)
                    drawGeometryElementSelection(g, rect, sceneModel.getSelectedGeometryElementKind(), sceneModel.getSelectedGeometryElementIndex());
            }

            g.setColour((selected ? cyanAccent() : juce::Colour(0xff6ca1bf)).withAlpha(selected ? 0.95f : 0.65f));
            g.drawRoundedRectangle(rect, 10.0f, selected ? 2.5f : 1.4f);

            auto inset = rect.reduced(rect.getWidth() * 0.18f, rect.getHeight() * 0.18f);
            g.setColour((selected ? accentColour() : juce::Colour(0xff8f7f56)).withAlpha(selected ? 0.85f : 0.60f));
            g.drawRect(inset, selected ? 2.0f : 1.0f);

            if (object.mirrorXEnabled && object.authoringState != EngineerSceneModel::AuthoringState::directGeometry)
            {
                auto mirrored = mirroredRectX(sceneArea, rect);
                g.setColour(cyanAccent().withAlpha(selected ? 0.38f : 0.24f));
                g.drawRoundedRectangle(mirrored, 10.0f, 1.2f);
                g.setColour(accentColour().withAlpha(selected ? 0.28f : 0.18f));
                g.fillRoundedRectangle(mirrored.reduced(6.0f), 8.0f);
            }
        }

        g.setColour(juce::Colour(0xffcfe8ff));
        g.setFont(juce::Font(13.0f));
        g.drawText("Dimension-ready design surface with shared object selection and primitive-to-geometry workflow", sceneArea.removeFromBottom(22.0f).toNearestInt(),
                   juce::Justification::centred, true);
    }
    else
    {
        juce::Path floorPath;
        floorPath.startNewSubPath(sceneArea.getX() + 40.0f, sceneArea.getBottom() - 40.0f);
        floorPath.lineTo(sceneArea.getCentreX(), sceneArea.getY() + 40.0f);
        floorPath.lineTo(sceneArea.getRight() - 40.0f, sceneArea.getBottom() - 40.0f);
        floorPath.closeSubPath();

        g.setColour(juce::Colour(0x2840556a));
        g.fillPath(floorPath);
        g.setColour(cyanAccent().withAlpha(0.7f));
        g.strokePath(floorPath, juce::PathStrokeType(2.0f));

        const auto selectedIndex = sceneModel.getSelectedObjectIndex();
        const auto& objects = sceneModel.getObjects();
        for (size_t i = 0; i < objects.size(); ++i)
        {
            const auto& object = objects[i];
            auto projected = getObjectBounds(object);
            const bool selected = static_cast<int>(i) == selectedIndex;

            if (object.authoringState == EngineerSceneModel::AuthoringState::directGeometry)
            {
                drawDirectGeometryOverlay(g, projected, object.geometryDepth, object.bevelAmount, selected);
                if (selected)
                    drawGeometryElementSelection(g, projected, sceneModel.getSelectedGeometryElementKind(), sceneModel.getSelectedGeometryElementIndex());
            }

            g.setColour((selected ? accentColour() : juce::Colour(0xff8e7958)).withAlpha(selected ? 0.92f : 0.68f));
            g.fillRoundedRectangle(projected, 10.0f);
            g.setColour(juce::Colours::white.withAlpha(selected ? 0.58f : 0.30f));
            g.drawRoundedRectangle(projected, 10.0f, selected ? 1.8f : 1.0f);

            if (object.mirrorXEnabled && object.authoringState != EngineerSceneModel::AuthoringState::directGeometry)
            {
                auto mirrored = mirroredRectX(sceneArea, projected);
                g.setColour(accentColour().withAlpha(selected ? 0.35f : 0.22f));
                g.fillRoundedRectangle(mirrored, 10.0f);
                g.setColour(juce::Colours::white.withAlpha(0.20f));
                g.drawRoundedRectangle(mirrored, 10.0f, 1.0f);
            }
        }

        g.setColour(juce::Colour(0xffcfe8ff));
        g.setFont(juce::Font(13.0f));
        g.drawText("Rendered assembly-floor inspection using the same scene objects", sceneArea.removeFromBottom(22.0f).toNearestInt(),
                   juce::Justification::centred, true);
    }
}

void EngineerViewportComponent::mouseDown(const juce::MouseEvent& event)
{
    const auto clickedIndex = hitTestObject(event.position);
    if (clickedIndex < 0)
        return;

    sceneModel.selectObject(clickedIndex);
    dragAnchor = event.position;
    dragStartPosition = sceneModel.getSelectedObject().normalizedPosition;
    isDraggingObject = true;
}

void EngineerViewportComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDraggingObject)
        return;

    const auto sceneBounds = getSceneBounds();
    if (sceneBounds.isEmpty())
        return;

    auto deltaPixels = event.position - dragAnchor;
    juce::Point<float> deltaNormalized(deltaPixels.x / sceneBounds.getWidth(),
                                       deltaPixels.y / sceneBounds.getHeight());
    auto next = dragStartPosition + deltaNormalized;
    next.x = juce::jlimit(0.10f, 0.90f, next.x);
    next.y = juce::jlimit(0.10f, 0.90f, next.y);
    sceneModel.setSelectedObjectPosition(next);
}

void EngineerViewportComponent::mouseUp(const juce::MouseEvent&)
{
    isDraggingObject = false;
}

void EngineerViewportComponent::resized()
{
    auto area = getLocalBounds().reduced(14);
    auto top = area.removeFromTop(36);
    auto secondRow = area.removeFromTop(30);
    auto thirdRow = area.removeFromTop(30);

    designViewButton.setBounds(top.removeFromLeft(120));
    top.removeFromLeft(8);
    assemblyFloorButton.setBounds(top.removeFromLeft(132));
    top.removeFromLeft(16);
    isoCameraButton.setBounds(top.removeFromLeft(64));
    top.removeFromLeft(6);
    topCameraButton.setBounds(top.removeFromLeft(64));
    top.removeFromLeft(6);
    walkCameraButton.setBounds(top.removeFromLeft(64));
    refreshButton.setBounds(top.removeFromRight(124));

    blockButton.setBounds(secondRow.removeFromLeft(88));
    secondRow.removeFromLeft(8);
    cylinderButton.setBounds(secondRow.removeFromLeft(88));
    secondRow.removeFromLeft(8);
    plateButton.setBounds(secondRow.removeFromLeft(88));

    objectOneButton.setBounds(thirdRow.removeFromLeft(120));
    thirdRow.removeFromLeft(8);
    objectTwoButton.setBounds(thirdRow.removeFromLeft(132));
    thirdRow.removeFromLeft(8);
    objectThreeButton.setBounds(thirdRow.removeFromLeft(108));
}

EngineerViewportComponent::ViewMode EngineerViewportComponent::getViewMode() const noexcept
{
    return viewMode;
}

juce::Rectangle<float> EngineerViewportComponent::getViewportBounds() const
{
    auto bounds = getLocalBounds().toFloat();
    auto viewport = bounds.reduced(14.0f, 0.0f);
    viewport.removeFromTop(10.0f);
    viewport.removeFromBottom(12.0f);
    return viewport;
}

juce::Rectangle<float> EngineerViewportComponent::getSceneBounds() const
{
    auto sceneArea = getViewportBounds().reduced(28.0f);
    sceneArea.removeFromTop(94.0f);
    return sceneArea;
}

juce::Rectangle<float> EngineerViewportComponent::getObjectBounds(const EngineerSceneModel::SceneObject& object) const
{
    auto bounds = normalizedRectToBounds(getSceneBounds(), object.normalizedPosition, object.normalizedSize);
    if (viewMode == ViewMode::assemblyFloor)
        return projectForAssemblyFloor(bounds, object.normalizedPosition);

    return bounds;
}

int EngineerViewportComponent::hitTestObject(juce::Point<float> point) const
{
    const auto& objects = sceneModel.getObjects();
    for (int i = static_cast<int>(objects.size()) - 1; i >= 0; --i)
    {
        if (getObjectBounds(objects[static_cast<size_t>(i)]).contains(point))
            return i;
    }

    return -1;
}

void EngineerViewportComponent::configureButton(juce::TextButton& button)
{
    button.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1a2533));
    button.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff25354a));
    button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    button.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
}

void EngineerViewportComponent::updateModeButtons()
{
    designViewButton.setToggleState(viewMode == ViewMode::design, juce::dontSendNotification);
    assemblyFloorButton.setToggleState(viewMode == ViewMode::assemblyFloor, juce::dontSendNotification);
}

void EngineerViewportComponent::updatePrimitiveButtons()
{
    blockButton.setToggleState(selectedPrimitive == "Block", juce::dontSendNotification);
    cylinderButton.setToggleState(selectedPrimitive == "Cylinder", juce::dontSendNotification);
    plateButton.setToggleState(selectedPrimitive == "Plate", juce::dontSendNotification);
}

void EngineerViewportComponent::updateCameraButtons()
{
    isoCameraButton.setToggleState(sceneModel.getCameraPreset() == EngineerSceneModel::CameraPreset::iso, juce::dontSendNotification);
    topCameraButton.setToggleState(sceneModel.getCameraPreset() == EngineerSceneModel::CameraPreset::top, juce::dontSendNotification);
    walkCameraButton.setToggleState(sceneModel.getCameraPreset() == EngineerSceneModel::CameraPreset::walk, juce::dontSendNotification);
}

void EngineerViewportComponent::updateObjectButtons()
{
    const auto& objects = sceneModel.getObjects();
    auto configureObjectButton = [&](juce::TextButton& button, int index)
    {
        if (index < static_cast<int>(objects.size()))
        {
            button.setButtonText(objects[static_cast<size_t>(index)].name);
            button.setEnabled(true);
            button.setToggleState(sceneModel.getSelectedObjectIndex() == index, juce::dontSendNotification);
        }
        else
        {
            button.setButtonText("Unused");
            button.setEnabled(false);
            button.setToggleState(false, juce::dontSendNotification);
        }
    };

    configureObjectButton(objectOneButton, 0);
    configureObjectButton(objectTwoButton, 1);
    configureObjectButton(objectThreeButton, 2);
}
