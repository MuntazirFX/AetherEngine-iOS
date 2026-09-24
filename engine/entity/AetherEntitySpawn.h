/* AetherEntitySpawn.h — Spawn runtime entities from BSP (STEP 18B).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_ENTITY_SPAWN_H
#define AETHER_ENTITY_SPAWN_H

#include "AetherEntityBase.h"
#include "../bsp/AetherBSP.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_entity_spawn_stats {
    u32 total;
    u32 worldspawn;
    u32 player_starts;
    u32 monsters;
    u32 lights;
    u32 other;
} aether_entity_spawn_stats_t;

/* Spawn all entities from a BSP's ENTITIES lump into the manager.
 * Returns number of entities spawned. Lights are spawned (for dynlight bridge). */
u32 aether_entity_spawn_from_bsp(aether_entity_mgr_t *mgr,
                                  const aether_bsp_t *bsp);

/* Same as spawn_from_bsp but fills optional stats (may be NULL). */
u32 aether_entity_spawn_from_bsp_ex(aether_entity_mgr_t *mgr,
                                    const aether_bsp_t *bsp,
                                    aether_entity_spawn_stats_t *stats);

/* Find first player start entity, return its origin + angles. */
aether_result_t aether_entity_get_player_start(const aether_entity_mgr_t *mgr,
                                                aether_vec3_t *out_pos,
                                                aether_vec3_t *out_angles);

/* Diagnostic - dump entity summary (monsters, weapons, items, etc.) */
void aether_entity_spawn_dump(const aether_entity_mgr_t *mgr);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_ENTITY_SPAWN_H */
