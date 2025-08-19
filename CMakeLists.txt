cmake_minimum_required(VERSION 3.21)

# ---- Project ----
project(TriBeat
    VERSION 1.0.0
    DESCRIPTION "Tri-Beat: Polyrhythm/Polymeter visual metronome with layered shapes"
    LANGUAGES CXX)

# ---- Options ----
option(TRIBEAT_FETCH_JUCE "Fetch JUCE with FetchContent (otherwise add_subdirectory or find_package)" ON)

# C++ standard & warnings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---- JUCE ----
if (TRIBEAT_FETCH_JUCE)
    include(FetchContent)

    set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(JUCE_BUILD_EXTRAS   OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.6
    )
    FetchContent_MakeAvailable(JUCE)
else()
    find_package(JUCE CONFIG REQUIRED) # or add_subdirectory(path/to/JUCE)
endif()

# ---- App Target ----
juce_add_gui_app(TriBeat
    COMPANY_NAME "Danial Kooshki"
    PRODUCT_NAME "Tri-Beat"
    BUNDLE_ID    com.danialkooshki.tribeat
    # ICON_BIG    "${CMAKE_SOURCE_DIR}/assets/icon.ico"  # (optional)
)

# Source files 
target_sources(TriBeat PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/OnboardingOverlay.h
)

juce_generate_juce_header(TriBeat)

# JUCE config
target_compile_definitions(TriBeat PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:TriBeat,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"

    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_MODAL_LOOPS_PERMITTED=1

    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

# JUCE modules you use
target_link_libraries(TriBeat PRIVATE
    juce::juce_gui_basics
    juce::juce_data_structures
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    # juce::juce_gui_extra      # enable if you start using it
    # juce::juce_audio_utils    # enable if you start using extra audio utils widgets
)

# Install rules 
include(GNUInstallDirs)
install(TARGETS TriBeat
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE  DESTINATION .
)

# Helpful for debugging in VS
set_target_properties(TriBeat PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
