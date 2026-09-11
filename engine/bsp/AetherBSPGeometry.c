/* AetherBSPGeometry.c — BSP faces → triangle mesh.
 * Uses fan triangulation: face with N edges → N-2 triangles.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherBSPGeometry.h"
#include <stdlib.h>
#include <string.h>

/* Surfedge → vertex index (signed: >=0 means edge.v0, <0 means edge.v1). */
static u16 surfedge_vertex(const aether_bsp_t *bsp, i32 se) {
    const aether_bsp_edge_t *e;
    if (se >= 0) {
        e = aether_bsp_edge_at(bsp, (u32)se);
        return e ? e->v0 : 0;
    } else {
        e = aether_bsp_edge_at(bsp, (u32)(-se));
        return e ? e->v1 : 0;
    }
}

aether_result_t aether_mesh_from_bsp(const aether_bsp_t *bsp, aether_mesh_t **out_mesh) {
    if (!bsp || !out_mesh || !aether_bsp_is_valid(bsp)) return AETHER_ERR_INVALID_ARG;
    *out_mesh = NULL;

    u32 face_count = aether_bsp_face_count(bsp);
    if (face_count == 0) return AETHER_ERR_NOT_FOUND;

    /* --- Pass 1: count total vertices and indices --- */
    u64 total_verts = 0, total_indices = 0;
    for (u32 f = 0; f < face_count; ++f) {
        const aether_bsp_face_t *face = aether_bsp_face_at(bsp, f);
        if (!face || face->num_edges < 3) continue;
        total_verts   += face->num_edges;
        total_indices += (u64)(face->num_edges - 2) * 3;
    }
    if (total_verts == 0 || total_indices == 0) return AETHER_ERR_NOT_FOUND;

    /* --- Allocate output --- */
    aether_mesh_t *m = (aether_mesh_t*)calloc(1, sizeof *m);
    if (!m) return AETHER_ERR_OUT_OF_MEM;

    m->vertices = (aether_mesh_vertex_t*)malloc((size_t)total_verts * sizeof(aether_mesh_vertex_t));
    m->indices  = (u32*)malloc((size_t)total_indices * sizeof(u32));
    if (!m->vertices || !m->indices) {
        free(m->vertices); free(m->indices); free(m);
        return AETHER_ERR_OUT_OF_MEM;
    }

    /* --- Init bounds --- */
    f32 inf = 1e30f;
    m->bounds_min[0] = m->bounds_min[1] = m->bounds_min[2] =  inf;
    m->bounds_max[0] = m->bounds_max[1] = m->bounds_max[2] = -inf;

    /* --- Pass 2: fill data --- */
    u32 vcursor = 0, icursor = 0;
    u32 faces_emitted = 0;

    for (u32 f = 0; f < face_count; ++f) {
        const aether_bsp_face_t *face = aether_bsp_face_at(bsp, f);
        if (!face || face->num_edges < 3) continue;

        const aether_bsp_plane_t *plane = aether_bsp_plane_at(bsp, face->plane);
        f32 nx = 0, ny = 0, nz = 1;
        if (plane) {
            nx = plane->normal[0];
            ny = plane->normal[1];
            nz = plane->normal[2];
            /* Flip normal if face is on back side of plane */
            if (face->side) { nx = -nx; ny = -ny; nz = -nz; }
        }

        u32 base_vertex = vcursor;

        /* Emit each edge as a vertex (duplicated per face — simple). */
        for (u16 i = 0; i < face->num_edges; ++i) {
            i32 se = 0;
            /* surfedges list */
            const u8 *se_data = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_SURFEDGES);
            if (!se_data) break;
            u32 se_off = (u32)(face->first_edge + i);
            /* read i32 LE */
            const u8 *p = se_data + se_off * 4;
            se = (i32)((u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24));

            u16 vtx_idx = surfedge_vertex(bsp, se);
            const aether_bsp_vertex_t *v = aether_bsp_vertex_at(bsp, vtx_idx);
            if (!v) continue;

            aether_mesh_vertex_t *out = &m->vertices[vcursor++];
            out->x = v->x; out->y = v->y; out->z = v->z;
            out->nx = nx; out->ny = ny; out->nz = nz;
            out->u = 0; out->v = 0;

            /* Update bounds */
            if (v->x < m->bounds_min[0]) m->bounds_min[0] = v->x;
            if (v->y < m->bounds_min[1]) m->bounds_min[1] = v->y;
            if (v->z < m->bounds_min[2]) m->bounds_min[2] = v->z;
            if (v->x > m->bounds_max[0]) m->bounds_max[0] = v->x;
            if (v->y > m->bounds_max[1]) m->bounds_max[1] = v->y;
            if (v->z > m->bounds_max[2]) m->bounds_max[2] = v->z;
        }

        /* Fan triangulate: (0,i,i+1) for i in 1..N-2 */
        for (u16 i = 1; i + 1 < face->num_edges; ++i) {
            m->indices[icursor++] = base_vertex + 0;
            m->indices[icursor++] = base_vertex + i;
            m->indices[icursor++] = base_vertex + i + 1;
        }
        faces_emitted++;
    }

    m->vertex_count = vcursor;
    m->index_count  = icursor;
    m->bounds_center[0] = (m->bounds_min[0] + m->bounds_max[0]) * 0.5f;
    m->bounds_center[1] = (m->bounds_min[1] + m->bounds_max[1]) * 0.5f;
    m->bounds_center[2] = (m->bounds_min[2] + m->bounds_max[2]) * 0.5f;

    aether_log(AETHER_LOG_INFO, "bsp-geo",
               "mesh built: %u faces, %u verts, %u indices",
               faces_emitted, m->vertex_count, m->index_count);
    *out_mesh = m;
    return AETHER_OK;
}

void aether_mesh_free(aether_mesh_t *mesh) {
    if (!mesh) return;
    free(mesh->vertices);
    free(mesh->indices);
    free(mesh);
}

void aether_mesh_dump(const aether_mesh_t *mesh) {
    if (!mesh) { aether_log(AETHER_LOG_WARN, "bsp-geo", "dump: null"); return; }
    aether_log(AETHER_LOG_INFO, "bsp-geo", "===== MESH =====");
    aether_log(AETHER_LOG_INFO, "bsp-geo", "  vertices : %u", mesh->vertex_count);
    aether_log(AETHER_LOG_INFO, "bsp-geo", "  indices  : %u", mesh->index_count);
    aether_log(AETHER_LOG_INFO, "bsp-geo", "  triangles: %u", mesh->index_count / 3);
    aether_log(AETHER_LOG_INFO, "bsp-geo", "  min      : %.1f %.1f %.1f",
               mesh->bounds_min[0], mesh->bounds_min[1], mesh->bounds_min[2]);
    aether_log(AETHER_LOG_INFO, "bsp-geo", "  max      : %.1f %.1f %.1f",
               mesh->bounds_max[0], mesh->bounds_max[1], mesh->bounds_max[2]);
    aether_log(AETHER_LOG_INFO, "bsp-geo", "  center   : %.1f %.1f %.1f",
               mesh->bounds_center[0], mesh->bounds_center[1], mesh->bounds_center[2]);
    aether_log(AETHER_LOG_INFO, "bsp-geo", "================");
}
