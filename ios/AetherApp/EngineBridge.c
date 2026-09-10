// EngineBridge.c
// Implementation of the bridge between Swift and AetherEngine.
// AetherEngine-iOS · Clean-room.

#include "EngineBridge.h"

#include "../../engine/core/AetherEngine.h"
#include "../../engine/game/AetherGameManager.h"
#include "../../engine/input/AetherInput.h"
#include "../../engine/config/AetherSettings.h"
#include "../../engine/fs/AetherFS.h"
#include "../../engine/audio/AetherAudio.h"

#include <string.h>
#include <stdlib.h>

/* ---------- Global state (owned by the bridge) ---------- */
static aether_engine_t       *g_engine        = NULL;
static aether_game_manager_t *g_game_manager  = NULL;
static aether_input_t        *g_input         = NULL;
static aether_settings_t     *g_settings      = NULL;
static aether_fs_t           *g_fs            = NULL;
static aether_audio_t        *g_audio         = NULL;

/* ---------- Helpers ---------- */
static aether_input_action_t map_action_name(const char *name) {
    if (!name) return AETHER_ACTION_NONE;
    if (strcmp(name, "fire")        == 0) return AETHER_ACTION_FIRE;
    if (strcmp(name, "jump")        == 0) return AETHER_ACTION_JUMP;
    if (strcmp(name, "duck")        == 0) return AETHER_ACTION_DUCK;
    if (strcmp(name, "use")         == 0) return AETHER_ACTION_USE;
    if (strcmp(name, "reload")      == 0) return AETHER_ACTION_RELOAD;
    if (strcmp(name, "weapon_next") == 0) return AETHER_ACTION_WEAPON_NEXT;
    if (strcmp(name, "weapon_prev") == 0) return AETHER_ACTION_WEAPON_PREV;
    if (strcmp(name, "pause")       == 0) return AETHER_ACTION_PAUSE;
    if (strcmp(name, "scoreboard")  == 0) return AETHER_ACTION_SCOREBOARD;
    return AETHER_ACTION_NONE;
}

/* ---------- Engine lifecycle ---------- */
void engine_init(const char *base_path, const char *asset_path) {
    if (g_engine) return;  /* already initialized */
    if (!base_path || !asset_path) {
        aether_log(AETHER_LOG_ERROR, "bridge", "engine_init: null paths");
        return;
    }

    /* 1. Settings */
    g_settings = aether_settings_create();
    aether_settings_register_engine_defaults(g_settings);

    /* 2. Virtual filesystem */
    g_fs = aether_fs_create();
    aether_fs_mount_dir(g_fs, base_path);    /* writable (Documents) */
    aether_fs_mount_dir(g_fs, asset_path);   /* read-only (bundle) */

    /* 3. Input */
    g_input = aether_input_create();

    /* Audio */
    g_audio = aether_audio_create();
    aether_audio_init(g_audio);

    /* 4. Engine */
    aether_engine_desc_t desc = {
        .base_path  = base_path,
        .asset_path = asset_path,
        .flags      = 0
    };
    g_engine = aether_engine_create(&desc);
    if (!g_engine) {
        aether_log(AETHER_LOG_ERROR, "bridge", "engine_create failed");
        return;
    }

    aether_result_t r = aether_engine_start(g_engine);
    if (r != AETHER_OK) {
        aether_log(AETHER_LOG_ERROR, "bridge", "engine_start failed: %s",
                   aether_result_string(r));
        return;
    }

    /* 5. Game manager */
    g_game_manager = aether_game_manager_create(g_engine, base_path);

    aether_log(AETHER_LOG_INFO, "bridge", "engine fully initialized (%s)",
               AETHER_VERSION_STRING);
}

void engine_shutdown(void) {
    if (g_game_manager) { aether_game_manager_destroy(g_game_manager); g_game_manager = NULL; }
    if (g_engine)       { aether_engine_stop(g_engine); aether_engine_destroy(g_engine); g_engine = NULL; }
    if (g_input)        { aether_input_destroy(g_input); g_input = NULL; }
    if (g_fs)           { aether_fs_destroy(g_fs); g_fs = NULL; }
    if (g_settings)     { aether_settings_destroy(g_settings); g_settings = NULL; }
    aether_log(AETHER_LOG_INFO, "bridge", "engine shutdown complete");
}

/* ---------- Game lifecycle ---------- */
void engine_launch_game(const char *game_dir) {
    if (!g_game_manager || !game_dir) return;

    const aether_game_info_t *info = aether_game_info_by_dir(game_dir);
    if (!info) {
        aether_log(AETHER_LOG_ERROR, "bridge", "game not found: %s", game_dir);
        return;
    }

    if (aether_game_select(g_game_manager, info->id) != AETHER_OK) return;
    if (aether_game_initialize(g_game_manager)      != AETHER_OK) return;

    /* Mount this game's data directory into the VFS at highest priority. */
    if (g_fs) {
        char game_dir_full[600];
        if (aether_game_resolve_path(g_game_manager, info->id,
                                     game_dir_full, sizeof game_dir_full) == AETHER_OK) {
            aether_fs_mount_dir(g_fs, game_dir_full);
        }
    }

    aether_game_launch(g_game_manager);
}

void engine_stop_game(void) {
    if (!g_game_manager) return;
    aether_game_shutdown(g_game_manager);
}

/* ---------- Input ---------- */
void engine_input_set_move(float x, float y) {
    if (g_input) aether_input_set_move(g_input, x, y);
}

void engine_input_add_look(float dx, float dy) {
    if (g_input) aether_input_add_look(g_input, dx, dy);
}

void engine_input_set_action(const char *action_name, bool pressed) {
    if (!g_input || !action_name) return;
    aether_input_action_t a = map_action_name(action_name);
    if (a != AETHER_ACTION_NONE) {
        aether_input_set_action(g_input, a, pressed);
    }
}

/* ---------- Settings persistence ---------- */
void engine_settings_save(const char *filepath) {
    if (g_settings && filepath) {
        (void)aether_settings_save(g_settings, filepath);
    }
}

void engine_settings_load(const char *filepath) {
    if (g_settings && filepath) {
        (void)aether_settings_load(g_settings, filepath);
    }
}

/* ---------- Utility ---------- */
const char *engine_version(void) {
    return AETHER_VERSION_STRING;
}
