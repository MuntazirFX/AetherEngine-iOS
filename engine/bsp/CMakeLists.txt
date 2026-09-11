# engine/bsp/CMakeLists.txt
# BSP map format parser module.

set(AETHER_BSP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/AetherBSP.c
)

set(AETHER_BSP_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/AetherBSP.h
)

target_sources(aether_engine PRIVATE
    ${AETHER_BSP_SOURCES}
    ${AETHER_BSP_HEADERS}
)

target_include_directories(aether_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
