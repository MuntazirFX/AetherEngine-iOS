// EngineBridge.h
// C header exposing AetherEngine functions to Swift.
// AetherEngine-iOS · Clean-room.

#ifndef ENGINE_BRIDGE_H
#define ENGINE_BRIDGE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Engine lifecycle ---------- */
void engine_init(const char *base_path, const char *asset_path);
void engine_shutdown(void);

/* ---------- Game lifecycle ---------- */
void engine_launch_game(const char *game_dir);
void engine_stop_game(void);

/* ---------- Input: movement ---------- */
void engine_input_set_move(float x, float y);

/* ---------- Input: look delta ---------- */
void engine_input_add_look(float dx, float dy);

/* ---------- Input: actions ---------- */
void engine_input_set_action(const char *action_name, bool pressed);

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

/* ---------- Utility ---------- */
const char *engine_version(void);

#ifdef __cplusplus
}
#endif
#endif /* ENGINE_BRIDGE_H */
