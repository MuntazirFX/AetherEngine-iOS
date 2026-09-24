// EngineBridge.c — AetherEngine-iOS · Clean-room.
// Complete fixed version — all includes + globals + functions.

#include "EngineBridge.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "../../engine/core/AetherCore.h"
#include "../../engine/core/AetherEngine.h"
#include "../../engine/game/AetherGameManager.h"
#include "../../engine/input/AetherInput.h"
#include "../../engine/config/AetherSettings.h"
#include "../../engine/fs/AetherFS.h"
#include "../../engine/audio/AetherAudio.h"
#include "../../engine/render/AetherRender.h"
#include "../../engine/render/AetherRenderFeatures.h"
#include "../../engine/render/AetherParticle.h"
#include "../../engine/render/AetherSky.h"
#include "../../engine/render/AetherWater.h"
#include "../../engine/render/AetherFog.h"
#include "../../engine/render/AetherLightmap.h"
#include "../../engine/bsp/AetherBSP.h"
#include "../../engine/bsp/AetherBSPGeometry.h"
#include "../../engine/bsp/AetherBSPSynthetic.h"
#include "../../engine/bsp/AetherBSPVis.h"
#include "../../engine/render/AetherWorld.h"
#include "../../engine/player/AetherPlayer.h"
#include "../../engine/player/AetherPlayerHealth.h"
#include "../../engine/player/AetherPlayerInventory.h"
#include "../../engine/client/hud/AetherHUD.h"
#include "../../engine/client/hud/AetherHealth.h"
#include "../../engine/client/hud/AetherAmmo.h"
#include "../../engine/client/hud/AetherCrosshair.h"
#include "../../engine/player/AetherCollision.h"
#include "../../engine/texture/AetherTexture.h"
#include "../../engine/model/AetherMDL.h"
#include "../../engine/model/AetherMDLGeometry.h"
#include "../../engine/entity/AetherEntityBase.h"
#include "../../engine/entity/AetherEntitySpawn.h"
#include "../../engine/game/monsters/AetherMonster.h"
#include "../../engine/vgui/AetherVGUIRuntime.h"
#include "../../engine/net/AetherNetScoreboard.h"
#include "../../engine/net/AetherNetChat.h"

/* ---------- Globals ---------- */
static aether_engine_t         *g_engine       = NULL;
static aether_game_manager_t   *g_game_manager = NULL;
static aether_input_t          *g_input        = NULL;
static aether_settings_t       *g_settings     = NULL;
static aether_fs_t             *g_fs           = NULL;
static aether_audio_t          *g_audio        = NULL;
static aether_renderer_t       *g_renderer     = NULL;
static aether_mesh_t           *g_active_mesh  = NULL;
static aether_bsp_t            *g_active_bsp   = NULL; /* kept for leaf/VIS queries + collision borrow */
static bool                     g_mesh_is_synthetic = false;
static float                    g_vis_view_origin[3] = {0.f, 0.f, 40.f};
static bool                     g_vis_force_full = false;
static aether_bsp_vis_mode_t    g_vis_mode = AETHER_BSP_VIS_USE_PVS;
static u32                     *g_vis_indices = NULL;
static u32                      g_vis_index_count = 0;
static u32                      g_vis_index_cap = 0;
static aether_bsp_vis_stats_t   g_vis_stats;
static aether_collision_t      *g_collision    = NULL;
static aether_texture_atlas_t  *g_atlas        = NULL;
static aether_model_mesh_t     *g_mdl_mesh     = NULL;
static aether_entity_mgr_t     *g_entity_mgr   = NULL;
static aether_monster_registry_t g_monsters;
static bool                     g_monsters_init = false;
static aether_palette_t         g_palette;
static aether_player_t          g_player;
static aether_player_health_t    g_player_health;
static aether_player_inventory_t g_player_inventory;
static aether_hud_t             *g_hud = NULL;
static aether_hud_health_t      *g_hud_health = NULL;
static aether_hud_ammo_t        *g_hud_ammo = NULL;
static aether_hud_crosshair_t   *g_hud_crosshair = NULL;
static i32                       g_hud_clip = 0;
static i32                       g_hud_clip_max = 0;
static char                     g_base_path[512] = {0};
static float                    g_mdl_render_pos[3] = { 0, 0, 0 };
static bool                     g_player_start_found = false;
static aether_scoreboard_t      g_scoreboard;
static aether_chat_log_t        g_chat;
static bool                     g_scoreboard_init = false;
static bool                     g_chat_init = false;

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

/* ---------- Lifecycle ---------- */
void engine_init(const char *base_path, const char *asset_path) {
    if (g_engine) return;
    if (!base_path || !asset_path) return;
    aether_str_copy(g_base_path, sizeof g_base_path, base_path);

    g_settings = aether_settings_create();
    aether_settings_register_engine_defaults(g_settings);
    g_fs       = aether_fs_create(base_path);
    g_input    = aether_input_create();
    g_audio    = aether_audio_create();
    aether_audio_init(g_audio);
    g_renderer = aether_renderer_create(AETHER_RENDER_METAL, NULL);
    aether_player_init(&g_player);
    aether_player_health_init(&g_player_health);
    aether_player_inv_init(&g_player_inventory);
    g_hud = aether_hud_create();
    if (g_hud) {
        g_hud_health = aether_hud_health_create(g_hud, &g_player_health);
        g_hud_ammo = aether_hud_ammo_create(g_hud, &g_player_inventory);
        g_hud_crosshair = aether_hud_crosshair_create(g_hud);
    }

    aether_engine_desc_t desc = { .base_path = base_path, .asset_path = asset_path, .flags = 0 };
    g_engine = aether_engine_create(&desc);
    if (!g_engine) return;
    if (aether_engine_start(g_engine) != AETHER_OK) return;
    (void)aether_vgui_runtime_init();
    aether_scoreboard_init(&g_scoreboard);
    aether_chat_init(&g_chat);
    g_scoreboard_init = true;
    g_chat_init = true;
    g_game_manager = aether_game_manager_create(g_engine, base_path);
    aether_log(AETHER_LOG_INFO, "bridge", "engine initialized (%s)", AETHER_VERSION_STRING);
}

void engine_shutdown(void) {
    aether_vgui_runtime_shutdown();
    g_hud_health = NULL;
    g_hud_ammo = NULL;
    g_hud_crosshair = NULL;
    if (g_hud) { aether_hud_destroy(g_hud); g_hud = NULL; }
    if (g_entity_mgr)   { aether_entity_mgr_destroy(g_entity_mgr); g_entity_mgr = NULL; }
    if (g_mdl_mesh)     { aether_mdl_geometry_free(g_mdl_mesh); g_mdl_mesh = NULL; }
    if (g_atlas)        { aether_texture_atlas_free(g_atlas); g_atlas = NULL; }
    if (g_collision)    { aether_collision_free(g_collision); g_collision = NULL; }
    if (g_active_mesh)  { aether_mesh_free(g_active_mesh); g_active_mesh = NULL; }
    if (g_active_bsp)   { aether_bsp_free(g_active_bsp); g_active_bsp = NULL; }
    free(g_vis_indices); g_vis_indices = NULL; g_vis_index_count = 0; g_vis_index_cap = 0;
    g_mesh_is_synthetic = false;
    if (g_game_manager) { aether_game_manager_destroy(g_game_manager); g_game_manager = NULL; }
    if (g_engine)       { aether_engine_stop(g_engine); aether_engine_destroy(g_engine); g_engine = NULL; }
    if (g_input)        { aether_input_destroy(g_input); g_input = NULL; }
    if (g_fs)           { aether_fs_destroy(g_fs); g_fs = NULL; }
    if (g_settings)     { aether_settings_destroy(g_settings); g_settings = NULL; }
    if (g_audio)        { aether_audio_shutdown(g_audio); aether_audio_destroy(g_audio); g_audio = NULL; }
    if (g_renderer)     { aether_renderer_shutdown(g_renderer); aether_renderer_destroy(g_renderer); g_renderer = NULL; }
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
    aether_player_health_reset(&g_player_health);
    aether_player_inv_reset(&g_player_inventory);
    g_hud_clip = 0;
    g_hud_clip_max = 0;
    if (g_hud_ammo) aether_hud_ammo_set_clip(g_hud_ammo, 0, 0);
    if (g_hud_crosshair) aether_hud_crosshair_set_spread(g_hud_crosshair, 0.0f);
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

/* ---------- Player ---------- */
void engine_player_spawn_at_mesh_center(void) {
    if (!g_active_mesh) return;
    aether_vec3_t c = {
        g_active_mesh->bounds_center[0],
        g_active_mesh->bounds_center[1],
        g_active_mesh->bounds_center[2] + 50.0f
    };
    aether_player_set_position(&g_player, c);
    g_player.yaw = 0.0f;
    g_player.pitch = 0.0f;
}

bool engine_player_has_start(void) { return g_player_start_found; }

void engine_player_spawn_at_start(void) {
    if (!g_entity_mgr) { engine_player_spawn_at_mesh_center(); return; }
    aether_vec3_t pos, ang;
    if (aether_entity_get_player_start(g_entity_mgr, &pos, &ang) == AETHER_OK) {
        g_player_start_found = true;
        aether_vec3_t spawn_pos = { pos.x, pos.y, pos.z + 30.0f };
        aether_player_set_position(&g_player, spawn_pos);
        g_player.yaw = ang.y;
        g_player.pitch = 0.0f;
        aether_log(AETHER_LOG_INFO, "player", "spawned at info_player_start (%.1f,%.1f,%.1f)",
                   pos.x, pos.y, pos.z);
    } else {
        engine_player_spawn_at_mesh_center();
    }
}

void engine_player_tick(float dt) {
    if (!g_input) return;
    aether_input_begin_frame(g_input);
    const aether_input_state_t *st = aether_input_state(g_input);
    aether_player_update(&g_player, st, g_collision, dt);
    aether_input_end_frame(g_input);

    if (g_hud_health) aether_hud_health_tick(g_hud_health, dt);
    if (g_hud_ammo) aether_hud_ammo_tick(g_hud_ammo, dt);
    if (g_hud_crosshair) aether_hud_crosshair_tick(g_hud_crosshair, dt);
    if (g_monsters_init) aether_monster_registry_tick(&g_monsters, dt);
    if (g_entity_mgr)    aether_entity_mgr_tick(g_entity_mgr, dt);
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
    out[0] = g_player.position.x; out[1] = g_player.position.y; out[2] = g_player.position.z;
}
void engine_player_set_position(float x, float y, float z) {
    aether_player_set_position(&g_player, (aether_vec3_t){x,y,z});
}
void  engine_player_set_angles(float y, float p) { g_player.yaw = y; g_player.pitch = p; }
float engine_player_get_yaw(void)   { return g_player.yaw; }
float engine_player_get_pitch(void) { return g_player.pitch; }
int engine_player_on_ground(void)   { return g_player.on_ground ? 1 : 0; }

/* ---------- Collision ---------- */
int engine_collision_ready(void) {
    return (g_collision && aether_collision_clipnode_count(g_collision) > 0) ? 1 : 0;
}
int engine_collision_clipnode_count(void) {
    return g_collision ? (int)aether_collision_clipnode_count(g_collision) : 0;
}
int engine_collision_hull_root(int hull_index) {
    return g_collision ? (int)aether_collision_hull_root(g_collision, hull_index) : -1;
}
int engine_collision_point_in_solid(float x, float y, float z, int hull_index) {
    if (!g_collision) return 0;
    aether_vec3_t p = { x, y, z };
    return aether_collision_point_in_solid(g_collision, p, hull_index) ? 1 : 0;
}
int engine_collision_move(float from_x, float from_y, float from_z,
                          float to_x, float to_y, float to_z,
                          int hull_index, float out_xyz[3]) {
    aether_vec3_t from = { from_x, from_y, from_z };
    aether_vec3_t to   = { to_x, to_y, to_z };
    bool on_ground = false;
    aether_vec3_t r = to;
    if (g_collision)
        r = aether_collision_move(g_collision, from, to, hull_index, &on_ground);
    if (out_xyz) { out_xyz[0] = r.x; out_xyz[1] = r.y; out_xyz[2] = r.z; }
    return on_ground ? 1 : 0;
}

/* ---------- HUD ---------- */
float engine_hud_health(void) { return aether_player_health_get(&g_player_health); }
float engine_hud_max_health(void) { return g_player_health.max_health; }
float engine_hud_armor(void) { return aether_player_health_get_armor(&g_player_health); }
float engine_hud_battery(void) { return aether_player_health_get_battery(&g_player_health); }
bool engine_hud_alive(void) { return aether_player_health_is_alive(&g_player_health); }

static aether_ammo_type_t bridge_ammo_type(aether_weapon_id_t w) {
    switch (w) {
        case AETHER_WPN_GLOCK:
        case AETHER_WPN_MP5: return AETHER_AMMO_9MM;
        case AETHER_WPN_PYTHON: return AETHER_AMMO_357;
        case AETHER_WPN_SHOTGUN: return AETHER_AMMO_BUCKSHOT;
        case AETHER_WPN_CROSSBOW: return AETHER_AMMO_BOLT;
        case AETHER_WPN_RPG: return AETHER_AMMO_RPG;
        case AETHER_WPN_GAUSS:
        case AETHER_WPN_EGON: return AETHER_AMMO_URANIUM;
        case AETHER_WPN_GRENADE: return AETHER_AMMO_GRENADE;
        default: return AETHER_AMMO_NONE;
    }
}

int engine_hud_active_weapon(void) { return (int)aether_player_inv_current(&g_player_inventory); }
int engine_hud_reserve_ammo(void) {
    aether_ammo_type_t t = bridge_ammo_type(aether_player_inv_current(&g_player_inventory));
    return t == AETHER_AMMO_NONE ? 0 : aether_player_inv_get_ammo(&g_player_inventory, t);
}
int engine_hud_clip(void) { return g_hud_clip; }
int engine_hud_clip_max(void) { return g_hud_clip_max; }
void engine_hud_set_clip(int clip, int clip_max) {
    if (clip < 0) clip = 0;
    if (clip_max < 0) clip_max = 0;
    g_hud_clip = clip;
    g_hud_clip_max = clip_max;
    if (g_hud_ammo) aether_hud_ammo_set_clip(g_hud_ammo, clip, clip_max);
}
void engine_hud_set_crosshair_style(int style) {
    if (g_hud_crosshair) aether_hud_crosshair_set_style(g_hud_crosshair, (aether_crosshair_style_t)style);
}
void engine_hud_set_crosshair_spread(float spread) {
    if (g_hud_crosshair) aether_hud_crosshair_set_spread(g_hud_crosshair, spread);
}
float engine_hud_crosshair_spread(void) {
    return g_hud_crosshair ? g_hud_crosshair->spread : 0.0f;
}

void engine_hud_give_demo_loadout(void) {
    /* Explicit debug/demo helper; normal game data can replace this later. */
    aether_player_inv_give_weapon(&g_player_inventory, AETHER_WPN_GLOCK);
    aether_player_inv_switch(&g_player_inventory, AETHER_WPN_GLOCK);
    aether_player_inv_give_ammo(&g_player_inventory, AETHER_AMMO_9MM, 100);
    engine_hud_set_clip(17, 17);
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
void engine_renderer_resize(unsigned int w, unsigned int h) {
    if (g_renderer) (void)aether_renderer_resize(g_renderer, w, h);
}
void engine_renderer_begin_frame(void) {
    engine_renderer_begin_frame_dt(1.0f / 60.0f);
}
void engine_renderer_begin_frame_dt(float dt) {
    if (g_renderer)
        (void)aether_renderer_begin_frame_dt(g_renderer, 0.05f, 0.05f, 0.08f, 1.0f, dt);
}
void engine_renderer_set_camera(const float view16[16], const float proj16[16]) {
    if (!g_renderer || !view16 || !proj16) return;
    aether_mat4_t view, proj;
    memcpy(view.m, view16, sizeof view.m);
    memcpy(proj.m, proj16, sizeof proj.m);
    (void)aether_renderer_set_camera(g_renderer, view, proj);
}
void engine_renderer_get_view(float out16[16]) {
    if (!out16) return;
    aether_mat4_t view;
    aether_renderer_get_view(g_renderer, &view);
    memcpy(out16, view.m, sizeof view.m);
}
void engine_renderer_get_proj(float out16[16]) {
    if (!out16) return;
    aether_mat4_t proj;
    aether_renderer_get_proj(g_renderer, &proj);
    memcpy(out16, proj.m, sizeof proj.m);
}
void engine_renderer_draw_world(void) {
    if (!g_renderer) return;
    aether_render_features_t *feat = aether_renderer_features(g_renderer);
    if (feat && g_active_mesh) {
        aether_world_render_set_surface_count(&feat->world,
            g_active_mesh->index_count / 3u);
        u32 vis_tris = g_vis_index_count ? (g_vis_index_count / 3u)
                                         : (g_active_mesh->index_count / 3u);
        aether_world_render_set_visible_surface_count(&feat->world, vis_tris);
    }
    (void)aether_renderer_draw_world(g_renderer);
}
void engine_renderer_draw_hud(void) {
    if (g_renderer) (void)aether_renderer_draw_hud(g_renderer);
}
void engine_renderer_draw_feature(int feature_cmd) {
    if (g_renderer)
        (void)aether_renderer_draw_feature(g_renderer, (aether_render_cmd_type_t)feature_cmd);
}
void engine_renderer_tick_features(float dt) {
    if (g_renderer) aether_renderer_tick_features(g_renderer, dt);
}
void engine_renderer_end_frame(void) {
    if (g_renderer) (void)aether_renderer_end_frame(g_renderer);
}

/* ---------- Particles ---------- */
static aether_particles_t *bridge_particles(void) {
    if (!g_renderer) return NULL;
    aether_render_features_t *f = aether_renderer_features(g_renderer);
    return f ? &f->particles : NULL;
}

int engine_particles_spawn_burst(float x, float y, float z, int count) {
    aether_particles_t *p = bridge_particles();
    if (!p || count <= 0) return 0;
    f32 origin[3] = { x, y, z };
    return (int)aether_particles_spawn_burst(p, origin, (u32)count);
}

int engine_particles_active_count(void) {
    aether_particles_t *p = bridge_particles();
    return p ? (int)aether_particles_active_count(p) : 0;
}

int engine_particles_copy_render(float *out_xyz_size_rgba, int max_particles) {
    aether_particles_t *p = bridge_particles();
    if (!p || !out_xyz_size_rgba || max_particles <= 0) return 0;
    /* Layout matches aether_particle_vertex_t (8 floats). */
    return (int)aether_particles_copy_render(
        p,
        (aether_particle_vertex_t *)out_xyz_size_rgba,
        (u32)max_particles);
}

void engine_particles_clear(void) {
    aether_particles_t *p = bridge_particles();
    if (p) aether_particles_clear(p);
}

/* ---------- Sky ---------- */
static aether_sky_t *bridge_sky(void) {
    if (!g_renderer) return NULL;
    aether_render_features_t *f = aether_renderer_features(g_renderer);
    return f ? &f->sky : NULL;
}

int engine_sky_enabled(void) {
    aether_sky_t *s = bridge_sky();
    return (s && s->enabled) ? 1 : 0;
}

void engine_sky_set_enabled(bool enabled) {
    aether_sky_t *s = bridge_sky();
    if (s) (void)aether_sky_set_enabled(s, enabled);
}

int engine_sky_face_count(void) {
    aether_sky_t *s = bridge_sky();
    return s ? (int)s->face_count : 0;
}

float engine_sky_radius(void) {
    aether_sky_t *s = bridge_sky();
    return s ? s->radius : 0.0f;
}

void engine_sky_set_radius(float radius) {
    aether_sky_t *s = bridge_sky();
    if (s) (void)aether_sky_set_radius(s, radius);
}

int engine_sky_set_name(const char *name) {
    aether_sky_t *s = bridge_sky();
    if (!s || !name) return 0;
    return aether_sky_set_name(s, name) == AETHER_OK ? 1 : 0;
}

int engine_sky_get_name(char *out, int out_cap) {
    aether_sky_t *s = bridge_sky();
    if (!s || !out || out_cap <= 0) return 0;
    aether_str_copy(out, (size_t)out_cap, s->name);
    return 1;
}

int engine_sky_copy_render(float *out_xyz_rgba, int max_vertices) {
    aether_sky_t *s = bridge_sky();
    if (!s || !out_xyz_rgba || max_vertices <= 0) return 0;
    /* Layout matches aether_sky_vertex_t (7 floats). */
    return (int)aether_sky_copy_render(
        s,
        (aether_sky_vertex_t *)out_xyz_rgba,
        (u32)max_vertices);
}

int engine_sky_render_vertex_capacity(void) {
    return (int)aether_sky_render_vertex_count();
}


/* ---------- Water ---------- */
static aether_water_t *bridge_water(void) {
    if (!g_renderer) return NULL;
    aether_render_features_t *f = aether_renderer_features(g_renderer);
    return f ? &f->water : NULL;
}

int engine_water_enabled(void) {
    aether_water_t *w = bridge_water();
    return (w && w->enabled) ? 1 : 0;
}

void engine_water_set_enabled(bool enabled) {
    aether_water_t *w = bridge_water();
    if (w) aether_water_set_enabled(w, enabled);
}

float engine_water_wave_time(void) {
    aether_water_t *w = bridge_water();
    return w ? w->wave_time : 0.0f;
}

float engine_water_opacity(void) {
    aether_water_t *w = bridge_water();
    return w ? w->opacity : 0.0f;
}

void engine_water_set_color(float r, float g, float b, float a) {
    aether_water_t *w = bridge_water();
    if (!w) return;
    f32 rgba[4] = { r, g, b, a };
    (void)aether_water_set_color(w, rgba);
}

void engine_water_set_size(float size) {
    aether_water_t *w = bridge_water();
    if (w) (void)aether_water_set_size(w, size);
}

void engine_water_set_height(float height) {
    aether_water_t *w = bridge_water();
    if (w) (void)aether_water_set_height(w, height);
}

void engine_water_set_origin(float x, float y) {
    aether_water_t *w = bridge_water();
    if (w) (void)aether_water_set_origin(w, x, y);
}

void engine_water_set_wave(float speed, float amp, float freq) {
    aether_water_t *w = bridge_water();
    if (w) (void)aether_water_set_wave(w, speed, amp, freq);
}

int engine_water_copy_render(float *out_xyz_uv_rgba, int max_vertices) {
    aether_water_t *w = bridge_water();
    if (!w || !out_xyz_uv_rgba || max_vertices <= 0) return 0;
    /* Layout matches aether_water_vertex_t (9 floats). */
    return (int)aether_water_copy_render(
        w,
        (aether_water_vertex_t *)out_xyz_uv_rgba,
        (u32)max_vertices);
}

int engine_water_render_vertex_capacity(void) {
    return (int)aether_water_render_vertex_count();
}






/* ---------- Fog ---------- */
static aether_fog_t *bridge_fog(void) {
    if (!g_renderer) return NULL;
    aether_render_features_t *f = aether_renderer_features(g_renderer);
    return f ? &f->fog : NULL;
}

int engine_fog_enabled(void) {
    aether_fog_t *fog = bridge_fog();
    return (fog && fog->enabled) ? 1 : 0;
}

void engine_fog_set_enabled(bool enabled) {
    aether_fog_t *fog = bridge_fog();
    if (fog) aether_fog_set_enabled(fog, enabled);
}

float engine_fog_density(void) {
    aether_fog_t *fog = bridge_fog();
    return fog ? fog->density : 0.0f;
}

float engine_fog_factor(void) {
    aether_fog_t *fog = bridge_fog();
    return fog ? fog->factor : 0.0f;
}

float engine_fog_start(void) {
    aether_fog_t *fog = bridge_fog();
    return fog ? fog->start : 0.0f;
}

float engine_fog_end(void) {
    aether_fog_t *fog = bridge_fog();
    return fog ? fog->end : 0.0f;
}

void engine_fog_set_density(float density) {
    aether_fog_t *fog = bridge_fog();
    if (fog) aether_fog_set_density(fog, density);
}

void engine_fog_set_factor(float factor) {
    aether_fog_t *fog = bridge_fog();
    if (fog) (void)aether_fog_set_factor(fog, factor);
}

void engine_fog_set_range(float start, float end) {
    aether_fog_t *fog = bridge_fog();
    if (fog) aether_fog_set_range(fog, start, end);
}

void engine_fog_set_color(float r, float g, float b, float a) {
    aether_fog_t *fog = bridge_fog();
    if (!fog) return;
    f32 rgba[4] = { r, g, b, a };
    (void)aether_fog_set_color(fog, rgba);
}

void engine_fog_get_color(float out_rgba[4]) {
    aether_fog_t *fog = bridge_fog();
    if (!out_rgba) return;
    if (!fog) { out_rgba[0]=out_rgba[1]=out_rgba[2]=out_rgba[3]=0.0f; return; }
    out_rgba[0] = fog->color[0];
    out_rgba[1] = fog->color[1];
    out_rgba[2] = fog->color[2];
    out_rgba[3] = fog->color[3];
}

int engine_fog_copy_render(float *out_xy_uv_rgba, int max_vertices) {
    aether_fog_t *fog = bridge_fog();
    if (!fog || !out_xy_uv_rgba || max_vertices <= 0) return 0;
    /* Layout matches aether_fog_vertex_t (8 floats). */
    return (int)aether_fog_copy_render(
        fog,
        (aether_fog_vertex_t *)out_xy_uv_rgba,
        (u32)max_vertices);
}

int engine_fog_render_vertex_capacity(void) {
    return (int)aether_fog_render_vertex_count();
}


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

/* ---------- BSP mesh + collision + atlas + entities + monsters ---------- */

/* Shared: activate a loaded BSP into mesh + entities + monsters.
 * Takes ownership of bsp and keeps it in g_active_bsp for leaf/VIS + collision. */
static int bridge_activate_bsp(aether_bsp_t *b, bool synthetic) {
    if (!b) return 0;

    if (g_monsters_init) { aether_monster_registry_reset(&g_monsters); g_monsters_init = false; }
    if (g_entity_mgr) { aether_entity_mgr_destroy(g_entity_mgr); g_entity_mgr = NULL; }
    if (g_atlas)      { aether_texture_atlas_free(g_atlas);    g_atlas = NULL; }
    if (g_collision)  { aether_collision_free(g_collision);    g_collision = NULL; }
    if (g_active_mesh){ aether_mesh_free(g_active_mesh);       g_active_mesh = NULL; }
    if (g_active_bsp) { aether_bsp_free(g_active_bsp);         g_active_bsp = NULL; }
    free(g_vis_indices); g_vis_indices = NULL; g_vis_index_count = 0; g_vis_index_cap = 0;
    memset(&g_vis_stats, 0, sizeof g_vis_stats);
    g_player_start_found = false;
    g_mesh_is_synthetic = false;

    if (!g_palette.loaded) {
        if (g_fs) {
            u32 wad_sz = aether_fs_read_file(g_fs, "halflife.wad", NULL, 0);
            if (wad_sz > 0 && wad_sz < 256u*1024u*1024u) {
                u8 *wbuf = (u8*)malloc(wad_sz);
                if (wbuf) {
                    u32 gw = aether_fs_read_file(g_fs, "halflife.wad", wbuf, wad_sz);
                    if (gw == wad_sz) {
                        aether_wad_t *w = aether_wad_load_from_memory(wbuf, wad_sz, "halflife.wad");
                        if (w) { (void)aether_palette_from_wad(&g_palette, w); aether_wad_free(w); }
                    }
                    free(wbuf);
                }
            }
        }
        if (!g_palette.loaded) aether_palette_default(&g_palette);
    }

    if (!synthetic)
        g_atlas = aether_texture_atlas_build(b, &g_palette);

    aether_mesh_t *m = NULL;
    if (aether_mesh_from_bsp(b, g_atlas, &m) != AETHER_OK || !m) {
        aether_bsp_free(b); return 0;
    }
    g_active_bsp = b;
    g_active_mesh = m;
    g_mesh_is_synthetic = synthetic;

    g_collision = aether_collision_build(g_active_bsp);

    g_entity_mgr = aether_entity_mgr_create();
    if (g_entity_mgr) {
        u32 spawned = aether_entity_spawn_from_bsp(g_entity_mgr, g_active_bsp);
        aether_log(AETHER_LOG_INFO, "bridge", "spawned %u runtime entities%s",
                   spawned, synthetic ? " (synthetic)" : "");
    }

    if (g_entity_mgr) {
        aether_monster_registry_init(&g_monsters, NULL);
        g_monsters_init = true;

        u32 total = aether_entity_mgr_count(g_entity_mgr);
        u32 monster_count = 0;
        for (u32 i = 0; i < total; ++i) {
            const aether_entity_t *e = aether_entity_mgr_at(g_entity_mgr, i);
            if (!e) continue;
            if (strncmp(e->classname, "monster_", 8) != 0) continue;

            aether_monster_id_t id = AETHER_MON_NONE;
            if (strstr(e->classname, "headcrab"))            id = AETHER_MON_HEADCRAB;
            else if (strstr(e->classname, "zombie"))         id = AETHER_MON_ZOMBIE;
            else if (strstr(e->classname, "barnacle"))       id = AETHER_MON_BARNACLE;
            else if (strstr(e->classname, "houndeye"))       id = AETHER_MON_HOUNDEYE;
            else if (strstr(e->classname, "bullsquid"))      id = AETHER_MON_BULLSQUID;
            else if (strstr(e->classname, "alien_grunt"))    id = AETHER_MON_ALIEN_GRUNT;
            else if (strstr(e->classname, "alien_slave"))    id = AETHER_MON_ALIEN_SLAVE;
            else if (strstr(e->classname, "gargantua"))      id = AETHER_MON_GARGANTUA;
            else if (strstr(e->classname, "human_grunt"))    id = AETHER_MON_HUMAN_GRUNT;
            else if (strstr(e->classname, "human_sergeant")) id = AETHER_MON_HUMAN_SERGEANT;
            else if (strstr(e->classname, "scientist"))      id = AETHER_MON_SCIENTIST;
            else if (strstr(e->classname, "barney"))         id = AETHER_MON_BARNEY;
            else if (strstr(e->classname, "gman"))           id = AETHER_MON_GMAN;
            else if (strstr(e->classname, "turret"))         id = AETHER_MON_TURRET;

            if (id != AETHER_MON_NONE) {
                aether_monster_registry_spawn(&g_monsters, id, e->origin);
                monster_count++;
            }
        }
        aether_log(AETHER_LOG_INFO, "bridge", "spawned %u monsters", monster_count);

        aether_vec3_t ps, pa;
        if (aether_entity_get_player_start(g_entity_mgr, &ps, &pa) == AETHER_OK)
            g_player_start_found = true;
    }

    if (g_renderer) {
        aether_render_features_t *feat = aether_renderer_features(g_renderer);
        if (feat) {
            aether_world_render_set_surface_count(&feat->world, m->index_count / 3u);
            /* Procedural lightmap stub so synthetic (and atlas-less) rooms are not flat. */
            if (aether_lightmap_bake_mesh_stub(&feat->lightmap, m) == AETHER_OK) {
                aether_log(AETHER_LOG_INFO, "bridge",
                           "lightmap stub %ux%u baked for mesh (%s)",
                           aether_lightmap_width(&feat->lightmap),
                           aether_lightmap_height(&feat->lightmap),
                           synthetic ? "synthetic" : "bsp");
            }
        }
    }

    /* Seed view origin at mesh center / player start height and build initial cull list. */
    g_vis_view_origin[0] = m->bounds_center[0];
    g_vis_view_origin[1] = m->bounds_center[1];
    g_vis_view_origin[2] = m->bounds_center[2];
    if (g_player_start_found && g_entity_mgr) {
        aether_vec3_t ps, pa;
        if (aether_entity_get_player_start(g_entity_mgr, &ps, &pa) == AETHER_OK) {
            g_vis_view_origin[0] = ps.x;
            g_vis_view_origin[1] = ps.y;
            g_vis_view_origin[2] = ps.z + 36.f; /* eye-ish */
        }
    }
    (void)engine_bsp_vis_update();
    aether_log(AETHER_LOG_INFO, "bridge",
               "VIS leaf=%d faces %u/%u indices %u/%u mode=%d",
               g_vis_stats.view_leaf,
               g_vis_stats.visible_faces, g_vis_stats.total_faces,
               g_vis_stats.visible_indices, g_vis_stats.total_indices,
               (int)g_vis_mode);
    return 1;
}

int engine_bsp_mesh_build(const char *vp) {
    if (!g_fs || !vp) return 0;

    u32 sz = aether_fs_read_file(g_fs, vp, NULL, 0);
    if (sz == 0 || sz > 64u*1024u*1024u) return 0;
    u8 *buf = (u8*)malloc(sz);
    if (!buf) return 0;
    u32 got = aether_fs_read_file(g_fs, vp, buf, sz);
    if (got != sz) { free(buf); return 0; }
    aether_bsp_t *b = aether_bsp_load_from_memory(buf, sz, vp);
    free(buf);
    if (!b) return 0;
    return bridge_activate_bsp(b, false);
}

int engine_bsp_mesh_build_synthetic(void) {
    aether_bsp_t *b = aether_bsp_create_synthetic_room();
    if (!b) return 0;
    return bridge_activate_bsp(b, true);
}

int engine_bsp_mesh_build_or_synthetic(const char *vpath) {
    if (vpath && engine_bsp_mesh_build(vpath) == 1) return 1;
    aether_log(AETHER_LOG_WARN, "bridge",
               "BSP '%s' unavailable — using synthetic demo room",
               vpath ? vpath : "(null)");
    return engine_bsp_mesh_build_synthetic();
}

int engine_bsp_mesh_is_synthetic(void) {
    return g_mesh_is_synthetic ? 1 : 0;
}

int  engine_bsp_mesh_vertex_count(void)   { return g_active_mesh ? (int)g_active_mesh->vertex_count : 0; }
int  engine_bsp_mesh_index_count(void)    { return g_active_mesh ? (int)g_active_mesh->index_count  : 0; }
int  engine_bsp_mesh_triangle_count(void) { return g_active_mesh ? (int)(g_active_mesh->index_count / 3) : 0; }
void engine_bsp_mesh_get_bounds(float mn[3], float mx[3], float ctr[3]) {
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
    g_mesh_is_synthetic = false;
    if (g_monsters_init) { aether_monster_registry_reset(&g_monsters); g_monsters_init = false; }
    if (g_entity_mgr) { aether_entity_mgr_destroy(g_entity_mgr); g_entity_mgr = NULL; }
    if (g_atlas)      { aether_texture_atlas_free(g_atlas); g_atlas = NULL; }
    if (g_collision)  { aether_collision_free(g_collision); g_collision = NULL; }
    if (g_active_mesh){ aether_mesh_free(g_active_mesh); g_active_mesh = NULL; }
    if (g_active_bsp) { aether_bsp_free(g_active_bsp); g_active_bsp = NULL; }
    free(g_vis_indices); g_vis_indices = NULL; g_vis_index_count = 0; g_vis_index_cap = 0;
    memset(&g_vis_stats, 0, sizeof g_vis_stats);
}



/* ---------- BSP VIS / leaf culling ---------- */
void engine_bsp_vis_set_view_origin(float x, float y, float z) {
    g_vis_view_origin[0] = x;
    g_vis_view_origin[1] = y;
    g_vis_view_origin[2] = z;
}

void engine_bsp_vis_get_view_origin(float out_xyz[3]) {
    if (!out_xyz) return;
    out_xyz[0] = g_vis_view_origin[0];
    out_xyz[1] = g_vis_view_origin[1];
    out_xyz[2] = g_vis_view_origin[2];
}

void engine_bsp_vis_set_force_full(bool enabled) {
    g_vis_force_full = enabled;
}

int engine_bsp_vis_force_full(void) {
    return g_vis_force_full ? 1 : 0;
}

void engine_bsp_vis_set_mode(int mode) {
    if (mode < 0 || mode > 2) mode = 0;
    g_vis_mode = (aether_bsp_vis_mode_t)mode;
}

int engine_bsp_vis_mode(void) {
    return (int)g_vis_mode;
}

int engine_bsp_vis_find_leaf_at(float x, float y, float z) {
    if (!g_active_bsp) return -1;
    return aether_bsp_find_leaf(g_active_bsp, x, y, z);
}

int engine_bsp_vis_find_leaf(void) {
    return engine_bsp_vis_find_leaf_at(
        g_vis_view_origin[0], g_vis_view_origin[1], g_vis_view_origin[2]);
}

int engine_bsp_vis_update(void) {
    g_vis_index_count = 0;
    memset(&g_vis_stats, 0, sizeof g_vis_stats);
    if (!g_active_bsp || !g_active_mesh || !g_active_mesh->indices) return 0;

    u32 need = g_active_mesh->index_count;
    if (need == 0) return 0;
    if (g_vis_index_cap < need) {
        u32 *nbuf = (u32 *)realloc(g_vis_indices, (size_t)need * sizeof(u32));
        if (!nbuf) return 0;
        g_vis_indices = nbuf;
        g_vis_index_cap = need;
    }

    aether_bsp_vis_mode_t mode = g_vis_force_full ? AETHER_BSP_VIS_FORCE_FULL : g_vis_mode;
    g_vis_index_count = aether_bsp_vis_cull_mesh(
        g_active_bsp, g_active_mesh,
        g_vis_view_origin[0], g_vis_view_origin[1], g_vis_view_origin[2],
        mode, g_vis_indices, g_vis_index_cap, &g_vis_stats);
    g_vis_stats.force_full_vis = g_vis_force_full || (mode == AETHER_BSP_VIS_FORCE_FULL);

    if (g_renderer) {
        aether_render_features_t *feat = aether_renderer_features(g_renderer);
        if (feat) {
            aether_world_render_set_surface_count(&feat->world,
                g_active_mesh->index_count / 3u);
            aether_world_render_set_visible_surface_count(&feat->world,
                g_vis_index_count / 3u);
        }
    }
    return (int)g_vis_index_count;
}

int engine_bsp_vis_visible_index_count(void) { return (int)g_vis_index_count; }
int engine_bsp_vis_visible_face_count(void)  { return (int)g_vis_stats.visible_faces; }
int engine_bsp_vis_total_face_count(void)    {
    return g_active_bsp ? (int)aether_bsp_face_count(g_active_bsp) : (int)g_vis_stats.total_faces;
}
int engine_bsp_vis_visible_leaf_count(void)  { return (int)g_vis_stats.visible_leaf_count; }

int engine_bsp_vis_copy_indices(uint32_t *out, int max_indices) {
    if (!out || max_indices <= 0) return 0;
    if (!g_vis_indices || g_vis_index_count == 0) {
        /* Fallback: full mesh so Metal still draws if update was skipped. */
        return engine_bsp_mesh_copy_indices(out, max_indices);
    }
    int n = (int)g_vis_index_count;
    if (n > max_indices) n = max_indices;
    memcpy(out, g_vis_indices, (size_t)n * sizeof(u32));
    return n;
}

/* ---------- Lightmap ---------- */
static aether_lightmap_t *bridge_lightmap(void) {
    if (!g_renderer) return NULL;
    aether_render_features_t *f = aether_renderer_features(g_renderer);
    return f ? &f->lightmap : NULL;
}

int engine_lightmap_enabled(void) {
    aether_lightmap_t *lm = bridge_lightmap();
    return (lm && aether_lightmap_is_enabled(lm)) ? 1 : 0;
}

void engine_lightmap_set_enabled(bool enabled) {
    aether_lightmap_t *lm = bridge_lightmap();
    if (lm) aether_lightmap_enable(lm, enabled);
}

int engine_lightmap_width(void) {
    aether_lightmap_t *lm = bridge_lightmap();
    return lm ? (int)aether_lightmap_width(lm) : 0;
}

int engine_lightmap_height(void) {
    aether_lightmap_t *lm = bridge_lightmap();
    return lm ? (int)aether_lightmap_height(lm) : 0;
}

int engine_lightmap_is_stub(void) {
    aether_lightmap_t *lm = bridge_lightmap();
    return (lm && aether_lightmap_is_stub(lm)) ? 1 : 0;
}

int engine_lightmap_copy_rgba(unsigned char *out, int max_bytes) {
    aether_lightmap_t *lm = bridge_lightmap();
    if (!lm || !out || max_bytes <= 0) return 0;
    return (int)aether_lightmap_copy_rgba(lm, out, (u32)max_bytes);
}

int engine_lightmap_bake_active_mesh(void) {
    aether_lightmap_t *lm = bridge_lightmap();
    if (!lm || !g_active_mesh) return 0;
    return aether_lightmap_bake_mesh_stub(lm, g_active_mesh) == AETHER_OK ? 1 : 0;
}

/* ---------- Texture / WAD diagnostics ---------- */
int engine_texture_dump_wad(const char *wad_vpath) {
    if (!g_fs || !wad_vpath) return 0;
    u32 sz = aether_fs_read_file(g_fs, wad_vpath, NULL, 0);
    if (sz == 0 || sz > 256u*1024u*1024u) return 0;
    u8 *buf = (u8*)malloc(sz); if (!buf) return 0;
    u32 got = aether_fs_read_file(g_fs, wad_vpath, buf, sz);
    if (got != sz) { free(buf); return 0; }
    aether_wad_t *w = aether_wad_load_from_memory(buf, sz, wad_vpath);
    free(buf); if (!w) return 0;
    aether_wad_dump(w); aether_wad_free(w); return 1;
}
int engine_texture_dump_bsp_miptex(void) {
    if (!g_fs) return 0;
    const char *vp = "maps/c0a0.bsp";
    u32 sz = aether_fs_read_file(g_fs, vp, NULL, 0);
    if (sz == 0) return 0;
    u8 *buf = (u8*)malloc(sz); if (!buf) return 0;
    u32 got = aether_fs_read_file(g_fs, vp, buf, sz);
    if (got != sz) { free(buf); return 0; }
    aether_bsp_t *b = aether_bsp_load_from_memory(buf, sz, vp);
    free(buf); if (!b) return 0;
    aether_bsp_miptex_dump(b); aether_bsp_free(b); return 1;
}
int engine_texture_summary_text(char *out_buf, int out_cap) {
    if (!g_fs || !out_buf || out_cap <= 0) return -1;
    int w = 0;
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "STEP 15A — Texture diagnostics\n\n");
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
                                          "  • %s  (%ux%u)\n", info.name, info.width, info.height);
                        }
                    }
                    if (count > shown)
                        w += snprintf(out_buf + w, (size_t)(out_cap - w), "  … +%u more\n", count - shown);
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
    u8 *wbuf = (u8*)malloc(wad_sz); if (!wbuf) return 1;
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
                  "WAD: halflife.wad (%u bytes)\n  total lumps : %u\n  miptex      : %u\n  palette     : %u\n  other       : %u\n",
                  wad_sz, total, miptex, palette, other);
    aether_wad_free(wad);
    return 1;
}

/* ---------- Texture atlas ---------- */
int engine_texture_build_atlas(void)      { return g_atlas ? 1 : 0; }
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

/* ---------- MDL ---------- */
int engine_mdl_dump_vfs(const char *mdl_vpath) {
    if (!g_fs || !mdl_vpath) return 0;
    u32 sz = aether_fs_read_file(g_fs, mdl_vpath, NULL, 0);
    if (sz == 0 || sz > 64u*1024u*1024u) return 0;
    u8 *buf = (u8*)malloc(sz);
    if (!buf) return 0;
    u32 got = aether_fs_read_file(g_fs, mdl_vpath, buf, sz);
    if (got != sz) { free(buf); return 0; }
    aether_mdl_t *m = aether_mdl_load_from_memory(buf, sz, mdl_vpath);
    free(buf);
    if (!m) return 0;
    aether_mdl_dump(m);
    aether_mdl_free(m);
    return 1;
}

int engine_mdl_summary_text(const char *mdl_vpath, char *out_buf, int out_cap) {
    if (!g_fs || !mdl_vpath || !out_buf || out_cap <= 0) return -1;
    u32 sz = aether_fs_read_file(g_fs, mdl_vpath, NULL, 0);
    if (sz == 0) {
        snprintf(out_buf, (size_t)out_cap, "❌ MDL NOT FOUND\n\nPath: %s", mdl_vpath);
        return 0;
    }
    if (sz > 64u*1024u*1024u) {
        snprintf(out_buf, (size_t)out_cap, "MDL too large: %u bytes", sz);
        return -1;
    }
    u8 *buf = (u8*)malloc(sz);
    if (!buf) return -1;
    u32 got = aether_fs_read_file(g_fs, mdl_vpath, buf, sz);
    if (got != sz) { free(buf); return -1; }
    aether_mdl_t *m = aether_mdl_load_from_memory(buf, sz, mdl_vpath);
    free(buf);
    if (!m) {
        snprintf(out_buf, (size_t)out_cap, "❌ MDL PARSE FAILED: %s", mdl_vpath);
        return -2;
    }

    const aether_mdl_info_t *info = aether_mdl_info(m);
    int w = 0;
    w += snprintf(out_buf + w, (size_t)(out_cap - w),
                  "✅ MDL Parsed Successfully\n\nFile: %s\nSize: %u bytes\n\n"
                  "Name       : %s\nBones      : %d\nBodyparts  : %d\nTextures   : %d\n"
                  "Sequences  : %d\nHitboxes   : %d\nAttachments: %d\n\n",
                  mdl_vpath, sz, info->name, info->bone_count, info->bodypart_count,
                  info->texture_count, info->sequence_count, info->hitbox_count,
                  info->attachment_count);

    if (info->texture_count > 0) {
        w += snprintf(out_buf + w, (size_t)(out_cap - w), "Textures:\n");
        int shown = info->texture_count > 5 ? 5 : info->texture_count;
        for (int i = 0; i < shown; ++i) {
            const aether_mdl_skin_t *s = aether_mdl_skin_at(m, i);
            if (s) w += snprintf(out_buf + w, (size_t)(out_cap - w),
                                 "  • %s (%dx%d)\n", s->name, s->width, s->height);
        }
        if (info->texture_count > shown)
            w += snprintf(out_buf + w, (size_t)(out_cap - w), "  … +%d more\n",
                          info->texture_count - shown);
    }
    aether_mdl_free(m);
    return 1;
}

int engine_mdl_mesh_build(const char *mdl_vpath) {
    if (!g_fs || !mdl_vpath) return 0;
    if (g_mdl_mesh) { aether_mdl_geometry_free(g_mdl_mesh); g_mdl_mesh = NULL; }

    u32 sz = aether_fs_read_file(g_fs, mdl_vpath, NULL, 0);
    if (sz == 0 || sz > 64u*1024u*1024u) return 0;
    u8 *buf = (u8*)malloc(sz);
    if (!buf) return 0;
    u32 got = aether_fs_read_file(g_fs, mdl_vpath, buf, sz);
    if (got != sz) { free(buf); return 0; }

    aether_mdl_t *mdl = aether_mdl_load_from_memory(buf, sz, mdl_vpath);
    free(buf);
    if (!mdl) return 0;

    aether_model_mesh_t *mesh = NULL;
    aether_result_t r = aether_mdl_geometry_extract(mdl, &mesh);
    aether_mdl_free(mdl);
    if (r != AETHER_OK || !mesh) return 0;

    g_mdl_mesh = mesh;

    float fwd[3] = { 1.0f, 0.0f, 0.0f };
    engine_player_get_forward(fwd);
    float ply[3] = { 0, 0, 0 };
    engine_player_get_position(ply);
    g_mdl_render_pos[0] = ply[0] + fwd[0] * 200.0f;
    g_mdl_render_pos[1] = ply[1] + fwd[1] * 200.0f;
    g_mdl_render_pos[2] = ply[2] + fwd[2] * 200.0f;
    return 1;
}

int  engine_mdl_mesh_vertex_count(void)   { return g_mdl_mesh ? (int)g_mdl_mesh->vertex_count   : 0; }
int  engine_mdl_mesh_triangle_count(void) { return g_mdl_mesh ? (int)g_mdl_mesh->triangle_count : 0; }

void engine_mdl_mesh_get_bounds(float mn[3], float mx[3], float ctr[3]) {
    if (!g_mdl_mesh) return;
    if (mn)  { mn[0]=g_mdl_mesh->bounds_min[0]; mn[1]=g_mdl_mesh->bounds_min[1]; mn[2]=g_mdl_mesh->bounds_min[2]; }
    if (mx)  { mx[0]=g_mdl_mesh->bounds_max[0]; mx[1]=g_mdl_mesh->bounds_max[1]; mx[2]=g_mdl_mesh->bounds_max[2]; }
    if (ctr) { ctr[0]=g_mdl_mesh->bounds_center[0]; ctr[1]=g_mdl_mesh->bounds_center[1]; ctr[2]=g_mdl_mesh->bounds_center[2]; }
}

int engine_mdl_mesh_copy_positions(float *out, int max_floats) {
    if (!g_mdl_mesh || !out || max_floats <= 0) return 0;
    int n = (int)(g_mdl_mesh->vertex_count * 3);
    if (n > max_floats) n = max_floats;
    memcpy(out, g_mdl_mesh->positions, (size_t)n * sizeof(f32));
    return n;
}
int engine_mdl_mesh_copy_normals(float *out, int max_floats) {
    if (!g_mdl_mesh || !out || max_floats <= 0) return 0;
    int n = (int)(g_mdl_mesh->vertex_count * 3);
    if (n > max_floats) n = max_floats;
    memcpy(out, g_mdl_mesh->normals, (size_t)n * sizeof(f32));
    return n;
}
int engine_mdl_mesh_copy_indices(uint32_t *out, int max_idx) {
    if (!g_mdl_mesh || !out || max_idx <= 0) return 0;
    int n = (int)(g_mdl_mesh->triangle_count * 3);
    if (n > max_idx) n = max_idx;
    memcpy(out, g_mdl_mesh->indices, (size_t)n * sizeof(u32));
    return n;
}

void engine_mdl_mesh_release(void) {
    if (g_mdl_mesh) { aether_mdl_geometry_free(g_mdl_mesh); g_mdl_mesh = NULL; }
}

void engine_mdl_mesh_get_render_pos(float out[3]) {
    out[0] = g_mdl_render_pos[0];
    out[1] = g_mdl_render_pos[1];
    out[2] = g_mdl_render_pos[2];
}

void engine_mdl_mesh_set_render_pos(float x, float y, float z) {
    g_mdl_render_pos[0] = x;
    g_mdl_render_pos[1] = y;
    g_mdl_render_pos[2] = z;
}

/* ---------- Entity diagnostics ---------- */
int engine_entity_dump_current_map(void) {
    if (!g_entity_mgr) {
        aether_log(AETHER_LOG_WARN, "bridge", "no entity manager");
        return 0;
    }
    aether_entity_mgr_dump(g_entity_mgr);
    return 1;
}

int engine_entity_summary_text(char *out_buf, int out_cap) {
    if (!out_buf || out_cap <= 0) return -1;

    int w = 0;
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "Entity diagnostics\n\n");

    if (!g_entity_mgr) {
        w += snprintf(out_buf + w, (size_t)(out_cap - w),
                      "No entities loaded.\n\nFirst load a map.");
        return 0;
    }

    u32 total = aether_entity_mgr_count(g_entity_mgr);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "Total entities: %u\n\n", total);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "Categories:\n");

    u32 monsters = 0, weapons = 0, items = 0, playerstart = 0, doors = 0, triggers = 0, other = 0;
    for (u32 i = 0; i < total; ++i) {
        const aether_entity_t *e = aether_entity_mgr_at(g_entity_mgr, i);
        if (!e) continue;
        if (strncmp(e->classname, "monster_", 8) == 0) monsters++;
        else if (strncmp(e->classname, "weapon_", 7) == 0) weapons++;
        else if (strncmp(e->classname, "item_", 5) == 0 || strncmp(e->classname, "ammo_", 5) == 0) items++;
        else if (strncmp(e->classname, "info_player", 11) == 0) playerstart++;
        else if (strncmp(e->classname, "func_door", 9) == 0) doors++;
        else if (strncmp(e->classname, "trigger_", 8) == 0) triggers++;
        else other++;
    }
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  MONSTER       : %u\n", monsters);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  WEAPON        : %u\n", weapons);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  ITEM/AMMO     : %u\n", items);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  PLAYER_START  : %u\n", playerstart);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  DOOR          : %u\n", doors);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  TRIGGER       : %u\n", triggers);
    w += snprintf(out_buf + w, (size_t)(out_cap - w), "  OTHER         : %u\n", other);

    aether_vec3_t ps_pos = {0,0,0};
    aether_vec3_t ps_ang = {0,0,0};
    if (aether_entity_get_player_start(g_entity_mgr, &ps_pos, &ps_ang) == AETHER_OK) {
        w += snprintf(out_buf + w, (size_t)(out_cap - w),
                      "\nPlayer start:\n  info_player_start @ (%.0f, %.0f, %.0f)\n",
                      ps_pos.x, ps_pos.y, ps_pos.z);
    } else {
        w += snprintf(out_buf + w, (size_t)(out_cap - w), "\nNo player start found\n");
    }

    w += snprintf(out_buf + w, (size_t)(out_cap - w), "\nFirst few monsters:\n");
    u32 shown = 0;
    for (u32 i = 0; i < total && shown < 5; ++i) {
        const aether_entity_t *e = aether_entity_mgr_at(g_entity_mgr, i);
        if (!e) continue;
        if (strncmp(e->classname, "monster_", 8) == 0) {
            w += snprintf(out_buf + w, (size_t)(out_cap - w),
                          "  %-24s @ (%.0f, %.0f, %.0f)\n",
                          e->classname, e->origin.x, e->origin.y, e->origin.z);
            shown++;
        }
    }
    if (shown == 0) w += snprintf(out_buf + w, (size_t)(out_cap - w), "  (none)\n");

    if (g_monsters_init) {
        w += snprintf(out_buf + w, (size_t)(out_cap - w),
                      "\nMonster registry: %u spawned, %u alive\n",
                      aether_monster_registry_count(&g_monsters),
                      aether_monster_registry_alive(&g_monsters));
    }
    return 1;
}

/* ---------- Entity spawning ---------- */
int engine_entity_spawn_current_map(void) {
    if (!g_entity_mgr) return 0;
    return (int)aether_entity_mgr_count(g_entity_mgr);
}
int engine_entity_count(void) {
    return g_entity_mgr ? (int)aether_entity_mgr_count(g_entity_mgr) : 0;
}
int engine_entity_monster_count(void) {
    if (!g_monsters_init) return 0;
    return (int)aether_monster_registry_count(&g_monsters);
}
int engine_entity_alive_monster_count(void) {
    if (!g_monsters_init) return 0;
    return (int)aether_monster_registry_alive(&g_monsters);
}

int engine_monster_positions_copy(float *out_xyz_flat, int max_monsters) {
    if (!g_monsters_init || !out_xyz_flat || max_monsters <= 0) return 0;
    int n = 0;
    for (u32 i = 0; i < g_monsters.count && n < max_monsters; ++i) {
        const aether_monster_t *m = &g_monsters.monsters[i];
        if (!m->entity) continue;
        out_xyz_flat[n*3 + 0] = m->entity->origin.x;
        out_xyz_flat[n*3 + 1] = m->entity->origin.y;
        out_xyz_flat[n*3 + 2] = m->entity->origin.z;
        n++;
    }
    return n;
}

int engine_monster_healths_copy(int *out_health, int max_monsters) {
    if (!g_monsters_init || !out_health || max_monsters <= 0) return 0;
    int n = 0;
    for (u32 i = 0; i < g_monsters.count && n < max_monsters; ++i) {
        if (!g_monsters.monsters[i].entity) continue;
        out_health[n] = (int)g_monsters.monsters[i].entity->health;
        n++;
    }
    return n;
}

/* ---------- Scoreboard / Chat ---------- */
void engine_scoreboard_init(void) { aether_scoreboard_init(&g_scoreboard); g_scoreboard_init = true; }
void engine_scoreboard_set_visible(bool v) { if (!g_scoreboard_init) engine_scoreboard_init(); aether_scoreboard_set_visible(&g_scoreboard, v); }
bool engine_scoreboard_visible(void) { return g_scoreboard.visible; }
int engine_scoreboard_count(void) { return (int)g_scoreboard.count; }
int engine_scoreboard_get_entry(int index, char *name, int name_cap, int *score, int *deaths, int *ping) {
    if (index < 0 || (u32)index >= g_scoreboard.count || !name || name_cap <= 0) return 0;
    const aether_scoreboard_entry_t *e = &g_scoreboard.entries[index];
    aether_str_copy(name, (size_t)name_cap, e->name);
    if (score) *score = e->score;
    if (deaths) *deaths = e->deaths;
    if (ping) *ping = e->ping_ms;
    return e->active ? 1 : 0;
}
void engine_scoreboard_demo_data(void) {
    engine_scoreboard_init();
    aether_scoreboard_entry_t demo[] = {
        {1, "Player", 12, 3, 42, true},
        {2, "Gordon", 9, 5, 55, true},
        {3, "Barney", 7, 6, 61, true},
        {4, "Scientist", 3, 8, 77, true}
    };
    memcpy(g_scoreboard.entries, demo, sizeof demo);
    g_scoreboard.count = 4;
}
void engine_chat_init(void) { aether_chat_init(&g_chat); g_chat_init = true; }
void engine_chat_set_visible(bool v) { if (!g_chat_init) engine_chat_init(); aether_chat_set_visible(&g_chat, v); }
bool engine_chat_visible(void) { return g_chat.visible; }
int engine_chat_count(void) { return (int)g_chat.count; }
int engine_chat_get_line(int index, char *text, int text_cap, unsigned int *player_id) {
    if (index < 0 || (u32)index >= g_chat.count || !text || text_cap <= 0) return 0;
    u32 start = (g_chat.count < AETHER_CHAT_LOG_SIZE) ? 0 : g_chat.head;
    const aether_chat_line_t *l = &g_chat.lines[(start + (u32)index) % AETHER_CHAT_LOG_SIZE];
    aether_str_copy(text, (size_t)text_cap, l->text);
    if (player_id) *player_id = l->player_id;
    return 1;
}
void engine_chat_add_text(const char *text) {
    if (!text || !text[0]) return;
    if (!g_chat_init) engine_chat_init();
    /* UI/runtime bridge: server transport can feed the same log through handle_packet. */
    aether_chat_add(&g_chat, 1, text, 0.0f);
}

/* ---------- VGUI / classic menu ---------- */
int engine_vgui_init(void) { return (int)aether_vgui_runtime_init(); }
void engine_vgui_shutdown(void) { aether_vgui_runtime_shutdown(); }
void engine_vgui_show_main(void) { aether_vgui_runtime_show_main(); }
void engine_vgui_show_options(void) { aether_vgui_runtime_show_options(); }
void engine_vgui_show_load_game(void) { aether_vgui_runtime_show_load_game(); }
void engine_vgui_show_multiplayer(void) { aether_vgui_runtime_show_multiplayer(); }
void engine_vgui_toggle_console(void) { aether_vgui_runtime_toggle_console(); }
bool engine_console_visible(void) { return aether_vgui_runtime_console_visible(); }
void engine_console_set_visible(bool visible) { if (aether_vgui_runtime_console_visible() != visible) aether_vgui_runtime_toggle_console(); }
int engine_console_execute(const char *line) { return aether_vgui_runtime_console_execute(line) == AETHER_OK ? 1 : 0; }
int engine_console_count(void) { return (int)aether_vgui_runtime_console_count(); }
int engine_console_get_line(int index, char *text, int text_cap, int *level) {
    if(index < 0 || !text || text_cap <= 0) return 0;
    const aether_vgui_console_line_t *line = aether_vgui_runtime_console_line((u32)index);
    if(!line) return 0;
    aether_str_copy(text,(size_t)text_cap,line->text);
    if(level) *level=(int)line->level;
    return 1;
}
const char *engine_console_input(void) { return aether_vgui_runtime_console_input(); }
void engine_console_set_input(const char *text) { (void)aether_vgui_runtime_console_set_input(text ? text : ""); }
bool engine_vgui_is_visible(void) { aether_vgui_t *v=aether_vgui_runtime_ui(); return v ? true : false; }
int engine_vgui_current_panel_text(char *out_buf, int out_cap) {
    if (!out_buf || out_cap <= 0) return 0;
    aether_vgui_t *v=aether_vgui_runtime_ui();
    if (!v) { out_buf[0]=0; return 0; }
    u32 active=aether_vgui_runtime_active_panel();
    aether_vgui_panel_t *panel=aether_vgui_panel(v,active);
    if (!panel) { out_buf[0]=0; return 0; }
    int written=snprintf(out_buf,(size_t)out_cap,"%s\n",panel->text);
    if (written<0) { out_buf[0]=0; return 0; }
    if (written>=out_cap) { out_buf[out_cap-1]=0; return out_cap-1; }
    for (u32 i=0; i<panel->child_count; ++i) {
        const aether_vgui_panel_t *p=aether_vgui_child_at(v,active,i);
        if (!p || !p->visible) continue;
        int n=snprintf(out_buf+written,(size_t)(out_cap-written),"%s\n",p->text);
        if(n<0) break;
        written += n;
        if(written>=out_cap){out_buf[out_cap-1]=0;return out_cap-1;}
    }
    return written;
}

int engine_vgui_item_count(void) {
    aether_vgui_t *v=aether_vgui_runtime_ui();
    if (!v) return 0;
    return (int)aether_vgui_child_count(v, aether_vgui_runtime_active_panel());
}

int engine_vgui_item_text(int index, char *out_buf, int out_cap) {
    if (!out_buf || out_cap <= 0 || index < 0) return 0;
    out_buf[0]=0;
    aether_vgui_t *v=aether_vgui_runtime_ui();
    if (!v) return 0;
    const aether_vgui_panel_t *p=aether_vgui_child_at(v, aether_vgui_runtime_active_panel(), (u32)index);
    if (!p || !p->visible) return 0;
    int n=snprintf(out_buf,(size_t)out_cap,"%s",p->text);
    if (n < 0) { out_buf[0]=0; return 0; }
    if (n >= out_cap) { out_buf[out_cap-1]=0; return out_cap-1; }
    return n;
}

int engine_vgui_item_type(int index) {
    if (index < 0) return -1;
    aether_vgui_t *v=aether_vgui_runtime_ui();
    if (!v) return -1;
    const aether_vgui_panel_t *p=aether_vgui_child_at(v, aether_vgui_runtime_active_panel(), (u32)index);
    return p ? (int)p->type : -1;
}

int engine_vgui_activate_item(int index) {
    if (index < 0) return 0;
    aether_vgui_t *v=aether_vgui_runtime_ui();
    if (!v) return 0;
    const aether_vgui_panel_t *p=aether_vgui_child_at(v, aether_vgui_runtime_active_panel(), (u32)index);
    if (!p) return 0;
    return aether_vgui_activate(v, p->id) == AETHER_OK ? 1 : 0;
}

int engine_vgui_new_game(void) {
    if (!g_game_manager) return 0;
    engine_launch_game("valve");
    return engine_bsp_mesh_build_or_synthetic("maps/c0a0.bsp");
}

/* ---------- Utility ---------- */
const char *engine_base_path(void) { return g_base_path; }
const char *engine_version(void)   { return AETHER_VERSION_STRING; }
