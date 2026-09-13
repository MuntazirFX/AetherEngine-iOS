/* AetherWeaponRegistry.c — Per-player weapon registry implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherWeaponRegistry.h"
#include <string.h>

void aether_weapon_registry_init(aether_weapon_registry_t *reg) {
    if (!reg) return;
    memset(reg, 0, sizeof *reg);
    reg->active = AETHER_WPN_NONE;
    for (int i = 0; i < AETHER_WEAPON_REGISTRY_MAX; ++i) {
        reg->states[i].id = AETHER_WPN_NONE;
        reg->states[i].def = NULL;
    }
    aether_log(AETHER_LOG_INFO, "wpnreg", "weapon registry initialized");
}

void aether_weapon_registry_reset(aether_weapon_registry_t *reg) {
    if (!reg) return;
    aether_weapon_id_t cur = reg->active;
    aether_weapon_registry_init(reg);
    reg->active = cur;
}

bool aether_weapon_registry_give(aether_weapon_registry_t *reg, aether_weapon_id_t id) {
    if (!reg || id <= 0 || id >= AETHER_WEAPON_REGISTRY_MAX) return false;

    aether_weapon_state_t *ws = &reg->states[id];
    if (ws->def == NULL) {
        aether_weapon_state_init(ws, id);
        aether_log(AETHER_LOG_INFO, "wpnreg", "gave weapon: %s",
                   aether_weapon_name(id));
    } else {
        /* Already owned - top off clip */
        if (ws->def)
            ws->clip_ammo = ws->def->clip_size;
    }
    return true;
}

bool aether_weapon_registry_select(aether_weapon_registry_t *reg, aether_weapon_id_t id) {
    if (!reg || id <= 0 || id >= AETHER_WEAPON_REGISTRY_MAX) return false;
    aether_weapon_state_t *ws = &reg->states[id];
    if (!ws->def) {
        aether_log(AETHER_LOG_WARN, "wpnreg", "weapon %d not owned", (int)id);
        return false;
    }
    reg->active = id;
    aether_log(AETHER_LOG_INFO, "wpnreg", "selected: %s", ws->def->display_name);
    return true;
}

aether_weapon_state_t *aether_weapon_registry_active(aether_weapon_registry_t *reg) {
    if (!reg || reg->active <= 0 || reg->active >= AETHER_WEAPON_REGISTRY_MAX) return NULL;
    return &reg->states[reg->active];
}

const aether_weapon_state_t *aether_weapon_registry_active_const(const aether_weapon_registry_t *reg) {
    if (!reg || reg->active <= 0 || reg->active >= AETHER_WEAPON_REGISTRY_MAX) return NULL;
    return &reg->states[reg->active];
}

aether_weapon_state_t *aether_weapon_registry_get(aether_weapon_registry_t *reg, aether_weapon_id_t id) {
    if (!reg || id <= 0 || id >= AETHER_WEAPON_REGISTRY_MAX) return NULL;
    return &reg->states[id];
}

void aether_weapon_registry_dump(const aether_weapon_registry_t *reg) {
    if (!reg) return;
    aether_log(AETHER_LOG_INFO, "wpnreg", "===== WEAPON REGISTRY =====");
    aether_log(AETHER_LOG_INFO, "wpnreg", "  active: %s",
               aether_weapon_name(reg->active));
    for (int i = 1; i < AETHER_WEAPON_REGISTRY_MAX; ++i) {
        if (reg->states[i].def) {
            aether_weapon_state_dump(&reg->states[i]);
        }
    }
    aether_log(AETHER_LOG_INFO, "wpnreg", "============================");
}
