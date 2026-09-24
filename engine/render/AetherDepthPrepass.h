/* AetherDepthPrepass.h — Metal depth-prepass encode plan + CPU depth record stub.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_DEPTH_PREPASS_H
#define AETHER_DEPTH_PREPASS_H

#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_depth_prepass {
    bool enabled;
    u32  width;
    u32  height;
    f32  clear_depth;   /* typically 1.0 */
    bool write_depth;
    bool recorded;      /* set when record_stub wrote samples */
    u32  sample_count;
} aether_depth_prepass_t;

typedef struct aether_depth_prepass_plan {
    u32 pass_count;     /* 1 when needed, else 0 */
    u32 width;
    u32 height;
    f32 clear_depth;
    bool write_depth;
    bool needed;
} aether_depth_prepass_plan_t;

aether_result_t aether_depth_prepass_init(aether_depth_prepass_t *d);
void aether_depth_prepass_shutdown(aether_depth_prepass_t *d);
void aether_depth_prepass_set_enabled(aether_depth_prepass_t *d, bool enabled);
aether_result_t aether_depth_prepass_ensure(aether_depth_prepass_t *d, u32 w, u32 h);

void aether_depth_prepass_encode_plan(const aether_depth_prepass_t *d,
                                      aether_depth_prepass_plan_t *out);

/* CPU stub: record linearized depth for N positions into out_depths[].
 * positions = xyz * count; near/far for linearize. Returns samples written.
 * Marks d->recorded when count > 0. */
u32 aether_depth_prepass_record_stub(aether_depth_prepass_t *d,
                                     const f32 *positions_xyz, u32 count,
                                     f32 near_z, f32 far_z,
                                     f32 *out_depths, u32 max_out);

/* Linearize clip-space Z (0..1) with reverse-Z optional (rev=false → standard). */
f32 aether_depth_prepass_linearize(f32 depth01, f32 near_z, f32 far_z, bool reverse_z);

bool aether_depth_prepass_encode_needed(const aether_depth_prepass_t *d);

/* Frame-order: depth prepass must bind before the main color pass. */
typedef struct aether_depth_prepass_frame {
    bool bind_before_main; /* true when encoder should run depth first */
    bool bound;            /* set after host/Metal records the bind */
    u32  main_pass_index;  /* expected main color pass index (usually 1) */
    u32  prepass_index;    /* 0 when bound first */
} aether_depth_prepass_frame_t;

void aether_depth_prepass_frame_init(aether_depth_prepass_frame_t *f);
/* Plan bind-before-main when encode is needed. Returns 1 if prepass should run first. */
int  aether_depth_prepass_bind_before_main(const aether_depth_prepass_t *d,
                                           aether_depth_prepass_frame_t *f);
/* Mark that Metal/host bound the depth prepass (before main). */
void aether_depth_prepass_mark_bound(aether_depth_prepass_frame_t *f);
bool aether_depth_prepass_was_bound_before_main(const aether_depth_prepass_frame_t *f);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_DEPTH_PREPASS_H */
