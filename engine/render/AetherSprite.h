#ifndef AETHER_SPRITE_H
#define AETHER_SPRITE_H
#include "../core/AetherCore.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_sprite {
    f32 position[3];
    f32 size[2];
    f32 uv[4];
    f32 rotation;
    f32 color[4];
    bool visible;
} aether_sprite_t;

/* Billboard quad vertex (pos + uv + rgba) for Metal. */
typedef struct aether_sprite_quad_vertex {
    f32 x, y, z;
    f32 u, v;
    f32 r, g, b, a;
} aether_sprite_quad_vertex_t;

aether_result_t aether_sprite_init(aether_sprite_t *s);
void aether_sprite_set_uv(aether_sprite_t *s, f32 u0, f32 v0, f32 u1, f32 v1);
void aether_sprite_set_color(aether_sprite_t *s, const f32 color[4]);
void aether_sprite_set_position(aether_sprite_t *s, f32 x, f32 y, f32 z);
void aether_sprite_set_size(aether_sprite_t *s, f32 w, f32 h);

/* Emit 6 verts (2 tris) as a camera-facing billboard stub (axis-aligned on XY, Z-up).
 * view_right/view_up may be NULL → world XY axes. Returns verts written (0 or 6). */
u32 aether_sprite_copy_quad(const aether_sprite_t *s,
                            const f32 *view_right, const f32 *view_up,
                            aether_sprite_quad_vertex_t *out, u32 max_out);

#ifdef __cplusplus
}
#endif
#endif
