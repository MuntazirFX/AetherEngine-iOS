/* AetherEntitySpawn.h — Spawn runtime entities from BSP entity data (STEP 18B).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_ENTITY_SPAWN_H
#define AETHER_ENTITY_SPAWN_H

#include "AetherEntityBase.h"
#include "AetherEntity.h"
#include "../bsp/AetherBSP.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Spawn all entities from a BSP's ENTITIES lump into the manager.
 * Returns number of entities spawned.
 */
u32 aether_entity_spawn_from_bsp(aether_entity_mgr_t *mgr,
                                  const aether_bsp_t *bsp);

/* Spawn a single entity from a parsed BSP entity. Returns pointer. */
aether_entity_t *aether_entity_spawn_one(aether_entity_mgr_t *mgr,
                                          const aether_entity_t *src); /* sees AetherEntity.h */

/* Get the player start position from spawned entities.
 * Returns AETHER_OK on success. */
aether_result_t aether_entity_get_player_start(const aether_entity_mgr_t *mgr,
                                                aether_vec3_t *out_pos,
                                                aether_vec3_t *out_angles);

/* Diagnostics */
void aether_entity_spawn_dump(const aether_entity_mgr_t *mgr);

#ifdef __cplusplus
}
#endif
#endif
