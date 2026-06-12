cmake_minimum_required(VERSION 3.28)
project(smfl-rtos-audio-vis LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(dr_libs
        GIT_REPOSITORY https://github.com/mackron/dr_libs.git
        GIT_TAG master
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(dr_libs)

FetchContent_Declare(miniaudio
        GIT_REPOSITORY https://github.com/mackron/miniaudio.git
        GIT_TAG 0.11.25
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(miniaudio)

add_executable(main src/main.cpp
        src/audio/AudioPlayer.cpp
        src/audio/AudioPlayer.h
        src/audio/miniaudio_impl.cpp
        src/audio/AudioRingBuffer.cpp
        src/audio/AudioRingBuffer.h
        src/analysis/FFT.cpp
        src/analysis/FFT.h
        src/analysis/SpectrumAnalyzer.h)
target_compile_features(main PRIVATE cxx_std_17)
target_include_directories(main PRIVATE ${miniaudio_SOURCE_DIR})
target_link_libraries(main PRIVATE SFML::Graphics)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)
