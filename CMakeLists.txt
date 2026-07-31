cmake_minimum_required(VERSION 3.16)

project(DesktopAmpSimulator
    VERSION 0.1.0
    DESCRIPTION "Cross-platform desktop guitar amp simulator"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(AMPSIM_BUILD_TESTS "Build unit tests" ON)

# Optional ASIO support (Windows). The Steinberg ASIO SDK is not
# redistributable, so the user must download it and point us at it:
#   cmake -DASIO_SDK_DIR=C:/SDKs/asiosdk ..
set(ASIO_SDK_DIR "" CACHE PATH "Path to the Steinberg ASIO SDK (Windows only)")

# ---------------------------------------------------------------------------
# JUCE (fetched automatically — no manual download required)
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

# ---------------------------------------------------------------------------
# Qt (autodetect: prefer Qt6, fall back to Qt5)
# ---------------------------------------------------------------------------
find_package(Qt6 QUIET COMPONENTS Widgets)
if(Qt6_FOUND)
    set(AMPSIM_QT_LIBS Qt6::Widgets)
    message(STATUS "Using Qt6 (${Qt6_VERSION})")
else()
    find_package(Qt5 5.12 REQUIRED COMPONENTS Widgets)
    set(AMPSIM_QT_LIBS Qt5::Widgets)
    message(STATUS "Using Qt5 (${Qt5_VERSION})")
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

add_subdirectory(src)

if(AMPSIM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
