#include "MainComponent.h"

#include "Branding.h"
#include "../Language/AppLanguagePolicy.h"
#include <creation/ui/CreationSuiteLogos.h>

namespace
{
void configureSummaryBox(juce::TextEditor& editor)
{
    editor.setMultiLine(true);
    editor.setReadOnly(true);
    editor.setScrollbarsShown(true);
    editor.setCaretVisible(false);
    editor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff121a24));
    editor.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff314155));
    editor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
}

juce::File getLayoutFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("LagDaemon")
        .getChildFile("CreationEngineer")
        .getChildFile("layout.json");
}
}

MainComponent::MainComponent()
{
    configureHeader();
    configureWorkbench();
    loadSuiteState();
    setSize(1500, 920);
}

MainComponent::~MainComponent()
{
    if (dockManager)
        dockManager->saveLayoutToFile(getLayoutFile());
}

void MainComponent::configureHeader()
{
    headerBar.setAppTitle("Creation Engineer");
    headerBar.setLogoImage(creation::ui::getSuiteLogoImage(creation::ui::SuiteLogoId::engineer));
    headerBar.setProjectLabel("Suite dock workbench");
    headerBar.setTransportControlsVisible(false);
    headerBar.audioButton.setButtonText("Refresh");
    headerBar.tourButton.setButtonText("EULA");
    headerBar.setStatusText("Loading Engineer workspace...");
    headerBar.onAudioRequested = [this]
    {
        loadSuiteState();
    };
    headerBar.onTourRequested = [this]
    {
        suiteShellController.showSuiteEula();
    };
    suiteShellController.attach(headerBar,
                                {
                                    "Creation Engineer",
                                    creation::assets::SuiteAppDomain::engineer,
                                    creation_engineer::branding::backgroundColour()
                                },
                                [this](const juce::String& status)
                                {
                                    headerBar.setStatusText(status);
                                    if (status.containsIgnoreCase("saved suite-wide"))
                                        loadSuiteState();
                                });

    addAndMakeVisible(headerBar);
}

void MainComponent::configureWorkbench()
{
    titleLabel.setText("Creation Engineer", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(31.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText("Engineer is now using the suite docking-workbench standard: docked viewport, navigator, modifiers, geometry elements, geometry tools, parameters, results, and platform panels.",
                          juce::dontSendNotification);
    subtitleLabel.setColour(juce::Label::textColourId, juce::Colour(0xffc9d3e3));
    addAndMakeVisible(subtitleLabel);

    runtimeLabel.setText(juce::String(creation_engineer::language::getLanguageRuntimeSummary()), juce::dontSendNotification);
    runtimeLabel.setColour(juce::Label::textColourId, creation_engineer::branding::accentColour());
    addAndMakeVisible(runtimeLabel);

    dockManager = std::make_unique<juce_docking::DockManager>(*this);
    addAndMakeVisible(*dockManager);

    auto viewport = std::make_unique<EngineerViewportComponent>(sceneModel);
    viewportComponent = viewport.get();

    auto navigator = std::make_unique<EngineerNavigatorComponent>(sceneModel);
    navigatorComponent = navigator.get();

    auto modifiers = std::make_unique<EngineerModifiersComponent>(sceneModel);
    modifiersComponent = modifiers.get();

    auto geometryElements = std::make_unique<EngineerGeometryElementsComponent>(sceneModel);
    geometryElementsComponent = geometryElements.get();

    auto geometryTools = std::make_unique<EngineerGeometryToolsComponent>(sceneModel);
    geometryToolsComponent = geometryTools.get();

    auto parameters = std::make_unique<EngineerPropertiesComponent>(sceneModel);
    propertiesComponent = parameters.get();

    auto results = std::make_unique<juce::TextEditor>();
    configureSummaryBox(*results);
    resultsSummary = results.get();

    auto platform = std::make_unique<juce::TextEditor>();
    configureSummaryBox(*platform);
    platformSummary = platform.get();

    dockManager->registerPanel("viewport",
                               "Engineering Viewport",
                               std::move(viewport),
                               juce_docking::DockTargetZone::CenterTab);
    dockManager->registerPanel("navigator",
                               "Engineering Navigator",
                               std::move(navigator),
                               juce_docking::DockTargetZone::Left);
    dockManager->registerPanel("parameters",
                               "Parameters & Properties",
                               std::move(parameters),
                               juce_docking::DockTargetZone::Right);
    dockManager->registerPanel("modifiers",
                               "Modifier Stack",
                               std::move(modifiers),
                               juce_docking::DockTargetZone::Right);
    dockManager->registerPanel("geometry-elements",
                               "Geometry Elements",
                               std::move(geometryElements),
                               juce_docking::DockTargetZone::Right);
    dockManager->registerPanel("geometry-tools",
                               "Direct Geometry Tools",
                               std::move(geometryTools),
                               juce_docking::DockTargetZone::Right);
    dockManager->registerPanel("platform",
                               "Suite Platform Context",
                               std::move(platform),
                               juce_docking::DockTargetZone::Right);
    dockManager->registerPanel("results",
                               "Results & Study Status",
                               std::move(results),
                               juce_docking::DockTargetZone::Bottom);

    dockManager->loadLayoutFromFile(getLayoutFile());
}

void MainComponent::loadSuiteState()
{
    juce::String suiteError;
    suiteSettings = suiteSettingsStore.load(suiteError);

    juce::String aiError;
    suiteAiSettings = suiteAiSettingsStore.load(aiError);

    juce::String registryError;
    const auto allProjects = creation::interop::ProjectRegistry::discoverProjects(suiteSettings, registryError);
    totalProjectCount = allProjects.size();

    creation::interop::ProjectQuery domainQuery;
    domainQuery.appDomain = currentDomain();
    const auto domainProjects = creation::interop::ProjectRegistry::queryProjects(suiteSettings, domainQuery, registryError);
    domainProjectCount = domainProjects.size();
    lastRegistryError = registryError;

    if (suiteError.isNotEmpty())
        headerBar.setStatusText("Suite settings: " + suiteError);
    else if (aiError.isNotEmpty())
        headerBar.setStatusText("AI settings: " + aiError);
    else if (registryError.isNotEmpty())
        headerBar.setStatusText("Project registry: " + registryError);
    else
        headerBar.setStatusText("Engineer dock workbench ready.");

    refreshShellSummary();
}

void MainComponent::refreshShellSummary()
{
    if (resultsSummary != nullptr)
        resultsSummary->setText(resultsSummaryText(), juce::dontSendNotification);

    if (platformSummary != nullptr)
        platformSummary->setText(platformSummaryText(), juce::dontSendNotification);
}

creation::assets::SuiteAppDomain MainComponent::currentDomain() const noexcept
{
    return creation::assets::SuiteAppDomain::engineer;
}

juce::String MainComponent::domainDisplayName() const
{
    return creation::assets::toDisplayName(currentDomain());
}

juce::String MainComponent::resultsSummaryText() const
{
    juce::String text;
    text << "App domain: " << domainDisplayName() << "\n";
    text << "Projects in this domain: " << domainProjectCount << "\n";
    text << "Projects across all known suite domains: " << totalProjectCount << "\n\n";
    text << "Reserved results surface:\n";
    text << "- study readiness and validation state\n";
    text << "- quick metrics and warnings\n";
    text << "- future plots, tables, and result summaries\n";
    text << "- comparison snapshots for parameter sweeps\n";
    text << "- dockable output tabs as analysis grows\n\n";
    text << "Current live authoring slice:\n";
    text << "- scene object creation, duplication, and deletion\n";
    text << "- dedicated modifier stack panel with mirror and shell entries\n";
    text << "- geometry-element panel for vertex, edge, and face proxy selection\n";
    text << "- direct geometry tool panel for translate, scale, extrude, and bevel\n";
    text << "- primitive, modifier-stack, and direct-geometry workflow states\n\n";
    text << "Analysis stays close to authoring so the workstation feels like engineering, not disconnected utilities.";

    if (lastRegistryError.isNotEmpty())
        text << "\n\nRegistry message: " << lastRegistryError;

    return text;
}

juce::String MainComponent::platformSummaryText() const
{
    const auto runtime = creation::services::SuiteAiSettingsResolver::resolveRuntimeSettingsForApp(suiteAiSettings,
                                                                                                    currentDomain());
    const auto configDirectory = suiteSettingsStore.getSuiteConfigDirectory().getFullPathName();
    const auto containersDirectory = creation::suite::getProjectContainerDirectory(suiteSettings).getFullPathName();

    juce::String text;
    text << "Suite config directory: " << configDirectory << "\n";
    text << "Project container root: " << containersDirectory << "\n";
    text << "Suite VFS root: " << suiteSettings.suiteVfsRoot << "\n";
    text << "Resolved AI provider: " << runtime.providerDisplayName << "\n";
    text << "Resolved AI model: " << runtime.modelName << "\n";
    text << "Language domain token: " << juce::String(creation_engineer::language::getAppDomainName()) << "\n";
    text << "Layout file: " << getLayoutFile().getFullPathName() << "\n\n";
    text << "Engineer now uses the suite docking-workbench pattern, which later apps can adopt while Tracker retains special status.";
    return text;
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(creation_engineer::branding::backgroundColour());

    auto bounds = getLocalBounds().toFloat().reduced(18.0f);
    g.setColour(creation_engineer::branding::panelColour());
    g.fillRoundedRectangle(bounds, 24.0f);

    g.setColour(creation_engineer::branding::accentColour().withAlpha(0.8f));
    g.drawRoundedRectangle(bounds, 24.0f, 1.4f);
}

void MainComponent::resized()
{
    headerBar.setBounds(getLocalBounds().removeFromTop(96));

    auto area = getLocalBounds().reduced(32, 26);
    area.removeFromTop(86);

    titleLabel.setBounds(area.removeFromTop(38));
    subtitleLabel.setBounds(area.removeFromTop(28));
    runtimeLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(16);

    if (dockManager)
        dockManager->setBounds(area);
}
