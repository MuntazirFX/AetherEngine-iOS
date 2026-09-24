/* AetherBSPGeometry.h — BSP → triangle mesh with UV coords (STEP 15B).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_BSP_GEOMETRY_H
#define AETHER_BSP_GEOMETRY_H

#include "AetherBSP.h"
#include "../texture/AetherTexture.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_mesh_vertex {
    f32 x, y, z;      /* position */
    f32 nx, ny, nz;   /* normal */
    f32 u, v;         /* atlas UV (0..1) */
    f32 lu, lv;       /* lightmap UV (0..1) — procedural stub or BSP lightmap */
    f32 face_id;      /* BSP face index for Metal multi-style LM blend (attr) */
} aether_mesh_vertex_t;

/* Floats per mesh vertex (pos3+n3+uv2+luv2+face_id). */
#define AETHER_MESH_VERTEX_FLOATS 11
#define AETHER_MESH_VERTEX_STRIDE 44

/* Maps one BSP face → a contiguous index range in the mesh (for VIS culling). */
typedef struct aether_mesh_face_range {
    u32 first_index;
    u32 index_count;
    u32 first_vertex;
    u32 vertex_count;
    u8  styles[4];   /* BSP face lightstyle indices (255 = unused) */
} aether_mesh_face_range_t;

typedef struct aether_mesh {
    aether_mesh_vertex_t    *vertices;
    u32                      vertex_count;
    u32                     *indices;
    u32                      index_count;
    aether_mesh_face_range_t *face_ranges; /* length == face_count; may be NULL on OOM path */
    u32                      face_count;  /* BSP face count at build time */
    f32                      bounds_min[3];
    f32                      bounds_max[3];
    f32                      bounds_center[3];
} aether_mesh_t;

/* Build a mesh. If atlas is NULL, UVs are zeroed.
 * If atlas provided, UVs are baked into atlas coordinates. */
aether_result_t aether_mesh_from_bsp(const aether_bsp_t *bsp,
                                     const aether_texture_atlas_t *atlas,
                                     aether_mesh_t **out_mesh);

void aether_mesh_free(aether_mesh_t *mesh);
void aether_mesh_dump(const aether_mesh_t *mesh);

/* Stamp face_id onto every vertex from face_ranges (idempotent). Returns verts stamped. */
u32 aether_mesh_assign_face_ids(aether_mesh_t *mesh);

/* Verify face_id matches owning face_ranges. Returns mismatch count (0 = ok). */
u32 aether_mesh_validate_face_ids(const aether_mesh_t *mesh);


#ifdef __cplusplus
}
#endif
#endif
