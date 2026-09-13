/* AetherMonsterDefs.h — All monster definitions (data table).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_MONSTER_DEFS_H
#define AETHER_MONSTER_DEFS_H

#include "AetherMonsterBase.h"

const aether_monster_def_t *aether_monster_defs_lookup(aether_monster_id_t id);
const aether_monster_def_t *aether_monster_defs_table(void);
u32                         aether_monster_defs_count(void);

#endif /* AETHER_MONSTER_DEFS_H */
