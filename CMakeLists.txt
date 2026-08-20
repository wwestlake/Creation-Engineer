cmake_minimum_required(VERSION 3.22)

set(CreationEngineer_VERSION "0.0.1" CACHE STRING "Creation Engineer product version")
project(CreationEngineer VERSION ${CreationEngineer_VERSION} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/CMake/CreationSuiteBuildSettings.cmake")
creation_suite_require_juce()

add_subdirectory("${JUCE_DIR}" juce)

set(CREATION_ENGINE_APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../CreationEngine")
set(CREATION_ENGINE_SOURCE_DIR "${CREATION_ENGINE_APP_DIR}/Source")

if(EXISTS "${CREATION_ENGINE_APP_DIR}/third_party/entt/CMakeLists.txt")
    add_subdirectory("${CREATION_ENGINE_APP_DIR}/third_party/entt"
                     "${CMAKE_CURRENT_BINARY_DIR}/CreationEngineThirdPartyEntt")
else()
    message(FATAL_ERROR "Could not find the entt dependency in apps/CreationEngine. Run `git submodule update --init --recursive` first.")
endif()

if(EXISTS "${CREATION_ENGINE_APP_DIR}/NodeSystem/CMakeLists.txt")
    add_subdirectory("${CREATION_ENGINE_APP_DIR}/NodeSystem"
                     "${CMAKE_CURRENT_BINARY_DIR}/CreationEngineNodeSystem")
    if(NOT TARGET creation_suite_node_system)
        add_library(creation_suite_node_system ALIAS node_system)
    endif()
else()
    message(FATAL_ERROR "Could not find NodeSystem in apps/CreationEngine.")
endif()

if(EXISTS "${CREATION_ENGINE_APP_DIR}/EngineCore/CMakeLists.txt")
    add_subdirectory("${CREATION_ENGINE_APP_DIR}/EngineCore"
                     "${CMAKE_CURRENT_BINARY_DIR}/CreationEngineCore")
else()
    message(FATAL_ERROR "Could not find EngineCore in apps/CreationEngine.")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../shared/CEL/CMakeLists.txt")
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/CEL"
                     "${CMAKE_CURRENT_BINARY_DIR}/CreationSuiteSharedCel")
else()
    message(FATAL_ERROR "Could not find shared/CEL.")
endif()

if(EXISTS "${CREATION_ENGINE_APP_DIR}/Language/CMakeLists.txt")
    add_subdirectory("${CREATION_ENGINE_APP_DIR}/Language"
                     "${CMAKE_CURRENT_BINARY_DIR}/CreationEngineLanguage")
else()
    message(FATAL_ERROR "Could not find Language in apps/CreationEngine.")
endif()

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/VFS" CreationSuiteSharedVFS)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/AssetSystem" CreationSuiteSharedAssetSystem)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/Services" CreationSuiteSharedServices)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../shared/UI" CreationSuiteSharedUI)

juce_add_gui_app(CreationEngineerApp
    PRODUCT_NAME "Creation Engineer"
    COMPANY_NAME "LagDaemon Software"
    BUNDLE_ID "com.lagdaemon.creationengineer"
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/../../shared/UI/assets/creation_engineer.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/../../shared/UI/assets/creation_engineer.png"
)

creation_suite_add_shared_bin_copy(CreationEngineerApp)
juce_generate_juce_header(CreationEngineerApp)

file(GLOB_RECURSE CREATION_ENGINE_RENDER_SOURCES CONFIGURE_DEPENDS
    "${CREATION_ENGINE_SOURCE_DIR}/Render/*.cpp"
    "${CREATION_ENGINE_SOURCE_DIR}/Render/*.h"
)

file(GLOB_RECURSE CREATION_ENGINE_VIEW_SOURCES CONFIGURE_DEPENDS
    "${CREATION_ENGINE_SOURCE_DIR}/Views/*.cpp"
    "${CREATION_ENGINE_SOURCE_DIR}/Views/*.h"
)

file(GLOB_RECURSE CREATION_ENGINE_SCENE_SOURCES CONFIGURE_DEPENDS
    "${CREATION_ENGINE_SOURCE_DIR}/Scene/*.cpp"
    "${CREATION_ENGINE_SOURCE_DIR}/Scene/*.h"
)

file(GLOB_RECURSE CREATION_ENGINE_IMPORT_SOURCES CONFIGURE_DEPENDS
    "${CREATION_ENGINE_SOURCE_DIR}/Import/*.cpp"
    "${CREATION_ENGINE_SOURCE_DIR}/Import/*.h"
)

file(GLOB_RECURSE CREATION_ENGINE_AUDIO_SOURCES CONFIGURE_DEPENDS
    "${CREATION_ENGINE_SOURCE_DIR}/Audio/*.cpp"
    "${CREATION_ENGINE_SOURCE_DIR}/Audio/*.h"
)

target_sources(CreationEngineerApp PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/Branding.cpp
    Source/Branding.h
    ${CREATION_ENGINE_RENDER_SOURCES}
    ${CREATION_ENGINE_VIEW_SOURCES}
    ${CREATION_ENGINE_SCENE_SOURCES}
    ${CREATION_ENGINE_IMPORT_SOURCES}
    ${CREATION_ENGINE_AUDIO_SOURCES}
)

target_include_directories(CreationEngineerApp PRIVATE
    Source
    "${CREATION_ENGINE_SOURCE_DIR}"
    "${CREATION_ENGINE_APP_DIR}/third_party"
)

target_compile_definitions(CreationEngineerApp PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_OPENGL3=1
    CE_ENABLE_SCRIPTING=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:CreationEngineerApp,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:CreationEngineerApp,JUCE_VERSION>"
    CE_SHADER_SOURCE_DIR="${CREATION_ENGINE_SOURCE_DIR}/Render/Shaders"
    CE_ASSET_SOURCE_DIR="${CREATION_ENGINE_APP_DIR}/assets"
)

target_link_libraries(CreationEngineerApp PRIVATE
    engine_core
    node_system
    ce_lang_frontend
    ce_lang_nodegen
    ce_engine_lang_world
    creation_suite_asset_system
    creation_suite_services
    creation_suite_ui
    creation_suite_ui_assets
    juce::juce_gui_extra
    juce::juce_opengl
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
