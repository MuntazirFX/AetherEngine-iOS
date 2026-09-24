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
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")
endif()

# ============================================================
# ENGINE SOURCES
# ============================================================
set(AETHER_ENGINE_SOURCES
    # -------- core --------
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherCore.c
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherEngine.c
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherMath.c
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherArray.c
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherEvent.c

    # -------- console / command / threading --------
    ${CMAKE_SOURCE_DIR}/../engine/console/AetherCVar.c
    ${CMAKE_SOURCE_DIR}/../engine/console/AetherConsole.c
    ${CMAKE_SOURCE_DIR}/../engine/command/AetherCmd.c
    ${CMAKE_SOURCE_DIR}/../engine/thread/AetherThread.c

    # -------- game --------
    ${CMAKE_SOURCE_DIR}/../engine/game/AetherGameLogic.c
    ${CMAKE_SOURCE_DIR}/../engine/game/AetherGameManager.c
    ${CMAKE_SOURCE_DIR}/../engine/game/AetherManifest.c

    # -------- game/weapons --------
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons/AetherWeapon.c
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons/AetherWeaponDefs.c
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons/AetherWeaponRegistry.c
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons/AetherWeaponFiring.c
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons/AetherWeaponProjectile.c
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons/AetherWeaponView.c

    # -------- game/monsters --------
    ${CMAKE_SOURCE_DIR}/../engine/game/monsters/AetherMonsterBase.c
    ${CMAKE_SOURCE_DIR}/../engine/game/monsters/AetherMonsterDefs.c
    ${CMAKE_SOURCE_DIR}/../engine/game/monsters/AetherMonsterRegistry.c
    ${CMAKE_SOURCE_DIR}/../engine/game/monsters/AetherMonsterAI.c

    # -------- game runtime / DLL-style modules --------
    ${CMAKE_SOURCE_DIR}/../engine/game/dll/AetherGameDLL.c
    ${CMAKE_SOURCE_DIR}/../engine/game/ai/AetherPathfinding.c
    ${CMAKE_SOURCE_DIR}/../engine/game/ai/AetherBot.c

    # -------- physics --------
    ${CMAKE_SOURCE_DIR}/../engine/physics/AetherPhysics.c

    # -------- save --------
    ${CMAKE_SOURCE_DIR}/../engine/save/AetherSaveFormat.c
    ${CMAKE_SOURCE_DIR}/../engine/save/AetherSave.c
    ${CMAKE_SOURCE_DIR}/../engine/save/AetherSavePlayer.c
    ${CMAKE_SOURCE_DIR}/../engine/save/AetherSaveEntity.c
    ${CMAKE_SOURCE_DIR}/../engine/save/AetherSaveWorld.c
    ${CMAKE_SOURCE_DIR}/../engine/save/AetherAutoSave.c

    # -------- net --------
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNetBuffer.c
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNet.c
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNetClient.c
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNetServer.c
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNetScoreboard.c
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNetChat.c
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNetSpectator.c

    # -------- input --------
    ${CMAKE_SOURCE_DIR}/../engine/input/AetherInput.c
    ${CMAKE_SOURCE_DIR}/../engine/input/AetherTouch.c

    # -------- config --------
    ${CMAKE_SOURCE_DIR}/../engine/config/AetherSettings.c

    # -------- fs --------
    ${CMAKE_SOURCE_DIR}/../engine/fs/AetherFS.c
    ${CMAKE_SOURCE_DIR}/../engine/fs/AetherPak.c

    # -------- audio --------
    ${CMAKE_SOURCE_DIR}/../engine/audio/AetherAudio.c
    ${CMAKE_SOURCE_DIR}/../engine/audio/AetherWav.c

    # -------- render / Xash3D-class backends + features --------
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherRender.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherRender_GL.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherRender_GLES3.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherRender_Soft.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherRenderFeatures.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherLightmap.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherWorld.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherWater.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherSky.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherFog.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherDecal.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherDynLight.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherParticle.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherSprite.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherMDLAnimation.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherShadow.c
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherPostFX.c

    # -------- bsp --------
    ${CMAKE_SOURCE_DIR}/../engine/bsp/AetherBSP.c
    ${CMAKE_SOURCE_DIR}/../engine/bsp/AetherBSPGeometry.c
    ${CMAKE_SOURCE_DIR}/../engine/bsp/AetherBSPSynthetic.c
    ${CMAKE_SOURCE_DIR}/../engine/bsp/AetherBSPVis.c
    ${CMAKE_SOURCE_DIR}/../engine/map/AetherMapLoad.c

    # -------- player --------
    ${CMAKE_SOURCE_DIR}/../engine/player/AetherPlayer.c
    ${CMAKE_SOURCE_DIR}/../engine/player/AetherCollision.c
    ${CMAKE_SOURCE_DIR}/../engine/player/AetherPlayerHealth.c
    ${CMAKE_SOURCE_DIR}/../engine/player/AetherPlayerDamage.c
    ${CMAKE_SOURCE_DIR}/../engine/player/AetherPlayerInventory.c

    # -------- texture --------
    ${CMAKE_SOURCE_DIR}/../engine/texture/AetherTexture.c

    # -------- model --------
    ${CMAKE_SOURCE_DIR}/../engine/model/AetherMDL.c
    ${CMAKE_SOURCE_DIR}/../engine/model/AetherMDLGeometry.c

    # -------- entity --------
    ${CMAKE_SOURCE_DIR}/../engine/entity/AetherEntityClassRegistry.c
    ${CMAKE_SOURCE_DIR}/../engine/entity/AetherEntity.c
    ${CMAKE_SOURCE_DIR}/../engine/entity/AetherEntityBase.c
    ${CMAKE_SOURCE_DIR}/../engine/entity/AetherEntitySpawn.c

    # -------- client/hud --------
    ${CMAKE_SOURCE_DIR}/../engine/client/hud/AetherHUD.c
    ${CMAKE_SOURCE_DIR}/../engine/client/hud/AetherHealth.c
    ${CMAKE_SOURCE_DIR}/../engine/client/hud/AetherAmmo.c
    ${CMAKE_SOURCE_DIR}/../engine/client/hud/AetherCrosshair.c

    # -------- VGUI / UI compatibility layer --------
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUI.c
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUIResource.c
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUIScheme.c
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUIFont.c
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUILocalization.c
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUIServerBrowser.c
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUIConsole.c
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUIRuntime.c

    # -------- client/menu --------
    ${CMAKE_SOURCE_DIR}/../engine/client/menu/AetherMenu.c
    ${CMAKE_SOURCE_DIR}/../engine/client/menu/AetherMainMenu.c
    ${CMAKE_SOURCE_DIR}/../engine/client/menu/AetherOptions.c
)

# ============================================================
# ENGINE HEADERS
# ============================================================
set(AETHER_ENGINE_HEADERS
    # -------- core --------
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherCore.h
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherEngine.h
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherMath.h
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherArray.h
    ${CMAKE_SOURCE_DIR}/../engine/core/AetherEvent.h

    # -------- console / command / threading --------
    ${CMAKE_SOURCE_DIR}/../engine/console/AetherCVar.h
    ${CMAKE_SOURCE_DIR}/../engine/console/AetherConsole.h
    ${CMAKE_SOURCE_DIR}/../engine/command/AetherCmd.h
    ${CMAKE_SOURCE_DIR}/../engine/thread/AetherThread.h

    # -------- game --------
    ${CMAKE_SOURCE_DIR}/../engine/game/AetherGameLogic.h
    ${CMAKE_SOURCE_DIR}/../engine/game/AetherGameManager.h
    ${CMAKE_SOURCE_DIR}/../engine/game/AetherManifest.h

    # -------- game/weapons --------
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons/AetherWeapon.h
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons/AetherWeaponDefs.h
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons/AetherWeaponRegistry.h
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons/AetherWeaponFiring.h
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons/AetherWeaponProjectile.h
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons/AetherWeaponView.h
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons/AetherWeapons.h

    # -------- game/monsters --------
    ${CMAKE_SOURCE_DIR}/../engine/game/monsters/AetherMonsterTypes.h
    ${CMAKE_SOURCE_DIR}/../engine/game/monsters/AetherMonsterBase.h
    ${CMAKE_SOURCE_DIR}/../engine/game/monsters/AetherMonsterDefs.h
    ${CMAKE_SOURCE_DIR}/../engine/game/monsters/AetherMonsterRegistry.h
    ${CMAKE_SOURCE_DIR}/../engine/game/monsters/AetherMonsterAI.h
    ${CMAKE_SOURCE_DIR}/../engine/game/monsters/AetherMonster.h

    # -------- game runtime / DLL-style modules --------
    ${CMAKE_SOURCE_DIR}/../engine/game/dll/AetherGameDLL.h
    ${CMAKE_SOURCE_DIR}/../engine/game/ai/AetherPathfinding.h
    ${CMAKE_SOURCE_DIR}/../engine/game/ai/AetherBot.h

    # -------- physics --------
    ${CMAKE_SOURCE_DIR}/../engine/physics/AetherPhysics.h

    # -------- save --------
    ${CMAKE_SOURCE_DIR}/../engine/save/AetherSaveFormat.h
    ${CMAKE_SOURCE_DIR}/../engine/save/AetherSave.h
    ${CMAKE_SOURCE_DIR}/../engine/save/AetherSavePlayer.h
    ${CMAKE_SOURCE_DIR}/../engine/save/AetherSaveEntity.h
    ${CMAKE_SOURCE_DIR}/../engine/save/AetherSaveWorld.h
    ${CMAKE_SOURCE_DIR}/../engine/save/AetherAutoSave.h
    ${CMAKE_SOURCE_DIR}/../engine/save/AetherSaves.h

    # -------- net --------
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNetProtocol.h
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNetBuffer.h
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNet.h
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNetClient.h
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNetServer.h
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNetScoreboard.h
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNetChat.h
    ${CMAKE_SOURCE_DIR}/../engine/net/AetherNets.h

    # -------- input --------
    ${CMAKE_SOURCE_DIR}/../engine/input/AetherInput.h
    ${CMAKE_SOURCE_DIR}/../engine/input/AetherTouch.h

    # -------- config --------
    ${CMAKE_SOURCE_DIR}/../engine/config/AetherSettings.h

    # -------- fs --------
    ${CMAKE_SOURCE_DIR}/../engine/fs/AetherFS.h
    ${CMAKE_SOURCE_DIR}/../engine/fs/AetherPak.h

    # -------- audio --------
    ${CMAKE_SOURCE_DIR}/../engine/audio/AetherAudio.h
    ${CMAKE_SOURCE_DIR}/../engine/audio/AetherWav.h

    # -------- render / Xash3D-class backends + features --------
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherRender.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherRender_GL.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherRender_GLES3.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherRender_Soft.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherRenderFeatures.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherLightmap.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherWorld.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherWater.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherSky.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherFog.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherDecal.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherDynLight.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherParticle.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherSprite.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherMDLAnimation.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherShadow.h
    ${CMAKE_SOURCE_DIR}/../engine/render/AetherPostFX.h

    # -------- bsp --------
    ${CMAKE_SOURCE_DIR}/../engine/bsp/AetherBSP.h
    ${CMAKE_SOURCE_DIR}/../engine/bsp/AetherBSPGeometry.h
    ${CMAKE_SOURCE_DIR}/../engine/bsp/AetherBSPSynthetic.h
    ${CMAKE_SOURCE_DIR}/../engine/bsp/AetherBSPVis.h
    ${CMAKE_SOURCE_DIR}/../engine/map/AetherMapLoad.h

    # -------- player --------
    ${CMAKE_SOURCE_DIR}/../engine/player/AetherPlayer.h
    ${CMAKE_SOURCE_DIR}/../engine/player/AetherCollision.h
    ${CMAKE_SOURCE_DIR}/../engine/player/AetherPlayerHealth.h
    ${CMAKE_SOURCE_DIR}/../engine/player/AetherPlayerDamage.h
    ${CMAKE_SOURCE_DIR}/../engine/player/AetherPlayerInventory.h

    # -------- texture --------
    ${CMAKE_SOURCE_DIR}/../engine/texture/AetherTexture.h

    # -------- model --------
    ${CMAKE_SOURCE_DIR}/../engine/model/AetherMDL.h
    ${CMAKE_SOURCE_DIR}/../engine/model/AetherMDLGeometry.h

    # -------- entity --------
    ${CMAKE_SOURCE_DIR}/../engine/entity/AetherEntityClassRegistry.h
    ${CMAKE_SOURCE_DIR}/../engine/entity/AetherEntity.h
    ${CMAKE_SOURCE_DIR}/../engine/entity/AetherEntityBase.h
    ${CMAKE_SOURCE_DIR}/../engine/entity/AetherEntitySpawn.h
    ${CMAKE_SOURCE_DIR}/../engine/entity/AetherEntityTypes.h

    # -------- client/hud --------
    ${CMAKE_SOURCE_DIR}/../engine/client/hud/AetherHUD.h
    ${CMAKE_SOURCE_DIR}/../engine/client/hud/AetherHealth.h
    ${CMAKE_SOURCE_DIR}/../engine/client/hud/AetherAmmo.h
    ${CMAKE_SOURCE_DIR}/../engine/client/hud/AetherCrosshair.h

    # -------- VGUI / UI compatibility layer --------
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUI.h
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUI1.h
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUI2.h
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUIResource.h
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUIScheme.h
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUIFont.h
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUILocalization.h
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUIServerBrowser.h
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUIConsole.h
    ${CMAKE_SOURCE_DIR}/../engine/vgui/AetherVGUIRuntime.h

    # -------- client/menu --------
    ${CMAKE_SOURCE_DIR}/../engine/client/menu/AetherMenu.h
    ${CMAKE_SOURCE_DIR}/../engine/client/menu/AetherMainMenu.h
    ${CMAKE_SOURCE_DIR}/../engine/client/menu/AetherOptions.h
)

# ============================================================
# LIBRARY TARGET
# ============================================================
add_library(aether_engine STATIC
    ${AETHER_ENGINE_SOURCES}
    ${AETHER_ENGINE_HEADERS}
)

target_include_directories(aether_engine PUBLIC
    ${CMAKE_SOURCE_DIR}/../engine/core
    ${CMAKE_SOURCE_DIR}/../engine/console
    ${CMAKE_SOURCE_DIR}/../engine/command
    ${CMAKE_SOURCE_DIR}/../engine/thread
    ${CMAKE_SOURCE_DIR}/../engine/game
    ${CMAKE_SOURCE_DIR}/../engine/game/weapons
    ${CMAKE_SOURCE_DIR}/../engine/game/monsters
    ${CMAKE_SOURCE_DIR}/../engine/game/dll
    ${CMAKE_SOURCE_DIR}/../engine/game/ai
    ${CMAKE_SOURCE_DIR}/../engine/physics
    ${CMAKE_SOURCE_DIR}/../engine/save
    ${CMAKE_SOURCE_DIR}/../engine/net
    ${CMAKE_SOURCE_DIR}/../engine/input
    ${CMAKE_SOURCE_DIR}/../engine/config
    ${CMAKE_SOURCE_DIR}/../engine/fs
    ${CMAKE_SOURCE_DIR}/../engine/audio
    ${CMAKE_SOURCE_DIR}/../engine/render
    ${CMAKE_SOURCE_DIR}/../engine/bsp
    ${CMAKE_SOURCE_DIR}/../engine/player
    ${CMAKE_SOURCE_DIR}/../engine/texture
    ${CMAKE_SOURCE_DIR}/../engine/model
    ${CMAKE_SOURCE_DIR}/../engine/entity
    ${CMAKE_SOURCE_DIR}/../engine/client/hud
    ${CMAKE_SOURCE_DIR}/../engine/client/menu
    ${CMAKE_SOURCE_DIR}/../engine/vgui
)

# ============================================================
# COMPILER FLAGS
# ============================================================
if(MSVC)
    target_compile_options(aether_engine PRIVATE /W4)
else()
    target_compile_options(aether_engine PRIVATE
        -Wall -Wextra -Werror
        -Wno-unused-parameter
        -fno-common
    )
endif()

# ============================================================
# iOS FRAMEWORKS
# ============================================================
if(AETHER_BUILD_IOS)
    target_link_libraries(aether_engine PUBLIC
        "-framework Foundation"
        "-framework Metal"
        "-framework MetalKit"
        "-framework AVFoundation"
        "-framework CoreGraphics"
    )
endif()

# ============================================================
# HOST SMOKE TEST (Linux/macOS; no iOS SDK required)
# ============================================================
if(AETHER_BUILD_TESTS)
    add_executable(host_smoke ${CMAKE_SOURCE_DIR}/../tests/host_smoke.c)
    target_link_libraries(host_smoke PRIVATE aether_engine m)
    target_include_directories(host_smoke PRIVATE
        ${CMAKE_SOURCE_DIR}/../engine/core
        ${CMAKE_SOURCE_DIR}/../engine/game
        ${CMAKE_SOURCE_DIR}/../engine/game/weapons
        ${CMAKE_SOURCE_DIR}/../engine/game/monsters
        ${CMAKE_SOURCE_DIR}/../engine/entity
        ${CMAKE_SOURCE_DIR}/../engine/player
        ${CMAKE_SOURCE_DIR}/../engine/net
        ${CMAKE_SOURCE_DIR}/../engine/vgui
    )
    enable_testing()
    add_test(NAME host_smoke COMMAND host_smoke
             WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/..)
endif()
