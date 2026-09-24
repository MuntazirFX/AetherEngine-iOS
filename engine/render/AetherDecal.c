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


typedef struct { f32 u, v; f32 x, y, z; } aether_decal_clip_vert_t;

/* Clip polygon against one half-plane: keep side where ax*u + ay*v + c >= 0. */
static u32 clip_poly_halfplane(const aether_decal_clip_vert_t *in, u32 nin,
                               aether_decal_clip_vert_t *out, u32 max_out,
                               f32 ax, f32 ay, f32 c) {
    if (nin == 0 || !out || max_out == 0) return 0;
    u32 w = 0;
    for (u32 i = 0; i < nin; ++i) {
        const aether_decal_clip_vert_t *A = &in[i];
        const aether_decal_clip_vert_t *B = &in[(i + 1) % nin];
        f32 da = ax * A->u + ay * A->v + c;
        f32 db = ax * B->u + ay * B->v + c;
        int ain = da >= -1e-5f;
        int bin = db >= -1e-5f;
        if (ain && bin) {
            if (w < max_out) out[w++] = *B;
        } else if (ain && !bin) {
            /* Leaving: emit intersection */
            f32 t = da / (da - db);
            if (t < 0.f) t = 0.f;
            if (t > 1.f) t = 1.f;
            aether_decal_clip_vert_t I;
            I.u = A->u + (B->u - A->u) * t;
            I.v = A->v + (B->v - A->v) * t;
            I.x = A->x + (B->x - A->x) * t;
            I.y = A->y + (B->y - A->y) * t;
            I.z = A->z + (B->z - A->z) * t;
            if (w < max_out) out[w++] = I;
        } else if (!ain && bin) {
            f32 t = da / (da - db);
            if (t < 0.f) t = 0.f;
            if (t > 1.f) t = 1.f;
            aether_decal_clip_vert_t I;
            I.u = A->u + (B->u - A->u) * t;
            I.v = A->v + (B->v - A->v) * t;
            I.x = A->x + (B->x - A->x) * t;
            I.y = A->y + (B->y - A->y) * t;
            I.z = A->z + (B->z - A->z) * t;
            if (w < max_out) out[w++] = I;
            if (w < max_out) out[w++] = *B;
        }
    }
    return w;
}

u32 aether_decals_project_onto_mesh(const aether_decals_t *d,
                                    const aether_mesh_t *mesh,
                                    aether_decal_quad_vertex_t *out, u32 max_out) {
    if (!d || !mesh || !mesh->vertices || !mesh->indices || !out || max_out < 3)
        return 0;
    u32 w = 0;
    u32 tris = mesh->index_count / 3u;
    aether_decal_clip_vert_t poly[16], tmp[16];

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

            f32 plane_err = 0.f;
            for (int k = 0; k < 3; ++k) {
                f32 pd = nx*vs[k]->x + ny*vs[k]->y + nz*vs[k]->z + d0;
                if (fabsf(pd) > plane_err) plane_err = fabsf(pd);
            }
            if (plane_err > 8.f) continue;

            /* Build tangent-space triangle */
            u32 npoly = 3;
            for (int k = 0; k < 3; ++k) {
                f32 dx = vs[k]->x - dec->position[0];
                f32 dy = vs[k]->y - dec->position[1];
                f32 dz = vs[k]->z - dec->position[2];
                poly[k].u = dx*tx + dy*ty + dz*tz;
                poly[k].v = dx*bx + dy*by + dz*bz;
                poly[k].x = vs[k]->x;
                poly[k].y = vs[k]->y;
                poly[k].z = vs[k]->z;
            }

            /* Sutherland–Hodgman clip against square [-hs,hs]^2 */
            npoly = clip_poly_halfplane(poly, npoly, tmp, 16, -1.f, 0.f, hs); /* u <= +hs */
            if (npoly < 3) continue;
            npoly = clip_poly_halfplane(tmp, npoly, poly, 16,  1.f, 0.f, hs); /* u >= -hs */
            if (npoly < 3) continue;
            npoly = clip_poly_halfplane(poly, npoly, tmp, 16,  0.f,-1.f, hs); /* v <= +hs */
            if (npoly < 3) continue;
            npoly = clip_poly_halfplane(tmp, npoly, poly, 16,  0.f, 1.f, hs); /* v >= -hs */
            if (npoly < 3) continue;

            /* Fan triangulate clipped polygon */
            f32 eps = 0.4f;
            for (u32 k = 1; k + 1 < npoly && w + 3 <= max_out; ++k) {
                aether_decal_clip_vert_t *cv[3] = { &poly[0], &poly[k], &poly[k+1] };
                for (int j = 0; j < 3; ++j) {
                    aether_decal_quad_vertex_t *o = &out[w++];
                    o->x = cv[j]->x + nx * eps;
                    o->y = cv[j]->y + ny * eps;
                    o->z = cv[j]->z + nz * eps;
                    o->u = 0.5f + cv[j]->u / (hs * 2.f);
                    o->v = 0.5f + cv[j]->v / (hs * 2.f);
                    o->fade = fade;
                    o->r = 0.9f; o->g = 0.2f; o->b = 0.15f; o->a = 0.8f * fade;
                }
            }
        }
    }
    return w;
}

u32 aether_decals_clip_to_world(const aether_decals_t *d,
                                const aether_mesh_t *mesh,
                                aether_decal_quad_vertex_t *out, u32 max_out) {
    return aether_decals_project_onto_mesh(d, mesh, out, max_out);
}
