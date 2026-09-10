// EngineBridge.c
// Implementation of the bridge between Swift and AetherEngine.

#include "EngineBridge.h"
#include "../../engine/core/AetherEngine.h"
#include "../../engine/core/AetherGameManager.h"

static aether_engine_t *g_engine = NULL;
static aether_game_manager_t *g_game_manager = NULL;

void engine_init(const char *base_path, const char *asset_path) {
    if (g_engine) return; // Already initialized

    aether_engine_desc_t desc = {
        .base_path = base_path,
        .asset_path = asset_path,
        .flags = 0
    };

    g_engine = aether_engine_create(&desc);
    if (!g_engine) {
        aether_log(AETHER_LOG_ERROR, "bridge", "Failed to create engine");
        return;
    }

    aether_engine_start(g_engine);
    g_game_manager = aether_game_manager_create(g_engine, base_path);
}

void engine_launch_game(const char *game_dir) {
    if (!g_game_manager) return;
    
    // Find game by directory name
    const aether_game_info_t *info = aether_game_info_by_dir(game_dir);
    if (!info) {
        aether_log(AETHER_LOG_ERROR, "bridge", "Game not found: %s", game_dir);
        return;
    }

    aether_game_select(g_game_manager, info->id);
    aether_game_initialize(g_game_manager);
    aether_game_launch(g_game_manager);
}

void engine_shutdown(void) {
    if (g_game_manager) {
        aether_game_manager_destroy(g_game_manager);
        g_game_manager = NULL;
    }
    if (g_engine) {
        aether_engine_stop(g_engine);
        aether_engine_destroy(g_engine);
        g_engine = NULL;
    }
}
