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

#endif
