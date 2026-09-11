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
void  engine_player_tick(float dt);
void  engine_player_get_eye(float out_xyz[3]);
void  engine_player_get_forward(float out_xyz[3]);
void  engine_player_get_position(float out_xyz[3]);
void  engine_player_set_position(float x, float y, float z);
void  engine_player_set_angles(float yaw, float pitch);
float engine_player_get_yaw(void);
float engine_player_get_pitch(void);

/* ---------- Settings ---------- */
void engine_settings_save(const char *filepath);
void engine_settings_load(const char *filepath);

/* ---------- Audio ---------- */
void engine_audio_init(void);
void engine_audio_shutdown(void);
void engine_audio_set_master_volume(float vol);
void engine_audio_set_mute(bool muted);
void engine_audio_play(const char *asset_path, float volume, bool loop);
void engine_audio_stop_all(void);

/* ---------- Renderer ---------- */
void engine_renderer_attach_metal(void *mtkView);
void engine_renderer_resize(unsigned int width, unsigned int height);
void engine_renderer_begin_frame(void);
void engine_renderer_end_frame(void);

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

/* ---------- Texture / WAD diagnostics ---------- */
int  engine_texture_dump_wad(const char *wad_vpath);
int  engine_texture_dump_bsp_miptex(void);
int  engine_texture_summary_text(char *out_buf, int out_cap);

/* ---------- Texture atlas ---------- */
int  engine_texture_build_atlas(void);
int  engine_texture_atlas_width(void);
int  engine_texture_atlas_height(void);
int  engine_texture_atlas_slot_count(void);
int  engine_texture_atlas_copy_rgba(unsigned char *out, int max_bytes);

/* ---------- MDL diagnostics ---------- */
int  engine_mdl_dump_vfs(const char *mdl_vpath);
int  engine_mdl_summary_text(const char *mdl_vpath, char *out_buf, int out_cap);

/* ---------- MDL mesh extraction (STEP 16B) ---------- */
int  engine_mdl_mesh_build(const char *mdl_vpath);
int  engine_mdl_mesh_vertex_count(void);
int  engine_mdl_mesh_triangle_count(void);
void engine_mdl_mesh_get_bounds(float out_min[3], float out_max[3], float out_center[3]);
int  engine_mdl_mesh_copy_positions(float *out, int max_floats);
int  engine_mdl_mesh_copy_normals(float *out, int max_floats);
int  engine_mdl_mesh_copy_indices(uint32_t *out, int max_indices);
void engine_mdl_mesh_release(void);

/* ---------- Utility ---------- */
const char *engine_base_path(void);
const char *engine_version(void);

#ifdef __cplusplus
}
#endif
#endif /* ENGINE_BRIDGE_H */
