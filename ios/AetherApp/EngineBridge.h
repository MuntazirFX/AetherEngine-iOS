// EngineBridge.h — AetherEngine-iOS · Clean-room.
#ifndef ENGINE_BRIDGE_H
#define ENGINE_BRIDGE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Engine lifecycle ---------- */
void engine_init(const char *base_path, const char *asset_path);
void engine_shutdown(void);
/* Host frame: tick engine subsystems (incl. game registry) + audio flush. */
void engine_host_frame(float dt);
int  engine_is_running(void);
unsigned long long engine_frame_count(void);
double engine_elapsed(void);
float  engine_last_dt(void);

/* ---------- Game lifecycle / 5-game selection ---------- */
void engine_launch_game(const char *game_dir);
void engine_stop_game(void);
int  engine_game_count(void);
/* Fill display/dir/start_map; returns 1 on success. */
int  engine_game_info(int index, char *name, int name_cap,
                      char *dir, int dir_cap, char *start_map, int map_cap);
int  engine_game_select(const char *game_dir);
const char *engine_game_active_dir(void);
const char *engine_game_start_map(void);
int  engine_game_state(void); /* see aether_game_state_t */
int  engine_game_data_present(const char *game_dir);
unsigned long long engine_game_run_frames(void);
/* Load bundled/repo manifests (host path or Documents/manifests). Returns count. */
int  engine_manifest_load_all(const char *dir_path);
int  engine_manifest_count(void);

/* ---------- Input ---------- */
void engine_input_set_move(float x, float y);
void engine_input_add_look(float dx, float dy);
void engine_input_set_action(const char *action_name, bool pressed);

/* ---------- Player ---------- */
void  engine_player_spawn_at_mesh_center(void);
void  engine_player_spawn_at_start(void);          /* NEW */
bool  engine_player_has_start(void);                /* NEW */
void  engine_player_tick(float dt);
void  engine_player_get_eye(float out_xyz[3]);
void  engine_player_get_forward(float out_xyz[3]);
void  engine_player_get_position(float out_xyz[3]);
void  engine_player_set_position(float x, float y, float z);
void  engine_player_set_angles(float yaw, float pitch);
float engine_player_get_yaw(void);
float engine_player_get_pitch(void); /* includes view punch */
float engine_player_view_punch_pitch(void);
float engine_player_fall_velocity(void);
int   engine_player_on_ground(void);
float engine_player_get_step_height(void);
void  engine_player_set_step_height(float height);
int   engine_player_is_crouching(void);
void  engine_player_set_crouching(bool crouching);
int   engine_player_hull_index(void);
float engine_player_eye_height(void);
int   engine_player_in_water(void);
/* Waterlevel: 0=dry, 1=feet/wade, 2=waist/swim, 3=eye/under. */
int   engine_player_waterlevel(void);
int   engine_player_eye_underwater(void);
float engine_player_air(void);
float engine_player_air_max(void);
int   engine_player_is_drowning(void);
/* Pending splash: 0=none, 1=enter, 2=exit. Consumes the event. */
int   engine_player_take_splash(void);
/* Queue splash + optionally spawn particle burst at (x,y,z). count<=0 → default 24. */
void  engine_player_trigger_splash(int splash_kind);
int   engine_player_splash_burst(float x, float y, float z, int count);

/* ---------- Collision (clipnode hull) ---------- */
int  engine_collision_ready(void);
int  engine_collision_clipnode_count(void);
int  engine_collision_hull_root(int hull_index);
/* Leaf contents: -1 empty, -2 solid, -3 water (see AETHER_CONTENTS_*). */
int  engine_collision_point_contents(float x, float y, float z, int hull_index);
/* 1 if point is solid for hull 1 (standing) or 2 (crouch). */
int  engine_collision_point_in_solid(float x, float y, float z, int hull_index);
/* Axis move-and-slide + optional step-up (max_step<=0 disables).
 * Writes corrected xyz; returns on_ground (1/0). */
int  engine_collision_move(float from_x, float from_y, float from_z,
                           float to_x, float to_y, float to_z,
                           int hull_index, float max_step, float out_xyz[3]);

/* ---------- HUD ---------- */
float engine_hud_health(void);
float engine_hud_max_health(void);
float engine_hud_armor(void);
float engine_hud_battery(void);
bool  engine_hud_alive(void);
/* Air / drown meter (HEV-style readout; mirrors player air state). */
float engine_hud_air(void);
float engine_hud_air_max(void);
int   engine_hud_drowning(void);
int   engine_hud_active_weapon(void);
int   engine_hud_reserve_ammo(void);
int   engine_hud_clip(void);
int   engine_hud_clip_max(void);
void  engine_hud_set_clip(int clip, int clip_max);
void  engine_hud_set_crosshair_style(int style);
void  engine_hud_set_crosshair_spread(float spread);
float engine_hud_crosshair_spread(void);
void  engine_hud_give_demo_loadout(void);

/* ---------- Settings / CVars / Audio / Renderer ---------- */
void engine_settings_save(const char *filepath);
void engine_settings_load(const char *filepath);
/* Default path: <base>/aether.cfg — returns 1 on success. */
int  engine_settings_save_default(void);
int  engine_settings_load_default(void);
void engine_settings_apply(void); /* push settings → audio/cvars/player look */
int  engine_settings_set_float(const char *key, float v);
int  engine_settings_set_int(const char *key, int v);
int  engine_settings_set_bool(const char *key, bool v);
float engine_settings_get_float(const char *key, float fallback);
int   engine_settings_get_int(const char *key, int fallback);
bool  engine_settings_get_bool(const char *key, bool fallback);

int  engine_cvar_set(const char *name, const char *value);
float engine_cvar_float(const char *name, float fallback);
int   engine_cvar_int(const char *name, int fallback);
bool  engine_cvar_bool(const char *name, bool fallback);

void engine_audio_init(void);
void engine_audio_shutdown(void);
int  engine_audio_ready(void);
void engine_audio_flush(void);
void engine_audio_set_master_volume(float vol);
void engine_audio_set_mute(bool muted);
void engine_audio_play(const char *asset_path, float volume, bool loop);
void engine_audio_stop_all(void);

/* Filesystem diagnostics (no assets bundled). */
int  engine_fs_root_count(void);
int  engine_fs_root_at(int index, char *out, int out_cap);
int  engine_fs_exists(const char *vpath);
void engine_renderer_attach_metal(void *mtkView);
void engine_renderer_resize(unsigned int width, unsigned int height);
void engine_renderer_begin_frame(void);
/* Begin frame and tick render features with the real frame delta (seconds). */
void engine_renderer_begin_frame_dt(float dt);
/* Push column-major 4x4 view/proj from Metal/Swift into the C renderer. */
void engine_renderer_set_camera(const float view16[16], const float proj16[16]);
/* Read back the last camera matrices (column-major). out may be NULL. */
void engine_renderer_get_view(float out16[16]);
void engine_renderer_get_proj(float out16[16]);
/* Submit world/HUD/feature draw commands through the active backend vtable. */
void engine_renderer_draw_world(void);
void engine_renderer_draw_hud(void);
void engine_renderer_draw_feature(int feature_cmd);
void engine_renderer_tick_features(float dt);
void engine_renderer_end_frame(void);

/* ---------- Particles (render-feature pool) ---------- */
/* Seed a burst around world origin (x,y,z). Returns spawned count. */
int   engine_particles_spawn_burst(float x, float y, float z, int count);
int   engine_particles_active_count(void);
/* Copy active particles as flat floats: [x,y,z,size,r,g,b,a] * N. Returns N. */
int   engine_particles_copy_render(float *out_xyz_size_rgba, int max_particles);
void  engine_particles_clear(void);
/* AETHER_CMD_DRAW_PARTICLES = 10 — submit feature draw through backend. */
#define ENGINE_CMD_DRAW_PARTICLES 10

/* ---------- Sky (render-feature placeholder dome) ---------- */
int   engine_sky_enabled(void);
void  engine_sky_set_enabled(bool enabled);
int   engine_sky_face_count(void);
float engine_sky_radius(void);
void  engine_sky_set_radius(float radius);
int   engine_sky_set_name(const char *name);
int   engine_sky_get_name(char *out, int out_cap);
/* Copy sky dome verts as flat floats: [x,y,z,r,g,b,a] * N. Returns N. */
int   engine_sky_copy_render(float *out_xyz_rgba, int max_vertices);
int   engine_sky_render_vertex_capacity(void);
/* AETHER_CMD_DRAW_SKY = 7 — submit feature draw through backend. */
#define ENGINE_CMD_DRAW_SKY 7

/* ---------- Water (render-feature animated plane) ---------- */
int   engine_water_enabled(void);
void  engine_water_set_enabled(bool enabled);
float engine_water_wave_time(void);
float engine_water_opacity(void);
void  engine_water_set_color(float r, float g, float b, float a);
void  engine_water_set_size(float size);
void  engine_water_set_height(float height);
void  engine_water_set_origin(float x, float y);
void  engine_water_set_wave(float speed, float amp, float freq);
/* Copy water verts as flat floats: [x,y,z,u,v,r,g,b,a] * N. Returns N. */
int   engine_water_copy_render(float *out_xyz_uv_rgba, int max_vertices);
int   engine_water_render_vertex_capacity(void);
/* AETHER_CMD_DRAW_WATER = 6 — submit feature draw through backend. */
#define ENGINE_CMD_DRAW_WATER 6


/* ---------- Fog (render-feature fullscreen tint) ---------- */
int   engine_fog_enabled(void);
void  engine_fog_set_enabled(bool enabled);
float engine_fog_density(void);
float engine_fog_factor(void);
float engine_fog_start(void);
float engine_fog_end(void);
void  engine_fog_set_density(float density);
void  engine_fog_set_factor(float factor);
void  engine_fog_set_range(float start, float end);
void  engine_fog_set_color(float r, float g, float b, float a);
void  engine_fog_get_color(float out_rgba[4]);
/* Copy fog fullscreen verts as flat floats: [x,y,u,v,r,g,b,a] * N. Returns N. */
int   engine_fog_copy_render(float *out_xy_uv_rgba, int max_vertices);
int   engine_fog_render_vertex_capacity(void);
/* AETHER_CMD_DRAW_FOG = 8 — submit feature draw through backend. */
#define ENGINE_CMD_DRAW_FOG 8


/* ---------- BSP ---------- */
int  engine_bsp_inspect(const char *bsp_path);
int  engine_bsp_inspect_vfs(const char *vpath);
int  engine_bsp_inspect_vfs_text(const char *vpath, char *out_buf, int out_cap);
int  engine_bsp_mesh_build(const char *vpath);
int  engine_bsp_mesh_vertex_count(void);
int  engine_bsp_mesh_index_count(void);
int  engine_bsp_mesh_triangle_count(void);
void engine_bsp_mesh_get_bounds(float out_min[3], float out_max[3], float out_center[3]);
int  engine_bsp_mesh_copy_vertices(float *out, int max_vertices);
int  engine_bsp_mesh_copy_indices(uint32_t *out, int max_indices);
void engine_bsp_mesh_release(void);
/* Synthetic demo room (no copyrighted .bsp). Builds mesh + spawns entities. */
int  engine_bsp_mesh_build_synthetic(void);
/* Try vpath; on failure fall back to synthetic demo room. */
int  engine_bsp_mesh_build_or_synthetic(const char *vpath);
/* 1 if the active mesh came from aether_bsp_create_synthetic_room. */
int  engine_bsp_mesh_is_synthetic(void);
#define ENGINE_CMD_DRAW_WORLD 4

/* ---------- BSP VIS / leaf culling ---------- */
/* View origin used for leaf find (typically player eye). */
void engine_bsp_vis_set_view_origin(float x, float y, float z);
void engine_bsp_vis_get_view_origin(float out_xyz[3]);
/* Debug: when set, cull treats every face as visible. */
void engine_bsp_vis_set_force_full(bool enabled);
int  engine_bsp_vis_force_full(void);
/* 0=USE_PVS, 1=CURRENT_LEAF_ONLY, 2=FORCE_FULL (see aether_bsp_vis_mode_t). */
void engine_bsp_vis_set_mode(int mode);
int  engine_bsp_vis_mode(void);
/* Leaf containing the current view origin (-1 if unknown / no tree). */
int  engine_bsp_vis_find_leaf(void);
int  engine_bsp_vis_find_leaf_at(float x, float y, float z);
/* Recompute culled index list from active BSP+mesh + view origin. Returns visible index count. */
int  engine_bsp_vis_update(void);
int  engine_bsp_vis_visible_index_count(void);
int  engine_bsp_vis_visible_face_count(void);
int  engine_bsp_vis_total_face_count(void);
int  engine_bsp_vis_visible_leaf_count(void);
int  engine_bsp_vis_copy_indices(uint32_t *out, int max_indices);

/* ---------- Lightmap (procedural stub atlas) ---------- */
int  engine_lightmap_enabled(void);
void engine_lightmap_set_enabled(bool enabled);
int  engine_lightmap_width(void);
int  engine_lightmap_height(void);
int  engine_lightmap_is_stub(void);
int  engine_lightmap_copy_rgba(unsigned char *out, int max_bytes);
/* Rebuild procedural stub + assign mesh lightmap UVs for the active BSP mesh. */
int  engine_lightmap_bake_active_mesh(void);

/* ---------- Texture atlas ---------- */
int  engine_texture_dump_wad(const char *wad_vpath);
int  engine_texture_dump_bsp_miptex(void);
int  engine_texture_summary_text(char *out_buf, int out_cap);
int  engine_texture_build_atlas(void);
int  engine_texture_atlas_width(void);
int  engine_texture_atlas_height(void);
int  engine_texture_atlas_slot_count(void);
int  engine_texture_atlas_copy_rgba(unsigned char *out, int max_bytes);

/* ---------- MDL ---------- */
int  engine_mdl_dump_vfs(const char *mdl_vpath);
int  engine_mdl_summary_text(const char *mdl_vpath, char *out_buf, int out_cap);
int  engine_mdl_mesh_build(const char *mdl_vpath);
int  engine_mdl_mesh_vertex_count(void);
int  engine_mdl_mesh_triangle_count(void);
void engine_mdl_mesh_get_bounds(float out_min[3], float out_max[3], float out_center[3]);
int  engine_mdl_mesh_copy_positions(float *out, int max_floats);
int  engine_mdl_mesh_copy_normals(float *out, int max_floats);
int  engine_mdl_mesh_copy_indices(uint32_t *out, int max_indices);
void engine_mdl_mesh_release(void);
void engine_mdl_mesh_get_render_pos(float out_xyz[3]);
void engine_mdl_mesh_set_render_pos(float x, float y, float z);

/* ---------- Entity diagnostics ---------- */
int  engine_entity_dump_current_map(void);
int  engine_entity_summary_text(char *out_buf, int out_cap);

/* ---------- Entity spawning (STEP 18B) ---------- */
int  engine_entity_spawn_current_map(void);            /* NEW */
int  engine_entity_count(void);                        /* NEW */
int  engine_entity_monster_count(void);                /* NEW */
int  engine_entity_alive_monster_count(void);          /* NEW */

/* Monster positions for rendering */
int  engine_monster_positions_copy(float *out_xyz_flat, int max_monsters);  /* NEW */
int  engine_monster_healths_copy(int *out_health, int max_monsters);        /* NEW */

/* ---------- Scoreboard / Chat ---------- */
void engine_scoreboard_init(void);
void engine_scoreboard_set_visible(bool visible);
bool engine_scoreboard_visible(void);
int engine_scoreboard_count(void);
int engine_scoreboard_get_entry(int index, char *name, int name_cap, int *score, int *deaths, int *ping);
void engine_scoreboard_demo_data(void);
void engine_chat_init(void);
void engine_chat_set_visible(bool visible);
bool engine_chat_visible(void);
int engine_chat_count(void);
int engine_chat_get_line(int index, char *text, int text_cap, unsigned int *player_id);
void engine_chat_add_text(const char *text);

/* ---------- VGUI / classic menu ---------- */
int  engine_vgui_init(void);
void engine_vgui_shutdown(void);
void engine_vgui_show_main(void);
void engine_vgui_show_options(void);
void engine_vgui_show_load_game(void);
void engine_vgui_show_multiplayer(void);
void engine_vgui_toggle_console(void);
bool engine_console_visible(void);
void engine_console_set_visible(bool visible);
int engine_console_execute(const char *line);
int engine_console_count(void);
int engine_console_get_line(int index, char *text, int text_cap, int *level);
const char *engine_console_input(void);
void engine_console_set_input(const char *text);
bool engine_vgui_is_visible(void);
int  engine_vgui_current_panel_text(char *out_buf, int out_cap);
int  engine_vgui_item_count(void);
int  engine_vgui_item_text(int index, char *out_buf, int out_cap);
int  engine_vgui_item_type(int index);
int  engine_vgui_activate_item(int index);
int  engine_vgui_new_game(void);

/* ---------- Map load (FS → synthetic) ---------- */
int  engine_map_load(const char *vpath);           /* 1=ok; uses FS then synthetic */
int  engine_map_load_named(const char *map_name);
int  engine_map_last_source(void);                 /* 1=file, 2=synthetic, 0=none */
int  engine_map_write_fixture(const char *abspath);/* host/CI fixture writer */

/* ---------- Audio platform / beep / WAV ---------- */
void engine_audio_set_platform_callback(void *fn, void *user); /* optional; Swift */
int  engine_audio_play_beep(float freq_hz, float duration_sec, float volume);
int  engine_audio_submit_pcm16(const short *samples, int frames,
                               int sample_rate, int channels, float volume);
int  engine_wav_parse_header(const unsigned char *data, int size,
                             unsigned *out_rate, unsigned *out_channels,
                             unsigned *out_bits, unsigned *out_data_bytes);

/* ---------- Decals / dynamic lights (Metal slice) ---------- */
int  engine_decals_add(float x, float y, float z,
                       float nx, float ny, float nz, float size, float life);
int  engine_decals_active_count(void);
int  engine_decals_copy_render(float *out_xyz_n_size_fade, int max_decals);
#define ENGINE_CMD_DRAW_DECALS 9

int  engine_dynlights_add(float x, float y, float z,
                          float r, float g, float b, float radius, float intensity);
int  engine_dynlights_active_count(void);
int  engine_dynlights_copy_render(float *out_xyz_radius_rgb_i, int max_lights);
int  engine_dynlights_from_map_lights(void); /* spawn dynlights from light entities */

/* ---------- Save / load smoke (host + bridge) ---------- */
int  engine_save_game(const char *filepath);
int  engine_load_game(const char *filepath);

/* ---------- Net listen/connect smoke ---------- */
int  engine_net_listen(int port);
/* ---------- Batch: UV/WAV/decals/net/hazards ---------- */
int  engine_audio_play_wav_data(const unsigned char *data, int size, float volume);
int  engine_decals_copy_quads(float *out_xyz_uv_fade_rgba, int max_verts);
int  engine_sprite_copy_quad(float x, float y, float z, float w, float h,
                             float *out_xyz_uv_rgba, int max_verts);
int  engine_dynlights_apply_mesh_tint(float *out_rgb, int max_floats);
int  engine_dynlights_modulate_lightmap(void);
int  engine_net_snapshot_demo_apply(unsigned tick);
int  engine_net_snapshot_scoreboard_count(void);
int  engine_player_set_on_fire(int on);
int  engine_player_is_on_fire(void);
int  engine_player_set_in_radiation(int on);
int  engine_player_is_in_radiation(void);
int  engine_lightmap_unpack_uvs_active(void);

int  engine_net_connect_localhost(int port);
int  engine_net_handshake_tick(float dt); /* pump server+client; 1 if connected */
int  engine_net_is_connected(void);
void engine_net_shutdown(void);

/* Lightmap: prefer BSP lighting lump when present */
int  engine_lightmap_bake_from_active_bsp(void);


/* ---------- Batch: GPU lights / decal clip / netplay / styles / fixtures ---------- */
int  engine_dynlights_fill_ubo(float *out_bytes, int max_floats); /* packs UBO as floats */
int  engine_dynlights_ubo_count(void);
int  engine_decals_project_onto_mesh(float *out_xyz_uv_fade_rgba, int max_verts);
int  engine_net_snapshot_live_tick(float dt); /* UDP ingest → HUD; returns snap count */
int  engine_net_snapshot_ingested_count(void);
int  engine_lightstyles_update(float time);
float engine_lightstyles_value(unsigned index);
int  engine_lightmap_apply_style(unsigned style_index);
int  engine_mdl_write_fixture(const char *filepath);
int  engine_sprite_write_fixture(const char *filepath);
int  engine_mdl_load_fixture_file(const char *filepath); /* load+build mesh for Metal */
int  engine_sprite_fixture_quad(float x, float y, float z, float w, float h,
                                float *out_xyz_uv_rgba, int max_verts);
int  engine_shadow_copy_blob(float px, float py, float ground_z, float radius,
                             float *out_xyz_uv_alpha_pad, int max_verts);
int  engine_postfx_set_from_settings(void);
int  engine_postfx_copy_fullscreen(float *out_xyz_uv, int max_verts);
float engine_postfx_brightness(void);
float engine_postfx_gamma(void);
int  engine_interact_trace(float eye_x, float eye_y, float eye_z,
                           float yaw_deg, float pitch_deg, float max_dist,
                           float *out_hit_xyz, char *out_classname, int classname_cap);


/* ---------- Batch: postfx offscreen / lightmap pingpong / delta+predict ---------- */
int  engine_postfx_ensure_offscreen(int width, int height);
int  engine_postfx_has_offscreen(void);
int  engine_postfx_fill_uniforms(float *out4); /* brightness,gamma,exposure,enabled */
int  engine_lightmap_capture_base(void);
int  engine_lightmap_apply_style_pingpong(unsigned style_index);
int  engine_lightmap_has_base(void);
int  engine_dynlights_fill_array(float *out, int max_floats);
int  engine_decals_clip_to_world(float *out_xyz_uv_fade_rgba, int max_verts);
int  engine_net_delta_encode(const void *baseline_snap, const void *current_snap,
                             unsigned char *out, int cap);
int  engine_net_delta_apply(const unsigned char *data, int size);
int  engine_net_interp_push_demo(unsigned tick, float time, float frac);
int  engine_net_interp_origin(unsigned player_id, float out[3]);
int  engine_net_predict_local_step(float forward, float side, float yaw_deg, float dt);
int  engine_net_predict_reconcile_demo(float blend);
int  engine_net_predict_get_origin(float out[3]);
int  engine_mdl_fixture_extract_verts(void); /* write fixture, extract; returns vertex_count */

/* ---------- Utility ---------- */
const char *engine_base_path(void);
const char *engine_version(void);

/* ---------- Batch: GPU lightstyles / skin / mp cmds / bloom / decal atlas ---------- */
int  engine_lightstyles_fill_gpu_weights(float *out_weights64, unsigned *out_count);
int  engine_mdl_skin_build_stub(unsigned bone_count, float time, float sway_deg);
int  engine_mdl_skin_fill_ubo(float *out, int max_floats);
int  engine_mdl_skin_transform_point(unsigned bone, float weight,
                                     const float in3[3], float out3[3]);
int  engine_mdl_write_textured_fixture(const char *filepath);
int  engine_mdl_fixture_texture_rgba(unsigned char *out, int cap, int *out_w, int *out_h);
int  engine_postfx_set_bloom_chain(float threshold, float intensity, float blur_radius);
int  engine_postfx_fill_uniforms_ex(float *out8);
int  engine_postfx_fill_bloom(float *out4); /* threshold,intensity,blur,enabled */
int  engine_decal_atlas_generate(void);
int  engine_decal_atlas_copy_rgba(unsigned char *out, int max_bytes);
int  engine_decal_atlas_sample(float u, float v, float out_rgb[3]);
int  engine_net_client_send_input(float forward, float side, float yaw_deg,
                                  float pitch_deg, unsigned buttons, float dt);
int  engine_net_client_live_tick(float dt, float forward, float side, float yaw_deg,
                                 unsigned buttons, float out_origin[3]);
int  engine_net_server_tick_authority(float dt);
int  engine_net_server_build_snapshot_players(void);
int  engine_net_lagcomp_cmd_seq(unsigned player_id, float lag_ms);

/* ---------- Batch: seq skin / per-face styles / spatial audio / HUD / predict+clip ---------- */
int  engine_mdl_skin_build_from_sequence(float frame);
int  engine_mdl_skin_mesh(const unsigned char *bone_indices, const float *weights,
                          const float *in_xyz, float *out_xyz, unsigned vert_count);
int  engine_mdl_write_seq_fixture(const char *filepath);
int  engine_lightmap_fill_face_style_indices(unsigned char *out, unsigned max_faces);
int  engine_lightmap_fill_face_style_weights(float *out, unsigned max_faces);
int  engine_audio_set_listener(float x, float y, float z, float fx, float fy, float fz);
int  engine_audio_spatial_atten(float sx, float sy, float sz, float ref_d, float max_d,
                                float *out_gain_pan_dist3);
int  engine_audio_play_beep_at(float freq, float dur, float vol, float sx, float sy, float sz);
int  engine_hud_layout_classic_pack(float *out24);
int  engine_hud_layout_apply_classic(void);
int  engine_net_predict_set_collision_from_bsp(void);
int  engine_net_predict_apply_cmd_clipped(float forward, float side, float yaw_deg, float dt);
int  engine_weapon_view_copy_stub(float *out_xyz_uv_rgba, int max_verts);
int  engine_monster_ai_tick_frame(float dt);
int  engine_postfx_bloom_encode_needed(void);

#ifdef __cplusplus
}
#endif

/* ---------- Batch: studio anim / multi-style / stereo / lagcomp / pvs-lights ---------- */
int  engine_mdl_write_studio_fixture(const char *filepath);
int  engine_mdl_sequence_load_studio(float frame); /* load fixture→seq→skin at frame */
int  engine_lightmap_fill_face_style_blend(float *out_weights4, unsigned max_faces);
int  engine_audio_play_beep_stereo_at(float freq, float dur, float vol,
                                      float sx, float sy, float sz);
int  engine_audio_spatial_stereo_gains(float pan, float *out_l, float *out_r);
int  engine_weapon_view_copy_mdl_fixture(float *out_xyz_uv_rgba, int max_verts);
int  engine_lagcomp_push_demo(float time, int id, float *mins3, float *maxs3);
int  engine_lagcomp_query(float time, int id, float *out_mins3, float *out_maxs3);
int  engine_dynlights_fill_ubo_pvs(float view_x, float view_y, float view_z,
                                   float *out_array, int max_floats);
int  engine_mdl_hitbox_trace_fixture(float ox, float oy, float oz,
                                     float dx, float dy, float dz, float max_dist,
                                     int *out_index, float *out_t);
int  engine_particles_spawn_muzzle(float ox, float oy, float oz,
                                   float fx, float fy, float fz, unsigned count);
int  engine_particles_spawn_trail(float x0, float y0, float z0,
                                  float x1, float y1, float z1, unsigned count);

/* ---------- Batch: metal blend / studio attach / lagcomp hit / RLE ---------- */
int  engine_lightmap_fill_style_blend_ubo(float *out, unsigned max_floats);
int  engine_lightmap_sample_style_blend(const float weights4[4],
                                        const float base_rgb[3], float out_rgb[3]);
int  engine_weapon_view_copy_skinned(float frame, float *out_xyz_uv_rgba, int max_verts,
                                     float *out_muzzle3, float *out_muzzle_fwd3);
int  engine_lagcomp_validate_hit(float now, float lag_ms,
                                 float eye_x, float eye_y, float eye_z, float max_dist,
                                 int *out_id, float *out_t);
int  engine_mdl_anim_rle_decode_fixture(float frame); /* load RLE→skin */
int  engine_dynlights_fill_ubo_pvs_bleed(float view_x, float view_y, float view_z,
                                         float *out_array, int max_floats);
int  engine_particles_spawn_viewmodel_fire(float mx, float my, float mz,
                                           float fx, float fy, float fz,
                                           unsigned muzzle_n, unsigned trail_n);
int  engine_mdl_studio_events_tick(float prev_frame, float frame,
                                   int *out_event, char *out_opts, int opts_cap);
int  engine_audio_play_studio_cue(const char *cue, float volume);
int  engine_postfx_bloom_encode_plan(unsigned *out_passes, unsigned *out_w, unsigned *out_h,
                                     int *out_separable);
int  engine_mdl_fixture_attachments_count(void);
int  engine_lagcomp_push_attack_cmd(float now, float yaw, float pitch, unsigned seq);

 /* ENGINE_BRIDGE_H */

/* ---------- Batch: face-id / bone lagcomp / portal flood / attach chain ---------- */
int  engine_mesh_validate_face_ids(void);
int  engine_lightmap_fill_style_blend_draw(float *out, unsigned max_floats, unsigned *out_face_count);
int  engine_lightmap_sample_style_blend_face(const float *draw_ubo, unsigned float_count,
                                             unsigned face_id,
                                             const float base_rgb[3], float out_rgb[3]);
int  engine_lagcomp_studio_push_demo(float time, int id,
                                     const float *bone_mats, unsigned bone_count,
                                     const float *hitbox_mins3, const float *hitbox_maxs3,
                                     int hitbox_bone, unsigned hitbox_count);
int  engine_lagcomp_studio_trace(float time,
                                 float ox, float oy, float oz,
                                 float dx, float dy, float dz, float max_dist,
                                 int *out_id, int *out_hitbox, float *out_t);
int  engine_dynlights_fill_ubo_portal_flood(float view_x, float view_y, float view_z,
                                            unsigned max_hops,
                                            float *out_array, int max_floats);
int  engine_mdl_attachment_chain_world(float *out_pos3, float *out_fwd3);
int  engine_particles_sync_muzzle_world(float vm_x, float vm_y, float vm_z,
                                        float vf_x, float vf_y, float vf_z,
                                        unsigned particle_count,
                                        float *out_world_pos3);
int  engine_inv_cycle(int dir);
int  engine_inv_apply_weapon_input(void);
int  engine_inv_current_weapon(void);


/* ---------- Batch: studio LOD / water reflect / netscore / predict smooth / depth ---------- */
int  engine_mdl_write_lod_fixture(const char *filepath);
int  engine_mdl_lod_select(float distance); /* returns lod index; loads fixture table */
int  engine_mdl_lod_tri_count(int lod);
int  engine_bodygroup_init_fixture(void);
int  engine_bodygroup_set(unsigned part, unsigned sub);
int  engine_bodygroup_get(unsigned part);
int  engine_bodygroup_cycle(unsigned part, int dir);
int  engine_bodygroup_tri_total(void);
int  engine_bodygroup_apply_lod(float distance);
int  engine_bodygroup_apply_input(void); /* BODYGROUP_NEXT */

int  engine_water_reflect_compute(float eye_x, float eye_y, float eye_z);
int  engine_water_reflect_fill_uniforms(float *out20); /* 16 mirror + 4 clip */
int  engine_water_reflect_encode_needed(void);
int  engine_water_reflect_point(float ix, float iy, float iz, float *out3);

int  engine_scoreboard_apply_join(unsigned player_id, const char *name);
int  engine_scoreboard_apply_leave(unsigned player_id);
int  engine_scoreboard_event_count(void);
int  engine_scoreboard_get_event(int index, int *out_kind, unsigned *out_id,
                                 char *name, int name_cap, float *out_time);
int  engine_scoreboard_handle_packet(const unsigned char *data, unsigned size, float time);
int  engine_net_broadcast_join_demo(unsigned player_id, const char *name);
int  engine_net_broadcast_leave_demo(unsigned player_id);

int  engine_net_predict_set_error_decay(float rate);
float engine_net_predict_smooth_tick(float dt);
float engine_net_predict_error_length(void);
int  engine_net_predict_reconcile_smooth(float snap_ox, float snap_oy, float snap_oz,
                                         float snap_blend, float dt);

int  engine_depth_prepass_ensure(unsigned w, unsigned h);
int  engine_depth_prepass_encode_plan(unsigned *out_passes, unsigned *out_w, unsigned *out_h,
                                      int *out_write_depth);
int  engine_depth_prepass_record_stub(const float *positions_xyz, unsigned count,
                                      float near_z, float far_z,
                                      float *out_depths, unsigned max_out);
int  engine_depth_prepass_encode_needed(void);
int  engine_console_exec_bodygroup(const char *line); /* "bodygroup <part> [sub|next|prev]" */

/* ---------- Batch: reflect RT / studio skin / MP score / chat cue / kill / teleport / depth bind ---------- */
int  engine_water_reflect_rt_ensure(unsigned fb_w, unsigned fb_h, float scale);
int  engine_water_reflect_rt_encode_plan(unsigned *out_passes, unsigned *out_w, unsigned *out_h,
                                         int *out_allocate, int *out_sample);
int  engine_water_reflect_rt_sample_needed(void);
unsigned engine_water_reflect_rt_tex_stub(void);

int  engine_mdl_lod_extract_by_distance(float distance,
                                        float *out_pos, unsigned max_verts,
                                        unsigned *out_idx, unsigned max_idx,
                                        unsigned *out_vert_count, unsigned *out_tri_count);
int  engine_mdl_lod_extract_mesh(int lod,
                                 float *out_pos, unsigned max_verts,
                                 unsigned *out_idx, unsigned max_idx,
                                 unsigned *out_vert_count, unsigned *out_tri_count);

int  engine_texgroup_init_fixture(void);
int  engine_texgroup_set(unsigned group, unsigned tex);
int  engine_texgroup_get(unsigned group);
int  engine_texgroup_cycle(unsigned group, int dir);
int  engine_texgroup_select_group(unsigned group);
int  engine_console_exec_skin(const char *line);

int  engine_net_score_sync_demo(unsigned player_id, int score, int deaths);
int  engine_chat_encode_send_demo(unsigned player_id, const char *text);
int  engine_chat_encode_voice_demo(unsigned player_id, const char *cue);
int  engine_chat_last_cue_kind(void);
int  engine_scoreboard_apply_kill(unsigned killer_id, const char *killer_name,
                                  unsigned victim_id, const char *victim_name);
int  engine_net_broadcast_kill_demo(unsigned killer_id, const char *killer_name,
                                    unsigned victim_id, const char *victim_name);

int  engine_net_predict_set_teleport_threshold(float units);
float engine_net_predict_get_teleport_threshold(void);
int  engine_net_predict_reconcile_teleport(float snap_ox, float snap_oy, float snap_oz,
                                           float soft_blend);
int  engine_net_predict_did_teleport(void);

int  engine_depth_prepass_bind_before_main(void);
int  engine_depth_prepass_mark_bound(void);
int  engine_depth_prepass_was_bound_before_main(void);


/* ---------- Batch: mirror-RT / multi-mesh LOD / MP kill-score / depth cam / spectator / IPA docs ---------- */
int  engine_water_reflect_rt_draw_plan(float *out_mvp16, unsigned *out_w, unsigned *out_h,
                                       int *out_clear, int *out_draw, int *out_resolve);
int  engine_water_reflect_rt_clear(float r, float g, float b, float a);
int  engine_water_reflect_rt_resolve(void);
int  engine_water_reflect_rt_gen_mips(void);
int  engine_water_reflect_rt_was_cleared(void);
int  engine_water_reflect_rt_was_resolved(void);
unsigned engine_water_reflect_rt_mip_levels(void);
int  engine_water_reflect_rt_build_mirror_mvp(const float *view16, const float *proj16,
                                             float *out_mvp16);

int  engine_mdl_lod_mesh_init_fixture(void);
int  engine_mdl_lod_mesh_select(float distance, unsigned *out_vert_count, unsigned *out_tri_count,
                                int *out_lod);
int  engine_mdl_lod_mesh_copy_selected(float *out_pos, unsigned max_verts,
                                       unsigned *out_idx, unsigned max_idx,
                                       unsigned *out_vert_count, unsigned *out_tri_count);

int  engine_net_server_tick_authority_kill_score(float dt, unsigned killer_id, unsigned victim_id,
                                                 unsigned *out_snaps, unsigned *out_kills,
                                                 unsigned *out_scoreboards, unsigned *out_reached);
int  engine_net_server_fanout_scores(void);

int  engine_depth_prepass_camera_set(const float *view16, const float *proj16,
                                     float eye_x, float eye_y, float eye_z);
int  engine_depth_prepass_camera_fill_mvp(float *out_mvp16);
int  engine_depth_prepass_camera_valid(void);
int  engine_depth_prepass_encode_plan_ex(unsigned *out_passes, unsigned *out_w, unsigned *out_h,
                                         int *out_write_depth, float *out_mvp16, int *out_has_mvp);

int  engine_spectator_init(void);
int  engine_spectator_follow(unsigned player_id);
int  engine_spectator_stop(void);
int  engine_spectator_tick(float dt, float tx, float ty, float tz,
                           float fx, float fy, float fz);
int  engine_spectator_get_eye(float *out3);
int  engine_spectator_get_forward(float *out3);
int  engine_spectator_is_following(void);

/* ---------- Batch: reflect-ents / studio-gpu-lod / spec-cycle / dmg-kill / IPA notes ---------- */
int  engine_water_reflect_ent_clear(void);
int  engine_water_reflect_ent_push(unsigned ent_id, int is_monster,
                                   float ox, float oy, float oz,
                                   float hx, float hy, float hz);
int  engine_water_reflect_ent_mark_above(float water_height);
int  engine_water_reflect_rt_draw_plan_full(float *out_mvp16, unsigned *out_w, unsigned *out_h,
                                           int *out_clear, int *out_draw_world,
                                           int *out_draw_ents, int *out_draw_monsters,
                                           unsigned *out_ent_count, unsigned *out_mon_count,
                                           int *out_resolve);
unsigned engine_water_reflect_ent_drawn(void);

int  engine_mdl_lod_gpu_issue_draw(float distance, int *out_lod, unsigned *out_verts,
                                   unsigned *out_tris, int *out_issue);
int  engine_mdl_lod_gpu_issue_draw_copy(float distance,
                                        float *out_pos, unsigned max_verts,
                                        unsigned *out_idx, unsigned max_idx,
                                        int *out_lod, unsigned *out_verts, unsigned *out_tris);

int  engine_spectator_roster_clear(void);
int  engine_spectator_roster_add(unsigned player_id, const char *name);
unsigned engine_spectator_cycle_next(void);
unsigned engine_spectator_cycle_prev(void);
unsigned engine_spectator_target_id(void);
int  engine_spectator_set_cam_mode(int mode); /* 0=follow, 1=copy_eye */
int  engine_spectator_get_cam_mode(void);
int  engine_spectator_hud_visible(void);
int  engine_spectator_hud_indicator(char *out, unsigned cap);

int  engine_player_apply_damage_auth(float amount, unsigned dmg_type,
                                     unsigned killer_id, unsigned victim_id,
                                     int fanout, int *out_died, int *out_registered);
int  engine_net_server_register_assist(unsigned assister_id, unsigned victim_id);
int  engine_net_server_get_assists(unsigned player_id);

/* ---------- Batch: rt-skins / assist-feed / hiz / auth-tick / spec-hp / IPA Actions ---------- */
int  engine_water_reflect_ent_push_studio(unsigned ent_id, int is_monster,
                                          float ox, float oy, float oz,
                                          float hx, float hy, float hz,
                                          int material, unsigned skin_group, unsigned skin_tex,
                                          int attach_index,
                                          float tr, float tg, float tb, float ta);
unsigned engine_water_reflect_studio_count(void);
int  engine_water_reflect_ent_get_studio(unsigned index, int *out_mat,
                                         unsigned *out_sg, unsigned *out_st,
                                         int *out_attach, float *out_tint4);
int  engine_water_reflect_rt_draw_plan_studio_flags(int *out_draw_studio, unsigned *out_studio_count);

int  engine_scoreboard_encode_assist(unsigned char *out, unsigned cap,
                                     unsigned assister_id, const char *assister_name,
                                     unsigned victim_id, const char *victim_name);
int  engine_scoreboard_apply_assist(unsigned assister_id, const char *assister_name,
                                    unsigned victim_id, const char *victim_name);

int  engine_mdl_hiz_init(void);
int  engine_mdl_hiz_push(float depth, float sx, float sy);
int  engine_mdl_lod_hiz_gate(float distance, float aabb_radius,
                             float min_pixels, float max_distance,
                             float sx, float sy, float depth_ndc,
                             int *out_lod, int *out_issue, int *out_occluded,
                             float *out_screen_px);
int  engine_mdl_lod_gpu_issue_draw_hiz(float distance, float aabb_radius,
                                       int *out_lod, unsigned *out_verts,
                                       unsigned *out_tris, int *out_issue,
                                       int *out_occluded);

int  engine_game_bind_auth_server_demo(void); /* creates ephemeral server for smoke/demo */
int  engine_game_auth_queue_damage(unsigned killer_id, unsigned victim_id,
                                   float damage, unsigned dmg_type);
int  engine_game_tick_auth(float dt, int *out_died, int *out_registered);
int  engine_game_has_auth_server(void);

int  engine_spectator_set_target_hp(int hp);
int  engine_spectator_set_target_name(const char *name);
int  engine_spectator_get_target_hp(void);
int  engine_spectator_get_target_name(char *out, unsigned cap);

/* ---------- Batch: gpu-hiz-mip / weapon-auth / portal-reflect / studio-tex / assist polish ---------- */
int  engine_mdl_hiz_pyramid_reset(unsigned mip0_w, unsigned mip0_h);
int  engine_mdl_hiz_pyramid_write(unsigned x, unsigned y, float depth);
int  engine_mdl_hiz_pyramid_fill_mip0(const float *depths, unsigned count);
unsigned engine_mdl_hiz_build_pyramid(void);
int  engine_mdl_hiz_vis_query(float x0, float y0, float x1, float y1, float obj_depth,
                              int *out_visible, int *out_occluded, float *out_hiz, int *out_mip);
int  engine_mdl_hiz_pyramid_set_gpu_hooks(int armed);
int  engine_mdl_hiz_pyramid_gpu_hooks(void);
int  engine_mdl_lod_hiz_pyramid_gate(float distance, float aabb_radius,
                                     float min_pixels, float sx, float sy, float depth_ndc,
                                     int *out_lod, int *out_issue, int *out_occluded,
                                     float *out_screen_px);

int  engine_game_weapon_hit_auth(unsigned weapon_id, float now,
                                 float ox, float oy, float oz,
                                 float dx, float dy, float dz,
                                 unsigned killer_id, unsigned victim_id,
                                 int force_hit,
                                 int *out_fired, int *out_queued, int *out_died,
                                 int *out_registered, float *out_damage);

int  engine_water_reflect_portal_set(float in_x, float in_y, float in_z,
                                     float out_x, float out_y, float out_z,
                                     int eye_crossed);
int  engine_water_reflect_compute_portal(float eye_x, float eye_y, float eye_z);
int  engine_water_reflect_rt_build_mirror_mvp_portal(float *out_mvp16);

int  engine_water_reflect_ent_set_studio_tex(unsigned index, unsigned skin_group, unsigned skin_tex);
int  engine_water_reflect_ent_sample_studio_tex(unsigned index, float u, float v, float *out_rgba4);
int  engine_water_reflect_studio_tex_sample(unsigned skin_group, unsigned skin_tex,
                                            float u, float v, float *out_rgba4);

int  engine_scoreboard_format_assist_line(int event_index, char *out, unsigned cap);
int  engine_scoreboard_get_event_ex(int index, int *out_kind, unsigned *out_id,
                                    char *name, int name_cap,
                                    char *victim, int victim_cap,
                                    char *line, int line_cap, float *out_time);


/* ---------- Batch: depth→hiz bind / portal winding / mdl skin pages / weapon hitgroup ---------- */
int  engine_depth_hiz_bind_plan(unsigned mip0_w, unsigned mip0_h,
                                int *out_needed, int *out_steps, unsigned *out_views);
int  engine_depth_hiz_bind_execute(unsigned mip0_w, unsigned mip0_h,
                                   const float *depth_samples, unsigned count,
                                   int *out_levels, int *out_views, int *out_bound);
int  engine_mdl_hiz_vis_query_at_mip(float x0, float y0, float x1, float y1,
                                     float obj_depth, int mip,
                                     int *out_visible, int *out_occluded, float *out_hiz);
int  engine_mdl_hiz_vis_query_multi_mip(float x0, float y0, float x1, float y1,
                                        float obj_depth,
                                        int *out_visible, int *out_occluded,
                                        float *out_hiz, int *out_mip);

int  engine_portal_winding_make_rect(float cx, float cy, float cz,
                                     float nx, float ny, float nz,
                                     float half_w, float half_h);
int  engine_portal_winding_clip_water(void);
unsigned engine_water_reflect_recursive_plan(float eye_x, float eye_y, float eye_z,
                                             unsigned max_depth,
                                             unsigned *out_views, unsigned *out_max_depth);

int  engine_mdl_skin_pages_build(unsigned page_count);
int  engine_mdl_skin_pages_sample(unsigned group, unsigned tex, float u, float v,
                                  float *out_rgba4);
int  engine_water_reflect_ent_bind_skin_page(unsigned ent_index, unsigned page_index);
int  engine_water_reflect_ent_sample_skin_page(unsigned ent_index, float u, float v,
                                               float *out_rgba4);

int  engine_game_weapon_hit_auth_hitgroup(unsigned weapon_id, float now,
                                          float ox, float oy, float oz,
                                          float dx, float dy, float dz,
                                          unsigned killer_id, unsigned victim_id,
                                          int force_hit, unsigned hitgroup,
                                          int *out_fired, int *out_queued, int *out_died,
                                          int *out_registered, float *out_damage,
                                          int *out_headshot);


/* ---------- Batch: hiz-array / portal-graph / mdl-skin-lumps / ipa-artifact ---------- */
int  engine_mdl_hiz_bind_texture2d_array(unsigned *out_slices, unsigned *out_mip0_w, unsigned *out_mip0_h);
int  engine_mdl_hiz_array_mark_bound(void);
int  engine_mdl_hiz_array_was_bound(void);
int  engine_mdl_hiz_vis_query_array_mip(float x0, float y0, float x1, float y1,
                                        float obj_depth, int array_mip,
                                        int *out_visible, int *out_occluded,
                                        float *out_hiz, int *out_mip);
int  engine_depth_hiz_array_bind(unsigned slice_count, int *out_slices, int *out_bound);

int  engine_bsp_portal_graph_build_multi(unsigned *out_leaves, unsigned *out_edges);
int  engine_bsp_portal_graph_build_from_current(unsigned *out_leaves, unsigned *out_edges);
int  engine_bsp_portal_graph_flood(unsigned start_leaf, unsigned max_depth,
                                   unsigned *out_reached, unsigned *out_depth_max);
unsigned engine_water_reflect_portal_graph_plan(float eye_x, float eye_y, float eye_z,
                                                unsigned eye_leaf, unsigned max_depth,
                                                unsigned *out_views, unsigned *out_flooded);

int  engine_mdl_skin_lumps_load(const unsigned char *bytes, unsigned size,
                                unsigned *out_count, int *out_from_asset);
int  engine_mdl_skin_lumps_load_or_fixture(const unsigned char *bytes, unsigned size,
                                           unsigned fixture_pages,
                                           unsigned *out_count, int *out_fallback);
int  engine_mdl_skin_lumps_sample(unsigned index, float u, float v, float *out_rgba4);
int  engine_mdl_skin_lump_bind_water_ent(unsigned ent_index, unsigned lump_index);


/* ---------- Batch: hiz-gpu-downsample / portal-windings / mdl-skinref / ipa-sign ---------- */
int  engine_mdl_hiz_array_downsample(unsigned *out_slices, unsigned *out_passes,
                                     int *out_ready);
int  engine_mdl_hiz_vis_query_downsampled(float x0, float y0, float x1, float y1,
                                          float obj_depth, int preferred_mip,
                                          int *out_visible, int *out_occluded,
                                          float *out_hiz, int *out_mip);
int  engine_depth_hiz_downsample_bind(unsigned slices, unsigned passes,
                                      int *out_bound, int *out_vis_ready);

int  engine_bsp_portal_windings_from_current(unsigned *out_count, int *out_from_bsp);
int  engine_bsp_portal_windings_fixture(unsigned *out_count);
int  engine_bsp_portal_winding_get(unsigned index,
                                   float *out_plane4, unsigned *out_verts,
                                   float *out_center3, int *out_from_marks);
int  engine_bsp_portal_graph_attach_windings(unsigned *out_attached);
unsigned engine_water_reflect_portal_winding_plan(float eye_x, float eye_y, float eye_z,
                                                  unsigned winding_index, unsigned max_depth,
                                                  unsigned *out_views);

int  engine_mdl_skinref_init_fixture(unsigned *out_families, unsigned *out_entries);
int  engine_mdl_skinref_select_family(unsigned family_id);
int  engine_mdl_skinref_select_family_name(const char *name);
int  engine_mdl_skinref_select_ref(unsigned ref_in_family);
int  engine_mdl_skinref_cycle_family(int dir);
int  engine_mdl_skinref_resolve(unsigned *out_family, unsigned *out_ref,
                                unsigned *out_group, unsigned *out_tex,
                                unsigned *out_skin_index);
int  engine_mdl_skinref_sample(float u, float v, float *out_rgba4);

#endif /* ENGINE_BRIDGE_H */

