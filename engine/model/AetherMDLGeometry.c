/* AetherMDLGeometry.c — Static MDL → mesh extraction (STEP 16B).
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherMDLGeometry.h"
#include <stdlib.h>
#include <string.h>

/* On-disk structure offsets. */
#define MDL_MESH_SIZE      20
#define MDL_VERTEX_SIZE    12   /* 3 floats; skip bone int for static */

#pragma pack(push, 1)
typedef struct mdl_mesh_disk {
    i32 num_tris;
    i32 tri_indexes;      /* offset to u16[3 * num_tris] */
    i32 skin_ref;
    i32 num_verts;
    i32 vert_info_index;  /* offset to u8[num_verts] */
    i32 num_norms;
    i32 norm_info_index;
    i32 num_groups;
    i32 group_index;
} mdl_mesh_disk_t;

typedef struct mdl_model_disk {
    char name[64];
    i32  type;
    f32  bounding_radius;
    i32  num_meshes;
    i32  mesh_index;
    i32  num_verts;
    i32  vert_info_index;
    i32  num_norms;
    i32  norm_info_index;
    i32  num_groups;
    i32  group_index;
} mdl_model_disk_t;

typedef struct mdl_bodypart_disk {
    char name[64];
    i32  num_models;
    i32  base;
    i32  model_index;
} mdl_bodypart_disk_t;

typedef struct mdl_bone_disk {
    char name[32];
    i32  parent;
    i32  flags;
    i32  bone_controller[6];
    f32  value[6];
    f32  scale[6];
} mdl_bone_disk_t;
#pragma pack(pop)

static i32 rd_i32(const u8 *p) {
    return (i32)((u32)p[0] | ((u32)p[1]<<8) | ((u32)p[2]<<16) | ((u32)p[3]<<24));
}
static u16 rd_u16(const u8 *p) {
    return (u16)((u32)p[0] | ((u32)p[1]<<8));
}
static f32 rd_f32(const u8 *p) {
    u32 v = (u32)p[0] | ((u32)p[1]<<8) | ((u32)p[2]<<16) | ((u32)p[3]<<24);
    f32 f; memcpy(&f, &v, 4); return f;
}

/* Header offsets we need */
#define HDR_NUMBODY     204
#define HDR_BODYIDX     208
#define HDR_NUMVERTS    232  /* actually in some versions this is at 208? */
/* Actually: header has no global verts array. Each model has its own. */
/* We'll rely on per-model vert_info_index. */

/* Since we don't have easy access to raw bytes from the aether_mdl_t opaque type,
 * we need to expose a raw accessor. For now, we'll re-parse here using
 * the file-level API we know (headers and section offsets). 
 *
 * NOTE: To keep this file self-contained, we re-read the MDL header from
 * the file-level accessors. But we need raw access. Let's add a helper
 * in AetherMDL.h to expose raw pointer. */

/* We'll add a helper: aether_mdl_raw() */
extern const u8 *aether_mdl_raw_data(const aether_mdl_t *m, u32 *out_size);

aether_result_t aether_mdl_geometry_extract(const aether_mdl_t *mdl,
                                              aether_model_mesh_t **out_mesh) {
    if (!mdl || !out_mesh || !aether_mdl_is_valid(mdl))
        return AETHER_ERR_INVALID_ARG;
    *out_mesh = NULL;

    u32 raw_size = 0;
    const u8 *raw = aether_mdl_raw_data(mdl, &raw_size);
    if (!raw || raw_size < 244) return AETHER_ERR_INVALID_ARG;

    const u8 *h = raw;
    i32 num_bodyparts = rd_i32(h + HDR_NUMBODY);
    i32 body_offset   = rd_i32(h + HDR_BODYIDX);
    if (num_bodyparts <= 0 || body_offset <= 0) return AETHER_ERR_NOT_FOUND;

    /* Pass 1: count total triangles + vertices */
    u64 total_tris = 0, total_verts = 0;

    for (i32 bp = 0; bp < num_bodyparts; ++bp) {
        u32 bp_off = (u32)body_offset + (u32)bp * sizeof(mdl_bodypart_disk_t);
        if (bp_off + sizeof(mdl_bodypart_disk_t) > raw_size) break;
        const mdl_bodypart_disk_t *bpv = (const mdl_bodypart_disk_t*)(raw + bp_off);

        for (i32 mi = 0; mi < bpv->num_models; ++mi) {
            u32 moff = (u32)bpv->model_index + (u32)mi * sizeof(mdl_model_disk_t);
            if (moff + sizeof(mdl_model_disk_t) > raw_size) break;
            const mdl_model_disk_t *mv = (const mdl_model_disk_t*)(raw + moff);

            for (i32 mesh_i = 0; mesh_i < mv->num_meshes; ++mesh_i) {
                u32 mesh_off = (u32)mv->mesh_index + (u32)mesh_i * MDL_MESH_SIZE;
                if (mesh_off + MDL_MESH_SIZE > raw_size) break;
                const mdl_mesh_disk_t *mesh = (const mdl_mesh_disk_t*)(raw + mesh_off);
                if (mesh->num_tris <= 0) continue;
                total_tris  += mesh->num_tris;
                total_verts += mesh->num_tris * 3;   /* expand for fan */
            }
        }
    }

    if (total_tris == 0 || total_verts == 0) return AETHER_ERR_NOT_FOUND;

    /* Allocate */
    aether_model_mesh_t *m = (aether_model_mesh_t*)calloc(1, sizeof *m);
    if (!m) return AETHER_ERR_OUT_OF_MEM;
    m->positions = (f32*)malloc((size_t)total_verts * 3 * sizeof(f32));
    m->normals   = (f32*)malloc((size_t)total_verts * 3 * sizeof(f32));
    m->indices   = (u32*)malloc((size_t)total_tris  * 3 * sizeof(u32));
    if (!m->positions || !m->normals || !m->indices) {
        aether_mdl_geometry_free(m);
        return AETHER_ERR_OUT_OF_MEM;
    }

    f32 inf = 1e30f;
    m->bounds_min[0]=m->bounds_min[1]=m->bounds_min[2]= inf;
    m->bounds_max[0]=m->bounds_max[1]=m->bounds_max[2]=-inf;

    u32 vcur = 0, icur = 0;

    /* Pass 2: extract */
    for (i32 bp = 0; bp < num_bodyparts; ++bp) {
        u32 bp_off = (u32)body_offset + (u32)bp * sizeof(mdl_bodypart_disk_t);
        if (bp_off + sizeof(mdl_bodypart_disk_t) > raw_size) break;
        const mdl_bodypart_disk_t *bpv = (const mdl_bodypart_disk_t*)(raw + bp_off);

        for (i32 mi = 0; mi < bpv->num_models; ++mi) {
            u32 moff = (u32)bpv->model_index + (u32)mi * sizeof(mdl_model_disk_t);
            if (moff + sizeof(mdl_model_disk_t) > raw_size) break;
            const mdl_model_disk_t *mv = (const mdl_model_disk_t*)(raw + moff);

            /* Per-model vertex + normal arrays (global layout) */
            const u8 *verts_arr = raw + mv->vert_info_index;
            const u8 *norms_arr = raw + mv->norm_info_index;

            for (i32 mesh_i = 0; mesh_i < mv->num_meshes; ++mesh_i) {
                u32 mesh_off = (u32)mv->mesh_index + (u32)mesh_i * MDL_MESH_SIZE;
                if (mesh_off + MDL_MESH_SIZE > raw_size) break;
                const mdl_mesh_disk_t *mesh = (const mdl_mesh_disk_t*)(raw + mesh_off);
                if (mesh->num_tris <= 0) continue;

                const u8 *tri_idx = raw + mesh->tri_indexes;
                u32 base_vertex = vcur;

                for (i32 t = 0; t < mesh->num_tris; ++t) {
                    for (int k = 0; k < 3; ++k) {
                        u16 v_idx = rd_u16(tri_idx + (t*3 + k) * 2);
                        u16 n_idx = v_idx;  /* In HL, normals usually match vertex index count */

                        if (v_idx >= mv->num_verts) continue;
                        if (n_idx >= mv->num_norms) n_idx = 0;

                        const u8 *vp = verts_arr + (u32)v_idx * MDL_VERTEX_SIZE;
                        const u8 *np = norms_arr + (u32)n_idx * MDL_VERTEX_SIZE;

                        f32 px = rd_f32(vp + 0);
                        f32 py = rd_f32(vp + 4);
                        f32 pz = rd_f32(vp + 8);
                        f32 nx = rd_f32(np + 0);
                        f32 ny = rd_f32(np + 4);
                        f32 nz = rd_f32(np + 8);

                        m->positions[vcur*3 + 0] = px;
                        m->positions[vcur*3 + 1] = py;
                        m->positions[vcur*3 + 2] = pz;
                        m->normals[vcur*3 + 0] = nx;
                        m->normals[vcur*3 + 1] = ny;
                        m->normals[vcur*3 + 2] = nz;

                        if (px < m->bounds_min[0]) m->bounds_min[0] = px;
                        if (py < m->bounds_min[1]) m->bounds_min[1] = py;
                        if (pz < m->bounds_min[2]) m->bounds_min[2] = pz;
                        if (px > m->bounds_max[0]) m->bounds_max[0] = px;
                        if (py > m->bounds_max[1]) m->bounds_max[1] = py;
                        if (pz > m->bounds_max[2]) m->bounds_max[2] = pz;

                        vcur++;
                    }
                    /* Fan triangle */
                    m->indices[icur++] = base_vertex + (u32)(t*3 + 0);
                    m->indices[icur++] = base_vertex + (u32)(t*3 + 1);
                    m->indices[icur++] = base_vertex + (u32)(t*3 + 2);
                }
            }
        }
    }

    m->vertex_count   = vcur;
    m->triangle_count = icur / 3;
    m->bounds_center[0] = (m->bounds_min[0] + m->bounds_max[0]) * 0.5f;
    m->bounds_center[1] = (m->bounds_min[1] + m->bounds_max[1]) * 0.5f;
    m->bounds_center[2] = (m->bounds_min[2] + m->bounds_max[2]) * 0.5f;

    aether_log(AETHER_LOG_INFO, "mdl-geo",
               "extracted: %u verts, %u tris, bounds [%.1f..%.1f]",
               m->vertex_count, m->triangle_count,
               m->bounds_min[0], m->bounds_max[0]);

    *out_mesh = m;
    return AETHER_OK;
}

void aether_mdl_geometry_free(aether_model_mesh_t *m) {
    if (!m) return;
    free(m->positions);
    free(m->normals);
    free(m->indices);
    free(m);
}

void aether_mdl_geometry_dump(const aether_model_mesh_t *m) {
    if (!m) return;
    aether_log(AETHER_LOG_INFO, "mdl-geo", "===== MDL MESH =====");
    aether_log(AETHER_LOG_INFO, "mdl-geo", "  verts: %u", m->vertex_count);
    aether_log(AETHER_LOG_INFO, "mdl-geo", "  tris : %u", m->triangle_count);
    aether_log(AETHER_LOG_INFO, "mdl-geo", "  min  : %.1f %.1f %.1f",
               m->bounds_min[0], m->bounds_min[1], m->bounds_min[2]);
    aether_log(AETHER_LOG_INFO, "mdl-geo", "  max  : %.1f %.1f %.1f",
               m->bounds_max[0], m->bounds_max[1], m->bounds_max[2]);
    aether_log(AETHER_LOG_INFO, "mdl-geo", "====================");
}
