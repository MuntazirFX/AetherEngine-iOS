// EngineBridge.c — AetherEngine-iOS · Clean-room.
// STEP 14: collision + gravity + player physics.
// STEP 15A: WAD + BSP miptex diagnostics.
// STEP 15B: Palette + texture atlas.

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
#include "../../engine/player/AetherPlayer.h"
#include "../../engine/player/AetherCollision.h"
#include "../../engine/texture/AetherTexture.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ---------- Globals ---------- */
static aether_engine_t         *g_engine       = NULL;
static aether_game_manager_t   *g_game_manager = NULL;
static aether_input_t          *g_input        = NULL;
static aether_settings_t       *g_settings     = NULL;
static aether_fs_t             *g_fs           = NULL;
static aether_audio_t          *g_audio        = NULL;
static aether_renderer_t       *g_renderer     = NULL;
static aether_mesh_t           *g_active_mesh  = NULL;
static aether_collision_t      *g_collision    = NULL;
static aether_texture_atlas_t  *g_atlas        = NULL;
static aether_palette_t         g_palette;
static aether_player_t          g_player;
static char                     g_base_path[512] = {0};

/* ---------- Metal hooks ---------- */
extern int32_t aether_metal_init_swift    (void *user, uint32_t w, uint32_t h);
extern int32_t aether_metal_resize_swift  (void *user, uint32_t w, uint32_t h);
extern int32_t aether_metal_submit_swift  (void *user, const void *cmd);
extern int32_t aether_metal_shutdown_swift(void *user);

aether_result_t aether_metal_init    (void *u, u32 w, u32 h) { return (aether_result_t)aether_metal_init_swift(u, (uint32_t)w, (uint32_t)h); }
aether_result_t aether_metal_resize  (void *u, u32 w, u32 h) { return (aether_result_t)aether_metal_resize_swift(u, (uint32_t)w, (uint32_t)h); }
aether_result_t aether_metal_submit  (void *u, const aether_render_cmd_t *c) { return (aether_result_t)aether_metal_submit_swift(u, c); }
aether_result_t aether_metal_shutdown(void *u) { return (aether_result_t)aether_metal_shutdown_swift(u); }

/* ---------- Helpers ---------- */
static aether_input_action_t map_action_name(const char *n) {
    if (!n) return AETHER_ACTION_NONE;
    if (strcmp(n, "fire")        == 0) return AETHER_ACTION_FIRE;
    if (strcmp(n, "jump")        == 0) return AETHER_ACTION_JUMP;
    if (strcmp(n, "duck")        == 0) return AETHER_ACTION_DUCK;
    if (strcmp(n, "use")         == 0) return AETHER_ACTION_USE;
    if (strcmp(n, "reload")      == 0) return AETHER_ACTION_RELOAD;
    if (strcmp(n, "weapon_next") == 0) return AETHER_ACTION_WEAPON_NEXT;
    if (strcmp(n, "weapon_prev") == 0) return AETHER_ACTION_WEAPON_PREV;
    if (strcmp(n, "pause")       == 0) return AETHER_ACTION_PAUSE;
    if (strcmp(n, "scoreboard")  == 0) return AETHER_ACTION_SCOREBOARD;
    return AETHER_ACTION_NONE;
}

/* ---------- Engine lifecycle ---------- */
void engine_init(const char *base_path, const char *asset_path) {
    if (g_engine) return;
    if (!base_path || !asset_path) return;

    aether_str_copy(g_base_path, sizeof g_base_path, base_path);
    aether_log(AETHER_LOG_INFO, "bridge", "base_path = %s", g_base_path);

    g_settings = aether_settings_create();
    aether_settings_register_engine_defaults(g_settings);
    g_fs       = aether_fs_create(base_path);
    g_input    = aether_input_create();
    g_audio    = aether_audio_create();
    aether_audio_init(g_audio);
    g_renderer = aether_renderer_create(AETHER_RENDER_METAL, NULL);

    aether_player_init(&g_player);

    aether_engine_desc_t desc = { .base_path = base_path, .asset_path = asset_path, .flags = 0 };
    g_engine = aether_engine_create(&desc);
    if (!g_engine) return;
    if (aether_engine_start(g_engine) != AETHER_OK) return;

    g_game_manager = aether_game_manager_create(g_engine, base_path);
    aether_log(AETHER_LOG_INFO, "bridge", "engine fully initialized (%s)", AETHER_VERSION_STRING);
}

void engine_shutdown(void) {
    if (g_atlas)        { aether_texture_atlas_free(g_atlas); g_atlas = NULL; }
    if (g_collision)    { aether_collision_free(g_collision); g_collision = NULL; }
    if (g_active_mesh)  { aether_mesh_free(g_active_mesh); g_active_mesh = NULL; }
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

/* ---------- Player (STEP 13/14) ---------- */
void engine_player_spawn_at_mesh_center(void) {
    if (!g_active_mesh) return;
    aether_vec3_t c = {
        g_active_mesh->bounds_center[0],
        g_active_mesh->bounds_center[1],
        g_active_mesh->bounds_center[2] + 50.0f
    };
    aether_player_set_position(&g_player, c);
    g_player.yaw   = 0.0f;
    g_player.pitch = 0.0f;
    aether_log(AETHER_LOG_INFO, "player",
               "spawned at (%.1f, %.1f, %.1f)", c.x, c.y, c.z);
}

void engine_player_tick(float dt) {
    if (!g_input) return;
    aether_input_begin_frame(g_input);
    const aether_input_state_t *st = aether_input_state(g_input);
    aether_player_update(&g_player, st, g_collision, dt);
    aether_input_end_frame(g_input);
}

void engine_player_get_eye(float out[3]) {
    aether_vec3_t e = aether_player_eye_position(&g_player);
    out[0] = e.x; out[1] = e.y; out[2] = e.z;
}
void engine_player_get_forward(float out[3]) {
    aether_vec3_t f = aether_player_forward(&g_player);
    out[0] = f.x; out[1] = f.y; out[2] = f.z;
}
void engine_player_get_position(float out[3]) {
    out[0] = g_player.position.x;
    out[1] = g_player.position.y;
    out[2] = g_player.position.z;
}
void engine_player_set_position(float x, float y, float z) {
    aether_player_set_position(&g_player, (aether_vec3_t){x,y,z});
}
void  engine_player_set_angles(float y, float p) { g_player.yaw = y; g_player.pitch = p; }
float engine_player_get_yaw(void)   { return g_player.yaw; }
float engine_player_get_pitch(void) { return g_player.pitch; }

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

/* ---------- BSP inspect ---------- */
int engine_bsp_inspect(const char *p) {
    if (!p) return 0;
    aether_bsp_t *b = aether_bsp_load(p);
    if (!b) return 0;
    aether_bsp_dump(b); aether_bsp_free(b); return 1;
}

int engine_bsp_inspect_vfs(const char *vp) {
    if (!g_fs || !vp) return 0;
    u32 sz = aether_fs_read_file(g_fs, vp, NULL, 0);
    if (sz == 0) return 0;
    u8 *buf = (u8*)malloc(sz); if (!buf) return 0;
    u32 got = aether_fs_read_file(g_fs, vp, buf, sz);
    if (got != sz) { free(buf); return 0; }
    aether_bsp_t *b = aether_bsp_load_from_memory(buf, sz, vp);
    free(buf); if (!b) return 0;
    aether_bsp_dump(b); aether_bsp_free(b); return 1;
}

int engine_bsp_inspect_vfs_text(const char *vp, char *ob, int cap) {
    if (!g_fs || !vp || !ob || cap <= 0) return -1;
    u32 sz = aether_fs_read_file(g_fs, vp, NULL, 0);
    if (sz == 0) { snprintf(ob, (size_t)cap, "NOT FOUND: %s", vp); return 0; }
    snprintf(ob, (size_t)cap, "Found %u bytes: %s", sz, vp);
    return 1;
}

/* ---------- BSP mesh + collision + atlas (STEP 12/14/15B) ---------- */
int engine_bsp_mesh_build(const char *vp) {
    if (!g_fs || !vp) return 0;

    /* Clean previous state */
    if (g_atlas)       { aether_texture_atlas_free(g_atlas); g_atlas = NULL; }
    if (g_active_mesh) { aether_mesh_free(g_active_mesh);   g_active_mesh = NULL; }
    if (g_collision)   { aether_collision_free(g_collision); g_collision = NULL; }

    /* Read BSP */
    u32 sz = aether_fs_read_file(g_fs, vp, NULL, 0);
    if (sz == 0 || sz > 64u*1024u*1024u) {
        aether_log(AETHER_LOG_ERROR, "bridge", "mesh: size %u invalid", sz);
        return 0;
    }
    u8 *buf = (u8*)malloc(sz);
    if (!buf) return 0;
    u32 got = aether_fs_read_file(g_fs, vp, buf, sz);
    if (got != sz) { free(buf); return 0; }

    aether_bsp_t *b = aether_bsp_load_from_memory(buf, sz, vp);
    free(buf);
    if (!b) return 0;

    /* ---- 1. Load palette (from halflife.wad, once) ---- */
    if (!g_palette.loaded) {
        u32 wad_sz = aether_fs_read_file(g_fs, "halflife.wad", NULL, 0);
        if (wad_sz > 0 && wad_sz < 256u*1024u*1024u) {
            u8 *wbuf = (u8*)malloc(wad_sz);
            if (wbuf) {
                u32 got_w = aether_fs_read_file(g_fs, "halflife.wad", wbuf, wad_sz);
                if (got_w == wad_sz) {
                    aether_wad_t *w = aether_wad_load_from_memory(wbuf, wad_sz, "halflife.wad");
                    if (w) {
                        (void)aether_palette_from_wad(&g_palette, w);
                        aether_wad_free(w);
                    }
                }
                free(wbuf);
            }
        }
        if (!g_palette.loaded) {
            aether_palette_default(&g_palette);
            aether_log(AETHER_LOG_WARN, "palette", "using default greyscale palette");
        }
    }

    /* ---- 2. Build texture atlas from BSP-embedded miptex ---- */
    g_atlas = aether_texture_atlas_build(b, &g_palette);
    if (g_atlas) {
        aether_texture_atlas_dump(g_atlas);
    } else {
        aether_log(AETHER_LOG_WARN, "bridge", "no atlas (mesh will have no UVs)");
    }

    /* ---- 3. Build mesh (with atlas UVs if available) ---- */
    aether_mesh_t *m = NULL;
    aether_result_t r = aether_mesh_from_bsp(b, g_atlas, &m);
    if (r != AETHER_OK || !m) {
        aether_bsp_free(b);
        return 0;
    }
    g_active_mesh = m;
    aether_log(AETHER_LOG_INFO, "bridge", "mesh OK: %u verts, %u tris",
               m->vertex_count, m->index_count / 3);

    /* ---- 4. Build collision ---- */
    g_collision = aether_collision_build(b);
    if (g_collision) aether_collision_dump(g_collision);
    else aether_log(AETHER_LOG_WARN, "bridge", "collision build failed (free-fly)");

    aether_bsp_free(b);
    return 1;
}

int  engine_bsp_mesh_vertex_count(void)   { return g_active_mesh ? (int)g_active_mesh->vertex_count : 0; }
int  engine_bsp_mesh_index_count(void)    { return g_active_mesh ? (int)g_active_mesh->index_count  : 0; }
int  engine_bsp_mesh_triangle_count(void) { return g_active_mesh ? (int)(g_active_mesh->index_count / 3) : 0; }

void engine_bsp_mesh_get_bounds(float *mn, float *mx, float *ctr) {
    if (!g_active_mesh) return;
    if (mn)  { mn[0]=g_active_mesh->bounds_min[0];    mn[1]=g_active_mesh->bounds_min[1];    mn[2]=g_active_mesh->bounds_min[2]; }
    if (mx)  { mx[0]=g_active_mesh->bounds_max[0];    mx[1]=g_active_mesh->bounds_max[1];    mx[2]=g_active_mesh->bounds_max[2]; }
    if (ctr) { ctr[0]=g_active_mesh->bounds_center[0];ctr[1]=g_active_mesh->bounds_center[1];ctr[2]=g_active_mesh->bounds_center[2]; }
}

int engine_bsp_mesh_copy_vertices(float *out, int maxv) {
    if (!g_active_mesh || !out || maxv <= 0) return 0;
    int n = (int)g_active_mesh->vertex_count; if (n > maxv) n = maxv;
    memcpy(out, g_active_mesh->vertices, (size_t)n * sizeof(aether_mesh_vertex_t));
    return n;
}
int engine_bsp_mesh_copy_indices(uint32_t *out, int maxi) {
    if (!g_active_mesh || !out || maxi <= 0) return 0;
    int n = (int)g_active_mesh->index_count; if (n > maxi) n = maxi;
    memcpy(out, g_active_mesh->indices, (size_t)n * sizeof(u32));
    return n;
}

void engine_bsp_mesh_release(void) {
    if (g_atlas)       { aether_texture_atlas_free(g_atlas); g_atlas = NULL; }
    if (g_active_mesh) { aether_mesh_free(g_active_mesh); g_active_mesh = NULL; }
    if (g_collision)   { aether_collision_free(g_collision); g_collision = NULL; }
}

/* ---------- Texture / WAD diagnostics (STEP 15A) ---------- */
int engine_texture_dump_wad(const char *wad_vpath) {
    if (!g_fs || !wad_vpath) return 0;
    u32 sz = aether_fs_read_file(g_fs, wad_vpath, NULL, 0);
    if (sz == 0 || sz > 256u*1024u*1024u) return 0;
    u8 *buf = (u8*)malloc(sz);
    if (!buf) return 0;
    u32 got = aether_fs_read_file(g_fs, wad_vpath, buf, sz);
    if (got != sz) { free(buf); return 0; }
    aether_wad_t *w = aether_wad_load_from_memory(buf, sz, wad_vpath);
    free(buf);
    if (!w) return 0;
    aether_wad_dump(w);
    aether_wad_free(w);
    return 1;
}

int engine_texture_dump_bsp_miptex(void) {
    if (!g_fs) return 0;
    const char *vp = "maps/c0a0.bsp";
    u32 sz = aether_fs_read_file(g_fs, vp, NULL, 0);
    if (sz == 0) return 0;
    u8 *buf = (u8*)malloc(sz);
    if (!buf) return 0;
    u32 got = aether_fs_read_file(g_fs, vp, buf, sz);
    if (got != sz) { free(buf); return 0; }
    aether_bsp_t *b = aether_bsp_load_from_memory(buf, sz, vp);
    free(buf);
    if (!b) return 0;
    aether_bsp_miptex_dump(b);
    aether_bsp_free(b);
    return 1;
}

int engine_texture_summary_text(char *out_buf, int out_cap) {
    if (!g_fs || !out_buf || out_cap <= 0) return -1;

    int w = 0;
    w += snprintf(out_buf + w, (size_t)(out_cap - w),
                  "STEP 15A — Texture diagnostics\n\n");

    const char *bsp_vp = "maps/c0a0.bsp";
    u32 bsp_sz = aether_fs_read_file(g_fs, bsp_vp, NULL, 0);
    if (bsp_sz > 0) {
        u8 *bbuf = (u8*)malloc(bsp_sz);
        if (bbuf) {
            u32 got = aether_fs_read_file(g_fs, bsp_vp, bbuf, bsp_sz);
            if (got == bsp_sz) {
                aether_bsp_t *b = aether_bsp_load_from_memory(bbuf, bsp_sz, bsp_vp);
                if (b) {
                    u32 count = aether_bsp_miptex_count(b);
                    w += snprintf(out_buf + w, (size_t)(out_cap - w),
                                  "BSP: c0a0.bsp\n  embedded miptex: %u\n", count);
                    u32 shown = count > 5 ? 5 : count;
                    for (u32 i = 0; i < shown; ++i) {
                        aether_miptex_info_t info;
                        if (aether_bsp_miptex_info(b, i, &info)) {
                            w += snprintf(out_buf + w, (size_t)(out_cap - w),
                                          "  • %s  (%ux%u)\n",
                                          info.name, info.width, info.height);
                        }
                    }
                    if (count > shown)
                        w += snprintf(out_buf + w, (size_t)(out_cap - w),
                                      "  … +%u more\n", count - shown);
                    aether_bsp_free(b);
                }
            }
            free(bbuf);
        }
    } else {
        w += snprintf(out_buf + w, (size_t)(out_cap - w), "BSP: c0a0.bsp NOT FOUND\n");
    }

    w += snprintf(out_buf + w, (size_t)(out_cap - w), "\n");
    const char *wad_vp = "halflife.wad";
    u32 wad_sz = aether_fs_read_file(g_fs, wad_vp, NULL, 0);
    if (wad_sz == 0) {
        w += snprintf(out_buf + w, (size_t)(out_cap - w), "WAD: halflife.wad NOT FOUND\n");
        return 1;
    }
    u8 *wbuf = (u8*)malloc(wad_sz);
    if (!wbuf) return 1;
    u32 got = aether_fs_read_file(g_fs, wad_vp, wbuf, wad_sz);
    if (got != wad_sz) { free(wbuf); return 1; }
    aether_wad_t *wad = aether_wad_load_from_memory(wbuf, wad_sz, wad_vp);
    free(wbuf);
    if (!wad) { w += snprintf(out_buf + w, (size_t)(out_cap - w), "WAD: parse failed\n"); return 1; }

    u32 total = aether_wad_lump_count(wad);
    u32 miptex = 0, palette = 0, other = 0;
    for (u32 i = 0; i < total; ++i) {
        const aether_wad_lump_t *L = aether_wad_lump_at(wad, i);
        if (!L) continue;
        if (L->type == AETHER_WAD_TYPE_MIPTEX) miptex++;
        else if (L->type == AETHER_WAD_TYPE_PALETTE) palette++;
        else other++;
    }
    w += snprintf(out_buf + w, (size_t)(out_cap - w),
                  "WAD: halflife.wad (%u bytes)\n"
                  "  total lumps : %u\n  miptex      : %u\n  palette     : %u\n  other       : %u\n",
                  wad_sz, total, miptex, palette, other);
    aether_wad_free(wad);
    return 1;
}

/* ---------- Texture atlas (STEP 15B) ---------- */
int engine_texture_build_atlas(void) {
    return g_atlas ? 1 : 0;
}

int engine_texture_atlas_width(void)      { return g_atlas ? (int)g_atlas->width  : 0; }
int engine_texture_atlas_height(void)     { return g_atlas ? (int)g_atlas->height : 0; }
int engine_texture_atlas_slot_count(void) { return g_atlas ? (int)g_atlas->slot_count : 0; }

int engine_texture_atlas_copy_rgba(unsigned char *out, int max_bytes) {
    if (!g_atlas || !g_atlas->rgba || !out || max_bytes <= 0) return 0;
    u32 needed = g_atlas->width * g_atlas->height * 4;
    if ((u32)max_bytes < needed) return 0;
    memcpy(out, g_atlas->rgba, needed);
    return (int)needed;
}

/* ---------- Utility ---------- */
const char *engine_base_path(void) { return g_base_path; }
const char *engine_version(void)   { return AETHER_VERSION_STRING; }
