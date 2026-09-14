#ifndef AETHER_PARTICLE_H
#define AETHER_PARTICLE_H
#include "../core/AetherCore.h"
typedef struct aether_particle { f32 position[3]; f32 velocity[3]; f32 size; f32 life; f32 age; f32 color[4]; bool active; } aether_particle_t;
#define AETHER_MAX_PARTICLES 2048
typedef struct aether_particles { aether_particle_t items[AETHER_MAX_PARTICLES]; u32 count; } aether_particles_t;
aether_result_t aether_particles_init(aether_particles_t *p);
aether_result_t aether_particles_spawn(aether_particles_t *p, const f32 pos[3], const f32 vel[3], f32 size, f32 life, const f32 color[4]);
void aether_particles_update(aether_particles_t *p, f32 dt);
void aether_particles_clear(aether_particles_t *p);
#endif
