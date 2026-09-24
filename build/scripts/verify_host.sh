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
