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
#define AETHER_MDL_MAX_SEQ_FRAMES 16

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

typedef struct aether_mdl_seq_bone_key {
    f32 pos[3];
    f32 angles_deg[3]; /* pitch/yaw/roll stub; yaw used for Z-up rotate */
} aether_mdl_seq_bone_key_t;

typedef struct aether_mdl_sequence {
    u32 bone_count;
    u32 frame_count;
    f32 fps;
    bool loop;
    aether_mdl_seq_bone_key_t keys[AETHER_MDL_MAX_SEQ_FRAMES][AETHER_MDL_MAX_BONES];
} aether_mdl_sequence_t;

aether_result_t aether_mdl_animation_init(aether_mdl_animation_t *a, u32 bone_count);
void aether_mdl_animation_play(aether_mdl_animation_t *a, u32 sequence, f32 fps, bool loop);
void aether_mdl_animation_update(aether_mdl_animation_t *a, f32 dt, u32 frame_count);
void aether_mdl_animation_stop(aether_mdl_animation_t *a);

void aether_mdl_skin_identity(aether_mdl_skin_state_t *sk, u32 bone_count);
void aether_mdl_skin_build_stub(aether_mdl_skin_state_t *sk, u32 bone_count, f32 time,
                                f32 sway_deg);
u32 aether_mdl_skin_fill_ubo(const aether_mdl_skin_state_t *sk, f32 *out, u32 max_floats);
void aether_mdl_skin_transform_point(const aether_mdl_skin_state_t *sk, u32 bone,
                                     f32 weight, const f32 in[3], f32 out[3]);

/* Sequence frame → bone mats → skinned verts. */
void aether_mdl_sequence_init_sway(aether_mdl_sequence_t *seq, u32 bone_count, u32 frames, f32 fps);
void aether_mdl_skin_build_from_sequence(aether_mdl_skin_state_t *sk,
                                         const aether_mdl_sequence_t *seq,
                                         f32 frame);
void aether_mdl_skin_transform_point2(const aether_mdl_skin_state_t *sk,
                                      u32 bone0, f32 w0, u32 bone1, f32 w1,
                                      const f32 in[3], f32 out[3]);
u32 aether_mdl_skin_mesh(const aether_mdl_skin_state_t *sk,
                         const u8 *bone_indices, const f32 *weights,
                         const f32 *in_xyz, f32 *out_xyz, u32 vert_count);

#ifdef __cplusplus
}
#endif

/* Load sequence keys from studio fixture bytes (magic 0xAE7E5E02). Returns AETHER_OK. */
aether_result_t aether_mdl_sequence_load_from_data(aether_mdl_sequence_t *seq,
                                                   const u8 *data, u32 size);

/* GoldSrc-ish anim RLE decode into sequence keys (magic 0xAE7E524C).
 * Falls back to aether_mdl_sequence_load_from_data if no RLE block. */
aether_result_t aether_mdl_anim_rle_decode(aether_mdl_sequence_t *seq,
                                           const u8 *data, u32 size);

#endif
