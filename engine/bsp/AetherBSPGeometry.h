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
} aether_mesh_vertex_t;

typedef struct aether_mesh {
    aether_mesh_vertex_t *vertices;
    u32                   vertex_count;
    u32                  *indices;
    u32                   index_count;
    f32                   bounds_min[3];
    f32                   bounds_max[3];
    f32                   bounds_center[3];
} aether_mesh_t;

/* Build a mesh. If atlas is NULL, UVs are zeroed.
 * If atlas provided, UVs are baked into atlas coordinates. */
aether_result_t aether_mesh_from_bsp(const aether_bsp_t *bsp,
                                     const aether_texture_atlas_t *atlas,
                                     aether_mesh_t **out_mesh);

void aether_mesh_free(aether_mesh_t *mesh);
void aether_mesh_dump(const aether_mesh_t *mesh);

#ifdef __cplusplus
}
#endif
#endif
