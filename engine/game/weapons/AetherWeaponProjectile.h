/* AetherWeaponProjectile.h — Physical projectiles (rockets, grenades, bolts).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_WEAPON_PROJECTILE_H
#define AETHER_WEAPON_PROJECTILE_H

#include "../../entity/AetherEntityBase.h"
#include "AetherWeapon.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_PROJECTILE_MAX 64

typedef enum aether_projectile_kind {
    AETHER_PROJ_ROCKET = 0,
    AETHER_PROJ_GRENADE,
    AETHER_PROJ_BOLT,
    AETHER_PROJ_SNARK,
    AETHER_PROJ_SATCHEL,
    AETHER_PROJ_TRIPMINE,
} aether_projectile_kind_t;

typedef struct aether_projectile {
    u32                       id;
    aether_projectile_kind_t  kind;
    aether_vec3_t             origin;
    aether_vec3_t             velocity;
    f32                       gravity;
    f32                       lifetime;
    f32                       explode_time;
    f32                       damage;
    f32                       radius;
    aether_entity_t          *owner;
    bool                      active;
} aether_projectile_t;

typedef struct aether_projectile_mgr {
    aether_projectile_t items[AETHER_PROJECTILE_MAX];
    u32                 count;
} aether_projectile_mgr_t;

void aether_projectile_mgr_init(aether_projectile_mgr_t *m);
/* Launch a projectile matching a weapon definition. Returns projectile id or -1. */
i32 aether_projectile_spawn_for_weapon(aether_projectile_mgr_t *m,
                                      const aether_weapon_def_t *def,
                                      aether_vec3_t origin,
                                      aether_vec3_t direction,
                                      aether_entity_t *owner);

void aether_projectile_mgr_tick(aether_projectile_mgr_t *m, f32 dt);

/* Spawn a projectile. Returns id (>=0) or -1 on failure. */
i32  aether_projectile_spawn(aether_projectile_mgr_t *m,
                              aether_projectile_kind_t kind,
                              aether_vec3_t origin,
                              aether_vec3_t velocity,
                              aether_entity_t *owner);

void aether_projectile_mgr_dump(const aether_projectile_mgr_t *m);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_WEAPON_PROJECTILE_H */
