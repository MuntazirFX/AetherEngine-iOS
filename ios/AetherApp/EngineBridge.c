// EngineBridge.c
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
#include "../../engine/bsp/AetherBSPGeometry.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ---------- Global state ---------- */
static aether_engine_t       *g_engine       = NULL;
static aether_game_manager_t *g_game_manager = NULL;
static aether_input_t        *g_input        = NULL;
static aether_settings_t     *g_settings     = NULL;
static aether_fs_t           *g_fs           = NULL;
static aether_audio_t        *g_audio        = NULL;
static aether_renderer_t     *g_renderer     = NULL;
static aether_mesh_t         *g_active_mesh  = NULL;
static char                   g_base_path[512] = {0};

/* ---------- Metal backend hooks (MetalCallbacks.swift) ---------- */
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
    if (g_engine) return;
    if (!base_path || !asset_path) { aether_log(AETHER_LOG_ERROR, "bridge", "null paths"); return; }

    aether_str_copy(g_base_path, sizeof g_base_path, base_path);
    aether_log(AETHER_LOG_INFO, "bridge", "base_path = %s", g_base_path);

    g_settings = aether_settings_create();
    aether_settings_register_engine_defaults(g_settings);
    g_fs       = aether_fs_create(base_path);
    g_input    = aether_input_create();
    g_audio    = aether_audio_create();
    aether_audio_init(g_audio);
    g_renderer = aether_renderer_create(AETHER_RENDER_METAL, NULL);

    aether_engine_desc_t desc = { .base_path = base_path, .asset_path = asset_path, .flags = 0 };
    g_engine = aether_engine_create(&desc);
    if (!g_engine) { aether_log(AETHER_LOG_ERROR, "bridge", "engine_create failed"); return; }
    aether_result_t r = aether_engine_start(g_engine);
    if (r != AETHER_OK) { aether_log(AETHER_LOG_ERROR, "bridge", "engine_start failed"); return; }
    g_game_manager = aether_game_manager_create(g_engine, base_path);
    aether_log(AETHER_LOG_INFO, "bridge", "engine fully initialized (%s)", AETHER_VERSION_STRING);
}

void engine_shutdown(void) {
    if (g_active_mesh) { aether_mesh_free(g_active_mesh); g_active_mesh = NULL; }
    if (g_game_manager) { aether_game_manager_destroy(g_game_manager); g_game_manager = NULL; }
    if (g_engine)       { aether_engine_stop(g_engine); aether_engine_destroy(g_engine); g_engine = NULL; }
    if (g_input)        { aether_input_destroy(g_input); g_input = NULL; }
    if (g_fs)           { aether_fs_destroy(g_fs); g_fs = NULL; }
    if (g_settings)     { aether_settings_destroy(g_settings); g_settings = NULL; }
    if (g_audio)        { aether_audio_shutdown(g_audio); aether_audio_destroy(g_audio); g_audio = NULL; }
    if (g_renderer)     { aether_renderer_shutdown(g_renderer); aether_renderer_destroy(g_renderer); g_renderer = NULL; }
    aether_log(AETHER_LOG_INFO, "bridge", "engine shutdown complete");
}

/* ---------- Game lifecycle ---------- */
void engine_launch_game(const char *game_dir) {
    if (!g_game_manager || !g_fs || !game_dir) return;
    const aether_game_info_t *info = aether_game_info_by_dir(game_dir);
    if (!info) return;
    if (aether_game_select(g_game_manager, info->id) != AETHER_OK) return;
    if (aether_game_initialize(g_game_manager)      != AETHER_OK) return;

    aether_fs_clear_roots(g_fs);
    char valve_dir[600];
    snprintf(valve_dir, sizeof valve_dir, "%s/valve", g_base_path);
    if (aether_fs_add_root(g_fs, valve_dir) == AETHER_OK)
        (void)aether_fs_auto_mount_paks(g_fs, valve_dir);

    if (!aether_str_eq(info->dir_name, "valve")) {
        char gd[600];
        if (aether_game_resolve_path(g_game_manager, info->id, gd, sizeof gd) == AETHER_OK) {
            if (aether_fs_add_root(g_fs, gd) == AETHER_OK)
                (void)aether_fs_auto_mount_paks(g_fs, gd);
        }
    }
    aether_game_launch(g_game_manager);
}

void engine_stop_game(void) { if (g_game_manager) aether_game_shutdown(g_game_manager); }

/* ---------- Input ---------- */
void engine_input_set_move(float x, float y)   { if (g_input) aether_input_set_move(g_input, x, y); }
void engine_input_add_look(float dx, float dy) { if (g_input) aether_input_add_look(g_input, dx, dy); }
void engine_input_set_action(const char *n, bool p) {
    if (!g_input || !n) return;
    aether_input_action_t a = map_action_name(n);
    if (a != AETHER_ACTION_NONE) aether_input_set_action(g_input, a, p);
}

/* ---------- Settings ---------- */
void engine_settings_save(const char *f) { if (g_settings && f) (void)aether_settings_save(g_settings, f); }
void engine_settings_load(const char *f) { if (g_settings && f) (void)aether_settings_load(g_settings, f); }

/* ---------- Audio ---------- */
void engine_audio_init(void)                 { if (!g_audio) g_audio = aether_audio_create(); aether_audio_init(g_audio); }
void engine_audio_shutdown(void)             { if (g_audio) aether_audio_shutdown(g_audio); }
void engine_audio_set_master_volume(float v) { if (g_audio) aether_audio_set_master_volume(g_audio, v); }
void engine_audio_set_mute(bool m)           { if (g_audio) aether_audio_set_mute(g_audio, m); }
void engine_audio_play(const char *p, float v, bool l) { if (g_audio && p) (void)aether_audio_play_effect(g_audio, p, v, l); }
void engine_audio_stop_all(void)             { if (g_audio) aether_audio_stop_all(g_audio); }

/* ---------- Renderer ---------- */
void engine_renderer_attach_metal(void *v) {
    if (!g_renderer) return;
    (void)aether_renderer_install_metal(g_renderer, v);
    (void)aether_renderer_init(g_renderer, 1080, 1920);
}
void engine_renderer_resize(unsigned int w, unsigned int h) { if (g_renderer) (void)aether_renderer_resize(g_renderer, w, h); }
void engine_renderer_begin_frame(void) { if (g_renderer) (void)aether_renderer_begin_frame(g_renderer, 0.05f, 0.05f, 0.08f, 1.0f); }
void engine_renderer_end_frame(void)   { if (g_renderer) (void)aether_renderer_end_frame(g_renderer); }

/* ---------- BSP inspect (STEP 11) ---------- */
int engine_bsp_inspect(const char *p) {
    if (!p) return 0;
    aether_bsp_t *b = aether_bsp_load(p);
    if (!b) return 0;
    aether_bsp_dump(b);
    aether_bsp_free(b);
    return 1;
}

int engine_bsp_inspect_vfs(const char *vp) {
    if (!g_fs || !vp) return 0;
    u32 sz = aether_fs_read_file(g_fs, vp, NULL, 0);
    if (sz == 0) return 0;
    u8 *buf = (u8*)malloc(sz);
    if (!buf) return 0;
    u32 got = aether_fs_read_file(g_fs, vp, buf, sz);
    if (got != sz) { free(buf); return 0; }
    aether_bsp_t *b = aether_bsp_load_from_memory(buf, sz, vp);
    free(buf);
    if (!b) return 0;
    aether_bsp_dump(b);
    aether_bsp_free(b);
    return 1;
}

int engine_bsp_inspect_vfs_text(const char *vp, char *ob, int cap) {
    if (!g_fs || !vp || !ob || cap <= 0) return -1;
    int w = 0;
    w += snprintf(ob + w, (size_t)(cap - w), "Documents: %s\nLooking for: %s\n\n", g_base_path, vp);

    u32 sz = aether_fs_read_file(g_fs, vp, NULL, 0);
    if (sz == 0) {
        snprintf(ob + w, (size_t)(cap - w),
                 "❌ FILE NOT FOUND\n\nExpected at:\n%s/valve/%s", g_base_path, vp);
        return 0;
    }
    w = (int)strlen(ob);
    w += snprintf(ob + w, (size_t)(cap - w), "✅ Found: %u bytes\n\n", sz);

    u8 *buf = (u8*)malloc(sz);
    if (!buf) return -1;
    u32 got = aether_fs_read_file(g_fs, vp, buf, sz);
    if (got != sz) { free(buf); return -1; }
    aether_bsp_t *b = aether_bsp_load_from_memory(buf, sz, vp);
    free(buf);
    if (!b) { snprintf(ob + w, (size_t)(cap - w), "❌ PARSE FAILED"); return -2; }

    w = (int)strlen(ob);
    snprintf(ob + w, (size_t)(cap - w),
             "✅ BSP v%u\n\nVertices: %u\nPlanes: %u\nEdges: %u\nFaces: %u\n"
             "Nodes: %u\nLeaves: %u\nModels: %u\nTexinfo: %u\nTextures: %u",
             aether_bsp_version(b),
             aether_bsp_vertex_count(b), aether_bsp_plane_count(b),
             aether_bsp_edge_count(b), aether_bsp_face_count(b),
             aether_bsp_node_count(b), aether_bsp_leaf_count(b),
             aether_bsp_model_count(b), aether_bsp_texinfo_count(b),
             aether_bsp_miptex_count(b));
    aether_bsp_free(b);
    return 1;
}

/* ---------- BSP mesh (STEP 12) ---------- */
int engine_bsp_mesh_build(const char *vp) {
    if (!g_fs || !vp) return 0;
    if (g_active_mesh) { aether_mesh_free(g_active_mesh); g_active_mesh = NULL; }

    u32 sz = aether_fs_read_file(g_fs, vp, NULL, 0);
    if (sz == 0) { aether_log(AETHER_LOG_ERROR, "bridge", "mesh: not found: %s", vp); return 0; }
    if (sz > 64u * 1024u * 1024u) { aether_log(AETHER_LOG_ERROR, "bridge", "mesh: too large"); return 0; }

    u8 *buf = (u8*)malloc(sz);
    if (!buf) return 0;
    u32 got = aether_fs_read_file(g_fs, vp, buf, sz);
    if (got != sz) { free(buf); return 0; }

    aether_bsp_t *b = aether_bsp_load_from_memory(buf, sz, vp);
    free(buf);
    if (!b) return 0;

    aether_mesh_t *m = NULL;
    aether_result_t r = aether_mesh_from_bsp(b, &m);
    aether_bsp_free(b);
    if (r != AETHER_OK || !m) return 0;

    aether_mesh_dump(m);
    g_active_mesh = m;
    return 1;
}

int engine_bsp_mesh_vertex_count(void)   { return g_active_mesh ? (int)g_active_mesh->vertex_count : 0; }
int engine_bsp_mesh_index_count(void)    { return g_active_mesh ? (int)g_active_mesh->index_count  : 0; }
int engine_bsp_mesh_triangle_count(void) { return g_active_mesh ? (int)(g_active_mesh->index_count / 3) : 0; }

void engine_bsp_mesh_get_bounds(float *mn, float *mx, float *ctr) {
    if (!g_active_mesh) return;
    if (mn)  { mn[0] = g_active_mesh->bounds_min[0]; mn[1] = g_active_mesh->bounds_min[1]; mn[2] = g_active_mesh->bounds_min[2]; }
    if (mx)  { mx[0] = g_active_mesh->bounds_max[0]; mx[1] = g_active_mesh->bounds_max[1]; mx[2] = g_active_mesh->bounds_max[2]; }
    if (ctr) { ctr[0] = g_active_mesh->bounds_center[0]; ctr[1] = g_active_mesh->bounds_center[1]; ctr[2] = g_active_mesh->bounds_center[2]; }
}

int engine_bsp_mesh_copy_vertices(float *out, int max_vertices) {
    if (!g_active_mesh || !out || max_vertices <= 0) return 0;
    int n = (int)g_active_mesh->vertex_count;
    if (n > max_vertices) n = max_vertices;
    memcpy(out, g_active_mesh->vertices, (size_t)n * sizeof(aether_mesh_vertex_t));
    return n;
}

int engine_bsp_mesh_copy_indices(uint32_t *out, int max_indices) {
    if (!g_active_mesh || !out || max_indices <= 0) return 0;
    int n = (int)g_active_mesh->index_count;
    if (n > max_indices) n = max_indices;
    memcpy(out, g_active_mesh->indices, (size_t)n * sizeof(u32));
    return n;
}

void engine_bsp_mesh_release(void) {
    if (g_active_mesh) { aether_mesh_free(g_active_mesh); g_active_mesh = NULL; }
}

/* ---------- Utility ---------- */
const char *engine_base_path(void) { return g_base_path; }
const char *engine_version(void)   { return AETHER_VERSION_STRING; }
