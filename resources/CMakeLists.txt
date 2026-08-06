target_sources(DesktopAmpSimulator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/resources.qrc)

# Native .exe icon (Explorer, taskbar, Alt-Tab). The runtime QIcon set from
# icons/app_icon.png in Main.cpp covers the window/title-bar icon on all
# platforms; this additionally covers the file icon itself on Windows.
if(WIN32)
    target_sources(DesktopAmpSimulator PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/app_icon.rc)
endif()
