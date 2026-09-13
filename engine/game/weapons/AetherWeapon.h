/* AetherWeapon.h — Base weapon class + data-driven definitions.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_WEAPON_H
#define AETHER_WEAPON_H

#include "../../core/AetherCore.h"
#include "../../core/AetherMath.h"
#include "../../player/AetherPlayerInventory.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_WEAPON_NAME_MAX  64
#define AETHER_WEAPON_VIEWMODEL_MAX 128
#define AETHER_WEAPON_WORLDMODEL_MAX 128

/* Fire modes */
typedef enum aether_fire_mode {
    AETHER_FIRE_SINGLE = 0,   /* one shot per click */
    AETHER_FIRE_AUTO,         /* full auto */
    AETHER_FIRE_BURST,        /* 3-round burst */
    AETHER_FIRE_MELEE,        /* crowbar */
    AETHER_FIRE_CHARGE,       /* gauss, egon */
} aether_fire_mode_t;

/* Weapon flags */
typedef enum aether_weapon_flags {
    AETHER_WFLAG_NONE      = 0,
    AETHER_WFLAG_HITSCAN   = (1u << 0),   /* instant ray (bullets) */
    AETHER_WFLAG_PROJECTILE= (1u << 1),   /* physical projectile */
    AETHER_WFLAG_EXPLOSIVE = (1u << 2),   /* RPG, grenade */
    AETHER_WFLAG_ZOOM      = (1u << 3),   /* crossbow, sniper */
    AETHER_WFLAG_LOOP_FIRE = (1u << 4),   /* egon beam */
} aether_weapon_flags_t;

/* Weapon definition (immutable, one per weapon type) */
typedef struct aether_weapon_def {
    aether_weapon_id_t      id;
    const char             *display_name;
    const char             *view_model;      /* "models/v_9mmhandgun.mdl" */
    const char             *world_model;     /* "models/w_9mmhandgun.mdl" */
    aether_ammo_type_t      ammo_type;
    aether_fire_mode_t      fire_mode;
    u32                     flags;

    i32                     clip_size;       /* rounds per magazine */
    i32                     damage;          /* per bullet */
    f32                     fire_rate;       /* seconds between shots */
    f32                     reload_time;     /* seconds to reload */
    f32                     spread;          /* accuracy (radians) */
    f32                     recoil;          /* view kick */
    f32                     range;           /* max distance */
    i32                     pellets;         /* shotgun = 6, rifle = 1 */
    f32                     bullet_speed;    /* for projectiles */
    f32                     explosion_damage;
    f32                     explosion_radius;

    /* Animation */
    const char             *idle_anim;
    const char             *fire_anim;
    const char             *reload_anim;
    const char             *draw_anim;
} aether_weapon_def_t;

/* Runtime weapon state (per player, per active weapon) */
typedef struct aether_weapon_state {
    aether_weapon_id_t      id;
    const aether_weapon_def_t *def;
    i32                     clip_ammo;
    f32                     next_fire_time;
    f32                     reload_end_time;
    bool                    reloading;
    f32                     last_fire_time;
    i32                     burst_remaining;
} aether_weapon_state_t;

/* ---------------- Weapon lifecycle ---------------- */
void aether_weapon_state_init(aether_weapon_state_t *ws, aether_weapon_id_t id);
bool aether_weapon_can_fire(const aether_weapon_state_t *ws, f32 now);
bool aether_weapon_can_reload(const aether_weapon_state_t *ws);
void aether_weapon_start_reload(aether_weapon_state_t *ws, f32 now);
void aether_weapon_finish_reload(aether_weapon_state_t *ws,
                                  aether_player_inventory_t *inv);

/* Attempt to fire. Returns true if a shot was made. */
bool aether_weapon_fire(aether_weapon_state_t *ws,
                         aether_player_inventory_t *inv,
                         f32 now);

/* Tick reload/fire timers */
void aether_weapon_tick(aether_weapon_state_t *ws, f32 dt);

/* Diagnostics */
const aether_weapon_def_t *aether_weapon_get_def(aether_weapon_id_t id);
const char *aether_weapon_name(aether_weapon_id_t id);
void aether_weapon_state_dump(const aether_weapon_state_t *ws);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_WEAPON_H */
