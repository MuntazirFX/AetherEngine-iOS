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

/* ---------- Camera matrices for depth prepass (not identity stub) ---------- */
typedef struct aether_depth_prepass_camera {
    f32 view[16];
    f32 proj[16];
    f32 mvp[16];       /* proj * view */
    f32 eye[3];
    bool valid;
} aether_depth_prepass_camera_t;

void aether_depth_prepass_camera_init(aether_depth_prepass_camera_t *c);
void aether_depth_prepass_camera_set(aether_depth_prepass_camera_t *c,
                                     const f32 view[16], const f32 proj[16],
                                     const f32 eye[3]);
void aether_depth_prepass_camera_fill_mvp(const aether_depth_prepass_camera_t *c,
                                          f32 out_mvp[16]);
bool aether_depth_prepass_camera_valid(const aether_depth_prepass_camera_t *c);

/* Encode plan that also carries MVP when camera is valid. */
typedef struct aether_depth_prepass_plan_ex {
    aether_depth_prepass_plan_t base;
    f32 mvp[16];
    bool has_mvp;
} aether_depth_prepass_plan_ex_t;

void aether_depth_prepass_encode_plan_ex(const aether_depth_prepass_t *d,
                                         const aether_depth_prepass_camera_t *cam,
                                         aether_depth_prepass_plan_ex_t *out);


/* ---------- Depth prepass → Hi-Z pyramid bind plan (encode order + texture views) ---------- */
#define AETHER_DEPTH_HIZ_MAX_MIP_VIEWS 8

typedef struct aether_depth_hiz_tex_view {
    u32 mip;
    u32 width;
    u32 height;
    u32 texel_offset;   /* into pyramid depth[] */
    bool valid;
} aether_depth_hiz_tex_view_t;

typedef struct aether_depth_hiz_bind_plan {
    bool depth_first;          /* encode depth prepass before Hi-Z fill */
    bool fill_mip0_from_depth; /* copy linearized depth → pyramid mip0 */
    bool build_pyramid;        /* downsample mips after fill */
    bool texture_views;        /* expose per-mip views for Metal */
    bool needed;
    bool bound;                /* host/Metal marked bind complete */
    u32  encode_steps;         /* 0..4 */
    u32  mip_view_count;
    u32  mip0_w, mip0_h;
    aether_depth_hiz_tex_view_t views[AETHER_DEPTH_HIZ_MAX_MIP_VIEWS];
} aether_depth_hiz_bind_plan_t;

void aether_depth_hiz_bind_plan_init(aether_depth_hiz_bind_plan_t *plan);
/* Plan encode order when depth prepass is needed: depth → fill → build → views. */
int  aether_depth_hiz_bind_plan_encode(const aether_depth_prepass_t *d,
                                       u32 mip0_w, u32 mip0_h,
                                       aether_depth_hiz_bind_plan_t *out);
void aether_depth_hiz_bind_plan_mark_bound(aether_depth_hiz_bind_plan_t *plan);
bool aether_depth_hiz_bind_plan_was_bound(const aether_depth_hiz_bind_plan_t *plan);
/* Fill texture-view descriptors for levels (no GPU alloc on host). */
u32  aether_depth_hiz_bind_plan_fill_views(aether_depth_hiz_bind_plan_t *plan,
                                           const u32 *level_w, const u32 *level_h,
                                           const u32 *level_off, u32 levels);

#endif /* AETHER_DEPTH_PREPASS_H */
