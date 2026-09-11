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

/* ---------- Player / gameplay (STEP 13) ---------- */
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

/* ---------- Utility ---------- */
const char *engine_base_path(void);
const char *engine_version(void);

#ifdef __cplusplus
}
#endif
#endif
