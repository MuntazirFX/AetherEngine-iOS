/* AetherWeapon.c — Base weapon implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherWeapon.h"
#include "AetherWeaponDefs.h"
#include <string.h>

void aether_weapon_state_init(aether_weapon_state_t *ws, aether_weapon_id_t id) {
    if (!ws) return;
    memset(ws, 0, sizeof *ws);
    ws->id = id;
    ws->def = aether_weapon_get_def(id);
    ws->clip_ammo = ws->def ? ws->def->clip_size : 0;
    ws->next_fire_time = 0.0f;
    ws->reload_end_time = 0.0f;
    ws->reloading = false;
    ws->last_fire_time = 0.0f;
    ws->burst_remaining = 0;
    if (ws->def)
        aether_log(AETHER_LOG_INFO, "weapon", "initialized '%s'", ws->def->display_name);
}

const aether_weapon_def_t *aether_weapon_get_def(aether_weapon_id_t id) {
    return aether_weapon_defs_lookup(id);
}

const char *aether_weapon_name(aether_weapon_id_t id) {
    const aether_weapon_def_t *d = aether_weapon_get_def(id);
    return d ? d->display_name : "unknown";
}

bool aether_weapon_can_fire(const aether_weapon_state_t *ws, f32 now) {
    if (!ws || !ws->def) return false;
    if (ws->reloading) return false;
    if (now < ws->next_fire_time) return false;
    if (ws->def->fire_mode != AETHER_FIRE_MELEE && ws->clip_ammo <= 0) return false;
    return true;
}

bool aether_weapon_can_reload(const aether_weapon_state_t *ws) {
    if (!ws || !ws->def) return false;
    if (ws->reloading) return false;
    if (ws->def->clip_size <= 0) return false;   /* melee has no mag */
    if (ws->clip_ammo >= ws->def->clip_size) return false;
    return true;
}

void aether_weapon_start_reload(aether_weapon_state_t *ws, f32 now) {
    if (!aether_weapon_can_reload(ws)) return;
    ws->reloading = true;
    ws->reload_end_time = now + ws->def->reload_time;
    aether_log(AETHER_LOG_DEBUG, "weapon", "%s: reloading (%.1fs)",
               ws->def->display_name, ws->def->reload_time);
}

void aether_weapon_finish_reload(aether_weapon_state_t *ws,
                                  aether_player_inventory_t *inv) {
    if (!ws || !ws->def || !inv) return;
    ws->reloading = false;

    i32 needed = ws->def->clip_size - ws->clip_ammo;
    i32 available = aether_player_inv_get_ammo(inv, ws->def->ammo_type);
    i32 to_load = needed < available ? needed : available;

    if (to_load > 0) {
        aether_player_inv_use_ammo(inv, ws->def->ammo_type, to_load);
        ws->clip_ammo += to_load;
    }

    aether_log(AETHER_LOG_DEBUG, "weapon", "%s: reloaded %d (clip=%d)",
               ws->def->display_name, to_load, ws->clip_ammo);
}

bool aether_weapon_fire(aether_weapon_state_t *ws,
                         aether_player_inventory_t *inv,
                         f32 now) {
    if (!aether_weapon_can_fire(ws, now)) return false;

    const aether_weapon_def_t *d = ws->def;

    /* Consume ammo (melee = free) */
    if (d->fire_mode != AETHER_FIRE_MELEE) {
        ws->clip_ammo -= 1;
        if (ws->clip_ammo < 0) ws->clip_ammo = 0;
    }

    ws->last_fire_time = now;
    ws->next_fire_time = now + d->fire_rate;

    aether_log(AETHER_LOG_INFO, "weapon", "%s fired (clip=%d/%d, dmg=%d)",
               d->display_name, ws->clip_ammo, d->clip_size, d->damage);

    (void)inv;
    return true;
}

void aether_weapon_tick(aether_weapon_state_t *ws, f32 dt) {
    if (!ws) return;
    /* Handled by fire/reload logic using absolute time — nothing per-frame */
    (void)dt;
}

void aether_weapon_state_dump(const aether_weapon_state_t *ws) {
    if (!ws || !ws->def) {
        aether_log(AETHER_LOG_INFO, "weapon", "  (none)");
        return;
    }
    aether_log(AETHER_LOG_INFO, "weapon",
               "  %-20s clip=%d/%d reload=%s",
               ws->def->display_name,
               ws->clip_ammo, ws->def->clip_size,
               ws->reloading ? "yes" : "no");
}
