# The application target. Sources are contributed by the subdirectories
# (Audio/, UI/, Shared/) via target_sources so the layout matches the
# architecture described in DEVELOPMENT.md.

add_executable(DesktopAmpSimulator)

target_sources(DesktopAmpSimulator PRIVATE
    Main.cpp)

add_subdirectory(Audio)
add_subdirectory(UI)
add_subdirectory(Shared)
add_subdirectory(DSP)

target_include_directories(DesktopAmpSimulator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(DesktopAmpSimulator PRIVATE
    JUCE_STANDALONE_APPLICATION=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=1
    $<$<BOOL:${ASIO_SDK_DIR}>:JUCE_ASIO=1>)

if(ASIO_SDK_DIR)
    target_include_directories(DesktopAmpSimulator PRIVATE "${ASIO_SDK_DIR}/common")
endif()

target_link_libraries(DesktopAmpSimulator PRIVATE
    ${AMPSIM_QT_LIBS}
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags)

# Warnings-as-errors on our own sources only. JUCE's module sources are
# compiled into this target via its INTERFACE libraries, so target-wide
# flags would apply to JUCE too — scope them per-file instead.
if(MSVC)
    set(AMPSIM_WARNING_FLAGS "/W4;/WX;/permissive-")
else()
    set(AMPSIM_WARNING_FLAGS "-Wall;-Wextra;-Werror")
endif()

set_source_files_properties(
    Main.cpp
    Audio/AudioEngine.cpp
    UI/MainWindow.cpp
    UI/Widgets/CustomKnob.cpp
    UI/Widgets/LevelMeter.cpp
    UI/Sections/AmpSection.cpp
    UI/Sections/CabinetSection.cpp
    UI/Sections/EffectsSection.cpp
    UI/Sections/MasterSection.cpp
    UI/Panels/PresetPanel.cpp
    PROPERTIES COMPILE_OPTIONS "${AMPSIM_WARNING_FLAGS}")

# GUI app (no console window on Windows, .app bundle on macOS)
set_target_properties(DesktopAmpSimulator PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "DesktopAmpSimulator"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.ampsim.desktopampsimulator"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
