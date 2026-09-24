#ifndef AETHER_PARTICLE_H
#define AETHER_PARTICLE_H
#include "../core/AetherCore.h"

typedef struct aether_particle {
    f32 position[3];
    f32 velocity[3];
    f32 size;
    f32 life;
    f32 age;
    f32 color[4];
    bool active;
} aether_particle_t;

#define AETHER_MAX_PARTICLES 2048

/* GPU/bridge-friendly snapshot: 8 floats (32 bytes). Alpha already faded by remaining life. */
typedef struct aether_particle_vertex {
    f32 x, y, z;
    f32 size;
    f32 r, g, b, a;
} aether_particle_vertex_t;

typedef struct aether_particles {
    aether_particle_t items[AETHER_MAX_PARTICLES];
    u32 count;
} aether_particles_t;

aether_result_t aether_particles_init(aether_particles_t *p);
aether_result_t aether_particles_spawn(aether_particles_t *p,
                                       const f32 pos[3],
                                       const f32 vel[3],
                                       f32 size,
                                       f32 life,
                                       const f32 color[4]);
/* Seed a small burst around origin (deterministic-ish from slot index). */
u32 aether_particles_spawn_burst(aether_particles_t *p,
                                 const f32 origin[3],
                                 u32 count);
void aether_particles_update(aether_particles_t *p, f32 dt);
void aether_particles_clear(aether_particles_t *p);
u32 aether_particles_active_count(const aether_particles_t *p);
/* Copy up to max_out active particles into out[]; returns number written. */
u32 aether_particles_copy_render(const aether_particles_t *p,
                                 aether_particle_vertex_t *out,
                                 u32 max_out);

/* Weapon-linked stubs: muzzle flash burst + tracer trail between two points. */
u32 aether_particles_spawn_muzzle(aether_particles_t *p,
                                  const f32 origin[3], const f32 forward[3],
                                  u32 count);
u32 aether_particles_spawn_trail(aether_particles_t *p,
                                 const f32 from[3], const f32 to[3],
                                 u32 count);

/* Spawn muzzle FX at an attachment point (view/world). */
u32 aether_particles_spawn_at_attachment(aether_particles_t *p,
                                         const f32 origin[3], const f32 forward[3],
                                         u32 count);

/* Viewmodel fire helper: muzzle burst + short trail along forward. */
u32 aether_particles_spawn_viewmodel_fire(aether_particles_t *p,
                                          const f32 muzzle[3], const f32 forward[3],
                                          u32 muzzle_count, u32 trail_count);

#endif
