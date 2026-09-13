/* AetherWeaponFiring.c — Firing logic implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherWeaponFiring.h"
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
