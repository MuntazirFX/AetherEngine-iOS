/* AetherPlayerInventory.h — Weapons, ammo, items inventory.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_PLAYER_INVENTORY_H
#define AETHER_PLAYER_INVENTORY_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Weapon slots (GoldSrc: 1-9 keys) */
#define AETHER_WEAPON_SLOT_COUNT  9
#define AETHER_AMMO_TYPE_COUNT    8

/* Ammo types */
typedef enum aether_ammo_type {
    AETHER_AMMO_9MM      = 0,
    AETHER_AMMO_357      = 1,
    AETHER_AMMO_BUCKSHOT = 2,
    AETHER_AMMO_BOLT     = 3,
    AETHER_AMMO_RPG      = 4,
    AETHER_AMMO_URANIUM  = 5,
    AETHER_AMMO_ARG      = 6,   /* AR grenade */
    AETHER_AMMO_GRENADE  = 7,   /* Hand grenade */
    AETHER_AMMO_NONE     = -1
} aether_ammo_type_t;

/* Weapon IDs (matching GoldSrc) */
typedef enum aether_weapon_id {
    AETHER_WPN_NONE       = 0,
    AETHER_WPN_CROWBAR,
    AETHER_WPN_GLOCK,
    AETHER_WPN_PYTHON,
    AETHER_WPN_MP5,
    AETHER_WPN_SHOTGUN,
    AETHER_WPN_CROSSBOW,
    AETHER_WPN_RPG,
    AETHER_WPN_GAUSS,
    AETHER_WPN_EGON,
    AETHER_WPN_HIVEHAND,
    AETHER_WPN_GRENADE,
    AETHER_WPN_SATCHEL,
    AETHER_WPN_TRIPMINE,
    AETHER_WPN_SNARK,
    AETHER_WPN_COUNT
} aether_weapon_id_t;

typedef struct aether_player_inventory {
    /* Weapon ownership */
    bool own_weapon[AETHER_WPN_COUNT];

    /* Currently selected weapon per slot */
    aether_weapon_id_t slots[AETHER_WEAPON_SLOT_COUNT];

    /* Active weapon */
    aether_weapon_id_t active;

    /* Ammo counts */
    i32 ammo[AETHER_AMMO_TYPE_COUNT];
    i32 max_ammo[AETHER_AMMO_TYPE_COUNT];

    /* Items */
    bool has_longjump;
    bool has_antidote;
    bool has_security;
} aether_player_inventory_t;

void aether_player_inv_init(aether_player_inventory_t *inv);
void aether_player_inv_reset(aether_player_inventory_t *inv);

/* Weapons */
void aether_player_inv_give_weapon(aether_player_inventory_t *inv, aether_weapon_id_t w);
bool aether_player_inv_has_weapon (const aether_player_inventory_t *inv, aether_weapon_id_t w);
void aether_player_inv_switch     (aether_player_inventory_t *inv, aether_weapon_id_t w);
aether_weapon_id_t aether_player_inv_current(const aether_player_inventory_t *inv);
int  aether_player_inv_weapon_slot(aether_weapon_id_t w);

/* Ammo */
void aether_player_inv_give_ammo(aether_player_inventory_t *inv, aether_ammo_type_t t, i32 count);
bool aether_player_inv_has_ammo (const aether_player_inventory_t *inv, aether_ammo_type_t t, i32 count);
bool aether_player_inv_use_ammo (aether_player_inventory_t *inv, aether_ammo_type_t t, i32 count);
i32  aether_player_inv_get_ammo (const aether_player_inventory_t *inv, aether_ammo_type_t t);

/* Items */
void aether_player_inv_give_longjump(aether_player_inventory_t *inv);

void aether_player_inv_dump(const aether_player_inventory_t *inv);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_PLAYER_INVENTORY_H */
