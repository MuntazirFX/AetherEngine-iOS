/* AetherBSPGeometry.h — Convert BSP lumps into a triangle mesh.
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_BSP_GEOMETRY_H
#define AETHER_BSP_GEOMETRY_H

#include "AetherBSP.h"

#ifdef __cplusplus
extern "C" {
#endif

/* GPU-friendly vertex: position + normal + uv (32 bytes, natural align). */
typedef struct aether_mesh_vertex {
    f32 x, y, z;      /* position */
    f32 nx, ny, nz;   /* face normal */
    f32 u, v;         /* texture UV (unused in STEP 12) */
} aether_mesh_vertex_t;

/* A triangle mesh built from a BSP file. */
typedef struct aether_mesh {
    aether_mesh_vertex_t *vertices;
    u32                   vertex_count;
    u32                  *indices;
    u32                   index_count;
    f32                   bounds_min[3];
    f32                   bounds_max[3];
    f32                   bounds_center[3];
} aether_mesh_t;

/* Build a mesh from an in-memory BSP. Allocates; caller frees. */
aether_result_t aether_mesh_from_bsp(const aether_bsp_t *bsp, aether_mesh_t **out_mesh);

void aether_mesh_free(aether_mesh_t *mesh);

/* Diagnostics */
void aether_mesh_dump(const aether_mesh_t *mesh);

#ifdef __cplusplus
}
#endif
#endif /* AETHER_BSP_GEOMETRY_H */
