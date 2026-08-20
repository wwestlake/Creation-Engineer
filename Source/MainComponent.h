#pragma once

#include <JuceHeader.h>
#include <creation/ui/CreationSuiteHeaderBar.h>
#include <creation/ui/SuiteShellController.h>

#include "engine/simulation.h"
#include "engine/world.h"
#include "Render/ViewportComponent.h"
#include "Views/HierarchyPanel.h"
#include "Views/ImportPanel.h"
#include "Views/LightPanel.h"
#include "Views/MaterialsPanel.h"
#include "Views/NodeEditor/LogicPanel.h"
#include "Views/PlaceholderPanel.h"
#include "Views/ScriptPanel.h"
#include "Views/TransformPanel.h"
#include "Views/ViewModeBar.h"

#include <creation/assets/ProjectSession.h>
#include <creation/assets/ProjectWorkspaceService.h>
#include <creation/suite/SuiteSettings.h>

class MainComponent final : public juce::Component,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void SetActiveMode(ce::WorkspaceMode mode);
    void SetPlaying(bool playing);

    void createNewProject();
    void openProject(const juce::String& projectId);
    void saveSessionToDisk(bool userInitiated = false);
    void loadSessionFromDisk();
    bool ensureProjectSessionActive(juce::String& errorMessage);
    void saveAppSettings();
    void loadAppSettings();

    creation::suite::SuiteSettings suiteSettings_;
    creation::suite::SuiteSettingsStore suiteSettingsStore_;
    creation::assets::ProjectSession projectSession_;
    bool projectDirty_ = false;

    ce::engine::World world_;
    bool isPlaying_ = false;

    CreationSuiteHeaderBar headerBar_;
    creation::ui::SuiteShellController suiteShellController_;
    ce::ViewModeBar viewModeBar_;
    ce::WorkspaceMode activeMode_ = ce::WorkspaceMode::Scene;

    ce::ViewportComponent viewport_;
    ce::HierarchyPanel hierarchyPanel_;
    juce::Label inspectorTitle_ { {}, "Inspector" };
    juce::Label tickLabel_;
    ce::TransformPanel transformPanel_;
    ce::ScriptPanel scriptPanel_;
    ce::MaterialsPanel pbrMaterialPanel_;
    ce::LightPanel lightPanel_;
    ce::ImportPanel importPanel_;
    ce::LogicPanel logicPanel_;
    ce::PlaceholderPanel materialsPanel_ { "Materials", "Node-based material editor - coming soon" };
    ce::PlaceholderPanel serverPanel_ { "Server", "Dedicated server operational view - coming soon" };
    ce::PlaceholderPanel settingsPanel_ { "Settings", "Application settings - coming soon" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
