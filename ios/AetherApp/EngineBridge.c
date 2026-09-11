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
#include "../../engine/render/AetherRender.h"
#include "../../engine/bsp/AetherBSP.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static aether_engine_t       *g_engine       = NULL;
static aether_game_manager_t *g_game_manager = NULL;
static aether_input_t        *g_input        = NULL;
static aether_settings_t     *g_settings     = NULL;
static aether_fs_t           *g_fs           = NULL;
static aether_audio_t        *g_audio        = NULL;
static aether_renderer_t     *g_renderer     = NULL;
static char                   g_base_path[512] = {0};

extern int32_t aether_metal_init_swift    (void *user, uint32_t w, uint32_t h);
extern int32_t aether_metal_resize_swift  (void *user, uint32_t w, uint32_t h);
extern int32_t aether_metal_submit_swift  (void *user, const void *cmd);
extern int32_t aether_metal_shutdown_swift(void *user);

aether_result_t aether_metal_init    (void *user, u32 w, u32 h) {
    return (aether_result_t)aether_metal_init_swift(user, (uint32_t)w, (uint32_t)h);
}
aether_result_t aether_metal_resize  (void *user, u32 w, u32 h) {
    return (aether_result_t)aether_metal_resize_swift(user, (uint32_t)w, (uint32_t)h);
}
aether_result_t aether_metal_submit  (void *user, const aether_render_cmd_t *cmd) {
    return (aether_result_t)aether_metal_submit_swift(user, cmd);
}
aether_result_t aether_metal_shutdown(void *user) {
    return (aether_result_t)aether_metal_shutdown_swift(user);
}

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

void engine_init(const char *base_path, const char *asset_path) {
    if (g_engine) return;
    if (!base_path || !asset_path) {
        aether_log(AETHER_LOG_ERROR, "bridge", "engine_init: null paths");
        return;
    }
    aether_str_copy(g_base_path, sizeof g_base_path, base_path);
    aether_log(AETHER_LOG_INFO, "bridge", "base_path = %s", g_base_path);

    g_settings = aether_settings_create();
    aether_settings_register_engine_defaults(g_settings);
    g_fs = aether_fs_create(base_path);
    g_input = aether_input_create();
    g_audio = aether_audio_create();
    aether_audio_init(g_audio);
    g_renderer = aether_renderer_create(AETHER_RENDER_METAL, NULL);

    aether_engine_desc_t desc = {
        .base_path  = base_path,
        .asset_path = asset_path,
        .flags      = 0
    };
    g_engine = aether_engine_create(&desc);
    if (!g_engine) { aether_log(AETHER_LOG_ERROR, "bridge", "engine_create failed"); return; }

    aether_result_t r = aether_engine_start(g_engine);
    if (r != AETHER_OK) {
        aether_log(AETHER_LOG_ERROR, "bridge", "engine_start failed: %s", aether_result_string(r));
        return;
    }
    g_game_manager = aether_game_manager_create(g_engine, base_path);
    aether_log(AETHER_LOG_INFO, "bridge", "engine fully initialized (%s)", AETHER_VERSION_STRING);
}

void engine_shutdown(void) {
    if (g_game_manager) { aether_game_manager_destroy(g_game_manager); g_game_manager = NULL; }
    if (g_engine)       { aether_engine_stop(g_engine); aether_engine_destroy(g_engine); g_engine = NULL; }
    if (g_input)        { aether_input_destroy(g_input); g_input = NULL; }
    if (g_fs)           { aether_fs_destroy(g_fs); g_fs = NULL; }
    if (g_settings)     { aether_settings_destroy(g_settings); g_settings = NULL; }
    if (g_audio)        { aether_audio_shutdown(g_audio); aether_audio_destroy(g_audio); g_audio = NULL; }
    if (g_renderer)     { aether_renderer_shutdown(g_renderer); aether_renderer_destroy(g_renderer); g_renderer = NULL; }
    aether_log(AETHER_LOG_INFO, "bridge", "engine shutdown complete");
}

void engine_launch_game(const char *game_dir) {
    if (!g_game_manager || !g_fs || !game_dir) return;
    const aether_game_info_t *info = aether_game_info_by_dir(game_dir);
    if (!info) {
        aether_log(AETHER_LOG_ERROR, "bridge", "game not found: %s", game_dir);
        return;
    }
    if (aether_game_select(g_game_manager, info->id) != AETHER_OK) return;
    if (aether_game_initialize(g_game_manager)      != AETHER_OK) return;

    aether_fs_clear_roots(g_fs);
    char valve_dir[600];
    snprintf(valve_dir, sizeof valve_dir, "%s/valve", g_base_path);
    if (aether_fs_add_root(g_fs, valve_dir) == AETHER_OK) {
        (void)aether_fs_auto_mount_paks(g_fs, valve_dir);
    }
    if (!aether_str_eq(info->dir_name, "valve")) {
        char game_dir_full[600];
        if (aether_game_resolve_path(g_game_manager, info->id,
                                     game_dir_full, sizeof game_dir_full) == AETHER_OK) {
            if (aether_fs_add_root(g_fs, game_dir_full) == AETHER_OK) {
                (void)aether_fs_auto_mount_paks(g_fs, game_dir_full);
            }
        }
    }
    aether_log(AETHER_LOG_INFO, "bridge", "VFS ready: %u roots",
               aether_fs_root_count(g_fs));
    aether_game_launch(g_game_manager);
}

void engine_stop_game(void) {
    if (!g_game_manager) return;
    aether_game_shutdown(g_game_manager);
}

void engine_input_set_move(float x, float y)   { if (g_input) aether_input_set_move(g_input, x, y); }
void engine_input_add_look(float dx, float dy) { if (g_input) aether_input_add_look(g_input, dx, dy); }
void engine_input_set_action(const char *name, bool pressed) {
    if (!g_input || !name) return;
    aether_input_action_t a = map_action_name(name);
    if (a != AETHER_ACTION_NONE) aether_input_set_action(g_input, a, pressed);
}

void engine_settings_save(const char *f) { if (g_settings && f) (void)aether_settings_save(g_settings, f); }
void engine_settings_load(const char *f) { if (g_settings && f) (void)aether_settings_load(g_settings, f); }

void engine_audio_init(void)                { if (!g_audio) g_audio = aether_audio_create(); aether_audio_init(g_audio); }
void engine_audio_shutdown(void)            { if (g_audio) aether_audio_shutdown(g_audio); }
void engine_audio_set_master_volume(float v){ if (g_audio) aether_audio_set_master_volume(g_audio, v); }
void engine_audio_set_mute(bool m)          { if (g_audio) aether_audio_set_mute(g_audio, m); }
void engine_audio_play(const char *p, float v, bool l) { if (g_audio && p) (void)aether_audio_play_effect(g_audio, p, v, l); }
void engine_audio_stop_all(void)            { if (g_audio) aether_audio_stop_all(g_audio); }

void engine_renderer_attach_metal(void *mtkView) {
    if (!g_renderer) return;
    (void)aether_renderer_install_metal(g_renderer, mtkView);
    (void)aether_renderer_init(g_renderer, 1080, 1920);
}
void engine_renderer_resize(unsigned int w, unsigned int h) { if (g_renderer) (void)aether_renderer_resize(g_renderer, w, h); }
void engine_renderer_begin_frame(void) { if (g_renderer) (void)aether_renderer_begin_frame(g_renderer, 0.05f, 0.05f, 0.08f, 1.0f); }
void engine_renderer_end_frame(void)   { if (g_renderer) (void)aether_renderer_end_frame(g_renderer); }

int engine_bsp_inspect(const char *bsp_path) {
    if (!bsp_path) return 0;
    aether_log(AETHER_LOG_INFO, "bridge", "BSP inspect (direct): %s", bsp_path);
    aether_bsp_t *bsp = aether_bsp_load(bsp_path);
    if (!bsp) return 0;
    aether_bsp_dump(bsp);
    aether_bsp_free(bsp);
    return 1;
}

int engine_bsp_inspect_vfs(const char *vpath) {
    if (!g_fs || !vpath) return 0;
    u32 size = aether_fs_read_file(g_fs, vpath, NULL, 0);
    if (size == 0) {
        aether_log(AETHER_LOG_ERROR, "bridge", "VFS: not found: %s", vpath);
        return 0;
    }
    u8 *buf = (u8*)malloc(size);
    if (!buf) return 0;
    u32 got = aether_fs_read_file(g_fs, vpath, buf, size);
    if (got != size) { free(buf); return 0; }

    aether_bsp_t *bsp = aether_bsp_load_from_memory(buf, size, vpath);
    free(buf);
    if (!bsp) return 0;
    aether_bsp_dump(bsp);
    aether_bsp_free(bsp);
    return 1;
}

const char *engine_version(void) { return AETHER_VERSION_STRING; }
