#ifndef AETHER_SKY_H
#define AETHER_SKY_H
#include "../core/AetherCore.h"

/* Six-face skybox order (GoldSrc-style): rt, lf, ft, bk, up, dn. */
typedef enum aether_sky_face {
    AETHER_SKY_FACE_RT = 0,
    AETHER_SKY_FACE_LF,
    AETHER_SKY_FACE_FT,
    AETHER_SKY_FACE_BK,
    AETHER_SKY_FACE_UP,
    AETHER_SKY_FACE_DN,
    AETHER_SKY_FACE_COUNT
} aether_sky_face_t;

/* GPU/bridge-friendly vertex: pos.xyz + color.rgba (7 floats, 28 bytes). */
typedef struct aether_sky_vertex {
    f32 x, y, z;
    f32 r, g, b, a;
} aether_sky_vertex_t;

typedef struct aether_sky {
    char name[64];
    u32  face_count;
    bool enabled;
    f32  radius;
    /* Per-face tint (RGBA). Used when building the placeholder mesh. */
    f32  face_color[AETHER_SKY_FACE_COUNT][4];
    /* Gradient stops derived from faces (or set explicitly). */
    f32  top_color[4];
    f32  horizon_color[4];
    f32  bottom_color[4];
} aether_sky_t;

aether_result_t aether_sky_init(aether_sky_t *s);
aether_result_t aether_sky_set_name(aether_sky_t *s, const char *name);
aether_result_t aether_sky_set_enabled(aether_sky_t *s, bool enabled);
aether_result_t aether_sky_set_radius(aether_sky_t *s, f32 radius);
aether_result_t aether_sky_set_face_color(aether_sky_t *s,
                                          aether_sky_face_t face,
                                          const f32 rgba[4]);
/* Refresh top/horizon/bottom from the six face colors. */
void aether_sky_rebuild_gradient(aether_sky_t *s);
void aether_sky_shutdown(aether_sky_t *s);

/* Copy a unit-sphere hemisphere (Z-up) tinted by gradient into out[].
 * Writes triangles as interleaved vertices. Returns vertex count written
 * (always a multiple of 3). max_out is vertex capacity. */
u32 aether_sky_copy_render(const aether_sky_t *s,
                           aether_sky_vertex_t *out,
                           u32 max_out);
/* How many vertices the default hemisphere mesh needs (rings*segs*6). */
u32 aether_sky_render_vertex_count(void);

#endif
