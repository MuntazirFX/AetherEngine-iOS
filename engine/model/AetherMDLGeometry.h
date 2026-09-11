/* AetherMDLGeometry.h — Extract static triangle mesh from MDL (STEP 16B).
 * Reads verts/norms/triangles ignoring bone transforms (bind pose).
 * AetherEngine-iOS · Clean-room.
 */
#ifndef AETHER_MDL_GEOMETRY_H
#define AETHER_MDL_GEOMETRY_H

#include "AetherMDL.h"
#include "../core/AetherMath.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct aether_model_mesh {
    /* Per-vertex: pos, normal (no UV yet) */
    f32 *positions;   /* 3 floats * vertex_count */
    f32 *normals;     /* 3 floats * vertex_count */
    u32 *indices;     /* triangle indices, 3 * triangle_count */
    u32  vertex_count;
    u32  triangle_count;

    f32  bounds_min[3];
    f32  bounds_max[3];
    f32  bounds_center[3];
} aether_model_mesh_t;

aether_result_t aether_mdl_geometry_extract(const aether_mdl_t *mdl,
                                              aether_model_mesh_t **out_mesh);
void            aether_mdl_geometry_free(aether_model_mesh_t *m);
void            aether_mdl_geometry_dump(const aether_model_mesh_t *m);

#ifdef __cplusplus
}
#endif
#endif
