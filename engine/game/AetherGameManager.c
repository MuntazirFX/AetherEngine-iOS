/* AetherGameManager.c — Game registry + lifecycle + host tick.
 * Auto-creates game folders on init (Xash3D-style basedir layout).
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherGameManager.h"
#include "../player/AetherPlayerDamage.h"
#include "../player/AetherPlayerHealth.h"
#include "../net/AetherNetServer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#define AETHER_FEAT_SINGLEPLAYER (1u << 0)
#define AETHER_FEAT_MULTIPLAYER  (1u << 1)
#define AETHER_FEAT_CUSTOM_MAPS  (1u << 2)

static const aether_game_info_t k_games[AETHER_GAME_COUNT] = {
    { AETHER_GAME_HALFLIFE, "Half-Life",                   "valve",   "hl",    "1.1.2.2", "c0a0",       AETHER_FEAT_SINGLEPLAYER | AETHER_FEAT_MULTIPLAYER | AETHER_FEAT_CUSTOM_MAPS },
    { AETHER_GAME_BLUESHIFT, "Half-Life: Blue Shift",      "bshift",  "bshift","1.1.2.2", "ba_tram1",   AETHER_FEAT_SINGLEPLAYER | AETHER_FEAT_CUSTOM_MAPS },
    { AETHER_GAME_OPFOR,    "Half-Life: Opposing Force",   "gearbox", "opfor", "1.1.2.2", "of0a0",      AETHER_FEAT_SINGLEPLAYER | AETHER_FEAT_MULTIPLAYER | AETHER_FEAT_CUSTOM_MAPS },
    { AETHER_GAME_CS16,     "Counter-Strike 1.6",          "cstrike", "cs16",  "1.1.2.7", "de_dust2",   AETHER_FEAT_MULTIPLAYER | AETHER_FEAT_CUSTOM_MAPS },
    { AETHER_GAME_CZERO,    "Counter-Strike: Condition Zero", "czero","czero", "1.1.2.7", "cz_de_dust2",AETHER_FEAT_SINGLEPLAYER | AETHER_FEAT_MULTIPLAYER | AETHER_FEAT_CUSTOM_MAPS },
};

struct aether_game_manager {
    aether_engine_t    *engine;
    char                user_data_root[512];
    aether_game_id_t    selected;
    aether_game_state_t state;
    u64                 run_frames;
    /* Live auth kill path */
    aether_net_server_t *auth_server;
    aether_game_auth_kill_pending_t auth_pending;
    aether_player_health_t auth_victim_health;
    bool auth_health_ready;
    u32  auth_kills_total;
};

static bool mkdir_if_missing(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) return S_ISDIR(st.st_mode);
    if (mkdir(path, 0755) == 0) return true;
    return false;
}

static bool path_has_game_content(const char *game_dir) {
    struct stat st;
    char probe[700];
    if (stat(game_dir, &st) != 0 || !S_ISDIR(st.st_mode)) return false;

    snprintf(probe, sizeof probe, "%s/pak0.pak", game_dir);
    if (stat(probe, &st) == 0 && S_ISREG(st.st_mode)) return true;

    snprintf(probe, sizeof probe, "%s/maps", game_dir);
    if (stat(probe, &st) == 0 && S_ISDIR(st.st_mode)) return true;

    snprintf(probe, sizeof probe, "%s/liblist.gam", game_dir);
    if (stat(probe, &st) == 0 && S_ISREG(st.st_mode)) return true;

    DIR *d = opendir(game_dir);
    if (!d) return false;
    bool any = false;
    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;
        any = true;
        break;
    }
    closedir(d);
    return any;
}

u32 aether_game_count(void) { return AETHER_GAME_COUNT; }

const aether_game_info_t *aether_game_at(u32 index) {
    if (index >= AETHER_GAME_COUNT) return NULL;
    return &k_games[index];
}

const aether_game_info_t *aether_game_info_by_id(aether_game_id_t id) {
    if (id < 0 || id >= AETHER_GAME_COUNT) return NULL;
    return &k_games[(int)id];
}

const aether_game_info_t *aether_game_info_by_dir(const char *dir_name) {
    if (!dir_name) return NULL;
    for (u32 i = 0; i < AETHER_GAME_COUNT; ++i) {
        if (aether_str_eq(k_games[i].dir_name, dir_name)) return &k_games[i];
    }
    return NULL;
}

aether_game_manager_t *aether_game_manager_create(aether_engine_t *engine,
                                                  const char *user_data_root) {
    if (!engine || !user_data_root) {
        aether_log(AETHER_LOG_ERROR, "game", "invalid manager args");
        return NULL;
    }
    aether_game_manager_t *m = (aether_game_manager_t*)calloc(1, sizeof *m);
    if (!m) return NULL;

    m->engine = engine;
    m->selected = AETHER_GAME_NONE;
    m->state    = AETHER_GAME_STATE_IDLE;
    m->run_frames = 0;
    aether_str_copy(m->user_data_root, sizeof m->user_data_root, user_data_root);

    (void)mkdir_if_missing(m->user_data_root);
    for (u32 i = 0; i < AETHER_GAME_COUNT; ++i) {
        char sub[600];
        snprintf(sub, sizeof sub, "%s/%s", m->user_data_root, k_games[i].dir_name);
        if (mkdir_if_missing(sub)) {
            aether_log(AETHER_LOG_INFO, "game", "ensured game dir: %s", sub);
        }
    }

    aether_log(AETHER_LOG_INFO, "game",
               "manager created: data_root='%s' games=%u",
               m->user_data_root, (unsigned)AETHER_GAME_COUNT);
    return m;
}

aether_result_t aether_game_manager_destroy(aether_game_manager_t *m) {
    if (!m) return AETHER_ERR_INVALID_ARG;
    if (m->state == AETHER_GAME_STATE_RUNNING) (void)aether_game_shutdown(m);
    free(m);
    return AETHER_OK;
}

static aether_result_t game_sub_init(void *user) {
    (void)user;
    return AETHER_OK;
}
static aether_result_t game_sub_tick(void *user, f32 dt) {
    return aether_game_tick((aether_game_manager_t *)user, dt);
}
static aether_result_t game_sub_shutdown(void *user) {
    aether_game_manager_t *m = (aether_game_manager_t *)user;
    if (m && m->state == AETHER_GAME_STATE_RUNNING)
        (void)aether_game_shutdown(m);
    return AETHER_OK;
}

aether_subsystem_t aether_game_manager_as_subsystem(aether_game_manager_t *m) {
    aether_subsystem_t s;
    memset(&s, 0, sizeof s);
    s.name = "game";
    s.user = m;
    s.init = game_sub_init;
    s.tick = game_sub_tick;
    s.shutdown = game_sub_shutdown;
    s.ready = false;
    return s;
}

aether_result_t aether_game_select(aether_game_manager_t *m, aether_game_id_t id) {
    if (!m) return AETHER_ERR_INVALID_ARG;
    if (m->state == AETHER_GAME_STATE_RUNNING) return AETHER_ERR_STATE;
    const aether_game_info_t *info = aether_game_info_by_id(id);
    if (!info) return AETHER_ERR_NOT_FOUND;
    m->selected = id;
    m->state    = AETHER_GAME_STATE_SELECTED;
    aether_log(AETHER_LOG_INFO, "game", "selected '%s' (dir='%s')",
               info->display_name, info->dir_name);
    return AETHER_OK;
}

aether_result_t aether_game_select_by_dir(aether_game_manager_t *m, const char *dir_name) {
    const aether_game_info_t *info = aether_game_info_by_dir(dir_name);
    if (!info) return AETHER_ERR_NOT_FOUND;
    return aether_game_select(m, info->id);
}

aether_game_id_t aether_game_selected(const aether_game_manager_t *m) {
    return m ? m->selected : AETHER_GAME_NONE;
}

aether_game_state_t aether_game_state_get(const aether_game_manager_t *m) {
    return m ? m->state : AETHER_GAME_STATE_IDLE;
}

const char *aether_game_active_dir(const aether_game_manager_t *m) {
    if (!m || m->selected == AETHER_GAME_NONE) return NULL;
    const aether_game_info_t *info = aether_game_info_by_id(m->selected);
    return info ? info->dir_name : NULL;
}

const char *aether_game_start_map(const aether_game_manager_t *m) {
    if (!m || m->selected == AETHER_GAME_NONE) return NULL;
    const aether_game_info_t *info = aether_game_info_by_id(m->selected);
    return info ? info->start_map : NULL;
}

aether_result_t aether_game_resolve_path(const aether_game_manager_t *m,
                                         aether_game_id_t id,
                                         char *out, size_t cap) {
    if (!m || !out || cap == 0) return AETHER_ERR_INVALID_ARG;
    const aether_game_info_t *info = aether_game_info_by_id(id);
    if (!info) return AETHER_ERR_NOT_FOUND;
    int n = snprintf(out, cap, "%s/%s", m->user_data_root, info->dir_name);
    if (n < 0 || (size_t)n >= cap) return AETHER_ERR_INVALID_ARG;
    return AETHER_OK;
}

bool aether_game_data_present_dir(const char *user_data_root, const char *dir_name) {
    if (!user_data_root || !dir_name) return false;
    char path[600];
    snprintf(path, sizeof path, "%s/%s", user_data_root, dir_name);
    return path_has_game_content(path);
}

bool aether_game_data_present(const aether_game_manager_t *m, aether_game_id_t id) {
    if (!m) return false;
    const aether_game_info_t *info = aether_game_info_by_id(id);
    if (!info) return false;
    return aether_game_data_present_dir(m->user_data_root, info->dir_name);
}

aether_result_t aether_game_initialize(aether_game_manager_t *m) {
    if (!m) return AETHER_ERR_INVALID_ARG;
    if (m->selected == AETHER_GAME_NONE) return AETHER_ERR_NOT_READY;
    if (m->state == AETHER_GAME_STATE_INITIALIZED ||
        m->state == AETHER_GAME_STATE_RUNNING) return AETHER_ERR_ALREADY;

    const aether_game_info_t *info = aether_game_info_by_id(m->selected);
    if (!info) return AETHER_ERR_NOT_FOUND;

    char game_dir[600];
    aether_result_t r = aether_game_resolve_path(m, m->selected, game_dir, sizeof game_dir);
    if (r != AETHER_OK) return r;

    bool present = aether_dir_exists(game_dir);
    bool content = path_has_game_content(game_dir);
    aether_log(AETHER_LOG_INFO, "game",
               "initialize '%s' -> dir=%s (%s, content=%s)",
               info->display_name, game_dir,
               present ? "found" : "missing",
               content ? "yes" : "no — synthetic demo OK");
    m->state = AETHER_GAME_STATE_INITIALIZED;
    return AETHER_OK;
}

aether_result_t aether_game_launch(aether_game_manager_t *m) {
    if (!m) return AETHER_ERR_INVALID_ARG;
    if (m->state != AETHER_GAME_STATE_INITIALIZED) return AETHER_ERR_STATE;

    const aether_game_info_t *info = aether_game_info_by_id(m->selected);
    if (!info) return AETHER_ERR_NOT_FOUND;

    char game_dir[600];
    aether_result_t r = aether_game_resolve_path(m, m->selected, game_dir, sizeof game_dir);
    if (r != AETHER_OK) return r;

    /* Empty dirs are OK — demo uses synthetic BSP. Only fail if path missing. */
    if (!aether_dir_exists(game_dir)) {
        aether_log(AETHER_LOG_ERROR, "game",
                   "cannot launch '%s': data dir missing at %s",
                   info->display_name, game_dir);
        m->state = AETHER_GAME_STATE_ERROR;
        return AETHER_ERR_NOT_FOUND;
    }

    m->state = AETHER_GAME_STATE_RUNNING;
    m->run_frames = 0;
    aether_log(AETHER_LOG_INFO, "game", "launched '%s' (dir=%s, map=%s, content=%s)",
               info->display_name, info->dir_name,
               info->start_map ? info->start_map : "<none>",
               path_has_game_content(game_dir) ? "yes" : "empty→synthetic");
    return AETHER_OK;
}

aether_result_t aether_game_tick(aether_game_manager_t *m, f32 dt) {
    if (!m) return AETHER_ERR_INVALID_ARG;
    if (m->state == AETHER_GAME_STATE_RUNNING)
        m->run_frames++;
    /* Wire auth kill into live game tick when server pointer is bound. */
    aether_game_tick_auth(m, dt, NULL);
    return AETHER_OK;
}

void aether_game_bind_auth_server(aether_game_manager_t *m, aether_game_auth_server_t *server) {
    if (!m) return;
    m->auth_server = (aether_net_server_t *)server;
}

aether_game_auth_server_t *aether_game_get_auth_server(const aether_game_manager_t *m) {
    return m ? (aether_game_auth_server_t *)m->auth_server : NULL;
}

void aether_game_auth_queue_damage(aether_game_manager_t *m,
                                   u32 killer_id, u32 victim_id,
                                   f32 damage, u32 dmg_type) {
    if (!m) return;
    m->auth_pending.pending = true;
    m->auth_pending.killer_id = killer_id;
    m->auth_pending.victim_id = victim_id;
    m->auth_pending.damage = damage;
    m->auth_pending.dmg_type = dmg_type;
    if (!m->auth_health_ready) {
        aether_player_health_init(&m->auth_victim_health);
        m->auth_health_ready = true;
    }
}

u32 aether_game_tick_auth(aether_game_manager_t *m, f32 dt,
                          aether_game_auth_tick_result_t *out) {
    (void)dt;
    if (out) memset(out, 0, sizeof(*out));
    if (!m) return 0;
    if (out) out->had_server = (m->auth_server != NULL);
    if (!m->auth_pending.pending) return 0;
    if (!m->auth_health_ready) {
        aether_player_health_init(&m->auth_victim_health);
        m->auth_health_ready = true;
    }
    aether_damage_event_t ev;
    memset(&ev, 0, sizeof ev);
    ev.amount = m->auth_pending.damage > 0.f ? m->auth_pending.damage : 100.f;
    ev.type = (aether_damage_type_t)m->auth_pending.dmg_type;
    aether_damage_kill_result_t kr;
    aether_player_apply_damage_auth(&m->auth_victim_health, &ev,
                                    (aether_damage_net_server_t *)m->auth_server,
                                    m->auth_pending.killer_id,
                                    m->auth_pending.victim_id,
                                    true, &kr);
    m->auth_pending.pending = false;
    u32 kills = 0;
    if (kr.registered_kill) {
        kills = 1;
        m->auth_kills_total++;
    }
    if (out) {
        out->applied = kr.applied;
        out->died = kr.died;
        out->registered_kill = kr.registered_kill;
        out->kills_this_tick = kills;
    }
    return kills;
}

u64 aether_game_run_frames(const aether_game_manager_t *m) {
    return m ? m->run_frames : 0;
}

aether_result_t aether_game_shutdown(aether_game_manager_t *m) {
    if (!m) return AETHER_ERR_INVALID_ARG;
    if (m->state != AETHER_GAME_STATE_RUNNING &&
        m->state != AETHER_GAME_STATE_ERROR) return AETHER_ERR_STATE;

    aether_log(AETHER_LOG_INFO, "game", "shutdown '%s' after %llu frames",
               aether_game_info_by_id(m->selected)->display_name,
               (unsigned long long)m->run_frames);
    m->run_frames = 0;
    m->state = (m->selected != AETHER_GAME_NONE)
             ? AETHER_GAME_STATE_SELECTED
             : AETHER_GAME_STATE_IDLE;
    return AETHER_OK;
}

#include "weapons/AetherWeaponFiring.h"

static void game_auth_queue_cb(void *user, u32 killer_id, u32 victim_id,
                               f32 damage, u32 dmg_type) {
    aether_game_auth_queue_damage((aether_game_manager_t *)user,
                                  killer_id, victim_id, damage, dmg_type);
}

u32 aether_game_weapon_hit_auth(aether_game_manager_t *m,
                                aether_weapon_state_t *ws,
                                aether_player_inventory_t *inv,
                                f32 now,
                                f32 ox, f32 oy, f32 oz,
                                f32 dx, f32 dy, f32 dz,
                                u32 killer_id, u32 victim_id,
                                bool force_hit,
                                aether_game_weapon_auth_result_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!m || !ws) return 0;
    aether_vec3_t origin = {ox, oy, oz};
    aether_vec3_t dir = {dx, dy, dz};
    aether_weapon_combat_hit_t hit;
    u32 q = aether_weapon_fire_combat_auth(ws, inv, now, origin, dir,
                                           killer_id, victim_id, force_hit,
                                           game_auth_queue_cb, m, &hit);
    aether_game_auth_tick_result_t ar;
    u32 kills = 0;
    if (q) kills = aether_game_tick_auth(m, 0.f, &ar);
    else memset(&ar, 0, sizeof ar);
    if (out) {
        out->fired = hit.fired;
        out->hit = hit.hit;
        out->queued = hit.queued;
        out->died = ar.died;
        out->registered_kill = ar.registered_kill;
        out->damage = hit.damage;
        out->pellets = hit.pellets;
    }
    return kills;
}
