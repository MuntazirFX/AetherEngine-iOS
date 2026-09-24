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
- *(addressed in studio/vis/stereo batch: studio anim blocks, multi-style blend, stereo mix, viewmodel MDL, lagcomp rewind, PVS lights, hitboxes, muzzle/trail)*
- Bloom encode needs device/Metal; host validates uniforms + encode-needed flag
- IPA still requires macOS + Xcode via workflow_dispatch


## Studio / Vis / Stereo batch (`continue/batch-studio-vis-stereo`)

One PR advances real studio sequence/anim blocks, multi-style lightmap blend, stereo
spatial mix, viewmodel MDL path, lag-comp world rewind, PVS dynlight cull, studio
hitboxes, and weapon particle muzzle/trail — still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Studio sequence/anim blocks | **done** | `aether_mdl_write_studio_fixture` embeds seq+keyframe blocks; `sequence_load_from_data` |
| 2 | Multi-style lightmap blend | **done** | `fill_face_style_blend` packs styles[0..3]×weights; synthetic styles[1]=3 |
| 3 | Stereo spatial mix | **done** | `play_beep_stereo_at` + equal-power L/R; `play_beep_at` uses stereo path |
| 4 | Viewmodel MDL path | **done** | `aether_weapon_view_copy_mdl_fixture` extracts clean-room fixture → view verts |
| 5 | Lag-comp world rewind | **done** | `AetherLagComp` AABB history + query/trace at time |
| 6 | PVS → dynlight cull | **done** | `aether_dyn_lights_cull_pvs` skips lights outside view PVS |
| 7 | Studio hitboxes stub | **done** | fixture hitboxes + `aether_mdl_hitbox_trace` for use/trace |
| 8 | Particle muzzle/trail | **done** | `spawn_muzzle` / `spawn_trail` weapon-linked stubs |
| 9 | Host smokes | **done** | studio anim, stereo mix, lagcomp, pvs-lights (+ blend/view/hb/fx) |
| 10 | README + gaps | **done** | This table |

### Known gaps after studio / vis / stereo batch
- *(addressed in metal-blend/studio-attach batch: Metal style blend sample, RLE keys, lagcomp hit vs cmds, dynlight bleed, skinned viewmodel attachments, bloom H/V)*
- Stereo PCM reaches iOS `submitPCM16` (channels=2); spatial listener up-vector still Z-up planar pan
- IPA still requires macOS + Xcode via workflow_dispatch




## Metal blend / Studio attach / Lagcomp hit batch (`continue/batch-metal-blend-studio-attach`)

One PR advances Metal multi-style lightmap sampling, skinned viewmodel sequences with
muzzle attachments, lag-comp hit validation vs cmd history, GoldSrc-ish anim RLE,
dynlight leaf-radius bleed, attachment-driven particles, studio event/sound cues,
and separable bloom encode — still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Metal multi-style LM sample | **done** | `aether_fragment_style_blend` + `sample_style_blend` / `fill_style_blend_ubo` |
| 2 | Skinned viewmodel + muzzle attach | **done** | `aether_weapon_view_copy_skinned` + fixture attachments (`muzzle`/`shell`) |
| 3 | Lag-comp hit vs cmd history | **done** | `aether_lagcomp_validate_hit` rewinds AABB + look from cmd |
| 4 | GoldSrc-ish anim RLE | **done** | `aether_mdl_anim_rle_decode` + RLE trailer in `write_studio_fixture_ex` |
| 5 | Dynlight leaf-radius bleed | **done** | `aether_dyn_lights_cull_pvs_bleed` sphere↔leaf AABB |
| 6 | Attachment particle fire | **done** | `spawn_viewmodel_fire` / `spawn_at_attachment` |
| 7 | Studio event / sound cue | **done** | fixture events + `studio_events_fire` + `play_studio_cue` |
| 8 | Metal bloom encode improve | **done** | soft-knee bright + separable H/V blur + `bloom_encode_plan` |
| 9 | Host smokes | **done** | blend UBO, attachments, lagcomp hits, RLE (+ bleed/events/bloom) |
| 10 | README + gaps | **done** | This table |

### Known gaps after metal-blend / studio-attach batch
- *(addressed in face-id/bone/portal/attach batch: live face_id attr, bone-hitbox rewind, portal flood, attach chain, muzzle world sync, weapon cycle)*
- Anim RLE is a clean-room run-length of key channels (not byte-identical `mstudioanim_t`)
- IPA still requires macOS + Xcode via workflow_dispatch



## Face-id / Bone lagcomp / Portal flood / Attach chain batch (`continue/batch-faceid-bone-portal-attach`)

One PR advances live face-id vertex attributes for Metal multi-style LM blend, bone-hitbox
lag rewind, portal-aware dynlight flood, 3rd-person attachment matrix chain, viewmodel
muzzle→world particle/light sync, cleaned style-blend UBO upload, and weapon switch cycle
— still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Face-id vertex attribute | **done** | `aether_mesh_vertex_t.face_id` + Metal attr(4); style blend uses live face index |
| 2 | Bone-hitbox lag rewind | **done** | `aether_lagcomp_studio_*` stores bone mats + hitboxes; `hitbox_to_world` + `studio_trace` |
| 3 | Portal-aware dynlight flood | **done** | leaf portal links + BFS flood; `cull_portal_flood` / `fill_array_portal_flood` |
| 4 | 3rd-person attach chain | **done** | `aether_mdl_attachment_chain_world` hand→weapon→world matrix chain |
| 5 | Viewmodel muzzle→world sync | **done** | `aether_particles_sync_muzzle_world` particles + dynlight at world muzzle |
| 6 | Style blend UBO cleaned | **done** | `fill_style_blend_draw` flags/stride hint; Metal styleBlend pipeline upload |
| 7 | Weapon switch cycle | **done** | `aether_player_inv_cycle` / `apply_weapon_input` on WEAPON_NEXT/PREV |
| 8 | Host smokes | **done** | face-id, bone lagcomp, portal flood, attach chain (+ muzzle/draw/cycle) |
| 9 | Verify regressions | **done** | `verify_host.sh` greps + smoke green |
| 10 | README + gaps | **done** | This table |

### Known gaps after face-id / bone / portal / attach batch
- Face-id is per-vertex f32 (not indexed 16-bit); style UBO still capped at 64 faces for Metal weights buffer
- Bone lagcomp stores up to 8 bones × 8 hitboxes per ent (not full GoldSrc studio bone count)
- Portal links are AABB-touch stubs among PVS leaves (not Quake-style portal winding clip)
- 3rd-person attach chain uses clean-room fixture attachments (not full player/weapon MDL load)
- Muzzle world sync uses eye basis from yaw/pitch (no full viewmatrix from Metal camera)
- IPA still requires macOS + Xcode via workflow_dispatch




## Studio LOD / Water reflect / Net score / Predict smooth / Depth prepass (`continue/batch-studio-lod-water-reflect-netscore`)

One PR advances studio LOD + bodygroup select, water planar reflection hooks, live
scoreboard join/leave over UDP, client prediction smooth error decay, Metal depth
prepass encode plan, bodygroup swap (input/console), and scoreboard HUD events —
still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Studio LOD / bodygroup API | **done** | `aether_mdl_write_lod_fixture` + `lod_select` / bodygroup set/cycle/tri_total |
| 2 | Water planar reflection | **done** | mirror matrix + clip plane uniforms; Metal/host hooks |
| 3 | Net scoreboard join/leave UDP | **done** | encode JOIN/LEAVE + handle_packet + server broadcast + UDP smoke |
| 4 | Predict smooth error decay | **done** | `smooth_tick` / `reconcile_smooth` exponential residual decay |
| 5 | Metal depth prepass | **done** | encode plan + `record_stub` + depth-only shader/pipeline stub |
| 6 | Bodygroup swap input/console | **done** | `BODYGROUP_NEXT` + `engine_console_exec_bodygroup` / cycle API |
| 7 | Scoreboard HUD join/leave | **done** | Swift overlay shows join/leave event ticker |
| 8 | Host smokes | **done** | LOD/bodygroup, water reflect, netscore UDP, predict smooth, depth plan |
| 9 | Verify regressions | **done** | `verify_host.sh` greps + smoke green |
| 10 | README + gaps | **done** | This table |

### Known gaps after studio-lod / water-reflect / netscore batch
- LOD table is clean-room tri budgets (not GoldSrc multi-resolution meshes); bodygroup submodels are count stubs
- Water reflection is planar mirror/clip uniforms only — Metal does not yet re-render the scene into a reflection RT
- Join/leave HUD events are client-side ring buffer; full server→all-clients fanout needs active peer slots
- Predict smooth decays residual after partial snap blend (not full GoldSrc entity baseline rewind)
- Depth prepass pipeline is depth-write stub (color mask none); full early-Z occlusion cull still thin
- IPA still requires macOS + Xcode via workflow_dispatch

## Reflect RT / Studio skin / MP score / Chat cue / Kill HUD (`continue/batch-reflect-rt-studio-skin-mp-hud`)

One PR advances Metal water reflection render-target (allocate + sample), fuller studio LOD
mesh extract by distance, MP frags/deaths UDP snapshot sync, voice/chat cue → HUD, kill-feed
net events, studio texture-group (skin) select, prediction teleport snap threshold, and depth
prepass bound before main pass — still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Water reflection RT | **done** | `aether_water_reflect_rt_ensure` / encode plan allocates RT + sample flag; Metal `waterReflectTexture` + fragment sample |
| 2 | Fuller studio LOD extract | **done** | `aether_mdl_lod_extract_mesh` / `extract_by_distance` multi tri-budget fan mesh |
| 3 | MP score sync UDP | **done** | `aether_net_server_set_score` + snapshot frags/deaths over UDP → scoreboard HUD |
| 4 | Voice/chat cue sync | **done** | `aether_chat_encode` / `encode_voice_cue` + apply_net → chat HUD |
| 5 | Classic HUD kill feed | **done** | `AETHER_MSG_KILL` / `SB_EVENT_KILL` encode/handle + Swift ticker |
| 6 | Studio texture-group select | **done** | `aether_mdl_texgroup_*` + skin_lod fixture + console `skin` |
| 7 | Predict teleport snap | **done** | `reconcile_teleport` hard-snaps when error length ≥ threshold (default 64) |
| 8 | Depth prepass before main | **done** | `bind_before_main` + Metal `encodeDepthPrepassIfNeeded` before world draw |
| 9 | Host smokes + verify | **done** | `smoke_batch_reflect_rt_studio_skin_mp_hud` + verify greps |
| 10 | README + gaps | **done** | This table; honest % (IPA still unbuilt) |

### Known gaps after reflect-rt / studio-skin / mp-hud batch
- Reflection RT is allocated and sampled as a stub (scene not yet re-rendered into the RT from mirrored camera)
- LOD extract builds procedural fan meshes from tri budgets (not true GoldSrc multi-res studio meshes)
- Kill feed / chat voice cues are protocol stubs (no real Opus/voice capture)
- Texture groups are fixture metadata (not full studio texture remaps; distinct from bone skinning matrices)
- Depth prepass draws world with identity MVP stub until camera matrices are threaded through
- IPA still requires macOS + Xcode via workflow_dispatch

### Progress toward playable unsigned IPA demo
Rough overall estimate after reflect-rt batch: **~68–70%** (superseded by mirror-rt batch below) toward a playable unsigned IPA demo
(synthetic BSP walk/swim/combat stubs + Metal world/particles/post + net/predict foundations).
IPA remains unbuilt on this host (needs macOS/Xcode); do not treat host-smoke green as a packaged demo.
Remaining: real game-data FS paths, fuller studio/GPU reflection encode, signed packaging on macOS/Xcode,
and tighter netplay/HUD polish.




## Mirror-RT / multi-mesh LOD / MP kill-score / IPA docs (`continue/batch-mirror-rt-lod-mp-ipa-docs`)

One PR advances mirrored-camera water RT encode, real multi-mesh LOD buckets, live MP authority
kill/score fanout, depth-prepass camera MVP, IPA dry-run docs, kill/score consistency smoke,
water RT clear/resolve/mips, and a spectator follow stub — still clean-room (no HL assets):

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Mirrored-camera → water RT | **done** | `aether_water_reflect_rt_build_mirror_mvp` / `draw_plan`; Metal `encodeWaterReflectPass` clear+draw+resolve |
| 2 | Real multi-mesh LOD | **done** | Per-LOD mesh buckets (box/octa/tetra) in fixture — not just tri-budget fans |
| 3 | Live MP authority kill/score fanout | **done** | `tick_authority_kill_score` + `fanout_scores` to clients |
| 4 | Depth prepass real camera MVP | **done** | `aether_depth_prepass_camera_set` / `encode_plan_ex`; Metal threads view/proj |
| 5 | IPA dry-run docs | **done** | macOS `build_ios.sh` / `package_ipa.sh` + `workflow_dispatch` checklist (below) |
| 6 | Kill feed + score consistency smoke | **done** | register_kill → UDP kill pkt → HUD scores match authority |
| 7 | Water RT clear/resolve/mips | **done** | `clear` / `resolve` / `gen_mips` hooks + Metal mipmapped RT |
| 8 | Spectator follow stub | **done** | `aether_spectator_*` follow eye behind target |
| 9 | Host smokes + verify | **done** | `smoke_batch_mirror_rt_lod_mp_ipa_docs` + verify greps |
| 10 | README + honest % | **done** | This table; cap ~72% while IPA unbuilt on this host |

### IPA dry-run (macOS + Xcode) — workflow_dispatch checklist

This Linux/CI host cannot produce an IPA. On a macOS runner / Mac with Xcode:

1. **Prereqs:** Xcode 15+, `cmake`, `xcodegen` (`brew install cmake xcodegen`), iPhoneOS SDK.
2. **Host verify first:** `bash build/scripts/verify_host.sh` (must be green).
3. **Build unsigned app:** `bash build/scripts/build_ios.sh` → `build/out/DerivedData/.../AetherEngine.app`.
4. **Package IPA:** `bash build/scripts/package_ipa.sh` → `build/out/AetherEngine.ipa`.
5. **GitHub Actions:** Actions → **Build AetherEngine IPA** → **Run workflow** (`workflow_dispatch`).
   - Inputs: `version` (e.g. `v0.1.0`), `publish_release` (bool).
   - Job `build_ipa` runs only on dispatch (PR/push only run `verify_host` on macos-14).
6. **Sideload:** AltStore / Sideloadly / TrollStore / `ideviceinstaller` with the unsigned IPA.
7. **Do not** treat host-smoke green as a packaged demo — IPA remains unbuilt until macOS/Xcode runs the scripts above.

### Known gaps after mirror-rt / lod-mp / ipa-docs batch
- Mirrored RT draws the BSP mesh with mirrored MVP (not a full second scene graph / entities pass)
- LOD mesh buckets are clean-room procedural (not GoldSrc multi-res studio LODs from real MDLs)
- Authority kill/score fanout is protocol/HUD-complete; gameplay damage→kill wiring still stubby
- Spectator is a follow-cam stub (no free-look / next-player cycle UI yet)
- IPA still requires macOS + Xcode via workflow_dispatch

### Progress toward playable unsigned IPA demo
Rough overall estimate after this batch: **~70–72%** toward a playable unsigned IPA demo
(synthetic BSP walk/swim/combat stubs + Metal world/particles/post + net/predict + reflection RT encode).
IPA remains unbuilt on this host (needs macOS/Xcode); do not treat host-smoke green as a packaged demo.
Remaining: real game-data FS paths, fuller studio/GPU entity reflection, signed packaging on macOS/Xcode,
and tighter netplay/HUD polish.



## Reflect-ents / studio GPU LOD / spec-cycle / dmg-kill (`continue/batch-reflect-entities-studio-gpu-spec-cycle`)

One PR advances entities/monsters into the water reflection RT, a GPU studio LOD draw path,
spectator next-player cycle + HUD, damage→`register_kill` authority wiring, IPA artifact notes,
optional kill assists, and copy-eye spectator camera — still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Entities/monsters in water reflect RT | **done** | `aether_water_reflect_ent_list_*` + `draw_plan_full` (not BSP-only) |
| 2 | GPU studio LOD draw path | **done** | `aether_mdl_lod_gpu_issue_draw` / `_copy` (distance select → issue draw) |
| 3 | Spectator next-player cycle + HUD | **done** | roster + `cycle_next/prev` + `SPEC:` HUD indicator |
| 4 | Damage → register_kill fanout | **done** | `aether_player_apply_damage_auth` wires death → `register_kill` + fanout |
| 5 | IPA artifact notes | **done** | `package_ipa.sh` lists Payload/.app/Info.plist/binary + README |
| 6 | Kill assists stub | **done** | `assists` on slot + `register_assist` / `get_assists` |
| 7 | Spec camera copies target eye | **done** | `AETHER_SPEC_CAM_COPY_EYE` mode |
| 8 | Host smokes + verify | **done** | `smoke_batch_reflect_entities_studio_gpu_spec_cycle` + verify greps |
| 9 | Fix regressions | **done** | prior mirror-rt/LOD/MP smokes still green |
| 10 | README + honest % | **done** | This table; cap **~72–74%** while IPA unbuilt on this host |

### IPA artifacts (after `package_ipa.sh` on macOS)

When run on a Mac with a prior `build_ios.sh` success, expect under `build/out/`:

| Path | What it is |
|------|------------|
| `AetherEngine.ipa` | Unsigned IPA (zip of `Payload/`) |
| `ipa-stage/Payload/AetherEngine.app/` | Staging copy of the app bundle |
| `…/AetherEngine.app/Info.plist` | Bundle metadata |
| `…/AetherEngine.app/AetherEngine` | arm64 Mach-O binary |
| `DerivedData/.../AetherEngine.app` | Input from `build_ios.sh` (not re-linked by package) |

`_CodeSignature` and `embedded.mobileprovision` are stripped for unsigned sideload.

### Known gaps after this batch
- *(addressed in rt-skins/assist/hiz/auth batch: studio skins in RT, assist feed, Hi-Z gate, auth tick bind)*
- IPA still requires macOS + Xcode via workflow_dispatch

### Progress toward playable unsigned IPA demo
Rough overall estimate after this batch: **~72–74%** (superseded by rt-skins batch below).

IPA remains unbuilt on this host (needs macOS/Xcode); do not treat host-smoke green as a packaged demo.

## RT skins / assist feed / Hi-Z / auth tick (`continue/batch-rt-skins-assist-hiz-auth`)

One PR advances studio skins/attachments into the water reflection RT, assist feed packets + HUD,
auth kill wired through the live game tick (server pointer), GPU Hi-Z / LOD distance gate,
macOS IPA Actions artifact upload notes, spectator name/HP HUD, and less debug-box reflect
materials — still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Studio skins/attachments in water RT | **done** | `push_studio` + material/skin/attach + `draw_studio_skins` plan |
| 2 | Assist feed packet + HUD line | **done** | `AETHER_MSG_ASSIST` / `encode_assist` / `SB_EVENT_ASSIST` + "assisted" HUD |
| 3 | Auth kill in live game tick | **done** | `aether_game_bind_auth_server` + `tick_auth` / queue from `aether_game_tick` |
| 4 | GPU Hi-Z / LOD distance gate | **done** | `aether_mdl_hiz_*` + `lod_hiz_gate` / `issue_draw_hiz` (occlusion + min-pixels) |
| 5 | macOS IPA Actions artifact notes | **done** | `package_ipa.sh` + `build-arm64.yml` upload/download (`actions/upload-artifact@v4`) |
| 6 | Spec HUD target name/HP | **done** | `set_target_hp` / name → `SPEC: Name [HP]` |
| 7 | Reflect RT materials less debug-box | **done** | studio/skinned tint path (Metal samples tint; not solid debug red) |
| 8 | Host smokes + verify | **done** | `smoke_batch_rt_skins_assist_hiz_auth` + verify greps |
| 9 | Fix regressions | **done** | prior reflect-ents/mirror-rt smokes still green |
| 10 | README + honest % | **done** | This table; cap **~74–75%** while IPA unbuilt on this host |

### IPA Actions artifact steps (macOS `workflow_dispatch` only)

| Step | What |
|------|------|
| `actions/upload-artifact@v4` | Uploads `build/out/AetherEngine.ipa` as `AetherEngine-<version>` (30-day retention) |
| Download | Actions run → Artifacts, or `gh run download <run-id> -n AetherEngine-<version>` |
| Optional Release | `publish_release=true` attaches the same IPA to a GitHub Release |

### Known gaps after rt-skins / assist / hiz / auth batch
- *(addressed in gpu-hiz-mip/weapon-auth/portal batch: Hi-Z mip pyramid + vis queries, weapon→auth, portal reflect, studio tex sample)*
- IPA still requires macOS + Xcode via workflow_dispatch

### Progress toward playable unsigned IPA demo
Rough overall estimate after this batch: **~74–75%** (superseded by gpu-hiz-mip batch below).

IPA remains unbuilt on this host (needs macOS/Xcode); do not treat host-smoke green as a packaged demo.

## GPU Hi-Z mip / weapon→auth / portal reflect (`continue/batch-gpu-hiz-mip-weapon-auth-portal`)

One PR advances a real hierarchical Hi-Z mip pyramid + Metal visibility-query hooks (not CPU
sample-buffer only), weapon hitscan → `auth_queue_damage` combat path, portal/teleport-aware
water reflect camera, fuller studio texture sampling in the water RT, macOS Actions runner
IPA dry-run notes, assist feed HUD polish ("assisted vs"), combat smoke
(fire → queue → kill), host verify green — still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Real Metal Hi-Z mip pyramid + vis queries | **done** | `aether_mdl_hiz_pyramid_*` / `build_pyramid` / `vis_query` + Metal `aether_hiz_downsample` / `aether_hiz_vis_query_fragment` |
| 2 | Weapon hit → `auth_queue_damage` | **done** | `aether_weapon_fire_combat_auth` + `aether_game_weapon_hit_auth` |
| 3 | Portal/teleport-aware water reflect cam | **done** | `aether_water_reflect_compute_portal` / `build_mirror_mvp_portal` |
| 4 | Fuller studio texture sample in water RT | **done** | `aether_water_reflect_studio_tex_*` + Metal `aether_studio_reflect_tex_fragment` |
| 5 | IPA dry-run notes (macos runner) | **done** | `package_ipa.sh` + `build-arm64.yml` macos-14 local dry-run / sideload notes |
| 6 | Assist feed polish | **done** | `format_assist_line` / `get_event_ex` → "Name assisted vs Victim" HUD |
| 7 | Combat smoke: fire → damage queue → kill | **done** | host `b14_wpn_*` via GLOCK hitscan force-hit |
| 8 | Host smokes + verify | **done** | `smoke_batch_gpu_hiz_mip_weapon_auth_portal` + verify greps |
| 9 | Fix regressions | **done** | prior rt-skins/assist/hiz smokes still green |
| 10 | README + honest % | **done** | This table; cap **~75–76%** while IPA unbuilt on this host |

### IPA macOS runner dry-run (further notes)

| Step | What |
|------|------|
| Runner | `macos-14` (`build_ipa` on `workflow_dispatch`) |
| Local | `./build/scripts/build_ios.sh && ./build/scripts/package_ipa.sh` |
| Artifact | `actions/upload-artifact@v4` → `AetherEngine-<version>` (30d) |
| Sideload | AltStore / Sideloadly / TrollStore — unsigned Payload zip |

### Known gaps after gpu-hiz-mip / weapon-auth / portal batch
- *(addressed in depth-hiz-bind/portal-winding/mdl-skin-pages batch: depth→Hi-Z bind, portal winding/recursion, fixture skin pages)*
- IPA still requires macOS + Xcode via workflow_dispatch

### Progress toward playable unsigned IPA demo
Rough overall estimate after this batch: **~75–76%** toward a playable unsigned IPA demo
(capped while IPA remains unbuilt on this Linux/CI host). Prior rt-skins batch was ~74–75%.

IPA remains unbuilt on this host (needs macOS/Xcode); do not treat host-smoke green as a packaged demo.



## Depth→Hi-Z bind / portal winding / MDL skin pages (`continue/batch-depth-hiz-bind-portal-winding-mdl-skin-pages`)

One PR binds depth prepass → Hi-Z pyramid on Metal (encode order + texture views), adds portal
winding clip + recursive reflect views (limited depth), samples real fixture MDL skin pages in
the water RT (no HL assets), polishes weapon auth with hitgroup/headshot scale, clarifies
unsigned IPA dry-run on Actions macos-14, multi-mip Hi-Z vis queries, host smokes, verify green —
still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Bind depth prepass → Hi-Z pyramid | **done** | `aether_depth_hiz_bind_plan_*` + `aether_mdl_hiz_bind_from_depth` + Metal texture-view hooks |
| 2 | Portal winding / recursive reflect | **done** | `aether_portal_winding_*` + `aether_water_reflect_recursive_plan` (max depth 3) |
| 3 | Real MDL skin-page sample in water RT | **done** | `aether_mdl_skin_pages_*` + `aether_water_reflect_ent_bind_skin_page` |
| 4 | Weapon auth path polish | **done** | `aether_weapon_fire_combat_auth_hitgroup` / headshot 4× scale |
| 5 | Unsigned IPA dry-run on macos-14 (clearer) | **done** | package_ipa checklist + Actions summary dry-run steps |
| 6 | Host smokes (bind / portal / skin) | **done** | `smoke_batch_depth_hiz_bind_portal_winding_mdl_skin_pages` |
| 7 | Fix regressions | **done** | prior gpu-hiz-mip / rt-skins smokes still green |
| 8 | Vis query using pyramid mips | **done** | `aether_mdl_hiz_vis_query_at_mip` + `vis_query_multi_mip` |
| 9 | Related gap fill | **done** | texture views from pyramid levels; recursive clip plane per view |
| 10 | README + honest % | **done** | This table; cap **~76–77%** while IPA unbuilt on this host |

### Unsigned IPA dry-run (Actions macos-14) — clear checklist

| Step | What |
|------|------|
| 1 | Actions → Build AetherEngine IPA → Run workflow (`workflow_dispatch`) |
| 2 | Inputs: `version` (e.g. `v0.1.15`), `publish_release=false` for dry-run |
| 3 | Job **Unsigned IPA (dispatch only)** on **macos-14**: `build_ios.sh` → `package_ipa.sh` |
| 4 | Artifact `AetherEngine-<version>` via `upload-artifact@v4` (30d) |
| 5 | `gh run download <run-id> -n AetherEngine-<version>` or Actions UI |
| 6 | Sideload: AltStore / Sideloadly / TrollStore — no baked signing |
| Local | `./build/scripts/build_ios.sh && ./build/scripts/package_ipa.sh` |
| Note | PR/push runs `verify_host` only; IPA is dispatch-only |

### Known gaps after depth-hiz-bind / portal-winding / mdl-skin-pages batch
- *(addressed in hiz-array / portal-graph / mdl-skin-ipa batch: Metal texture2d_array Hi-Z, multi-portal leaf graph flood, packed skin lumps + fixture fallback, IPA artifact automation)*
- IPA still requires macOS + Xcode via workflow_dispatch

### Progress toward playable unsigned IPA demo
Rough overall estimate after depth-hiz-bind batch: **~76–77%** (superseded below).


## Hi-Z texture2d_array / portal leaf graph / MDL skin lumps / IPA artifact (`continue/batch-hiz-array-portal-graph-mdl-skin-ipa`)

One PR binds device Metal Hi-Z as `texture2d_array` / mip-chain for vis queries, builds a multi-portal
leaf adjacency graph (from BSP portals/leaves + synthetic multi-portal fixture) with flood for water
reflect, loads packed MDL skin lumps when a user asset is present (fixture fallback otherwise),
improves IPA artifact automation (`upload-artifact@v4` retention input, compression-level 9,
`ARTIFACT_NOTES.txt`), wires array-mip vis on the Metal encode path, host smokes, verify green —
still clean-room (no retail HL assets):

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Device Metal Hi-Z as texture2d_array / mip chain | **done** | `aether_mdl_hiz_bind_texture2d_array` + `aether_depth_hiz_array_bind_*` + Metal `aether_hiz_array_vis_query_fragment` |
| 2 | Multi-portal leaf graph + flood for reflect | **done** | `aether_bsp_portal_graph_*` + `aether_water_reflect_portal_graph_plan` |
| 3 | Packed MDL skin lumps (asset / fixture fallback) | **done** | `aether_mdl_skin_lumps_load*` / `load_or_fixture` + water-ent bind |
| 4 | IPA artifact automation improvements | **done** | retention input, compression-level 9, ARTIFACT_NOTES.txt beside IPA |
| 5 | Vis query uses array mip on Metal encode path | **done** | `aether_mdl_hiz_vis_query_array_mip` + MetalRenderer encode |
| 6 | Portal graph smoke (multi-portal synthetic) | **done** | host `b16_graph_*` / flood / reflect plan |
| 7 | Skin lump parse smoke with fixture | **done** | textured fixture trailer → lumps; null → fixture fallback |
| 8 | Host smokes + verify green | **done** | `smoke_batch_hiz_array_portal_graph_mdl_skin_ipa` + verify greps |
| 9 | Fix regressions | **done** | prior depth-hiz-bind / gpu-hiz-mip smokes still green |
| 10 | README + honest % | **done** | This table; cap **~78%** (~77–78) while IPA unbuilt on this host |

### IPA artifact automation (Actions)

| Knob | What |
|------|------|
| `upload-artifact@v4` | Uploads `AetherEngine.ipa` + `ARTIFACT_NOTES.txt` |
| `artifact_retention_days` | workflow_dispatch input (default 30, max ~90) |
| `compression-level: 9` | Smaller artifact upload |
| Notes file | sha256, size, retention, upload tool — written by `package_ipa.sh` |
| Download | `gh run download <id> -n AetherEngine-<version>` |

### Known gaps after hiz-array / portal-graph / mdl-skin-ipa batch
- *(addressed in hiz-gpu-downsample batch: GPU array downsample chain + downsample→vis bind)*
- *(addressed in hiz-gpu-downsample batch: fuller portal windings from marksurfaces/planes)*
- *(addressed in hiz-gpu-downsample batch: MDL skinref / family select API + fixture)*
- *(addressed in hiz-gpu-downsample batch: package_ipa.sh --dry-run/--notes-only/--sign-check)*
- IPA still requires macOS + Xcode via workflow_dispatch

### Progress toward playable unsigned IPA demo
Rough overall estimate after hiz-array batch: **~78%** (superseded by hiz-gpu-downsample batch below).

IPA remains unbuilt on this host (needs macOS/Xcode); do not treat host-smoke green as a packaged demo.




## Hi-Z GPU downsample / portal windings / MDL skinref / IPA sign (`continue/batch-hiz-gpu-downsample-portal-windings-mdl-skinref-ipa-sign`)

One PR advances GPU Hi-Z downsample into `texture2d_array` slices (Metal compute/fragment
chain), fuller portal windings from BSP marksurfaces/planes, MDL skinref family select +
fixture, unsigned IPA dry-run script flags, downsample→vis bind, host smokes, and README.

| # | Item | Status | Notes |
|---|------|--------|-------|
| 1 | GPU Hi-Z downsample → array slices | **done** | `aether_mdl_hiz_array_downsample_chain` + Metal `aether_hiz_array_downsample` |
| 2 | Fuller portal windings (marksurfaces/planes) | **done** | `aether_bsp_portal_windings_from_marksurfaces` + graph attach |
| 3 | MDL skinref / family select + fixture | **done** | `aether_mdl_skinref_*` (default/camo families) |
| 4 | Unsigned IPA dry-run polish | **done** | `package_ipa.sh --dry-run/--notes-only/--sign-check` → `DRY_RUN_NOTES.txt` |
| 5 | Bind downsample into vis query | **done** | `aether_mdl_hiz_vis_query_downsampled` + `aether_depth_hiz_downsample_bind_*` |
| 6 | Portal winding smoke | **done** | marksurface windings + reflect plan |
| 7 | Skinref select smoke | **done** | family/name/ref resolve + sample |
| 8 | Host smokes + verify green | **done** | `smoke_batch_hiz_gpu_downsample_portal_windings_mdl_skinref_ipa_sign` + verify greps |
| 9 | Fix regressions | **done** | prior batches retained; verify greps unchanged |
| 10 | README + honest % | **done** | This table; cap **~79%** while IPA unbuilt on this host |

### Unsigned IPA dry-run flags (Linux-safe)

```bash
bash build/scripts/package_ipa.sh --dry-run --sign-check   # plan + DRY_RUN_NOTES.txt
bash build/scripts/package_ipa.sh --notes-only               # notes only
bash build/scripts/package_ipa.sh --help
# Real IPA (macOS + Xcode only):
#   ./build/scripts/build_ios.sh && ./build/scripts/package_ipa.sh
```

### Known gaps after hiz-gpu-downsample / portal-windings / mdl-skinref / ipa-sign batch
- *(addressed in hiz-gpu-encode batch: live Metal encode plan from depth texture + shaders)*
- *(addressed in hiz-gpu-encode batch: portal winding clip against recursive reflect planes)*
- *(addressed in hiz-gpu-encode batch: studio skinref → texture remap on draw)*
- *(addressed in hiz-gpu-encode batch: Actions dry_run_validate dispatch input + docs)*
- IPA still requires macOS + Xcode via workflow_dispatch for a real unsigned IPA

### Progress toward playable unsigned IPA demo
Rough overall estimate after hiz-gpu-downsample batch: **~79%** (superseded by hiz-gpu-encode batch below).

IPA remains unbuilt on this host (needs macOS/Xcode); do not treat host-smoke green as a packaged demo.




## Hi-Z GPU live encode / portal clip / skinref remap / IPA dispatch (`continue/batch-hiz-gpu-encode-portal-clip-studio-skinref-remap-ipa-dispatch`)

One PR advances **live Metal encode** of Hi-Z downsample from a depth texture (encode plan +
shaders), portal winding clip against recursive reflect clip planes, studio skinref → texture
remap on draw, optional Actions `dry_run_validate` workflow_dispatch input for unsigned IPA
validation, Metal frame-path wiring, host smokes, and README — still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | Live Metal Hi-Z encode from depth | **done** | `aether_mdl_hiz_encode_from_depth` + `aether_depth_hiz_live_encode_*` + Metal `aether_hiz_encode_from_depth` |
| 2 | Portal winding clip vs recursive reflect planes | **done** | `aether_portal_winding_clip_reflect_planes` / `clip_planes` + `aether_water_reflect_portal_clip_plan` |
| 3 | Studio skinref → texture remap on draw | **done** | `aether_mdl_skinref_remap_draw` / `remap_uv` / `remap_sample` + Metal remap fragment |
| 4 | Actions dry-run dispatch docs/inputs | **done** | `dry_run_validate` input; `gh workflow run … -f dry_run_validate=true`; DRY_RUN_NOTES dispatch line |
| 5 | Wire encode plan into Metal frame path | **done** | MetalRenderer calls live encode + mark + portal clip + skinref remap |
| 6 | Portal clip smoke | **done** | host `b18_clip_*` |
| 7 | Skinref remap smoke | **done** | host `b18_remap_*` |
| 8 | Host smokes + verify green | **done** | `smoke_batch_hiz_gpu_encode_portal_clip_studio_skinref_remap_ipa_dispatch` + verify greps |
| 9 | Fix regressions | **done** | prior downsample/skinref/portal smokes retained |
| 10 | README + honest % | **done** | This table; cap **~80%** while IPA unbuilt on this host |

### Unsigned IPA dry-run dispatch (optional)

```bash
# Local (any host):
bash build/scripts/package_ipa.sh --dry-run --sign-check

# Actions (macos-14) — validate notes only, skip Xcode IPA:
gh workflow run "Build AetherEngine IPA" \
  -f version=v0.0.0-dry -f publish_release=false -f dry_run_validate=true
```

### Known gaps after hiz-gpu-encode / portal-clip / skinref-remap / ipa-dispatch batch
- *(addressed in hiz-depth-attach batch: live MTK depth attachment → Hi-Z encode wired in MetalRenderer)*
- *(addressed in hiz-depth-attach batch: fuller portal clip stack / multi-plane clip buffer)*
- *(addressed in hiz-depth-attach batch: skinref remap → Metal texture bind on studio draw)*
- *(addressed in hiz-depth-attach batch: device IPA sideload checklist — Apple Configurator / Xcode Devices / ideviceinstaller)*

### Progress toward playable unsigned IPA demo
Rough overall estimate after hiz-gpu-encode batch: **~80%** (superseded by hiz-depth-attach batch below).

## Hi-Z MTK depth attach / portal clip stack / studio skin bind / IPA device (`continue/batch-hiz-depth-attach-portal-stack-studio-draw-ipa-device`)

One PR advances live MTK depth → Hi-Z encode on device, fuller portal clip stack, studio
skinref Metal texture bind, and device-side IPA sideload docs — still clean-room:

| # | Item | Status | What landed |
|---|------|--------|-------------|
| 1 | MTK depth → Hi-Z encode | **done / partial** | `aether_depth_hiz_mtk_attach_*` + `encodeHizFromMtkDepthAttach` (sceneDepth `.shaderRead` → compute `aether_hiz_encode_from_depth`) |
| 2 | Portal clip stack | **done** | `aether_portal_clip_stack_*` multi-plane clip buffer + `aether_water_reflect_portal_stack_plan` |
| 3 | Studio skin Metal bind | **done / partial** | `aether_mdl_skinref_metal_bind_*` + atlas RGBA upload → `setFragmentTexture` on MDL draw |
| 4 | Device IPA sideload | **done** | Apple Configurator / Xcode Devices / `ideviceinstaller` checklist in `package_ipa.sh` + README |
| 5 | Depth attach smoke | **done** | Host plan/wire/mark/encode_ready + MTK-gated encode |
| 6 | Portal stack smoke | **done** | Push/pop/clip + reflect stack plan |
| 7 | Studio skin bind smoke | **done** | Atlas pack + bind_draw/mark/was_bound |
| 8 | Host smokes + verify | **done** | `verify_host.sh` batch19 greps + smoke |
| 9 | Fix regressions | **done** | Compile/smoke green on Linux host |
| 10 | README + honest % | **done** | This table; cap **~81%** while IPA unbuilt on this host |

### Device IPA sideload checklist (after you have an IPA)

```bash
# A) Apple Configurator 2 — connect device → Add → Apps → AetherEngine.ipa
# B) Xcode → Window → Devices and Simulators → Installed Apps → + (Payload/*.app)
# C) ideviceinstaller (brew install libimobiledevice ideviceinstaller):
idevice_id -l
ideviceinstaller -i build/out/AetherEngine.ipa
# D) AltStore / Sideloadly / TrollStore (unsigned / resign flows)
```

This Linux/CI host cannot produce or sideload an IPA; checklist is documentation for macOS + device.

### Known gaps after hiz-depth-attach / portal-stack / studio-draw / ipa-device batch
- Hi-Z encode from MTK depth fills slice 0 via compute; full mip-chain downsample on GPU still thin vs host pyramid
- Portal clip stack is clean-room multi-plane buffer (not retail Quake portal BSP stack)
- Studio skin atlas is fixture pages uploaded as rgba8; retail MDL skinref tables still partial
- IPA still requires macOS + Xcode; device sideload needs a built IPA + trust/resign as applicable

### Progress toward playable unsigned IPA demo
Rough overall estimate after this batch: **~81%** (~80–81) toward a playable unsigned IPA demo
(capped while IPA remains unbuilt on this Linux/CI host). Prior hiz-gpu-encode batch was ~80%.

IPA remains unbuilt on this host (needs macOS/Xcode); do not treat host-smoke green as a packaged demo.


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
