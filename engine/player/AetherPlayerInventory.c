/* AetherPlayerInventory.c — Player inventory implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherPlayerInventory.h"
#include <string.h>

/* Max ammo defaults (GoldSrc values) */
static const i32 k_default_max_ammo[AETHER_AMMO_TYPE_COUNT] = {
    250,   /* 9mm */
    36,    /* 357 */
    125,   /* buckshot */
    50,    /* bolt */
    5,     /* rpg */
    100,   /* uranium */
    10,    /* AR grenade */
    10,    /* hand grenade */
};

/* Weapon → ammo type map */
static aether_ammo_type_t weapon_ammo(aether_weapon_id_t w) {
    switch (w) {
        case AETHER_WPN_GLOCK:      return AETHER_AMMO_9MM;
        case AETHER_WPN_PYTHON:     return AETHER_AMMO_357;
        case AETHER_WPN_MP5:        return AETHER_AMMO_9MM;
        case AETHER_WPN_SHOTGUN:    return AETHER_AMMO_BUCKSHOT;
        case AETHER_WPN_CROSSBOW:   return AETHER_AMMO_BOLT;
        case AETHER_WPN_RPG:        return AETHER_AMMO_RPG;
        case AETHER_WPN_GAUSS:      return AETHER_AMMO_URANIUM;
        case AETHER_WPN_EGON:       return AETHER_AMMO_URANIUM;
        case AETHER_WPN_HIVEHAND:   return AETHER_AMMO_NONE;
        case AETHER_WPN_GRENADE:    return AETHER_AMMO_GRENADE;
        default:                    return AETHER_AMMO_NONE;
    }
}

int aether_player_inv_weapon_slot(aether_weapon_id_t w) {
    switch (w) {
        case AETHER_WPN_CROWBAR:  return 0;
        case AETHER_WPN_GLOCK:
        case AETHER_WPN_PYTHON:   return 1;
        case AETHER_WPN_MP5:
        case AETHER_WPN_SHOTGUN:
        case AETHER_WPN_CROSSBOW: return 2;
        case AETHER_WPN_RPG:
        case AETHER_WPN_GAUSS:
        case AETHER_WPN_EGON:     return 3;
        case AETHER_WPN_HIVEHAND: return 4;
        case AETHER_WPN_GRENADE:
        case AETHER_WPN_SATCHEL:
        case AETHER_WPN_TRIPMINE: return 5;
        case AETHER_WPN_SNARK:    return 6;
        default:                  return -1;
    }
}

void aether_player_inv_init(aether_player_inventory_t *inv) {
    if (!inv) return;
    memset(inv, 0, sizeof *inv);
    for (int i = 0; i < AETHER_AMMO_TYPE_COUNT; ++i) {
        inv->ammo[i] = 0;
        inv->max_ammo[i] = k_default_max_ammo[i];
    }
    inv->active = AETHER_WPN_NONE;

    /* Player starts with crowbar (GoldSrc default) */
    aether_player_inv_give_weapon(inv, AETHER_WPN_CROWBAR);

    aether_log(AETHER_LOG_INFO, "inv", "inventory initialized");
}

void aether_player_inv_reset(aether_player_inventory_t *inv) {
    if (!inv) return;
    bool had_longjump = inv->has_longjump;
    aether_player_inv_init(inv);
    inv->has_longjump = had_longjump;
}

void aether_player_inv_give_weapon(aether_player_inventory_t *inv, aether_weapon_id_t w) {
    if (!inv || w <= 0 || w >= AETHER_WPN_COUNT) return;
    if (inv->own_weapon[w]) return;

    inv->own_weapon[w] = true;

    /* Assign to slot if empty */
    int slot = aether_player_inv_weapon_slot(w);
    if (slot >= 0 && slot < AETHER_WEAPON_SLOT_COUNT) {
        if (inv->slots[slot] == AETHER_WPN_NONE)
            inv->slots[slot] = w;
    }

    aether_log(AETHER_LOG_INFO, "inv", "given weapon id=%d", (int)w);
}

bool aether_player_inv_has_weapon(const aether_player_inventory_t *inv, aether_weapon_id_t w) {
    if (!inv || w <= 0 || w >= AETHER_WPN_COUNT) return false;
    return inv->own_weapon[w];
}

void aether_player_inv_switch(aether_player_inventory_t *inv, aether_weapon_id_t w) {
    if (!inv || w <= 0 || w >= AETHER_WPN_COUNT) return;
    if (!inv->own_weapon[w]) {
        aether_log(AETHER_LOG_WARN, "inv", "weapon %d not owned", (int)w);
        return;
    }
    inv->active = w;
    aether_log(AETHER_LOG_INFO, "inv", "switched to weapon id=%d", (int)w);
}

aether_weapon_id_t aether_player_inv_current(const aether_player_inventory_t *inv) {
    return inv ? inv->active : AETHER_WPN_NONE;
}

void aether_player_inv_give_ammo(aether_player_inventory_t *inv, aether_ammo_type_t t, i32 count) {
    if (!inv || t < 0 || t >= AETHER_AMMO_TYPE_COUNT || count <= 0) return;
    inv->ammo[t] += count;
    if (inv->ammo[t] > inv->max_ammo[t]) inv->ammo[t] = inv->max_ammo[t];
    aether_log(AETHER_LOG_DEBUG, "inv", "ammo[%d] = %d", (int)t, inv->ammo[t]);
}

bool aether_player_inv_has_ammo(const aether_player_inventory_t *inv, aether_ammo_type_t t, i32 count) {
    if (!inv || t < 0 || t >= AETHER_AMMO_TYPE_COUNT) return false;
    return inv->ammo[t] >= count;
}

bool aether_player_inv_use_ammo(aether_player_inventory_t *inv, aether_ammo_type_t t, i32 count) {
    if (!inv || t < 0 || t >= AETHER_AMMO_TYPE_COUNT || count <= 0) return false;
    if (inv->ammo[t] < count) return false;
    inv->ammo[t] -= count;
    return true;
}

i32 aether_player_inv_get_ammo(const aether_player_inventory_t *inv, aether_ammo_type_t t) {
    if (!inv || t < 0 || t >= AETHER_AMMO_TYPE_COUNT) return 0;
    return inv->ammo[t];
}

void aether_player_inv_give_longjump(aether_player_inventory_t *inv) {
    if (!inv) return;
    inv->has_longjump = true;
    aether_log(AETHER_LOG_INFO, "inv", "longjump module acquired");
}

void aether_player_inv_dump(const aether_player_inventory_t *inv) {
    if (!inv) return;
    aether_log(AETHER_LOG_INFO, "inv", "===== INVENTORY =====");
    aether_log(AETHER_LOG_INFO, "inv", "  active weapon: %d", (int)inv->active);
    aether_log(AETHER_LOG_INFO, "inv", "  ammo:");
    aether_log(AETHER_LOG_INFO, "inv", "    9mm     : %d", inv->ammo[AETHER_AMMO_9MM]);
    aether_log(AETHER_LOG_INFO, "inv", "    357     : %d", inv->ammo[AETHER_AMMO_357]);
    aether_log(AETHER_LOG_INFO, "inv", "    buckshot: %d", inv->ammo[AETHER_AMMO_BUCKSHOT]);
    aether_log(AETHER_LOG_INFO, "inv", "    bolt    : %d", inv->ammo[AETHER_AMMO_BOLT]);
    aether_log(AETHER_LOG_INFO, "inv", "    rpg     : %d", inv->ammo[AETHER_AMMO_RPG]);
    aether_log(AETHER_LOG_INFO, "inv", "    uranium : %d", inv->ammo[AETHER_AMMO_URANIUM]);
    aether_log(AETHER_LOG_INFO, "inv", "    argren  : %d", inv->ammo[AETHER_AMMO_ARG]);
    aether_log(AETHER_LOG_INFO, "inv", "    grenade : %d", inv->ammo[AETHER_AMMO_GRENADE]);
    aether_log(AETHER_LOG_INFO, "inv", "  longjump: %s",
               inv->has_longjump ? "yes" : "no");
    aether_log(AETHER_LOG_INFO, "inv", "=====================");
}
