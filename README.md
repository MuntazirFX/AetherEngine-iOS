# AetherEngine-iOS

A brand-new, clean-room iOS game engine and application built from scratch.
**No XashFusion / xash3d-fwgs code or dependencies.**

## Target Games
1. Half-Life (`valve`)
2. Half-Life: Blue Shift (`bshift`)
3. Half-Life: Opposing Force (`gearbox`)
4. Counter-Strike 1.6 (`cstrike`)
5. Counter-Strike: Condition Zero (`czero`)

> **Note:** User must provide legally obtained game data. No copyrighted assets are bundled.

## Architecture
- `engine/core` – Foundation: types, arena, logging, engine runtime
- `engine/game` – Game registry & lifecycle manager

## Host verification (STEP 2)

Linux/macOS (no Xcode required):

```bash
bash build/scripts/verify_host.sh
```

This compiles all `engine/**/*.c` sources, archives `libaether_engine.a`, and runs `tests/host_smoke.c` (arena, engine lifecycle, 5-game registry, manifests, entity/weapon/monster tables, scoreboard/chat, VGUI runtime).

GitHub Actions workflow `.github/workflows/verify.yml` runs the same script on every push/PR to `main`.

## Build Status
- [x] STEP 1: Core modules
- [x] STEP 2: Verification (host compile + smoke via `build/scripts/verify_host.sh` / `.github/workflows/verify.yml`)
- [x] STEP 2b: Metal/EngineBridge frame path — camera matrices, begin_frame_dt, feature ticks, HUD crosshair spread (see `continue/metal-bridge`)
- [x] STEP 2c: Metal particles — C pool getters + bridge + point-sprite encode from engine state (see `continue/metal-particles`)
- [ ] STEP 3: Engine foundation
- [ ] STEP 4: iOS application
- [ ] STEP 5: 5-game configuration
- [ ] STEP 6: Touch/Input/Settings
- [ ] STEP 7: Renderer/Audio/Filesystem
- [ ] STEP 8: Build system
- [ ] STEP 9: GitHub Actions ARM64
- [ ] STEP 10: Complete verification

## Target
- iOS (iPhone / iPad)
- ARM64
- Unsigned IPA build supported

## Renderer module set

AetherEngine now exposes an Xash3D-class renderer feature layer with clean-room Aether modules:

- `AetherRender_GL` — OpenGL backend interface
- `AetherRender_GLES3` — GLES3 backend interface
- `AetherRender_Soft` — software/reference backend interface
- `AetherRenderFeatures` — feature lifecycle/update coordinator
- `AetherWorld` — world-surface render state
- `AetherLightmap` — lightmap/style state
- `AetherWater` — animated water state
- `AetherSky` — six-face sky state
- `AetherFog` — fog parameters
- `AetherDecal` — decal pool (blood/bullet-style decals)
- `AetherParticle` — particle pool/update + burst spawn + render vertex copy
- `AetherSprite` — sprite state/UVs
- `AetherMDLAnimation` — sequence/bone animation timing state
- `AetherShadow` — shadow configuration
- `AetherPostFX` — bloom/exposure/post-processing configuration

Metal remains the primary iOS backend. The OpenGL/GLES3/software entries are compatibility/reference backend interfaces; they are not a claim that iOS ships an active OpenGL implementation in this project. The feature layer is intentionally independent of any third-party engine source.

## Xash3D-class game logic coverage

AetherEngine includes clean-room game-runtime equivalents for the game-logic areas used by the iOS target:

- `engine/game/dll/` — static server/client module registry (iOS-safe replacement for dynamic DLL loading)
- `engine/game/ai/` — A* navigation graph and bot runtime
- `engine/physics/` — generic rigid-body movement/slide/ground response with a collision callback
- `engine/entity/AetherEntityClassRegistry.*` — built-in entity classname registry (113 GoldSrc-style classnames currently enumerated in this repo)
- `engine/game/weapons/` — weapon definitions, firing/recoil, projectiles, viewmodel animation
- `engine/game/monsters/` — monster definitions, runtime state machine and perception
- `engine/net/` — UDP client/server, snapshots, chat and scoreboard
- `engine/save/` — save/load and autosave formats

These are AetherEngine implementations, not copies of Xash3D source. The feature names indicate the target capability; they do not claim byte-for-byte or 100% gameplay compatibility with every Xash3D/GoldSrc behavior.

## Xash3D-class UI / VGUI coverage

The project now includes a clean-room VGUI compatibility layer under `engine/vgui/`. It provides:

- VGUI2-style retained-mode panel/control API
- VGUI1 compatibility aliases
- `GameMenu.res`-style key/value resource parsing primitives
- scheme/color/font registration primitives
- localization token tables
- server-browser data model and ping sorting
- console overlay line buffer
- classic main/options/load-game/multiplayer panel runtime
- iOS bridge entry points in `EngineBridge.h`

This is an AetherEngine implementation, not a copy of Valve/Xash3D source. Full visual/pixel parity and complete network discovery still require further integration with the Metal renderer and game networking.


## Classic GoldSrc HUD bridge
The iOS game view now presents a clean-room GoldSrc-style HUD overlay backed by the C HUD/player state: health, armor, HEV battery, weapon/ammo state, death state, and classic crosshair presentation. The HUD is a presentation bridge; gameplay state remains in the engine.


## Scoreboard + Chat UI
The iOS game screen now exposes the clean-room C scoreboard/chat runtime through a classic overlay. Scoreboard state is read from the C networking layer; chat uses the same log buffer and is ready for server packet integration.
