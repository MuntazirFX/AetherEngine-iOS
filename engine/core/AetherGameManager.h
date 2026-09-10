#ifndef AETHER_GAME_MANAGER_H
#define AETHER_GAME_MANAGER_H

#include "../core/AetherCore.h"
#include "../core/AetherEngine.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum aether_game_id {
    AETHER_GAME_NONE = -1,
    AETHER_GAME_HALFLIFE = 0,
    AETHER_GAME_BLUESHIFT,
    AETHER_GAME_OPFOR,
    AETHER_GAME_CS16,
    AETHER_GAME_CZERO,
    AETHER_GAME_COUNT
} aether_game_id_t;

typedef enum aether_game_state {
    AETHER_GAME_STATE_IDLE = 0,
    AETHER_GAME_STATE_SELECTED,
    AETHER_GAME_STATE_INITIALIZED,
    AETHER_GAME_STATE_RUNNING,
    AETHER_GAME_STATE_ERROR
} aether_game_state_t;

typedef struct aether_game_info {
    aether_game_id_t id;
    const char      *display_name;
    const char      *dir_name;
    const char      *short_code;
    const char      *game_version;
    const char      *start_map;
    u32              supported_features;
} aether_game_info_t;

typedef struct aether_game_manager aether_game_manager_t;

aether_game_manager_t *aether_game_manager_create(aether_engine_t *engine, const char *user_data_root);
aether_result_t        aether_game_manager_destroy(aether_game_manager_t *m);

u32                           aether_game_count(void);
const aether_game_info_t     *aether_game_at(u32 index);
const aether_game_info_t     *aether_game_info_by_id(aether_game_id_t id);
const aether_game_info_t     *aether_game_info_by_dir(const char *dir_name);

aether_result_t  aether_game_select       (aether_game_manager_t *m, aether_game_id_t id);
aether_game_id_t aether_game_selected     (const aether_game_manager_t *m);
aether_game_state_t aether_game_state_get (const aether_game_manager_t *m);

aether_result_t  aether_game_initialize   (aether_game_manager_t *m);
aether_result_t  aether_game_launch       (aether_game_manager_t *m);
aether_result_t  aether_game_shutdown     (aether_game_manager_t *m);

aether_result_t  aether_game_resolve_path (const aether_game_manager_t *m, aether_game_id_t id, char *out, size_t cap);

#ifdef __cplusplus
}
#endif
#endif
