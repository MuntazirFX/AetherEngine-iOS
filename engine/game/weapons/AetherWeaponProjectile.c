/* AetherWeaponProjectile.c — Projectile manager implementation.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherWeaponProjectile.h"
#include <string.h>
#include <math.h>

void aether_projectile_mgr_init(aether_projectile_mgr_t *m) {
    if (!m) return;
    memset(m, 0, sizeof *m);
    aether_log(AETHER_LOG_INFO, "proj", "projectile manager initialized");
}

static void projectile_kind_defaults(aether_projectile_kind_t k,
                                      f32 *gravity, f32 *damage,
                                      f32 *radius, f32 *explode_delay) {
    switch (k) {
        case AETHER_PROJ_ROCKET:  *gravity = 0.0f;   *damage = 100.0f; *radius = 200.0f; *explode_delay = 10.0f; break;
        case AETHER_PROJ_GRENADE: *gravity = 800.0f; *damage = 100.0f; *radius = 200.0f; *explode_delay = 2.5f;  break;
        case AETHER_PROJ_BOLT:    *gravity = 0.0f;   *damage = 100.0f; *radius = 0.0f;   *explode_delay = 10.0f; break;
        case AETHER_PROJ_SNARK:   *gravity = 800.0f; *damage = 20.0f;  *radius = 100.0f; *explode_delay = 15.0f; break;
        case AETHER_PROJ_SATCHEL: *gravity = 800.0f; *damage = 150.0f; *radius = 250.0f; *explode_delay = 3.0f;  break;
        case AETHER_PROJ_TRIPMINE:*gravity = 800.0f; *damage = 150.0f; *radius = 150.0f; *explode_delay = 30.0f; break;
        default:                  *gravity = 0.0f;   *damage = 0.0f;   *radius = 0.0f;   *explode_delay = 0.0f;  break;
    }
}

i32 aether_projectile_spawn(aether_projectile_mgr_t *m,
                              aether_projectile_kind_t kind,
                              aether_vec3_t origin,
                              aether_vec3_t velocity,
                              aether_entity_t *owner) {
    if (!m) return -1;
    if (m->count >= AETHER_PROJECTILE_MAX) {
        aether_log(AETHER_LOG_WARN, "proj", "max projectiles reached");
        return -1;
    }

    aether_projectile_t *p = &m->items[m->count];
    memset(p, 0, sizeof *p);
    p->id = m->count + 1;
    p->kind = kind;
    p->origin = origin;
    p->velocity = velocity;
    p->owner = owner;
    p->active = true;

    projectile_kind_defaults(kind, &p->gravity, &p->damage, &p->radius, &p->explode_time);
    p->lifetime = 0.0f;

    aether_log(AETHER_LOG_INFO, "proj",
               "spawned kind=%d id=%u at (%.0f,%.0f,%.0f) dmg=%.0f",
               (int)kind, p->id, origin.x, origin.y, origin.z, p->damage);

    m->count++;
    return (i32)p->id;
}

void aether_projectile_mgr_tick(aether_projectile_mgr_t *m, f32 dt) {
    if (!m) return;

    for (u32 i = 0; i < m->count; ++i) {
        aether_projectile_t *p = &m->items[i];
        if (!p->active) continue;

        /* Gravity */
        p->velocity.z -= p->gravity * dt;

        /* Integrate */
        p->origin.x += p->velocity.x * dt;
        p->origin.y += p->velocity.y * dt;
        p->origin.z += p->velocity.z * dt;

        p->lifetime += dt;

        /* Explode on timeout */
        if (p->explode_time > 0.0f && p->lifetime >= p->explode_time) {
            aether_log(AETHER_LOG_INFO, "proj",
                       "id=%u exploded (lifetime=%.1fs, dmg=%.0f)",
                       p->id, p->lifetime, p->damage);
            p->active = false;
        }
    }

    /* Compact inactive */
    u32 write = 0;
    for (u32 i = 0; i < m->count; ++i) {
        if (!m->items[i].active) continue;
        if (write != i) m->items[write] = m->items[i];
        write++;
    }
    m->count = write;
}

void aether_projectile_mgr_dump(const aether_projectile_mgr_t *m) {
    if (!m) return;
    aether_log(AETHER_LOG_INFO, "proj", "===== PROJECTILES: %u active =====", m->count);
    for (u32 i = 0; i < m->count; ++i) {
        const aether_projectile_t *p = &m->items[i];
        aether_log(AETHER_LOG_INFO, "proj",
                   "  id=%u kind=%d  pos=(%.0f,%.0f,%.0f)  vel=(%.0f,%.0f,%.0f)",
                   p->id, (int)p->kind, p->origin.x, p->origin.y, p->origin.z,
                   p->velocity.x, p->velocity.y, p->velocity.z);
    }
}
