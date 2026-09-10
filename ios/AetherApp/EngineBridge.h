// EngineBridge.h
// C header to expose AetherEngine functions to Swift.

#ifndef ENGINE_BRIDGE_H
#define ENGINE_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the engine
void engine_init(const char *base_path, const char *asset_path);

// Launch a specific game
void engine_launch_game(const char *game_dir);

// Shutdown the engine
void engine_shutdown(void);

#ifdef __cplusplus
}
#endif
#endif /* ENGINE_BRIDGE_H */
