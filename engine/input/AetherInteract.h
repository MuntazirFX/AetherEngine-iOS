/* AetherInteract.h — Use/interact eye-ray against world + entity AABBs.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_INTERACT_H
#define AETHER_INTERACT_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum aether_interact_hit_kind {
    AETHER_INTERACT_NONE = 0,
    AETHER_INTERACT_WORLD,
    AETHER_INTERACT_ENTITY
} aether_interact_hit_kind_t;

typedef struct aether_interact_hit {
    aether_interact_hit_kind_t kind;
    f32 point[3];
    f32 normal[3];
    f32 distance;
    i32 entity_id; /* -1 if world/none */
    char classname[64];
} aether_interact_hit_t;

typedef struct aether_interact_target {
    i32  id;
    char classname[64];
    f32  mins[3];
    f32  maxs[3];
    bool usable;
} aether_interact_target_t;

/* Ray vs AABB. Returns true on hit; writes t_enter along ray (0..1 of max_dist). */
bool aether_interact_ray_aabb(const f32 origin[3], const f32 dir[3], f32 max_dist,
                              const f32 mins[3], const f32 maxs[3],
                              f32 *out_t, f32 out_point[3]);

/* Trace eye ray against optional ground plane (z=ground_z) + target list.
 * Picks nearest usable hit within max_dist. */
void aether_interact_trace(const f32 eye[3], const f32 forward[3], f32 max_dist,
                           f32 ground_z,
                           const aether_interact_target_t *targets, u32 target_count,
                           aether_interact_hit_t *out_hit);

/* Convenience: build look forward from yaw/pitch (degrees, Z-up). */
void aether_interact_forward_from_view(f32 yaw_deg, f32 pitch_deg, f32 out_fwd[3]);

#ifdef __cplusplus
}
#endif
#endif
