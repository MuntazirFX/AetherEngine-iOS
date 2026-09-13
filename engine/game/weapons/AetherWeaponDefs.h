/* AetherWeaponDefs.h — All 15 weapon definitions (data table).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_WEAPON_DEFS_H
#define AETHER_WEAPON_DEFS_H

#include "AetherWeapon.h"

/* Returns the built-in weapon definition for the given ID.
 * NULL if ID is invalid. */
const aether_weapon_def_t *aether_weapon_defs_lookup(aether_weapon_id_t id);

/* Full table (for iteration) */
const aether_weapon_def_t *aether_weapon_defs_table(void);
u32                        aether_weapon_defs_count(void);

#endif /* AETHER_WEAPON_DEFS_H */
