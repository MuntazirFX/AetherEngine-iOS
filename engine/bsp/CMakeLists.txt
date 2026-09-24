# engine/bsp/CMakeLists.txt
# BSP map format parser + synthetic demo room.

set(AETHER_BSP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/AetherBSP.c
    ${CMAKE_CURRENT_SOURCE_DIR}/AetherBSPGeometry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/AetherBSPSynthetic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/AetherBSPVis.c
)

set(AETHER_BSP_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/AetherBSP.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AetherBSPGeometry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AetherBSPSynthetic.h
    ${CMAKE_CURRENT_SOURCE_DIR}/AetherBSPVis.h
)

target_sources(aether_engine PRIVATE
    ${AETHER_BSP_SOURCES}
    ${AETHER_BSP_HEADERS}
)

target_include_directories(aether_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
