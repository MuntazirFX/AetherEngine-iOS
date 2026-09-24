#ifndef AETHER_WATER_H
#define AETHER_WATER_H
#include "../core/AetherCore.h"

/* GPU/bridge-friendly vertex: pos.xyz + uv.xy + color.rgba (9 floats, 36 bytes). */
typedef struct aether_water_vertex {
    f32 x, y, z;
    f32 u, v;
    f32 r, g, b, a;
} aether_water_vertex_t;

typedef struct aether_water {
    f32 wave_time;
    f32 wave_speed;
    f32 wave_amp;
    f32 wave_freq;
    f32 opacity;
    f32 size;       /* half-extent of the plane on X/Y */
    f32 height;     /* Z of the undisturbed surface */
    f32 origin[2];  /* XY center of the plane */
    f32 color[4];   /* base tint RGBA */
    bool enabled;
} aether_water_t;

aether_result_t aether_water_init(aether_water_t *w);
void aether_water_shutdown(aether_water_t *w);
void aether_water_update(aether_water_t *w, f32 dt);
void aether_water_set_enabled(aether_water_t *w, bool enabled);
aether_result_t aether_water_set_color(aether_water_t *w, const f32 rgba[4]);
aether_result_t aether_water_set_size(aether_water_t *w, f32 size);
aether_result_t aether_water_set_height(aether_water_t *w, f32 height);
aether_result_t aether_water_set_origin(aether_water_t *w, f32 x, f32 y);
aether_result_t aether_water_set_wave(aether_water_t *w, f32 speed, f32 amp, f32 freq);

/* Copy a tessellated wavy plane into out[]. Returns vertex count written
 * (always a multiple of 3). max_out is vertex capacity. */
u32 aether_water_copy_render(const aether_water_t *w,
                             aether_water_vertex_t *out,
                             u32 max_out);
/* How many vertices the default water grid needs. */
u32 aether_water_render_vertex_count(void);


/* Planar reflection stub: mirror matrix + clip plane for Metal encode. */
typedef struct aether_water_reflect {
    f32 mirror[16];      /* column-major 4x4: reflect about water plane */
    f32 clip_plane[4];   /* ax + by + cz + d = 0 (world space) */
    f32 plane_origin[3];
    f32 plane_normal[3]; /* typically 0,0,1 (Z-up) */
    f32 eye_reflected[3];
    bool enabled;
} aether_water_reflect_t;

/* Uniforms Metal/host can upload (mirror 16 + clip 4 + flags). */
typedef struct aether_water_reflect_uniforms {
    f32 mirror[16];
    f32 clip_plane[4];
    f32 enabled;     /* 1 when reflection pass should run */
    f32 pad[3];
} aether_water_reflect_uniforms_t;

/* Build reflection about water height plane (normal +Z). eye = camera world pos. */
void aether_water_reflect_compute(const aether_water_t *w, const f32 eye[3],
                                  aether_water_reflect_t *out);

/* Fill GPU/host uniform block. */
void aether_water_reflect_fill_uniforms(const aether_water_reflect_t *r,
                                        aether_water_reflect_uniforms_t *out);

/* Reflect a point across the water plane (CPU helper / smoke). */
void aether_water_reflect_point(const aether_water_t *w, const f32 in[3], f32 out[3]);

/* True when reflection hooks are ready for Metal encode. */
bool aether_water_reflect_encode_needed(const aether_water_reflect_t *r);

/* ---------- Reflection render-target plan (allocates RT + samples it) ---------- */
typedef struct aether_water_reflect_rt {
    u32  width;
    u32  height;
    f32  scale;          /* relative to framebuffer; default 0.5 */
    bool allocated;      /* ensure() succeeded */
    bool sample_enabled; /* fragment samples reflection texture */
    bool enabled;
    u32  tex_stub_id;    /* host/Metal texture handle stub (non-zero when allocated) */
    /* clear / resolve / mip hooks (host + Metal) */
    bool cleared;
    bool resolved;
    u32  mip_levels;     /* 1 = no mips; >1 after gen_mips */
    f32  clear_rgba[4];
} aether_water_reflect_rt_t;

typedef struct aether_water_reflect_rt_plan {
    u32  pass_count;     /* 1 = render mirrored scene into RT */
    u32  width;
    u32  height;
    bool allocate;
    bool sample;
    bool needed;
} aether_water_reflect_rt_plan_t;

aether_result_t aether_water_reflect_rt_init(aether_water_reflect_rt_t *rt);
void aether_water_reflect_rt_shutdown(aether_water_reflect_rt_t *rt);
void aether_water_reflect_rt_set_enabled(aether_water_reflect_rt_t *rt, bool enabled);

/* Allocate (or resize) reflection RT at fb_w*scale × fb_h*scale. Marks allocated. */
aether_result_t aether_water_reflect_rt_ensure(aether_water_reflect_rt_t *rt,
                                               u32 fb_w, u32 fb_h, f32 scale);

/* Encode plan: allocate RT + sample it (not uniforms-only). */
void aether_water_reflect_rt_encode_plan(const aether_water_reflect_rt_t *rt,
                                         const aether_water_reflect_t *reflect,
                                         aether_water_reflect_rt_plan_t *out);

bool aether_water_reflect_rt_sample_needed(const aether_water_reflect_rt_t *rt);
bool aether_water_reflect_rt_encode_needed(const aether_water_reflect_rt_t *rt,
                                           const aether_water_reflect_t *reflect);

/* ---------- Mirrored-camera encode into reflection RT ---------- */
typedef struct aether_water_reflect_rt_draw {
    f32 mirror_mvp[16];   /* column-major: proj * view_mirrored */
    f32 mirror_view[16];
    f32 mirror_proj[16];
    f32 eye_reflected[3];
    f32 clip_plane[4];
    u32 width;
    u32 height;
    bool clear;           /* clear RT before draw */
    bool draw_world;      /* draw world mesh with mirror_mvp */
    bool resolve;         /* resolve/mip after draw */
    bool needed;
} aether_water_reflect_rt_draw_t;

/* Build mirrored view from eye + reflection, then MVP = proj * view_m.
 * view/proj are column-major 4x4 (pass identity proj for smoke). */
void aether_water_reflect_rt_build_mirror_mvp(const aether_water_reflect_t *reflect,
                                              const f32 view[16], const f32 proj[16],
                                              f32 out_mvp[16], f32 out_view_m[16]);

/* Full clear → draw-world → resolve plan for Metal encode into RT. */
void aether_water_reflect_rt_draw_plan(const aether_water_reflect_rt_t *rt,
                                       const aether_water_reflect_t *reflect,
                                       const f32 view[16], const f32 proj[16],
                                       aether_water_reflect_rt_draw_t *out);

/* Clear / resolve / mip stubs (host smoke + Metal hooks). */
aether_result_t aether_water_reflect_rt_clear(aether_water_reflect_rt_t *rt,
                                              f32 r, f32 g, f32 b, f32 a);
aether_result_t aether_water_reflect_rt_resolve(aether_water_reflect_rt_t *rt);
aether_result_t aether_water_reflect_rt_gen_mips(aether_water_reflect_rt_t *rt);
bool aether_water_reflect_rt_was_cleared(const aether_water_reflect_rt_t *rt);
bool aether_water_reflect_rt_was_resolved(const aether_water_reflect_rt_t *rt);
u32  aether_water_reflect_rt_mip_levels(const aether_water_reflect_rt_t *rt);

#endif /* AETHER_WATER_H */
