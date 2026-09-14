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

## Build Status
- [x] STEP 1: Core modules
- [ ] STEP 2: Verification
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
- `AetherParticle` — particle pool/update
- `AetherSprite` — sprite state/UVs
- `AetherMDLAnimation` — sequence/bone animation timing state
- `AetherShadow` — shadow configuration
- `AetherPostFX` — bloom/exposure/post-processing configuration

Metal remains the primary iOS backend. The OpenGL/GLES3/software entries are compatibility/reference backend interfaces; they are not a claim that iOS ships an active OpenGL implementation in this project. The feature layer is intentionally independent of any third-party engine source.
