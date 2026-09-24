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
- `engine/core` – Foundation: types, arena, logging, engine runtime / host frame
- `engine/game` – Game registry, manifests, lifecycle + subsystem tick
- `engine/fs` – Multi-root VFS (`setup_game` mounts valve + mod dir; no assets bundled)
- `engine/audio` / `engine/input` / `engine/config` – Mixer queue, touch/input, settings+cvars
- `ios/AetherApp` – SwiftUI + Metal + EngineBridge

## Host verification (STEP 2)

Linux/macOS (no Xcode required):

```bash
bash build/scripts/verify_host.sh
```

### Synthetic demo world (no game assets)

When Half-Life `.bsp` files are not present, the engine builds a tiny clean-room BSP v30 room
(`aether_bsp_create_synthetic_room` → `aether_mesh_from_bsp` → entity spawn). On iOS use
**Start Demo World (synthetic BSP)** or any Launch path (falls back automatically). Metal draws
the room mesh plus a procedural lightmap stub (grayscale atlas × vertex color), leaf/PVS face culling
(synthetic X=0 split + marksurfaces; `vis_offset=-1` → all empty leaves visible), clipnode hulls for walk/slide/ground/step-up/crouch/swim (distinct crouch Z, jump ceiling clamp, low alcove duck, +Y CONTENTS_WATER pool, waterlevel feet/waist/eye + splash + drown/fall damage), and red monster debug boxes from the entity/monster registries.

This compiles all `engine/**/*.c` sources, archives `libaether_engine.a`, and runs `tests/host_smoke.c` (arena, engine lifecycle, 5-game registry, manifests, entity/weapon/monster tables, scoreboard/chat, VGUI runtime).

GitHub Actions workflow `.github/workflows/verify.yml` runs the same script on every push/PR to `main`.

## STEPs 3–10 batch (`continue/steps-3-10-batch`)

One PR advances the post-verify foundation without shipping Half-Life assets:

| STEP | Status | What landed |
|------|--------|-------------|
| 3 Engine foundation | **done / partial** | `aether_engine_host_frame`, last_dt/fixed_dt, game manager as subsystem tick |
| 4 iOS application | **partial** | Settings sheet, selected-game launch, `engine_host_frame` in Metal draw |
| 5 5-game config | **done / partial** | Manifest `load_all` (5 JSON), select/launch/FS roots; empty→synthetic |
| 6 Touch/Input/Settings | **done / partial** | Holdable touch actions, settings→C settings/cvars/audio, `Documents/aether.cfg` |
| 7 Renderer/Audio/FS | **partial** | `aether_fs_setup_game`, audio ready/flush; Metal path already present |
| 8 Build system | **done / partial** | `cmake_host.sh`, Info.plist path fix, verify path list |
| 9 GA ARM64 | **partial** | workflow uses verify_host; IPA still manual macos dispatch |
| 10 Verification | **done / partial** | smoke covers host_frame/FS/manifests/settings/audio/cvars |

```bash
bash build/scripts/verify_host.sh          # Linux/macOS host (required green)
bash build/scripts/cmake_host.sh           # optional CMake host lib + smoke target
# iOS IPA (macOS + Xcode + XcodeGen):
bash build/scripts/build_ios.sh && bash build/scripts/package_ipa.sh
```


## Map / Audio / CI batch (`continue/batch-map-audio-ci`)

One PR advances map load, entity spawn from file BSPs, audio platform hooks, lightmaps, Metal decals/dynlights, save/net smokes, and CI split — still clean-room (no HL/Valve assets):

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Map load path | **done** | `aether_map_load` FS→synthetic; minimal clean-room BSP fixture writer; bridge `engine_map_load*` |
| 2 | Entity spawn from map | **done** | Lights + worldspawn/info_player_start/monsters from entities lump; `aether_entity_spawn_from_bsp_ex` stats |
| 3 | Audio platform callbacks | **done / partial** | Buffer submit + beep/tone; iOS `submitPCM16`; platform voice callback already present |
| 4 | WAV header stub | **done** | `aether_wav_parse_header` / tone writer scaffold (body decode not required) |
| 5 | Lightmap improve | **done / partial** | `aether_lightmap_bake_from_bsp` uses LIGHTING lump when present; else procedural stub path |
| 6 | Decals / dyn lights → Metal | **done / partial** | Decal `copy_render` + `AetherDynLight` slice; MetalCallbacks tracks DRAW_DECALS; full GPU encode still thin |
| 7 | Save/load roundtrip smoke | **done** | Host save→load player/world stub via `aether_save_write/read` |
| 8 | Net listen/connect smoke | **done / partial** | Localhost UDP handshake stub in host smoke (server listen + client connect) |
| 9 | CI split | **done** | `verify.yml` job `verify_host` always; `build-arm64.yml` IPA on `workflow_dispatch` only |
| 10 | README honesty | **done** | This table + known gaps |

### Known gaps after this batch
- Real GoldSrc `.bsp` lighting/VIS/clip still need user-provided maps under Documents
- WAV body decode / streaming not implemented (header + procedural beep only)
- Decal/dynlight Metal encode is a command/counter slice — not full projected decals
- Net is handshake smoke only (no gameplay snapshots yet)
- IPA still requires macOS + Xcode via workflow_dispatch

## UV / WAV / Decals / Net batch (`continue/batch-uv-wav-decals-net`)

One PR advances lightmap UV unpack, VIS PVS stubs, WAV stream playback, projected decals, snapshot HUD, dyn-light tint, sprite/MDL Metal stubs, hazard ticks, and frustum cull — still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Lightmap UV unpack | **done** | `aether_lightmap_unpack_uvs_from_bsp` uses face texinfo vecs + face_ranges; synthetic has LIGHTING+texinfo |
| 2 | VIS face bake / visbits | **done** | Synthetic multi-leaf PVS RLE stub (asymmetric); decompress path exercised |
| 3 | WAV stream play | **done** | `aether_wav_extract_pcm16` + `aether_audio_play_wav_data` → buffer callback |
| 4 | Projected decal quads | **done / partial** | `aether_decals_copy_quads` (6 verts/decal) + Metal pipeline draw |
| 5 | Snapshot → scoreboard/chat | **done** | `AetherNetSnapshot` encode/decode/apply_hud; bridge demo tick |
| 6 | Dyn lights vertex/lightmap tint | **done / partial** | sample_rgb + mesh tint buffer + lightmap modulate stub |
| 7 | MDL / sprite Metal path | **done / partial** | Sprite billboard quads → Metal; MDL path already present (needs user `.mdl`) |
| 8 | Fire / radiation ticks | **done** | Player flags + LAVA/SLIME refresh; bridge ticks like drown |
| 9 | Frustum AABB cull | **done** | `AetherFrustum` + `aether_bsp_vis_apply_frustum` on leaf AABBs |
| 10 | README + gaps | **done** | This table |

### Known gaps after UV/WAV/decals/net batch
- Real maps still required for full GoldSrc lightmap extents / style animations
- Decal projection is normal-aligned billboard quads (not clipped to world triangles)
- Dyn-light Metal fragment uniform array not yet wired (C tint + lightmap modulate only)
- Snapshot HUD is local apply / demo tick — not yet driven by live UDP gameplay
- Sprite/MDL draw needs user assets for textured models; procedural sprite stub only
- IPA still requires macOS + Xcode via workflow_dispatch




## GPU lights / Decal clip / Netplay batch (`continue/batch-gpu-lights-decal-clip-netplay`)

One PR advances Metal dyn-light UBO, world-clipped decals, live UDP snapshot HUD, lightstyles,
clean-room MDL/SPR fixtures, blob shadows, PostFX brightness/gamma, and use/interact traces — still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Metal dyn-light UBO | **done / partial** | `aether_dyn_lights_fill_ubo` + Metal `aether_fragment_dynlights_world` buffer(2) |
| 2 | Decal triangle clip | **done / partial** | `aether_decals_project_onto_mesh` (normal-aligned face tris; UV clip stub) |
| 3 | Client snapshot UDP ingest | **done** | NetClient stores SERVER_SNAPSHOT; live tick → scoreboard/chat; host smoke |
| 4 | Style-animated lightmaps | **done** | GoldSrc-style `aether_lightstyles_*` + `aether_lightmap_apply_style` |
| 5 | Clean-room MDL/SPR fixture | **done** | `aether_mdl_write_fixture` / `aether_sprite_write_fixture` (no HL IP) |
| 6 | Sprite/MDL fixture path | **done / partial** | Host loads fixture; sprite quad draw; MDL header/bones; Metal path via bridge |
| 7 | Soft/blob shadow stub | **done** | `aether_shadow_copy_blob` + Metal blob pipeline under player |
| 8 | PostFX brightness/gamma | **done / partial** | settings/cvars `r_brightness`/`r_gamma` → PostFX; Metal pass hook (offscreen TBD) |
| 9 | Use/interact trace | **done** | `aether_interact_trace` ray/AABB + USE action wires eye trace |
| 10 | README + gaps | **done** | This table |

### Known gaps after GPU lights / decal clip / netplay batch
- Dyn-light UBO uses world-pos vertex path; atlas-only maps still need full light entity binding polish
- *(addressed in postfx/mdl/predict batch: SH decal clip, offscreen PostFX, ping-pong lightmaps, fixture tris, delta/predict)*
- IPA still requires macOS + Xcode via workflow_dispatch




## PostFX / Lightmap ping-pong / MDL / Predict batch (`continue/batch-postfx-lightmap-mdl-predict`)

One PR advances offscreen PostFX, dual lightmap styles, fixture studio mesh, client
interp/delta/prediction stubs, dyn-light array uniforms, and world-clipped decals — still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Offscreen PostFX | **done** | Color+depth offscreen target; Metal PostFX pass samples scene with brightness/gamma uniforms |
| 2 | Dual lightmap styles | **done** | `base_rgba` + `apply_style_pingpong` (copy base→rgba then modulate; no accum) |
| 3 | MDL triangle fixture | **done** | Fixture embeds 1 studio mesh / 3 verts; geometry extract → Metal draw |
| 4 | Client interp | **done** | `AetherNetInterp` prev/curr lerp of player origins |
| 5 | Delta snapshots | **done** | `AetherNetDelta` encode/apply changed players only (`AETHER_MSG_SERVER_DELTA`) |
| 6 | Prediction stub | **done** | `AetherNetPredict` local move + soft reconcile on snapshot |
| 7 | Dyn-light array uniforms | **done / partial** | `fill_array` + fragment N·L attenuation polish |
| 8 | World-clipped decals | **done** | Sutherland–Hodgman clip to decal square in tangent space |
| 9 | Host smoke | **done** | postfx uniforms, MDL verts>0, delta+predict, pingpong, clip |
| 10 | README + gaps | **done** | This table |

### Known gaps after postfx/lightmap/mdl/predict batch
- PostFX is brightness/gamma only (no bloom/DOF chain yet)
- *(addressed in gpu-lightstyles/skin/mp batch: GPU style weights, skinning stub, live cmd+delta)*
- Decal clip is tangent-square SH (not full mesh silhouette / CSG)
- IPA still requires macOS + Xcode via workflow_dispatch



## GPU lightstyles / Skin / MP cmds batch (`continue/batch-gpu-lightstyles-skin-mp`)

One PR advances GPU lightstyle weights, MDL bone/skinning + textured fixture, live UDP
cmd→authority→snapshot with delta/predict/interp, bloom PostFX chain, and decal atlas — still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | GPU lightstyle weights | **done** | `aether_lightstyles_fill_gpu_weights` + Metal helper; base LM stays on GPU, CPU rewrite optional |
| 2 | MDL bone/skinning + textured fixture | **done** | `aether_mdl_skin_*` stub matrices + dual-bone textured clean-room fixture |
| 3 | Delta/predict/interp live tick | **done** | `aether_net_client_live_tick` pushes snaps→interp, predict+reconcile |
| 4 | Input cmd stream | **done** | `AetherNetCmd` encode/decode + `send_input` (move/look/buttons) |
| 5 | Bloom PostFX chain | **done** | bright/blur/combine uniforms + Metal fragments on offscreen path |
| 6 | Decal atlas Metal sample | **done** | procedural RGBA atlas + `aether_decal_atlas_fragment` |
| 7 | Server authority + snapshot loop | **done** | apply cmds, `tick_authority` broadcast at snapshot rate |
| 8 | Lag compensation stub | **done** | per-slot cmd history + `lagcomp_cmd` lookup |
| 9 | Host smoke | **done** | skinning matrices, bloom uniforms, UDP cmd+delta roundtrip |
| 10 | README + gaps | **done** | This table |

### Known gaps after GPU lightstyles / skin / mp batch
- *(addressed in seq/styles/spatial/HUD batch: per-face styles, sequence skinning, bloom encode)*
- Lag-comp is cmd-history lookup only (no rewind world / hit validation)
- IPA still requires macOS + Xcode via workflow_dispatch




## Seq / Per-face styles / Spatial / HUD / Predict+clip batch (`continue/batch-seq-pvs-audio-ui`)

One PR advances real MDL sequence skinning, per-face lightstyle indices, spatial audio,
HUD layout polish, clipnode prediction, bloom iOS encode, viewmodel stub, and monster AI
frame hook — still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | MDL sequence skinning | **done** | `aether_mdl_sequence_*` + `skin_build_from_sequence` → bone mats → `skin_mesh`; seq fixture |
| 2 | Per-face lightstyle indices | **done** | mesh `face_ranges.styles[]` from BSP; synthetic 0/2 alternate; GPU face weights |
| 3 | Spatial audio stub | **done** | listener + distance/pan atten; `play_beep_at` / `play_wav_at` |
| 4 | Live HUD polish | **done** | `AetherHUDLayout` classic rects; Swift chrome consistency for health/armor/air/ammo |
| 5 | Predict + collision | **done** | `aether_net_predict_apply_cmd_clipped` uses clipnode `collision_move` |
| 6 | Bloom iOS encode | **done** | Metal bright→blur→combine encode path when bloom enabled |
| 7 | Weapon viewmodel stub | **done** | `aether_weapon_view_copy_stub` 6-vert gun quad |
| 8 | Monster AI frame tick | **done** | `aether_monster_ai_tick_registry` hooked in registry_tick |
| 9 | Host smokes | **done** | seq skin, face styles, spatial, predict+clip (+ bloom/viewmodel/AI) |
| 10 | README + gaps | **done** | This table |

### Known gaps after seq / styles / spatial / HUD batch
- Sequence skinning uses clean-room sway keys (not full Studio sequence/anim blocks from `.mdl`)
- Per-face style weights are primary style only (styles[1..3] blend TBD on GPU)
- Spatial pan is computed but mono beep path applies gain only (stereo mix on iOS TBD)
- Bloom encode needs device/Metal; host validates uniforms + encode-needed flag
- Viewmodel is a colored stub quad (no real v_*.mdl)
- IPA still requires macOS + Xcode via workflow_dispatch


## Build Status
- [x] STEP 1: Core modules
- [x] STEP 2: Verification (host compile + smoke via `build/scripts/verify_host.sh` / `.github/workflows/verify.yml`)
- [x] STEP 2b: Metal/EngineBridge frame path — camera matrices, begin_frame_dt, feature ticks, HUD crosshair spread (see `continue/metal-bridge`)
- [x] STEP 2c: Metal particles — C pool getters + bridge + point-sprite encode from engine state (see `continue/metal-particles`)
- [x] STEP 2d: Metal sky — AetherSky face colors → gradient dome + bridge + Metal placeholder (see `continue/metal-sky`)
- [x] STEP 2e: Metal water — AetherWater wavy plane + bridge + Metal translucent pass (see `continue/metal-water`)
- [x] STEP 2f: Metal fog — AetherFog params + fullscreen tint copy_render + bridge + Metal pass (see `continue/metal-fog`)
- [x] STEP 2g: BSP/world → Metal — synthetic demo room BSP → mesh + entities + DRAW_WORLD (see `continue/bsp-metal`)
- [x] STEP 2h: BSP lightmap stub — procedural grayscale atlas + mesh LUV + Metal sample (see `continue/bsp-lightmap`)
- [x] STEP 2i: BSP VIS / leaf culling — find leaf, marksurfaces, culled index list → Metal (see `continue/bsp-vis`)
- [x] STEP 2j: BSP clipnodes / collision — synthetic hulls + move-and-slide → player on floor (see `continue/bsp-collision`)
- [x] STEP 2k: BSP step-up / stairs — Quake-style auto-step ≤18u + synthetic 16u ledge (see `continue/bsp-stepup`)
- [x] STEP 2l: Crouch hull + jump ceiling — distinct hull-2 Z AABB, alcove duck, ceiling clamp (see `continue/bsp-crouch-jump`)
- [x] STEP 2m: Water contents / swim — CONTENTS_WATER clip + buoyancy/swim move (see `continue/bsp-water-swim`)
- [x] STEP 2n: Waterlevel + splash / enter-exit FX — feet/waist/eye tiers, particle splash, air/drown stub (see `continue/bsp-waterlevel-splash`)
- [x] STEP 2o: Drown → health damage + HEV air HUD — tick_drown wire, air meter bridge/ClassicHUD (see `continue/bsp-drown-health`)
- [x] STEP 2p: Fall damage on land + HUD punch/flash — calc_fall_damage wire, water soft, view punch (see `continue/bsp-fall-damage`)
- [x] STEP 3: Engine foundation — host_frame/timebase, subsystem registry, game manager tick (partial: no full mod DLL tick yet)
- [x] STEP 4: iOS application — App/settings/game-select/host_frame wired into Metal loop (partial: playable demo loop exists; full campaign load still needs user assets)
- [x] STEP 5: 5-game configuration — manifests load_all + Dashboard selection → launch/FS roots end-to-end (partial: empty dirs fall back to synthetic BSP)
- [x] STEP 6: Touch/Input/Settings — touch hold buttons, settings→cvars/audio/look, aether.cfg persistence (partial: no controller profile UI yet)
- [x] STEP 7: Renderer/Audio/Filesystem — FS map load + WAV stream/beep + lightmap UV unpack + projected decals (partial: decal not world-clipped; dynlight GPU thin)
- [x] STEP 8: Build system — CMake host script + XcodeGen Info.plist path + verify_host path checks (partial: iOS IPA still requires macOS/Xcode)
- [x] STEP 9: GitHub Actions ARM64 — `verify.yml` always `verify_host`; `build-arm64.yml` splits verify_host (PR) vs IPA (`workflow_dispatch` only)
- [x] STEP 10: Complete verification — expanded `host_smoke` + verify_host API greps; README marks honest

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
- `AetherWorld` — world-surface render state (triangle count from active BSP mesh)
- `AetherBSPSynthetic` — clean-room BSP v30 demo room (no copyrighted maps) for host/iOS verify
- `AetherLightmap` — lightmap/style state + procedural grayscale atlas stub + mesh LUV bake
- `AetherBSPVis` — leaf-from-point, PVS/marksurface face filter, culled mesh indices for Metal
- `AetherCollision` — clipnode hull contents/solid + move-and-slide + step-up + crouch/stand hull heights + water (synthetic room, 16u ledge, low alcove, +Y pool)
- `AetherPlayer` — waterlevel tiers (dry/wade/swim/under), splash events, air/drown → `aether_player_tick_drown`, fall impact → `aether_player_calc_fall_damage`
- `AetherWater` — animated water state + wavy plane vertex copy for Metal
- `AetherSky` — six-face sky state + gradient dome vertex copy for Metal
- `AetherFog` — fog parameters + fullscreen tint vertex copy for Metal
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
The iOS game view now presents a clean-room GoldSrc-style HUD overlay backed by the C HUD/player state: health, armor, HEV battery, air/drown meter, weapon/ammo state, death state, and classic crosshair presentation. The HUD is a presentation bridge; gameplay state remains in the engine.


## Scoreboard + Chat UI
The iOS game screen now exposes the clean-room C scoreboard/chat runtime through a classic overlay. Scoreboard state is read from the C networking layer; chat uses the same log buffer and is ready for server packet integration.
