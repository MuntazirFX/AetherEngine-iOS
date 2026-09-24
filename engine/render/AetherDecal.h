#ifndef AETHER_DECAL_H
#define AETHER_DECAL_H
#include "../core/AetherCore.h"

struct aether_mesh;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_decal {
    f32 position[3];
    f32 normal[3];
    f32 size;
    f32 life;
    f32 age;
    bool active;
} aether_decal_t;

#define AETHER_MAX_DECALS 256

typedef struct aether_decals {
    aether_decal_t items[AETHER_MAX_DECALS];
    u32 count;
} aether_decals_t;

/* Metal slice (point): center + normal + size + fade = 8 floats */
typedef struct aether_decal_vertex {
    f32 x, y, z;
    f32 nx, ny, nz;
    f32 size;
    f32 fade; /* 1..0 over life */
} aether_decal_vertex_t;

/* Projected / billboard quad vertex for Metal triangles (pos + uv + fade + rgba). */
typedef struct aether_decal_quad_vertex {
    f32 x, y, z;
    f32 u, v;
    f32 fade;
    f32 r, g, b, a;
} aether_decal_quad_vertex_t;

aether_result_t aether_decals_init(aether_decals_t *d);
aether_result_t aether_decals_add(aether_decals_t *d, const f32 pos[3], const f32 normal[3],
                                  f32 size, f32 life);
void aether_decals_update(aether_decals_t *d, f32 dt);
void aether_decals_clear(aether_decals_t *d);
u32  aether_decals_active_count(const aether_decals_t *d);
u32  aether_decals_copy_render(const aether_decals_t *d,
                               aether_decal_vertex_t *out, u32 max_out);

/* Expand each active decal into 6 verts (2 tris) oriented on the surface normal.
 * Returns vertex count written (multiple of 6). max_out is vertex capacity. */
u32 aether_decals_copy_quads(const aether_decals_t *d,
                             aether_decal_quad_vertex_t *out, u32 max_out);

/* Project / clip each active decal onto mesh triangles whose normals align with
 * the decal normal. Uses Sutherland–Hodgman clip against the decal square in
 * tangent space (closer to real world clip than AABB accept/reject).
 * Returns vertex count written (multiple of 3). */
u32 aether_decals_project_onto_mesh(const aether_decals_t *d,
                                    const struct aether_mesh *mesh,
                                    aether_decal_quad_vertex_t *out, u32 max_out);

/* Explicit world-clip entry (same as project_onto_mesh; kept for API greps). */
u32 aether_decals_clip_to_world(const aether_decals_t *d,
                                const struct aether_mesh *mesh,
                                aether_decal_quad_vertex_t *out, u32 max_out);


/* Procedural decal atlas (RGBA8) for Metal sample — clean-room, no HL sprites. */
#define AETHER_DECAL_ATLAS_W 64
#define AETHER_DECAL_ATLAS_H 64

typedef struct aether_decal_atlas {
    u32 width, height;
    u8 *rgba; /* owned */
} aether_decal_atlas_t;

aether_result_t aether_decal_atlas_init(aether_decal_atlas_t *a, u32 w, u32 h);
void aether_decal_atlas_shutdown(aether_decal_atlas_t *a);
/* Paint soft circular / scorched blob into atlas (procedural). */
aether_result_t aether_decal_atlas_generate_stub(aether_decal_atlas_t *a);
u32 aether_decal_atlas_copy_rgba(const aether_decal_atlas_t *a, u8 *out, u32 max_bytes);
/* Sample atlas at UV into rgb (CPU path for smoke). */
void aether_decal_atlas_sample(const aether_decal_atlas_t *a, f32 u, f32 v, f32 rgb[3]);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_DECAL_H */
