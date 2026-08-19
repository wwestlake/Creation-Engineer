#include "EngineerViewportComponent.h"

#include <array>

namespace
{
juce::Colour panelFill() noexcept { return juce::Colour(0xff101722); }
juce::Colour frameColour() noexcept { return juce::Colour(0xff2d3e55); }
juce::Colour accentColour() noexcept { return juce::Colour(0xffffa247); }
juce::Colour cyanAccent() noexcept { return juce::Colour(0xff59d0ff); }
juce::Colour xAxisColour() noexcept { return juce::Colour(0xffff6b6b); }
juce::Colour yAxisColour() noexcept { return juce::Colour(0xff5ae08a); }

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

void drawGeometryElementProxies(juce::Graphics& g,
                                juce::Rectangle<float> rect,
                                EngineerSceneModel::GeometryElementKind selectedKind,
                                int selectedIndex)
{
    const auto vertices = rectangleVertices(rect);
    const auto midpoints = rectangleEdgeMidpoints(rect);
    const auto faceRect = rect.reduced(8.0f);

    for (int i = 0; i < 4; ++i)
    {
        const bool selected = selectedKind == EngineerSceneModel::GeometryElementKind::vertex && selectedIndex == i;
        g.setColour(juce::Colour(0xffffd27a).withAlpha(selected ? 1.0f : 0.55f));
        const auto point = vertices[static_cast<size_t>(i)];
        g.fillEllipse(point.x - (selected ? 6.0f : 4.0f),
                      point.y - (selected ? 6.0f : 4.0f),
                      selected ? 12.0f : 8.0f,
                      selected ? 12.0f : 8.0f);
    }

    for (int i = 0; i < 4; ++i)
    {
        const bool selected = selectedKind == EngineerSceneModel::GeometryElementKind::edge && selectedIndex == i;
        g.setColour(juce::Colour(0xff59d0ff).withAlpha(selected ? 1.0f : 0.50f));
        const auto point = midpoints[static_cast<size_t>(i)];
        juce::Rectangle<float> handle(point.x - (selected ? 10.0f : 8.0f),
                                      point.y - (selected ? 5.0f : 4.0f),
                                      selected ? 20.0f : 16.0f,
                                      selected ? 10.0f : 8.0f);
        g.fillRoundedRectangle(handle, 3.0f);
    }

    g.setColour(juce::Colour(0xffcfe8ff).withAlpha(selectedKind == EngineerSceneModel::GeometryElementKind::face ? 0.90f : 0.35f));
    g.drawRoundedRectangle(faceRect, 8.0f, selectedKind == EngineerSceneModel::GeometryElementKind::face ? 2.2f : 1.0f);
}

void drawViewportGizmo(juce::Graphics& g,
                       juce::Point<float> anchor,
                       EngineerSceneModel::GeometryTool tool,
                       EngineerViewportComponent::GizmoDragMode activeMode)
{
    const float axisLength = 34.0f;
    const float handleRadius = 5.0f;

    auto xEnd = juce::Point<float>(anchor.x + axisLength, anchor.y);
    auto yEnd = juce::Point<float>(anchor.x, anchor.y - axisLength);

    g.setColour(xAxisColour().withAlpha(activeMode == EngineerViewportComponent::GizmoDragMode::xAxis ? 1.0f : 0.82f));
    g.drawArrow(juce::Line<float>(anchor, xEnd), 2.6f, 10.0f, 8.0f);
    g.fillEllipse(xEnd.x - handleRadius, xEnd.y - handleRadius, handleRadius * 2.0f, handleRadius * 2.0f);

    g.setColour(yAxisColour().withAlpha(activeMode == EngineerViewportComponent::GizmoDragMode::yAxis ? 1.0f : 0.82f));
    g.drawArrow(juce::Line<float>(anchor, yEnd), 2.6f, 10.0f, 8.0f);
    g.fillEllipse(yEnd.x - handleRadius, yEnd.y - handleRadius, handleRadius * 2.0f, handleRadius * 2.0f);

    g.setColour(cyanAccent().withAlpha(activeMode == EngineerViewportComponent::GizmoDragMode::planar ? 0.95f : 0.65f));
    g.fillRoundedRectangle(juce::Rectangle<float>(anchor.x - 7.0f, anchor.y - 7.0f, 14.0f, 14.0f), 4.0f);

    if (tool == EngineerSceneModel::GeometryTool::extrude)
    {
        auto depthEnd = juce::Point<float>(anchor.x + 24.0f, anchor.y - 24.0f);
        g.setColour(accentColour().withAlpha(activeMode == EngineerViewportComponent::GizmoDragMode::depthAxis ? 1.0f : 0.82f));
        g.drawArrow(juce::Line<float>(anchor, depthEnd), 2.2f, 9.0f, 7.0f);
        g.fillEllipse(depthEnd.x - 4.0f, depthEnd.y - 4.0f, 8.0f, 8.0f);
    }

    if (tool == EngineerSceneModel::GeometryTool::bevel)
    {
        g.setColour(accentColour().withAlpha(activeMode == EngineerViewportComponent::GizmoDragMode::bevelAxis ? 1.0f : 0.82f));
        g.drawEllipse(anchor.x - 18.0f, anchor.y - 18.0f, 36.0f, 36.0f, 2.2f);
    }
}
}

EngineerViewportComponent::EngineerViewportComponent(EngineerSceneModel& model)
    : sceneModel(model)
{
    sceneModel.addListener(this);
    applyCameraPreset(sceneModel.getCameraPreset());
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
    if (!isNavigatingView)
        applyCameraPreset(sceneModel.getCameraPreset());
    updateCameraButtons();
    updatePrimitiveButtons();
    updateObjectButtons();
    repaint();
}

void EngineerViewportComponent::showContextMenu(const juce::MouseEvent& event)
{
    juce::PopupMenu menu;
    juce::PopupMenu viewMenu;
    viewMenu.addItem(101, "Technical Design View", true, viewMode == ViewMode::design);
    viewMenu.addItem(102, "Assembly Floor View", true, viewMode == ViewMode::assemblyFloor);
    menu.addSubMenu("View Mode", viewMenu);

    juce::PopupMenu cameraMenu;
    cameraMenu.addItem(111, "ISO", true, sceneModel.getCameraPreset() == EngineerSceneModel::CameraPreset::iso);
    cameraMenu.addItem(112, "Top", true, sceneModel.getCameraPreset() == EngineerSceneModel::CameraPreset::top);
    cameraMenu.addItem(113, "Walk", true, sceneModel.getCameraPreset() == EngineerSceneModel::CameraPreset::walk);
    cameraMenu.addSeparator();
    cameraMenu.addItem(114, "Reset Navigation");
    menu.addSubMenu("Camera", cameraMenu);

    const auto clickedIndex = hitTestObject(event.position);
    if (clickedIndex >= 0)
    {
        menu.addItem(120, "Select " + sceneModel.getObjects()[static_cast<size_t>(clickedIndex)].name);

        juce::PopupMenu objectMenu;
        objectMenu.addItem(121, "Block", true, sceneModel.getObjects()[static_cast<size_t>(clickedIndex)].primitiveType == "Block");
        objectMenu.addItem(122, "Cylinder", true, sceneModel.getObjects()[static_cast<size_t>(clickedIndex)].primitiveType == "Cylinder");
        objectMenu.addItem(123, "Plate", true, sceneModel.getObjects()[static_cast<size_t>(clickedIndex)].primitiveType == "Plate");
        menu.addSubMenu("Primitive", objectMenu);
    }

    if (sceneModel.isSelectedObjectDirectGeometry())
    {
        juce::PopupMenu geometryMenu;
        geometryMenu.addItem(131, "Translate", true, sceneModel.getSelectedGeometryTool() == EngineerSceneModel::GeometryTool::translate);
        geometryMenu.addItem(132, "Scale", true, sceneModel.getSelectedGeometryTool() == EngineerSceneModel::GeometryTool::scale);
        geometryMenu.addItem(133, "Extrude", true, sceneModel.getSelectedGeometryTool() == EngineerSceneModel::GeometryTool::extrude);
        geometryMenu.addItem(134, "Bevel", true, sceneModel.getSelectedGeometryTool() == EngineerSceneModel::GeometryTool::bevel);
        geometryMenu.addSeparator();
        geometryMenu.addItem(135, sceneModel.isGeometrySnappingEnabled() ? "Disable Snapping" : "Enable Snapping");
        menu.addSubMenu("Direct Geometry", geometryMenu);
    }

    juce::Component::SafePointer<EngineerViewportComponent> safeThis(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea({ event.getScreenPosition(), { 1, 1 } }),
                       [safeThis, clickedIndex](int result)
                       {
                           if (safeThis != nullptr)
                               safeThis->handleContextMenuResult(result, clickedIndex);
                       });
}

void EngineerViewportComponent::handleContextMenuResult(int result, int clickedIndex)
{
    switch (result)
    {
        case 101: viewMode = ViewMode::design; break;
        case 102: viewMode = ViewMode::assemblyFloor; break;
        case 111: sceneModel.setCameraPreset(EngineerSceneModel::CameraPreset::iso); break;
        case 112: sceneModel.setCameraPreset(EngineerSceneModel::CameraPreset::top); break;
        case 113: sceneModel.setCameraPreset(EngineerSceneModel::CameraPreset::walk); break;
        case 114:
            viewPan = {};
            viewZoom = 1.0f;
            applyCameraPreset(sceneModel.getCameraPreset());
            break;
        case 120:
            if (clickedIndex >= 0)
                sceneModel.selectObject(clickedIndex);
            break;
        case 121:
        case 122:
        case 123:
            if (clickedIndex >= 0)
            {
                sceneModel.selectObject(clickedIndex);
                sceneModel.setSelectedObjectPrimitiveType(result == 121 ? "Block" : result == 122 ? "Cylinder" : "Plate");
            }
            break;
        case 131: sceneModel.setSelectedGeometryTool(EngineerSceneModel::GeometryTool::translate); break;
        case 132: sceneModel.setSelectedGeometryTool(EngineerSceneModel::GeometryTool::scale); break;
        case 133: sceneModel.setSelectedGeometryTool(EngineerSceneModel::GeometryTool::extrude); break;
        case 134: sceneModel.setSelectedGeometryTool(EngineerSceneModel::GeometryTool::bevel); break;
        case 135: sceneModel.setGeometrySnappingEnabled(!sceneModel.isGeometrySnappingEnabled()); break;
        default: break;
    }

    repaint();
}

void EngineerViewportComponent::applyCameraPreset(EngineerSceneModel::CameraPreset preset)
{
    switch (preset)
    {
        case EngineerSceneModel::CameraPreset::iso:
            viewYaw = 0.40f;
            viewPitch = 0.30f;
            break;
        case EngineerSceneModel::CameraPreset::top:
            viewYaw = 0.0f;
            viewPitch = 0.0f;
            break;
        case EngineerSceneModel::CameraPreset::walk:
            viewYaw = 0.75f;
            viewPitch = 0.18f;
            break;
    }
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
    g.drawText("Right-click: viewport menu   |   Mouse wheel: zoom   |   Middle drag: orbit   |   Shift+middle drag: pan",
               headerArea.removeFromTop(22.0f).toNearestInt(),
               juce::Justification::centredLeft,
               true);

    const auto& selectedObject = sceneModel.getSelectedObject();
    g.drawText("Selected object: " + selectedObject.name + "   |   Source: " + selectedObject.primitiveType,
               headerArea.removeFromTop(22.0f).toNearestInt(),
               juce::Justification::centredLeft,
               true);

    g.drawText("Workflow: " + EngineerSceneModel::toDisplayString(selectedObject.authoringState)
                + "   |   Camera: " + cameraLabel(sceneModel.getCameraPreset())
                + "   |   Zoom: " + juce::String(viewZoom, 2),
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
                {
                    drawGeometryElementProxies(g, rect, sceneModel.getSelectedGeometryElementKind(), sceneModel.getSelectedGeometryElementIndex());
                    drawViewportGizmo(g,
                                      getSelectedGeometryAnchor(object, rect),
                                      sceneModel.getSelectedGeometryTool(),
                                      gizmoDragMode);
                }
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
                {
                    drawGeometryElementProxies(g, projected, sceneModel.getSelectedGeometryElementKind(), sceneModel.getSelectedGeometryElementIndex());
                    drawViewportGizmo(g,
                                      getSelectedGeometryAnchor(object, projected),
                                      sceneModel.getSelectedGeometryTool(),
                                      gizmoDragMode);
                }
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
    if (event.mods.isPopupMenu())
    {
        showContextMenu(event);
        return;
    }

    if (event.mods.isMiddleButtonDown() || (event.mods.isLeftButtonDown() && event.mods.isAltDown()))
    {
        dragAnchor = event.position;
        isNavigatingView = true;
        isDraggingObject = false;
        isDraggingGeometryElement = false;
        gizmoDragMode = GizmoDragMode::none;
        return;
    }

    const auto clickedIndex = hitTestObject(event.position);
    if (clickedIndex < 0)
        return;

    sceneModel.selectObject(clickedIndex);

    const auto& selectedObject = sceneModel.getSelectedObject();
    if (selectedObject.authoringState == EngineerSceneModel::AuthoringState::directGeometry)
    {
        const auto objectBounds = getObjectBounds(selectedObject);
        const auto gizmoHit = hitTestGizmo(event.position, selectedObject, objectBounds);
        if (gizmoHit.valid)
        {
            dragAnchor = event.position;
            gizmoDragMode = gizmoHit.mode;
            isDraggingGeometryElement = true;
            isDraggingObject = false;
            return;
        }

        const auto geometryHit = hitTestGeometryElement(event.position, selectedObject, objectBounds);
        if (geometryHit.valid)
        {
            sceneModel.setSelectedGeometryElementKind(geometryHit.kind);
            sceneModel.setSelectedGeometryElementIndex(geometryHit.index);
            dragAnchor = event.position;
            isDraggingGeometryElement = true;
            gizmoDragMode = GizmoDragMode::elementProxy;
            isDraggingObject = false;
            return;
        }
    }

    dragAnchor = event.position;
    dragStartPosition = sceneModel.getSelectedObject().normalizedPosition;
    isDraggingObject = true;
}

void EngineerViewportComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isNavigatingView)
    {
        const auto deltaPixels = event.position - dragAnchor;
        dragAnchor = event.position;

        if (event.mods.isShiftDown())
        {
            viewPan += deltaPixels;
        }
        else
        {
            viewYaw += deltaPixels.x * 0.01f;
            viewPitch = juce::jlimit(-0.85f, 0.85f, viewPitch - deltaPixels.y * 0.008f);
        }

        repaint();
        return;
    }

    if (isDraggingGeometryElement)
    {
        const auto sceneBounds = getSceneBounds();
        if (sceneBounds.isEmpty())
            return;

        auto deltaPixels = event.position - dragAnchor;
        juce::Point<float> deltaNormalized(deltaPixels.x / sceneBounds.getWidth(),
                                           deltaPixels.y / sceneBounds.getHeight());

        switch (gizmoDragMode)
        {
            case GizmoDragMode::elementProxy:
                sceneModel.nudgeSelectedGeometryElement(deltaNormalized);
                dragAnchor = event.position;
                return;
            case GizmoDragMode::xAxis:
                deltaNormalized.y = 0.0f;
                break;
            case GizmoDragMode::yAxis:
                deltaNormalized.x = 0.0f;
                break;
            case GizmoDragMode::planar:
                break;
            case GizmoDragMode::depthAxis:
            {
                sceneModel.extrudeSelectedDirectGeometry((deltaNormalized.x - deltaNormalized.y) * 0.14f);
                dragAnchor = event.position;
                return;
            }
            case GizmoDragMode::bevelAxis:
            {
                sceneModel.bevelSelectedDirectGeometry((deltaNormalized.x - deltaNormalized.y) * 0.07f);
                dragAnchor = event.position;
                return;
            }
            case GizmoDragMode::none:
                break;
        }

        switch (sceneModel.getSelectedGeometryTool())
        {
            case EngineerSceneModel::GeometryTool::translate:
                sceneModel.nudgeSelectedDirectGeometryPosition(deltaNormalized);
                break;
            case EngineerSceneModel::GeometryTool::scale:
                sceneModel.scaleSelectedDirectGeometry({ deltaNormalized.x, -deltaNormalized.y });
                break;
            case EngineerSceneModel::GeometryTool::extrude:
                sceneModel.extrudeSelectedDirectGeometry((deltaNormalized.x - deltaNormalized.y) * 0.12f);
                break;
            case EngineerSceneModel::GeometryTool::bevel:
                sceneModel.bevelSelectedDirectGeometry((deltaNormalized.x - deltaNormalized.y) * 0.06f);
                break;
        }

        dragAnchor = event.position;
        return;
    }

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
    isDraggingGeometryElement = false;
    isNavigatingView = false;
    gizmoDragMode = GizmoDragMode::none;
}

void EngineerViewportComponent::mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails& wheel)
{
    viewZoom = juce::jlimit(0.45f, 2.75f, viewZoom + wheel.deltaY * 0.22f);
    repaint();
}

void EngineerViewportComponent::resized()
{
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
    const auto sceneBounds = getSceneBounds();
    auto bounds = normalizedRectToBounds(sceneBounds, object.normalizedPosition, object.normalizedSize);
    bounds = transformRect(bounds, sceneBounds);
    if (viewMode == ViewMode::assemblyFloor)
        return projectForAssemblyFloor(bounds, transformPoint(normalizedRectToBounds(sceneBounds, object.normalizedPosition, object.normalizedSize).getCentre(), sceneBounds));

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

juce::Point<float> EngineerViewportComponent::getSelectedGeometryAnchor(const EngineerSceneModel::SceneObject&,
                                                                        juce::Rectangle<float> rect) const
{
    switch (sceneModel.getSelectedGeometryElementKind())
    {
        case EngineerSceneModel::GeometryElementKind::vertex:
            return rectangleVertices(rect)[static_cast<size_t>(juce::jlimit(0, 3, sceneModel.getSelectedGeometryElementIndex()))];
        case EngineerSceneModel::GeometryElementKind::edge:
            return rectangleEdgeMidpoints(rect)[static_cast<size_t>(juce::jlimit(0, 3, sceneModel.getSelectedGeometryElementIndex()))];
        case EngineerSceneModel::GeometryElementKind::face:
            return rect.getCentre();
    }

    return rect.getCentre();
}

EngineerViewportComponent::GeometryHit EngineerViewportComponent::hitTestGeometryElement(juce::Point<float> point,
                                                                                         const EngineerSceneModel::SceneObject& object,
                                                                                         juce::Rectangle<float> rect) const
{
    GeometryHit hit;
    if (object.authoringState != EngineerSceneModel::AuthoringState::directGeometry)
        return hit;

    const auto vertices = rectangleVertices(rect);
    for (int i = 0; i < 4; ++i)
    {
        juce::Rectangle<float> handle(vertices[static_cast<size_t>(i)].x - 8.0f,
                                      vertices[static_cast<size_t>(i)].y - 8.0f,
                                      16.0f,
                                      16.0f);
        if (handle.contains(point))
            return { true, EngineerSceneModel::GeometryElementKind::vertex, i };
    }

    const auto midpoints = rectangleEdgeMidpoints(rect);
    for (int i = 0; i < 4; ++i)
    {
        juce::Rectangle<float> handle(midpoints[static_cast<size_t>(i)].x - 10.0f,
                                      midpoints[static_cast<size_t>(i)].y - 6.0f,
                                      20.0f,
                                      12.0f);
        if (handle.contains(point))
            return { true, EngineerSceneModel::GeometryElementKind::edge, i };
    }

    if (rect.reduced(10.0f).contains(point))
        return { true, EngineerSceneModel::GeometryElementKind::face, 0 };

    return hit;
}

EngineerViewportComponent::GizmoHit EngineerViewportComponent::hitTestGizmo(juce::Point<float> point,
                                                                            const EngineerSceneModel::SceneObject& object,
                                                                            juce::Rectangle<float> rect) const
{
    GizmoHit hit;
    if (object.authoringState != EngineerSceneModel::AuthoringState::directGeometry)
        return hit;

    const auto anchor = getSelectedGeometryAnchor(object, rect);
    const auto tool = sceneModel.getSelectedGeometryTool();

    juce::Rectangle<float> planar(anchor.x - 8.0f, anchor.y - 8.0f, 16.0f, 16.0f);
    juce::Rectangle<float> xHandle(anchor.x + 24.0f, anchor.y - 8.0f, 16.0f, 16.0f);
    juce::Rectangle<float> yHandle(anchor.x - 8.0f, anchor.y - 40.0f, 16.0f, 16.0f);

    if (xHandle.contains(point))
        return { true, GizmoDragMode::xAxis };
    if (yHandle.contains(point))
        return { true, GizmoDragMode::yAxis };

    if (tool == EngineerSceneModel::GeometryTool::extrude)
    {
        juce::Rectangle<float> depthHandle(anchor.x + 16.0f, anchor.y - 32.0f, 16.0f, 16.0f);
        if (depthHandle.contains(point))
            return { true, GizmoDragMode::depthAxis };
    }

    if (tool == EngineerSceneModel::GeometryTool::bevel)
    {
        juce::Rectangle<float> bevelRing(anchor.x - 20.0f, anchor.y - 20.0f, 40.0f, 40.0f);
        if (bevelRing.contains(point) && !planar.reduced(2.0f).contains(point))
            return { true, GizmoDragMode::bevelAxis };
    }

    if (planar.contains(point))
        return { true, GizmoDragMode::planar };

    return hit;
}

juce::Point<float> EngineerViewportComponent::transformPoint(juce::Point<float> point, juce::Rectangle<float> sceneBounds) const
{
    auto centered = point - sceneBounds.getCentre();
    const auto cosYaw = std::cos(viewYaw);
    const auto sinYaw = std::sin(viewYaw);
    juce::Point<float> rotated(centered.x * cosYaw - centered.y * sinYaw,
                               centered.x * sinYaw + centered.y * cosYaw);
    rotated.x *= viewZoom;
    rotated.y *= viewZoom * (0.88f + 0.12f * std::cos(viewPitch));
    return sceneBounds.getCentre() + rotated + viewPan;
}

juce::Rectangle<float> EngineerViewportComponent::transformRect(juce::Rectangle<float> rect, juce::Rectangle<float> sceneBounds) const
{
    const auto transformedCentre = transformPoint(rect.getCentre(), sceneBounds);
    auto width = rect.getWidth() * viewZoom;
    auto height = rect.getHeight() * viewZoom * (0.88f + 0.12f * std::cos(viewPitch));
    return { transformedCentre.x - width * 0.5f,
             transformedCentre.y - height * 0.5f,
             width,
             height };
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
