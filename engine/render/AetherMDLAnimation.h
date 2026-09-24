/* AetherMDLAnimation.h — Sequence clock + bone/skinning matrix stub.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_MDL_ANIMATION_H
#define AETHER_MDL_ANIMATION_H
#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AETHER_MDL_MAX_BONES 32

typedef struct aether_mdl_animation {
    u32 sequence;
    u32 bone_count;
    f32 time;
    f32 fps;
    f32 frame;
    bool loop;
    bool playing;
} aether_mdl_animation_t;

/* Column-major 4x4 bone matrix (16 floats). */
typedef struct aether_mdl_bone_matrix {
    f32 m[16];
} aether_mdl_bone_matrix_t;

typedef struct aether_mdl_skin_state {
    aether_mdl_bone_matrix_t bones[AETHER_MDL_MAX_BONES];
    u32 bone_count;
    f32 time;
} aether_mdl_skin_state_t;

aether_result_t aether_mdl_animation_init(aether_mdl_animation_t *a, u32 bone_count);
void aether_mdl_animation_play(aether_mdl_animation_t *a, u32 sequence, f32 fps, bool loop);
void aether_mdl_animation_update(aether_mdl_animation_t *a, f32 dt, u32 frame_count);
void aether_mdl_animation_stop(aether_mdl_animation_t *a);

/* Identity skin matrices for bone_count (cap AETHER_MDL_MAX_BONES). */
void aether_mdl_skin_identity(aether_mdl_skin_state_t *sk, u32 bone_count);

/* Simple stub: root bone orbits / sways with time; children inherit identity offset.
 * Useful for Metal skinning smoke without real sequence data. */
void aether_mdl_skin_build_stub(aether_mdl_skin_state_t *sk, u32 bone_count, f32 time,
                                f32 sway_deg);

/* Pack bone matrices into flat float array (16 * bone_count). Returns floats written. */
u32 aether_mdl_skin_fill_ubo(const aether_mdl_skin_state_t *sk, f32 *out, u32 max_floats);

/* CPU skin one position by bone index + weight (single-bone stub). */
void aether_mdl_skin_transform_point(const aether_mdl_skin_state_t *sk, u32 bone,
                                     f32 weight, const f32 in[3], f32 out[3]);

#ifdef __cplusplus
}
#endif
#endif
