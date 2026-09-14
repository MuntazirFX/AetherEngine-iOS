#ifndef AETHER_PHYSICS_H
#define AETHER_PHYSICS_H
#include "../core/AetherCore.h"
#include "../core/AetherMath.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { aether_vec3_t mins,maxs,velocity; f32 gravity,friction,mass; bool on_ground; } aether_physics_body_t;
typedef bool (*aether_physics_sweep_fn)(void *user,aether_vec3_t from,aether_vec3_t to,aether_vec3_t mins,aether_vec3_t maxs,aether_vec3_t *hit_pos,aether_vec3_t *normal);
void aether_physics_body_init(aether_physics_body_t*b,aether_vec3_t mins,aether_vec3_t maxs);
void aether_physics_step(aether_physics_body_t*b,aether_vec3_t *position,f32 dt,aether_physics_sweep_fn sweep,void *user);
void aether_physics_apply_impulse(aether_physics_body_t*b,aether_vec3_t impulse);
#ifdef __cplusplus
}
#endif
#endif
