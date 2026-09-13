/* AetherMonsterRegistry.h — Per-level monster manager.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_MONSTER_REGISTRY_H
#define AETHER_MONSTER_REGISTRY_H

#include "AetherMonsterBase.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_MONSTER_MAX  128

typedef struct aether_monster_registry {
    aether_monster_t     monsters[AETHER_MONSTER_MAX];
    u32                  count;
    aether_entity_t     *player;    /* borrowed */
    f32                  time;
} aether_monster_registry_t;

void aether_monster_registry_init(aether_monster_registry_t *reg, aether_entity_t *player);
void aether_monster_registry_reset(aether_monster_registry_t *reg);

/* Spawn a monster at position. Returns pointer or NULL. */
aether_monster_t *aether_monster_registry_spawn(aether_monster_registry_t *reg,
                                                  aether_monster_id_t id,
                                                  aether_vec3_t pos);

/* Tick all monsters */
void aether_monster_registry_tick(aether_monster_registry_t *reg, f32 dt);

/* Counts */
u32  aether_monster_registry_count   (const aether_monster_registry_t *reg);
u32  aether_monster_registry_alive   (const aether_monster_registry_t *reg);

/* Diagnostics */
void aether_monster_registry_dump(const aether_monster_registry_t *reg);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_MONSTER_REGISTRY_H */
