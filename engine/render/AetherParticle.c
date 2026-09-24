#include "AetherParticle.h"
#include <math.h>
#include <string.h>

aether_result_t aether_particles_init(aether_particles_t *p) {
    if (!p) return AETHER_ERR_INVALID_ARG;
    memset(p, 0, sizeof(*p));
    return AETHER_OK;
}

aether_result_t aether_particles_spawn(aether_particles_t *p,
                                       const f32 pos[3],
                                       const f32 vel[3],
                                       f32 size,
                                       f32 life,
                                       const f32 color[4]) {
    if (!p || !pos || !vel || !color || size <= 0.0f || life <= 0.0f)
        return AETHER_ERR_INVALID_ARG;
    u32 slot = p->count < AETHER_MAX_PARTICLES
                   ? p->count
                   : (p->count % AETHER_MAX_PARTICLES);
    aether_particle_t *x = &p->items[slot];
    memcpy(x->position, pos, sizeof(x->position));
    memcpy(x->velocity, vel, sizeof(x->velocity));
    memcpy(x->color, color, sizeof(x->color));
    x->size = size;
    x->life = life;
    x->age = 0.0f;
    x->active = true;
    if (p->count < AETHER_MAX_PARTICLES) p->count++;
    return AETHER_OK;
}

u32 aether_particles_spawn_burst(aether_particles_t *p,
                                 const f32 origin[3],
                                 u32 count) {
    if (!p || !origin || count == 0) return 0;
    if (count > 256) count = 256;
    u32 spawned = 0;
    static const f32 palette[][4] = {
        {1.00f, 0.85f, 0.35f, 1.0f},
        {0.45f, 0.75f, 1.00f, 1.0f},
        {1.00f, 0.45f, 0.35f, 1.0f},
        {0.55f, 1.00f, 0.55f, 1.0f},
    };
    for (u32 i = 0; i < count; ++i) {
        f32 t = (f32)i;
        f32 ang = t * 0.6180339887f * 6.28318530718f;
        f32 elev = ((f32)(i % 7) - 3.0f) * 0.18f;
        f32 speed = 40.0f + (f32)(i % 11) * 8.0f;
        f32 pos[3] = {
            origin[0] + cosf(ang) * 4.0f,
            origin[1] + sinf(ang) * 4.0f,
            origin[2] + elev * 6.0f
        };
        f32 vel[3] = {
            cosf(ang) * speed,
            sinf(ang) * speed,
            35.0f + elev * 25.0f
        };
        f32 size = 6.0f + (f32)(i % 5) * 2.0f;
        f32 life = 1.2f + (f32)(i % 9) * 0.15f;
        const f32 *col = palette[i % 4];
        if (aether_particles_spawn(p, pos, vel, size, life, col) == AETHER_OK)
            spawned++;
    }
    return spawned;
}

void aether_particles_update(aether_particles_t *p, f32 dt) {
    if (!p || dt <= 0.0f) return;
    const f32 gravity = -120.0f;
    for (u32 i = 0; i < p->count; i++) {
        aether_particle_t *x = &p->items[i];
        if (!x->active) continue;
        x->velocity[2] += gravity * dt;
        for (int k = 0; k < 3; k++)
            x->position[k] += x->velocity[k] * dt;
        x->age += dt;
        if (x->age >= x->life) x->active = false;
    }
}

void aether_particles_clear(aether_particles_t *p) {
    if (p) memset(p, 0, sizeof(*p));
}

u32 aether_particles_active_count(const aether_particles_t *p) {
    if (!p) return 0;
    u32 n = 0;
    for (u32 i = 0; i < p->count; ++i)
        if (p->items[i].active) n++;
    return n;
}

u32 aether_particles_copy_render(const aether_particles_t *p,
                                 aether_particle_vertex_t *out,
                                 u32 max_out) {
    if (!p || !out || max_out == 0) return 0;
    u32 written = 0;
    for (u32 i = 0; i < p->count && written < max_out; ++i) {
        const aether_particle_t *x = &p->items[i];
        if (!x->active) continue;
        f32 life_left = (x->life > 0.0f) ? (1.0f - x->age / x->life) : 0.0f;
        if (life_left < 0.0f) life_left = 0.0f;
        if (life_left > 1.0f) life_left = 1.0f;
        aether_particle_vertex_t *v = &out[written++];
        v->x = x->position[0];
        v->y = x->position[1];
        v->z = x->position[2];
        v->size = x->size * (0.35f + 0.65f * life_left);
        v->r = x->color[0];
        v->g = x->color[1];
        v->b = x->color[2];
        v->a = x->color[3] * life_left;
    }
    return written;
}

u32 aether_particles_spawn_muzzle(aether_particles_t *p,
                                  const f32 origin[3], const f32 forward[3],
                                  u32 count) {
    if (!p || !origin || !forward || count == 0) return 0;
    if (count > 64) count = 64;
    f32 fx = forward[0], fy = forward[1], fz = forward[2];
    f32 len = sqrtf(fx*fx + fy*fy + fz*fz);
    if (len < 1e-5f) { fx = 1.f; fy = 0.f; fz = 0.f; }
    else { fx /= len; fy /= len; fz /= len; }
    u32 spawned = 0;
    static const f32 col[4] = {1.f, 0.85f, 0.35f, 1.f};
    for (u32 i = 0; i < count; ++i) {
        f32 jitter = ((f32)(i % 5) - 2.f) * 8.f;
        f32 pos[3] = {
            origin[0] + fx * 4.f,
            origin[1] + fy * 4.f,
            origin[2] + fz * 4.f
        };
        f32 vel[3] = {
            fx * (120.f + jitter) + ((f32)(i % 3) - 1.f) * 20.f,
            fy * (120.f + jitter) + ((f32)((i+1) % 3) - 1.f) * 20.f,
            fz * (120.f + jitter) + 30.f
        };
        f32 size = 4.f + (f32)(i % 4);
        f32 life = 0.12f + (f32)(i % 5) * 0.03f;
        if (aether_particles_spawn(p, pos, vel, size, life, col) == AETHER_OK)
            spawned++;
    }
    return spawned;
}

u32 aether_particles_spawn_trail(aether_particles_t *p,
                                 const f32 from[3], const f32 to[3],
                                 u32 count) {
    if (!p || !from || !to || count == 0) return 0;
    if (count > 64) count = 64;
    u32 spawned = 0;
    static const f32 col[4] = {1.f, 0.7f, 0.2f, 0.9f};
    for (u32 i = 0; i < count; ++i) {
        f32 t = (count == 1) ? 0.f : (f32)i / (f32)(count - 1);
        f32 pos[3] = {
            from[0] + (to[0] - from[0]) * t,
            from[1] + (to[1] - from[1]) * t,
            from[2] + (to[2] - from[2]) * t
        };
        f32 vel[3] = {0.f, 0.f, 10.f + (f32)(i % 3) * 5.f};
        f32 size = 2.5f + (1.f - t) * 2.f;
        f32 life = 0.2f + (1.f - t) * 0.25f;
        if (aether_particles_spawn(p, pos, vel, size, life, col) == AETHER_OK)
            spawned++;
    }
    return spawned;
}

u32 aether_particles_spawn_at_attachment(aether_particles_t *p,
                                         const f32 origin[3], const f32 forward[3],
                                         u32 count) {
    return aether_particles_spawn_muzzle(p, origin, forward, count);
}

u32 aether_particles_spawn_viewmodel_fire(aether_particles_t *p,
                                          const f32 muzzle[3], const f32 forward[3],
                                          u32 muzzle_count, u32 trail_count) {
    if (!p || !muzzle || !forward) return 0;
    u32 n = aether_particles_spawn_muzzle(p, muzzle, forward, muzzle_count);
    f32 to[3] = {
        muzzle[0] + forward[0] * 40.f,
        muzzle[1] + forward[1] * 40.f,
        muzzle[2] + forward[2] * 40.f
    };
    n += aether_particles_spawn_trail(p, muzzle, to, trail_count);
    return n;
}
