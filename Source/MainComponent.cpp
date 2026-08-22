#include "MainComponent.h"

#include <creation/services/SuiteVfsJsonStore.h>
#include <creation/ui/CreationSuiteLogos.h>

#include "EngineerSceneSerialization.h"

namespace
{
// Wraps an existing component (not owned) as a dock panel's content, filling
// whatever bounds the dock zone/tab gives it.
class NonOwningPanelHost final : public juce::Component
{
public:
    explicit NonOwningPanelHost(juce::Component& contentToHost) : content(contentToHost)
    {
        addAndMakeVisible(content);
    }

    void resized() override
    {
        content.setBounds(getLocalBounds());
    }

private:
    juce::Component& content;
};

const juce::String panelIdNavigator = "navigator";
const juce::String panelIdViewportDesign3D = "viewportDesign3D";
const juce::String panelIdViewportAssemblyFloor = "viewportAssemblyFloor";
const juce::String panelIdViewportPlanarElectronics = "viewportPlanarElectronics";
const juce::String panelIdProperties = "properties";
const juce::String panelIdModifiers = "modifiers";
const juce::String panelIdGeometryElements = "geometryElements";
const juce::String panelIdGeometryTools = "geometryTools";
const juce::String panelIdLibrary = "library";
const juce::String panelIdLayers = "layers";

constexpr int menuIdPanelNavigator = 3001;
constexpr int menuIdPanelViewportDesign3D = 3002;
constexpr int menuIdPanelProperties = 3003;
constexpr int menuIdPanelModifiers = 3004;
constexpr int menuIdPanelGeometryElements = 3005;
constexpr int menuIdPanelGeometryTools = 3006;
constexpr int menuIdResetLayout = 3007;
constexpr int menuIdPanelViewportAssemblyFloor = 3008;
constexpr int menuIdPanelViewportPlanarElectronics = 3009;
constexpr int menuIdPanelLibrary = 3010;
constexpr int menuIdPanelLayers = 3011;
constexpr int menuIdSaveDrawing = 3012;

constexpr const char* kEngineerDrawingEntryPath = "Project/engineer-drawing.json";
}

MainComponent::MainComponent()
    : navigatorPanel_(sceneModel_),
      propertiesPanel_(sceneModel_),
      modifiersPanel_(sceneModel_),
      geometryElementsPanel_(sceneModel_),
      geometryToolsPanel_(sceneModel_),
      libraryPanel_(sceneModel_, combinedSpecLibrary_, userSpecLibrary_, [this] { onUserSpecLibraryChanged(); }),
      layersPanel_(sceneModel_),
      viewportDesign3D_(sceneModel_, EngineerViewportComponent::ViewMode::design3D, combinedSpecLibrary_),
      viewportAssemblyFloor_(sceneModel_, EngineerViewportComponent::ViewMode::assemblyFloor, combinedSpecLibrary_),
      viewportPlanarElectronics_(sceneModel_, EngineerViewportComponent::ViewMode::planarElectronics, combinedSpecLibrary_) {
    sceneModel_.addListener(this);

    juce::String suiteErr;
    suiteSettings_ = suiteSettingsStore_.load(suiteErr);

    // Builtin (generic, shipped) + user-authored (persisted separately) part
    // library, combined into combinedSpecLibrary_ before any viewport or the
    // library panel ever reads from it -- see the parametric-part-libraries
    // plan's Part B/F. A missing/empty user library file is not an error,
    // same convention engineer-settings.json already uses.
    juce::String builtinSpecError;
    creation::engineering::loadBuiltinSpecLibrary(juce::File(ENGINEERING_SPECS_DATA_DIR), builtinSpecLibrary_,
                                                  builtinSpecError);

    juce::String userLibraryLoadError;
    creation::engineering::fromVar(
        creation::services::SuiteVfsJsonStore::loadJson("engineering-user-library.json", userLibraryLoadError),
        userSpecLibrary_);

    combinedSpecLibrary_ = builtinSpecLibrary_;
    combinedSpecLibrary_.merge(userSpecLibrary_);

    headerBar_.setAppTitle("Creation Engineer");
    headerBar_.setLogoImage(creation::ui::getSuiteLogoImage(creation::ui::SuiteLogoId::engineer));
    headerBar_.setProjectLabel("Project: Untitled Engineer");
    headerBar_.audioButton.setButtonText("Engineer");
    headerBar_.tourButton.setButtonText("Tools");
    headerBar_.setTransportButtonVisible(CreationSuiteHeaderBar::TransportButtonSlot::rewind, false);
    headerBar_.setTransportButtonVisible(CreationSuiteHeaderBar::TransportButtonSlot::fastForward, false);
    headerBar_.setTransportButtonVisible(CreationSuiteHeaderBar::TransportButtonSlot::record, false);
    headerBar_.setTransportButtonVisible(CreationSuiteHeaderBar::TransportButtonSlot::loop, false);
    headerBar_.setTransportButtonVisible(CreationSuiteHeaderBar::TransportButtonSlot::click, false);
    // No simulation to play/pause/stop -- this is a geometry editor, not the
    // old borrowed Engine shell's play-in-viewport transport.
    headerBar_.setTransportButtonVisible(CreationSuiteHeaderBar::TransportButtonSlot::stop, false);
    headerBar_.setTransportButtonVisible(CreationSuiteHeaderBar::TransportButtonSlot::playPause, false);
    suiteShellController_.attach(headerBar_,
                                 {
                                     "Creation Engineer",
                                     creation::assets::SuiteAppDomain::engineer,
                                     juce::Colour(0xff15181d),
                                     creation::ui::SuiteAssetManagerCapability{ "Creation Engineer",
                                                                                creation::assets::SuiteAppDomain::engineer,
                                                                                { ".cel" },
                                                                                { ".cel" } }
                                 },
                                 [this](const juce::String& status)
                                 {
                                     headerBar_.setStatusText(status);
                                 });
    suiteShellController_.onProjectOpenRequested = [this](const juce::String& projectId)
    {
        openProject(projectId);
    };
    headerBar_.onProjectMenuRequested = [this]
    {
        suiteShellController_.showProjectBrowser();
    };
    addAndMakeVisible(headerBar_);
    headerBar_.setStatusText("Editing");

    // navigatorPanel_/propertiesPanel_/modifiersPanel_/geometryElementsPanel_/
    // geometryToolsPanel_/viewport_ are reparented into dock panels below (see
    // initialiseDockingWorkspace) -- they self-register with sceneModel_ via
    // EngineerSceneModel::Listener in their own constructors, so no manual
    // cross-panel wiring is needed here (contrast the old hierarchyPanel_.
    // onSelectionChanged fan-out this MainComponent used to hand-wire).

    menuBar_ = std::make_unique<juce::MenuBarComponent>(static_cast<juce::MenuBarModel*>(this));
    // Nothing in this app sets a suite-wide dark LookAndFeel, so MenuBarComponent
    // falls back to LookAndFeel_V4::drawMenuBarItem/drawMenuBarBackground, which key
    // off TextButton colour ids (not PopupMenu's) -- the default scheme renders dark
    // text on a dark bar, invisible against this app's dark theme without this.
    menuBar_->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1c2230));
    menuBar_->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff2a3244));
    menuBar_->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    menuBar_->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    addAndMakeVisible(*menuBar_);

    dockManager_ = std::make_unique<CreationDock::DockManager>(*this);
    addAndMakeVisible(*dockManager_);
    initialiseDockingWorkspace();
    // setSize() below fires resized() immediately; menuBar_/dockManager_ must
    // already exist and be registered before that happens, or they're silently
    // left at zero bounds (addAndMakeVisible alone doesn't trigger a layout pass).
    resized();

    setSize(1400, 900);
}

MainComponent::~MainComponent()
{
    sceneModel_.removeListener(this);
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(0xff15181d));
}

void MainComponent::resized() {
    auto bounds = getLocalBounds();

    headerBar_.setBounds(bounds.removeFromTop(96));

    if (menuBar_ != nullptr)
        menuBar_->setBounds(bounds.removeFromTop(28));

    if (dockManager_ != nullptr)
        dockManager_->setBounds(bounds);
}

juce::StringArray MainComponent::getMenuBarNames() {
    return { "File", "Panels" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int topLevelMenuIndex, const juce::String&) {
    juce::PopupMenu menu;

    if (topLevelMenuIndex == 0) {
        menu.addItem(menuIdSaveDrawing, "Save Drawing", projectDirty_);
        return menu;
    }

    const auto isOpen = [this](const juce::String& id) {
        return dockManager_ != nullptr && dockManager_->isPanelOpen(id);
    };

    menu.addItem(menuIdPanelNavigator, "Navigator", true, isOpen(panelIdNavigator));
    menu.addItem(menuIdPanelViewportDesign3D, "3D Design", true, isOpen(panelIdViewportDesign3D));
    menu.addItem(menuIdPanelViewportAssemblyFloor, "Assembly Floor", true, isOpen(panelIdViewportAssemblyFloor));
    menu.addItem(menuIdPanelViewportPlanarElectronics, "Planar/Electronics", true, isOpen(panelIdViewportPlanarElectronics));
    menu.addItem(menuIdPanelProperties, "Properties", true, isOpen(panelIdProperties));
    menu.addItem(menuIdPanelModifiers, "Modifiers", true, isOpen(panelIdModifiers));
    menu.addItem(menuIdPanelGeometryElements, "Geometry Elements", true, isOpen(panelIdGeometryElements));
    menu.addItem(menuIdPanelGeometryTools, "Geometry Tools", true, isOpen(panelIdGeometryTools));
    menu.addItem(menuIdPanelLibrary, "Part Library", true, isOpen(panelIdLibrary));
    menu.addItem(menuIdPanelLayers, "Layers", true, isOpen(panelIdLayers));
    menu.addSeparator();
    menu.addItem(menuIdResetLayout, "Reset Dock Layout");
    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int) {
    switch (menuItemID) {
        case menuIdPanelNavigator:        toggleDockPanel(panelIdNavigator, CreationDock::DockTargetZone::Left); break;
        case menuIdPanelViewportDesign3D: toggleDockPanel(panelIdViewportDesign3D, CreationDock::DockTargetZone::CenterTab); break;
        case menuIdPanelViewportAssemblyFloor: toggleDockPanel(panelIdViewportAssemblyFloor, CreationDock::DockTargetZone::CenterTab); break;
        case menuIdPanelViewportPlanarElectronics: toggleDockPanel(panelIdViewportPlanarElectronics, CreationDock::DockTargetZone::CenterTab); break;
        case menuIdPanelProperties:       toggleDockPanel(panelIdProperties, CreationDock::DockTargetZone::Right); break;
        case menuIdPanelModifiers:        toggleDockPanel(panelIdModifiers, CreationDock::DockTargetZone::Right); break;
        case menuIdPanelGeometryElements: toggleDockPanel(panelIdGeometryElements, CreationDock::DockTargetZone::Bottom); break;
        case menuIdPanelGeometryTools:    toggleDockPanel(panelIdGeometryTools, CreationDock::DockTargetZone::Bottom); break;
        case menuIdPanelLibrary:          toggleDockPanel(panelIdLibrary, CreationDock::DockTargetZone::Left); break;
        case menuIdPanelLayers:           toggleDockPanel(panelIdLayers, CreationDock::DockTargetZone::Right); break;
        case menuIdResetLayout:           if (dockManager_ != nullptr) dockManager_->resetLayout(); break;
        case menuIdSaveDrawing:           saveSessionToDisk(true); break;
        default: break;
    }

    menuItemsChanged();
}

void MainComponent::initialiseDockingWorkspace() {
    if (dockManager_ == nullptr)
        return;

    dockManager_->registerPanel(panelIdNavigator, "Navigator",
        std::make_unique<NonOwningPanelHost>(navigatorPanel_), CreationDock::DockTargetZone::Left);
    dockManager_->registerPanel(panelIdViewportDesign3D, "3D Design",
        std::make_unique<NonOwningPanelHost>(viewportDesign3D_), CreationDock::DockTargetZone::CenterTab);
    dockManager_->registerPanel(panelIdViewportAssemblyFloor, "Assembly Floor",
        std::make_unique<NonOwningPanelHost>(viewportAssemblyFloor_), CreationDock::DockTargetZone::CenterTab);
    dockManager_->registerPanel(panelIdViewportPlanarElectronics, "Planar/Electronics",
        std::make_unique<NonOwningPanelHost>(viewportPlanarElectronics_), CreationDock::DockTargetZone::CenterTab);
    dockManager_->registerPanel(panelIdProperties, "Properties",
        std::make_unique<NonOwningPanelHost>(propertiesPanel_), CreationDock::DockTargetZone::Right);
    dockManager_->registerPanel(panelIdModifiers, "Modifiers",
        std::make_unique<NonOwningPanelHost>(modifiersPanel_), CreationDock::DockTargetZone::Right);
    dockManager_->registerPanel(panelIdGeometryElements, "Geometry Elements",
        std::make_unique<NonOwningPanelHost>(geometryElementsPanel_), CreationDock::DockTargetZone::Bottom);
    dockManager_->registerPanel(panelIdGeometryTools, "Geometry Tools",
        std::make_unique<NonOwningPanelHost>(geometryToolsPanel_), CreationDock::DockTargetZone::Bottom);
    dockManager_->registerPanel(panelIdLibrary, "Part Library",
        std::make_unique<NonOwningPanelHost>(libraryPanel_), CreationDock::DockTargetZone::Left);
    dockManager_->registerPanel(panelIdLayers, "Layers",
        std::make_unique<NonOwningPanelHost>(layersPanel_), CreationDock::DockTargetZone::Right);

    // All ten start open -- a small, cohesive panel set with no reason to
    // hide any of them by default (unlike Engine's decision to close its
    // less-used modes/placeholders). All three viewports share the centre
    // tab group, same as the single viewport used to occupy -- dragging any
    // tab out into its own window is free via CreationDock::FloatingDockWindow.
}

void MainComponent::toggleDockPanel(const juce::String& panelId, CreationDock::DockTargetZone fallbackZone) {
    if (dockManager_ == nullptr)
        return;

    if (dockManager_->isPanelOpen(panelId))
        dockManager_->closePanel(panelId);
    else
        dockManager_->showPanel(panelId, fallbackZone);

    menuItemsChanged();
}

void MainComponent::createNewProject()
{
    auto* prompt = new juce::AlertWindow("Create New Engineer Project",
                                         "Enter a name for your new Creation Engineer project container:",
                                         juce::MessageBoxIconType::QuestionIcon);
    prompt->addTextEditor("projectName", "");
    prompt->addButton("Create Project", 1);
    prompt->addButton("Cancel", 0);

    auto options = juce::Component::SafePointer<MainComponent>(this);
    prompt->enterModalState(true, juce::ModalCallbackFunction::create([options, prompt](int result) mutable
    {
        std::unique_ptr<juce::AlertWindow> dialog(prompt);
        if (result != 1 || options == nullptr)
            return;

        auto name = dialog->getTextEditorContents("projectName").trim();
        if (name.isEmpty())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Project Error", "Project name cannot be empty.");
            return;
        }

        juce::String err;
        if (! creation::assets::ProjectWorkspaceService::createProject(options->suiteSettings_, creation::assets::SuiteAppDomain::engineer, name, "1.0.0", "1.0.0", options->projectSession_, err))
        {
            juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Project Error", err);
            return;
        }

        options->headerBar_.setProjectLabel("Project: " + options->projectSession_.getManifest().projectName);
        options->saveSessionToDisk(true);
        options->saveAppSettings();
        options->headerBar_.setStatusText("Created project: " + options->projectSession_.getManifest().projectName);
    }), true);
}

void MainComponent::openProject(const juce::String& projectId)
{
    juce::String err;
    if (! creation::assets::ProjectWorkspaceService::openProject(suiteSettings_, projectId, projectSession_, err))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Project Error", err);
        return;
    }

    headerBar_.setProjectLabel("Project: " + projectSession_.getManifest().projectName);
    loadSessionFromDisk();
    saveAppSettings();
}

void MainComponent::saveSessionToDisk(bool userInitiated)
{
    if (! projectSession_.isValid())
    {
        if (userInitiated)
            headerBar_.setStatusText("No active project session to save.");
        return;
    }

    const auto drawingJson = juce::JSON::toString(EngineerSceneSerialization::toVar(sceneModel_), true);
    const juce::MemoryBlock drawingData(drawingJson.toRawUTF8(), drawingJson.getNumBytesAsUTF8());
    if (! projectSession_.writeEntry(kEngineerDrawingEntryPath, drawingData))
    {
        headerBar_.setStatusText("Project save failed: could not write drawing data.");
        return;
    }

    juce::String commitError;
    if (! projectSession_.commit(commitError))
    {
        headerBar_.setStatusText("Project save failed: " + commitError);
        return;
    }

    projectDirty_ = false;
    menuItemsChanged();

    if (userInitiated)
        headerBar_.setStatusText("Project saved: " + projectSession_.getManifest().projectName);
}

void MainComponent::loadSessionFromDisk()
{
    juce::MemoryBlock drawingData;
    if (! projectSession_.readEntry(kEngineerDrawingEntryPath, drawingData))
        return; // Nothing saved yet for this project -- keep sceneModel_'s constructor default.

    const auto jsonText = juce::String::fromUTF8(static_cast<const char*>(drawingData.getData()),
                                                  static_cast<int>(drawingData.getSize()));
    const auto parsed = juce::JSON::parse(jsonText);

    juce::String loadError;
    if (! EngineerSceneSerialization::fromVar(parsed, sceneModel_, loadError))
    {
        juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Could Not Load Drawing", loadError);
        return;
    }

    projectDirty_ = false;
    menuItemsChanged();
}

bool MainComponent::ensureProjectSessionActive(juce::String& errorMessage)
{
    if (projectSession_.isValid())
        return true;

    juce::String settingsError;
    auto settings = creation::services::SuiteVfsJsonStore::loadJson("engineer-settings.json", settingsError);
    if (auto* settingsObject = settings.getDynamicObject())
    {
        auto lastProjectId = settingsObject->getProperty("lastOpenedProjectId").toString();
        if (lastProjectId.isNotEmpty())
        {
            if (creation::assets::ProjectWorkspaceService::openProject(suiteSettings_, lastProjectId, projectSession_, errorMessage))
            {
                headerBar_.setProjectLabel("Project: " + projectSession_.getManifest().projectName);
                loadSessionFromDisk();
                return true;
            }
        }
    }

    auto availableProjects = creation::assets::ProjectContainerService::listProjects(
        suiteSettings_, creation::assets::SuiteAppDomain::engineer, errorMessage);

    if (! availableProjects.isEmpty())
    {
        if (creation::assets::ProjectWorkspaceService::openProject(suiteSettings_, availableProjects.getFirst().projectId, projectSession_, errorMessage))
        {
            headerBar_.setProjectLabel("Project: " + projectSession_.getManifest().projectName);
            loadSessionFromDisk();
            return true;
        }
    }

    createNewProject();
    return false;
}

void MainComponent::saveAppSettings()
{
    auto* object = new juce::DynamicObject();
    if (projectSession_.isValid())
        object->setProperty("lastOpenedProjectId", projectSession_.getProjectId());

    juce::String errorMessage;
    creation::services::SuiteVfsJsonStore::saveJson("engineer-settings.json", juce::var(object), errorMessage);
}

void MainComponent::loadAppSettings()
{
}

void MainComponent::engineerSceneModelChanged()
{
    projectDirty_ = true;
    menuItemsChanged();
}

void MainComponent::onUserSpecLibraryChanged()
{
    juce::String saveError;
    creation::services::SuiteVfsJsonStore::saveJson("engineering-user-library.json",
                                                     creation::engineering::toVar(userSpecLibrary_), saveError);

    combinedSpecLibrary_ = builtinSpecLibrary_;
    combinedSpecLibrary_.merge(userSpecLibrary_);
}
