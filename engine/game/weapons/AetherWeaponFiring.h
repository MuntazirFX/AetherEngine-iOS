/* AetherWeaponFiring.h — Hitscan/projectile firing logic (spread, recoil).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_WEAPON_FIRING_H
#define AETHER_WEAPON_FIRING_H

#include "AetherWeapon.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Result of a single hitscan shot */
typedef struct aether_hitscan_result {
    bool        hit;
    aether_vec3_t start;
    aether_vec3_t end;
    aether_vec3_t hit_point;
    f32         distance;
    f32         damage;
    u32         surface_flags;   /* texture flags (metal, wood, etc.) */
} aether_hitscan_result_t;

/* Compute a shot direction with spread applied.
 * view_dir must be unit length. Output dir_out is unit length. */
void aether_weapon_apply_spread(aether_vec3_t view_dir,
                                 f32 spread,
                                 aether_vec3_t *dir_out);

/* Fire a hitscan (bullets/shotgun pellets).
 * Returns number of pellets traced. Fills results[] (one per pellet). */
u32  aether_weapon_fire_hitscan(const aether_weapon_def_t *def,
                                 aether_vec3_t origin,
                                 aether_vec3_t view_dir,
                                 aether_hitscan_result_t *results,
                                 u32 max_results);

/* Compute recoil: kick view angles. */
void aether_weapon_apply_recoil(aether_vec3_t *view_angles,
                                 f32 recoil_strength);

/* ---------- Combat path: hitscan hit → auth damage queue ---------- */
typedef struct aether_weapon_combat_hit {
    bool fired;
    bool hit;
    bool queued;
    u32  pellets;
    u32  killer_id;
    u32  victim_id;
    f32  damage;
    u32  dmg_type;
    aether_hitscan_result_t primary;
} aether_weapon_combat_hit_t;

typedef void (*aether_weapon_auth_queue_fn)(void *user,
                                           u32 killer_id, u32 victim_id,
                                           f32 damage, u32 dmg_type);

/* Fire hitscan; if force_hit (or a pellet hits), invoke queue_fn with total damage.
 * Does not require GameManager — bridge/game tick supplies the queue callback. */
u32 aether_weapon_fire_combat_auth(aether_weapon_state_t *ws,
                                   aether_player_inventory_t *inv,
                                   f32 now,
                                   aether_vec3_t origin,
                                   aether_vec3_t view_dir,
                                   u32 killer_id, u32 victim_id,
                                   bool force_hit,
                                   aether_weapon_auth_queue_fn queue_fn,
                                   void *queue_user,
                                   aether_weapon_combat_hit_t *out);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_WEAPON_FIRING_H */
