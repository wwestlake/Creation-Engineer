#pragma once

#include <JuceHeader.h>
#include <creation/ui/CreationSuiteHeaderBar.h>
#include <creation/ui/SuiteShellController.h>
#include <CreationDock/DockManager.h>

#include "EngineerSceneModel.h"
#include "EngineerViewportComponent.h"
#include "EngineerNavigatorComponent.h"
#include "EngineerPropertiesComponent.h"
#include "EngineerModifiersComponent.h"
#include "EngineerGeometryElementsComponent.h"
#include "EngineerGeometryToolsComponent.h"

#include <creation/assets/ProjectSession.h>
#include <creation/assets/ProjectWorkspaceService.h>
#include <creation/suite/SuiteSettings.h>

class MainComponent final : public juce::Component,
                            private juce::MenuBarModel
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void createNewProject();
    void openProject(const juce::String& projectId);
    void saveSessionToDisk(bool userInitiated = false);
    void loadSessionFromDisk();
    bool ensureProjectSessionActive(juce::String& errorMessage);
    void saveAppSettings();
    void loadAppSettings();

    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String&) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;
    void initialiseDockingWorkspace();
    void toggleDockPanel(const juce::String& panelId, CreationDock::DockTargetZone fallbackZone);

    std::unique_ptr<juce::MenuBarComponent> menuBar_;
    std::unique_ptr<CreationDock::DockManager> dockManager_;

    creation::suite::SuiteSettings suiteSettings_;
    creation::suite::SuiteSettingsStore suiteSettingsStore_;
    creation::assets::ProjectSession projectSession_;
    bool projectDirty_ = false;

    CreationSuiteHeaderBar headerBar_;
    creation::ui::SuiteShellController suiteShellController_;

    // Real Creation Engineer workspace (geometry model + the six panels that
    // observe it directly via EngineerSceneModel::Listener) -- replaces the
    // borrowed CreationEngine shell (ce::World/ViewportComponent/HierarchyPanel/
    // etc.) this app used to run as a placeholder. See docking rollout plan.
    EngineerSceneModel sceneModel_;
    EngineerNavigatorComponent navigatorPanel_;
    EngineerPropertiesComponent propertiesPanel_;
    EngineerModifiersComponent modifiersPanel_;
    EngineerGeometryElementsComponent geometryElementsPanel_;
    EngineerGeometryToolsComponent geometryToolsPanel_;
    EngineerViewportComponent viewport_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
