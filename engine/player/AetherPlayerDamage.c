/* AetherPlayerDamage.c — Player damage implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherPlayerDamage.h"
#include "../net/AetherNetServer.h"
#include <math.h>
#include <string.h>

/* Damage scale per type (multiplier on base amount) */
static f32 damage_scale(aether_damage_type_t t) {
    switch (t) {
        case AETHER_DMG_BULLET:    return 1.0f;
        case AETHER_DMG_SLASH:     return 1.0f;
        case AETHER_DMG_BURN:      return 0.5f;
        case AETHER_DMG_FREEZE:    return 1.5f;
        case AETHER_DMG_FALL:      return 1.0f;
        case AETHER_DMG_BLAST:     return 1.5f;
        case AETHER_DMG_CLUB:      return 1.0f;
        case AETHER_DMG_SHOCK:     return 1.2f;
        case AETHER_DMG_SONIC:     return 1.0f;
        case AETHER_DMG_ENERGYBEAM:return 1.0f;
        case AETHER_DMG_DROWN:     return 1.0f;
        case AETHER_DMG_POISON:    return 1.0f;
        case AETHER_DMG_RADIATION: return 1.0f;
        case AETHER_DMG_ACID:      return 1.5f;
        default:                   return 1.0f;
    }
}

void aether_player_apply_damage(aether_player_health_t *h,
                                 const aether_damage_event_t *ev) {
    if (!h || !ev || ev->amount <= 0) return;
    if (h->dead) return;

    f32 scaled = ev->amount * damage_scale(ev->type);

    /* Suit reduces certain damage types by 20% */
    if (h->has_suit) {
        switch (ev->type) {
            case AETHER_DMG_BURN:
            case AETHER_DMG_SHOCK:
            case AETHER_DMG_ENERGYBEAM:
            case AETHER_DMG_RADIATION:
                scaled *= 0.8f;
                break;
            default: break;
        }
    }

    aether_log(AETHER_LOG_DEBUG, "damage",
               "applying %.1f (type=0x%X) -> scaled=%.1f",
               ev->amount, (unsigned)ev->type, scaled);

    aether_player_health_damage(h, scaled);
}

f32 aether_player_calc_fall_damage(f32 velocity_z) {
    /* GoldSrc: no damage below 580 u/s landing speed */
    f32 speed = fabsf(velocity_z);
    if (speed <= 580.0f) return 0.0f;

    f32 over = speed - 580.0f;
    f32 dmg = over * 0.1f;  /* rough: 10 dmg per 100 over threshold */

    /* Cap at 200 */
    if (dmg > 200.0f) dmg = 200.0f;
    return dmg;
}

void aether_player_tick_drown(aether_player_health_t *h, f32 dt, bool drowning) {
    if (!h || !drowning || h->dead || dt <= 0.0f) return;
    /* 10 hp/s while drowning (air depleted + eye underwater). */
    aether_player_health_damage(h, 10.0f * dt);
}

void aether_player_tick_fire(aether_player_health_t *h, f32 dt, bool on_fire) {
    if (!h || !on_fire || h->dead) return;
    /* 20 hp per second */
    aether_player_health_damage(h, 20.0f * dt);
}

void aether_player_tick_radiation(aether_player_health_t *h, f32 dt, bool in_rad) {
    if (!h || !in_rad || h->dead) return;
    /* 5 hp per second */
    aether_player_health_damage(h, 5.0f * dt);
}

void aether_player_force_lethal_for_auth(aether_player_health_t *h) {
    if (!h) return;
    h->health = 1.f;
    h->armor = 0.f;
    h->dead = false;
}

void aether_player_apply_damage_auth(aether_player_health_t *h,
                                     const aether_damage_event_t *ev,
                                     aether_damage_net_server_t *server,
                                     u32 killer_id, u32 victim_id,
                                     bool fanout_scores,
                                     aether_damage_kill_result_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!h || !ev) return;
    bool was_dead = h->dead;
    aether_player_apply_damage(h, ev);
    if (out) {
        out->applied = true;
        out->killer_id = killer_id;
        out->victim_id = victim_id;
    }
    if (was_dead || !h->dead) return;
    if (out) out->died = true;
    if (!server || (killer_id == 0 && victim_id == 0)) return;
    aether_net_server_t *s = (aether_net_server_t *)server;
    if (aether_net_server_register_kill(s, killer_id, victim_id)) {
        if (out) out->registered_kill = true;
        if (fanout_scores) aether_net_server_fanout_scores(s);
    }
}
