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

/* ---------- Game lifecycle ---------- */
void engine_launch_game(const char *game_dir);
void engine_stop_game(void);

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
float engine_player_get_pitch(void);

/* ---------- HUD ---------- */
float engine_hud_health(void);
float engine_hud_max_health(void);
float engine_hud_armor(void);
float engine_hud_battery(void);
bool  engine_hud_alive(void);
int   engine_hud_active_weapon(void);
int   engine_hud_reserve_ammo(void);
int   engine_hud_clip(void);
int   engine_hud_clip_max(void);
void  engine_hud_set_clip(int clip, int clip_max);
void  engine_hud_set_crosshair_style(int style);
void  engine_hud_set_crosshair_spread(float spread);
float engine_hud_crosshair_spread(void);
void  engine_hud_give_demo_loadout(void);

/* ---------- Settings / Audio / Renderer ---------- */
void engine_settings_save(const char *filepath);
void engine_settings_load(const char *filepath);
void engine_audio_init(void);
void engine_audio_shutdown(void);
void engine_audio_set_master_volume(float vol);
void engine_audio_set_mute(bool muted);
void engine_audio_play(const char *asset_path, float volume, bool loop);
void engine_audio_stop_all(void);
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

/* ---------- Utility ---------- */
const char *engine_base_path(void);
const char *engine_version(void);

#ifdef __cplusplus
}
#endif
#endif /* ENGINE_BRIDGE_H */
