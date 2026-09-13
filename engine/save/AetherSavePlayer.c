/* AetherSavePlayer.c — Player state serialization implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherSavePlayer.h"
#include <string.h>

void aether_save_player_pack(aether_save_player_t *out,
                              const aether_player_health_t *health,
                              const aether_player_inventory_t *inv,
                              aether_vec3_t origin,
                              aether_vec3_t angles,
                              aether_vec3_t velocity) {
    if (!out) return;
    memset(out, 0, sizeof *out);

    out->origin[0]   = origin.x;   out->origin[1]   = origin.y;   out->origin[2]   = origin.z;
    out->angles[0]   = angles.x;   out->angles[1]   = angles.y;   out->angles[2]   = angles.z;
    out->velocity[0] = velocity.x; out->velocity[1] = velocity.y; out->velocity[2] = velocity.z;

    if (health) {
        out->health     = health->health;
        out->armor      = health->armor;
        out->battery    = health->battery;
        out->has_suit   = health->has_suit ? 1u : 0u;
        out->flashlight = health->flashlight_on ? 1u : 0u;
    }

    if (inv) {
        for (int i = 0; i < AETHER_WPN_COUNT; ++i)
            out->weapons_owned[i] = inv->own_weapon[i] ? 1u : 0u;
        out->active_weapon = (u32)inv->active;
        for (int i = 0; i < AETHER_AMMO_TYPE_COUNT; ++i)
            out->ammo[i] = inv->ammo[i];
        out->has_longjump = inv->has_longjump ? 1u : 0u;
        out->has_antidote = inv->has_antidote ? 1u : 0u;
        out->has_security = inv->has_security ? 1u : 0u;
    }
}

void aether_save_player_unpack(const aether_save_player_t *in,
                                aether_player_health_t *health,
                                aether_player_inventory_t *inv,
                                aether_vec3_t *out_origin,
                                aether_vec3_t *out_angles,
                                aether_vec3_t *out_velocity) {
    if (!in) return;

    if (out_origin)   *out_origin   = (aether_vec3_t){ in->origin[0], in->origin[1], in->origin[2] };
    if (out_angles)   *out_angles   = (aether_vec3_t){ in->angles[0], in->angles[1], in->angles[2] };
    if (out_velocity) *out_velocity = (aether_vec3_t){ in->velocity[0], in->velocity[1], in->velocity[2] };

    if (health) {
        health->health       = in->health;
        health->max_health   = 100.0f;
        health->armor        = in->armor;
        health->max_armor    = 100.0f;
        health->battery      = in->battery;
        health->max_battery  = 100.0f;
        health->has_suit     = in->has_suit != 0;
        health->flashlight_on= in->flashlight != 0;
        health->dead         = in->health <= 0.0f;
    }

    if (inv) {
        for (int i = 0; i < AETHER_WPN_COUNT; ++i)
            inv->own_weapon[i] = in->weapons_owned[i] != 0;
        inv->active = (aether_weapon_id_t)in->active_weapon;
        for (int i = 0; i < AETHER_AMMO_TYPE_COUNT; ++i)
            inv->ammo[i] = in->ammo[i];
        inv->has_longjump = in->has_longjump != 0;
        inv->has_antidote = in->has_antidote != 0;
        inv->has_security = in->has_security != 0;
    }
}

void aether_save_player_dump(const aether_save_player_t *p) {
    if (!p) return;
    aether_log(AETHER_LOG_INFO, "save-player",
               "pos=(%.0f,%.0f,%.0f) hp=%.0f armor=%.0f weapon=%u",
               p->origin[0], p->origin[1], p->origin[2],
               p->health, p->armor, p->active_weapon);
}
