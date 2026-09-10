#include "AetherGameManager.h"
#include <string.h>
#include <stdio.h>

#define AETHER_FEAT_SINGLEPLAYER (1u << 0)
#define AETHER_FEAT_MULTIPLAYER  (1u << 1)
#define AETHER_FEAT_CUSTOM_MAPS  (1u << 2)

static const aether_game_info_t k_games[AETHER_GAME_COUNT] = {
    { AETHER_GAME_HALFLIFE, "Half-Life", "valve", "hl", "1.1.2.0", "c0a0", AETHER_FEAT_SINGLEPLAYER | AETHER_FEAT_MULTIPLAYER | AETHER_FEAT_CUSTOM_MAPS },
    { AETHER_GAME_BLUESHIFT, "Half-Life: Blue Shift", "bshift", "bshift", "1.0.0.0", "ba_tram1", AETHER_FEAT_SINGLEPLAYER | AETHER_FEAT_CUSTOM_MAPS },
    { AETHER_GAME_OPFOR, "Half-Life: Opposing Force", "gearbox", "opfor", "1.0.0.0", "of0a0", AETHER_FEAT_SINGLEPLAYER | AETHER_FEAT_MULTIPLAYER | AETHER_FEAT_CUSTOM_MAPS },
    { AETHER_GAME_CS16, "Counter-Strike 1.6", "cstrike", "cs16", "1.1.2.0", "de_dust2", AETHER_FEAT_MULTIPLAYER | AETHER_FEAT_CUSTOM_MAPS },
    { AETHER_GAME_CZERO, "Counter-Strike: Condition Zero", "czero", "czero", "1.0.0.0", "cz_de_dust2", AETHER_FEAT_SINGLEPLAYER | AETHER_FEAT_MULTIPLAYER | AETHER_FEAT_CUSTOM_MAPS },
};

struct aether_game_manager {
    aether_engine_t    *engine;
    char                user_data_root[512];
    aether_game_id_t    selected;
    aether_game_state_t state;
};

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

aether_game_manager_t *aether_game_manager_create(aether_engine_t *engine, const char *user_data_root) {
    if (!engine || !user_data_root) { aether_log(AETHER_LOG_ERROR, "game", "invalid manager args"); return NULL; }
    aether_game_manager_t *m = (aether_game_manager_t*)calloc(1, sizeof *m);
    if (!m) return NULL;
    m->engine = engine; m->selected = AETHER_GAME_NONE; m->state = AETHER_GAME_STATE_IDLE;
    aether_str_copy(m->user_data_root, sizeof m->user_data_root, user_data_root);
    aether_log(AETHER_LOG_INFO, "game", "manager created: data_root='%s' games=%u", m->user_data_root, (unsigned)AETHER_GAME_COUNT);
    return m;
}

aether_result_t aether_game_manager_destroy(aether_game_manager_t *m) {
    if (!m) return AETHER_ERR_INVALID_ARG;
    if (m->state == AETHER_GAME_STATE_RUNNING) (void)aether_game_shutdown(m);
    free(m);
    return AETHER_OK;
}

aether_result_t aether_game_select(aether_game_manager_t *m, aether_game_id_t id) {
    if (!m) return AETHER_ERR_INVALID_ARG;
    if (m->state == AETHER_GAME_STATE_RUNNING) return AETHER_ERR_STATE;
    const aether_game_info_t *info = aether_game_info_by_id(id);
    if (!info) return AETHER_ERR_NOT_FOUND;
    m->selected = id; m->state = AETHER_GAME_STATE_SELECTED;
    aether_log(AETHER_LOG_INFO, "game", "selected '%s' (dir='%s')", info->display_name, info->dir_name);
    return AETHER_OK;
}

aether_game_id_t aether_game_selected(const aether_game_manager_t *m) {
    return m ? m->selected : AETHER_GAME_NONE;
}

aether_game_state_t aether_game_state_get(const aether_game_manager_t *m) {
    return m ? m->state : AETHER_GAME_STATE_IDLE;
}

aether_result_t aether_game_resolve_path(const aether_game_manager_t *m, aether_game_id_t id, char *out, size_t cap) {
    if (!m || !out || cap == 0) return AETHER_ERR_INVALID_ARG;
    const aether_game_info_t *info = aether_game_info_by_id(id);
    if (!info) return AETHER_ERR_NOT_FOUND;
    int n = snprintf(out, cap, "%s/%s", m->user_data_root, info->dir_name);
    if (n < 0 || (size_t)n >= cap) return AETHER_ERR_INVALID_ARG;
    return AETHER_OK;
}

aether_result_t aether_game_initialize(aether_game_manager_t *m) {
    if (!m) return AETHER_ERR_INVALID_ARG;
    if (m->selected == AETHER_GAME_NONE) return AETHER_ERR_NOT_READY;
    if (m->state == AETHER_GAME_STATE_INITIALIZED || m->state == AETHER_GAME_STATE_RUNNING) return AETHER_ERR_ALREADY;
    const aether_game_info_t *info = aether_game_info_by_id(m->selected);
    if (!info) return AETHER_ERR_NOT_FOUND;
    char game_dir[600];
    aether_result_t r = aether_game_resolve_path(m, m->selected, game_dir, sizeof game_dir);
    if (r != AETHER_OK) return r;
    bool present = aether_dir_exists(game_dir);
    aether_log(AETHER_LOG_INFO, "game", "initialize '%s' -> dir=%s (%s)", info->display_name, game_dir, present ? "found" : "missing (user must import)");
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
    if (!aether_dir_exists(game_dir)) {
        aether_log(AETHER_LOG_ERROR, "game", "cannot launch '%s': data dir missing at %s", info->display_name, game_dir);
        m->state = AETHER_GAME_STATE_ERROR;
        return AETHER_ERR_NOT_FOUND;
    }
    m->state = AETHER_GAME_STATE_RUNNING;
    aether_log(AETHER_LOG_INFO, "game", "launched '%s' (dir=%s, map=%s)", info->display_name, info->dir_name, info->start_map ? info->start_map : "<none>");
    return AETHER_OK;
}

aether_result_t aether_game_shutdown(aether_game_manager_t *m) {
    if (!m) return AETHER_ERR_INVALID_ARG;
    if (m->state != AETHER_GAME_STATE_RUNNING && m->state != AETHER_GAME_STATE_ERROR) return AETHER_ERR_STATE;
    aether_log(AETHER_LOG_INFO, "game", "shutdown '%s'", aether_game_info_by_id(m->selected)->display_name);
    m->state = (m->selected != AETHER_GAME_NONE) ? AETHER_GAME_STATE_SELECTED : AETHER_GAME_STATE_IDLE;
    return AETHER_OK;
}
