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

#endif
