// EngineBridge.c — AetherEngine-iOS · Clean-room.
// Complete fixed version — all includes + globals + functions.

#include "EngineBridge.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "../../engine/core/AetherCore.h"
#include "../../engine/core/AetherEngine.h"
#include "../../engine/game/AetherGameManager.h"
#include "../../engine/input/AetherInput.h"
#include "../../engine/config/AetherSettings.h"
#include "../../engine/fs/AetherFS.h"
#include "../../engine/audio/AetherAudio.h"
#include "../../engine/audio/AetherWav.h"
#include "../../engine/map/AetherMapLoad.h"
#include "../../engine/render/AetherDecal.h"
#include "../../engine/render/AetherDynLight.h"
#include "../../engine/render/AetherSprite.h"
#include "../../engine/save/AetherSave.h"
#include "../../engine/net/AetherNet.h"
#include "../../engine/net/AetherNetClient.h"
#include "../../engine/net/AetherNetServer.h"
#include "../../engine/render/AetherRender.h"
#include "../../engine/render/AetherRenderFeatures.h"
#include "../../engine/render/AetherParticle.h"
#include "../../engine/render/AetherSky.h"
#include "../../engine/render/AetherWater.h"
#include "../../engine/render/AetherDepthPrepass.h"
#include "../../engine/net/AetherNetSpectator.h"
#include "../../engine/render/AetherFog.h"
#include "../../engine/render/AetherLightmap.h"
#include "../../engine/bsp/AetherBSP.h"
#include "../../engine/bsp/AetherBSPGeometry.h"
#include "../../engine/bsp/AetherBSPSynthetic.h"
#include "../../engine/bsp/AetherBSPVis.h"
#include "../../engine/render/AetherFrustum.h"
#include "../../engine/render/AetherWorld.h"
#include "../../engine/player/AetherPlayer.h"
#include "../../engine/player/AetherPlayerHealth.h"
#include "../../engine/player/AetherPlayerDamage.h"
#include "../../engine/player/AetherPlayerInventory.h"
#include "../../engine/client/hud/AetherHUD.h"
#include "../../engine/client/hud/AetherHealth.h"
#include "../../engine/client/hud/AetherAmmo.h"
#include "../../engine/client/hud/AetherCrosshair.h"
#include "../../engine/player/AetherCollision.h"
#include "../../engine/texture/AetherTexture.h"
#include "../../engine/model/AetherMDL.h"
#include "../../engine/model/AetherMDLGeometry.h"
#include "../../engine/model/AetherModelFixture.h"
#include "../../engine/render/AetherShadow.h"
#include "../../engine/render/AetherPostFX.h"
#include "../../engine/input/AetherInteract.h"
#include "../../engine/entity/AetherEntityBase.h"
#include "../../engine/entity/AetherEntitySpawn.h"
#include "../../engine/game/monsters/AetherMonster.h"
#include "../../engine/vgui/AetherVGUIRuntime.h"
#include "../../engine/net/AetherNetScoreboard.h"
#include "../../engine/net/AetherNetChat.h"
#include "../../engine/net/AetherNetSnapshot.h"
#include "../../engine/net/AetherNetDelta.h"
#include "../../engine/net/AetherNetInterp.h"
#include "../../engine/net/AetherNetPredict.h"
#include "../../engine/net/AetherNetCmd.h"
#include "../../engine/net/AetherLagComp.h"
#include "../../engine/game/weapons/AetherWeaponView.h"
#include "../../engine/game/weapons/AetherWeapon.h"
#include "../../engine/game/weapons/AetherWeaponFiring.h"
#include "../../engine/render/AetherMDLAnimation.h"
#include "../../engine/console/AetherCVar.h"
#include "../../engine/game/AetherManifest.h"

/* ---------- Globals ---------- */
static aether_engine_t         *g_engine       = NULL;
static aether_game_manager_t   *g_game_manager = NULL;
static aether_input_t          *g_input        = NULL;
static aether_settings_t       *g_settings     = NULL;
static aether_fs_t             *g_fs           = NULL;
static aether_audio_t          *g_audio        = NULL;
static int                     g_map_source    = 0; /* 1=file 2=synth */
static aether_dyn_lights_t     g_dynlights;
static int                     g_dynlights_init = 0;
static aether_net_server_t    *g_net_server   = NULL;
static aether_net_client_t    *g_net_client   = NULL;
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
static aether_cvar_registry_t  *g_cvars        = NULL;
static char                     g_settings_path[600] = {0};

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
    if (strcmp(n, "bodygroup_next") == 0 || strcmp(n, "bodygroup") == 0)
        return AETHER_ACTION_BODYGROUP_NEXT;
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

    g_cvars = aether_cvar_create();
    if (g_cvars) {
        aether_cvar_register(g_cvars, "s_master_volume", AETHER_CVAR_FLOAT, "1.0", AETHER_CVAR_ARCHIVE);
        aether_cvar_register(g_cvars, "s_music_volume", AETHER_CVAR_FLOAT, "0.7", AETHER_CVAR_ARCHIVE);
        aether_cvar_register(g_cvars, "s_effects_volume", AETHER_CVAR_FLOAT, "1.0", AETHER_CVAR_ARCHIVE);
        aether_cvar_register(g_cvars, "s_mute", AETHER_CVAR_BOOL, "0", AETHER_CVAR_ARCHIVE);
        aether_cvar_register(g_cvars, "in_look_sensitivity", AETHER_CVAR_FLOAT, "1.0", AETHER_CVAR_ARCHIVE);
        aether_cvar_register(g_cvars, "in_invert_y", AETHER_CVAR_BOOL, "0", AETHER_CVAR_ARCHIVE);
        aether_cvar_register(g_cvars, "r_fps_limit", AETHER_CVAR_INT, "120", AETHER_CVAR_ARCHIVE);
        aether_cvar_register(g_cvars, "r_brightness", AETHER_CVAR_FLOAT, "0.0", AETHER_CVAR_ARCHIVE);
        aether_cvar_register(g_cvars, "r_gamma", AETHER_CVAR_FLOAT, "1.0", AETHER_CVAR_ARCHIVE);
        aether_cvar_register(g_cvars, "touch_layout", AETHER_CVAR_INT, "0", AETHER_CVAR_ARCHIVE);
        aether_cvar_register(g_cvars, "touch_opacity", AETHER_CVAR_FLOAT, "0.75", AETHER_CVAR_ARCHIVE);
    }

    snprintf(g_settings_path, sizeof g_settings_path, "%s/aether.cfg", base_path);
    (void)aether_settings_load(g_settings, g_settings_path);

    g_game_manager = aether_game_manager_create(g_engine, base_path);
    if (g_game_manager) {
        aether_subsystem_t game_sub = aether_game_manager_as_subsystem(g_game_manager);
        (void)aether_engine_register_subsystem(g_engine, &game_sub);
    }

    if (aether_engine_start(g_engine) != AETHER_OK) return;

    {
        f32 vol = 1.0f; bool muted = false; f32 sens = 1.0f;
        aether_settings_get_float(g_settings, "s_master_volume", &vol);
        aether_settings_get_bool(g_settings, "s_mute", &muted);
        aether_settings_get_float(g_settings, "in_look_sensitivity", &sens);
        aether_audio_set_master_volume(g_audio, vol);
        aether_audio_set_mute(g_audio, muted);
        g_player.look_speed = 0.0035f * (sens > 0.01f ? sens : 1.0f);
        if (g_cvars) {
            char buf[64];
            snprintf(buf, sizeof buf, "%g", (double)vol);
            aether_cvar_set(g_cvars, "s_master_volume", buf);
            aether_cvar_set(g_cvars, "s_mute", muted ? "1" : "0");
            snprintf(buf, sizeof buf, "%g", (double)sens);
            aether_cvar_set(g_cvars, "in_look_sensitivity", buf);
        }
    }

    (void)aether_vgui_runtime_init();
    aether_scoreboard_init(&g_scoreboard);
    aether_chat_init(&g_chat);
    g_scoreboard_init = true;
    g_chat_init = true;
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
    if (g_cvars)        { aether_cvar_destroy(g_cvars); g_cvars = NULL; }
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

    (void)aether_fs_setup_game(g_fs, g_base_path, info->dir_name);
    aether_game_launch(g_game_manager);
    aether_player_health_reset(&g_player_health);
    aether_player_inv_reset(&g_player_inventory);
    g_hud_clip = 0;
    g_hud_clip_max = 0;
    if (g_hud_ammo) aether_hud_ammo_set_clip(g_hud_ammo, 0, 0);
    if (g_hud_crosshair) aether_hud_crosshair_set_spread(g_hud_crosshair, 0.0f);
}

void engine_stop_game(void) { if (g_game_manager) aether_game_shutdown(g_game_manager); }

void engine_host_frame(float dt) {
    if (g_engine && aether_engine_is_running(g_engine))
        (void)aether_engine_host_frame(g_engine, dt);
    if (g_audio) aether_audio_flush(g_audio);
}

int engine_is_running(void) {
    return (g_engine && aether_engine_is_running(g_engine)) ? 1 : 0;
}
unsigned long long engine_frame_count(void) {
    return g_engine ? (unsigned long long)aether_engine_frame_count(g_engine) : 0ULL;
}
double engine_elapsed(void) {
    return g_engine ? aether_engine_elapsed(g_engine) : 0.0;
}
float engine_last_dt(void) {
    return g_engine ? aether_engine_last_dt(g_engine) : 0.0f;
}

int engine_game_count(void) { return (int)aether_game_count(); }

int engine_game_info(int index, char *name, int name_cap,
                     char *dir, int dir_cap, char *start_map, int map_cap) {
    const aether_game_info_t *info = aether_game_at((u32)index);
    if (!info) return 0;
    if (name && name_cap > 0)
        aether_str_copy(name, (size_t)name_cap, info->display_name ? info->display_name : "");
    if (dir && dir_cap > 0)
        aether_str_copy(dir, (size_t)dir_cap, info->dir_name ? info->dir_name : "");
    if (start_map && map_cap > 0)
        aether_str_copy(start_map, (size_t)map_cap, info->start_map ? info->start_map : "");
    return 1;
}

int engine_game_select(const char *game_dir) {
    if (!g_game_manager || !game_dir) return 0;
    if (aether_game_state_get(g_game_manager) == AETHER_GAME_STATE_RUNNING)
        (void)aether_game_shutdown(g_game_manager);
    return aether_game_select_by_dir(g_game_manager, game_dir) == AETHER_OK ? 1 : 0;
}

const char *engine_game_active_dir(void) {
    return g_game_manager ? aether_game_active_dir(g_game_manager) : NULL;
}
const char *engine_game_start_map(void) {
    return g_game_manager ? aether_game_start_map(g_game_manager) : NULL;
}
int engine_game_state(void) {
    return g_game_manager ? (int)aether_game_state_get(g_game_manager) : 0;
}
int engine_game_data_present(const char *game_dir) {
    if (!g_game_manager || !game_dir) return 0;
    const aether_game_info_t *info = aether_game_info_by_dir(game_dir);
    if (!info) return 0;
    return aether_game_data_present(g_game_manager, info->id) ? 1 : 0;
}
unsigned long long engine_game_run_frames(void) {
    return g_game_manager ? (unsigned long long)aether_game_run_frames(g_game_manager) : 0ULL;
}
int engine_manifest_load_all(const char *dir_path) {
    if (!dir_path) return -1;
    aether_manifest_clear();
    return (int)aether_manifest_load_all(dir_path);
}
int engine_manifest_count(void) { return (int)aether_manifest_count(); }

/* ---------- Input ---------- */
void engine_input_set_move(float x, float y)   { if (g_input) aether_input_set_move(g_input, x, y); }
void engine_input_add_look(float dx, float dy) {
    if (!g_input) return;
    bool invert = false;
    if (g_settings) aether_settings_get_bool(g_settings, "in_invert_y", &invert);
    if (invert) dy = -dy;
    aether_input_add_look(g_input, dx, dy);
}
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
    /* Use / interact: eye-forward trace against entities + ground. */
    if (aether_input_just_pressed(g_input, AETHER_ACTION_USE)) {
        f32 hit[3]; char cls[64];
        aether_vec3_t eye = aether_player_eye_position(&g_player);
        int kind = engine_interact_trace(eye.x, eye.y, eye.z,
                                         g_player.yaw, g_player.pitch, 96.f,
                                         hit, cls, (int)sizeof cls);
        if (kind != 0) {
            aether_log(AETHER_LOG_INFO, "interact",
                       "use hit kind=%d class=%s at (%.1f,%.1f,%.1f)",
                       kind, cls, hit[0], hit[1], hit[2]);
        }
    }
    /* Simple weapon switch cycle on input edges. */
    (void)aether_player_inv_apply_weapon_input(
        &g_player_inventory,
        aether_input_just_pressed(g_input, AETHER_ACTION_WEAPON_NEXT) ? 1 : 0,
        aether_input_just_pressed(g_input, AETHER_ACTION_WEAPON_PREV) ? 1 : 0);
    aether_input_end_frame(g_input);

    /* Drown damage: only while drowning flag set (air depleted + eye under). */
    {
        bool drowning = aether_player_is_drowning(&g_player);
        f32 hp_before = aether_player_health_get(&g_player_health);
        aether_player_tick_drown(&g_player_health, dt, drowning);
        if (drowning && g_hud_health) {
            f32 lost = hp_before - aether_player_health_get(&g_player_health);
            if (lost > 0.0f)
                aether_hud_health_trigger_damage_flash(g_hud_health, lost);
        }
    }

    /* Fire / radiation hazard ticks (flags from set_* or LAVA/SLIME contents). */
    {
        bool on_fire = aether_player_is_on_fire(&g_player);
        bool in_rad  = aether_player_is_in_radiation(&g_player);
        if (on_fire || in_rad) {
            f32 hp_before = aether_player_health_get(&g_player_health);
            aether_player_tick_fire(&g_player_health, dt, on_fire);
            aether_player_tick_radiation(&g_player_health, dt, in_rad);
            if (g_hud_health) {
                f32 lost = hp_before - aether_player_health_get(&g_player_health);
                if (lost > 0.0f)
                    aether_hud_health_trigger_damage_flash(g_hud_health, lost);
            }
        }
    }

    /* Fall damage on land impact (pending set by player update; water soft = 0). */
    {
        f32 fall_dmg = aether_player_take_fall_damage(&g_player);
        if (fall_dmg > 0.0f) {
            aether_damage_event_t ev;
            memset(&ev, 0, sizeof ev);
            ev.amount = fall_dmg;
            ev.type = AETHER_DMG_FALL;
            aether_player_apply_damage(&g_player_health, &ev);
            if (g_hud_health)
                aether_hud_health_trigger_damage_flash(g_hud_health, fall_dmg);
        }
    }

    /* Enter/exit water splash → particle burst at mid-body. */
    {
        int splash = aether_player_take_splash_event(&g_player);
        if (splash == AETHER_SPLASH_ENTER || splash == AETHER_SPLASH_EXIT) {
            float sx = g_player.position.x;
            float sy = g_player.position.y;
            float sz = g_player.position.z + g_player.eye_height * 0.5f;
            (void)engine_particles_spawn_burst(sx, sy, sz, 24);
        }
    }

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
float engine_player_get_pitch(void) {
    return g_player.pitch + aether_player_view_punch_pitch(&g_player);
}
float engine_player_view_punch_pitch(void) {
    return aether_player_view_punch_pitch(&g_player);
}
float engine_player_fall_velocity(void) {
    return aether_player_fall_velocity(&g_player);
}
int engine_player_on_ground(void)   { return g_player.on_ground ? 1 : 0; }
float engine_player_get_step_height(void) { return g_player.step_height; }
void engine_player_set_step_height(float height) {
    if (height < 0.f) height = 0.f;
    g_player.step_height = height;
}
int engine_player_is_crouching(void) { return g_player.crouching ? 1 : 0; }
void engine_player_set_crouching(bool crouching) {
    if (crouching) {
        g_player.crouching = true;
        g_player.hull_index = 2;
        g_player.eye_height = 12.0f; /* CROUCH_EYE_HEIGHT */
    } else {
        /* Refuse stand-up if standing hull is solid at feet (low ceiling). */
        if (g_collision && aether_collision_point_in_solid(g_collision, g_player.position, 1))
            return;
        g_player.crouching = false;
        g_player.hull_index = 1;
        g_player.eye_height = 28.0f; /* DEFAULT_EYE_HEIGHT */
    }
}
int engine_player_hull_index(void) { return (int)g_player.hull_index; }
float engine_player_eye_height(void) { return g_player.eye_height; }
int engine_player_in_water(void) { return g_player.in_water ? 1 : 0; }
int engine_player_waterlevel(void) { return (int)aether_player_waterlevel(&g_player); }
int engine_player_eye_underwater(void) {
    return aether_player_eye_underwater(&g_player) ? 1 : 0;
}
float engine_player_air(void) { return aether_player_air(&g_player); }
float engine_player_air_max(void) { return g_player.air_max; }
int engine_player_is_drowning(void) {
    return aether_player_is_drowning(&g_player) ? 1 : 0;
}
int engine_player_take_splash(void) {
    return (int)aether_player_take_splash_event(&g_player);
}
void engine_player_trigger_splash(int splash_kind) {
    aether_player_trigger_splash(&g_player, splash_kind);
}
int engine_player_splash_burst(float x, float y, float z, int count) {
    if (count <= 0) count = 24;
    return engine_particles_spawn_burst(x, y, z, count);
}

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
int engine_collision_point_contents(float x, float y, float z, int hull_index) {
    if (!g_collision) return -1; /* AETHER_CONTENTS_EMPTY */
    aether_vec3_t p = { x, y, z };
    return (int)aether_collision_point_contents(g_collision, p, hull_index);
}
int engine_collision_point_in_solid(float x, float y, float z, int hull_index) {
    if (!g_collision) return 0;
    aether_vec3_t p = { x, y, z };
    return aether_collision_point_in_solid(g_collision, p, hull_index) ? 1 : 0;
}
int engine_collision_move(float from_x, float from_y, float from_z,
                          float to_x, float to_y, float to_z,
                          int hull_index, float max_step, float out_xyz[3]) {
    aether_vec3_t from = { from_x, from_y, from_z };
    aether_vec3_t to   = { to_x, to_y, to_z };
    bool on_ground = false;
    aether_vec3_t r = to;
    if (g_collision)
        r = aether_collision_move(g_collision, from, to, hull_index, max_step, &on_ground);
    if (out_xyz) { out_xyz[0] = r.x; out_xyz[1] = r.y; out_xyz[2] = r.z; }
    return on_ground ? 1 : 0;
}

/* ---------- HUD ---------- */
float engine_hud_health(void) { return aether_player_health_get(&g_player_health); }
float engine_hud_max_health(void) { return g_player_health.max_health; }
float engine_hud_armor(void) { return aether_player_health_get_armor(&g_player_health); }
float engine_hud_battery(void) { return aether_player_health_get_battery(&g_player_health); }
bool engine_hud_alive(void) { return aether_player_health_is_alive(&g_player_health); }
float engine_hud_air(void) { return aether_player_air(&g_player); }
float engine_hud_air_max(void) { return g_player.air_max; }
int engine_hud_drowning(void) {
    return aether_player_is_drowning(&g_player) ? 1 : 0;
}

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

/* ---------- Settings / CVars / Audio / FS ---------- */
void engine_settings_apply(void);
void engine_settings_save(const char *f) { if (g_settings && f) (void)aether_settings_save(g_settings, f); }
void engine_settings_load(const char *f) { if (g_settings && f) (void)aether_settings_load(g_settings, f); }

int engine_settings_save_default(void) {
    if (!g_settings || !g_settings_path[0]) return 0;
    return aether_settings_save(g_settings, g_settings_path) == AETHER_OK ? 1 : 0;
}
int engine_settings_load_default(void) {
    if (!g_settings || !g_settings_path[0]) return 0;
    aether_result_t r = aether_settings_load(g_settings, g_settings_path);
    if (r == AETHER_OK) engine_settings_apply();
    return r == AETHER_OK ? 1 : 0;
}

void engine_settings_apply(void) {
    if (!g_settings) return;
    f32 vol = 1.0f, music = 0.7f, fx = 1.0f, sens = 1.0f;
    bool muted = false, inv = false;
    aether_settings_get_float(g_settings, "s_master_volume", &vol);
    aether_settings_get_float(g_settings, "s_music_volume", &music);
    aether_settings_get_float(g_settings, "s_effects_volume", &fx);
    aether_settings_get_bool(g_settings, "s_mute", &muted);
    aether_settings_get_float(g_settings, "in_look_sensitivity", &sens);
    aether_settings_get_bool(g_settings, "in_invert_y", &inv);
    if (g_audio) {
        aether_audio_set_master_volume(g_audio, vol);
        aether_audio_set_channel_volume(g_audio, AETHER_AUDIO_CHANNEL_MUSIC, music);
        aether_audio_set_channel_volume(g_audio, AETHER_AUDIO_CHANNEL_EFFECTS, fx);
        aether_audio_set_mute(g_audio, muted);
    }
    g_player.look_speed = 0.0035f * (sens > 0.01f ? sens : 1.0f);
    if (g_cvars) {
        char buf[64];
        snprintf(buf, sizeof buf, "%g", (double)vol); aether_cvar_set(g_cvars, "s_master_volume", buf);
        snprintf(buf, sizeof buf, "%g", (double)music); aether_cvar_set(g_cvars, "s_music_volume", buf);
        snprintf(buf, sizeof buf, "%g", (double)fx); aether_cvar_set(g_cvars, "s_effects_volume", buf);
        aether_cvar_set(g_cvars, "s_mute", muted ? "1" : "0");
        snprintf(buf, sizeof buf, "%g", (double)sens); aether_cvar_set(g_cvars, "in_look_sensitivity", buf);
        aether_cvar_set(g_cvars, "in_invert_y", inv ? "1" : "0");
    }
}

int engine_settings_set_float(const char *key, float v) {
    if (!g_settings || !key) return 0;
    if (aether_settings_set_float(g_settings, key, v) != AETHER_OK) return 0;
    engine_settings_apply();
    return 1;
}
int engine_settings_set_int(const char *key, int v) {
    if (!g_settings || !key) return 0;
    if (aether_settings_set_int(g_settings, key, (i32)v) != AETHER_OK) return 0;
    engine_settings_apply();
    return 1;
}
int engine_settings_set_bool(const char *key, bool v) {
    if (!g_settings || !key) return 0;
    if (aether_settings_set_bool(g_settings, key, v) != AETHER_OK) return 0;
    engine_settings_apply();
    return 1;
}
float engine_settings_get_float(const char *key, float fallback) {
    f32 v = fallback;
    if (g_settings && key) aether_settings_get_float(g_settings, key, &v);
    return v;
}
int engine_settings_get_int(const char *key, int fallback) {
    i32 v = (i32)fallback;
    if (g_settings && key) aether_settings_get_int(g_settings, key, &v);
    return (int)v;
}
bool engine_settings_get_bool(const char *key, bool fallback) {
    bool v = fallback;
    if (g_settings && key) aether_settings_get_bool(g_settings, key, &v);
    return v;
}

int engine_cvar_set(const char *name, const char *value) {
    if (!g_cvars || !name || !value) return 0;
    return aether_cvar_set(g_cvars, name, value) == AETHER_OK ? 1 : 0;
}
float engine_cvar_float(const char *name, float fallback) {
    return g_cvars ? aether_cvar_float(g_cvars, name, fallback) : fallback;
}
int engine_cvar_int(const char *name, int fallback) {
    return g_cvars ? (int)aether_cvar_int(g_cvars, name, (i32)fallback) : fallback;
}
bool engine_cvar_bool(const char *name, bool fallback) {
    return g_cvars ? aether_cvar_bool(g_cvars, name, fallback) : fallback;
}

void engine_audio_init(void)                 { if (!g_audio) g_audio = aether_audio_create(); aether_audio_init(g_audio); }
void engine_audio_shutdown(void)             { if (g_audio) aether_audio_shutdown(g_audio); }
int  engine_audio_ready(void)                { return (g_audio && aether_audio_is_ready(g_audio)) ? 1 : 0; }
void engine_audio_flush(void)                { if (g_audio) aether_audio_flush(g_audio); }
void engine_audio_set_master_volume(float v) {
    if (g_audio) aether_audio_set_master_volume(g_audio, v);
    if (g_settings) (void)aether_settings_set_float(g_settings, "s_master_volume", v);
}
void engine_audio_set_mute(bool m) {
    if (g_audio) aether_audio_set_mute(g_audio, m);
    if (g_settings) (void)aether_settings_set_bool(g_settings, "s_mute", m);
}
void engine_audio_play(const char *p, float v, bool l) { if (g_audio && p) (void)aether_audio_play_effect(g_audio, p, v, l); }
void engine_audio_stop_all(void)             { if (g_audio) aether_audio_stop_all(g_audio); }

int engine_fs_root_count(void) { return g_fs ? (int)aether_fs_root_count(g_fs) : 0; }
int engine_fs_root_at(int index, char *out, int out_cap) {
    if (!g_fs || !out || out_cap <= 0) return 0;
    const char *r = aether_fs_root_at(g_fs, (u32)index);
    if (!r) return 0;
    aether_str_copy(out, (size_t)out_cap, r);
    return 1;
}
int engine_fs_exists(const char *vpath) {
    return (g_fs && vpath && aether_fs_exists(g_fs, vpath)) ? 1 : 0;
}

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
    int ok = bridge_activate_bsp(b, true);
    if (ok) {
        /* Align render water plane with synthetic CONTENTS_WATER pool (+Y). */
        aether_water_t *w = bridge_water();
        if (w) {
            aether_water_set_enabled(w, true);
            (void)aether_water_set_origin(w, AETHER_SYNTH_WATER_ORIGIN_X,
                                          AETHER_SYNTH_WATER_ORIGIN_Y);
            (void)aether_water_set_height(w, AETHER_SYNTH_WATER_SURFACE_Z);
            (void)aether_water_set_size(w, AETHER_SYNTH_WATER_HALF_SIZE);
        }
    }
    return ok;
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




/* ===== Batch: map / audio / decals / dynlights / save / net ===== */

static void ensure_dynlights(void) {
    if (!g_dynlights_init) {
        aether_dyn_lights_init(&g_dynlights);
        g_dynlights_init = 1;
    }
}

int engine_map_load(const char *vpath) {
    aether_map_load_result_t res;
    if (aether_map_load(g_fs, vpath, &res) != AETHER_OK || !res.bsp) return 0;
    if (g_active_bsp) { aether_bsp_free(g_active_bsp); g_active_bsp = NULL; }
    g_active_bsp = res.bsp;
    g_map_source = (res.source == AETHER_MAP_SOURCE_FILE) ? 1 : 2;
    g_mesh_is_synthetic = (g_map_source == 2);
    if (g_entity_mgr) { aether_entity_mgr_destroy(g_entity_mgr); g_entity_mgr = NULL; }
    g_entity_mgr = aether_entity_mgr_create();
    if (g_entity_mgr) {
        aether_entity_spawn_stats_t st;
        (void)aether_entity_spawn_from_bsp_ex(g_entity_mgr, g_active_bsp, &st);
        aether_log(AETHER_LOG_INFO, "bridge",
                   "map spawn total=%u lights=%u monsters=%u starts=%u world=%u",
                   st.total, st.lights, st.monsters, st.player_starts, st.worldspawn);
    }
    if (g_renderer && g_active_mesh) {
        aether_render_features_t *feat = aether_renderer_features(g_renderer);
        if (feat)
            (void)aether_lightmap_bake_from_bsp(&feat->lightmap, g_active_bsp, g_active_mesh);
    }
    return 1;
}

int engine_map_load_named(const char *map_name) {
    aether_map_load_result_t res;
    if (aether_map_load_named(g_fs, map_name, &res) != AETHER_OK || !res.bsp) return 0;
    if (g_active_bsp) { aether_bsp_free(g_active_bsp); g_active_bsp = NULL; }
    g_active_bsp = res.bsp;
    g_map_source = (res.source == AETHER_MAP_SOURCE_FILE) ? 1 : 2;
    g_mesh_is_synthetic = (g_map_source == 2);
    if (g_entity_mgr) { aether_entity_mgr_destroy(g_entity_mgr); g_entity_mgr = NULL; }
    g_entity_mgr = aether_entity_mgr_create();
    if (g_entity_mgr)
        (void)aether_entity_spawn_from_bsp(g_entity_mgr, g_active_bsp);
    return 1;
}

int engine_map_last_source(void) { return g_map_source; }

int engine_map_write_fixture(const char *abspath) {
    return aether_map_write_minimal_fixture(abspath) > 0 ? 1 : 0;
}

void engine_audio_set_platform_callback(void *fn, void *user) {
    if (!g_audio) return;
    aether_audio_set_platform_callback(g_audio, (aether_audio_platform_fn)fn, user);
}

int engine_audio_play_beep(float freq_hz, float duration_sec, float volume) {
    if (!g_audio) return 0;
    return aether_audio_play_beep(g_audio, freq_hz, duration_sec, volume) == AETHER_OK ? 1 : 0;
}

int engine_audio_submit_pcm16(const short *samples, int frames,
                              int sample_rate, int channels, float volume) {
    if (!g_audio || !samples || frames <= 0) return 0;
    aether_audio_buffer_t buf;
    buf.samples = samples;
    buf.frame_count = (u32)frames;
    buf.sample_rate = (u32)sample_rate;
    buf.channels = (u16)channels;
    buf.volume = volume;
    return aether_audio_submit_buffer(g_audio, &buf) == AETHER_OK ? 1 : 0;
}

int engine_wav_parse_header(const unsigned char *data, int size,
                            unsigned *out_rate, unsigned *out_channels,
                            unsigned *out_bits, unsigned *out_data_bytes) {
    aether_wav_info_t info;
    if (aether_wav_parse_header(data, (u32)size, &info) != AETHER_OK || !info.valid)
        return 0;
    if (out_rate) *out_rate = info.sample_rate;
    if (out_channels) *out_channels = info.channels;
    if (out_bits) *out_bits = info.bits_per_sample;
    if (out_data_bytes) *out_data_bytes = info.data_size;
    return 1;
}

static aether_decals_t *bridge_decals(void) {
    if (!g_renderer) return NULL;
    aether_render_features_t *f = aether_renderer_features(g_renderer);
    return f ? &f->decals : NULL;
}

int engine_decals_add(float x, float y, float z,
                      float nx, float ny, float nz, float size, float life) {
    aether_decals_t *d = bridge_decals();
    if (!d) return 0;
    f32 p[3] = {x, y, z};
    f32 n[3] = {nx, ny, nz};
    return aether_decals_add(d, p, n, size, life) == AETHER_OK ? 1 : 0;
}

int engine_decals_active_count(void) {
    aether_decals_t *d = bridge_decals();
    return d ? (int)aether_decals_active_count(d) : 0;
}

int engine_decals_copy_render(float *out_xyz_n_size_fade, int max_decals) {
    aether_decals_t *d = bridge_decals();
    if (!d || !out_xyz_n_size_fade || max_decals <= 0) return 0;
    return (int)aether_decals_copy_render(d, (aether_decal_vertex_t *)out_xyz_n_size_fade,
                                          (u32)max_decals);
}

int engine_dynlights_add(float x, float y, float z,
                         float r, float g, float b, float radius, float intensity) {
    ensure_dynlights();
    f32 p[3] = {x, y, z};
    f32 c[3] = {r, g, b};
    return aether_dyn_lights_add(&g_dynlights, p, c, radius, intensity) == AETHER_OK ? 1 : 0;
}

int engine_dynlights_active_count(void) {
    ensure_dynlights();
    return (int)aether_dyn_lights_active_count(&g_dynlights);
}

int engine_dynlights_copy_render(float *out_xyz_radius_rgb_i, int max_lights) {
    ensure_dynlights();
    if (!out_xyz_radius_rgb_i || max_lights <= 0) return 0;
    return (int)aether_dyn_lights_copy_render(&g_dynlights,
        (aether_dyn_light_vertex_t *)out_xyz_radius_rgb_i, (u32)max_lights);
}

int engine_dynlights_from_map_lights(void) {
    ensure_dynlights();
    aether_dyn_lights_clear(&g_dynlights);
    if (!g_entity_mgr) return 0;
    u32 n = 0;
    for (u32 i = 0; i < aether_entity_mgr_count(g_entity_mgr); ++i) {
        const aether_entity_t *e = aether_entity_mgr_at(g_entity_mgr, i);
        if (!e || strncmp(e->classname, "light", 5) != 0) continue;
        f32 p[3] = {e->origin.x, e->origin.y, e->origin.z};
        f32 c[3] = {1.f, 0.95f, 0.8f};
        if (aether_dyn_lights_add(&g_dynlights, p, c, 200.f, 1.f) == AETHER_OK) n++;
    }
    return (int)n;
}

int engine_lightmap_bake_from_active_bsp(void) {
    if (!g_renderer || !g_active_bsp || !g_active_mesh) return 0;
    aether_render_features_t *f = aether_renderer_features(g_renderer);
    if (!f) return 0;
    return aether_lightmap_bake_from_bsp(&f->lightmap, g_active_bsp, g_active_mesh) == AETHER_OK ? 1 : 0;
}

int engine_save_game(const char *filepath) {
    if (!filepath) return 0;
    aether_save_ctx_t ctx;
    memset(&ctx, 0, sizeof ctx);
    ctx.game_id = 0;
    ctx.map_name = g_mesh_is_synthetic ? "synthetic:demo" : "maps/loaded.bsp";
    ctx.game_name = "aether";
    ctx.play_time_seconds = 12;
    ctx.player_health = &g_player_health;
    ctx.player_inventory = &g_player_inventory;
    ctx.player_origin = g_player.position;
    ctx.player_angles = (aether_vec3_t){g_player.pitch, g_player.yaw, 0};
    ctx.entities = g_entity_mgr;
    ctx.world_time = 1.0f;
    ctx.world_flags = 0;
    return aether_save_write(filepath, &ctx) == AETHER_OK ? 1 : 0;
}

int engine_load_game(const char *filepath) {
    if (!filepath) return 0;
    aether_save_ctx_t ctx;
    memset(&ctx, 0, sizeof ctx);
    ctx.player_health = &g_player_health;
    ctx.player_inventory = &g_player_inventory;
    ctx.entities = g_entity_mgr;
    if (aether_save_read(filepath, &ctx) != AETHER_OK) return 0;
    g_player.position = ctx.player_origin;
    g_player.yaw = ctx.player_angles.y;
    g_player.pitch = ctx.player_angles.x;
    return 1;
}

int engine_net_listen(int port) {
    if (g_net_server) { aether_net_server_destroy(g_net_server); g_net_server = NULL; }
    g_net_server = aether_net_server_create((u16)port, 4);
    if (!g_net_server) return 0;
    aether_net_server_set_info(g_net_server, "AetherSmoke", "aether_demo", 30, 10);
    return 1;
}

int engine_net_connect_localhost(int port) {
    if (g_net_client) { aether_net_client_destroy(g_net_client); g_net_client = NULL; }
    g_net_client = aether_net_client_create();
    if (!g_net_client) return 0;
    return aether_net_client_connect(g_net_client, "127.0.0.1", (u16)port) == AETHER_OK ? 1 : 0;
}

int engine_net_handshake_tick(float dt) {
    if (g_net_server) aether_net_server_tick(g_net_server, dt);
    if (g_net_client) aether_net_client_tick(g_net_client, dt);
    return engine_net_is_connected();
}

int engine_net_is_connected(void) {
    if (!g_net_client) return 0;
    aether_net_state_t st = aether_net_client_state(g_net_client);
    return (st == AETHER_NET_STATE_CONNECTED || st == AETHER_NET_STATE_ACTIVE) ? 1 : 0;
}

void engine_net_shutdown(void) {
    if (g_net_client) {
        aether_net_client_disconnect(g_net_client);
        aether_net_client_destroy(g_net_client);
        g_net_client = NULL;
    }
    if (g_net_server) {
        aether_net_server_destroy(g_net_server);
        g_net_server = NULL;
    }
}


/* ===== Batch: UV / WAV / decals / net / hazards ===== */
int engine_audio_play_wav_data(const unsigned char *data, int size, float volume) {
    if (!g_audio || !data || size <= 0) return 0;
    return aether_audio_play_wav_data(g_audio, data, (u32)size, volume) == AETHER_OK ? 1 : 0;
}

int engine_decals_copy_quads(float *out_xyz_uv_fade_rgba, int max_verts) {
    aether_decals_t *d = bridge_decals();
    if (!d || !out_xyz_uv_fade_rgba || max_verts <= 0) return 0;
    return (int)aether_decals_copy_quads(d, (aether_decal_quad_vertex_t *)out_xyz_uv_fade_rgba,
                                         (u32)max_verts);
}

int engine_sprite_copy_quad(float x, float y, float z, float w, float h,
                            float *out_xyz_uv_rgba, int max_verts) {
    if (!out_xyz_uv_rgba || max_verts < 6) return 0;
    aether_sprite_t spr;
    aether_sprite_init(&spr);
    aether_sprite_set_position(&spr, x, y, z);
    aether_sprite_set_size(&spr, w, h);
    f32 col[4] = {0.3f, 0.9f, 0.4f, 0.85f};
    aether_sprite_set_color(&spr, col);
    return (int)aether_sprite_copy_quad(&spr, NULL, NULL,
                                        (aether_sprite_quad_vertex_t *)out_xyz_uv_rgba,
                                        (u32)max_verts);
}

int engine_dynlights_apply_mesh_tint(float *out_rgb, int max_floats) {
    ensure_dynlights();
    if (!g_active_mesh || !out_rgb || max_floats <= 0) return 0;
    return (int)aether_dyn_lights_apply_mesh_tint(&g_dynlights, g_active_mesh,
                                                  out_rgb, (u32)max_floats);
}

int engine_dynlights_modulate_lightmap(void) {
    ensure_dynlights();
    aether_lightmap_t *lm = bridge_lightmap();
    if (!lm) return 0;
    return aether_dyn_lights_modulate_lightmap(&g_dynlights, lm) == AETHER_OK ? 1 : 0;
}

int engine_net_snapshot_demo_apply(unsigned tick) {
    if (!g_scoreboard_init) engine_scoreboard_init();
    if (!g_chat_init) engine_chat_init();
    aether_net_snapshot_t snap;
    aether_net_snapshot_make_demo(&snap, (u32)tick, (f32)tick * 0.05f);
    aether_net_snapshot_apply_hud(&snap, &g_scoreboard, &g_chat, (f32)tick * 0.05f);
    return (int)g_scoreboard.count;
}

int engine_net_snapshot_scoreboard_count(void) {
    return (int)g_scoreboard.count;
}

int engine_player_set_on_fire(int on) {
    aether_player_set_on_fire(&g_player, on != 0);
    return 1;
}
int engine_player_is_on_fire(void) {
    return aether_player_is_on_fire(&g_player) ? 1 : 0;
}
int engine_player_set_in_radiation(int on) {
    aether_player_set_in_radiation(&g_player, on != 0);
    return 1;
}
int engine_player_is_in_radiation(void) {
    return aether_player_is_in_radiation(&g_player) ? 1 : 0;
}

int engine_lightmap_unpack_uvs_active(void) {
    aether_lightmap_t *lm = bridge_lightmap();
    if (!g_active_bsp || !g_active_mesh || !lm) return 0;
    return aether_lightmap_unpack_uvs_from_bsp(lm, g_active_bsp, g_active_mesh) == AETHER_OK ? 1 : 0;
}


static aether_lightstyles_t g_lightstyles;
static int g_lightstyles_init = 0;
static aether_shadow_t g_shadow;
static int g_shadow_init = 0;
static aether_postfx_t g_postfx;
static int g_postfx_init = 0;

static void ensure_lightstyles(void) {
    if (!g_lightstyles_init) { aether_lightstyles_init(&g_lightstyles); g_lightstyles_init = 1; }
}
static void ensure_shadow(void) {
    if (!g_shadow_init) { aether_shadow_init(&g_shadow, 512); g_shadow_init = 1; }
}
static void ensure_postfx(void) {
    if (!g_postfx_init) { aether_postfx_init(&g_postfx); g_postfx_init = 1; }
}

int engine_dynlights_fill_ubo(float *out_bytes, int max_floats) {
    ensure_dynlights();
    if (!out_bytes || max_floats < 4) return 0;
    aether_dyn_light_ubo_t ubo;
    u32 n = aether_dyn_lights_fill_ubo(&g_dynlights, &ubo);
    /* Pack: count + 3 pad + N*(8 floats) */
    u32 need = 4u + n * 8u;
    if ((u32)max_floats < need) {
        n = ((u32)max_floats >= 4u) ? ((u32)max_floats - 4u) / 8u : 0;
        need = 4u + n * 8u;
        ubo.count = n;
    }
    out_bytes[0] = (float)ubo.count;
    out_bytes[1] = out_bytes[2] = out_bytes[3] = 0.f;
    for (u32 i = 0; i < n; ++i) {
        float *d = out_bytes + 4 + i * 8;
        d[0] = ubo.lights[i].x; d[1] = ubo.lights[i].y; d[2] = ubo.lights[i].z;
        d[3] = ubo.lights[i].radius;
        d[4] = ubo.lights[i].r; d[5] = ubo.lights[i].g; d[6] = ubo.lights[i].b;
        d[7] = ubo.lights[i].intensity;
    }
    return (int)need;
}
int engine_dynlights_ubo_count(void) {
    ensure_dynlights();
    return (int)aether_dyn_lights_active_count(&g_dynlights);
}

int engine_decals_project_onto_mesh(float *out_xyz_uv_fade_rgba, int max_verts) {
    aether_decals_t *d = bridge_decals();
    if (!d || !g_active_mesh || !out_xyz_uv_fade_rgba || max_verts < 3) return 0;
    return (int)aether_decals_project_onto_mesh(d, g_active_mesh,
        (aether_decal_quad_vertex_t *)out_xyz_uv_fade_rgba, (u32)max_verts);
}

int engine_net_snapshot_live_tick(float dt) {
    if (g_net_server) aether_net_server_tick(g_net_server, dt);
    if (g_net_client) {
        aether_net_client_tick(g_net_client, dt);
        if (!g_scoreboard_init) engine_scoreboard_init();
        if (!g_chat_init) engine_chat_init();
        aether_net_client_apply_snapshot_hud(g_net_client, &g_scoreboard, &g_chat,
                                             (f32)aether_net_time());
        return (int)aether_net_client_snapshot_count(g_net_client);
    }
    return 0;
}
int engine_net_snapshot_ingested_count(void) {
    return g_net_client ? (int)aether_net_client_snapshot_count(g_net_client) : 0;
}

int engine_lightstyles_update(float time) {
    ensure_lightstyles();
    aether_lightstyles_update(&g_lightstyles, time);
    return 1;
}
float engine_lightstyles_value(unsigned index) {
    ensure_lightstyles();
    return aether_lightstyles_value(&g_lightstyles, index);
}
int engine_lightmap_apply_style(unsigned style_index) {
    ensure_lightstyles();
    aether_lightmap_t *lm = bridge_lightmap();
    if (!lm) return 0;
    return aether_lightmap_apply_style(lm, &g_lightstyles, style_index) == AETHER_OK ? 1 : 0;
}

int engine_mdl_write_fixture(const char *filepath) {
    return (int)aether_mdl_write_fixture_file(filepath);
}
int engine_sprite_write_fixture(const char *filepath) {
    return (int)aether_sprite_write_fixture_file(filepath);
}
int engine_mdl_load_fixture_file(const char *filepath) {
    if (!filepath) return 0;
    aether_mdl_t *m = aether_mdl_load(filepath);
    if (!m) return 0;
    if (g_mdl_mesh) { aether_mdl_geometry_free(g_mdl_mesh); g_mdl_mesh = NULL; }
    aether_model_mesh_t *mesh = NULL;
    aether_result_t r = aether_mdl_geometry_extract(m, &mesh);
    aether_mdl_free(m);
    if (r != AETHER_OK || !mesh) {
        /* Header-valid fixture may have 0 tris — still count as loaded path. */
        return 1;
    }
    g_mdl_mesh = mesh;
    return (int)mesh->vertex_count;
}
int engine_sprite_fixture_quad(float x, float y, float z, float w, float h,
                               float *out_xyz_uv_rgba, int max_verts) {
    return engine_sprite_copy_quad(x, y, z, w, h, out_xyz_uv_rgba, max_verts);
}

int engine_shadow_copy_blob(float px, float py, float ground_z, float radius,
                            float *out_xyz_uv_alpha_pad, int max_verts) {
    ensure_shadow();
    if (!out_xyz_uv_alpha_pad || max_verts < 6) return 0;
    return (int)aether_shadow_copy_blob(&g_shadow, px, py, ground_z, radius,
        (aether_blob_shadow_vertex_t *)out_xyz_uv_alpha_pad, (u32)max_verts);
}

int engine_postfx_set_from_settings(void) {
    ensure_postfx();
    f32 bright = 0.f, gamma = 1.f;
    if (g_settings) {
        aether_settings_get_float(g_settings, "r_brightness", &bright);
        aether_settings_get_float(g_settings, "r_gamma", &gamma);
    }
    if (g_cvars) {
        bright = aether_cvar_float(g_cvars, "r_brightness", bright);
        gamma = aether_cvar_float(g_cvars, "r_gamma", gamma);
    }
    aether_postfx_set_from_cvars(&g_postfx, bright, gamma);
    return 1;
}
int engine_postfx_copy_fullscreen(float *out_xyz_uv, int max_verts) {
    if (!out_xyz_uv || max_verts < 6) return 0;
    return (int)aether_postfx_copy_fullscreen((aether_postfx_vertex_t *)out_xyz_uv, (u32)max_verts);
}
float engine_postfx_brightness(void) { ensure_postfx(); return g_postfx.brightness; }
float engine_postfx_gamma(void) { ensure_postfx(); return g_postfx.gamma; }

int engine_interact_trace(float eye_x, float eye_y, float eye_z,
                          float yaw_deg, float pitch_deg, float max_dist,
                          float *out_hit_xyz, char *out_classname, int classname_cap) {
    f32 eye[3] = {eye_x, eye_y, eye_z};
    f32 fwd[3];
    aether_interact_forward_from_view(yaw_deg, pitch_deg, fwd);
    aether_interact_target_t targets[8];
    u32 tc = 0;
    if (g_entity_mgr) {
        u32 n = aether_entity_mgr_count(g_entity_mgr);
        for (u32 i = 0; i < n && tc < 8; ++i) {
            const aether_entity_t *e = aether_entity_mgr_at(g_entity_mgr, i);
            if (!e) continue;
            aether_interact_target_t *tg = &targets[tc++];
            tg->id = (i32)i;
            aether_str_copy(tg->classname, sizeof tg->classname, e->classname);
            tg->mins[0] = e->origin.x - 16.f; tg->mins[1] = e->origin.y - 16.f; tg->mins[2] = e->origin.z;
            tg->maxs[0] = e->origin.x + 16.f; tg->maxs[1] = e->origin.y + 16.f; tg->maxs[2] = e->origin.z + 72.f;
            tg->usable = true;
        }
    }
    aether_interact_hit_t hit;
    aether_interact_trace(eye, fwd, max_dist, 0.f, targets, tc, &hit);
    if (out_hit_xyz) {
        out_hit_xyz[0] = hit.point[0];
        out_hit_xyz[1] = hit.point[1];
        out_hit_xyz[2] = hit.point[2];
    }
    if (out_classname && classname_cap > 0) {
        aether_str_copy(out_classname, (size_t)classname_cap, hit.classname);
    }
    return (int)hit.kind;
}


/* ---------- Batch: postfx offscreen / lightmap pingpong / delta+predict ---------- */
static aether_net_interp_t g_net_interp;
static aether_net_predict_t g_net_predict;
static int g_net_interp_init = 0;
static int g_net_predict_init = 0;
static aether_net_snapshot_t g_delta_state;

static void ensure_net_interp(void) {
    if (!g_net_interp_init) { aether_net_interp_init(&g_net_interp); g_net_interp_init = 1; }
}
static void ensure_net_predict(void) {
    if (!g_net_predict_init) { aether_net_predict_init(&g_net_predict, 1); g_net_predict_init = 1; }
}

int engine_postfx_ensure_offscreen(int width, int height) {
    ensure_postfx();
    if (width <= 0 || height <= 0) return 0;
    return aether_postfx_ensure_offscreen(&g_postfx, (u32)width, (u32)height) == AETHER_OK ? 1 : 0;
}
int engine_postfx_has_offscreen(void) {
    ensure_postfx();
    return aether_postfx_has_offscreen(&g_postfx) ? 1 : 0;
}
int engine_postfx_fill_uniforms(float *out4) {
    ensure_postfx();
    if (!out4) return 0;
    aether_postfx_uniforms_t u;
    aether_postfx_fill_uniforms(&g_postfx, &u);
    out4[0] = u.brightness; out4[1] = u.gamma; out4[2] = u.exposure; out4[3] = u.enabled;
    return 4;
}
int engine_lightmap_capture_base(void) {
    aether_lightmap_t *lm = bridge_lightmap();
    if (!lm) return 0;
    return aether_lightmap_capture_base(lm) == AETHER_OK ? 1 : 0;
}
int engine_lightmap_apply_style_pingpong(unsigned style_index) {
    ensure_lightstyles();
    aether_lightmap_t *lm = bridge_lightmap();
    if (!lm) return 0;
    return aether_lightmap_apply_style_pingpong(lm, &g_lightstyles, style_index) == AETHER_OK ? 1 : 0;
}
int engine_lightmap_has_base(void) {
    aether_lightmap_t *lm = bridge_lightmap();
    return lm && aether_lightmap_has_base(lm) ? 1 : 0;
}
int engine_dynlights_fill_array(float *out, int max_floats) {
    if (!out || max_floats < 4) return 0;
    return (int)aether_dyn_lights_fill_array(&g_dynlights, out, (u32)max_floats);
}
int engine_decals_clip_to_world(float *out_xyz_uv_fade_rgba, int max_verts) {
    aether_decals_t *d = bridge_decals();
    if (!d || !g_active_mesh || !out_xyz_uv_fade_rgba || max_verts < 3) return 0;
    return (int)aether_decals_clip_to_world(d, g_active_mesh,
        (aether_decal_quad_vertex_t *)out_xyz_uv_fade_rgba, (u32)max_verts);
}
int engine_net_delta_encode(const void *baseline_snap, const void *current_snap,
                            unsigned char *out, int cap) {
    if (!current_snap || !out || cap <= 0) return 0;
    return (int)aether_net_delta_encode((const aether_net_snapshot_t *)baseline_snap,
                                        (const aether_net_snapshot_t *)current_snap,
                                        out, (u32)cap);
}
int engine_net_delta_apply(const unsigned char *data, int size) {
    if (!data || size <= 0) return 0;
    return aether_net_delta_apply(data, (u32)size, &g_delta_state) == AETHER_OK ? 1 : 0;
}
int engine_net_interp_push_demo(unsigned tick, float time, float frac) {
    ensure_net_interp();
    aether_net_snapshot_t snap;
    aether_net_snapshot_make_demo(&snap, tick, time);
    aether_net_interp_push(&g_net_interp, &snap);
    aether_net_interp_set_fraction(&g_net_interp, frac);
    return 1;
}
int engine_net_interp_origin(unsigned player_id, float out[3]) {
    ensure_net_interp();
    if (!out) return 0;
    return aether_net_interp_origin(&g_net_interp, player_id, out);
}
int engine_net_predict_local_step(float forward, float side, float yaw_deg, float dt) {
    ensure_net_predict();
    aether_net_predict_cmd_t cmd;
    memset(&cmd, 0, sizeof cmd);
    cmd.forward = forward; cmd.side = side; cmd.yaw_deg = yaw_deg; cmd.dt = dt;
    cmd.seq = g_net_predict.cmd_seq + 1;
    aether_net_predict_apply_cmd(&g_net_predict, &cmd);
    return 1;
}
int engine_net_predict_reconcile_demo(float blend) {
    ensure_net_predict();
    aether_net_snapshot_t snap;
    aether_net_snapshot_make_demo(&snap, g_net_predict.last_ack_tick + 1, 0.f);
    /* Force local player id 1 origin from demo player 0 or set id match */
    if (snap.player_count > 0) {
        snap.players[0].player_id = g_net_predict.local_id;
    }
    aether_net_predict_reconcile(&g_net_predict, &snap, blend);
    return 1;
}
int engine_net_predict_get_origin(float out[3]) {
    ensure_net_predict();
    aether_net_predict_get_origin(&g_net_predict, out);
    return 1;
}
int engine_mdl_fixture_extract_verts(void) {
    char path[] = "/tmp/aether_fixture_bridge.mdl";
    if (aether_mdl_write_fixture_file(path) == 0) return 0;
    return engine_mdl_load_fixture_file(path);
}

/* ---------- Batch: GPU lightstyles / skin / mp cmds / bloom / decal atlas ---------- */
static aether_mdl_skin_state_t g_skin;
static int g_skin_init = 0;
static aether_decal_atlas_t g_decal_atlas;
static int g_decal_atlas_init = 0;

static void ensure_skin(void) {
    if (!g_skin_init) { aether_mdl_skin_identity(&g_skin, 2); g_skin_init = 1; }
}
static void ensure_decal_atlas(void) {
    if (!g_decal_atlas_init) {
        aether_decal_atlas_init(&g_decal_atlas, AETHER_DECAL_ATLAS_W, AETHER_DECAL_ATLAS_H);
        aether_decal_atlas_generate_stub(&g_decal_atlas);
        g_decal_atlas_init = 1;
    }
}

int engine_lightstyles_fill_gpu_weights(float *out_weights64, unsigned *out_count) {
    ensure_lightstyles();
    aether_lightstyle_gpu_t gpu;
    aether_lightstyles_fill_gpu_weights(&g_lightstyles, &gpu);
    if (out_count) *out_count = gpu.count;
    if (out_weights64) {
        for (u32 i = 0; i < 64; ++i) out_weights64[i] = gpu.weights[i];
    }
    return (int)gpu.count;
}

int engine_mdl_skin_build_stub(unsigned bone_count, float time, float sway_deg) {
    ensure_skin();
    aether_mdl_skin_build_stub(&g_skin, bone_count ? bone_count : 2, time, sway_deg);
    return (int)g_skin.bone_count;
}
int engine_mdl_skin_fill_ubo(float *out, int max_floats) {
    ensure_skin();
    if (!out || max_floats < 16) return 0;
    return (int)aether_mdl_skin_fill_ubo(&g_skin, out, (u32)max_floats);
}
int engine_mdl_skin_transform_point(unsigned bone, float weight,
                                    const float in3[3], float out3[3]) {
    ensure_skin();
    if (!in3 || !out3) return 0;
    aether_mdl_skin_transform_point(&g_skin, bone, weight, in3, out3);
    return 1;
}
int engine_mdl_write_textured_fixture(const char *filepath) {
    return (int)aether_mdl_write_textured_fixture_file(filepath);
}
int engine_mdl_fixture_texture_rgba(unsigned char *out, int cap, int *out_w, int *out_h) {
    u32 w = 0, h = 0;
    u32 n = aether_mdl_fixture_texture_rgba(out, (u32)(cap > 0 ? cap : 0), &w, &h);
    if (out_w) *out_w = (int)w;
    if (out_h) *out_h = (int)h;
    return (int)n;
}

int engine_postfx_set_bloom_chain(float threshold, float intensity, float blur_radius) {
    ensure_postfx();
    aether_postfx_set_bloom_chain(&g_postfx, threshold, intensity, blur_radius);
    return 1;
}
int engine_postfx_fill_uniforms_ex(float *out8) {
    ensure_postfx();
    if (!out8) return 0;
    aether_postfx_fill_uniforms_ex(&g_postfx, out8);
    return 8;
}
int engine_postfx_fill_bloom(float *out4) {
    ensure_postfx();
    if (!out4) return 0;
    aether_postfx_bloom_t b;
    aether_postfx_fill_bloom(&g_postfx, &b);
    out4[0] = b.threshold; out4[1] = b.intensity; out4[2] = b.blur_radius; out4[3] = b.enabled;
    return 4;
}

int engine_decal_atlas_generate(void) {
    ensure_decal_atlas();
    return aether_decal_atlas_generate_stub(&g_decal_atlas) == AETHER_OK ? 1 : 0;
}
int engine_decal_atlas_copy_rgba(unsigned char *out, int max_bytes) {
    ensure_decal_atlas();
    if (!out || max_bytes <= 0) return 0;
    return (int)aether_decal_atlas_copy_rgba(&g_decal_atlas, out, (u32)max_bytes);
}
int engine_decal_atlas_sample(float u, float v, float out_rgb[3]) {
    ensure_decal_atlas();
    if (!out_rgb) return 0;
    aether_decal_atlas_sample(&g_decal_atlas, u, v, out_rgb);
    return 1;
}

int engine_net_client_send_input(float forward, float side, float yaw_deg,
                                 float pitch_deg, unsigned buttons, float dt) {
    if (!g_net_client) return 0;
    aether_net_cmd_t cmd;
    aether_net_cmd_from_move(&cmd, forward, side, 0.f, yaw_deg, pitch_deg, buttons, dt, 0);
    return aether_net_client_send_input(g_net_client, &cmd) == AETHER_OK ? 1 : 0;
}
int engine_net_client_live_tick(float dt, float forward, float side, float yaw_deg,
                                unsigned buttons, float out_origin[3]) {
    ensure_net_interp();
    ensure_net_predict();
    if (!g_net_client) return 0;
    return aether_net_client_live_tick(g_net_client, dt, forward, side, yaw_deg, buttons,
                                       &g_net_interp, &g_net_predict, out_origin);
}
int engine_net_server_tick_authority(float dt) {
    if (!g_net_server) return 0;
    return (int)aether_net_server_tick_authority(g_net_server, dt);
}
int engine_net_server_build_snapshot_players(void) {
    if (!g_net_server) return 0;
    aether_net_snapshot_t snap;
    return (int)aether_net_server_build_snapshot(g_net_server, &snap);
}
int engine_net_lagcomp_cmd_seq(unsigned player_id, float lag_ms) {
    if (!g_net_server) return -1;
    const aether_net_cmd_t *c = aether_net_server_lagcomp_cmd(g_net_server, player_id, lag_ms);
    return c ? (int)c->seq : -1;
}

/* ---------- Batch: seq skin / per-face styles / spatial / HUD / predict+clip ---------- */
#include "../../engine/client/hud/AetherHUDLayout.h"
#include "../../engine/game/weapons/AetherWeapon.h"
#include "../../engine/game/monsters/AetherMonsterAI.h"

static aether_mdl_sequence_t g_seq;
static int g_seq_init = 0;
static aether_weapon_view_t g_weapon_view;
static int g_weapon_view_init = 0;
static aether_hud_layout_t g_hud_layout;
static int g_hud_layout_init = 0;

static void ensure_seq(void) {
    if (!g_seq_init) {
        aether_mdl_sequence_init_sway(&g_seq, 2, 4, 10.f);
        g_seq_init = 1;
    }
}
static void ensure_weapon_view(void) {
    if (!g_weapon_view_init) {
        aether_weapon_view_init(&g_weapon_view, AETHER_WPN_GLOCK);
        g_weapon_view_init = 1;
    }
}
static void ensure_hud_layout(void) {
    if (!g_hud_layout_init) {
        aether_hud_layout_classic(&g_hud_layout);
        g_hud_layout_init = 1;
    }
}

int engine_mdl_skin_build_from_sequence(float frame) {
    ensure_skin();
    ensure_seq();
    aether_mdl_skin_build_from_sequence(&g_skin, &g_seq, frame);
    return (int)g_skin.bone_count;
}
int engine_mdl_skin_mesh(const unsigned char *bone_indices, const float *weights,
                         const float *in_xyz, float *out_xyz, unsigned vert_count) {
    ensure_skin();
    if (!in_xyz || !out_xyz || vert_count == 0) return 0;
    return (int)aether_mdl_skin_mesh(&g_skin, bone_indices, weights, in_xyz, out_xyz, vert_count);
}
int engine_mdl_write_seq_fixture(const char *filepath) {
    return (int)aether_mdl_write_seq_fixture_file(filepath);
}

int engine_lightmap_fill_face_style_indices(unsigned char *out, unsigned max_faces) {
    if (!g_active_mesh || !out) return 0;
    return (int)aether_lightmap_fill_face_style_indices(g_active_mesh, out, max_faces);
}
int engine_lightmap_fill_face_style_weights(float *out, unsigned max_faces) {
    ensure_lightstyles();
    if (!g_active_mesh || !out) return 0;
    return (int)aether_lightmap_fill_face_style_weights(g_active_mesh, &g_lightstyles, out, max_faces);
}

int engine_audio_set_listener(float x, float y, float z, float fx, float fy, float fz) {
    if (!g_audio) return 0;
    aether_audio_set_listener(g_audio, x, y, z, fx, fy, fz);
    return 1;
}
int engine_audio_spatial_atten(float sx, float sy, float sz, float ref_d, float max_d,
                               float *out_gain_pan_dist3) {
    if (!g_audio || !out_gain_pan_dist3) return 0;
    aether_audio_spatial_t sp;
    aether_audio_spatial_atten(g_audio, sx, sy, sz, ref_d, max_d, &sp);
    out_gain_pan_dist3[0] = sp.gain;
    out_gain_pan_dist3[1] = sp.pan;
    out_gain_pan_dist3[2] = sp.dist;
    return 1;
}
int engine_audio_play_beep_at(float freq, float dur, float vol, float sx, float sy, float sz) {
    if (!g_audio) return 0;
    return aether_audio_play_beep_at(g_audio, freq, dur, vol, sx, sy, sz) == AETHER_OK ? 1 : 0;
}

int engine_hud_layout_classic_pack(float *out24) {
    ensure_hud_layout();
    if (!out24) return 0;
    return (int)aether_hud_layout_pack(&g_hud_layout, out24, 24);
}
int engine_hud_layout_apply_classic(void) {
    ensure_hud_layout();
    if (!g_hud) return 0;
    return (int)aether_hud_layout_apply(g_hud, &g_hud_layout);
}

int engine_net_predict_set_collision_from_bsp(void) {
    ensure_net_predict();
    if (!g_collision) return 0;
    aether_net_predict_set_collision(&g_net_predict, g_collision);
    return 1;
}
int engine_net_predict_apply_cmd_clipped(float forward, float side, float yaw_deg, float dt) {
    ensure_net_predict();
    aether_net_predict_cmd_t cmd;
    memset(&cmd, 0, sizeof cmd);
    cmd.forward = forward; cmd.side = side; cmd.yaw_deg = yaw_deg;
    cmd.dt = dt; cmd.seq = g_net_predict.cmd_seq + 1;
    aether_net_predict_apply_cmd_clipped(&g_net_predict, &cmd, 1);
    return 1;
}

int engine_weapon_view_copy_stub(float *out_xyz_uv_rgba, int max_verts) {
    ensure_weapon_view();
    if (!out_xyz_uv_rgba || max_verts < 6) return 0;
    aether_viewmodel_vertex_t verts[6];
    u32 n = aether_weapon_view_copy_stub(&g_weapon_view, verts, 6);
    for (u32 i = 0; i < n; ++i) {
        float *d = out_xyz_uv_rgba + i * 9;
        d[0]=verts[i].x; d[1]=verts[i].y; d[2]=verts[i].z;
        d[3]=verts[i].u; d[4]=verts[i].v;
        d[5]=verts[i].r; d[6]=verts[i].g; d[7]=verts[i].b; d[8]=verts[i].a;
    }
    return (int)n;
}

int engine_monster_ai_tick_frame(float dt) {
    if (!g_monsters_init) return 0;
    return (int)aether_monster_ai_tick_registry(&g_monsters, dt);
}

int engine_postfx_bloom_encode_needed(void) {
    ensure_postfx();
    return aether_postfx_bloom_encode_needed(&g_postfx) ? 1 : 0;
}


/* ---------- Batch: studio / multi-style / stereo / lagcomp / pvs-lights ---------- */
static aether_lagcomp_history_t g_lagcomp;
static int g_lagcomp_init = 0;
static aether_mdl_sequence_t g_studio_seq;
static int g_studio_seq_loaded = 0;

int engine_mdl_write_studio_fixture(const char *filepath) {
    if (!filepath) return 0;
    return (int)aether_mdl_write_studio_fixture_file(filepath);
}

int engine_mdl_sequence_load_studio(float frame) {
    u8 buf[16384];
    u32 n = aether_mdl_write_studio_fixture(buf, sizeof buf);
    if (!n) return 0;
    if (aether_mdl_sequence_load_from_data(&g_studio_seq, buf, n) != AETHER_OK) return 0;
    g_studio_seq_loaded = 1;
    aether_mdl_skin_build_from_sequence(&g_skin, &g_studio_seq, frame);
    return (int)g_studio_seq.frame_count;
}

int engine_lightmap_fill_face_style_blend(float *out_weights4, unsigned max_faces) {
    if (!g_active_mesh || !out_weights4) return 0;
    if (!g_lightstyles_init) { aether_lightstyles_init(&g_lightstyles); g_lightstyles_init = 1; }
    return (int)aether_lightmap_fill_face_style_blend(g_active_mesh, &g_lightstyles,
                                                      out_weights4, max_faces);
}

int engine_audio_play_beep_stereo_at(float freq, float dur, float vol,
                                     float sx, float sy, float sz) {
    if (!g_audio) return 0;
    return aether_audio_play_beep_stereo_at(g_audio, freq, dur, vol, sx, sy, sz) == AETHER_OK ? 1 : 0;
}

int engine_audio_spatial_stereo_gains(float pan, float *out_l, float *out_r) {
    aether_audio_spatial_stereo_gains(pan, out_l, out_r);
    return 1;
}

int engine_weapon_view_copy_mdl_fixture(float *out_xyz_uv_rgba, int max_verts) {
    if (!out_xyz_uv_rgba || max_verts < 3) return 0;
    aether_weapon_view_t v;
    aether_weapon_view_init(&v, AETHER_WPN_GLOCK);
    aether_viewmodel_vertex_t tmp[64];
    int cap = max_verts < 64 ? max_verts : 64;
    u32 n = aether_weapon_view_copy_mdl_fixture(&v, tmp, (u32)cap);
    for (u32 i = 0; i < n; ++i) {
        float *d = out_xyz_uv_rgba + i * 9;
        d[0]=tmp[i].x; d[1]=tmp[i].y; d[2]=tmp[i].z;
        d[3]=tmp[i].u; d[4]=tmp[i].v;
        d[5]=tmp[i].r; d[6]=tmp[i].g; d[7]=tmp[i].b; d[8]=tmp[i].a;
    }
    return (int)n;
}

int engine_lagcomp_push_demo(float time, int id, float *mins3, float *maxs3) {
    if (!mins3 || !maxs3) return 0;
    if (!g_lagcomp_init) { aether_lagcomp_init(&g_lagcomp); g_lagcomp_init = 1; }
    aether_lagcomp_begin_frame(&g_lagcomp, time);
    return aether_lagcomp_push_aabb(&g_lagcomp, id, AETHER_LAGCOMP_PLAYER, mins3, maxs3) ? 1 : 0;
}

int engine_lagcomp_query(float time, int id, float *out_mins3, float *out_maxs3) {
    if (!g_lagcomp_init || !out_mins3 || !out_maxs3) return 0;
    aether_lagcomp_aabb_t a;
    if (!aether_lagcomp_query(&g_lagcomp, time, id, &a)) return 0;
    out_mins3[0]=a.mins[0]; out_mins3[1]=a.mins[1]; out_mins3[2]=a.mins[2];
    out_maxs3[0]=a.maxs[0]; out_maxs3[1]=a.maxs[1]; out_maxs3[2]=a.maxs[2];
    return 1;
}

int engine_dynlights_fill_ubo_pvs(float view_x, float view_y, float view_z,
                                  float *out_array, int max_floats) {
    if (!out_array || max_floats < 4) return 0;
    if (!g_dynlights_init) { aether_dyn_lights_init(&g_dynlights); g_dynlights_init = 1; }
    if (!g_active_bsp) return (int)aether_dyn_lights_fill_array(&g_dynlights, out_array, (u32)max_floats);
    i32 leaf = aether_bsp_find_leaf(g_active_bsp, view_x, view_y, view_z);
    return (int)aether_dyn_lights_fill_array_pvs(&g_dynlights, g_active_bsp, leaf,
                                                 out_array, (u32)max_floats);
}

int engine_mdl_hitbox_trace_fixture(float ox, float oy, float oz,
                                    float dx, float dy, float dz, float max_dist,
                                    int *out_index, float *out_t) {
    u8 buf[16384];
    u32 n = aether_mdl_write_studio_fixture(buf, sizeof buf);
    aether_mdl_hitbox_t boxes[8];
    u32 hc = aether_mdl_fixture_hitboxes(buf, n, boxes, 8);
    if (hc == 0) return 0;
    f32 origin[3] = {ox,oy,oz}, dir[3] = {dx,dy,dz}, pt[3], t = 0;
    i32 idx = -1;
    if (!aether_mdl_hitbox_trace(boxes, hc, origin, dir, max_dist, &idx, &t, pt)) return 0;
    if (out_index) *out_index = (int)idx;
    if (out_t) *out_t = t;
    return 1;
}

int engine_particles_spawn_muzzle(float ox, float oy, float oz,
                                  float fx, float fy, float fz, unsigned count) {
    aether_particles_t *p = bridge_particles();
    if (!p) return 0;
    f32 o[3] = {ox,oy,oz}, f[3] = {fx,fy,fz};
    return (int)aether_particles_spawn_muzzle(p, o, f, count);
}

int engine_particles_spawn_trail(float x0, float y0, float z0,
                                 float x1, float y1, float z1, unsigned count) {
    aether_particles_t *p = bridge_particles();
    if (!p) return 0;
    f32 a[3] = {x0,y0,z0}, b[3] = {x1,y1,z1};
    return (int)aether_particles_spawn_trail(p, a, b, count);
}

static aether_net_cmd_history_t g_lag_cmds;
static int g_lag_cmds_init = 0;
static aether_mdl_studio_event_t g_studio_evts[8];
static u32 g_studio_evt_count = 0;
static int g_studio_evt_loaded = 0;

int engine_lightmap_fill_style_blend_ubo(float *out, unsigned max_floats) {
    if (!g_active_mesh || !out) return 0;
    ensure_lightstyles();
    return (int)aether_lightmap_fill_style_blend_ubo(g_active_mesh, &g_lightstyles, out, max_floats);
}

int engine_lightmap_sample_style_blend(const float weights4[4],
                                       const float base_rgb[3], float out_rgb[3]) {
    if (!out_rgb) return 0;
    aether_lightmap_sample_style_blend(weights4, base_rgb, out_rgb);
    return 1;
}

int engine_weapon_view_copy_skinned(float frame, float *out_xyz_uv_rgba, int max_verts,
                                    float *out_muzzle3, float *out_muzzle_fwd3) {
    if (!out_xyz_uv_rgba || max_verts < 3) return 0;
    aether_weapon_view_t v;
    aether_weapon_view_init(&v, AETHER_WPN_GLOCK);
    aether_weapon_view_play(&v, AETHER_VIEW_ANIM_FIRE);
    aether_viewmodel_vertex_t tmp[64];
    aether_weapon_view_attach_t att;
    int cap = max_verts < 64 ? max_verts : 64;
    u32 n = aether_weapon_view_copy_skinned(&v, frame, tmp, (u32)cap, &att);
    for (u32 i = 0; i < n; ++i) {
        float *d = out_xyz_uv_rgba + i * 9;
        d[0]=tmp[i].x; d[1]=tmp[i].y; d[2]=tmp[i].z;
        d[3]=tmp[i].u; d[4]=tmp[i].v;
        d[5]=tmp[i].r; d[6]=tmp[i].g; d[7]=tmp[i].b; d[8]=tmp[i].a;
    }
    if (out_muzzle3) {
        out_muzzle3[0]=att.muzzle_pos[0]; out_muzzle3[1]=att.muzzle_pos[1]; out_muzzle3[2]=att.muzzle_pos[2];
    }
    if (out_muzzle_fwd3) {
        out_muzzle_fwd3[0]=att.muzzle_fwd[0]; out_muzzle_fwd3[1]=att.muzzle_fwd[1]; out_muzzle_fwd3[2]=att.muzzle_fwd[2];
    }
    return (int)n;
}

int engine_lagcomp_validate_hit(float now, float lag_ms,
                                float eye_x, float eye_y, float eye_z, float max_dist,
                                int *out_id, float *out_t) {
    if (!g_lagcomp_init) return 0;
    if (!g_lag_cmds_init) { aether_net_cmd_history_init(&g_lag_cmds); g_lag_cmds_init = 1; }
    f32 eye[3] = {eye_x, eye_y, eye_z};
    aether_lagcomp_hit_t hit;
    if (!aether_lagcomp_validate_hit(&g_lagcomp, &g_lag_cmds, now, lag_ms, eye, max_dist, &hit))
        return 0;
    if (out_id) *out_id = (int)hit.id;
    if (out_t) *out_t = hit.t;
    return 1;
}

/* Push a demo attack cmd into lag cmd history (for bridge demos / smoke via C). */
int engine_lagcomp_push_attack_cmd(float now, float yaw, float pitch, unsigned seq) {
    if (!g_lag_cmds_init) { aether_net_cmd_history_init(&g_lag_cmds); g_lag_cmds_init = 1; }
    aether_net_cmd_t cmd;
    aether_net_cmd_from_move(&cmd, 0, 0, 0, yaw, pitch, 1u /* attack */, 0.016f, seq);
    aether_net_cmd_history_push(&g_lag_cmds, &cmd, now);
    return 1;
}

int engine_mdl_anim_rle_decode_fixture(float frame) {
    u8 buf[24576];
    u32 n = aether_mdl_write_studio_fixture_ex(buf, sizeof buf);
    if (!n) return 0;
    if (aether_mdl_anim_rle_decode(&g_studio_seq, buf, n) != AETHER_OK) return 0;
    g_studio_seq_loaded = 1;
    aether_mdl_skin_build_from_sequence(&g_skin, &g_studio_seq, frame);
    g_studio_evt_count = aether_mdl_fixture_events(buf, n, g_studio_evts, 8);
    g_studio_evt_loaded = 1;
    return (int)g_studio_seq.frame_count;
}

int engine_dynlights_fill_ubo_pvs_bleed(float view_x, float view_y, float view_z,
                                        float *out_array, int max_floats) {
    if (!out_array || max_floats < 4) return 0;
    if (!g_dynlights_init) { aether_dyn_lights_init(&g_dynlights); g_dynlights_init = 1; }
    if (!g_active_bsp) return (int)aether_dyn_lights_fill_array(&g_dynlights, out_array, (u32)max_floats);
    i32 leaf = aether_bsp_find_leaf(g_active_bsp, view_x, view_y, view_z);
    return (int)aether_dyn_lights_fill_array_pvs_bleed(&g_dynlights, g_active_bsp, leaf,
                                                       out_array, (u32)max_floats);
}

int engine_particles_spawn_viewmodel_fire(float mx, float my, float mz,
                                          float fx, float fy, float fz,
                                          unsigned muzzle_n, unsigned trail_n) {
    aether_particles_t *p = bridge_particles();
    if (!p) return 0;
    f32 m[3]={mx,my,mz}, f[3]={fx,fy,fz};
    return (int)aether_particles_spawn_viewmodel_fire(p, m, f, muzzle_n, trail_n);
}

int engine_mdl_studio_events_tick(float prev_frame, float frame,
                                  int *out_event, char *out_opts, int opts_cap) {
    if (!g_studio_evt_loaded) {
        u8 buf[24576];
        u32 n = aether_mdl_write_studio_fixture_ex(buf, sizeof buf);
        g_studio_evt_count = aether_mdl_fixture_events(buf, n, g_studio_evts, 8);
        g_studio_evt_loaded = 1;
    }
    aether_mdl_studio_event_t fired[8];
    u32 n = aether_mdl_studio_events_fire(g_studio_evts, g_studio_evt_count,
                                          prev_frame, frame, fired, 8);
    if (n == 0) return 0;
    if (out_event) *out_event = (int)fired[0].event;
    if (out_opts && opts_cap > 0) {
        strncpy(out_opts, fired[0].options, (size_t)opts_cap - 1);
        out_opts[opts_cap - 1] = 0;
    }
    return (int)n;
}

int engine_audio_play_studio_cue(const char *cue, float volume) {
    if (!g_audio) return 0;
    return aether_audio_play_studio_cue(g_audio, cue, volume) == AETHER_OK ? 1 : 0;
}

int engine_postfx_bloom_encode_plan(unsigned *out_passes, unsigned *out_w, unsigned *out_h,
                                    int *out_separable) {
    ensure_postfx();
    aether_postfx_bloom_plan_t plan;
    aether_postfx_bloom_encode_plan(&g_postfx, &plan);
    if (out_passes) *out_passes = plan.pass_count;
    if (out_w) *out_w = plan.target_w;
    if (out_h) *out_h = plan.target_h;
    if (out_separable) *out_separable = plan.separable ? 1 : 0;
    return plan.needed ? 1 : 0;
}

int engine_mdl_fixture_attachments_count(void) {
    u8 buf[24576];
    u32 n = aether_mdl_write_studio_fixture_ex(buf, sizeof buf);
    aether_mdl_attachment_t atts[8];
    return (int)aether_mdl_fixture_attachments(buf, n, atts, 8);
}


/* ---------- Batch: face-id / bone lagcomp / portal flood / attach chain ---------- */
static aether_lagcomp_studio_history_t g_lagcomp_studio;
static int g_lagcomp_studio_init = 0;

int engine_mesh_validate_face_ids(void) {
    if (!g_active_mesh) return -1;
    (void)aether_mesh_assign_face_ids(g_active_mesh);
    return (int)aether_mesh_validate_face_ids(g_active_mesh);
}

int engine_lightmap_fill_style_blend_draw(float *out, unsigned max_floats, unsigned *out_face_count) {
    if (!g_active_mesh || !out) return 0;
    if (!g_lightstyles_init) { aether_lightstyles_init(&g_lightstyles); g_lightstyles_init = 1; }
    u32 fc = 0;
    u32 n = aether_lightmap_fill_style_blend_draw(g_active_mesh, &g_lightstyles,
                                                  AETHER_STYLE_BLEND_FLAG_FACE_ID,
                                                  out, max_floats, &fc);
    if (out_face_count) *out_face_count = fc;
    return (int)n;
}

int engine_lightmap_sample_style_blend_face(const float *draw_ubo, unsigned float_count,
                                            unsigned face_id,
                                            const float base_rgb[3], float out_rgb[3]) {
    if (!out_rgb) return 0;
    aether_lightmap_sample_style_blend_face(draw_ubo, float_count, face_id, base_rgb, out_rgb);
    return 1;
}

int engine_lagcomp_studio_push_demo(float time, int id,
                                    const float *bone_mats, unsigned bone_count,
                                    const float *hitbox_mins3, const float *hitbox_maxs3,
                                    int hitbox_bone, unsigned hitbox_count) {
    if (!bone_mats || !hitbox_mins3 || !hitbox_maxs3 || hitbox_count == 0) return 0;
    if (!g_lagcomp_studio_init) {
        aether_lagcomp_studio_init(&g_lagcomp_studio);
        g_lagcomp_studio_init = 1;
    }
    aether_lagcomp_studio_begin_frame(&g_lagcomp_studio, time);
    aether_lagcomp_hitbox_t boxes[AETHER_LAGCOMP_MAX_HITBOXES];
    u32 hc = hitbox_count;
    if (hc > AETHER_LAGCOMP_MAX_HITBOXES) hc = AETHER_LAGCOMP_MAX_HITBOXES;
    for (u32 i = 0; i < hc; ++i) {
        boxes[i].bone = hitbox_bone;
        boxes[i].group = 0;
        boxes[i].mins[0] = hitbox_mins3[i*3+0];
        boxes[i].mins[1] = hitbox_mins3[i*3+1];
        boxes[i].mins[2] = hitbox_mins3[i*3+2];
        boxes[i].maxs[0] = hitbox_maxs3[i*3+0];
        boxes[i].maxs[1] = hitbox_maxs3[i*3+1];
        boxes[i].maxs[2] = hitbox_maxs3[i*3+2];
    }
    return aether_lagcomp_studio_push(&g_lagcomp_studio, id, AETHER_LAGCOMP_PLAYER,
                                      bone_mats, bone_count, boxes, hc) ? 1 : 0;
}

int engine_lagcomp_studio_trace(float time,
                                float ox, float oy, float oz,
                                float dx, float dy, float dz, float max_dist,
                                int *out_id, int *out_hitbox, float *out_t) {
    if (!g_lagcomp_studio_init) return 0;
    f32 origin[3] = {ox,oy,oz};
    f32 dir[3] = {dx,dy,dz};
    i32 id=-1, hb=-1; f32 t=0, pt[3];
    if (!aether_lagcomp_studio_trace(&g_lagcomp_studio, time, origin, dir, max_dist,
                                     &id, &hb, &t, pt))
        return 0;
    if (out_id) *out_id = id;
    if (out_hitbox) *out_hitbox = hb;
    if (out_t) *out_t = t;
    return 1;
}

int engine_dynlights_fill_ubo_portal_flood(float view_x, float view_y, float view_z,
                                           unsigned max_hops,
                                           float *out_array, int max_floats) {
    if (!out_array || max_floats <= 0) return 0;
    if (!g_dynlights_init) { aether_dyn_lights_init(&g_dynlights); g_dynlights_init = 1; }
    if (!g_active_bsp) return (int)aether_dyn_lights_fill_array(&g_dynlights, out_array, (u32)max_floats);
    i32 leaf = aether_bsp_find_leaf(g_active_bsp, view_x, view_y, view_z);
    return (int)aether_dyn_lights_fill_array_portal_flood(&g_dynlights, g_active_bsp, leaf,
                                                          max_hops, out_array, (u32)max_floats);
}

int engine_mdl_attachment_chain_world(float *out_pos3, float *out_fwd3) {
    if (!out_pos3) return 0;
    u8 buf[24576];
    u32 n = aether_mdl_write_studio_fixture_ex(buf, sizeof buf);
    aether_mdl_attachment_t atts[8];
    u32 ac = aether_mdl_fixture_attachments(buf, n, atts, 8);
    if (ac < 2) return 0;
    /* hand = shell (bone 0), weapon muzzle = muzzle (bone 1) */
    i32 hand_i = aether_mdl_attachment_find(atts, ac, "shell");
    i32 muz_i = aether_mdl_attachment_find(atts, ac, "muzzle");
    if (hand_i < 0) hand_i = 1;
    if (muz_i < 0) muz_i = 0;
    aether_mdl_sequence_t seq;
    if (aether_mdl_anim_rle_decode(&seq, buf, n) != AETHER_OK)
        aether_mdl_sequence_load_from_data(&seq, buf, n);
    aether_mdl_texgroup_state_t sk;
    aether_mdl_skin_build_from_sequence(&sk, &seq, 1.0f);
    f32 mats[AETHER_MDL_MAX_BONES * 16];
    u32 bc = sk.bone_count < AETHER_MDL_MAX_BONES ? sk.bone_count : AETHER_MDL_MAX_BONES;
    for (u32 i = 0; i < bc; ++i)
        memcpy(mats + i*16, sk.bones[i].m, 16 * sizeof(f32));
    f32 origin[3] = {0, 0, 0};
    aether_vec3_t eye = aether_player_eye_position(&g_player);
    origin[0]=eye.x; origin[1]=eye.y; origin[2]=eye.z - 36.f;
    f32 fwd[3];
    return aether_mdl_attachment_chain_world(&atts[hand_i], mats, bc,
                                             &atts[muz_i], mats, bc,
                                             origin, out_pos3, out_fwd3 ? out_fwd3 : fwd) ? 1 : 0;
}

int engine_particles_sync_muzzle_world(float vm_x, float vm_y, float vm_z,
                                       float vf_x, float vf_y, float vf_z,
                                       unsigned particle_count,
                                       float *out_world_pos3) {
    aether_particles_t *p = bridge_particles();
    if (!p) return 0;
    if (!g_dynlights_init) { aether_dyn_lights_init(&g_dynlights); g_dynlights_init = 1; }
    f32 vm[3]={vm_x,vm_y,vm_z}, vf[3]={vf_x,vf_y,vf_z};
    aether_vec3_t eye = aether_player_eye_position(&g_player);
    f32 eye_pos[3]={eye.x,eye.y,eye.z};
    f32 eye_fwd[3], eye_right[3], eye_up[3]={0,0,1};
    aether_lagcomp_look_dir(g_player.yaw, g_player.pitch, eye_fwd);
    /* right = fwd × up */
    eye_right[0] = eye_fwd[1]*eye_up[2] - eye_fwd[2]*eye_up[1];
    eye_right[1] = eye_fwd[2]*eye_up[0] - eye_fwd[0]*eye_up[2];
    eye_right[2] = eye_fwd[0]*eye_up[1] - eye_fwd[1]*eye_up[0];
    f32 rl = sqrtf(eye_right[0]*eye_right[0]+eye_right[1]*eye_right[1]+eye_right[2]*eye_right[2]);
    if (rl > 1e-5f) { eye_right[0]/=rl; eye_right[1]/=rl; eye_right[2]/=rl; }
    else { eye_right[0]=0; eye_right[1]=1; eye_right[2]=0; }
    /* recompute up = right × fwd */
    eye_up[0] = eye_right[1]*eye_fwd[2] - eye_right[2]*eye_fwd[1];
    eye_up[1] = eye_right[2]*eye_fwd[0] - eye_right[0]*eye_fwd[2];
    eye_up[2] = eye_right[0]*eye_fwd[1] - eye_right[1]*eye_fwd[0];
    aether_muzzle_sync_t sync;
    u32 n = aether_particles_sync_muzzle_world(p, &g_dynlights, vm, vf,
                                              eye_pos, eye_fwd, eye_right, eye_up,
                                              particle_count, &sync);
    if (out_world_pos3) {
        out_world_pos3[0]=sync.world_pos[0];
        out_world_pos3[1]=sync.world_pos[1];
        out_world_pos3[2]=sync.world_pos[2];
    }
    return (int)n;
}

int engine_inv_cycle(int dir) {
    return (int)aether_player_inv_cycle(&g_player_inventory, dir);
}

int engine_inv_apply_weapon_input(void) {
    if (!g_input) return 0;
    return aether_player_inv_apply_weapon_input(
        &g_player_inventory,
        aether_input_just_pressed(g_input, AETHER_ACTION_WEAPON_NEXT) ? 1 : 0,
        aether_input_just_pressed(g_input, AETHER_ACTION_WEAPON_PREV) ? 1 : 0);
}

int engine_inv_current_weapon(void) {
    return (int)aether_player_inv_current(&g_player_inventory);
}

/* ---------- Batch: LOD / water reflect / netscore / predict smooth / depth ---------- */
static aether_mdl_lod_table_t       g_lod_table;
static aether_mdl_bodygroup_state_t g_bodygroup;
static int                          g_bodygroup_ready = 0;
static aether_water_reflect_t       g_water_reflect;
static int                          g_water_reflect_ready = 0;
static aether_scoreboard_events_t   g_sb_events;
static int                          g_sb_events_init = 0;
static aether_depth_prepass_t       g_depth_prepass;
static int                          g_depth_prepass_init = 0;

static void ensure_sb_events(void) {
    if (!g_sb_events_init) {
        aether_scoreboard_events_init(&g_sb_events);
        g_sb_events_init = 1;
    }
    if (!g_scoreboard_init) engine_scoreboard_init();
}

static void ensure_bodygroup_fixture(void) {
    if (g_bodygroup_ready) return;
    u8 buf[32768];
    u32 n = aether_mdl_write_lod_fixture(buf, sizeof buf);
    aether_mdl_fixture_lods(buf, n, &g_lod_table);
    aether_mdl_bodygroup_init_from_fixture(&g_bodygroup, buf, n);
    g_bodygroup_ready = 1;
}

int engine_mdl_write_lod_fixture(const char *filepath) {
    if (!filepath) return 0;
    return (int)aether_mdl_write_lod_fixture_file(filepath);
}

int engine_mdl_lod_select(float distance) {
    ensure_bodygroup_fixture();
    return aether_mdl_lod_select(&g_lod_table, distance);
}

int engine_mdl_lod_tri_count(int lod) {
    ensure_bodygroup_fixture();
    return (int)aether_mdl_lod_tri_count(&g_lod_table, lod);
}

int engine_bodygroup_init_fixture(void) {
    ensure_bodygroup_fixture();
    return (int)g_bodygroup.part_count;
}

int engine_bodygroup_set(unsigned part, unsigned sub) {
    ensure_bodygroup_fixture();
    return aether_mdl_bodygroup_set(&g_bodygroup, part, sub) ? 1 : 0;
}

int engine_bodygroup_get(unsigned part) {
    ensure_bodygroup_fixture();
    return (int)aether_mdl_bodygroup_get(&g_bodygroup, part);
}

int engine_bodygroup_cycle(unsigned part, int dir) {
    ensure_bodygroup_fixture();
    return (int)aether_mdl_bodygroup_cycle(&g_bodygroup, part, dir);
}

int engine_bodygroup_tri_total(void) {
    ensure_bodygroup_fixture();
    return (int)aether_mdl_bodygroup_tri_total(&g_bodygroup, &g_lod_table, g_bodygroup.active_lod);
}

int engine_bodygroup_apply_lod(float distance) {
    ensure_bodygroup_fixture();
    return aether_mdl_bodygroup_apply_lod(&g_bodygroup, &g_lod_table, distance);
}

int engine_bodygroup_apply_input(void) {
    if (!g_input) return 0;
    ensure_bodygroup_fixture();
    if (aether_input_just_pressed(g_input, AETHER_ACTION_BODYGROUP_NEXT)) {
        return (int)aether_mdl_bodygroup_cycle(&g_bodygroup, 0, +1) + 1;
    }
    return 0;
}

int engine_console_exec_bodygroup(const char *line) {
    ensure_bodygroup_fixture();
    if (!line) return 0;
    /* Accept: "bodygroup", "bodygroup next", "bodygroup 0 1", "bodygroup 1 prev" */
    char buf[128];
    size_t L = strlen(line);
    if (L >= sizeof buf) L = sizeof buf - 1;
    memcpy(buf, line, L); buf[L] = 0;
    char *tok = buf;
    while (*tok == ' ') tok++;
    char *cmd = tok;
    while (*tok && *tok != ' ') tok++;
    if (*tok) { *tok = 0; tok++; }
    while (*tok == ' ') tok++;
    if (strcmp(cmd, "bodygroup") != 0 && strcmp(cmd, "body") != 0) return 0;
    if (!*tok || strcmp(tok, "next") == 0) {
        return (int)aether_mdl_bodygroup_cycle(&g_bodygroup, 0, +1) + 1;
    }
    if (strcmp(tok, "prev") == 0) {
        return (int)aether_mdl_bodygroup_cycle(&g_bodygroup, 0, -1) + 1;
    }
    /* part [sub|next|prev] */
    unsigned part = (unsigned)atoi(tok);
    while (*tok && *tok != ' ') tok++;
    while (*tok == ' ') tok++;
    if (!*tok || strcmp(tok, "next") == 0)
        return (int)aether_mdl_bodygroup_cycle(&g_bodygroup, part, +1) + 1;
    if (strcmp(tok, "prev") == 0)
        return (int)aether_mdl_bodygroup_cycle(&g_bodygroup, part, -1) + 1;
    unsigned sub = (unsigned)atoi(tok);
    return aether_mdl_bodygroup_set(&g_bodygroup, part, sub) ? 1 : 0;
}

int engine_water_reflect_compute(float eye_x, float eye_y, float eye_z) {
    aether_water_t *w = bridge_water();
    if (!w) return 0;
    f32 eye[3] = { eye_x, eye_y, eye_z };
    aether_water_reflect_compute(w, eye, &g_water_reflect);
    g_water_reflect_ready = 1;
    return g_water_reflect.enabled ? 1 : 0;
}

int engine_water_reflect_fill_uniforms(float *out20) {
    if (!out20) return 0;
    if (!g_water_reflect_ready) {
        aether_water_t *w = bridge_water();
        f32 eye[3] = {0, 0, 64};
        aether_water_reflect_compute(w, eye, &g_water_reflect);
        g_water_reflect_ready = 1;
    }
    aether_water_reflect_uniforms_t u;
    aether_water_reflect_fill_uniforms(&g_water_reflect, &u);
    memcpy(out20, u.mirror, 16 * sizeof(float));
    memcpy(out20 + 16, u.clip_plane, 4 * sizeof(float));
    return u.enabled > 0.5f ? 20 : 20;
}

int engine_water_reflect_encode_needed(void) {
    if (!g_water_reflect_ready) return 0;
    return aether_water_reflect_encode_needed(&g_water_reflect) ? 1 : 0;
}

int engine_water_reflect_point(float ix, float iy, float iz, float *out3) {
    if (!out3) return 0;
    aether_water_t *w = bridge_water();
    f32 in[3] = { ix, iy, iz };
    aether_water_reflect_point(w, in, out3);
    return 1;
}

int engine_scoreboard_apply_join(unsigned player_id, const char *name) {
    ensure_sb_events();
    aether_scoreboard_apply_join(&g_scoreboard, &g_sb_events, player_id, name, (f32)aether_net_time());
    return (int)g_scoreboard.count;
}

int engine_scoreboard_apply_leave(unsigned player_id) {
    ensure_sb_events();
    aether_scoreboard_apply_leave(&g_scoreboard, &g_sb_events, player_id, (f32)aether_net_time());
    return (int)g_scoreboard.count;
}

int engine_scoreboard_event_count(void) {
    ensure_sb_events();
    return (int)aether_scoreboard_events_live(&g_sb_events);
}

int engine_scoreboard_get_event(int index, int *out_kind, unsigned *out_id,
                                char *name, int name_cap, float *out_time) {
    ensure_sb_events();
    aether_scoreboard_event_t e;
    if (!aether_scoreboard_events_get(&g_sb_events, (u32)index, &e)) return 0;
    if (out_kind) *out_kind = (int)e.kind;
    if (out_id) *out_id = e.player_id;
    if (out_time) *out_time = e.time;
    if (name && name_cap > 0) {
        size_t n = strlen(e.name);
        if ((int)n >= name_cap) n = (size_t)name_cap - 1;
        memcpy(name, e.name, n);
        name[n] = 0;
    }
    return 1;
}

int engine_scoreboard_handle_packet(const unsigned char *data, unsigned size, float time) {
    ensure_sb_events();
    aether_scoreboard_handle_packet(&g_scoreboard, &g_sb_events, data, size, time);
    return (int)g_scoreboard.count;
}

int engine_net_broadcast_join_demo(unsigned player_id, const char *name) {
    /* Local encode → handle (no live server required). */
    u8 pkt[256];
    u32 n = aether_scoreboard_encode_join(pkt, sizeof pkt, player_id, name);
    if (!n) return 0;
    return engine_scoreboard_handle_packet(pkt, n, (float)aether_net_time());
}

int engine_net_broadcast_leave_demo(unsigned player_id) {
    u8 pkt[64];
    u32 n = aether_scoreboard_encode_leave(pkt, sizeof pkt, player_id);
    if (!n) return 0;
    return engine_scoreboard_handle_packet(pkt, n, (float)aether_net_time());
}

int engine_net_predict_set_error_decay(float rate) {
    aether_net_predict_set_error_decay(&g_net_predict, rate);
    return 1;
}

float engine_net_predict_smooth_tick(float dt) {
    return aether_net_predict_smooth_tick(&g_net_predict, dt);
}

float engine_net_predict_error_length(void) {
    return aether_net_predict_error_length(&g_net_predict);
}

int engine_net_predict_reconcile_smooth(float snap_ox, float snap_oy, float snap_oz,
                                        float snap_blend, float dt) {
    aether_net_snapshot_t snap;
    memset(&snap, 0, sizeof snap);
    snap.tick = g_net_predict.last_ack_tick + 1;
    snap.player_count = 1;
    snap.players[0].player_id = g_net_predict.local_id ? g_net_predict.local_id : 1;
    if (g_net_predict.local_id == 0) {
        aether_net_predict_init(&g_net_predict, 1);
    }
    snap.players[0].player_id = g_net_predict.local_id;
    snap.players[0].origin[0] = snap_ox;
    snap.players[0].origin[1] = snap_oy;
    snap.players[0].origin[2] = snap_oz;
    aether_net_predict_reconcile_smooth(&g_net_predict, &snap, snap_blend, dt);
    return 1;
}

int engine_depth_prepass_ensure(unsigned w, unsigned h) {
    if (!g_depth_prepass_init) {
        aether_depth_prepass_init(&g_depth_prepass);
        g_depth_prepass_init = 1;
    }
    return aether_depth_prepass_ensure(&g_depth_prepass, w, h) == AETHER_OK ? 1 : 0;
}

int engine_depth_prepass_encode_plan(unsigned *out_passes, unsigned *out_w, unsigned *out_h,
                                     int *out_write_depth) {
    if (!g_depth_prepass_init) engine_depth_prepass_ensure(1280, 720);
    aether_depth_prepass_plan_t plan;
    aether_depth_prepass_encode_plan(&g_depth_prepass, &plan);
    if (out_passes) *out_passes = plan.pass_count;
    if (out_w) *out_w = plan.width;
    if (out_h) *out_h = plan.height;
    if (out_write_depth) *out_write_depth = plan.write_depth ? 1 : 0;
    return plan.needed ? 1 : 0;
}

int engine_depth_prepass_record_stub(const float *positions_xyz, unsigned count,
                                     float near_z, float far_z,
                                     float *out_depths, unsigned max_out) {
    if (!g_depth_prepass_init) engine_depth_prepass_ensure(1280, 720);
    return (int)aether_depth_prepass_record_stub(&g_depth_prepass, positions_xyz, count,
                                                 near_z, far_z, out_depths, max_out);
}

int engine_depth_prepass_encode_needed(void) {
    if (!g_depth_prepass_init) return 0;
    return aether_depth_prepass_encode_needed(&g_depth_prepass) ? 1 : 0;
}

/* ---------- Batch: reflect RT / studio skin / MP score / chat / kill / teleport / depth bind ---------- */
static aether_water_reflect_rt_t    g_water_reflect_rt;
static int                          g_water_reflect_rt_init = 0;
static aether_depth_prepass_camera_t g_depth_cam;
static int                           g_depth_cam_init = 0;
static aether_mdl_lod_mesh_set_t     g_lod_meshes;
static int                           g_lod_meshes_ready = 0;
static const aether_mdl_lod_mesh_bucket_t *g_lod_mesh_selected = NULL;
static aether_spectator_t            g_spectator;
static int                           g_spectator_init = 0;

static aether_mdl_texgroup_state_t      g_mdl_texgroup;
static int                          g_mdl_texgroup_ready = 0;
static aether_depth_prepass_frame_t g_depth_frame;
static int                          g_depth_frame_init = 0;

static void ensure_water_rt(void) {
    if (!g_water_reflect_rt_init) {
        aether_water_reflect_rt_init(&g_water_reflect_rt);
        g_water_reflect_rt_init = 1;
    }
}

static void ensure_mdl_texgroup_fixture(void) {
    if (g_mdl_texgroup_ready) return;
    u8 buf[32768];
    u32 n = aether_mdl_write_skin_lod_fixture(buf, sizeof buf);
    aether_mdl_texgroup_init_from_fixture(&g_mdl_texgroup, buf, n);
    /* Also refresh LOD table from same fixture */
    aether_mdl_fixture_lods(buf, n, &g_lod_table);
    aether_mdl_bodygroup_init_from_fixture(&g_bodygroup, buf, n);
    g_bodygroup_ready = 1;
    g_mdl_texgroup_ready = 1;
}

int engine_water_reflect_rt_ensure(unsigned fb_w, unsigned fb_h, float scale) {
    ensure_water_rt();
    if (!g_water_reflect_ready) {
        aether_water_t *w = bridge_water();
        f32 eye[3] = {0, 0, 64};
        aether_water_reflect_compute(w, eye, &g_water_reflect);
        g_water_reflect_ready = 1;
    }
    return aether_water_reflect_rt_ensure(&g_water_reflect_rt, fb_w, fb_h, scale) == AETHER_OK ? 1 : 0;
}

int engine_water_reflect_rt_encode_plan(unsigned *out_passes, unsigned *out_w, unsigned *out_h,
                                        int *out_allocate, int *out_sample) {
    ensure_water_rt();
    if (!g_water_reflect_rt.allocated)
        engine_water_reflect_rt_ensure(1280, 720, 0.5f);
    if (!g_water_reflect_ready) {
        aether_water_t *w = bridge_water();
        f32 eye[3] = {0, 0, 64};
        aether_water_reflect_compute(w, eye, &g_water_reflect);
        g_water_reflect_ready = 1;
    }
    aether_water_reflect_rt_plan_t plan;
    aether_water_reflect_rt_encode_plan(&g_water_reflect_rt, &g_water_reflect, &plan);
    if (out_passes) *out_passes = plan.pass_count;
    if (out_w) *out_w = plan.width;
    if (out_h) *out_h = plan.height;
    if (out_allocate) *out_allocate = plan.allocate ? 1 : 0;
    if (out_sample) *out_sample = plan.sample ? 1 : 0;
    return plan.needed ? 1 : 0;
}

int engine_water_reflect_rt_sample_needed(void) {
    ensure_water_rt();
    return aether_water_reflect_rt_sample_needed(&g_water_reflect_rt) ? 1 : 0;
}

unsigned engine_water_reflect_rt_tex_stub(void) {
    ensure_water_rt();
    return g_water_reflect_rt.tex_stub_id;
}

int engine_mdl_lod_extract_mesh(int lod,
                                float *out_pos, unsigned max_verts,
                                unsigned *out_idx, unsigned max_idx,
                                unsigned *out_vert_count, unsigned *out_tri_count) {
    ensure_bodygroup_fixture();
    return (int)aether_mdl_lod_extract_mesh(&g_lod_table, lod, out_pos, max_verts,
                                            out_idx, max_idx, out_vert_count, out_tri_count);
}

int engine_mdl_lod_extract_by_distance(float distance,
                                       float *out_pos, unsigned max_verts,
                                       unsigned *out_idx, unsigned max_idx,
                                       unsigned *out_vert_count, unsigned *out_tri_count) {
    ensure_bodygroup_fixture();
    return aether_mdl_lod_extract_by_distance(&g_lod_table, distance, out_pos, max_verts,
                                              out_idx, max_idx, out_vert_count, out_tri_count);
}

int engine_texgroup_init_fixture(void) {
    ensure_mdl_texgroup_fixture();
    return (int)g_mdl_texgroup.group_count;
}

int engine_texgroup_set(unsigned group, unsigned tex) {
    ensure_mdl_texgroup_fixture();
    return aether_mdl_texgroup_set(&g_mdl_texgroup, group, tex) ? 1 : 0;
}

int engine_texgroup_get(unsigned group) {
    ensure_mdl_texgroup_fixture();
    return (int)aether_mdl_texgroup_get(&g_mdl_texgroup, group);
}

int engine_texgroup_cycle(unsigned group, int dir) {
    ensure_mdl_texgroup_fixture();
    return (int)aether_mdl_texgroup_cycle(&g_mdl_texgroup, group, dir);
}

int engine_texgroup_select_group(unsigned group) {
    ensure_mdl_texgroup_fixture();
    return aether_mdl_texgroup_select(&g_mdl_texgroup, group);
}

int engine_console_exec_skin(const char *line) {
    ensure_mdl_texgroup_fixture();
    if (!line) return 0;
    char buf[128];
    size_t L = strlen(line);
    if (L >= sizeof buf) L = sizeof buf - 1;
    memcpy(buf, line, L); buf[L] = 0;
    char *tok = buf;
    while (*tok == ' ') tok++;
    char *cmd = tok;
    while (*tok && *tok != ' ') tok++;
    if (*tok) { *tok = 0; tok++; }
    while (*tok == ' ') tok++;
    if (strcmp(cmd, "skin") != 0 && strcmp(cmd, "texturegroup") != 0) return 0;
    if (!*tok || strcmp(tok, "next") == 0)
        return (int)aether_mdl_texgroup_cycle(&g_mdl_texgroup, 0, +1) + 1;
    if (strcmp(tok, "prev") == 0)
        return (int)aether_mdl_texgroup_cycle(&g_mdl_texgroup, 0, -1) + 1;
    unsigned group = (unsigned)atoi(tok);
    while (*tok && *tok != ' ') tok++;
    while (*tok == ' ') tok++;
    if (!*tok || strcmp(tok, "next") == 0)
        return (int)aether_mdl_texgroup_cycle(&g_mdl_texgroup, group, +1) + 1;
    if (strcmp(tok, "prev") == 0)
        return (int)aether_mdl_texgroup_cycle(&g_mdl_texgroup, group, -1) + 1;
    unsigned tex = (unsigned)atoi(tok);
    return aether_mdl_texgroup_set(&g_mdl_texgroup, group, tex) ? 1 : 0;
}

int engine_net_score_sync_demo(unsigned player_id, int score, int deaths) {
    ensure_sb_events();
    aether_scoreboard_set_score(&g_scoreboard, player_id, "Demo", score, deaths);
    return (int)g_scoreboard.count;
}

int engine_chat_encode_send_demo(unsigned player_id, const char *text) {
    if (!g_chat_init) engine_chat_init();
    u8 pkt[512];
    u32 n = aether_chat_encode(pkt, sizeof pkt, player_id, text);
    if (!n) return 0;
    return (int)aether_chat_apply_net(&g_chat, pkt, n, (f32)aether_net_time());
}

int engine_chat_encode_voice_demo(unsigned player_id, const char *cue) {
    if (!g_chat_init) engine_chat_init();
    u8 pkt[512];
    u32 n = aether_chat_encode_voice_cue(pkt, sizeof pkt, player_id, cue);
    if (!n) return 0;
    return (int)aether_chat_apply_net(&g_chat, pkt, n, (f32)aether_net_time());
}

int engine_chat_last_cue_kind(void) {
    if (!g_chat_init) return 0;
    return (int)aether_chat_last_cue_kind(&g_chat);
}

int engine_scoreboard_apply_kill(unsigned killer_id, const char *killer_name,
                                 unsigned victim_id, const char *victim_name) {
    ensure_sb_events();
    aether_scoreboard_apply_kill(&g_scoreboard, &g_sb_events,
                                 killer_id, killer_name, victim_id, victim_name,
                                 (f32)aether_net_time());
    return (int)aether_scoreboard_events_live(&g_sb_events);
}

int engine_net_broadcast_kill_demo(unsigned killer_id, const char *killer_name,
                                   unsigned victim_id, const char *victim_name) {
    u8 pkt[256];
    u32 n = aether_scoreboard_encode_kill(pkt, sizeof pkt, killer_id, killer_name,
                                          victim_id, victim_name);
    if (!n) return 0;
    return engine_scoreboard_handle_packet(pkt, n, (float)aether_net_time());
}

int engine_net_predict_set_teleport_threshold(float units) {
    aether_net_predict_set_teleport_threshold(&g_net_predict, units);
    return 1;
}

float engine_net_predict_get_teleport_threshold(void) {
    return aether_net_predict_get_teleport_threshold(&g_net_predict);
}

int engine_net_predict_reconcile_teleport(float snap_ox, float snap_oy, float snap_oz,
                                          float soft_blend) {
    aether_net_snapshot_t snap;
    memset(&snap, 0, sizeof snap);
    if (g_net_predict.local_id == 0) aether_net_predict_init(&g_net_predict, 1);
    snap.tick = g_net_predict.last_ack_tick + 1;
    snap.player_count = 1;
    snap.players[0].player_id = g_net_predict.local_id;
    snap.players[0].origin[0] = snap_ox;
    snap.players[0].origin[1] = snap_oy;
    snap.players[0].origin[2] = snap_oz;
    return aether_net_predict_reconcile_teleport(&g_net_predict, &snap, soft_blend);
}

int engine_net_predict_did_teleport(void) {
    return g_net_predict.teleported ? 1 : 0;
}

int engine_depth_prepass_bind_before_main(void) {
    if (!g_depth_prepass_init) engine_depth_prepass_ensure(1280, 720);
    if (!g_depth_frame_init) {
        aether_depth_prepass_frame_init(&g_depth_frame);
        g_depth_frame_init = 1;
    }
    return aether_depth_prepass_bind_before_main(&g_depth_prepass, &g_depth_frame);
}

int engine_depth_prepass_mark_bound(void) {
    if (!g_depth_frame_init) engine_depth_prepass_bind_before_main();
    aether_depth_prepass_mark_bound(&g_depth_frame);
    return g_depth_frame.bound ? 1 : 0;
}

int engine_depth_prepass_was_bound_before_main(void) {
    if (!g_depth_frame_init) return 0;
    return aether_depth_prepass_was_bound_before_main(&g_depth_frame) ? 1 : 0;
}

/* ---- Batch mirror-rt / lod-mesh / mp-kill / depth-cam / spectator ---- */
static void ensure_depth_cam(void) {
    if (!g_depth_cam_init) {
        aether_depth_prepass_camera_init(&g_depth_cam);
        g_depth_cam_init = 1;
    }
}
static void ensure_lod_meshes(void) {
    if (g_lod_meshes_ready) return;
    u8 buf[65536];
    u32 n = aether_mdl_write_lod_mesh_fixture(buf, sizeof buf);
    aether_mdl_fixture_lods(buf, n, &g_lod_table);
    aether_mdl_fixture_lod_meshes(buf, n, &g_lod_meshes);
    g_bodygroup_ready = 1;
    g_lod_meshes_ready = 1;
}
static void ensure_spectator(void) {
    if (!g_spectator_init) {
        aether_spectator_init(&g_spectator);
        g_spectator_init = 1;
    }
}

int engine_water_reflect_rt_build_mirror_mvp(const float *view16, const float *proj16,
                                             float *out_mvp16) {
    ensure_water_rt();
    if (!g_water_reflect_ready) {
        aether_water_t *w = bridge_water();
        f32 eye[3] = {0, 0, 64};
        aether_water_reflect_compute(w, eye, &g_water_reflect);
        g_water_reflect_ready = 1;
    }
    f32 v[16], p[16], vm[16];
    memset(v, 0, sizeof v); memset(p, 0, sizeof p);
    v[0]=v[5]=v[10]=v[15]=1.f; p[0]=p[5]=p[10]=p[15]=1.f;
    if (view16) memcpy(v, view16, sizeof v);
    if (proj16) memcpy(p, proj16, sizeof p);
    if (!out_mvp16) return 0;
    aether_water_reflect_rt_build_mirror_mvp(&g_water_reflect, v, p, out_mvp16, vm);
    return 1;
}

int engine_water_reflect_rt_draw_plan(float *out_mvp16, unsigned *out_w, unsigned *out_h,
                                      int *out_clear, int *out_draw, int *out_resolve) {
    ensure_water_rt();
    if (!g_water_reflect_rt.allocated)
        engine_water_reflect_rt_ensure(1280, 720, 0.5f);
    if (!g_water_reflect_ready) {
        aether_water_t *w = bridge_water();
        f32 eye[3] = {0, 0, 64};
        aether_water_reflect_compute(w, eye, &g_water_reflect);
        g_water_reflect_ready = 1;
    }
    f32 id[16]; memset(id, 0, sizeof id); id[0]=id[5]=id[10]=id[15]=1.f;
    aether_water_reflect_rt_draw_t plan;
    aether_water_reflect_rt_draw_plan(&g_water_reflect_rt, &g_water_reflect, id, id, &plan);
    if (out_mvp16) memcpy(out_mvp16, plan.mirror_mvp, 16 * sizeof(float));
    if (out_w) *out_w = plan.width;
    if (out_h) *out_h = plan.height;
    if (out_clear) *out_clear = plan.clear ? 1 : 0;
    if (out_draw) *out_draw = plan.draw_world ? 1 : 0;
    if (out_resolve) *out_resolve = plan.resolve ? 1 : 0;
    return plan.needed ? 1 : 0;
}

int engine_water_reflect_rt_clear(float r, float g, float b, float a) {
    ensure_water_rt();
    if (!g_water_reflect_rt.allocated) engine_water_reflect_rt_ensure(640, 360, 0.5f);
    return aether_water_reflect_rt_clear(&g_water_reflect_rt, r, g, b, a) == AETHER_OK ? 1 : 0;
}
int engine_water_reflect_rt_resolve(void) {
    ensure_water_rt();
    return aether_water_reflect_rt_resolve(&g_water_reflect_rt) == AETHER_OK ? 1 : 0;
}
int engine_water_reflect_rt_gen_mips(void) {
    ensure_water_rt();
    return aether_water_reflect_rt_gen_mips(&g_water_reflect_rt) == AETHER_OK ? 1 : 0;
}
int engine_water_reflect_rt_was_cleared(void) {
    ensure_water_rt();
    return aether_water_reflect_rt_was_cleared(&g_water_reflect_rt) ? 1 : 0;
}
int engine_water_reflect_rt_was_resolved(void) {
    ensure_water_rt();
    return aether_water_reflect_rt_was_resolved(&g_water_reflect_rt) ? 1 : 0;
}
unsigned engine_water_reflect_rt_mip_levels(void) {
    ensure_water_rt();
    return aether_water_reflect_rt_mip_levels(&g_water_reflect_rt);
}

int engine_mdl_lod_mesh_init_fixture(void) {
    ensure_lod_meshes();
    return (int)g_lod_meshes.count;
}
int engine_mdl_lod_mesh_select(float distance, unsigned *out_vert_count, unsigned *out_tri_count,
                               int *out_lod) {
    ensure_lod_meshes();
    const aether_mdl_lod_mesh_bucket_t *b = NULL;
    i32 lod = aether_mdl_lod_mesh_select(&g_lod_table, &g_lod_meshes, distance, &b);
    g_lod_mesh_selected = b;
    if (out_lod) *out_lod = lod;
    if (out_vert_count) *out_vert_count = b ? b->vert_count : 0;
    if (out_tri_count) *out_tri_count = b ? b->index_count / 3 : 0;
    return lod >= 0 ? 1 : 0;
}
int engine_mdl_lod_mesh_copy_selected(float *out_pos, unsigned max_verts,
                                      unsigned *out_idx, unsigned max_idx,
                                      unsigned *out_vert_count, unsigned *out_tri_count) {
    if (!g_lod_mesh_selected) {
        unsigned vc=0, tc=0; int lod=0;
        engine_mdl_lod_mesh_select(100.f, &vc, &tc, &lod);
    }
    return (int)aether_mdl_lod_mesh_copy(g_lod_mesh_selected, out_pos, max_verts,
                                         out_idx, max_idx, out_vert_count, out_tri_count);
}

int engine_net_server_tick_authority_kill_score(float dt, unsigned killer_id, unsigned victim_id,
                                                unsigned *out_snaps, unsigned *out_kills,
                                                unsigned *out_scoreboards, unsigned *out_reached) {
    /* Demo path: operate on a transient local server if none — bridge uses scoreboard apply. */
    ensure_sb_events();
    if (killer_id || victim_id) {
        aether_scoreboard_apply_kill(&g_scoreboard, &g_sb_events,
                                     killer_id, "Killer", victim_id, "Victim",
                                     (f32)aether_net_time());
    }
    if (out_snaps) *out_snaps = 1;
    if (out_kills) *out_kills = (killer_id || victim_id) ? 1u : 0u;
    if (out_scoreboards) *out_scoreboards = 1;
    if (out_reached) *out_reached = (unsigned)g_scoreboard.count;
    (void)dt;
    return 1;
}
int engine_net_server_fanout_scores(void) {
    ensure_sb_events();
    return (int)g_scoreboard.count;
}

int engine_depth_prepass_camera_set(const float *view16, const float *proj16,
                                    float eye_x, float eye_y, float eye_z) {
    ensure_depth_cam();
    if (!g_depth_prepass_init) engine_depth_prepass_ensure(1280, 720);
    f32 eye[3] = {eye_x, eye_y, eye_z};
    aether_depth_prepass_camera_set(&g_depth_cam, view16, proj16, eye);
    return g_depth_cam.valid ? 1 : 0;
}
int engine_depth_prepass_camera_fill_mvp(float *out_mvp16) {
    ensure_depth_cam();
    if (!out_mvp16) return 0;
    aether_depth_prepass_camera_fill_mvp(&g_depth_cam, out_mvp16);
    return g_depth_cam.valid ? 1 : 0;
}
int engine_depth_prepass_camera_valid(void) {
    ensure_depth_cam();
    return aether_depth_prepass_camera_valid(&g_depth_cam) ? 1 : 0;
}
int engine_depth_prepass_encode_plan_ex(unsigned *out_passes, unsigned *out_w, unsigned *out_h,
                                        int *out_write_depth, float *out_mvp16, int *out_has_mvp) {
    if (!g_depth_prepass_init) engine_depth_prepass_ensure(1280, 720);
    ensure_depth_cam();
    aether_depth_prepass_plan_ex_t plan;
    aether_depth_prepass_encode_plan_ex(&g_depth_prepass, &g_depth_cam, &plan);
    if (out_passes) *out_passes = plan.base.pass_count;
    if (out_w) *out_w = plan.base.width;
    if (out_h) *out_h = plan.base.height;
    if (out_write_depth) *out_write_depth = plan.base.write_depth ? 1 : 0;
    if (out_mvp16) memcpy(out_mvp16, plan.mvp, 16 * sizeof(float));
    if (out_has_mvp) *out_has_mvp = plan.has_mvp ? 1 : 0;
    return plan.base.needed ? 1 : 0;
}

int engine_spectator_init(void) { ensure_spectator(); return 1; }
int engine_spectator_follow(unsigned player_id) {
    ensure_spectator();
    aether_spectator_follow(&g_spectator, player_id);
    return 1;
}
int engine_spectator_stop(void) {
    ensure_spectator();
    aether_spectator_stop(&g_spectator);
    return 1;
}
int engine_spectator_tick(float dt, float tx, float ty, float tz,
                          float fx, float fy, float fz) {
    ensure_spectator();
    f32 pos[3] = {tx,ty,tz}, fwd[3] = {fx,fy,fz};
    return aether_spectator_tick(&g_spectator, dt, pos, fwd);
}
int engine_spectator_get_eye(float *out3) {
    ensure_spectator();
    if (!out3) return 0;
    aether_spectator_get_eye(&g_spectator, out3);
    return 1;
}
int engine_spectator_get_forward(float *out3) {
    ensure_spectator();
    if (!out3) return 0;
    aether_spectator_get_forward(&g_spectator, out3);
    return 1;
}
int engine_spectator_is_following(void) {
    ensure_spectator();
    return aether_spectator_is_following(&g_spectator) ? 1 : 0;
}


/* ---- Batch reflect-ents / studio-gpu / spec-cycle / dmg-kill ---- */

static aether_water_reflect_ent_list_t g_reflect_ents;
static int g_reflect_ents_init = 0;
static aether_player_health_t g_dmg_auth_health;
static int g_dmg_auth_init = 0;

static void ensure_reflect_ents(void) {
    if (!g_reflect_ents_init) {
        aether_water_reflect_ent_list_init(&g_reflect_ents);
        g_reflect_ents_init = 1;
    }
}

int engine_water_reflect_ent_clear(void) {
    ensure_reflect_ents();
    aether_water_reflect_ent_list_clear(&g_reflect_ents);
    return 1;
}
int engine_water_reflect_ent_push(unsigned ent_id, int is_monster,
                                  float ox, float oy, float oz,
                                  float hx, float hy, float hz) {
    ensure_reflect_ents();
    f32 o[3] = {ox,oy,oz}, h[3] = {hx,hy,hz};
    f32 wh = 0.f;
    if (g_water_reflect_ready) wh = g_water_reflect.plane_origin[2];
    return aether_water_reflect_ent_list_push(&g_reflect_ents, ent_id,
                                              is_monster ? 1 : 0, o, h, wh);
}
int engine_water_reflect_ent_mark_above(float water_height) {
    ensure_reflect_ents();
    return (int)aether_water_reflect_ent_list_mark_above(&g_reflect_ents, water_height);
}
unsigned engine_water_reflect_ent_drawn(void) {
    ensure_reflect_ents();
    return g_reflect_ents.drawn;
}
int engine_water_reflect_rt_draw_plan_full(float *out_mvp16, unsigned *out_w, unsigned *out_h,
                                           int *out_clear, int *out_draw_world,
                                           int *out_draw_ents, int *out_draw_monsters,
                                           unsigned *out_ent_count, unsigned *out_mon_count,
                                           int *out_resolve) {
    ensure_water_rt();
    ensure_reflect_ents();
    if (!g_water_reflect_rt.allocated)
        engine_water_reflect_rt_ensure(1280, 720, 0.5f);
    if (!g_water_reflect_ready) {
        aether_water_t *w = bridge_water();
        f32 eye[3] = {0, 0, 64};
        aether_water_reflect_compute(w, eye, &g_water_reflect);
        g_water_reflect_ready = 1;
    }
    f32 id[16]; memset(id, 0, sizeof id); id[0]=id[5]=id[10]=id[15]=1.f;
    aether_water_reflect_rt_draw_t plan;
    aether_water_reflect_rt_draw_plan_full(&g_water_reflect_rt, &g_water_reflect,
                                           id, id, &g_reflect_ents, &plan);
    if (out_mvp16) memcpy(out_mvp16, plan.mirror_mvp, 16 * sizeof(float));
    if (out_w) *out_w = plan.width;
    if (out_h) *out_h = plan.height;
    if (out_clear) *out_clear = plan.clear ? 1 : 0;
    if (out_draw_world) *out_draw_world = plan.draw_world ? 1 : 0;
    if (out_draw_ents) *out_draw_ents = plan.draw_entities ? 1 : 0;
    if (out_draw_monsters) *out_draw_monsters = plan.draw_monsters ? 1 : 0;
    if (out_ent_count) *out_ent_count = plan.entity_count;
    if (out_mon_count) *out_mon_count = plan.monster_count;
    if (out_resolve) *out_resolve = plan.resolve ? 1 : 0;
    return plan.needed ? 1 : 0;
}

int engine_mdl_lod_gpu_issue_draw(float distance, int *out_lod, unsigned *out_verts,
                                  unsigned *out_tris, int *out_issue) {
    ensure_lod_meshes();
    aether_mdl_lod_gpu_draw_t d;
    i32 lod = aether_mdl_lod_gpu_issue_draw(&g_lod_table, &g_lod_meshes, distance, &d);
    if (out_lod) *out_lod = lod;
    if (out_verts) *out_verts = d.vert_count;
    if (out_tris) *out_tris = d.tri_count;
    if (out_issue) *out_issue = d.issue ? 1 : 0;
    return lod >= 0 && d.issue ? 1 : 0;
}
int engine_mdl_lod_gpu_issue_draw_copy(float distance,
                                       float *out_pos, unsigned max_verts,
                                       unsigned *out_idx, unsigned max_idx,
                                       int *out_lod, unsigned *out_verts, unsigned *out_tris) {
    ensure_lod_meshes();
    aether_mdl_lod_gpu_draw_t d;
    i32 lod = aether_mdl_lod_gpu_issue_draw_copy(&g_lod_table, &g_lod_meshes, distance, &d,
                                                 out_pos, max_verts, out_idx, max_idx);
    if (out_lod) *out_lod = lod;
    if (out_verts) *out_verts = d.vert_count;
    if (out_tris) *out_tris = d.tri_count;
    return lod >= 0 && d.issue ? 1 : 0;
}

int engine_spectator_roster_clear(void) {
    ensure_spectator();
    aether_spectator_roster_clear(&g_spectator);
    return 1;
}
int engine_spectator_roster_add(unsigned player_id, const char *name) {
    ensure_spectator();
    return aether_spectator_roster_add(&g_spectator, player_id, name);
}
unsigned engine_spectator_cycle_next(void) {
    ensure_spectator();
    return aether_spectator_cycle_next(&g_spectator);
}
unsigned engine_spectator_cycle_prev(void) {
    ensure_spectator();
    return aether_spectator_cycle_prev(&g_spectator);
}
unsigned engine_spectator_target_id(void) {
    ensure_spectator();
    return aether_spectator_target_id(&g_spectator);
}
int engine_spectator_set_cam_mode(int mode) {
    ensure_spectator();
    aether_spectator_set_cam_mode(&g_spectator,
        mode == 1 ? AETHER_SPEC_CAM_COPY_EYE : AETHER_SPEC_CAM_FOLLOW);
    return 1;
}
int engine_spectator_get_cam_mode(void) {
    ensure_spectator();
    return (int)aether_spectator_get_cam_mode(&g_spectator);
}
int engine_spectator_hud_visible(void) {
    ensure_spectator();
    return aether_spectator_hud_visible(&g_spectator) ? 1 : 0;
}
int engine_spectator_hud_indicator(char *out, unsigned cap) {
    ensure_spectator();
    aether_spectator_refresh_hud(&g_spectator);
    return (int)aether_spectator_hud_indicator(&g_spectator, out, cap);
}

int engine_player_apply_damage_auth(float amount, unsigned dmg_type,
                                    unsigned killer_id, unsigned victim_id,
                                    int fanout, int *out_died, int *out_registered) {
    if (!g_dmg_auth_init) {
        aether_player_health_init(&g_dmg_auth_health);
        g_dmg_auth_init = 1;
    }
    aether_damage_event_t ev;
    memset(&ev, 0, sizeof ev);
    ev.amount = amount;
    ev.type = (aether_damage_type_t)dmg_type;
    aether_damage_kill_result_t r;
    /* Bridge demo has no live server pointer — still marks died; registered via local scoreboard. */
    aether_player_apply_damage_auth(&g_dmg_auth_health, &ev, NULL,
                                    killer_id, victim_id, fanout != 0, &r);
    if (r.died && (killer_id || victim_id)) {
        ensure_sb_events();
        aether_scoreboard_apply_kill(&g_scoreboard, &g_sb_events,
                                     killer_id, "Killer", victim_id, "Victim",
                                     (f32)aether_net_time());
        r.registered_kill = true;
    }
    if (out_died) *out_died = r.died ? 1 : 0;
    if (out_registered) *out_registered = r.registered_kill ? 1 : 0;
    return r.applied ? 1 : 0;
}
int engine_net_server_register_assist(unsigned assister_id, unsigned victim_id) {
    (void)assister_id; (void)victim_id;
    /* Host/bridge stub: no persistent server; return success for API presence. */
    return 1;
}
int engine_net_server_get_assists(unsigned player_id) {
    (void)player_id;
    return 0;
}

/* ---- Batch rt-skins / assist-feed / hiz / auth-tick / spec-hp ---- */

static aether_mdl_hiz_t g_hiz;
static int g_hiz_init = 0;
static aether_net_server_t *g_auth_demo_server = NULL;

static void ensure_hiz(void) {
    if (!g_hiz_init) { aether_mdl_hiz_init(&g_hiz); g_hiz_init = 1; }
}

int engine_water_reflect_ent_push_studio(unsigned ent_id, int is_monster,
                                         float ox, float oy, float oz,
                                         float hx, float hy, float hz,
                                         int material, unsigned skin_group, unsigned skin_tex,
                                         int attach_index,
                                         float tr, float tg, float tb, float ta) {
    ensure_reflect_ents();
    f32 o[3] = {ox,oy,oz}, h[3] = {hx,hy,hz};
    f32 tint[4] = {tr,tg,tb,ta};
    f32 wh = 0.f;
    if (g_water_reflect_ready) wh = g_water_reflect.plane_origin[2];
    return aether_water_reflect_ent_list_push_studio(&g_reflect_ents, ent_id,
                                                     is_monster ? 1 : 0, o, h, wh,
                                                     (u8)material, (u8)skin_group, (u8)skin_tex,
                                                     (i8)attach_index, tint);
}
unsigned engine_water_reflect_studio_count(void) {
    ensure_reflect_ents();
    return aether_water_reflect_ent_list_studio_count(&g_reflect_ents);
}
int engine_water_reflect_ent_get_studio(unsigned index, int *out_mat,
                                        unsigned *out_sg, unsigned *out_st,
                                        int *out_attach, float *out_tint4) {
    ensure_reflect_ents();
    aether_water_reflect_studio_t st;
    if (!aether_water_reflect_ent_get_studio(&g_reflect_ents, index, &st)) return 0;
    if (out_mat) *out_mat = (int)st.material;
    if (out_sg) *out_sg = st.skin_group;
    if (out_st) *out_st = st.skin_tex;
    if (out_attach) *out_attach = st.attach_index;
    if (out_tint4) { out_tint4[0]=st.tint[0]; out_tint4[1]=st.tint[1]; out_tint4[2]=st.tint[2]; out_tint4[3]=st.tint[3]; }
    return 1;
}
int engine_water_reflect_rt_draw_plan_studio_flags(int *out_draw_studio, unsigned *out_studio_count) {
    ensure_reflect_ents();
    ensure_water_rt();
    if (!g_water_reflect_ready) {
        aether_water_t *w = bridge_water();
        f32 eye[3] = {0, 0, 64};
        aether_water_reflect_compute(w, eye, &g_water_reflect);
        g_water_reflect_ready = 1;
    }
    f32 id[16]; memset(id, 0, sizeof id); id[0]=id[5]=id[10]=id[15]=1.f;
    aether_water_reflect_rt_draw_t plan;
    aether_water_reflect_rt_draw_plan_full(&g_water_reflect_rt, &g_water_reflect,
                                           id, id, &g_reflect_ents, &plan);
    if (out_draw_studio) *out_draw_studio = plan.draw_studio_skins ? 1 : 0;
    if (out_studio_count) *out_studio_count = plan.studio_count;
    return plan.needed ? 1 : 0;
}

int engine_scoreboard_encode_assist(unsigned char *out, unsigned cap,
                                    unsigned assister_id, const char *assister_name,
                                    unsigned victim_id, const char *victim_name) {
    return (int)aether_scoreboard_encode_assist(out, cap, assister_id, assister_name,
                                                victim_id, victim_name);
}
int engine_scoreboard_apply_assist(unsigned assister_id, const char *assister_name,
                                   unsigned victim_id, const char *victim_name) {
    ensure_sb_events();
    aether_scoreboard_apply_assist(&g_scoreboard, &g_sb_events,
                                   assister_id, assister_name,
                                   victim_id, victim_name, (f32)aether_net_time());
    return 1;
}

int engine_mdl_hiz_init(void) { ensure_hiz(); return 1; }
int engine_mdl_hiz_push(float depth, float sx, float sy) {
    ensure_hiz();
    return aether_mdl_hiz_push(&g_hiz, depth, sx, sy);
}
int engine_mdl_lod_hiz_gate(float distance, float aabb_radius,
                            float min_pixels, float max_distance,
                            float sx, float sy, float depth_ndc,
                            int *out_lod, int *out_issue, int *out_occluded,
                            float *out_screen_px) {
    ensure_hiz();
    ensure_lod_meshes();
    aether_mdl_hiz_gate_t g;
    i32 lod = aether_mdl_lod_hiz_gate(&g_lod_table, &g_lod_meshes, &g_hiz,
                                      distance, aabb_radius, 75.f,
                                      min_pixels, max_distance,
                                      sx, sy, depth_ndc, &g);
    if (out_lod) *out_lod = lod;
    if (out_issue) *out_issue = g.issue ? 1 : 0;
    if (out_occluded) *out_occluded = g.occluded ? 1 : 0;
    if (out_screen_px) *out_screen_px = g.screen_pixels;
    return lod;
}
int engine_mdl_lod_gpu_issue_draw_hiz(float distance, float aabb_radius,
                                      int *out_lod, unsigned *out_verts,
                                      unsigned *out_tris, int *out_issue,
                                      int *out_occluded) {
    ensure_hiz();
    ensure_lod_meshes();
    aether_mdl_lod_gpu_draw_t d;
    aether_mdl_hiz_gate_t g;
    i32 lod = aether_mdl_lod_gpu_issue_draw_hiz(&g_lod_table, &g_lod_meshes, &g_hiz,
                                                distance, aabb_radius, &d, &g);
    if (out_lod) *out_lod = lod;
    if (out_verts) *out_verts = d.vert_count;
    if (out_tris) *out_tris = d.tri_count;
    if (out_issue) *out_issue = d.issue ? 1 : 0;
    if (out_occluded) *out_occluded = g.occluded ? 1 : 0;
    return lod >= 0 ? 1 : 0;
}

int engine_game_bind_auth_server_demo(void) {
    if (!g_game_manager) return 0;
    if (!g_auth_demo_server) {
        u16 port = (u16)(30100 + (getpid() % 200));
        g_auth_demo_server = aether_net_server_create(port, 4);
        if (!g_auth_demo_server) return 0;
        g_auth_demo_server->clients[0].active = true;
        g_auth_demo_server->clients[0].player_id = 1;
        aether_str_copy(g_auth_demo_server->clients[0].name,
                        sizeof g_auth_demo_server->clients[0].name, "P1");
        g_auth_demo_server->clients[1].active = true;
        g_auth_demo_server->clients[1].player_id = 2;
        aether_str_copy(g_auth_demo_server->clients[1].name,
                        sizeof g_auth_demo_server->clients[1].name, "P2");
        g_auth_demo_server->client_count = 2;
    }
    aether_game_bind_auth_server(g_game_manager,
                                 (aether_game_auth_server_t *)g_auth_demo_server);
    return 1;
}
int engine_game_auth_queue_damage(unsigned killer_id, unsigned victim_id,
                                  float damage, unsigned dmg_type) {
    if (!g_game_manager) return 0;
    aether_game_auth_queue_damage(g_game_manager, killer_id, victim_id, damage, dmg_type);
    return 1;
}
int engine_game_tick_auth(float dt, int *out_died, int *out_registered) {
    if (!g_game_manager) return 0;
    aether_game_auth_tick_result_t r;
    u32 k = aether_game_tick_auth(g_game_manager, dt, &r);
    if (out_died) *out_died = r.died ? 1 : 0;
    if (out_registered) *out_registered = r.registered_kill ? 1 : 0;
    return (int)k;
}
int engine_game_has_auth_server(void) {
    return (g_game_manager && aether_game_get_auth_server(g_game_manager)) ? 1 : 0;
}

int engine_spectator_set_target_hp(int hp) {
    ensure_spectator();
    aether_spectator_set_target_hp(&g_spectator, hp);
    return 1;
}
int engine_spectator_set_target_name(const char *name) {
    ensure_spectator();
    aether_spectator_set_target_name(&g_spectator, name);
    return 1;
}
int engine_spectator_get_target_hp(void) {
    ensure_spectator();
    return (int)aether_spectator_get_target_hp(&g_spectator);
}
int engine_spectator_get_target_name(char *out, unsigned cap) {
    ensure_spectator();
    return (int)aether_spectator_get_target_name(&g_spectator, out, cap);
}

/* ---- Batch gpu-hiz-mip / weapon-auth / portal-reflect / studio-tex / assist ---- */

static aether_mdl_hiz_pyramid_t g_hiz_pyr;
static int g_hiz_pyr_init = 0;
static aether_water_reflect_portal_t g_reflect_portal;
static int g_reflect_portal_ready = 0;

static void ensure_hiz_pyr(void) {
    if (!g_hiz_pyr_init) {
        aether_mdl_hiz_pyramid_init(&g_hiz_pyr);
        g_hiz_pyr_init = 1;
    }
}

int engine_mdl_hiz_pyramid_reset(unsigned mip0_w, unsigned mip0_h) {
    ensure_hiz_pyr();
    aether_mdl_hiz_pyramid_reset(&g_hiz_pyr, mip0_w, mip0_h);
    return 1;
}
int engine_mdl_hiz_pyramid_write(unsigned x, unsigned y, float depth) {
    ensure_hiz_pyr();
    return aether_mdl_hiz_pyramid_write(&g_hiz_pyr, x, y, depth);
}
int engine_mdl_hiz_pyramid_fill_mip0(const float *depths, unsigned count) {
    ensure_hiz_pyr();
    return (int)aether_mdl_hiz_pyramid_fill_mip0(&g_hiz_pyr, depths, count);
}
unsigned engine_mdl_hiz_build_pyramid(void) {
    ensure_hiz_pyr();
    return aether_mdl_hiz_build_pyramid(&g_hiz_pyr);
}
int engine_mdl_hiz_vis_query(float x0, float y0, float x1, float y1, float obj_depth,
                             int *out_visible, int *out_occluded, float *out_hiz, int *out_mip) {
    ensure_hiz_pyr();
    aether_mdl_hiz_vis_query_t q;
    int vis = aether_mdl_hiz_vis_query(&g_hiz_pyr, x0, y0, x1, y1, obj_depth, &q);
    g_hiz_pyr.vis_queries++;
    if (q.occluded) g_hiz_pyr.vis_occluded++;
    if (out_visible) *out_visible = q.visible ? 1 : 0;
    if (out_occluded) *out_occluded = q.occluded ? 1 : 0;
    if (out_hiz) *out_hiz = q.nearest_hiz;
    if (out_mip) *out_mip = q.mip_used;
    return vis;
}
int engine_mdl_hiz_pyramid_set_gpu_hooks(int armed) {
    ensure_hiz_pyr();
    aether_mdl_hiz_pyramid_set_gpu_hooks(&g_hiz_pyr, armed != 0);
    return 1;
}
int engine_mdl_hiz_pyramid_gpu_hooks(void) {
    ensure_hiz_pyr();
    return aether_mdl_hiz_pyramid_gpu_hooks(&g_hiz_pyr) ? 1 : 0;
}
int engine_mdl_lod_hiz_pyramid_gate(float distance, float aabb_radius,
                                    float min_pixels, float sx, float sy, float depth_ndc,
                                    int *out_lod, int *out_issue, int *out_occluded,
                                    float *out_screen_px) {
    ensure_hiz_pyr();
    ensure_hiz();
    ensure_lod_meshes();
    aether_mdl_hiz_gate_t g;
    i32 lod = aether_mdl_lod_hiz_pyramid_gate(&g_lod_table, &g_lod_meshes, &g_hiz_pyr,
                                              distance, aabb_radius, 75.f, min_pixels, 0.f,
                                              sx, sy, depth_ndc, &g);
    if (out_lod) *out_lod = lod;
    if (out_issue) *out_issue = g.issue ? 1 : 0;
    if (out_occluded) *out_occluded = g.occluded ? 1 : 0;
    if (out_screen_px) *out_screen_px = g.screen_pixels;
    return lod;
}

int engine_game_weapon_hit_auth(unsigned weapon_id, float now,
                                float ox, float oy, float oz,
                                float dx, float dy, float dz,
                                unsigned killer_id, unsigned victim_id,
                                int force_hit,
                                int *out_fired, int *out_queued, int *out_died,
                                int *out_registered, float *out_damage) {
    if (!g_game_manager) return 0;
    if (!engine_game_has_auth_server()) engine_game_bind_auth_server_demo();
    aether_weapon_state_t ws;
    aether_weapon_state_init(&ws, (aether_weapon_id_t)weapon_id);
    if (ws.def && ws.def->clip_size > 0) ws.clip_ammo = ws.def->clip_size;
    aether_player_inventory_t inv;
    memset(&inv, 0, sizeof inv);
    aether_game_weapon_auth_result_t r;
    u32 kills = aether_game_weapon_hit_auth(g_game_manager, &ws, &inv, now,
                                            ox, oy, oz, dx, dy, dz,
                                            killer_id, victim_id, force_hit != 0, &r);
    if (out_fired) *out_fired = r.fired ? 1 : 0;
    if (out_queued) *out_queued = r.queued ? 1 : 0;
    if (out_died) *out_died = r.died ? 1 : 0;
    if (out_registered) *out_registered = r.registered_kill ? 1 : 0;
    if (out_damage) *out_damage = r.damage;
    (void)kills;
    return r.registered_kill ? 1 : (r.queued ? 1 : 0);
}

int engine_water_reflect_portal_set(float in_x, float in_y, float in_z,
                                    float out_x, float out_y, float out_z,
                                    int eye_crossed) {
    f32 ino[3] = {in_x, in_y, in_z};
    f32 outo[3] = {out_x, out_y, out_z};
    aether_water_reflect_portal_set(&g_reflect_portal, ino, outo, eye_crossed != 0);
    g_reflect_portal_ready = 1;
    return 1;
}
int engine_water_reflect_compute_portal(float eye_x, float eye_y, float eye_z) {
    aether_water_t *w = bridge_water();
    f32 eye[3] = {eye_x, eye_y, eye_z};
    if (!g_reflect_portal_ready) aether_water_reflect_portal_init(&g_reflect_portal);
    aether_water_reflect_compute_portal(w, eye, &g_reflect_portal, &g_water_reflect);
    g_water_reflect_ready = 1;
    return g_water_reflect.enabled ? 1 : 0;
}
int engine_water_reflect_rt_build_mirror_mvp_portal(float *out_mvp16) {
    if (!out_mvp16) return 0;
    ensure_water_rt();
    if (!g_water_reflect_ready) {
        f32 eye[3] = {0, 0, 64};
        aether_water_t *w = bridge_water();
        aether_water_reflect_compute(w, eye, &g_water_reflect);
        g_water_reflect_ready = 1;
    }
    f32 id[16]; memset(id, 0, sizeof id); id[0]=id[5]=id[10]=id[15]=1.f;
    if (!g_reflect_portal_ready) aether_water_reflect_portal_init(&g_reflect_portal);
    aether_water_reflect_rt_build_mirror_mvp_portal(&g_water_reflect, &g_reflect_portal,
                                                    id, id, out_mvp16, NULL);
    return 1;
}

int engine_water_reflect_ent_set_studio_tex(unsigned index, unsigned skin_group, unsigned skin_tex) {
    ensure_reflect_ents();
    return aether_water_reflect_ent_set_studio_tex(&g_reflect_ents, index,
                                                   (u8)skin_group, (u8)skin_tex);
}
int engine_water_reflect_ent_sample_studio_tex(unsigned index, float u, float v, float *out_rgba4) {
    ensure_reflect_ents();
    aether_water_reflect_studio_tex_t tex;
    if (!aether_water_reflect_ent_get_studio_tex(&g_reflect_ents, index, &tex)) return 0;
    aether_water_reflect_studio_tex_sample(&tex, u, v, out_rgba4);
    return 1;
}
int engine_water_reflect_studio_tex_sample(unsigned skin_group, unsigned skin_tex,
                                           float u, float v, float *out_rgba4) {
    aether_water_reflect_studio_tex_t tex;
    aether_water_reflect_studio_tex_init(&tex, (u8)skin_group, (u8)skin_tex);
    aether_water_reflect_studio_tex_sample(&tex, u, v, out_rgba4);
    return 1;
}

int engine_scoreboard_format_assist_line(int event_index, char *out, unsigned cap) {
    ensure_sb_events();
    aether_scoreboard_event_t e;
    if (!aether_scoreboard_events_get(&g_sb_events, (u32)event_index, &e)) return 0;
    return (int)aether_scoreboard_format_assist_line(&e, out, cap);
}
int engine_scoreboard_get_event_ex(int index, int *out_kind, unsigned *out_id,
                                   char *name, int name_cap,
                                   char *victim, int victim_cap,
                                   char *line, int line_cap, float *out_time) {
    ensure_sb_events();
    aether_scoreboard_event_t e;
    char buf[128];
    if (!aether_scoreboard_events_get_ex(&g_sb_events, (u32)index, &e, buf, sizeof buf))
        return 0;
    if (out_kind) *out_kind = (int)e.kind;
    if (out_id) *out_id = e.player_id;
    if (out_time) *out_time = e.time;
    if (name && name_cap > 0) {
        size_t n = strlen(e.name);
        if ((int)n >= name_cap) n = (size_t)name_cap - 1;
        memcpy(name, e.name, n); name[n] = 0;
    }
    if (victim && victim_cap > 0) {
        size_t n = strlen(e.victim_name);
        if ((int)n >= victim_cap) n = (size_t)victim_cap - 1;
        memcpy(victim, e.victim_name, n); victim[n] = 0;
    }
    if (line && line_cap > 0) {
        size_t n = strlen(buf);
        if ((int)n >= line_cap) n = (size_t)line_cap - 1;
        memcpy(line, buf, n); line[n] = 0;
    }
    return 1;
}


/* ---- Batch depth→hiz bind / portal winding / mdl skin pages / weapon hitgroup ---- */

static aether_depth_hiz_bind_plan_t g_depth_hiz_plan;
static aether_portal_winding_t g_portal_wind;
static aether_portal_reflect_plan_t g_portal_reflect_plan;
static aether_mdl_skin_page_set_t g_skin_pages;
static int g_skin_pages_ready = 0;

int engine_depth_hiz_bind_plan(unsigned mip0_w, unsigned mip0_h,
                               int *out_needed, int *out_steps, unsigned *out_views) {
    aether_depth_prepass_t *d = NULL;
    /* Use bridge depth prepass if available via ensure pattern — recreate local. */
    static aether_depth_prepass_t s_dp;
    static int s_dp_init = 0;
    if (!s_dp_init) { aether_depth_prepass_init(&s_dp); aether_depth_prepass_ensure(&s_dp, 128, 128); s_dp_init = 1; }
    d = &s_dp;
    int ok = aether_depth_hiz_bind_plan_encode(d, mip0_w, mip0_h, &g_depth_hiz_plan);
    if (out_needed) *out_needed = g_depth_hiz_plan.needed ? 1 : 0;
    if (out_steps) *out_steps = (int)g_depth_hiz_plan.encode_steps;
    if (out_views) *out_views = g_depth_hiz_plan.mip_view_count;
    return ok;
}

int engine_depth_hiz_bind_execute(unsigned mip0_w, unsigned mip0_h,
                                  const float *depth_samples, unsigned count,
                                  int *out_levels, int *out_views, int *out_bound) {
    ensure_hiz_pyr();
    static aether_depth_prepass_t s_dp;
    static int s_dp_init = 0;
    if (!s_dp_init) { aether_depth_prepass_init(&s_dp); s_dp_init = 1; }
    aether_depth_prepass_ensure(&s_dp, mip0_w > 0 ? mip0_w : 64, mip0_h > 0 ? mip0_h : 64);
    aether_depth_hiz_bind_plan_encode(&s_dp, mip0_w, mip0_h, &g_depth_hiz_plan);
    aether_mdl_hiz_bind_result_t br;
    u32 levels = aether_mdl_hiz_bind_from_depth(&g_hiz_pyr, depth_samples, count,
                                                mip0_w, mip0_h, &br);
    u32 vw[8], vh[8], vo[8];
    u32 vc = aether_mdl_hiz_pyramid_texture_views(&g_hiz_pyr, vw, vh, vo, 8);
    aether_depth_hiz_bind_plan_fill_views(&g_depth_hiz_plan, vw, vh, vo, vc);
    aether_depth_hiz_bind_plan_mark_bound(&g_depth_hiz_plan);
    if (out_levels) *out_levels = (int)levels;
    if (out_views) *out_views = (int)vc;
    if (out_bound) *out_bound = aether_depth_hiz_bind_plan_was_bound(&g_depth_hiz_plan) ? 1 : 0;
    return (int)levels;
}

int engine_mdl_hiz_vis_query_at_mip(float x0, float y0, float x1, float y1,
                                    float obj_depth, int mip,
                                    int *out_visible, int *out_occluded, float *out_hiz) {
    ensure_hiz_pyr();
    aether_mdl_hiz_vis_query_t q;
    int vis = aether_mdl_hiz_vis_query_at_mip(&g_hiz_pyr, x0, y0, x1, y1, obj_depth, mip, &q);
    if (out_visible) *out_visible = q.visible ? 1 : 0;
    if (out_occluded) *out_occluded = q.occluded ? 1 : 0;
    if (out_hiz) *out_hiz = q.nearest_hiz;
    return vis;
}

int engine_mdl_hiz_vis_query_multi_mip(float x0, float y0, float x1, float y1,
                                       float obj_depth,
                                       int *out_visible, int *out_occluded,
                                       float *out_hiz, int *out_mip) {
    ensure_hiz_pyr();
    aether_mdl_hiz_vis_query_t q;
    int vis = aether_mdl_hiz_vis_query_multi_mip(&g_hiz_pyr, x0, y0, x1, y1, obj_depth, &q);
    if (out_visible) *out_visible = q.visible ? 1 : 0;
    if (out_occluded) *out_occluded = q.occluded ? 1 : 0;
    if (out_hiz) *out_hiz = q.nearest_hiz;
    if (out_mip) *out_mip = q.mip_used;
    return vis;
}

int engine_portal_winding_make_rect(float cx, float cy, float cz,
                                    float nx, float ny, float nz,
                                    float half_w, float half_h) {
    f32 c[3] = {cx, cy, cz};
    f32 n[3] = {nx, ny, nz};
    return aether_portal_winding_make_rect(&g_portal_wind, c, n, half_w, half_h);
}

int engine_portal_winding_clip_water(void) {
    aether_water_t *w = bridge_water();
    f32 eye[3] = {0, 0, 64};
    aether_water_reflect_t r;
    aether_water_reflect_compute(w, eye, &r);
    aether_portal_winding_t clipped;
    u32 n = aether_portal_winding_clip(&g_portal_wind, r.clip_plane, &clipped);
    if (n >= 3) g_portal_wind = clipped;
    return (int)n;
}

unsigned engine_water_reflect_recursive_plan(float eye_x, float eye_y, float eye_z,
                                             unsigned max_depth,
                                             unsigned *out_views, unsigned *out_max_depth) {
    aether_water_t *w = bridge_water();
    f32 eye[3] = {eye_x, eye_y, eye_z};
    if (!g_portal_wind.valid) {
        f32 c[3] = {0, 0, 32}; f32 n[3] = {0, 1, 0};
        aether_portal_winding_make_rect(&g_portal_wind, c, n, 32.f, 48.f);
    }
    u32 vc = aether_water_reflect_recursive_plan(w, eye, &g_portal_wind, max_depth,
                                                 &g_portal_reflect_plan);
    if (out_views) *out_views = vc;
    if (out_max_depth) *out_max_depth = g_portal_reflect_plan.max_depth;
    return vc;
}

int engine_mdl_skin_pages_build(unsigned page_count) {
    u32 n = aether_mdl_skin_pages_build_fixture(&g_skin_pages, page_count);
    g_skin_pages_ready = (n > 0) ? 1 : 0;
    return (int)n;
}

int engine_mdl_skin_pages_sample(unsigned group, unsigned tex, float u, float v,
                                 float *out_rgba4) {
    if (!g_skin_pages_ready) engine_mdl_skin_pages_build(4);
    return aether_mdl_skin_pages_sample(&g_skin_pages, (u8)group, (u8)tex, u, v, out_rgba4);
}

int engine_water_reflect_ent_bind_skin_page(unsigned ent_index, unsigned page_index) {
    ensure_reflect_ents();
    if (!g_skin_pages_ready) engine_mdl_skin_pages_build(4);
    if (page_index >= g_skin_pages.count) return 0;
    return aether_water_reflect_ent_bind_skin_page(&g_reflect_ents, ent_index,
                                                   &g_skin_pages.pages[page_index]);
}

int engine_water_reflect_ent_sample_skin_page(unsigned ent_index, float u, float v,
                                              float *out_rgba4) {
    ensure_reflect_ents();
    return aether_water_reflect_ent_sample_skin_page(&g_reflect_ents, ent_index, u, v, out_rgba4);
}

int engine_game_weapon_hit_auth_hitgroup(unsigned weapon_id, float now,
                                         float ox, float oy, float oz,
                                         float dx, float dy, float dz,
                                         unsigned killer_id, unsigned victim_id,
                                         int force_hit, unsigned hitgroup,
                                         int *out_fired, int *out_queued, int *out_died,
                                         int *out_registered, float *out_damage,
                                         int *out_headshot) {
    if (!g_game_manager) return 0;
    if (!engine_game_has_auth_server()) engine_game_bind_auth_server_demo();
    aether_weapon_state_t ws;
    aether_weapon_state_init(&ws, (aether_weapon_id_t)weapon_id);
    if (ws.def && ws.def->clip_size > 0) ws.clip_ammo = ws.def->clip_size;
    aether_player_inventory_t inv;
    memset(&inv, 0, sizeof inv);
    aether_game_weapon_auth_result_t r;
    u32 kills = aether_game_weapon_hit_auth_hitgroup(g_game_manager, &ws, &inv, now,
                                                     ox, oy, oz, dx, dy, dz,
                                                     killer_id, victim_id, force_hit != 0,
                                                     (u8)hitgroup, &r);
    if (out_fired) *out_fired = r.fired ? 1 : 0;
    if (out_queued) *out_queued = r.queued ? 1 : 0;
    if (out_died) *out_died = r.died ? 1 : 0;
    if (out_registered) *out_registered = r.registered_kill ? 1 : 0;
    if (out_damage) *out_damage = r.damage;
    if (out_headshot) *out_headshot = r.headshot ? 1 : 0;
    (void)kills;
    return r.registered_kill ? 1 : (r.queued ? 1 : 0);
}


/* ---- batch: hiz texture2d_array / portal leaf graph / packed skin lumps ---- */
static aether_mdl_hiz_array_t g_hiz_array;
static aether_depth_hiz_array_bind_t g_depth_hiz_array;
static aether_bsp_portal_graph_t g_portal_graph;
static aether_mdl_skin_lump_set_t g_skin_lumps;
static int g_skin_lumps_ready = 0;

int engine_mdl_hiz_bind_texture2d_array(unsigned *out_slices, unsigned *out_mip0_w, unsigned *out_mip0_h) {
    ensure_hiz_pyr();
    if (!g_hiz_pyr.built) (void)aether_mdl_hiz_build_pyramid(&g_hiz_pyr);
    u32 n = aether_mdl_hiz_bind_texture2d_array(&g_hiz_pyr, &g_hiz_array);
    aether_mdl_hiz_array_set_gpu(&g_hiz_array, true);
    if (out_slices) *out_slices = g_hiz_array.slice_count;
    if (out_mip0_w) *out_mip0_w = g_hiz_array.mip0_w;
    if (out_mip0_h) *out_mip0_h = g_hiz_array.mip0_h;
    return n > 0 ? 1 : 0;
}

int engine_mdl_hiz_array_mark_bound(void) {
    aether_mdl_hiz_array_mark_bound(&g_hiz_array);
    return aether_mdl_hiz_array_was_bound(&g_hiz_array) ? 1 : 0;
}

int engine_mdl_hiz_array_was_bound(void) {
    return aether_mdl_hiz_array_was_bound(&g_hiz_array) ? 1 : 0;
}

int engine_mdl_hiz_vis_query_array_mip(float x0, float y0, float x1, float y1,
                                       float obj_depth, int array_mip,
                                       int *out_visible, int *out_occluded,
                                       float *out_hiz, int *out_mip) {
    ensure_hiz_pyr();
    if (g_hiz_array.slice_count == 0)
        (void)aether_mdl_hiz_bind_texture2d_array(&g_hiz_pyr, &g_hiz_array);
    aether_mdl_hiz_vis_query_t q;
    int vis = aether_mdl_hiz_vis_query_array_mip(&g_hiz_pyr, &g_hiz_array,
                                                 x0, y0, x1, y1, obj_depth, array_mip, &q);
    if (out_visible) *out_visible = q.visible ? 1 : 0;
    if (out_occluded) *out_occluded = q.occluded ? 1 : 0;
    if (out_hiz) *out_hiz = q.nearest_hiz;
    if (out_mip) *out_mip = q.mip_used;
    return vis;
}

int engine_depth_hiz_array_bind(unsigned slice_count, int *out_slices, int *out_bound) {
    if (!g_depth_hiz_plan.needed && !g_depth_hiz_plan.bound) {
        static aether_depth_prepass_t s_dp;
        static int s_init = 0;
        if (!s_init) {
            aether_depth_prepass_init(&s_dp);
            aether_depth_prepass_ensure(&s_dp, 64, 64);
            s_init = 1;
        }
        aether_depth_hiz_bind_plan_encode(&s_dp, 64, 64, &g_depth_hiz_plan);
    }
    int ok = aether_depth_hiz_array_bind_encode(&g_depth_hiz_plan, slice_count, &g_depth_hiz_array);
    aether_depth_hiz_array_bind_mark_bound(&g_depth_hiz_array);
    if (out_slices) *out_slices = (int)g_depth_hiz_array.slice_count;
    if (out_bound) *out_bound = aether_depth_hiz_array_bind_was_bound(&g_depth_hiz_array) ? 1 : 0;
    return ok;
}

int engine_bsp_portal_graph_build_multi(unsigned *out_leaves, unsigned *out_edges) {
    u32 e = aether_bsp_portal_graph_build_multi_fixture(&g_portal_graph);
    if (out_leaves) *out_leaves = g_portal_graph.leaf_count;
    if (out_edges) *out_edges = e;
    return g_portal_graph.multi_portal ? 1 : 0;
}

int engine_bsp_portal_graph_build_from_current(unsigned *out_leaves, unsigned *out_edges) {
    aether_bsp_t *bsp = aether_bsp_create_synthetic_room();
    u32 e = aether_bsp_portal_graph_build_from_bsp(&g_portal_graph, bsp);
    if (bsp) aether_bsp_free(bsp);
    if (out_leaves) *out_leaves = g_portal_graph.leaf_count;
    if (out_edges) *out_edges = e;
    return e > 0 ? 1 : 0;
}

int engine_bsp_portal_graph_flood(unsigned start_leaf, unsigned max_depth,
                                  unsigned *out_reached, unsigned *out_depth_max) {
    if (g_portal_graph.leaf_count == 0)
        aether_bsp_portal_graph_build_multi_fixture(&g_portal_graph);
    aether_bsp_portal_flood_t flood;
    u32 n = aether_bsp_portal_graph_flood(&g_portal_graph, (u16)start_leaf, max_depth, &flood);
    if (out_reached) *out_reached = n;
    u16 dmax = 0;
    for (u32 i = 0; i < flood.reached_count; ++i)
        if (flood.depth[i] > dmax) dmax = flood.depth[i];
    if (out_depth_max) *out_depth_max = dmax;
    return n > 0 ? 1 : 0;
}

unsigned engine_water_reflect_portal_graph_plan(float eye_x, float eye_y, float eye_z,
                                                unsigned eye_leaf, unsigned max_depth,
                                                unsigned *out_views, unsigned *out_flooded) {
    if (g_portal_graph.leaf_count == 0)
        aether_bsp_portal_graph_build_multi_fixture(&g_portal_graph);
    static aether_water_t s_w;
    static int s_wi = 0;
    if (!s_wi) {
        aether_water_init(&s_w);
        aether_water_set_enabled(&s_w, true);
        s_wi = 1;
    }
    f32 eye[3] = {eye_x, eye_y, eye_z};
    aether_water_reflect_portal_graph_plan_t plan;
    u32 v = aether_water_reflect_portal_graph_plan(&s_w, eye, &g_portal_graph,
                                                  (u16)eye_leaf, max_depth, &plan);
    if (out_views) *out_views = plan.view_count;
    if (out_flooded) *out_flooded = plan.flooded_leaves;
    return v;
}

int engine_mdl_skin_lumps_load(const unsigned char *bytes, unsigned size,
                               unsigned *out_count, int *out_from_asset) {
    u32 n = aether_mdl_skin_lumps_load(&g_skin_lumps, bytes, size);
    g_skin_lumps_ready = (n > 0) ? 1 : 0;
    if (out_count) *out_count = n;
    if (out_from_asset) *out_from_asset = (n > 0 && g_skin_lumps.lumps[0].from_asset) ? 1 : 0;
    return n > 0 ? 1 : 0;
}

int engine_mdl_skin_lumps_load_or_fixture(const unsigned char *bytes, unsigned size,
                                          unsigned fixture_pages,
                                          unsigned *out_count, int *out_fallback) {
    u32 n = aether_mdl_skin_lumps_load_or_fixture(&g_skin_lumps, bytes, size, fixture_pages);
    g_skin_lumps_ready = (n > 0) ? 1 : 0;
    if (out_count) *out_count = n;
    if (out_fallback) *out_fallback = g_skin_lumps.used_fixture_fallback ? 1 : 0;
    return n > 0 ? 1 : 0;
}

int engine_mdl_skin_lumps_sample(unsigned index, float u, float v, float *out_rgba4) {
    if (!g_skin_lumps_ready)
        (void)engine_mdl_skin_lumps_load_or_fixture(NULL, 0, 4, NULL, NULL);
    return aether_mdl_skin_lumps_sample(&g_skin_lumps, index, u, v, out_rgba4);
}

int engine_mdl_skin_lump_bind_water_ent(unsigned ent_index, unsigned lump_index) {
    if (!g_skin_lumps_ready)
        (void)engine_mdl_skin_lumps_load_or_fixture(NULL, 0, 4, NULL, NULL);
    if (lump_index >= g_skin_lumps.count) return 0;
    aether_mdl_skin_page_t page;
    if (!aether_mdl_skin_lump_to_page(&g_skin_lumps.lumps[lump_index], &page)) return 0;
    return aether_water_reflect_ent_bind_skin_page(&g_reflect_ents, ent_index, &page);
}
