#!/usr/bin/env bash
# verify_host.sh — Host-side compile + smoke test for AetherEngine C sources.
# Runs on Linux/macOS without Xcode. AetherEngine-iOS · Clean-room.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

GREEN='\033[0;32m'; RED='\033[0;31m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok()   { echo -e "${GREEN}OK${NC}  $*"; }
fail() { echo -e "${RED}FAIL${NC} $*" >&2; exit 1; }
info() { echo -e "${YELLOW}==>${NC} $*"; }

OBJ_DIR="${ROOT}/build/out/host-obj"
LIB_DIR="${ROOT}/build/out/host-lib"
BIN_DIR="${ROOT}/build/out/host-bin"
rm -rf "${OBJ_DIR}"
mkdir -p "${OBJ_DIR}" "${LIB_DIR}" "${BIN_DIR}"

CC="${CC:-cc}"
CFLAGS=(
  -std=c11 -Wall -Wextra -Wno-unused-parameter
  -O0 -g
  -Iengine/core -Iengine/console -Iengine/command -Iengine/thread
  -Iengine/game -Iengine/game/weapons -Iengine/game/monsters
  -Iengine/game/dll -Iengine/game/ai -Iengine/physics
  -Iengine/save -Iengine/net -Iengine/input -Iengine/config
  -Iengine/fs -Iengine/audio -Iengine/render -Iengine/bsp
  -Iengine/map
  -Iengine/player -Iengine/texture -Iengine/model -Iengine/entity
  -Iengine/client/hud -Iengine/client/menu -Iengine/vgui
)

info "1/6 Verify required paths (STEPs 3–10)"
REQUIRED=(
  engine/core/AetherCore.c
  engine/core/AetherEngine.c
  engine/game/AetherGameManager.c
  engine/game/AetherManifest.c
  engine/fs/AetherFS.c
  engine/audio/AetherAudio.c
  engine/input/AetherInput.c
  engine/config/AetherSettings.c
  engine/console/AetherCVar.c
  engine/bsp/AetherBSP.c
  engine/map/AetherMapLoad.c
  engine/audio/AetherWav.c
  engine/render/AetherDynLight.c
  ios/AetherApp/EngineBridge.c
  ios/AetherApp/EngineBridge.h
  ios/AetherApp/AetherApp.swift
  ios/AetherApp/MetalRenderer.swift
  ios/AetherApp/AetherAudioIOS.swift
  ios/AetherApp/SettingsView.swift
  ios/AetherApp/TouchControlsView.swift
  ios/AetherApp/DashboardView.swift
  build/CMakeLists.txt
  build/project.yml
  build/scripts/build_ios.sh
  build/scripts/cmake_host.sh
  .github/workflows/verify.yml
  .github/workflows/build-arm64.yml
  tests/host_smoke.c
)
for f in "${REQUIRED[@]}"; do
  [ -f "$f" ] || fail "missing $f"
  ok "$f"
done

info "2/6 Verify game manifests"
for m in valve bshift gearbox cstrike czero; do
  [ -f "engine/game/manifests/$m.json" ] || fail "missing manifest $m.json"
  ok "manifest $m.json"
done

info "3/6 Compile all engine C sources"
COUNT=0
FAIL=0
LIST_FILE="${OBJ_DIR}/sources.list"
find engine -name '*.c' | sort > "${LIST_FILE}"
while IFS= read -r f; do
  COUNT=$((COUNT + 1))
  base="$(echo "$f" | tr '/' '_')"
  out="${OBJ_DIR}/${base%.c}.o"
  if ! "${CC}" "${CFLAGS[@]}" -c "$f" -o "$out"; then
    echo "compile failed: $f" >&2
    FAIL=1
  fi
done < "${LIST_FILE}"
[ "$COUNT" -ge 80 ] || fail "expected >=80 engine .c files, found $COUNT"
ok "found $COUNT engine .c files"
[ "$FAIL" -eq 0 ] || fail "one or more C sources failed to compile"
ok "compiled $COUNT object files"

info "4/6 Archive static library"
# shellcheck disable=SC2086
ar rcs "${LIB_DIR}/libaether_engine.a" ${OBJ_DIR}/*.o
ok "libaether_engine.a"

info "5/6 Link and run host smoke test"
"${CC}" "${CFLAGS[@]}" -O1 \
  tests/host_smoke.c \
  "${LIB_DIR}/libaether_engine.a" \
  -lm \
  -o "${BIN_DIR}/host_smoke"
# host_smoke main() is large at -O0; raise stack for nested locals.
ulimit -S -s unlimited 2>/dev/null || ulimit -s 1048576 2>/dev/null || ulimit -s 65536 || true
echo "host_smoke stack limit: $(ulimit -s)"
"${BIN_DIR}/host_smoke"
ok "host smoke passed"

info "6/6 API surface greps (bridge + host frame)"
grep -q "aether_engine_host_frame" engine/core/AetherEngine.h || fail "missing aether_engine_host_frame"
grep -q "aether_fs_setup_game" engine/fs/AetherFS.h || fail "missing aether_fs_setup_game"
grep -q "engine_host_frame" ios/AetherApp/EngineBridge.h || fail "missing engine_host_frame"
grep -q "engine_game_select" ios/AetherApp/EngineBridge.h || fail "missing engine_game_select"
grep -q "engine_settings_apply" ios/AetherApp/EngineBridge.h || fail "missing engine_settings_apply"
grep -q "aether_game_manager_as_subsystem" engine/game/AetherGameManager.h || fail "missing game subsystem glue"
grep -q "aether_map_load" engine/map/AetherMapLoad.h || fail "missing aether_map_load"
grep -q "aether_audio_play_beep" engine/audio/AetherAudio.h || fail "missing aether_audio_play_beep"
grep -q "aether_wav_parse_header" engine/audio/AetherWav.h || fail "missing aether_wav_parse_header"
grep -q "aether_lightmap_bake_from_bsp" engine/render/AetherLightmap.h || fail "missing lightmap bake_from_bsp"
grep -q "aether_decals_copy_render" engine/render/AetherDecal.h || fail "missing decals_copy_render"
grep -q "aether_dyn_lights_add" engine/render/AetherDynLight.h || fail "missing dyn_lights_add"
grep -q "aether_save_write" engine/save/AetherSave.h || fail "missing aether_save_write"
grep -q "aether_net_client_connect" engine/net/AetherNetClient.h || fail "missing net client connect"
grep -q "engine_map_load" ios/AetherApp/EngineBridge.h || fail "missing engine_map_load"
ok "STEP 3–7 + batch map/audio/ci API symbols present"

grep -q "aether_lightmap_unpack_uvs_from_bsp" engine/render/AetherLightmap.h || fail "missing lightmap unpack_uvs"
grep -q "aether_bsp_vis_encode_pvs_row" engine/bsp/AetherBSPVis.h || fail "missing vis encode_pvs"
grep -q "aether_audio_play_wav_data" engine/audio/AetherAudio.h || fail "missing play_wav_data"
grep -q "aether_decals_copy_quads" engine/render/AetherDecal.h || fail "missing decals_copy_quads"
grep -q "aether_net_snapshot_apply_hud" engine/net/AetherNetSnapshot.h || fail "missing snapshot apply_hud"
grep -q "aether_dyn_lights_apply_mesh_tint" engine/render/AetherDynLight.h || fail "missing dynlight mesh tint"
grep -q "aether_sprite_copy_quad" engine/render/AetherSprite.h || fail "missing sprite_copy_quad"
grep -q "aether_player_tick_fire" engine/player/AetherPlayerDamage.h || fail "missing tick_fire"
grep -q "aether_frustum_aabb_visible" engine/render/AetherFrustum.h || fail "missing frustum aabb"
grep -q "engine_decals_copy_quads" ios/AetherApp/EngineBridge.h || fail "missing bridge decals_copy_quads"
ok "batch UV/WAV/decals/net API symbols present"



grep -q "aether_dyn_lights_fill_ubo" engine/render/AetherDynLight.h || fail "missing dyn_lights_fill_ubo"
grep -q "aether_decals_project_onto_mesh" engine/render/AetherDecal.h || fail "missing decals_project_onto_mesh"
grep -q "aether_net_client_apply_snapshot_hud" engine/net/AetherNetClient.h || fail "missing client apply_snapshot_hud"
grep -q "aether_lightstyles_update" engine/render/AetherLightmap.h || fail "missing lightstyles_update"
grep -q "aether_mdl_write_fixture" engine/model/AetherModelFixture.h || fail "missing mdl_write_fixture"
grep -q "aether_shadow_copy_blob" engine/render/AetherShadow.h || fail "missing shadow_copy_blob"
grep -q "aether_postfx_set_from_cvars" engine/render/AetherPostFX.h || fail "missing postfx_set_from_cvars"
grep -q "aether_interact_trace" engine/input/AetherInteract.h || fail "missing interact_trace"
grep -q "engine_dynlights_fill_ubo" ios/AetherApp/EngineBridge.h || fail "missing bridge dynlights_fill_ubo"
grep -q "engine_interact_trace" ios/AetherApp/EngineBridge.h || fail "missing bridge interact_trace"
ok "batch GPU lights / decal clip / netplay API symbols present"


grep -q "aether_postfx_ensure_offscreen" engine/render/AetherPostFX.h || fail "missing postfx_ensure_offscreen"
grep -q "aether_postfx_fill_uniforms" engine/render/AetherPostFX.h || fail "missing postfx_fill_uniforms"
grep -q "aether_lightmap_apply_style_pingpong" engine/render/AetherLightmap.h || fail "missing lightmap pingpong"
grep -q "aether_net_delta_encode" engine/net/AetherNetDelta.h || fail "missing net_delta_encode"
grep -q "aether_net_interp_origin" engine/net/AetherNetInterp.h || fail "missing net_interp_origin"
grep -q "aether_net_predict_reconcile" engine/net/AetherNetPredict.h || fail "missing net_predict_reconcile"
grep -q "aether_dyn_lights_fill_array" engine/render/AetherDynLight.h || fail "missing dyn_lights_fill_array"
grep -q "aether_decals_clip_to_world" engine/render/AetherDecal.h || fail "missing decals_clip_to_world"
grep -q "engine_postfx_ensure_offscreen" ios/AetherApp/EngineBridge.h || fail "missing bridge postfx_ensure_offscreen"
grep -q "engine_net_predict_local_step" ios/AetherApp/EngineBridge.h || fail "missing bridge predict"
ok "batch postfx/lightmap/mdl/predict API symbols present"




grep -q "aether_lightstyles_fill_gpu_weights" engine/render/AetherLightmap.h || fail "missing lightstyles_fill_gpu_weights"
grep -q "aether_mdl_skin_build_stub" engine/render/AetherMDLAnimation.h || fail "missing mdl_skin_build_stub"
grep -q "aether_mdl_write_textured_fixture" engine/model/AetherModelFixture.h || fail "missing textured_fixture"
grep -q "aether_net_client_live_tick" engine/net/AetherNetClient.h || fail "missing client_live_tick"
grep -q "aether_net_cmd_encode" engine/net/AetherNetCmd.h || fail "missing net_cmd_encode"
grep -q "aether_postfx_fill_bloom" engine/render/AetherPostFX.h || fail "missing postfx_fill_bloom"
grep -q "aether_decal_atlas_generate_stub" engine/render/AetherDecal.h || fail "missing decal_atlas"
grep -q "aether_net_server_tick_authority" engine/net/AetherNetServer.h || fail "missing server_tick_authority"
grep -q "aether_net_cmd_history_at_lag" engine/net/AetherNetCmd.h || fail "missing cmd_history_lag"
grep -q "engine_lightstyles_fill_gpu_weights" ios/AetherApp/EngineBridge.h || fail "missing bridge style weights"
grep -q "engine_net_client_live_tick" ios/AetherApp/EngineBridge.h || fail "missing bridge live_tick"
ok "batch gpu-lightstyles/skin/mp API symbols present"

grep -q "aether_mdl_skin_build_from_sequence" engine/render/AetherMDLAnimation.h || fail "missing seq skin"
grep -q "aether_mdl_write_seq_fixture" engine/model/AetherModelFixture.h || fail "missing seq fixture"
grep -q "aether_lightmap_fill_face_style_weights" engine/render/AetherLightmap.h || fail "missing face style weights"
grep -q "aether_audio_spatial_atten" engine/audio/AetherAudio.h || fail "missing spatial atten"
grep -q "aether_hud_layout_classic" engine/client/hud/AetherHUDLayout.h || fail "missing hud layout"
grep -q "aether_net_predict_apply_cmd_clipped" engine/net/AetherNetPredict.h || fail "missing predict clipped"
grep -q "aether_weapon_view_copy_stub" engine/game/weapons/AetherWeaponView.h || fail "missing viewmodel stub"
grep -q "aether_monster_ai_tick_registry" engine/game/monsters/AetherMonsterAI.h || fail "missing monster ai tick"
grep -q "aether_postfx_bloom_encode_needed" engine/render/AetherPostFX.h || fail "missing bloom encode needed"
grep -q "engine_mdl_skin_build_from_sequence" ios/AetherApp/EngineBridge.h || fail "missing bridge seq skin"
grep -q "engine_net_predict_apply_cmd_clipped" ios/AetherApp/EngineBridge.h || fail "missing bridge predict clip"
ok "batch seq/styles/spatial/HUD API symbols present"

grep -q "aether_mdl_write_studio_fixture" engine/model/AetherModelFixture.h || fail "missing studio fixture"
grep -q "aether_mdl_sequence_load_from_data" engine/render/AetherMDLAnimation.h || fail "missing seq load_from_data"
grep -q "aether_lightmap_fill_face_style_blend" engine/render/AetherLightmap.h || fail "missing style blend"
grep -q "aether_audio_play_beep_stereo_at" engine/audio/AetherAudio.h || fail "missing stereo beep"
grep -q "aether_weapon_view_copy_mdl_fixture" engine/game/weapons/AetherWeaponView.h || fail "missing view mdl"
grep -q "aether_lagcomp_query" engine/net/AetherLagComp.h || fail "missing lagcomp query"
grep -q "aether_dyn_lights_cull_pvs" engine/render/AetherDynLight.h || fail "missing dynlight pvs cull"
grep -q "aether_mdl_hitbox_trace" engine/model/AetherModelFixture.h || fail "missing hitbox trace"
grep -q "aether_particles_spawn_muzzle" engine/render/AetherParticle.h || fail "missing muzzle particles"
grep -q "engine_audio_play_beep_stereo_at" ios/AetherApp/EngineBridge.h || fail "missing bridge stereo"
grep -q "engine_dynlights_fill_ubo_pvs" ios/AetherApp/EngineBridge.h || fail "missing bridge pvs lights"
ok "batch studio/vis/stereo API symbols present"

info "All host verification checks passed."

grep -q "aether_lightmap_sample_style_blend" engine/render/AetherLightmap.h || fail "missing style blend sample"
grep -q "aether_lightmap_fill_style_blend_ubo" engine/render/AetherLightmap.h || fail "missing style blend ubo"
grep -q "aether_weapon_view_copy_skinned" engine/game/weapons/AetherWeaponView.h || fail "missing skinned viewmodel"
grep -q "aether_lagcomp_validate_hit" engine/net/AetherLagComp.h || fail "missing lagcomp validate"
grep -q "aether_mdl_anim_rle_decode" engine/render/AetherMDLAnimation.h || fail "missing anim RLE"
grep -q "aether_dyn_lights_cull_pvs_bleed" engine/render/AetherDynLight.h || fail "missing dynlight bleed"
grep -q "aether_particles_spawn_viewmodel_fire" engine/render/AetherParticle.h || fail "missing vm fire particles"
grep -q "aether_mdl_studio_events_fire" engine/model/AetherModelFixture.h || fail "missing studio events"
grep -q "aether_postfx_bloom_encode_plan" engine/render/AetherPostFX.h || fail "missing bloom encode plan"
grep -q "aether_fragment_style_blend" ios/AetherApp/Shaders.metal || fail "missing metal style blend"
grep -q "aether_bloom_blur_h_fragment" ios/AetherApp/Shaders.metal || fail "missing metal bloom H"
grep -q "engine_lagcomp_validate_hit" ios/AetherApp/EngineBridge.h || fail "missing bridge lagcomp validate"
grep -q "engine_weapon_view_copy_skinned" ios/AetherApp/EngineBridge.h || fail "missing bridge skinned view"
ok "batch metal-blend/studio-attach API symbols present"

grep -q "aether_mesh_assign_face_ids\|face_id" engine/bsp/AetherBSPGeometry.h || fail "missing mesh face_id"
grep -q "AETHER_MESH_VERTEX_STRIDE" engine/bsp/AetherBSPGeometry.h || fail "missing mesh vertex stride"
grep -q "aether_lagcomp_studio_trace" engine/net/AetherLagComp.h || fail "missing lagcomp studio trace"
grep -q "aether_lagcomp_hitbox_to_world" engine/net/AetherLagComp.h || fail "missing hitbox_to_world"
grep -q "aether_dyn_lights_cull_portal_flood" engine/render/AetherDynLight.h || fail "missing portal flood"
grep -q "aether_bsp_build_leaf_portal_links" engine/render/AetherDynLight.h || fail "missing portal links"
grep -q "aether_mdl_attachment_chain_world" engine/model/AetherModelFixture.h || fail "missing attach chain"
grep -q "aether_particles_sync_muzzle_world" engine/render/AetherParticle.h || fail "missing muzzle sync"
grep -q "aether_lightmap_fill_style_blend_draw" engine/render/AetherLightmap.h || fail "missing style blend draw"
grep -q "aether_player_inv_cycle" engine/player/AetherPlayerInventory.h || fail "missing inv cycle"
grep -q "face_id" ios/AetherApp/Shaders.metal || fail "missing metal face_id attr"
grep -q "aether_fragment_style_blend" ios/AetherApp/Shaders.metal || fail "missing metal style blend"
grep -q "engine_lightmap_fill_style_blend_draw" ios/AetherApp/EngineBridge.h || fail "missing bridge style blend draw"
grep -q "engine_lagcomp_studio_trace" ios/AetherApp/EngineBridge.h || fail "missing bridge studio lagcomp"
grep -q "engine_inv_cycle" ios/AetherApp/EngineBridge.h || fail "missing bridge inv cycle"
ok "batch faceid/bone/portal/attach API symbols present"

grep -q "aether_mdl_write_lod_fixture\|aether_mdl_lod_select" engine/model/AetherModelFixture.h || fail "missing lod fixture/select"
grep -q "aether_mdl_bodygroup_cycle\|aether_mdl_bodygroup_set" engine/model/AetherModelFixture.h || fail "missing bodygroup API"
grep -q "aether_water_reflect_compute\|aether_water_reflect_fill_uniforms" engine/render/AetherWater.h || fail "missing water reflect"
grep -q "aether_scoreboard_encode_join\|aether_scoreboard_handle_packet" engine/net/AetherNetScoreboard.h || fail "missing netscore join/leave"
grep -q "aether_net_server_broadcast_join" engine/net/AetherNetServer.h || fail "missing server broadcast_join"
grep -q "aether_net_predict_smooth_tick\|aether_net_predict_reconcile_smooth" engine/net/AetherNetPredict.h || fail "missing predict smooth"
grep -q "aether_depth_prepass_encode_plan\|aether_depth_prepass_record_stub" engine/render/AetherDepthPrepass.h || fail "missing depth prepass"
grep -q "aether_depth_prepass_vertex\|aether_depth_prepass_fragment" ios/AetherApp/Shaders.metal || fail "missing metal depth prepass"
grep -q "engine_scoreboard_event_count\|engine_bodygroup_cycle" ios/AetherApp/EngineBridge.h || fail "missing bridge lod/netscore"
grep -q "engine_water_reflect_compute\|engine_depth_prepass_encode_plan" ios/AetherApp/EngineBridge.h || fail "missing bridge water/depth"
grep -q "JoinLeaveRow\|engine_scoreboard_get_event" ios/AetherApp/ClassicScoreboardChatOverlay.swift || fail "missing scoreboard join/leave HUD"
grep -q "smoke_batch_studio_lod_water_reflect_netscore" tests/host_smoke.c || fail "missing lod/water/netscore smoke"
ok "batch studio-lod/water-reflect/netscore API symbols present"

grep -q "aether_water_reflect_rt_ensure\|aether_water_reflect_rt_encode_plan" engine/render/AetherWater.h || fail "missing water reflect RT"
grep -q "aether_mdl_lod_extract_by_distance\|aether_mdl_lod_extract_mesh" engine/model/AetherModelFixture.h || fail "missing lod mesh extract"
grep -q "aether_mdl_texgroup_cycle\|aether_mdl_write_skin_lod_fixture" engine/model/AetherModelFixture.h || fail "missing skin/texture group"
grep -q "aether_net_server_set_score\|aether_net_server_register_kill" engine/net/AetherNetServer.h || fail "missing MP score/kill"
grep -q "aether_chat_encode_voice_cue\|aether_chat_apply_net" engine/net/AetherNetChat.h || fail "missing chat/voice cue"
grep -q "aether_scoreboard_encode_kill\|AETHER_SB_EVENT_KILL" engine/net/AetherNetScoreboard.h || fail "missing kill feed"
grep -q "aether_net_predict_reconcile_teleport\|teleport_threshold" engine/net/AetherNetPredict.h || fail "missing predict teleport"
grep -q "aether_depth_prepass_bind_before_main\|aether_depth_prepass_was_bound_before_main" engine/render/AetherDepthPrepass.h || fail "missing depth bind-before-main"
grep -q "aether_water_reflect_sample\|reflectOn" ios/AetherApp/Shaders.metal || fail "missing metal reflect sample"
grep -q "encodeDepthPrepassIfNeeded\|ensureWaterReflectRT\|waterReflectTexture" ios/AetherApp/MetalRenderer.swift || fail "missing metal depth/reflect RT"
grep -q "engine_water_reflect_rt_ensure\|engine_mdl_lod_extract_by_distance\|engine_texgroup_cycle\|engine_net_predict_reconcile_teleport\|engine_depth_prepass_bind_before_main" ios/AetherApp/EngineBridge.h || fail "missing bridge batch10"
grep -q "killed\|kind == 2" ios/AetherApp/ClassicScoreboardChatOverlay.swift || fail "missing kill feed HUD"
grep -q "smoke_batch_reflect_rt_studio_skin_mp_hud" tests/host_smoke.c || fail "missing reflect-rt/skin/mp-hud smoke"
ok "batch reflect-rt/studio-skin/mp-hud API symbols present"

grep -q "aether_water_reflect_rt_draw_plan\|aether_water_reflect_rt_build_mirror_mvp" engine/render/AetherWater.h || fail "missing water mirror MVP/draw plan"
grep -q "aether_water_reflect_rt_clear\|aether_water_reflect_rt_gen_mips" engine/render/AetherWater.h || fail "missing water RT clear/mips"
grep -q "aether_mdl_write_lod_mesh_fixture\|aether_mdl_lod_mesh_select" engine/model/AetherModelFixture.h || fail "missing multi-mesh LOD buckets"
grep -q "aether_net_server_tick_authority_kill_score\|aether_net_server_fanout_scores" engine/net/AetherNetServer.h || fail "missing MP kill/score fanout"
grep -q "aether_depth_prepass_camera_set\|aether_depth_prepass_encode_plan_ex" engine/render/AetherDepthPrepass.h || fail "missing depth camera MVP"
grep -q "aether_spectator_follow\|aether_spectator_tick" engine/net/AetherNetSpectator.h || fail "missing spectator follow"
grep -q "encodeWaterReflectPass\|engine_depth_prepass_camera_set\|engine_spectator_is_following" ios/AetherApp/MetalRenderer.swift || fail "missing metal mirror-RT/depth-cam/spectator"
grep -q "engine_water_reflect_rt_draw_plan\|engine_mdl_lod_mesh_select\|engine_depth_prepass_camera_set\|engine_spectator_follow" ios/AetherApp/EngineBridge.h || fail "missing bridge batch11"
grep -q "workflow_dispatch\|build_ios.sh\|package_ipa.sh" README.md || fail "missing IPA dry-run docs"
grep -q "IPA dry-run\|workflow_dispatch checklist\|build_ios.sh" README.md || fail "missing IPA checklist section"
grep -q "smoke_batch_mirror_rt_lod_mp_ipa_docs" tests/host_smoke.c || fail "missing mirror-rt/lod-mp smoke"
ok "batch mirror-rt/lod-mp/ipa-docs API symbols present"

grep -q "aether_water_reflect_rt_draw_plan_full\|aether_water_reflect_ent_list_push" engine/render/AetherWater.h || fail "missing water reflect ents"
grep -q "aether_mdl_lod_gpu_issue_draw\|aether_mdl_lod_gpu_draw_t" engine/model/AetherModelFixture.h || fail "missing GPU studio LOD draw"
grep -q "aether_spectator_cycle_next\|aether_spectator_hud_indicator" engine/net/AetherNetSpectator.h || fail "missing spectator cycle/HUD"
grep -q "aether_player_apply_damage_auth\|aether_damage_kill_result" engine/player/AetherPlayerDamage.h || fail "missing damage→kill auth"
grep -q "aether_net_server_register_assist\|assists" engine/net/AetherNetServer.h || fail "missing kill assists stub"
grep -q "AETHER_SPEC_CAM_COPY_EYE\|aether_spectator_set_cam_mode" engine/net/AetherNetSpectator.h || fail "missing spec copy-eye"
grep -q "Artifact notes\|Payload/AetherEngine.app\|Info.plist" build/scripts/package_ipa.sh || fail "missing IPA artifact notes"
grep -q "engine_water_reflect_rt_draw_plan_full\|engine_mdl_lod_gpu_issue_draw\|engine_spectator_cycle_next\|engine_player_apply_damage_auth" ios/AetherApp/EngineBridge.h || fail "missing bridge batch12"
grep -q "engine_water_reflect_ent_push\|engine_mdl_lod_gpu_issue_draw\|draw_plan_full" ios/AetherApp/MetalRenderer.swift || fail "missing metal reflect-ents/LOD"
grep -q "SPEC:\|engine_spectator_hud_indicator\|specLabel" ios/AetherApp/ClassicHUDOverlay.swift || fail "missing spectator HUD indicator"
grep -q "IPA artifact\|package_ipa.sh\|Payload" README.md || fail "missing IPA artifact README notes"
grep -q "smoke_batch_reflect_entities_studio_gpu_spec_cycle" tests/host_smoke.c || fail "missing reflect-ents/spec-cycle smoke"
ok "batch reflect-ents/studio-gpu/spec-cycle API symbols present"

grep -q "aether_water_reflect_ent_list_push_studio\|aether_water_reflect_rt_draw_plan_studio" engine/render/AetherWater.h || fail "missing reflect studio skins"
grep -q "AETHER_MSG_ASSIST\|aether_scoreboard_encode_assist\|AETHER_SB_EVENT_ASSIST" engine/net/AetherNetProtocol.h engine/net/AetherNetScoreboard.h || fail "missing assist feed"
grep -q "aether_net_server_broadcast_assist\|register_assist" engine/net/AetherNetServer.h || fail "missing assist broadcast"
grep -q "aether_game_bind_auth_server\|aether_game_tick_auth\|aether_game_auth_queue_damage" engine/game/AetherGameManager.h || fail "missing auth game tick"
grep -q "aether_mdl_lod_hiz_gate\|aether_mdl_hiz_t\|aether_mdl_lod_gpu_issue_draw_hiz" engine/model/AetherModelFixture.h || fail "missing Hi-Z LOD gate"
grep -q "aether_spectator_set_target_hp\|target_hp" engine/net/AetherNetSpectator.h || fail "missing spec target HP"
grep -q "Actions artifact\|upload-artifact\|gh run download" build/scripts/package_ipa.sh || fail "missing IPA Actions artifact notes"
grep -q "Artifact upload\|Artifact download\|upload-artifact" .github/workflows/build-arm64.yml || fail "missing Actions artifact summary"
grep -q "engine_water_reflect_ent_push_studio\|engine_mdl_lod_hiz_gate\|engine_game_tick_auth\|engine_spectator_set_target_hp\|engine_scoreboard_encode_assist" ios/AetherApp/EngineBridge.h || fail "missing bridge batch13"
grep -q "engine_water_reflect_ent_push_studio\|engine_mdl_lod_gpu_issue_draw_hiz\|draw_plan_studio" ios/AetherApp/MetalRenderer.swift || fail "missing metal studio/hiz reflect"
grep -q "assisted\|kind == 3" ios/AetherApp/ClassicScoreboardChatOverlay.swift || fail "missing assist HUD line"
grep -q "Actions artifact\|upload-artifact\|rt-skins\|assist-hiz\|~75%" README.md || fail "missing batch13 README"
grep -q "smoke_batch_rt_skins_assist_hiz_auth" tests/host_smoke.c || fail "missing rt-skins/assist/hiz/auth smoke"
ok "batch rt-skins/assist-hiz/auth API symbols present"


grep -q "aether_mdl_hiz_build_pyramid\|aether_mdl_hiz_vis_query\|aether_mdl_hiz_pyramid" engine/model/AetherModelFixture.h || fail "missing Hi-Z mip pyramid"
grep -q "aether_weapon_fire_combat_auth\|aether_game_weapon_hit_auth" engine/game/weapons/AetherWeaponFiring.h engine/game/AetherGameManager.h || fail "missing weapon→auth combat"
grep -q "aether_water_reflect_compute_portal\|aether_water_reflect_rt_build_mirror_mvp_portal" engine/render/AetherWater.h || fail "missing portal water reflect"
grep -q "aether_water_reflect_studio_tex_sample\|aether_water_reflect_ent_set_studio_tex" engine/render/AetherWater.h || fail "missing studio tex sample"
grep -q "macos-14\|Local dry-run\|build_ios.sh && ./build/scripts/package_ipa" build/scripts/package_ipa.sh .github/workflows/build-arm64.yml || fail "missing IPA macos runner dry-run notes"
grep -q "aether_scoreboard_format_assist_line\|assisted vs" engine/net/AetherNetScoreboard.h engine/net/AetherNetScoreboard.c || fail "missing assist feed polish"
grep -q "aether_hiz_downsample\|aether_hiz_vis_query_fragment\|aether_studio_reflect_tex_fragment" ios/AetherApp/Shaders.metal || fail "missing Metal Hi-Z/studio tex hooks"
grep -q "engine_mdl_hiz_build_pyramid\|engine_game_weapon_hit_auth\|engine_water_reflect_compute_portal\|engine_scoreboard_get_event_ex" ios/AetherApp/EngineBridge.h || fail "missing bridge batch14"
grep -q "engine_mdl_hiz_build_pyramid\|engine_water_reflect_rt_build_mirror_mvp_portal\|engine_water_reflect_ent_sample_studio_tex" ios/AetherApp/MetalRenderer.swift || fail "missing metal batch14 encode"
grep -q "assisted vs\|get_event_ex" ios/AetherApp/ClassicScoreboardChatOverlay.swift || fail "missing assist HUD polish"
grep -q "gpu-hiz-mip\|weapon-auth\|~76%\|Hi-Z mip pyramid" README.md || fail "missing batch14 README"
grep -q "smoke_batch_gpu_hiz_mip_weapon_auth_portal" tests/host_smoke.c || fail "missing batch14 smoke"
ok "batch gpu-hiz-mip/weapon-auth/portal API symbols present"


grep -q "aether_depth_hiz_bind_plan_encode\|aether_depth_hiz_bind_plan_was_bound" engine/render/AetherDepthPrepass.h || fail "missing depth→hiz bind plan"
grep -q "aether_mdl_hiz_bind_from_depth\|aether_mdl_hiz_vis_query_multi_mip\|aether_mdl_hiz_pyramid_texture_views" engine/model/AetherModelFixture.h || fail "missing hiz bind/multi-mip"
grep -q "aether_portal_winding_clip\|aether_water_reflect_recursive_plan\|aether_portal_winding_make_rect" engine/render/AetherWater.h || fail "missing portal winding/recursive"
grep -q "aether_mdl_skin_pages_build_fixture\|aether_mdl_skin_page_sample\|aether_water_reflect_ent_bind_skin_page" engine/model/AetherModelFixture.h engine/render/AetherWater.h || fail "missing mdl skin pages"
grep -q "aether_weapon_fire_combat_auth_hitgroup\|aether_weapon_hitgroup_scale\|aether_game_weapon_hit_auth_hitgroup" engine/game/weapons/AetherWeaponFiring.h engine/game/AetherGameManager.h || fail "missing weapon hitgroup auth"
grep -q "macos-14\|Unsigned IPA dry-run\|workflow_dispatch" README.md build/scripts/package_ipa.sh .github/workflows/build-arm64.yml || fail "missing clearer IPA macos-14 dry-run docs"
grep -q "aether_depth_hiz_bind_fragment\|aether_portal_recursive_vertex\|aether_mdl_skin_page_fragment" ios/AetherApp/Shaders.metal || fail "missing Metal depth-hiz/portal/skin hooks"
grep -q "engine_depth_hiz_bind_execute\|engine_water_reflect_recursive_plan\|engine_mdl_skin_pages_build\|engine_game_weapon_hit_auth_hitgroup" ios/AetherApp/EngineBridge.h || fail "missing bridge batch15"
grep -q "engine_depth_hiz_bind_execute\|engine_water_reflect_recursive_plan\|engine_water_reflect_ent_bind_skin_page" ios/AetherApp/MetalRenderer.swift || fail "missing metal batch15 encode"
grep -q "depth-hiz-bind\|portal-winding\|mdl-skin-pages\|~77%" README.md || fail "missing batch15 README"
grep -q "smoke_batch_depth_hiz_bind_portal_winding_mdl_skin_pages" tests/host_smoke.c || fail "missing batch15 smoke"
ok "batch depth-hiz-bind/portal-winding/mdl-skin-pages API symbols present"


grep -q "aether_mdl_hiz_bind_texture2d_array\|aether_mdl_hiz_vis_query_array_mip\|aether_mdl_hiz_array_was_bound" engine/model/AetherModelFixture.h || fail "missing Hi-Z texture2d_array bind"
grep -q "aether_bsp_portal_graph_build_from_bsp\|aether_bsp_portal_graph_flood\|aether_bsp_portal_graph_build_multi_fixture" engine/bsp/AetherBSPVis.h || fail "missing portal leaf graph"
grep -q "aether_water_reflect_portal_graph_plan\|aether_bsp_portal_graph_t" engine/render/AetherWater.h engine/bsp/AetherBSPVis.h || fail "missing portal graph reflect plan"
grep -q "aether_mdl_skin_lumps_load\|aether_mdl_skin_lumps_load_or_fixture\|aether_mdl_skin_lump_sample" engine/model/AetherModelFixture.h || fail "missing packed MDL skin lumps"
grep -q "upload-artifact@v4\|retention-days\|compression-level\|ARTIFACT_NOTES\|ipa-artifact" .github/workflows/build-arm64.yml build/scripts/package_ipa.sh || fail "missing IPA artifact automation notes"
grep -q "aether_depth_hiz_array_bind_encode\|aether_mdl_hiz_vis_query_array_mip" engine/render/AetherDepthPrepass.h engine/model/AetherModelFixture.h || fail "missing array mip vis encode path"
grep -q "aether_hiz_array_vis_query_fragment\|aether_portal_graph_flood_fragment\|aether_mdl_skin_lump_fragment" ios/AetherApp/Shaders.metal || fail "missing Metal hiz-array/portal-graph/skin-lump hooks"
grep -q "engine_mdl_hiz_bind_texture2d_array\|engine_water_reflect_portal_graph_plan\|engine_mdl_skin_lumps_load_or_fixture\|engine_bsp_portal_graph_flood" ios/AetherApp/EngineBridge.h || fail "missing bridge batch16"
grep -q "engine_mdl_hiz_bind_texture2d_array\|engine_water_reflect_portal_graph_plan\|engine_mdl_skin_lumps_load_or_fixture" ios/AetherApp/MetalRenderer.swift || fail "missing metal batch16 encode"
grep -q "hiz-array\|portal-graph\|mdl-skin-ipa\|~78%" README.md || fail "missing batch16 README"
grep -q "smoke_batch_hiz_array_portal_graph_mdl_skin_ipa" tests/host_smoke.c || fail "missing batch16 smoke"
ok "batch hiz-array/portal-graph/mdl-skin-ipa API symbols present"


info "batch hiz-gpu-downsample / portal-windings / mdl-skinref / ipa-sign"
grep -q "aether_mdl_hiz_array_downsample_chain\|aether_mdl_hiz_vis_query_downsampled\|aether_mdl_hiz_array_downsample_ready" engine/model/AetherModelFixture.h || fail "missing Hi-Z GPU array downsample"
grep -q "aether_bsp_portal_windings_from_marksurfaces\|aether_bsp_portal_graph_attach_windings\|aether_bsp_portal_winding_to_render" engine/bsp/AetherBSPVis.h || fail "missing portal windings from marksurfaces"
grep -q "aether_portal_winding_from_bsp\|aether_water_reflect_portal_winding_plan" engine/render/AetherWater.h || fail "missing portal winding reflect plan"
grep -q "aether_mdl_skinref_build_fixture\|aether_mdl_skinref_select_family\|aether_mdl_skinref_resolve" engine/model/AetherModelFixture.h || fail "missing MDL skinref family select"
grep -q "aether_depth_hiz_downsample_bind_encode\|aether_depth_hiz_downsample_vis_ready" engine/render/AetherDepthPrepass.h || fail "missing downsample→vis bind"
grep -q "\-\-dry-run\|DRY_RUN_NOTES\|--sign-check\|--notes-only" build/scripts/package_ipa.sh || fail "missing IPA dry-run polish flags"
grep -q "aether_hiz_array_downsample\|aether_hiz_vis_query_downsampled_fragment\|aether_mdl_skinref_select_fragment\|aether_portal_winding_marksurface_fragment" ios/AetherApp/Shaders.metal || fail "missing Metal downsample/skinref/portal winding hooks"
grep -q "engine_mdl_hiz_array_downsample\|engine_bsp_portal_windings_from_current\|engine_mdl_skinref_select_family\|engine_mdl_hiz_vis_query_downsampled" ios/AetherApp/EngineBridge.h || fail "missing bridge batch17"
grep -q "engine_mdl_hiz_array_downsample\|engine_bsp_portal_windings_from_current\|engine_mdl_skinref_init_fixture\|engine_depth_hiz_downsample_bind" ios/AetherApp/MetalRenderer.swift || fail "missing metal batch17 encode"
grep -q "hiz-gpu-downsample\|portal-windings\|mdl-skinref\|~79%" README.md || fail "missing batch17 README"
grep -q "smoke_batch_hiz_gpu_downsample_portal_windings_mdl_skinref_ipa_sign" tests/host_smoke.c || fail "missing batch17 smoke"
# Linux-safe IPA dry-run exercise
bash build/scripts/package_ipa.sh --dry-run --sign-check >/dev/null || fail "package_ipa --dry-run failed"
[ -f build/out/DRY_RUN_NOTES.txt ] || fail "missing DRY_RUN_NOTES.txt after --dry-run"
ok "batch hiz-gpu-downsample/portal-windings/mdl-skinref/ipa-sign API symbols present"




info "batch hiz-gpu-encode / portal-clip / studio-skinref-remap / ipa-dispatch"
grep -q "aether_mdl_hiz_encode_from_depth\|aether_mdl_hiz_live_encode_plan\|aether_mdl_hiz_live_encode_was_encoded" engine/model/AetherModelFixture.h || fail "missing Hi-Z live encode from depth"
grep -q "aether_depth_hiz_live_encode_plan\|aether_depth_hiz_live_encode_needed" engine/render/AetherDepthPrepass.h || fail "missing depth Hi-Z live encode plan"
grep -q "aether_portal_winding_clip_reflect_planes\|aether_portal_winding_clip_planes\|aether_water_reflect_portal_clip_plan" engine/render/AetherWater.h || fail "missing portal clip reflect planes"
grep -q "aether_mdl_skinref_remap_draw\|aether_mdl_skinref_remap_uv\|aether_mdl_skinref_remap_sample" engine/model/AetherModelFixture.h || fail "missing skinref remap draw"
grep -q "dry_run_validate\|gh workflow run\|workflow_dispatch" .github/workflows/build-arm64.yml README.md || fail "missing IPA dry-run dispatch docs/inputs"
grep -q "aether_hiz_encode_from_depth\|aether_portal_winding_reflect_clip_fragment\|aether_mdl_skinref_remap_fragment" ios/AetherApp/Shaders.metal || fail "missing Metal live encode/portal clip/skinref remap"
grep -q "engine_mdl_hiz_encode_from_depth\|engine_portal_winding_clip_reflect_planes\|engine_mdl_skinref_remap_draw\|engine_water_reflect_portal_clip_plan" ios/AetherApp/EngineBridge.h || fail "missing bridge batch18"
grep -q "engine_mdl_hiz_encode_from_depth\|engine_portal_winding_clip_reflect_planes\|engine_mdl_skinref_remap_draw\|engine_depth_hiz_live_encode_plan" ios/AetherApp/MetalRenderer.swift || fail "missing metal batch18 encode"
grep -q "hiz-gpu-encode\|portal-clip\|skinref-remap\|~80%" README.md || fail "missing batch18 README"
grep -q "smoke_batch_hiz_gpu_encode_portal_clip_studio_skinref_remap_ipa_dispatch" tests/host_smoke.c || fail "missing batch18 smoke"
ok "batch hiz-gpu-encode/portal-clip/studio-skinref-remap/ipa-dispatch API symbols present"
