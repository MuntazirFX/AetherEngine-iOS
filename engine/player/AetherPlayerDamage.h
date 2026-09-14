/* AetherPlayerDamage.h — Player damage system with damage types.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_PLAYER_DAMAGE_H
#define AETHER_PLAYER_DAMAGE_H

#include "../core/AetherCore.h"
#include "../core/AetherMath.h"
#include "AetherPlayerHealth.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Damage types (GoldSrc style) */
typedef enum aether_damage_type {
    AETHER_DMG_GENERIC      = 0,
    AETHER_DMG_BULLET       = (1 << 0),
    AETHER_DMG_SLASH        = (1 << 1),
    AETHER_DMG_BURN         = (1 << 2),
    AETHER_DMG_FREEZE       = (1 << 3),
    AETHER_DMG_FALL         = (1 << 4),
    AETHER_DMG_BLAST        = (1 << 5),
    AETHER_DMG_CLUB         = (1 << 6),
    AETHER_DMG_SHOCK        = (1 << 7),
    AETHER_DMG_SONIC        = (1 << 8),
    AETHER_DMG_ENERGYBEAM   = (1 << 9),
    AETHER_DMG_DROWN        = (1 << 10),
    AETHER_DMG_POISON       = (1 << 11),
    AETHER_DMG_RADIATION    = (1 << 12),
    AETHER_DMG_ACID         = (1 << 13),
} aether_damage_type_t;

typedef struct aether_damage_event {
    f32                    amount;
    aether_damage_type_t   type;
    aether_vec3_t          source;
    aether_vec3_t          direction;
    void                  *attacker;
} aether_damage_event_t;

/* Apply damage event to player health. */
void aether_player_apply_damage(aether_player_health_t *h,
                                 const aether_damage_event_t *ev);

/* Fall damage based on landing velocity (GoldSrc formula). */
f32 aether_player_calc_fall_damage(f32 velocity_z);

/* Per-frame hazard ticks */
void aether_player_tick_drown(aether_player_health_t *h, f32 dt, bool underwater);
void aether_player_tick_fire(aether_player_health_t *h, f32 dt, bool on_fire);
void aether_player_tick_radiation(aether_player_health_t *h, f32 dt, bool in_rad);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_PLAYER_DAMAGE_H */
