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

#include "../bsp/AetherBSPGeometry.h"

u32 aether_decals_project_onto_mesh(const aether_decals_t *d,
                                    const aether_mesh_t *mesh,
                                    aether_decal_quad_vertex_t *out, u32 max_out) {
    if (!d || !mesh || !mesh->vertices || !mesh->indices || !out || max_out < 3)
        return 0;
    u32 w = 0;
    u32 tris = mesh->index_count / 3u;
    for (u32 di = 0; di < d->count; ++di) {
        const aether_decal_t *dec = &d->items[di];
        if (!dec->active) continue;
        f32 fade = 1.f;
        if (dec->life > 0.f) {
            fade = 1.f - (dec->age / dec->life);
            if (fade < 0.f) fade = 0.f;
        }
        f32 nx = dec->normal[0], ny = dec->normal[1], nz = dec->normal[2];
        f32 nlen = sqrtf(nx*nx + ny*ny + nz*nz);
        if (nlen < 1e-6f) { nx = 0; ny = 0; nz = 1; }
        else { nx /= nlen; ny /= nlen; nz /= nlen; }
        f32 hs = dec->size * 0.5f;
        if (hs < 1.f) hs = 1.f;

        f32 tx, ty, tz, bx, by, bz;
        orthonormal_basis(nx, ny, nz, &tx, &ty, &tz, &bx, &by, &bz);

        /* Plane distance of decal origin along normal — used for face coplanarity. */
        f32 d0 = -(nx * dec->position[0] + ny * dec->position[1] + nz * dec->position[2]);

        for (u32 t = 0; t < tris && w + 3 <= max_out; ++t) {
            u32 i0 = mesh->indices[t*3+0];
            u32 i1 = mesh->indices[t*3+1];
            u32 i2 = mesh->indices[t*3+2];
            if (i0 >= mesh->vertex_count || i1 >= mesh->vertex_count || i2 >= mesh->vertex_count)
                continue;
            const aether_mesh_vertex_t *vs[3] = {
                &mesh->vertices[i0], &mesh->vertices[i1], &mesh->vertices[i2]
            };
            f32 fnx = (vs[0]->nx + vs[1]->nx + vs[2]->nx) * (1.f/3.f);
            f32 fny = (vs[0]->ny + vs[1]->ny + vs[2]->ny) * (1.f/3.f);
            f32 fnz = (vs[0]->nz + vs[1]->nz + vs[2]->nz) * (1.f/3.f);
            f32 ndot = fnx*nx + fny*ny + fnz*nz;
            if (ndot < 0.25f) continue;

            /* Coplanar-ish with decal plane? */
            f32 plane_err = 0.f;
            for (int k = 0; k < 3; ++k) {
                f32 pd = nx*vs[k]->x + ny*vs[k]->y + nz*vs[k]->z + d0;
                if (fabsf(pd) > plane_err) plane_err = fabsf(pd);
            }
            if (plane_err > 8.f) continue;

            f32 local[3][2];
            f32 umin = 1e9f, umax = -1e9f, vmin = 1e9f, vmax = -1e9f;
            for (int k = 0; k < 3; ++k) {
                f32 dx = vs[k]->x - dec->position[0];
                f32 dy = vs[k]->y - dec->position[1];
                f32 dz = vs[k]->z - dec->position[2];
                local[k][0] = dx*tx + dy*ty + dz*tz;
                local[k][1] = dx*bx + dy*by + dz*bz;
                if (local[k][0] < umin) umin = local[k][0];
                if (local[k][0] > umax) umax = local[k][0];
                if (local[k][1] < vmin) vmin = local[k][1];
                if (local[k][1] > vmax) vmax = local[k][1];
            }
            /* Tangent-space AABB overlap with decal square [-hs, hs]^2 */
            if (umax < -hs || umin > hs || vmax < -hs || vmin > hs) continue;

            f32 eps = 0.4f;
            for (int k = 0; k < 3; ++k) {
                aether_decal_quad_vertex_t *o = &out[w++];
                o->x = vs[k]->x + nx * eps;
                o->y = vs[k]->y + ny * eps;
                o->z = vs[k]->z + nz * eps;
                o->u = 0.5f + local[k][0] / (hs * 2.f);
                o->v = 0.5f + local[k][1] / (hs * 2.f);
                o->fade = fade;
                o->r = 0.9f; o->g = 0.2f; o->b = 0.15f; o->a = 0.8f * fade;
            }
        }
    }
    return w;
}
