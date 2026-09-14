/* AetherWeaponDefs.c — Data table of all 15 weapons.
 * AetherEngine-iOS · Clean-room.
 * Uses designated initializers for NONE to avoid field-count errors.
 */
#include "AetherWeaponDefs.h"
#include <string.h>

/* Static table (one entry per AETHER_WPN_* id).
 * NONE entry uses designated init — all unspecified fields = 0/NULL.
 * Real entries use positional init for compactness. */
static const aether_weapon_def_t k_weapons[AETHER_WPN_COUNT] = {

    /* 0: NONE — designated initializer (rest auto-zeroed) */
    [AETHER_WPN_NONE] = {
        .id           = AETHER_WPN_NONE,
        .display_name = "None",
        .ammo_type    = AETHER_AMMO_NONE,
        .fire_mode    = AETHER_FIRE_SINGLE
    },

    /* 1: CROWBAR */
    [AETHER_WPN_CROWBAR] = {
        .id           = AETHER_WPN_CROWBAR,
        .display_name = "Crowbar",
        .view_model   = "models/v_crowbar.mdl",
        .world_model  = "models/w_crowbar.mdl",
        .ammo_type    = AETHER_AMMO_NONE,
        .fire_mode    = AETHER_FIRE_MELEE,
        .flags        = AETHER_WFLAG_HITSCAN,
        .clip_size    = 0,
        .damage       = 15,
        .fire_rate    = 0.4f,
        .reload_time  = 0.0f,
        .spread       = 0.0f,
        .recoil       = 0.0f,
        .range        = 64.0f,
        .pellets      = 1,
        .idle_anim    = "crowbar_idle",
        .fire_anim    = "crowbar_attack",
        .reload_anim  = NULL,
        .draw_anim    = "crowbar_draw"
    },

    /* 2: GLOCK 17 */
    [AETHER_WPN_GLOCK] = {
        .id           = AETHER_WPN_GLOCK,
        .display_name = "9mm Handgun",
        .view_model   = "models/v_9mmhandgun.mdl",
        .world_model  = "models/w_9mmhandgun.mdl",
        .ammo_type    = AETHER_AMMO_9MM,
        .fire_mode    = AETHER_FIRE_SINGLE,
        .flags        = AETHER_WFLAG_HITSCAN,
        .clip_size    = 17,
        .damage       = 8,
        .fire_rate    = 0.15f,
        .reload_time  = 2.2f,
        .spread       = 0.008f,
        .recoil       = 0.6f,
        .range        = 8192.0f,
        .pellets      = 1,
        .idle_anim    = "glock_idle1",
        .fire_anim    = "glock_shoot1",
        .reload_anim  = "glock_reload",
        .draw_anim    = "glock_draw"
    },

    /* 3: .357 MAGNUM */
    [AETHER_WPN_PYTHON] = {
        .id           = AETHER_WPN_PYTHON,
        .display_name = "357 Magnum",
        .view_model   = "models/v_357.mdl",
        .world_model  = "models/w_357.mdl",
        .ammo_type    = AETHER_AMMO_357,
        .fire_mode    = AETHER_FIRE_SINGLE,
        .flags        = AETHER_WFLAG_HITSCAN,
        .clip_size    = 6,
        .damage       = 40,
        .fire_rate    = 0.6f,
        .reload_time  = 3.0f,
        .spread       = 0.005f,
        .recoil       = 2.0f,
        .range        = 8192.0f,
        .pellets      = 1,
        .idle_anim    = "python_idle1",
        .fire_anim    = "python_shoot",
        .reload_anim  = "python_reload",
        .draw_anim    = "python_draw"
    },

    /* 4: MP5 */
    [AETHER_WPN_MP5] = {
        .id           = AETHER_WPN_MP5,
        .display_name = "9mm AR (MP5)",
        .view_model   = "models/v_9mmAR.mdl",
        .world_model  = "models/w_9mmAR.mdl",
        .ammo_type    = AETHER_AMMO_9MM,
        .fire_mode    = AETHER_FIRE_AUTO,
        .flags        = AETHER_WFLAG_HITSCAN,
        .clip_size    = 30,
        .damage       = 8,
        .fire_rate    = 0.1f,
        .reload_time  = 2.5f,
        .spread       = 0.015f,
        .recoil       = 0.8f,
        .range        = 8192.0f,
        .pellets      = 1,
        .idle_anim    = "mp5_idle1",
        .fire_anim    = "mp5_fire1",
        .reload_anim  = "mp5_reload",
        .draw_anim    = "mp5_draw"
    },

    /* 5: SHOTGUN */
    [AETHER_WPN_SHOTGUN] = {
        .id           = AETHER_WPN_SHOTGUN,
        .display_name = "Shotgun",
        .view_model   = "models/v_shotgun.mdl",
        .world_model  = "models/w_shotgun.mdl",
        .ammo_type    = AETHER_AMMO_BUCKSHOT,
        .fire_mode    = AETHER_FIRE_SINGLE,
        .flags        = AETHER_WFLAG_HITSCAN,
        .clip_size    = 8,
        .damage       = 10,
        .fire_rate    = 0.75f,
        .reload_time  = 0.5f,
        .spread       = 0.08f,
        .recoil       = 3.0f,
        .range        = 2048.0f,
        .pellets      = 6,
        .idle_anim    = "shotgun_idle",
        .fire_anim    = "shotgun_fire",
        .reload_anim  = "shotgun_reload",
        .draw_anim    = "shotgun_draw"
    },

    /* 6: CROSSBOW */
    [AETHER_WPN_CROSSBOW] = {
        .id           = AETHER_WPN_CROSSBOW,
        .display_name = "Crossbow",
        .view_model   = "models/v_crossbow.mdl",
        .world_model  = "models/w_crossbow.mdl",
        .ammo_type    = AETHER_AMMO_BOLT,
        .fire_mode    = AETHER_FIRE_SINGLE,
        .flags        = AETHER_WFLAG_HITSCAN | AETHER_WFLAG_ZOOM,
        .clip_size    = 5,
        .damage       = 100,
        .fire_rate    = 0.75f,
        .reload_time  = 4.0f,
        .spread       = 0.001f,
        .recoil       = 2.0f,
        .range        = 8192.0f,
        .pellets      = 1,
        .idle_anim    = "crossbow_idle1",
        .fire_anim    = "crossbow_fire",
        .reload_anim  = "crossbow_reload",
        .draw_anim    = "crossbow_draw"
    },

    /* 7: RPG */
    [AETHER_WPN_RPG] = {
        .id               = AETHER_WPN_RPG,
        .display_name     = "RPG",
        .view_model       = "models/v_rpg.mdl",
        .world_model      = "models/w_rpg.mdl",
        .ammo_type        = AETHER_AMMO_RPG,
        .fire_mode        = AETHER_FIRE_SINGLE,
        .flags            = AETHER_WFLAG_PROJECTILE | AETHER_WFLAG_EXPLOSIVE,
        .clip_size        = 1,
        .damage           = 100,
        .fire_rate        = 1.5f,
        .reload_time      = 5.0f,
        .spread           = 0.0f,
        .recoil           = 3.0f,
        .range            = 8192.0f,
        .pellets          = 1,
        .bullet_speed     = 900.0f,
        .explosion_damage = 100.0f,
        .explosion_radius = 200.0f,
        .idle_anim        = "rpg_idle",
        .fire_anim        = "rpg_fire",
        .reload_anim      = "rpg_reload",
        .draw_anim        = "rpg_draw"
    },

    /* 8: GAUSS */
    [AETHER_WPN_GAUSS] = {
        .id           = AETHER_WPN_GAUSS,
        .display_name = "Gauss Gun",
        .view_model   = "models/v_gauss.mdl",
        .world_model  = "models/w_gauss.mdl",
        .ammo_type    = AETHER_AMMO_URANIUM,
        .fire_mode    = AETHER_FIRE_CHARGE,
        .flags        = AETHER_WFLAG_HITSCAN | AETHER_WFLAG_ZOOM,
        .clip_size    = 0,
        .damage       = 20,
        .fire_rate    = 0.3f,
        .reload_time  = 0.0f,
        .spread       = 0.005f,
        .recoil       = 1.5f,
        .range        = 8192.0f,
        .pellets      = 1,
        .idle_anim    = "gauss_idle",
        .fire_anim    = "gauss_fire",
        .reload_anim  = NULL,
        .draw_anim    = "gauss_draw"
    },

    /* 9: EGON */
    [AETHER_WPN_EGON] = {
        .id           = AETHER_WPN_EGON,
        .display_name = "Egon",
        .view_model   = "models/v_egon.mdl",
        .world_model  = "models/w_egon.mdl",
        .ammo_type    = AETHER_AMMO_URANIUM,
        .fire_mode    = AETHER_FIRE_CHARGE,
        .flags        = AETHER_WFLAG_LOOP_FIRE,
        .clip_size    = 0,
        .damage       = 15,
        .fire_rate    = 0.1f,
        .reload_time  = 0.0f,
        .spread       = 0.005f,
        .recoil       = 1.0f,
        .range        = 8192.0f,
        .pellets      = 1,
        .idle_anim    = "egon_idle",
        .fire_anim    = "egon_fire",
        .reload_anim  = NULL,
        .draw_anim    = "egon_draw"
    },

    /* 10: HIVEHAND */
    [AETHER_WPN_HIVEHAND] = {
        .id           = AETHER_WPN_HIVEHAND,
        .display_name = "Hivehand",
        .view_model   = "models/v_hivehand.mdl",
        .world_model  = "models/w_hivehand.mdl",
        .ammo_type    = AETHER_AMMO_NONE,
        .fire_mode    = AETHER_FIRE_AUTO,
        .flags        = AETHER_WFLAG_PROJECTILE,
        .clip_size    = 0,
        .damage       = 10,
        .fire_rate    = 0.15f,
        .reload_time  = 0.0f,
        .spread       = 0.02f,
        .recoil       = 0.5f,
        .range        = 2048.0f,
        .pellets      = 1,
        .bullet_speed = 800.0f,
        .idle_anim    = "hivehand_idle",
        .fire_anim    = "hivehand_fire",
        .reload_anim  = NULL,
        .draw_anim    = "hivehand_draw"
    },

    /* 11: HAND GRENADE */
    [AETHER_WPN_GRENADE] = {
        .id               = AETHER_WPN_GRENADE,
        .display_name     = "Hand Grenade",
        .view_model       = "models/v_handgrenade.mdl",
        .world_model      = "models/w_handgrenade.mdl",
        .ammo_type        = AETHER_AMMO_GRENADE,
        .fire_mode        = AETHER_FIRE_SINGLE,
        .flags            = AETHER_WFLAG_PROJECTILE | AETHER_WFLAG_EXPLOSIVE,
        .clip_size        = 0,
        .damage           = 100,
        .fire_rate        = 0.8f,
        .reload_time      = 0.0f,
        .spread           = 0.0f,
        .recoil           = 0.0f,
        .range            = 8192.0f,
        .pellets          = 1,
        .bullet_speed     = 900.0f,
        .explosion_damage = 100.0f,
        .explosion_radius = 200.0f,
        .idle_anim        = "gren_idle",
        .fire_anim        = "gren_throw",
        .reload_anim      = NULL,
        .draw_anim        = "gren_draw"
    },

    /* 12: SATCHEL */
    [AETHER_WPN_SATCHEL] = {
        .id               = AETHER_WPN_SATCHEL,
        .display_name     = "Satchel Charge",
        .view_model       = "models/v_satchel.mdl",
        .world_model      = "models/w_satchel.mdl",
        .ammo_type        = AETHER_AMMO_NONE,
        .fire_mode        = AETHER_FIRE_SINGLE,
        .flags            = AETHER_WFLAG_PROJECTILE | AETHER_WFLAG_EXPLOSIVE,
        .clip_size        = 0,
        .damage           = 150,
        .fire_rate        = 0.5f,
        .reload_time      = 0.0f,
        .spread           = 0.0f,
        .recoil           = 0.0f,
        .range            = 8192.0f,
        .pellets          = 1,
        .bullet_speed     = 500.0f,
        .explosion_damage = 150.0f,
        .explosion_radius = 250.0f,
        .idle_anim        = "satchel_idle",
        .fire_anim        = "satchel_throw",
        .reload_anim      = NULL,
        .draw_anim        = "satchel_draw"
    },

    /* 13: TRIPMINE */
    [AETHER_WPN_TRIPMINE] = {
        .id               = AETHER_WPN_TRIPMINE,
        .display_name     = "Trip Mine",
        .view_model       = "models/v_tripmine.mdl",
        .world_model      = "models/w_tripmine.mdl",
        .ammo_type        = AETHER_AMMO_NONE,
        .fire_mode        = AETHER_FIRE_SINGLE,
        .flags            = AETHER_WFLAG_PROJECTILE | AETHER_WFLAG_EXPLOSIVE,
        .clip_size        = 0,
        .damage           = 150,
        .fire_rate        = 1.0f,
        .reload_time      = 0.0f,
        .spread           = 0.0f,
        .recoil           = 0.0f,
        .range            = 2048.0f,
        .pellets          = 1,
        .bullet_speed     = 500.0f,
        .explosion_damage = 150.0f,
        .explosion_radius = 150.0f,
        .idle_anim        = "tripmine_idle",
        .fire_anim        = "tripmine_place",
        .reload_anim      = NULL,
        .draw_anim        = "tripmine_draw"
    },

    /* 14: SNARK */
    [AETHER_WPN_SNARK] = {
        .id               = AETHER_WPN_SNARK,
        .display_name     = "Snark",
        .view_model       = "models/v_squeak.mdl",
        .world_model      = "models/w_squeak.mdl",
        .ammo_type        = AETHER_AMMO_NONE,
        .fire_mode        = AETHER_FIRE_SINGLE,
        .flags            = AETHER_WFLAG_PROJECTILE,
        .clip_size        = 0,
        .damage           = 20,
        .fire_rate        = 0.5f,
        .reload_time      = 0.0f,
        .spread           = 0.0f,
        .recoil           = 0.0f,
        .range            = 2048.0f,
        .pellets          = 1,
        .bullet_speed     = 800.0f,
        .explosion_damage = 20.0f,
        .explosion_radius = 100.0f,
        .idle_anim        = "snark_idle",
        .fire_anim        = "snark_throw",
        .reload_anim      = NULL,
        .draw_anim        = "snark_draw"
    },
};

const aether_weapon_def_t *aether_weapon_defs_lookup(aether_weapon_id_t id) {
    if (id <= 0 || id >= AETHER_WPN_COUNT) return NULL;
    return &k_weapons[id];
}

const aether_weapon_def_t *aether_weapon_defs_table(void) { return k_weapons; }
u32 aether_weapon_defs_count(void) { return AETHER_WPN_COUNT; }
