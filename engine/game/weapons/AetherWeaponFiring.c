/* AetherWeaponFiring.c — Firing logic implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherWeaponFiring.h"
#include "../../player/AetherPlayerDamage.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Simple xorshift RNG (deterministic if seeded) */
static u32 rng_state = 0x12345678;

static u32 rng_next(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static f32 rng_float(void) {
    return (f32)(rng_next() & 0xFFFFFF) / (f32)0x1000000;
}

static f32 rng_range(f32 a, f32 b) {
    return a + (b - a) * rng_float();
}

void aether_weapon_apply_spread(aether_vec3_t view_dir,
                                 f32 spread,
                                 aether_vec3_t *dir_out) {
    if (!dir_out) return;
    if (spread <= 0.0f) { *dir_out = view_dir; return; }

    /* Add small random perturbation */
    aether_vec3_t p;
    p.x = view_dir.x + rng_range(-spread, spread);
    p.y = view_dir.y + rng_range(-spread, spread);
    p.z = view_dir.z + rng_range(-spread, spread);

    *dir_out = aether_vec3_normalize(p);
}

u32 aether_weapon_fire_hitscan(const aether_weapon_def_t *def,
                                aether_vec3_t origin,
                                aether_vec3_t view_dir,
                                aether_hitscan_result_t *results,
                                u32 max_results) {
    if (!def || !results) return 0;

    u32 pellets = (u32)(def->pellets > 0 ? def->pellets : 1);
    if (pellets > max_results) pellets = max_results;

    view_dir = aether_vec3_normalize(view_dir);

    for (u32 i = 0; i < pellets; ++i) {
        aether_vec3_t dir;
        aether_weapon_apply_spread(view_dir, def->spread, &dir);

        aether_hitscan_result_t *r = &results[i];
        memset(r, 0, sizeof *r);
        r->start = origin;
        r->end = aether_vec3_add(origin, aether_vec3_scale(dir, def->range));
        r->hit = false;   /* real trace against BSP hulls would go here */
        r->damage = (f32)def->damage;
        r->distance = def->range;
        r->hit_point = r->end;
    }

    aether_log(AETHER_LOG_DEBUG, "firing",
               "%s fired %u pellet(s), spread=%.4f, dmg=%d",
               def->display_name, pellets, def->spread, def->damage);

    return pellets;
}

void aether_weapon_apply_recoil(aether_vec3_t *view_angles,
                                 f32 recoil_strength) {
    if (!view_angles || recoil_strength <= 0.0f) return;
    /* Kick pitch up + slight yaw jitter */
    view_angles->x -= recoil_strength;      /* pitch up */
    view_angles->y += rng_range(-recoil_strength * 0.5f, recoil_strength * 0.5f);
}

u32 aether_weapon_fire_combat_auth(aether_weapon_state_t *ws,
                                   aether_player_inventory_t *inv,
                                   f32 now,
                                   aether_vec3_t origin,
                                   aether_vec3_t view_dir,
                                   u32 killer_id, u32 victim_id,
                                   bool force_hit,
                                   aether_weapon_auth_queue_fn queue_fn,
                                   void *queue_user,
                                   aether_weapon_combat_hit_t *out) {
    if (out) memset(out, 0, sizeof(*out));
    if (!ws || !ws->def) return 0;
    if (!aether_weapon_fire(ws, inv, now)) {
        if (out) out->fired = false;
        return 0;
    }
    aether_hitscan_result_t results[16];
    u32 n = aether_weapon_fire_hitscan(ws->def, origin, view_dir, results, 16);
    f32 total = 0.f;
    bool any_hit = force_hit;
    for (u32 i = 0; i < n; ++i) {
        if (force_hit) {
            results[i].hit = true;
            results[i].distance = ws->def->range * 0.25f;
            results[i].hit_point = aether_vec3_add(origin,
                aether_vec3_scale(aether_vec3_normalize(view_dir), results[i].distance));
        }
        if (results[i].hit) {
            any_hit = true;
            total += results[i].damage;
        }
    }
    if (force_hit && total <= 0.f && n > 0) total = results[0].damage;
    if (force_hit && total <= 0.f) total = (f32)ws->def->damage;
    u32 dmg_type = (u32)AETHER_DMG_BULLET;
    if (ws->def->flags & AETHER_WFLAG_EXPLOSIVE) dmg_type = (u32)AETHER_DMG_BLAST;
    bool queued = false;
    if (any_hit && total > 0.f && queue_fn) {
        queue_fn(queue_user, killer_id, victim_id, total, dmg_type);
        queued = true;
    }
    if (out) {
        out->fired = true;
        out->hit = any_hit;
        out->queued = queued;
        out->pellets = n;
        out->killer_id = killer_id;
        out->victim_id = victim_id;
        out->damage = total;
        out->dmg_type = dmg_type;
        if (n > 0) out->primary = results[0];
    }
    return queued ? 1 : 0;
}


f32 aether_weapon_hitgroup_scale(u8 hitgroup) {
    switch (hitgroup) {
        case AETHER_HITGROUP_HEAD:    return 4.f;
        case AETHER_HITGROUP_CHEST:   return 1.f;
        case AETHER_HITGROUP_STOMACH: return 1.25f;
        case AETHER_HITGROUP_ARM:     return 0.75f;
        case AETHER_HITGROUP_LEG:     return 0.75f;
        default:                      return 1.f;
    }
}

u32 aether_weapon_fire_combat_auth_hitgroup(aether_weapon_state_t *ws,
                                            aether_player_inventory_t *inv,
                                            f32 now,
                                            aether_vec3_t origin,
                                            aether_vec3_t view_dir,
                                            u32 killer_id, u32 victim_id,
                                            bool force_hit,
                                            u8 hitgroup,
                                            aether_weapon_auth_queue_fn queue_fn,
                                            void *queue_user,
                                            aether_weapon_combat_hit_t *out) {
    aether_weapon_combat_hit_t hit;
    u32 q = aether_weapon_fire_combat_auth(ws, inv, now, origin, view_dir,
                                           killer_id, victim_id, force_hit,
                                           NULL, NULL, &hit);
    f32 scale = aether_weapon_hitgroup_scale(hitgroup);
    f32 scaled = hit.damage * scale;
    bool queued = false;
    if (hit.hit && scaled > 0.f && queue_fn) {
        /* Re-queue with scaled damage (base path queued with NULL above). */
        u32 dmg_type = hit.dmg_type;
        if (hitgroup == AETHER_HITGROUP_HEAD)
            dmg_type |= (u32)AETHER_DMG_BULLET; /* keep bullet; headshot flagged in out */
        queue_fn(queue_user, killer_id, victim_id, scaled, dmg_type);
        queued = true;
        q = 1;
    } else if (q && queue_fn == NULL) {
        /* Should not happen — if base queued without fn, nothing to do. */
    }
    /* If force path already queued via non-null fn in base — we passed NULL.
     * Always queue here when hit. */
    if (!queued && hit.fired && hit.hit && scaled > 0.f && queue_fn) {
        queue_fn(queue_user, killer_id, victim_id, scaled, hit.dmg_type);
        queued = true;
        q = 1;
    }
    if (out) {
        *out = hit;
        out->damage = scaled;
        out->damage_scale = scale;
        out->hitgroup = hitgroup;
        out->headshot = (hitgroup == AETHER_HITGROUP_HEAD);
        out->queued = queued;
    }
    return queued ? 1 : 0;
}
