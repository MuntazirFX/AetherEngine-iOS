/* AetherCollision.h — BSP hull (clipnode) based collision for the player.
 * GoldSrc uses 3 hulls: 0=point, 1=standing, 2=crouching.
 * We test a point (player feet origin) against hull 1 or 2.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_COLLISION_H
#define AETHER_COLLISION_H

#include "../core/AetherCore.h"
#include "../core/AetherMath.h"
#include "../bsp/AetherBSP.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Leaf content types (matches GoldSrc / Quake conventions). */
#define AETHER_CONTENTS_EMPTY   (-1)
#define AETHER_CONTENTS_SOLID   (-2)
#define AETHER_CONTENTS_WATER   (-3)
#define AETHER_CONTENTS_SLIME   (-4)
#define AETHER_CONTENTS_LAVA    (-5)
#define AETHER_CONTENTS_SKY     (-6)

typedef struct aether_collision aether_collision_t;

aether_collision_t *aether_collision_build(const aether_bsp_t *bsp);
void                aether_collision_free (aether_collision_t *c);

/* Test if a point is inside solid geometry.
 * hull_index: 1 = standing, 2 = crouching. */
bool aether_collision_point_in_solid(const aether_collision_t *c,
                                     aether_vec3_t point,
                                     i32 hull_index);

/* Try to move from `from` to `to`. Returns corrected position after
 * resolving collisions along each axis. If out_on_ground is non-NULL,
 * sets it to true if the downward motion was blocked (player is on floor). */
aether_vec3_t aether_collision_move(aether_collision_t *c,
                                     aether_vec3_t from,
                                     aether_vec3_t to,
                                     i32 hull_index,
                                     bool *out_on_ground);

/* Diagnostics */
void aether_collision_dump(const aether_collision_t *c);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_COLLISION_H */
