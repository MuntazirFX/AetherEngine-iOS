#include "AetherDecal.h"
#include <math.h>
#include <string.h>

aether_result_t aether_decals_init(aether_decals_t *d) {
    if (!d) return AETHER_ERR_INVALID_ARG;
    memset(d, 0, sizeof(*d));
    return AETHER_OK;
}

aether_result_t aether_decals_add(aether_decals_t *d, const f32 p[3], const f32 n[3],
                                  f32 size, f32 life) {
    if (!d || !p || !n || size <= 0 || life < 0) return AETHER_ERR_INVALID_ARG;
    u32 slot = d->count < AETHER_MAX_DECALS ? d->count : (d->count % AETHER_MAX_DECALS);
    aether_decal_t *x = &d->items[slot];
    memcpy(x->position, p, sizeof x->position);
    memcpy(x->normal, n, sizeof x->normal);
    x->size = size; x->life = life; x->age = 0; x->active = true;
    if (d->count < AETHER_MAX_DECALS) d->count++;
    return AETHER_OK;
}

void aether_decals_update(aether_decals_t *d, f32 dt) {
    if (!d) return;
    for (u32 i = 0; i < d->count; i++) {
        aether_decal_t *x = &d->items[i];
        if (!x->active) continue;
        x->age += dt;
        if (x->life > 0 && x->age >= x->life) x->active = false;
    }
}

void aether_decals_clear(aether_decals_t *d) {
    if (d) memset(d, 0, sizeof(*d));
}

u32 aether_decals_active_count(const aether_decals_t *d) {
    if (!d) return 0;
    u32 n = 0;
    for (u32 i = 0; i < d->count; ++i) if (d->items[i].active) n++;
    return n;
}

u32 aether_decals_copy_render(const aether_decals_t *d,
                              aether_decal_vertex_t *out, u32 max_out) {
    if (!d || !out || max_out == 0) return 0;
    u32 w = 0;
    for (u32 i = 0; i < d->count && w < max_out; ++i) {
        const aether_decal_t *x = &d->items[i];
        if (!x->active) continue;
        f32 fade = 1.f;
        if (x->life > 0.f) {
            fade = 1.f - (x->age / x->life);
            if (fade < 0.f) fade = 0.f;
        }
        out[w].x = x->position[0]; out[w].y = x->position[1]; out[w].z = x->position[2];
        out[w].nx = x->normal[0]; out[w].ny = x->normal[1]; out[w].nz = x->normal[2];
        out[w].size = x->size;
        out[w].fade = fade;
        w++;
    }
    return w;
}

static void orthonormal_basis(f32 nx, f32 ny, f32 nz,
                              f32 *tx, f32 *ty, f32 *tz,
                              f32 *bx, f32 *by, f32 *bz) {
    f32 ax = fabsf(nx), ay = fabsf(ny), az = fabsf(nz);
    if (ax < ay && ax < az) { *tx = 0.f; *ty = -nz; *tz = ny; }
    else if (ay < az)       { *tx = -nz; *ty = 0.f; *tz = nx; }
    else                    { *tx = -ny; *ty = nx;  *tz = 0.f; }
    f32 tlen = sqrtf((*tx)*(*tx) + (*ty)*(*ty) + (*tz)*(*tz));
    if (tlen < 1e-6f) { *tx = 1.f; *ty = 0.f; *tz = 0.f; tlen = 1.f; }
    *tx /= tlen; *ty /= tlen; *tz /= tlen;
    *bx = ny * (*tz) - nz * (*ty);
    *by = nz * (*tx) - nx * (*tz);
    *bz = nx * (*ty) - ny * (*tx);
}

u32 aether_decals_copy_quads(const aether_decals_t *d,
                             aether_decal_quad_vertex_t *out, u32 max_out) {
    if (!d || !out || max_out < 6) return 0;
    u32 w = 0;
    for (u32 i = 0; i < d->count && w + 6 <= max_out; ++i) {
        const aether_decal_t *x = &d->items[i];
        if (!x->active) continue;
        f32 fade = 1.f;
        if (x->life > 0.f) {
            fade = 1.f - (x->age / x->life);
            if (fade < 0.f) fade = 0.f;
        }
        f32 nx = x->normal[0], ny = x->normal[1], nz = x->normal[2];
        f32 nlen = sqrtf(nx*nx + ny*ny + nz*nz);
        if (nlen < 1e-6f) { nx = 0.f; ny = 0.f; nz = 1.f; }
        else { nx /= nlen; ny /= nlen; nz /= nlen; }

        f32 tx, ty, tz, bx, by, bz;
        orthonormal_basis(nx, ny, nz, &tx, &ty, &tz, &bx, &by, &bz);
        f32 hs = x->size * 0.5f;
        /* Slightly offset along normal to avoid z-fight. */
        f32 cx = x->position[0] + nx * 0.5f;
        f32 cy = x->position[1] + ny * 0.5f;
        f32 cz = x->position[2] + nz * 0.5f;

        f32 corners[4][3];
        f32 uvs[4][2] = {{0,0},{1,0},{1,1},{0,1}};
        for (int c = 0; c < 4; ++c) {
            f32 su = (c == 0 || c == 3) ? -hs : hs;
            f32 sv = (c < 2) ? -hs : hs;
            corners[c][0] = cx + tx * su + bx * sv;
            corners[c][1] = cy + ty * su + by * sv;
            corners[c][2] = cz + tz * su + bz * sv;
        }
        /* Two triangles: 0,1,2 and 0,2,3 */
        int idx[6] = {0,1,2, 0,2,3};
        for (int k = 0; k < 6; ++k) {
            int c = idx[k];
            aether_decal_quad_vertex_t *v = &out[w++];
            v->x = corners[c][0]; v->y = corners[c][1]; v->z = corners[c][2];
            v->u = uvs[c][0]; v->v = uvs[c][1];
            v->fade = fade;
            v->r = 0.85f; v->g = 0.25f; v->b = 0.15f; v->a = 0.75f * fade;
        }
    }
    return w;
}
