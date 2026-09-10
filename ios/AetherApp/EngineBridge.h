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
/* x: -1.0 (left)  .. 1.0 (right)
   y: -1.0 (back)  .. 1.0 (forward) */
void engine_input_set_move(float x, float y);

/* ---------- Input: look delta (accumulated per frame) ---------- */
void engine_input_add_look(float dx, float dy);

/* ---------- Input: actions ---------- */
/* Accepted action names: "fire", "jump", "duck", "use", "reload",
   "weapon_next", "weapon_prev", "pause", "scoreboard" */
void engine_input_set_action(const char *action_name, bool pressed);

/* ---------- Audio ---------- */
void        engine_audio_init(void);
void        engine_audio_shutdown(void);
void        engine_audio_set_master_volume(float vol);
void        engine_audio_set_mute(bool muted);
void        engine_audio_play(const char *asset_path, float volume, bool loop);
void        engine_audio_stop_all(void);

/* ---------- Settings ---------- */
void engine_settings_save(const char *filepath);
void engine_settings_load(const char *filepath);

/* ---------- Utility ---------- */
const char *engine_version(void);

#ifdef __cplusplus
}
#endif
#endif /* ENGINE_BRIDGE_H */
