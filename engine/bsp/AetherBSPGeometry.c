/* AetherBSPGeometry.c — BSP → mesh with atlas UVs (STEP 15B).
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherBSPGeometry.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

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

aether_result_t aether_mesh_from_bsp(const aether_bsp_t *bsp,
                                     const aether_texture_atlas_t *atlas,
                                     aether_mesh_t **out_mesh) {
    if (!bsp || !out_mesh || !aether_bsp_is_valid(bsp)) return AETHER_ERR_INVALID_ARG;
    *out_mesh = NULL;

    u32 face_count = aether_bsp_face_count(bsp);
    if (face_count == 0) return AETHER_ERR_NOT_FOUND;

    u64 total_verts = 0, total_indices = 0;
    for (u32 f = 0; f < face_count; ++f) {
        const aether_bsp_face_t *face = aether_bsp_face_at(bsp, f);
        if (!face || face->num_edges < 3) continue;
        total_verts   += face->num_edges;
        total_indices += (u64)(face->num_edges - 2) * 3;
    }
    if (total_verts == 0 || total_indices == 0) return AETHER_ERR_NOT_FOUND;

    aether_mesh_t *m = (aether_mesh_t*)calloc(1, sizeof *m);
    if (!m) return AETHER_ERR_OUT_OF_MEM;
    m->vertices = (aether_mesh_vertex_t*)malloc((size_t)total_verts * sizeof(aether_mesh_vertex_t));
    m->indices  = (u32*)malloc((size_t)total_indices * sizeof(u32));
    if (!m->vertices || !m->indices) {
        free(m->vertices); free(m->indices); free(m); return AETHER_ERR_OUT_OF_MEM;
    }

    f32 inf = 1e30f;
    m->bounds_min[0] = m->bounds_min[1] = m->bounds_min[2] =  inf;
    m->bounds_max[0] = m->bounds_max[1] = m->bounds_max[2] = -inf;

    u32 vcursor = 0, icursor = 0;

    for (u32 f = 0; f < face_count; ++f) {
        const aether_bsp_face_t *face = aether_bsp_face_at(bsp, f);
        if (!face || face->num_edges < 3) continue;

        const aether_bsp_plane_t *plane = aether_bsp_plane_at(bsp, face->plane);
        f32 nx = 0, ny = 0, nz = 1;
        if (plane) {
            nx = plane->normal[0]; ny = plane->normal[1]; nz = plane->normal[2];
            if (face->side) { nx = -nx; ny = -ny; nz = -nz; }
        }

        /* --- Lookup texture slot in atlas --- */
        const aether_bsp_texinfo_t *ti = aether_bsp_texinfo_at(bsp, face->texinfo);
        f32 su_x = 0, su_y = 0, su_z = 0, su_off = 0;
        f32 sv_x = 0, sv_y = 0, sv_z = 0, sv_off = 0;
        f32 atlas_u0 = 0, atlas_v0 = 0, atlas_du = 0, atlas_dv = 0;
        u32 tex_w = 64, tex_h = 64;

        if (ti) {
            su_x = ti->vecs[0][0]; su_y = ti->vecs[0][1]; su_z = ti->vecs[0][2]; su_off = ti->vecs[0][3];
            sv_x = ti->vecs[1][0]; sv_y = ti->vecs[1][1]; sv_z = ti->vecs[1][2]; sv_off = ti->vecs[1][3];
        }
        if (atlas && ti) {
            i32 slot = aether_texture_atlas_find(atlas, bsp, (u32)ti->miptex);
            if (slot >= 0) {
                const aether_texture_slot_t *s = &atlas->slots[slot];
                atlas_u0 = (f32)s->x / (f32)atlas->width;
                atlas_v0 = (f32)s->y / (f32)atlas->height;
                atlas_du = (f32)s->w / (f32)atlas->width;
                atlas_dv = (f32)s->h / (f32)atlas->height;
                tex_w = s->w; tex_h = s->h;
            }
        }

        u32 base_vertex = vcursor;

        for (u16 i = 0; i < face->num_edges; ++i) {
            const u8 *se_data = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_SURFEDGES);
            if (!se_data) break;
            u32 se_off = (u32)(face->first_edge + i);
            const u8 *p = se_data + se_off * 4;
            i32 se = (i32)((u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24));

            u16 vtx_idx = surfedge_vertex(bsp, se);
            const aether_bsp_vertex_t *v = aether_bsp_vertex_at(bsp, vtx_idx);
            if (!v) continue;

            /* Compute raw texture UV (in texel space) */
            f32 u_tex = su_x*v->x + su_y*v->y + su_z*v->z + su_off;
            f32 v_tex = sv_x*v->x + sv_y*v->y + sv_z*v->z + sv_off;
            /* Normalize to 0..1 (with fract to wrap) */
            f32 u_norm = u_tex / (f32)tex_w;
            f32 v_norm = v_tex / (f32)tex_h;
            u_norm -= floorf(u_norm);
            v_norm -= floorf(v_norm);
            /* Map into atlas slot */
            f32 u_atlas = atlas_u0 + u_norm * atlas_du;
            f32 v_atlas = atlas_v0 + v_norm * atlas_dv;

            aether_mesh_vertex_t *out = &m->vertices[vcursor++];
            out->x = v->x; out->y = v->y; out->z = v->z;
            out->nx = nx; out->ny = ny; out->nz = nz;
            out->u = u_atlas; out->v = v_atlas;

            if (v->x < m->bounds_min[0]) m->bounds_min[0] = v->x;
            if (v->y < m->bounds_min[1]) m->bounds_min[1] = v->y;
            if (v->z < m->bounds_min[2]) m->bounds_min[2] = v->z;
            if (v->x > m->bounds_max[0]) m->bounds_max[0] = v->x;
            if (v->y > m->bounds_max[1]) m->bounds_max[1] = v->y;
            if (v->z > m->bounds_max[2]) m->bounds_max[2] = v->z;
        }

        for (u16 i = 1; i + 1 < face->num_edges; ++i) {
            m->indices[icursor++] = base_vertex + 0;
            m->indices[icursor++] = base_vertex + i;
            m->indices[icursor++] = base_vertex + i + 1;
        }
    }

    m->vertex_count = vcursor;
    m->index_count  = icursor;
    m->bounds_center[0] = (m->bounds_min[0] + m->bounds_max[0]) * 0.5f;
    m->bounds_center[1] = (m->bounds_min[1] + m->bounds_max[1]) * 0.5f;
    m->bounds_center[2] = (m->bounds_min[2] + m->bounds_max[2]) * 0.5f;

    aether_log(AETHER_LOG_INFO, "bsp-geo",
               "mesh: %u verts, %u indices (atlas=%s)",
               m->vertex_count, m->index_count, atlas ? "yes" : "no");
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
    if (!mesh) return;
    aether_log(AETHER_LOG_INFO, "bsp-geo", "MESH: %u verts, %u tris",
               mesh->vertex_count, mesh->index_count / 3);
}
