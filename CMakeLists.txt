cmake_minimum_required(VERSION 3.22)

set(CreationEngineer_VERSION "0.0.1" CACHE STRING "Creation Engineer product version")
project(CreationEngineer VERSION ${CreationEngineer_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/CMake/CreationSuiteBuildSettings.cmake")
creation_suite_require_juce()

add_subdirectory("${JUCE_DIR}" juce)

# This app no longer borrows CreationEngine's Render/Views/Scene/Import/Audio
# trees (or its entt/NodeSystem/EngineCore/CEL/Language dependency chain) --
# it has its own self-contained geometry workspace (EngineerSceneModel + six
# panels, Source/Engineer*.{h,cpp}) that touches none of that. See the docking
# rollout plan for how/why this got dropped.
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/VFS" CreationSuiteSharedVFS)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/AssetSystem" CreationSuiteSharedAssetSystem)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/Services" CreationSuiteSharedServices)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/UI" CreationSuiteSharedUI)
if(NOT TARGET CreationDock)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/JuceDocking" CreationSuiteSharedDocking)
endif()
if(NOT TARGET creation_suite_render)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/Render" CreationSuiteSharedRender)
endif()
if(NOT TARGET creation_suite_engineering_specs)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/EngineeringSpecs" CreationSuiteSharedEngineeringSpecs)
endif()

juce_add_gui_app(CreationEngineerApp
    PRODUCT_NAME "Creation Engineer"
    COMPANY_NAME "LagDaemon Software"
    BUNDLE_ID "com.lagdaemon.creationengineer"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/../../shared/UI/assets/creation_engineer.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/../../shared/UI/assets/creation_engineer.png"
)

creation_suite_add_shared_bin_copy(CreationEngineerApp)
juce_generate_juce_header(CreationEngineerApp)

target_sources(CreationEngineerApp PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/Branding.cpp
    Source/Branding.h
    Source/EngineerSceneModel.cpp
    Source/EngineerSceneModel.h
    Source/EngineerViewportComponent.cpp
    Source/EngineerViewportComponent.h
    Source/EngineerOrthoCameraController.cpp
    Source/EngineerOrthoCameraController.h
    Source/EngineerNavigatorComponent.cpp
    Source/EngineerNavigatorComponent.h
    Source/EngineerPropertiesComponent.cpp
    Source/EngineerPropertiesComponent.h
    Source/EngineerModifiersComponent.cpp
    Source/EngineerModifiersComponent.h
    Source/EngineerGeometryElementsComponent.cpp
    Source/EngineerGeometryElementsComponent.h
    Source/EngineerGeometryToolsComponent.cpp
    Source/EngineerGeometryToolsComponent.h
    Source/EngineerLibraryComponent.cpp
    Source/EngineerLibraryComponent.h
    Source/EngineerUnits.cpp
    Source/EngineerUnits.h
    Source/EngineerLayersComponent.cpp
    Source/EngineerLayersComponent.h
    Source/EngineerSceneSerialization.cpp
    Source/EngineerSceneSerialization.h
)

target_include_directories(CreationEngineerApp PRIVATE
    Source
    "${CMAKE_CURRENT_SOURCE_DIR}/../../shared/JuceDocking/include"
)

target_compile_definitions(CreationEngineerApp PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_OPENGL3=1
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:CreationEngineerApp,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:CreationEngineerApp,JUCE_VERSION>"
    ENGINEER_SHADER_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../shared/Render/Shaders"
    ENGINEERING_SPECS_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../shared/EngineeringSpecs/data"
)

target_link_libraries(CreationEngineerApp PRIVATE
    creation_suite_asset_system
    creation_suite_services
    creation_suite_ui
    creation_suite_ui_assets
    creation_suite_render
    creation_suite_engineering_specs
    CreationDock
    juce::juce_gui_extra
    juce::juce_opengl
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
