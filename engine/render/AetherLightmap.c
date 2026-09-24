/* AetherLightmap.c — Lightmap state + procedural grayscale atlas stub.
 * AetherEngine-iOS · Clean-room.
 */
#include "AetherLightmap.h"
#include "../bsp/AetherBSPGeometry.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

aether_result_t aether_lightmap_init(aether_lightmap_t *lm, u32 w, u32 h, u32 styles) {
    if (!lm || !w || !h) return AETHER_ERR_INVALID_ARG;
    memset(lm, 0, sizeof(*lm));
    lm->width = w;
    lm->height = h;
    lm->style_count = styles ? styles : 1;
    lm->enabled = true;
    return AETHER_OK;
}

void aether_lightmap_shutdown(aether_lightmap_t *lm) {
    if (!lm) return;
    free(lm->rgba);
    free(lm->base_rgba);
    memset(lm, 0, sizeof(*lm));
}

void aether_lightmap_enable(aether_lightmap_t *lm, bool e) {
    if (lm) lm->enabled = e;
}

bool aether_lightmap_is_enabled(const aether_lightmap_t *lm) {
    return lm ? lm->enabled : false;
}

u32 aether_lightmap_width(const aether_lightmap_t *lm) {
    return lm ? lm->width : 0;
}

u32 aether_lightmap_height(const aether_lightmap_t *lm) {
    return lm ? lm->height : 0;
}

bool aether_lightmap_is_stub(const aether_lightmap_t *lm) {
    return lm ? lm->stub : false;
}

u32 aether_lightmap_copy_rgba(const aether_lightmap_t *lm, u8 *out, u32 max_bytes) {
    if (!lm || !lm->rgba || !out || lm->width == 0 || lm->height == 0) return 0;
    u32 need = lm->width * lm->height * 4u;
    if (max_bytes < need) return 0;
    memcpy(out, lm->rgba, need);
    return need;
}

static u32 ceil_sqrt_u32(u32 n) {
    if (n == 0) return 1;
    u32 r = 1;
    while (r * r < n) ++r;
    return r;
}

static f32 clampf(f32 v, f32 lo, f32 hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

aether_result_t aether_lightmap_generate_stub(aether_lightmap_t *lm, u32 width, u32 height, u32 face_tiles) {
    if (!lm || !width || !height) return AETHER_ERR_INVALID_ARG;
    if (width > 2048u || height > 2048u) return AETHER_ERR_INVALID_ARG;

    u8 *buf = (u8 *)malloc((size_t)width * (size_t)height * 4u);
    if (!buf) return AETHER_ERR_OUT_OF_MEM;

    u32 tiles = face_tiles ? face_tiles : 1u;
    u32 cols = ceil_sqrt_u32(tiles);
    u32 rows = (tiles + cols - 1u) / cols;
    if (rows == 0) rows = 1;

    for (u32 y = 0; y < height; ++y) {
        for (u32 x = 0; x < width; ++x) {
            u32 col = (x * cols) / width;
            u32 row = (y * rows) / height;
            u32 tile = row * cols + col;
            if (tile >= tiles) tile = tiles - 1u;

            /* Local 0..1 inside this tile. */
            f32 u0 = (f32)col / (f32)cols;
            f32 v0 = (f32)row / (f32)rows;
            f32 du = 1.f / (f32)cols;
            f32 dv = 1.f / (f32)rows;
            f32 lu = ((f32)x / (f32)width - u0) / du;
            f32 lv = ((f32)y / (f32)height - v0) / dv;
            lu = clampf(lu, 0.f, 1.f);
            lv = clampf(lv, 0.f, 1.f);

            /* Soft radial falloff from tile center (reads as a simple light). */
            f32 dx = lu - 0.5f;
            f32 dy = lv - 0.5f;
            f32 dist = sqrtf(dx * dx + dy * dy);
            f32 radial = clampf(1.f - dist * 1.55f, 0.f, 1.f);
            /* Mild directional bias so faces don't look identical. */
            f32 bias = 0.08f * sinf((f32)tile * 1.7f + lu * 3.1f + lv * 2.3f);
            f32 shade = 0.28f + 0.62f * radial * radial + bias;
            shade = clampf(shade, 0.12f, 1.f);
            u8 g = (u8)(shade * 255.f + 0.5f);
            u32 i = (y * width + x) * 4u;
            buf[i + 0] = g;
            buf[i + 1] = g;
            buf[i + 2] = g;
            buf[i + 3] = 255;
        }
    }

    free(lm->rgba);
    free(lm->base_rgba); lm->base_rgba = NULL;
    lm->rgba = buf;
    lm->width = width;
    lm->height = height;
    lm->stub = true;
    lm->face_tiles = tiles;
    lm->enabled = true;
    (void)aether_lightmap_capture_base(lm);
    aether_log(AETHER_LOG_INFO, "lightmap",
               "stub atlas %ux%u tiles=%u (procedural, no assets)",
               width, height, tiles);
    return AETHER_OK;
}

/* Estimate face runs: contiguous verts sharing the same normal (BSP fans). */
static u32 count_face_runs(const aether_mesh_t *mesh) {
    if (!mesh || mesh->vertex_count == 0) return 0;
    u32 runs = 1;
    for (u32 i = 1; i < mesh->vertex_count; ++i) {
        const aether_mesh_vertex_t *a = &mesh->vertices[i - 1];
        const aether_mesh_vertex_t *b = &mesh->vertices[i];
        if (fabsf(a->nx - b->nx) > 1e-4f ||
            fabsf(a->ny - b->ny) > 1e-4f ||
            fabsf(a->nz - b->nz) > 1e-4f) {
            runs++;
        }
    }
    return runs;
}

aether_result_t aether_lightmap_assign_mesh_uvs(aether_lightmap_t *lm, aether_mesh_t *mesh) {
    if (!lm || !mesh || !mesh->vertices || mesh->vertex_count == 0)
        return AETHER_ERR_INVALID_ARG;

    u32 runs = count_face_runs(mesh);
    if (runs == 0) runs = 1;
    u32 cols = ceil_sqrt_u32(runs);
    u32 rows = (runs + cols - 1u) / cols;
    if (rows == 0) rows = 1;

    /* If stub face_tiles differs, still lay out against current run count. */
    (void)lm;

    u32 run_idx = 0;
    u32 i = 0;
    while (i < mesh->vertex_count) {
        u32 start = i;
        f32 nx = mesh->vertices[i].nx;
        f32 ny = mesh->vertices[i].ny;
        f32 nz = mesh->vertices[i].nz;
        while (i < mesh->vertex_count &&
               fabsf(mesh->vertices[i].nx - nx) <= 1e-4f &&
               fabsf(mesh->vertices[i].ny - ny) <= 1e-4f &&
               fabsf(mesh->vertices[i].nz - nz) <= 1e-4f) {
            ++i;
        }
        u32 end = i;

        /* Build orthonormal tangent frame from normal. */
        f32 ax = fabsf(nx), ay = fabsf(ny), az = fabsf(nz);
        f32 tx, ty, tz, bx, by, bz;
        if (ax < ay && ax < az) {
            tx = 0.f; ty = -nz; tz = ny;
        } else if (ay < az) {
            tx = -nz; ty = 0.f; tz = nx;
        } else {
            tx = -ny; ty = nx; tz = 0.f;
        }
        f32 tlen = sqrtf(tx * tx + ty * ty + tz * tz);
        if (tlen < 1e-6f) { tx = 1.f; ty = 0.f; tz = 0.f; tlen = 1.f; }
        tx /= tlen; ty /= tlen; tz /= tlen;
        bx = ny * tz - nz * ty;
        by = nz * tx - nx * tz;
        bz = nx * ty - ny * tx;

        f32 umin = 1e30f, umax = -1e30f, vmin = 1e30f, vmax = -1e30f;
        for (u32 v = start; v < end; ++v) {
            f32 px = mesh->vertices[v].x;
            f32 py = mesh->vertices[v].y;
            f32 pz = mesh->vertices[v].z;
            f32 u = px * tx + py * ty + pz * tz;
            f32 vv = px * bx + py * by + pz * bz;
            if (u < umin) umin = u;
            if (u > umax) umax = u;
            if (vv < vmin) vmin = vv;
            if (vv > vmax) vmax = vv;
        }
        f32 ud = umax - umin; if (ud < 1e-3f) ud = 1.f;
        f32 vd = vmax - vmin; if (vd < 1e-3f) vd = 1.f;

        u32 col = run_idx % cols;
        u32 row = run_idx / cols;
        f32 pad = 0.02f;
        f32 u0 = ((f32)col + pad) / (f32)cols;
        f32 v0 = ((f32)row + pad) / (f32)rows;
        f32 du = (1.f - 2.f * pad) / (f32)cols;
        f32 dv = (1.f - 2.f * pad) / (f32)rows;

        for (u32 v = start; v < end; ++v) {
            f32 px = mesh->vertices[v].x;
            f32 py = mesh->vertices[v].y;
            f32 pz = mesh->vertices[v].z;
            f32 u = ((px * tx + py * ty + pz * tz) - umin) / ud;
            f32 vv = ((px * bx + py * by + pz * bz) - vmin) / vd;
            mesh->vertices[v].lu = u0 + clampf(u, 0.f, 1.f) * du;
            mesh->vertices[v].lv = v0 + clampf(vv, 0.f, 1.f) * dv;
        }
        run_idx++;
    }

    aether_log(AETHER_LOG_INFO, "lightmap",
               "assigned LUV for %u verts across %u face runs (%ux%u tiles)",
               mesh->vertex_count, runs, cols, rows);
    return AETHER_OK;
}

aether_result_t aether_lightmap_bake_mesh_stub(aether_lightmap_t *lm, aether_mesh_t *mesh) {
    if (!lm || !mesh) return AETHER_ERR_INVALID_ARG;
    u32 runs = count_face_runs(mesh);
    if (runs == 0) runs = 1;
    aether_result_t r = aether_lightmap_generate_stub(lm, 256, 256, runs);
    if (r != AETHER_OK) return r;
    return aether_lightmap_assign_mesh_uvs(lm, mesh);
}


#include "../bsp/AetherBSP.h"

aether_result_t aether_lightmap_bake_from_bsp(aether_lightmap_t *lm,
                                              const aether_bsp_t *bsp,
                                              aether_mesh_t *mesh) {
    if (!lm || !mesh) return AETHER_ERR_INVALID_ARG;
    if (!bsp) return aether_lightmap_bake_mesh_stub(lm, mesh);

    u32 lighting_sz = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_LIGHTING);
    const u8 *lighting = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_LIGHTING);
    u32 face_count = aether_bsp_face_count(bsp);

    if (!lighting || lighting_sz < 3 || face_count == 0) {
        aether_log(AETHER_LOG_INFO, "lightmap",
                   "no LIGHTING lump — procedural stub path");
        aether_result_t r = aether_lightmap_bake_mesh_stub(lm, mesh);
        if (r != AETHER_OK) return r;
        /* Still unpack texinfo LUV when face_ranges exist (real UV layout). */
        if (aether_bsp_texinfo_count(bsp) > 0 && mesh->face_ranges && mesh->face_count > 0)
            (void)aether_lightmap_unpack_uvs_from_bsp(lm, bsp, mesh);
        return AETHER_OK;
    }

    /* Atlas: one tile per face, sample first RGB from each face light_offset. */
    u32 tiles = face_count ? face_count : 1u;
    u32 cols = 1;
    while (cols * cols < tiles) ++cols;
    u32 rows = (tiles + cols - 1u) / cols;
    if (rows == 0) rows = 1;
    const u32 W = 256, H = 256;
    u8 *buf = (u8 *)malloc((size_t)W * (size_t)H * 4u);
    if (!buf) return AETHER_ERR_OUT_OF_MEM;
    memset(buf, 32, (size_t)W * H * 4u);

    for (u32 fi = 0; fi < face_count; ++fi) {
        const aether_bsp_face_t *face = aether_bsp_face_at(bsp, fi);
        if (!face) continue;
        u8 r = 180, g = 180, b = 180;
        if (face->light_offset >= 0 &&
            (u32)face->light_offset + 3u <= lighting_sz) {
            const u8 *s = lighting + face->light_offset;
            r = s[0]; g = s[1]; b = s[2];
        }
        u32 col = fi % cols;
        u32 row = fi / cols;
        u32 x0 = (col * W) / cols;
        u32 y0 = (row * H) / rows;
        u32 x1 = ((col + 1u) * W) / cols;
        u32 y1 = ((row + 1u) * H) / rows;
        for (u32 y = y0; y < y1; ++y) {
            for (u32 x = x0; x < x1; ++x) {
                u32 i = (y * W + x) * 4u;
                buf[i+0] = r; buf[i+1] = g; buf[i+2] = b; buf[i+3] = 255;
            }
        }
    }

    free(lm->rgba);
    free(lm->base_rgba); lm->base_rgba = NULL;
    lm->rgba = buf;
    lm->width = W;
    lm->height = H;
    lm->stub = false;
    lm->face_tiles = tiles;
    lm->enabled = true;
    (void)aether_lightmap_capture_base(lm);
    /* Prefer texinfo-based LUV unpack when present; else face-run tiling. */
    aether_result_t uv;
    if (aether_bsp_texinfo_count(bsp) > 0 && mesh->face_ranges && mesh->face_count > 0)
        uv = aether_lightmap_unpack_uvs_from_bsp(lm, bsp, mesh);
    else
        uv = aether_lightmap_assign_mesh_uvs(lm, mesh);
    aether_log(AETHER_LOG_INFO, "lightmap",
               "baked from BSP LIGHTING lump (%u bytes, %u faces) stub=0 texinfo_uv=%d",
               lighting_sz, face_count,
               (aether_bsp_texinfo_count(bsp) > 0 && mesh->face_ranges) ? 1 : 0);
    return uv;
}

aether_result_t aether_lightmap_unpack_uvs_from_bsp(aether_lightmap_t *lm,
                                                    const aether_bsp_t *bsp,
                                                    aether_mesh_t *mesh) {
    if (!lm || !bsp || !mesh || !mesh->vertices || mesh->vertex_count == 0)
        return AETHER_ERR_INVALID_ARG;
    if (!mesh->face_ranges || mesh->face_count == 0)
        return aether_lightmap_assign_mesh_uvs(lm, mesh);

    u32 face_count = mesh->face_count;
    if (face_count > aether_bsp_face_count(bsp))
        face_count = aether_bsp_face_count(bsp);
    if (face_count == 0) return aether_lightmap_assign_mesh_uvs(lm, mesh);

    u32 tiles = face_count;
    u32 cols = ceil_sqrt_u32(tiles);
    u32 rows = (tiles + cols - 1u) / cols;
    if (rows == 0) rows = 1;
    f32 pad = 0.02f;
    u32 unpacked = 0;

    for (u32 fi = 0; fi < face_count; ++fi) {
        const aether_mesh_face_range_t *fr = &mesh->face_ranges[fi];
        if (fr->vertex_count == 0) continue;
        const aether_bsp_face_t *face = aether_bsp_face_at(bsp, fi);
        const aether_bsp_texinfo_t *ti =
            face ? aether_bsp_texinfo_at(bsp, face->texinfo) : NULL;

        f32 su[4] = {1,0,0,0}, sv[4] = {0,1,0,0};
        if (ti) {
            for (int k = 0; k < 4; ++k) {
                su[k] = ti->vecs[0][k];
                sv[k] = ti->vecs[1][k];
            }
        }

        f32 umin = 1e30f, umax = -1e30f, vmin = 1e30f, vmax = -1e30f;
        for (u32 v = 0; v < fr->vertex_count; ++v) {
            const aether_mesh_vertex_t *vtx = &mesh->vertices[fr->first_vertex + v];
            f32 u = su[0]*vtx->x + su[1]*vtx->y + su[2]*vtx->z + su[3];
            f32 vv = sv[0]*vtx->x + sv[1]*vtx->y + sv[2]*vtx->z + sv[3];
            if (u < umin) umin = u;
            if (u > umax) umax = u;
            if (vv < vmin) vmin = vv;
            if (vv > vmax) vmax = vv;
        }
        f32 ud = umax - umin; if (ud < 1e-3f) ud = 1.f;
        f32 vd = vmax - vmin; if (vd < 1e-3f) vd = 1.f;

        u32 col = fi % cols;
        u32 row = fi / cols;
        f32 u0 = ((f32)col + pad) / (f32)cols;
        f32 v0 = ((f32)row + pad) / (f32)rows;
        f32 du = (1.f - 2.f * pad) / (f32)cols;
        f32 dv = (1.f - 2.f * pad) / (f32)rows;

        for (u32 v = 0; v < fr->vertex_count; ++v) {
            aether_mesh_vertex_t *vtx = &mesh->vertices[fr->first_vertex + v];
            f32 u = (su[0]*vtx->x + su[1]*vtx->y + su[2]*vtx->z + su[3] - umin) / ud;
            f32 vv = (sv[0]*vtx->x + sv[1]*vtx->y + sv[2]*vtx->z + sv[3] - vmin) / vd;
            vtx->lu = u0 + clampf(u, 0.f, 1.f) * du;
            vtx->lv = v0 + clampf(vv, 0.f, 1.f) * dv;
        }
        ++unpacked;
    }

    if (lm->face_tiles == 0) lm->face_tiles = tiles;
    aether_log(AETHER_LOG_INFO, "lightmap",
               "unpacked LUV from texinfo for %u/%u faces (%ux%u tiles)",
               unpacked, face_count, cols, rows);
    return unpacked > 0 ? AETHER_OK : aether_lightmap_assign_mesh_uvs(lm, mesh);
}

void aether_lightstyles_init(aether_lightstyles_t *ls) {
    if (!ls) return;
    memset(ls, 0, sizeof(*ls));
    /* GoldSrc defaults: style 0 = "m" (mid/full), a few classic flickers. */
    aether_str_copy(ls->strings[0], AETHER_LIGHTSTYLE_LEN, "m");
    aether_str_copy(ls->strings[1], AETHER_LIGHTSTYLE_LEN, "mmnmmommommnonmmonqnmmo");
    aether_str_copy(ls->strings[2], AETHER_LIGHTSTYLE_LEN, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");
    aether_str_copy(ls->strings[3], AETHER_LIGHTSTYLE_LEN, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");
    aether_str_copy(ls->strings[4], AETHER_LIGHTSTYLE_LEN, "mamamamamama");
    ls->count = 5;
    for (u32 i = 0; i < AETHER_MAX_LIGHTSTYLES; ++i) ls->values[i] = 1.f;
    aether_lightstyles_update(ls, 0.f);
}

void aether_lightstyles_set(aether_lightstyles_t *ls, u32 index, const char *pattern) {
    if (!ls || index >= AETHER_MAX_LIGHTSTYLES || !pattern) return;
    aether_str_copy(ls->strings[index], AETHER_LIGHTSTYLE_LEN, pattern);
    if (index >= ls->count) ls->count = index + 1;
}

void aether_lightstyles_update(aether_lightstyles_t *ls, f32 time) {
    if (!ls) return;
    ls->time = time;
    /* ~10 Hz like GoldSrc (cl_lightstyle / 10). */
    f32 frame = time * 10.f;
    for (u32 i = 0; i < AETHER_MAX_LIGHTSTYLES; ++i) {
        const char *s = ls->strings[i];
        size_t len = strlen(s);
        if (len == 0) { ls->values[i] = 1.f; continue; }
        u32 k = (u32)frame % (u32)len;
        char c = s[k];
        if (c < 'a') c = 'a';
        if (c > 'z') c = 'z';
        ls->values[i] = (f32)(c - 'a') / 25.f; /* a=0 .. z=1 */
    }
}

f32 aether_lightstyles_value(const aether_lightstyles_t *ls, u32 index) {
    if (!ls || index >= AETHER_MAX_LIGHTSTYLES) return 1.f;
    return ls->values[index];
}

aether_result_t aether_lightmap_apply_style(aether_lightmap_t *lm,
                                            const aether_lightstyles_t *ls,
                                            u32 style_index) {
    if (!lm || !lm->rgba || !ls || lm->width == 0 || lm->height == 0)
        return AETHER_ERR_INVALID_ARG;
    f32 v = aether_lightstyles_value(ls, style_index);
    if (v < 0.f) v = 0.f;
    if (v > 1.f) v = 1.f;
    /* Soft modulate: keep a floor so the atlas never goes fully black. */
    f32 scale = 0.25f + 0.75f * v;
    u32 n = lm->width * lm->height;
    for (u32 i = 0; i < n; ++i) {
        u32 idx = i * 4u;
        lm->rgba[idx+0] = (u8)((f32)lm->rgba[idx+0] * scale);
        lm->rgba[idx+1] = (u8)((f32)lm->rgba[idx+1] * scale);
        lm->rgba[idx+2] = (u8)((f32)lm->rgba[idx+2] * scale);
    }
    return AETHER_OK;
}


aether_result_t aether_lightmap_capture_base(aether_lightmap_t *lm) {
    if (!lm || !lm->rgba || lm->width == 0 || lm->height == 0)
        return AETHER_ERR_INVALID_ARG;
    u32 bytes = lm->width * lm->height * 4u;
    u8 *nb = (u8 *)realloc(lm->base_rgba, bytes);
    if (!nb) return AETHER_ERR_OUT_OF_MEM;
    lm->base_rgba = nb;
    memcpy(lm->base_rgba, lm->rgba, bytes);
    return AETHER_OK;
}

bool aether_lightmap_has_base(const aether_lightmap_t *lm) {
    return lm && lm->base_rgba && lm->rgba && lm->width > 0 && lm->height > 0;
}

aether_result_t aether_lightmap_apply_style_pingpong(aether_lightmap_t *lm,
                                                     const aether_lightstyles_t *ls,
                                                     u32 style_index) {
    if (!lm || !ls || !lm->rgba || lm->width == 0 || lm->height == 0)
        return AETHER_ERR_INVALID_ARG;
    if (!lm->base_rgba) {
        aether_result_t r = aether_lightmap_capture_base(lm);
        if (r != AETHER_OK) return r;
    }
    u32 bytes = lm->width * lm->height * 4u;
    memcpy(lm->rgba, lm->base_rgba, bytes);
    return aether_lightmap_apply_style(lm, ls, style_index);
}

f32 aether_lightstyles_gpu_scale(f32 value) {
    if (value < 0.f) value = 0.f;
    if (value > 1.f) value = 1.f;
    return 0.25f + 0.75f * value;
}

u32 aether_lightstyles_fill_gpu_weights(const aether_lightstyles_t *ls,
                                        aether_lightstyle_gpu_t *out) {
    if (!out) return 0;
    memset(out, 0, sizeof(*out));
    if (!ls) { out->weights[0] = 1.f; out->count = 1; return 4 + AETHER_MAX_LIGHTSTYLES; }
    out->count = ls->count > 0 ? ls->count : 1;
    out->time = ls->time;
    for (u32 i = 0; i < AETHER_MAX_LIGHTSTYLES; ++i) {
        out->weights[i] = aether_lightstyles_gpu_scale(ls->values[i]);
    }
    return 4 + AETHER_MAX_LIGHTSTYLES; /* floats conceptually */
}


u32 aether_lightmap_fill_face_style_indices(const struct aether_mesh *mesh,
                                            u8 *out_indices, u32 max_faces) {
    if (!mesh || !out_indices || !mesh->face_ranges || mesh->face_count == 0) return 0;
    u32 n = mesh->face_count;
    if (n > max_faces) n = max_faces;
    for (u32 i = 0; i < n; ++i) {
        u8 s = mesh->face_ranges[i].styles[0];
        out_indices[i] = (s == 255) ? 0 : s;
    }
    return n;
}

u32 aether_lightmap_fill_face_style_weights(const struct aether_mesh *mesh,
                                            const aether_lightstyles_t *ls,
                                            f32 *out_weights, u32 max_faces) {
    if (!mesh || !ls || !out_weights || !mesh->face_ranges || mesh->face_count == 0) return 0;
    u32 n = mesh->face_count;
    if (n > max_faces) n = max_faces;
    for (u32 i = 0; i < n; ++i) {
        u8 s = mesh->face_ranges[i].styles[0];
        if (s == 255) s = 0;
        f32 v = aether_lightstyles_value(ls, s);
        out_weights[i] = aether_lightstyles_gpu_scale(v);
    }
    return n;
}

u32 aether_lightmap_fill_face_style_blend(const struct aether_mesh *mesh,
                                          const aether_lightstyles_t *ls,
                                          f32 *out_weights4, u32 max_faces) {
    if (!mesh || !ls || !out_weights4 || !mesh->face_ranges || mesh->face_count == 0) return 0;
    u32 n = mesh->face_count;
    if (n > max_faces) n = max_faces;
    for (u32 i = 0; i < n; ++i) {
        for (u32 s = 0; s < 4; ++s) {
            u8 sty = mesh->face_ranges[i].styles[s];
            if (sty == 255) {
                out_weights4[i * 4u + s] = 0.f;
            } else {
                f32 v = aether_lightstyles_value(ls, sty);
                out_weights4[i * 4u + s] = aether_lightstyles_gpu_scale(v);
            }
        }
    }
    return n;
}

u32 aether_lightmap_fill_face_style_blend_scalar(const struct aether_mesh *mesh,
                                                 const aether_lightstyles_t *ls,
                                                 f32 *out_weights, u32 max_faces) {
    if (!mesh || !ls || !out_weights || !mesh->face_ranges || mesh->face_count == 0) return 0;
    u32 n = mesh->face_count;
    if (n > max_faces) n = max_faces;
    for (u32 i = 0; i < n; ++i) {
        f32 sum = 0.f;
        u32 active = 0;
        for (u32 s = 0; s < 4; ++s) {
            u8 sty = mesh->face_ranges[i].styles[s];
            if (sty == 255) continue;
            sum += aether_lightstyles_gpu_scale(aether_lightstyles_value(ls, sty));
            active++;
        }
        out_weights[i] = active ? (sum / (f32)active) : aether_lightstyles_gpu_scale(1.f);
    }
    return n;
}

void aether_lightmap_sample_style_blend(const f32 weights4[4],
                                        const f32 base_rgb[3],
                                        f32 out_rgb[3]) {
    if (!out_rgb) return;
    if (!base_rgb) { out_rgb[0]=out_rgb[1]=out_rgb[2]=0.f; return; }
    f32 wsum = 0.f;
    if (weights4) {
        for (int i = 0; i < 4; ++i) {
            f32 w = weights4[i];
            if (w > 0.f) wsum += w;
        }
    }
    f32 scale = (wsum > 1e-6f) ? wsum : 1.f;
    /* GoldSrc-ish: styles accumulate; clamp soft. */
    if (scale > 4.f) scale = 4.f;
    out_rgb[0] = base_rgb[0] * scale;
    out_rgb[1] = base_rgb[1] * scale;
    out_rgb[2] = base_rgb[2] * scale;
}

u32 aether_lightmap_fill_style_blend_ubo(const struct aether_mesh *mesh,
                                         const aether_lightstyles_t *ls,
                                         f32 *out, u32 max_floats) {
    if (!out || max_floats < 8) return 0;
    f32 w4[256 * 4];
    u32 faces = aether_lightmap_fill_face_style_blend(mesh, ls, w4, 256);
    if (faces == 0) {
        out[0]=0; out[1]=0; out[2]=0; out[3]=0;
        return 4;
    }
    u32 need = 4u + faces * 4u;
    if (need > max_floats) {
        faces = (max_floats - 4u) / 4u;
        need = 4u + faces * 4u;
    }
    out[0] = (f32)faces; out[1]=0; out[2]=0; out[3]=0;
    memcpy(out + 4, w4, faces * 4u * sizeof(f32));
    return need;
}
