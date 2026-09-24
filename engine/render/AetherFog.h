#ifndef AETHER_FOG_H
#define AETHER_FOG_H
#include "../core/AetherCore.h"

/* GPU/bridge-friendly fullscreen vertex: NDC xy + uv + color.rgba (8 floats, 32 bytes). */
typedef struct aether_fog_vertex {
    f32 x, y;       /* clip-space NDC */
    f32 u, v;
    f32 r, g, b, a; /* tint * fog factor */
} aether_fog_vertex_t;

typedef struct aether_fog {
    f32 density;    /* 0..1 base strength */
    f32 start;      /* linear fog start (world units; reserved for depth fog) */
    f32 end;        /* linear fog end */
    f32 color[4];   /* RGBA tint */
    f32 factor;     /* overall fullscreen alpha multiplier */
    bool enabled;
} aether_fog_t;

aether_result_t aether_fog_init(aether_fog_t *f);
void aether_fog_shutdown(aether_fog_t *f);
void aether_fog_set_enabled(aether_fog_t *f, bool enabled);
void aether_fog_set_range(aether_fog_t *f, f32 start, f32 end);
void aether_fog_set_density(aether_fog_t *f, f32 density);
aether_result_t aether_fog_set_color(aether_fog_t *f, const f32 rgba[4]);
aether_result_t aether_fog_set_factor(aether_fog_t *f, f32 factor);

/* Copy a fullscreen tint quad into out[]. Returns vertex count written
 * (always a multiple of 3). max_out is vertex capacity. */
u32 aether_fog_copy_render(const aether_fog_t *f,
                           aether_fog_vertex_t *out,
                           u32 max_out);
/* How many vertices the fullscreen fog quad needs (2 tris = 6). */
u32 aether_fog_render_vertex_count(void);

#endif
