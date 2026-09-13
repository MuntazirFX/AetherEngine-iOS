/* AetherWeaponRegistry.h — Per-player weapon runtime registry.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_WEAPON_REGISTRY_H
#define AETHER_WEAPON_REGISTRY_H

#include "AetherWeapon.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_WEAPON_REGISTRY_MAX  AETHER_WPN_COUNT

typedef struct aether_weapon_registry {
    aether_weapon_state_t states[AETHER_WEAPON_REGISTRY_MAX];
    aether_weapon_id_t    active;
} aether_weapon_registry_t;

void aether_weapon_registry_init(aether_weapon_registry_t *reg);
void aether_weapon_registry_reset(aether_weapon_registry_t *reg);

/* Give weapon - sets up state */
bool aether_weapon_registry_give(aether_weapon_registry_t *reg, aether_weapon_id_t id);

/* Select weapon */
bool aether_weapon_registry_select(aether_weapon_registry_t *reg, aether_weapon_id_t id);

/* Active weapon state */
aether_weapon_state_t *aether_weapon_registry_active(aether_weapon_registry_t *reg);
const aether_weapon_state_t *aether_weapon_registry_active_const(const aether_weapon_registry_t *reg);

/* Access a specific weapon's state */
aether_weapon_state_t *aether_weapon_registry_get(aether_weapon_registry_t *reg, aether_weapon_id_t id);

/* Diagnostics */
void aether_weapon_registry_dump(const aether_weapon_registry_t *reg);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_WEAPON_REGISTRY_H */
