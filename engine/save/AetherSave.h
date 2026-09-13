/* AetherSave.h — High-level save/load API.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_SAVE_H
#define AETHER_SAVE_H

#include "AetherSaveFormat.h"
#include "../player/AetherPlayerHealth.h"
#include "../player/AetherPlayerInventory.h"
#include "../entity/AetherEntityBase.h"
#include "../game/monsters/AetherMonsterRegistry.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Save context (state to serialize) ---------- */
typedef struct aether_save_ctx {
    /* Identity */
    int          game_id;
    const char  *map_name;
    const char  *game_name;
    u32          play_time_seconds;

    /* Player */
    aether_player_health_t    *player_health;
    aether_player_inventory_t *player_inventory;
    aether_vec3_t              player_origin;
    aether_vec3_t              player_angles;

    /* Entities */
    aether_entity_mgr_t       *entities;
    aether_monster_registry_t *monsters;

    /* World (opaque for now) */
    f32                        world_time;
    u32                        world_flags;
} aether_save_ctx_t;

/* ---------- High-level API ---------- */
/* Save current game state to filepath. Returns AETHER_OK on success. */
aether_result_t aether_save_write(const char *filepath, const aether_save_ctx_t *ctx);

/* Load game state from filepath. Populates the ctx (pointers must be valid). */
aether_result_t aether_save_read(const char *filepath, aether_save_ctx_t *ctx);

/* Read only the header (fast — no full load). */
aether_result_t aether_save_read_header(const char *filepath, aether_save_header_t *out);

/* ---------- Slot management ---------- */
typedef struct aether_save_slot {
    char    name[AETHER_SAVE_SLOT_NAME_MAX];
    char    filepath[AETHER_SAVE_SLOT_NAME_MAX * 2];
    bool    occupied;
    u32     timestamp;
    u32     play_time_seconds;
    char    map_name[AETHER_SAVE_MAP_NAME_MAX];
} aether_save_slot_t;

/* List all save slots in the given directory. */
u32 aether_save_list_slots(const char *dir, aether_save_slot_t *out_slots, u32 max_slots);

/* Delete a save file. */
aether_result_t aether_save_delete(const char *filepath);

/* Diagnostics */
void aether_save_dump_header(const aether_save_header_t *h);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_SAVE_H */
