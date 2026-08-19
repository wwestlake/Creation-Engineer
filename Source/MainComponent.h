#pragma once

#include <JuceHeader.h>
#include "EngineerGeometryElementsComponent.h"
#include "EngineerGeometryToolsComponent.h"
#include "EngineerModifiersComponent.h"
#include "EngineerNavigatorComponent.h"
#include "EngineerPropertiesComponent.h"
#include "EngineerSceneModel.h"
#include "EngineerViewportComponent.h"
#include <creation/assets/ProjectManifest.h>
#include <creation/interop/ProjectRegistry.h>
#include <creation/services/SuiteAiSettings.h>
#include <creation/suite/SuiteSettings.h>
#include <creation/suite/SuiteStoragePaths.h>
#include <creation/ui/CreationSuiteHeaderBar.h>
#include <creation/ui/SuiteShellController.h>
#include <juce_docking/DockManager.h>

class MainComponent final : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void configureHeader();
    void configureWorkbench();
    void loadSuiteState();
    void refreshShellSummary();
    creation::assets::SuiteAppDomain currentDomain() const noexcept;
    juce::String domainDisplayName() const;
    juce::String resultsSummaryText() const;
    juce::String platformSummaryText() const;

    CreationSuiteHeaderBar headerBar;
    creation::ui::SuiteShellController suiteShellController;
    EngineerSceneModel sceneModel;
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label runtimeLabel;
    std::unique_ptr<juce_docking::DockManager> dockManager;

    EngineerViewportComponent* viewportComponent = nullptr;
    EngineerNavigatorComponent* navigatorComponent = nullptr;
    EngineerModifiersComponent* modifiersComponent = nullptr;
    EngineerGeometryElementsComponent* geometryElementsComponent = nullptr;
    EngineerGeometryToolsComponent* geometryToolsComponent = nullptr;
    EngineerPropertiesComponent* propertiesComponent = nullptr;
    juce::TextEditor* resultsSummary = nullptr;
    juce::TextEditor* platformSummary = nullptr;

    creation::suite::SuiteSettingsStore suiteSettingsStore;
    creation::services::SuiteAiSettingsStore suiteAiSettingsStore;
    creation::suite::SuiteSettings suiteSettings;
    creation::services::SuiteAiSettings suiteAiSettings;

    juce::String lastRegistryError;
    int domainProjectCount = 0;
    int totalProjectCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
