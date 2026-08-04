include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.4
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(AmpSimTests
    AudioEngineTests.cpp
    MeterTests.cpp
    AmpModelTests.cpp
    EffectsTests.cpp)

target_include_directories(AmpSimTests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(AmpSimTests PRIVATE Catch2::Catch2WithMain)

if(MSVC)
    target_compile_options(AmpSimTests PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(AmpSimTests PRIVATE -Wall -Wextra -Werror)
endif()

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(AmpSimTests)
