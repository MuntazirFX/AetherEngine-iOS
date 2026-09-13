/* AetherSavePlayer.h — Player state serialization.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_SAVE_PLAYER_H
#define AETHER_SAVE_PLAYER_H

#include "AetherSaveFormat.h"
#include "../player/AetherPlayerHealth.h"
#include "../player/AetherPlayerInventory.h"
#include "../core/AetherMath.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Serialized player state (fixed layout for forward compat) */
typedef struct aether_save_player {
    /* Position */
    f32  origin[3];
    f32  angles[3];
    f32  velocity[3];

    /* Health */
    f32  health;
    f32  armor;
    f32  battery;
    u32  has_suit;      /* 0/1 */
    u32  flashlight;    /* 0/1 */

    /* Inventory — weapons owned */
    u32  weapons_owned[AETHER_WPN_COUNT];

    /* Active weapon */
    u32  active_weapon;

    /* Ammo */
    i32  ammo[AETHER_AMMO_TYPE_COUNT];

    /* Items */
    u32  has_longjump;
    u32  has_antidote;
    u32  has_security;
} aether_save_player_t;

void aether_save_player_pack(aether_save_player_t *out,
                              const aether_player_health_t *health,
                              const aether_player_inventory_t *inv,
                              aether_vec3_t origin,
                              aether_vec3_t angles,
                              aether_vec3_t velocity);

void aether_save_player_unpack(const aether_save_player_t *in,
                                aether_player_health_t *health,
                                aether_player_inventory_t *inv,
                                aether_vec3_t *out_origin,
                                aether_vec3_t *out_angles,
                                aether_vec3_t *out_velocity);

void aether_save_player_dump(const aether_save_player_t *p);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_SAVE_PLAYER_H */
