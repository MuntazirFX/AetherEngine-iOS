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

/* ---------- Utility ---------- */
const char *engine_base_path(void);
const char *engine_version(void);

#ifdef __cplusplus
}
#endif
#endif /* ENGINE_BRIDGE_H */
