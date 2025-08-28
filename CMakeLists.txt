cmake_minimum_required(VERSION 3.20)
project(EtherSynth VERSION 1.0.0 LANGUAGES CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build configuration
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Platform detection
if(APPLE)
    set(PLATFORM_MAC TRUE)
    add_definitions(-DPLATFORM_MAC)
elseif(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    add_definitions(-DPLATFORM_WINDOWS)
else()
    set(PLATFORM_STM32 TRUE)
    add_definitions(-DPLATFORM_STM32)
endif()

# Source files - Core (platform independent)
set(CORE_SOURCES
    src/core/EtherSynth.cpp
    src/audio/AudioEngine.cpp
    src/audio/VoiceManager.cpp
    src/audio/AudioBuffer.cpp
    src/synthesis/SynthEngine.cpp
    src/synthesis/SubtractiveEngine.cpp
    src/synthesis/FMEngine.cpp
    src/synthesis/PlaitsEngine.cpp
    src/synthesis/GranularEngine.cpp
    src/synthesis/SamplerEngine.cpp
    src/instruments/InstrumentSlot.cpp
    src/instruments/ChordGenerator.cpp
    src/instruments/DrumMachine.cpp
    src/sequencer/Timeline.cpp
    src/sequencer/EuclideanRhythm.cpp
    src/sequencer/Arpeggiator.cpp
    src/sequencer/M8StyleFX.cpp
    src/modulation/ModulationMatrix.cpp
    src/modulation/LFO.cpp
    src/modulation/Envelope.cpp
    src/effects/EffectsChain.cpp
    src/effects/Reverb.cpp
    src/effects/Filter.cpp
    src/effects/Delay.cpp
    src/ui/ParameterManager.cpp
    src/ui/DisplayManager.cpp
)

# Platform-specific sources
if(PLATFORM_MAC)
    set(PLATFORM_SOURCES
        src/hardware/MacHardware.cpp
        src/ui/MacUIManager.cpp
    )
    
    # macOS frameworks
    find_library(COREAUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIOUNIT_FRAMEWORK AudioUnit)
    find_library(COREMIDI_FRAMEWORK CoreMIDI)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    
    set(PLATFORM_LIBS
        ${COREAUDIO_FRAMEWORK}
        ${AUDIOUNIT_FRAMEWORK}
        ${COREMIDI_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
    )
    
elseif(PLATFORM_STM32)
    set(PLATFORM_SOURCES
        src/hardware/STM32Hardware.cpp
        src/ui/TouchGFXUIManager.cpp
    )
    
    # STM32 specific configuration would go here
    # This would be set up when targeting STM32
    set(PLATFORM_LIBS "")
    
else()
    # Windows or other platforms
    set(PLATFORM_SOURCES
        src/hardware/GenericHardware.cpp
        src/ui/GenericUIManager.cpp
    )
    set(PLATFORM_LIBS "")
endif()

# Create the main library
add_library(EtherCore STATIC ${CORE_SOURCES})
target_include_directories(EtherCore PUBLIC src)

# Platform-specific library
add_library(EtherPlatform STATIC ${PLATFORM_SOURCES})
target_include_directories(EtherPlatform PUBLIC src)
target_link_libraries(EtherPlatform EtherCore ${PLATFORM_LIBS})

# Main executable (only for desktop platforms)
if(PLATFORM_MAC OR PLATFORM_WINDOWS)
    add_executable(EtherSynth src/main.cpp)
    target_link_libraries(EtherSynth EtherPlatform EtherCore ${PLATFORM_LIBS})
    
    # macOS specific settings
    if(PLATFORM_MAC)
        set_target_properties(EtherSynth PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/platforms/mac/Info.plist
        )
    endif()
endif()

# Tests
option(BUILD_TESTS "Build test suite" ON)
if(BUILD_TESTS AND (PLATFORM_MAC OR PLATFORM_WINDOWS))
    enable_testing()
    add_subdirectory(tests)
endif()

# Tools
option(BUILD_TOOLS "Build development tools" ON)
if(BUILD_TOOLS AND (PLATFORM_MAC OR PLATFORM_WINDOWS))
    add_subdirectory(tools)
endif()

# Documentation
option(BUILD_DOCS "Build documentation" OFF)
if(BUILD_DOCS)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        add_subdirectory(docs)
    endif()
endif()

# Installation
if(PLATFORM_MAC)
    install(TARGETS EtherSynth
        BUNDLE DESTINATION .
        RUNTIME DESTINATION bin
    )
elseif(PLATFORM_WINDOWS)
    install(TARGETS EtherSynth
        RUNTIME DESTINATION bin
    )
endif()

# Development helpers
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Add sanitizers for debug builds
    if(PLATFORM_MAC OR PLATFORM_WINDOWS)
        target_compile_options(EtherCore PRIVATE
            -fsanitize=address
            -fsanitize=undefined
        )
        target_link_options(EtherCore PRIVATE
            -fsanitize=address
            -fsanitize=undefined
        )
    endif()
endif()

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
if(PLATFORM_MAC)
    message(STATUS "Target: macOS prototype")
elseif(PLATFORM_STM32)
    message(STATUS "Target: STM32 hardware")
else()
    message(STATUS "Target: Generic platform")
endif()