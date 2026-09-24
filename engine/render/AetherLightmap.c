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
    lm->rgba = buf;
    lm->width = width;
    lm->height = height;
    lm->stub = true;
    lm->face_tiles = tiles;
    lm->enabled = true;
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
