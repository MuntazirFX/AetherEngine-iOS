/* AetherWeaponDefs.c — Data table of all 15 weapons.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherWeaponDefs.h"
#include <string.h>

/* Static table (one entry per AETHER_WPN_* id) */
static const aether_weapon_def_t k_weapons[AETHER_WPN_COUNT] = {
    /* -------- 0: NONE (unused) -------- */
    { AETHER_WPN_NONE, "None", NULL, NULL, AETHER_AMMO_NONE, AETHER_FIRE_SINGLE, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL },

    /* -------- 1: CROWBAR -------- */
    { AETHER_WPN_CROWBAR, "Crowbar",
      "models/v_crowbar.mdl", "models/w_crowbar.mdl",
      AETHER_AMMO_NONE, AETHER_FIRE_MELEE, AETHER_WFLAG_HITSCAN,
      0, 15, 0.4f, 0.0f, 0.0f, 0.0f, 64.0f, 1, 0.0f, 0.0f, 0.0f,
      "crowbar_idle", "crowbar_attack", NULL, "crowbar_draw" },

    /* -------- 2: GLOCK 17 -------- */
    { AETHER_WPN_GLOCK, "9mm Handgun",
      "models/v_9mmhandgun.mdl", "models/w_9mmhandgun.mdl",
      AETHER_AMMO_9MM, AETHER_FIRE_SINGLE, AETHER_WFLAG_HITSCAN,
      17, 8, 0.15f, 2.2f, 0.008f, 0.6f, 8192.0f, 1, 0.0f, 0.0f, 0.0f,
      "glock_idle1", "glock_shoot1", "glock_reload", "glock_draw" },

    /* -------- 3: .357 MAGNUM -------- */
    { AETHER_WPN_PYTHON, "357 Magnum",
      "models/v_357.mdl", "models/w_357.mdl",
      AETHER_AMMO_357, AETHER_FIRE_SINGLE, AETHER_WFLAG_HITSCAN,
      6, 40, 0.6f, 3.0f, 0.005f, 2.0f, 8192.0f, 1, 0.0f, 0.0f, 0.0f,
      "python_idle1", "python_shoot", "python_reload", "python_draw" },

    /* -------- 4: MP5 -------- */
    { AETHER_WPN_MP5, "9mm AR (MP5)",
      "models/v_9mmAR.mdl", "models/w_9mmAR.mdl",
      AETHER_AMMO_9MM, AETHER_FIRE_AUTO, AETHER_WFLAG_HITSCAN,
      30, 8, 0.1f, 2.5f, 0.015f, 0.8f, 8192.0f, 1, 0.0f, 0.0f, 0.0f,
      "mp5_idle1", "mp5_fire1", "mp5_reload", "mp5_draw" },

    /* -------- 5: SHOTGUN -------- */
    { AETHER_WPN_SHOTGUN, "Shotgun",
      "models/v_shotgun.mdl", "models/w_shotgun.mdl",
      AETHER_AMMO_BUCKSHOT, AETHER_FIRE_SINGLE, AETHER_WFLAG_HITSCAN,
      8, 10, 0.75f, 0.5f, 0.08f, 3.0f, 2048.0f, 6, 0.0f, 0.0f, 0.0f,
      "shotgun_idle", "shotgun_fire", "shotgun_reload", "shotgun_draw" },

    /* -------- 6: CROSSBOW -------- */
    { AETHER_WPN_CROSSBOW, "Crossbow",
      "models/v_crossbow.mdl", "models/w_crossbow.mdl",
      AETHER_AMMO_BOLT, AETHER_FIRE_SINGLE, AETHER_WFLAG_HITSCAN | AETHER_WFLAG_ZOOM,
      5, 100, 0.75f, 4.0f, 0.001f, 2.0f, 8192.0f, 1, 0.0f, 0.0f, 0.0f,
      "crossbow_idle1", "crossbow_fire", "crossbow_reload", "crossbow_draw" },

    /* -------- 7: RPG -------- */
    { AETHER_WPN_RPG, "RPG",
      "models/v_rpg.mdl", "models/w_rpg.mdl",
      AETHER_AMMO_RPG, AETHER_FIRE_SINGLE, AETHER_WFLAG_PROJECTILE | AETHER_WFLAG_EXPLOSIVE,
      1, 100, 1.5f, 5.0f, 0.0f, 3.0f, 8192.0f, 1, 900.0f, 100.0f, 200.0f,
      "rpg_idle", "rpg_fire", "rpg_reload", "rpg_draw" },

    /* -------- 8: GAUSS -------- */
    { AETHER_WPN_GAUSS, "Gauss Gun",
      "models/v_gauss.mdl", "models/w_gauss.mdl",
      AETHER_AMMO_URANIUM, AETHER_FIRE_CHARGE, AETHER_WFLAG_HITSCAN | AETHER_WFLAG_ZOOM,
      0, 20, 0.3f, 0.0f, 0.005f, 1.5f, 8192.0f, 1, 0.0f, 0.0f, 0.0f,
      "gauss_idle", "gauss_fire", NULL, "gauss_draw" },

    /* -------- 9: EGON -------- */
    { AETHER_WPN_EGON, "Egon",
      "models/v_egon.mdl", "models/w_egon.mdl",
      AETHER_AMMO_URANIUM, AETHER_FIRE_CHARGE, AETHER_WFLAG_LOOP_FIRE,
      0, 15, 0.1f, 0.0f, 0.005f, 1.0f, 8192.0f, 1, 0.0f, 0.0f, 0.0f,
      "egon_idle", "egon_fire", NULL, "egon_draw" },

    /* -------- 10: HIVEHAND -------- */
    { AETHER_WPN_HIVEHAND, "Hivehand",
      "models/v_hivehand.mdl", "models/w_hivehand.mdl",
      AETHER_AMMO_NONE, AETHER_FIRE_AUTO, AETHER_WFLAG_PROJECTILE,
      0, 10, 0.15f, 0.0f, 0.02f, 0.5f, 2048.0f, 1, 800.0f, 0.0f, 0.0f,
      "hivehand_idle", "hivehand_fire", NULL, "hivehand_draw" },

    /* -------- 11: HAND GRENADE -------- */
    { AETHER_WPN_GRENADE, "Hand Grenade",
      "models/v_handgrenade.mdl", "models/w_handgrenade.mdl",
      AETHER_AMMO_GRENADE, AETHER_FIRE_SINGLE, AETHER_WFLAG_PROJECTILE | AETHER_WFLAG_EXPLOSIVE,
      0, 100, 0.8f, 0.0f, 0.0f, 0.0f, 8192.0f, 1, 900.0f, 100.0f, 200.0f,
      "gren_idle", "gren_throw", NULL, "gren_draw" },

    /* -------- 12: SATCHEL -------- */
    { AETHER_WPN_SATCHEL, "Satchel Charge",
      "models/v_satchel.mdl", "models/w_satchel.mdl",
      AETHER_AMMO_NONE, AETHER_FIRE_SINGLE, AETHER_WFLAG_PROJECTILE | AETHER_WFLAG_EXPLOSIVE,
      0, 150, 0.5f, 0.0f, 0.0f, 0.0f, 8192.0f, 1, 500.0f, 150.0f, 250.0f,
      "satchel_idle", "satchel_throw", NULL, "satchel_draw" },

    /* -------- 13: TRIPMINE -------- */
    { AETHER_WPN_TRIPMINE, "Trip Mine",
      "models/v_tripmine.mdl", "models/w_tripmine.mdl",
      AETHER_AMMO_NONE, AETHER_FIRE_SINGLE, AETHER_WFLAG_PROJECTILE | AETHER_WFLAG_EXPLOSIVE,
      0, 150, 1.0f, 0.0f, 0.0f, 0.0f, 2048.0f, 1, 500.0f, 150.0f, 150.0f,
      "tripmine_idle", "tripmine_place", NULL, "tripmine_draw" },

    /* -------- 14: SNARK -------- */
    { AETHER_WPN_SNARK, "Snark",
      "models/v_squeak.mdl", "models/w_squeak.mdl",
      AETHER_AMMO_NONE, AETHER_FIRE_SINGLE, AETHER_WFLAG_PROJECTILE,
      0, 20, 0.5f, 0.0f, 0.0f, 0.0f, 2048.0f, 1, 800.0f, 20.0f, 100.0f,
      "snark_idle", "snark_throw", NULL, "snark_draw" },
};

const aether_weapon_def_t *aether_weapon_defs_lookup(aether_weapon_id_t id) {
    if (id <= 0 || id >= AETHER_WPN_COUNT) return NULL;
    return &k_weapons[id];
}

const aether_weapon_def_t *aether_weapon_defs_table(void) { return k_weapons; }
u32 aether_weapon_defs_count(void) { return AETHER_WPN_COUNT; }
