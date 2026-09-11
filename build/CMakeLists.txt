# CMakeLists.txt
# Top-level build for AetherEngine C core.
# AetherEngine-iOS · Clean-room.

cmake_minimum_required(VERSION 3.20)
project(AetherEngine C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(AETHER_BUILD_TESTS "Build AetherEngine test targets" OFF)
option(AETHER_BUILD_IOS   "Build for iOS (ARM64)"          OFF)

if(AETHER_BUILD_IOS)
    set(CMAKE_SYSTEM_NAME iOS)
    set(CMAKE_OSX_ARCHITECTURES arm64)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0")
endif()

set(AETHER_ENGINE_SOURCES
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherCore.c
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherEngine.c
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherMath.c
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherArray.c
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherEvent.c
    ${CMAKE_SOURCE_DIR}/../engine/game/AetherGameManager.c
    ${CMAKE_SOURCE_DIR}/../engine/game/AetherManifest.c
    ${CMAKE_SOURCE_DIR}/../engine/input/AetherInput.c
    ${CMAKE_SOURCE_DIR}/../engine/input/AetherTouch.c
    ${CMAKE_SOURCE_DIR}/../engine/config/AetherSettings.c
    ${CMAKE_SOURCE_DIR}/../engine/fs/AetherFS.c
    ${CMAKE_SOURCE_DIR}/../engine/fs/AetherPak.c
    ${CMAKE_SOURCE_DIR}/../engine/audio/AetherAudio.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherRender.c
    ${CMAKE_SOURCE_DIR}/../engine/bsp/AetherBSP.c
)

set(AETHER_ENGINE_HEADERS
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherCore.h
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherEngine.h
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherMath.h
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherArray.h
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherEvent.h
    ${CMAKE_SOURCE_DIR}/../engine/game/AetherGameManager.h
    ${CMAKE_SOURCE_DIR}/../engine/game/AetherManifest.h
    ${CMAKE_SOURCE_DIR}/../engine/input/AetherInput.h
    ${CMAKE_SOURCE_DIR}/../engine/input/AetherTouch.h
    ${CMAKE_SOURCE_DIR}/../engine/config/AetherSettings.h
    ${CMAKE_SOURCE_DIR}/../engine/fs/AetherFS.h
    ${CMAKE_SOURCE_DIR}/../engine/fs/AetherPak.h
    ${CMAKE_SOURCE_DIR}/../engine/audio/AetherAudio.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherRender.h
    ${CMAKE_SOURCE_DIR}/../engine/bsp/AetherBSP.h
)

add_library(aether_engine STATIC
    ${AETHER_ENGINE_SOURCES}
    ${AETHER_ENGINE_HEADERS}
)

target_include_directories(aether_engine PUBLIC
    ${CMAKE_SOURCE_DIR}/../engine/core
    ${CMAKE_SOURCE_DIR}/../engine/game
    ${CMAKE_SOURCE_DIR}/../engine/input
    ${CMAKE_SOURCE_DIR}/../engine/config
    ${CMAKE_SOURCE_DIR}/../engine/fs
    ${CMAKE_SOURCE_DIR}/../engine/audio
    ${CMAKE_SOURCE_DIR}/../engine/render
    ${CMAKE_SOURCE_DIR}/../engine/bsp
)

if(MSVC)
    target_compile_options(aether_engine PRIVATE /W4)
else()
    target_compile_options(aether_engine PRIVATE
        -Wall -Wextra -Werror
        -Wno-unused-parameter
        -fno-common
    )
endif()

if(AETHER_BUILD_IOS)
    target_link_libraries(aether_engine PUBLIC
        "-framework Foundation"
        "-framework Metal"
        "-framework MetalKit"
        "-framework AVFoundation"
        "-framework CoreGraphics"
    )
endif()
