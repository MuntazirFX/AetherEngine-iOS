#include "AetherDynLight.h"
#include "AetherLightmap.h"
#include "../bsp/AetherBSPGeometry.h"
#include <math.h>
#include <string.h>

aether_result_t aether_dyn_lights_init(aether_dyn_lights_t *dl) {
    if (!dl) return AETHER_ERR_INVALID_ARG;
    memset(dl, 0, sizeof(*dl));
    return AETHER_OK;
}
void aether_dyn_lights_clear(aether_dyn_lights_t *dl) {
    if (dl) memset(dl, 0, sizeof(*dl));
}
aether_result_t aether_dyn_lights_add(aether_dyn_lights_t *dl,
                                      const f32 pos[3], const f32 color[3],
                                      f32 radius, f32 intensity) {
    if (!dl || !pos || !color || radius <= 0.f) return AETHER_ERR_INVALID_ARG;
    u32 slot = dl->count < AETHER_MAX_DYN_LIGHTS ? dl->count
                                                 : (dl->count % AETHER_MAX_DYN_LIGHTS);
    aether_dyn_light_t *L = &dl->items[slot];
    memcpy(L->position, pos, sizeof L->position);
    memcpy(L->color, color, sizeof L->color);
    L->radius = radius;
    L->intensity = intensity;
    L->active = true;
    if (dl->count < AETHER_MAX_DYN_LIGHTS) dl->count++;
    return AETHER_OK;
}
u32 aether_dyn_lights_active_count(const aether_dyn_lights_t *dl) {
    if (!dl) return 0;
    u32 n = 0;
    for (u32 i = 0; i < dl->count; ++i) if (dl->items[i].active) n++;
    return n;
}
u32 aether_dyn_lights_copy_render(const aether_dyn_lights_t *dl,
                                  aether_dyn_light_vertex_t *out, u32 max_out) {
    if (!dl || !out || max_out == 0) return 0;
    u32 w = 0;
    for (u32 i = 0; i < dl->count && w < max_out; ++i) {
        const aether_dyn_light_t *L = &dl->items[i];
        if (!L->active) continue;
        out[w].x = L->position[0]; out[w].y = L->position[1]; out[w].z = L->position[2];
        out[w].radius = L->radius;
        out[w].r = L->color[0]; out[w].g = L->color[1]; out[w].b = L->color[2];
        out[w].intensity = L->intensity;
        w++;
    }
    return w;
}

void aether_dyn_lights_sample_rgb(const aether_dyn_lights_t *dl,
                                  f32 x, f32 y, f32 z,
                                  f32 out_rgb[3]) {
    if (!out_rgb) return;
    out_rgb[0] = out_rgb[1] = out_rgb[2] = 0.f;
    if (!dl) return;
    for (u32 i = 0; i < dl->count; ++i) {
        const aether_dyn_light_t *L = &dl->items[i];
        if (!L->active || L->radius <= 0.f) continue;
        f32 dx = x - L->position[0];
        f32 dy = y - L->position[1];
        f32 dz = z - L->position[2];
        f32 dist = sqrtf(dx*dx + dy*dy + dz*dz);
        if (dist >= L->radius) continue;
        f32 attn = 1.f - (dist / L->radius);
        attn *= attn;
        f32 s = attn * L->intensity;
        out_rgb[0] += L->color[0] * s;
        out_rgb[1] += L->color[1] * s;
        out_rgb[2] += L->color[2] * s;
    }
}

u32 aether_dyn_lights_apply_mesh_tint(const aether_dyn_lights_t *dl,
                                      const aether_mesh_t *mesh,
                                      f32 *out_rgb, u32 max_floats) {
    if (!dl || !mesh || !mesh->vertices || !out_rgb) return 0;
    u32 n = mesh->vertex_count;
    if (n * 3u > max_floats) n = max_floats / 3u;
    for (u32 i = 0; i < n; ++i) {
        f32 rgb[3];
        const aether_mesh_vertex_t *v = &mesh->vertices[i];
        aether_dyn_lights_sample_rgb(dl, v->x, v->y, v->z, rgb);
        /* Ambient base so unlit verts stay visible. */
        out_rgb[i*3+0] = 0.35f + rgb[0];
        out_rgb[i*3+1] = 0.35f + rgb[1];
        out_rgb[i*3+2] = 0.35f + rgb[2];
        if (out_rgb[i*3+0] > 2.f) out_rgb[i*3+0] = 2.f;
        if (out_rgb[i*3+1] > 2.f) out_rgb[i*3+1] = 2.f;
        if (out_rgb[i*3+2] > 2.f) out_rgb[i*3+2] = 2.f;
    }
    return n;
}

aether_result_t aether_dyn_lights_modulate_lightmap(const aether_dyn_lights_t *dl,
                                                    aether_lightmap_t *lm) {
    if (!dl || !lm || !lm->rgba || lm->width == 0 || lm->height == 0)
        return AETHER_ERR_INVALID_ARG;
    /* Soft radial boost at atlas center for each active light (visual stub). */
    u32 W = lm->width, H = lm->height;
    for (u32 i = 0; i < dl->count; ++i) {
        const aether_dyn_light_t *L = &dl->items[i];
        if (!L->active) continue;
        f32 cx = 0.5f + 0.15f * sinf(L->position[0] * 0.01f);
        f32 cy = 0.5f + 0.15f * cosf(L->position[1] * 0.01f);
        i32 rad = (i32)(fminf(W, H) * 0.25f);
        if (rad < 4) rad = 4;
        i32 x0 = (i32)(cx * (f32)W);
        i32 y0 = (i32)(cy * (f32)H);
        for (i32 y = y0 - rad; y <= y0 + rad; ++y) {
            if (y < 0 || (u32)y >= H) continue;
            for (i32 x = x0 - rad; x <= x0 + rad; ++x) {
                if (x < 0 || (u32)x >= W) continue;
                f32 dx = (f32)(x - x0) / (f32)rad;
                f32 dy = (f32)(y - y0) / (f32)rad;
                f32 d = sqrtf(dx*dx + dy*dy);
                if (d > 1.f) continue;
                f32 a = (1.f - d) * (1.f - d) * L->intensity * 0.35f;
                u32 idx = ((u32)y * W + (u32)x) * 4u;
                f32 r = lm->rgba[idx+0] + L->color[0] * a * 255.f;
                f32 g = lm->rgba[idx+1] + L->color[1] * a * 255.f;
                f32 b = lm->rgba[idx+2] + L->color[2] * a * 255.f;
                lm->rgba[idx+0] = (u8)(r > 255.f ? 255.f : r);
                lm->rgba[idx+1] = (u8)(g > 255.f ? 255.f : g);
                lm->rgba[idx+2] = (u8)(b > 255.f ? 255.f : b);
            }
        }
    }
    return AETHER_OK;
}

u32 aether_dyn_lights_fill_ubo(const aether_dyn_lights_t *dl, aether_dyn_light_ubo_t *ubo) {
    if (!ubo) return 0;
    memset(ubo, 0, sizeof(*ubo));
    if (!dl) return 0;
    u32 n = aether_dyn_lights_copy_render(dl, ubo->lights, AETHER_DYN_LIGHT_UBO_MAX);
    ubo->count = n;
    return n;
}


u32 aether_dyn_lights_fill_array(const aether_dyn_lights_t *dl,
                                 f32 *out, u32 max_floats) {
    if (!out || max_floats < 4) return 0;
    aether_dyn_light_ubo_t ubo;
    u32 n = aether_dyn_lights_fill_ubo(dl, &ubo);
    /* Need 4 + n*8 floats */
    u32 need = 4u + n * 8u;
    if (need > max_floats) {
        n = (max_floats - 4u) / 8u;
        need = 4u + n * 8u;
    }
    out[0] = (f32)n; out[1] = 0; out[2] = 0; out[3] = 0;
    for (u32 i = 0; i < n; ++i) {
        f32 *d = out + 4u + i * 8u;
        d[0] = ubo.lights[i].x; d[1] = ubo.lights[i].y; d[2] = ubo.lights[i].z;
        d[3] = ubo.lights[i].radius;
        d[4] = ubo.lights[i].r; d[5] = ubo.lights[i].g; d[6] = ubo.lights[i].b;
        d[7] = ubo.lights[i].intensity;
    }
    return need;
}

void aether_dyn_lights_sample_rgb_ex(const aether_dyn_lights_t *dl,
                                     f32 x, f32 y, f32 z,
                                     f32 ambient, f32 out_rgb[3]) {
    if (!out_rgb) return;
    if (ambient < 0.f) ambient = 0.f;
    aether_dyn_lights_sample_rgb(dl, x, y, z, out_rgb);
    out_rgb[0] += ambient;
    out_rgb[1] += ambient;
    out_rgb[2] += ambient;
    /* Soft quadratic falloff polish already in sample; clamp HDR-ish. */
    for (int i = 0; i < 3; ++i) if (out_rgb[i] > 2.f) out_rgb[i] = 2.f;
}


#include "../bsp/AetherBSPVis.h"
#include "../bsp/AetherBSP.h"
#include <stdlib.h>

u32 aether_dyn_lights_cull_pvs(const aether_dyn_lights_t *dl,
                               const struct aether_bsp *bsp,
                               i32 view_leaf,
                               aether_dyn_light_ubo_t *ubo) {
    if (!ubo) return 0;
    memset(ubo, 0, sizeof(*ubo));
    if (!dl || !bsp || view_leaf < 0)
        return aether_dyn_lights_fill_ubo(dl, ubo);

    u32 leaf_count = aether_bsp_leaf_count(bsp);
    u32 face_count = aether_bsp_face_count(bsp);
    if (leaf_count == 0)
        return aether_dyn_lights_fill_ubo(dl, ubo);

    u8 *face_bits = NULL;
    if (face_count > 0) {
        face_bits = (u8 *)calloc(face_count, 1);
        if (face_bits) {
            (void)aether_bsp_vis_mark_faces(bsp, view_leaf, AETHER_BSP_VIS_USE_PVS,
                                           face_bits, face_count);
        }
    }

    u8 *leaf_vis = (u8 *)calloc(leaf_count, 1);
    if (!leaf_vis) {
        free(face_bits);
        return aether_dyn_lights_fill_ubo(dl, ubo);
    }

    u32 mark_count = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_MARKSURFACES) / sizeof(u16);
    const u8 *mark_raw = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_MARKSURFACES);

    for (u32 li = 0; li < leaf_count; ++li) {
        if (!aether_bsp_leaf_is_drawable(bsp, (i32)li)) continue;
        const aether_bsp_leaf_t *leaf = aether_bsp_leaf_at(bsp, li);
        if (!leaf) continue;
        if (!face_bits || !mark_raw || leaf->num_marksurfaces == 0 || mark_count == 0) {
            leaf_vis[li] = 1; /* no mark data → visible stub */
            continue;
        }
        int any = 0;
        for (u32 m = 0; m < leaf->num_marksurfaces; ++m) {
            u32 off = (u32)leaf->first_marksurface + m;
            if (off >= mark_count) break;
            const u8 *p = mark_raw + off * 2u;
            u16 face = (u16)((u32)p[0] | ((u32)p[1] << 8));
            if (face < face_count && face_bits[face]) { any = 1; break; }
        }
        leaf_vis[li] = (u8)(any ? 1 : 0);
    }
    if ((u32)view_leaf < leaf_count) leaf_vis[view_leaf] = 1;

    u32 packed = 0;
    for (u32 i = 0; i < dl->count && packed < AETHER_DYN_LIGHT_UBO_MAX; ++i) {
        const aether_dyn_light_t *L = &dl->items[i];
        if (!L->active) continue;
        i32 leaf = aether_bsp_find_leaf(bsp, L->position[0], L->position[1], L->position[2]);
        if (leaf < 0 || (u32)leaf >= leaf_count || !leaf_vis[leaf]) continue;
        aether_dyn_light_vertex_t *o = &ubo->lights[packed++];
        o->x = L->position[0]; o->y = L->position[1]; o->z = L->position[2];
        o->radius = L->radius;
        o->r = L->color[0]; o->g = L->color[1]; o->b = L->color[2];
        o->intensity = L->intensity;
    }
    ubo->count = packed;
    free(face_bits);
    free(leaf_vis);
    return packed;
}

u32 aether_dyn_lights_fill_array_pvs(const aether_dyn_lights_t *dl,
                                     const struct aether_bsp *bsp,
                                     i32 view_leaf,
                                     f32 *out, u32 max_floats) {
    if (!out || max_floats < 4) return 0;
    aether_dyn_light_ubo_t ubo;
    u32 n = aether_dyn_lights_cull_pvs(dl, bsp, view_leaf, &ubo);
    u32 need = 4u + n * 8u;
    if (need > max_floats) {
        n = (max_floats - 4u) / 8u;
        need = 4u + n * 8u;
    }
    out[0] = (f32)n; out[1] = 0; out[2] = 0; out[3] = 0;
    for (u32 i = 0; i < n; ++i) {
        f32 *d = out + 4u + i * 8u;
        d[0] = ubo.lights[i].x; d[1] = ubo.lights[i].y; d[2] = ubo.lights[i].z;
        d[3] = ubo.lights[i].radius;
        d[4] = ubo.lights[i].r; d[5] = ubo.lights[i].g; d[6] = ubo.lights[i].b;
        d[7] = ubo.lights[i].intensity;
    }
    return need;
}

static int sphere_aabb_overlap(f32 cx, f32 cy, f32 cz, f32 radius,
                               f32 mn0, f32 mn1, f32 mn2,
                               f32 mx0, f32 mx1, f32 mx2) {
    f32 qx = cx < mn0 ? mn0 : (cx > mx0 ? mx0 : cx);
    f32 qy = cy < mn1 ? mn1 : (cy > mx1 ? mx1 : cy);
    f32 qz = cz < mn2 ? mn2 : (cz > mx2 ? mx2 : cz);
    f32 dx = cx - qx, dy = cy - qy, dz = cz - qz;
    return (dx*dx + dy*dy + dz*dz) <= (radius * radius);
}

u32 aether_dyn_lights_cull_pvs_bleed(const aether_dyn_lights_t *dl,
                                     const struct aether_bsp *bsp,
                                     i32 view_leaf,
                                     aether_dyn_light_ubo_t *ubo) {
    if (!ubo) return 0;
    memset(ubo, 0, sizeof(*ubo));
    if (!dl || !bsp || view_leaf < 0)
        return aether_dyn_lights_fill_ubo(dl, ubo);

    u32 leaf_count = aether_bsp_leaf_count(bsp);
    u32 face_count = aether_bsp_face_count(bsp);
    if (leaf_count == 0)
        return aether_dyn_lights_fill_ubo(dl, ubo);

    u8 *face_bits = NULL;
    if (face_count > 0) {
        face_bits = (u8 *)calloc(face_count, 1);
        if (face_bits)
            (void)aether_bsp_vis_mark_faces(bsp, view_leaf, AETHER_BSP_VIS_USE_PVS,
                                           face_bits, face_count);
    }
    u8 *leaf_vis = (u8 *)calloc(leaf_count, 1);
    if (!leaf_vis) {
        free(face_bits);
        return aether_dyn_lights_fill_ubo(dl, ubo);
    }
    u32 mark_count = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_MARKSURFACES) / sizeof(u16);
    const u8 *mark_raw = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_MARKSURFACES);
    for (u32 li = 0; li < leaf_count; ++li) {
        if (!aether_bsp_leaf_is_drawable(bsp, (i32)li)) continue;
        const aether_bsp_leaf_t *leaf = aether_bsp_leaf_at(bsp, li);
        if (!leaf) continue;
        if (!face_bits || !mark_raw || leaf->num_marksurfaces == 0 || mark_count == 0) {
            leaf_vis[li] = 1;
            continue;
        }
        int any = 0;
        for (u32 m = 0; m < leaf->num_marksurfaces; ++m) {
            u32 off = (u32)leaf->first_marksurface + m;
            if (off >= mark_count) break;
            const u8 *p = mark_raw + off * 2u;
            u16 face = (u16)((u32)p[0] | ((u32)p[1] << 8));
            if (face < face_count && face_bits[face]) { any = 1; break; }
        }
        leaf_vis[li] = (u8)(any ? 1 : 0);
    }
    if ((u32)view_leaf < leaf_count) leaf_vis[view_leaf] = 1;

    u32 packed = 0;
    for (u32 i = 0; i < dl->count && packed < AETHER_DYN_LIGHT_UBO_MAX; ++i) {
        const aether_dyn_light_t *L = &dl->items[i];
        if (!L->active) continue;
        i32 leaf = aether_bsp_find_leaf(bsp, L->position[0], L->position[1], L->position[2]);
        int keep = 0;
        if (leaf >= 0 && (u32)leaf < leaf_count && leaf_vis[leaf])
            keep = 1;
        if (!keep) {
            /* Radius bleed: overlap any visible leaf AABB. */
            f32 r = L->radius > 0.f ? L->radius : 0.f;
            for (u32 li = 0; li < leaf_count; ++li) {
                if (!leaf_vis[li]) continue;
                const aether_bsp_leaf_t *lf = aether_bsp_leaf_at(bsp, li);
                if (!lf) continue;
                if (sphere_aabb_overlap(L->position[0], L->position[1], L->position[2], r,
                                        (f32)lf->mins[0], (f32)lf->mins[1], (f32)lf->mins[2],
                                        (f32)lf->maxs[0], (f32)lf->maxs[1], (f32)lf->maxs[2])) {
                    keep = 1; break;
                }
            }
        }
        if (!keep) continue;
        aether_dyn_light_vertex_t *o = &ubo->lights[packed++];
        o->x = L->position[0]; o->y = L->position[1]; o->z = L->position[2];
        o->radius = L->radius;
        o->r = L->color[0]; o->g = L->color[1]; o->b = L->color[2];
        o->intensity = L->intensity;
    }
    ubo->count = packed;
    free(face_bits);
    free(leaf_vis);
    return packed;
}

u32 aether_dyn_lights_fill_array_pvs_bleed(const aether_dyn_lights_t *dl,
                                           const struct aether_bsp *bsp,
                                           i32 view_leaf,
                                           f32 *out, u32 max_floats) {
    if (!out || max_floats < 4) return 0;
    aether_dyn_light_ubo_t ubo;
    u32 n = aether_dyn_lights_cull_pvs_bleed(dl, bsp, view_leaf, &ubo);
    u32 need = 4u + n * 8u;
    if (need > max_floats) {
        n = (max_floats - 4u) / 8u;
        need = 4u + n * 8u;
    }
    out[0] = (f32)n; out[1]=0; out[2]=0; out[3]=0;
    for (u32 i = 0; i < n; ++i) {
        f32 *d = out + 4u + i * 8u;
        d[0]=ubo.lights[i].x; d[1]=ubo.lights[i].y; d[2]=ubo.lights[i].z;
        d[3]=ubo.lights[i].radius;
        d[4]=ubo.lights[i].r; d[5]=ubo.lights[i].g; d[6]=ubo.lights[i].b;
        d[7]=ubo.lights[i].intensity;
    }
    return need;
}

#include "../bsp/AetherBSPVis.h"

u32 aether_bsp_build_leaf_portal_links(const aether_bsp_t *bsp,
                                       u8 *out_links, u32 leaf_cap) {
    if (!bsp || !out_links) return 0;
    u32 lc = aether_bsp_leaf_count(bsp);
    if (lc == 0 || leaf_cap < lc) return 0;
    memset(out_links, 0, (size_t)lc * (size_t)lc);
    u32 links = 0;
    for (u32 a = 0; a < lc; ++a) {
        if (!aether_bsp_leaf_is_drawable(bsp, (i32)a)) continue;
        const aether_bsp_leaf_t *la = aether_bsp_leaf_at(bsp, a);
        if (!la) continue;
        for (u32 b = a + 1; b < lc; ++b) {
            if (!aether_bsp_leaf_is_drawable(bsp, (i32)b)) continue;
            const aether_bsp_leaf_t *lb = aether_bsp_leaf_at(bsp, b);
            if (!lb) continue;
            /* AABB touch / near-touch = portal stub link */
            int touch = 1;
            for (int ax = 0; ax < 3; ++ax) {
                f32 gap = 0.f;
                if (la->maxs[ax] < lb->mins[ax]) gap = (f32)(lb->mins[ax] - la->maxs[ax]);
                else if (lb->maxs[ax] < la->mins[ax]) gap = (f32)(la->mins[ax] - lb->maxs[ax]);
                if (gap > 4.f) { touch = 0; break; }
            }
            if (!touch) continue;
            out_links[a * lc + b] = 1;
            out_links[b * lc + a] = 1;
            links++;
        }
    }
    return links;
}

u32 aether_dyn_lights_cull_portal_flood(const aether_dyn_lights_t *dl,
                                        const aether_bsp_t *bsp,
                                        i32 view_leaf,
                                        u32 max_hops,
                                        aether_dyn_light_ubo_t *ubo) {
    if (ubo) memset(ubo, 0, sizeof(*ubo));
    if (!dl || !bsp || !ubo || view_leaf < 0) return 0;
    u32 leaf_count = aether_bsp_leaf_count(bsp);
    if (leaf_count == 0 || (u32)view_leaf >= leaf_count) return 0;
    if (max_hops == 0) max_hops = 4;

    /* PVS face → leaf visibility seed */
    u8 *face_bits = NULL;
    u32 face_count = aether_bsp_face_count(bsp);
    if (face_count > 0) {
        face_bits = (u8 *)calloc(face_count, 1);
        if (face_bits) {
            (void)aether_bsp_vis_mark_faces(bsp, view_leaf, AETHER_BSP_VIS_USE_PVS,
                                           face_bits, face_count);
        }
    }
    const u8 *mark_raw = aether_bsp_lump_data(bsp, AETHER_BSP_LUMP_MARKSURFACES);
    u32 mark_count = aether_bsp_lump_size(bsp, AETHER_BSP_LUMP_MARKSURFACES) / 2u;

    u8 *leaf_vis = (u8 *)calloc(leaf_count, 1);
    u8 *links = (u8 *)calloc((size_t)leaf_count * leaf_count, 1);
    u8 *flooded = (u8 *)calloc(leaf_count, 1);
    u16 *queue = (u16 *)calloc(leaf_count, sizeof(u16));
    u8 *depth = (u8 *)calloc(leaf_count, 1);
    if (!leaf_vis || !links || !flooded || !queue || !depth) {
        free(face_bits); free(leaf_vis); free(links); free(flooded); free(queue); free(depth);
        return aether_dyn_lights_cull_pvs_bleed(dl, bsp, view_leaf, ubo);
    }

    for (u32 li = 0; li < leaf_count; ++li) {
        if (!aether_bsp_leaf_is_drawable(bsp, (i32)li)) continue;
        const aether_bsp_leaf_t *leaf = aether_bsp_leaf_at(bsp, li);
        if (!leaf) continue;
        if (!face_bits || !mark_raw || leaf->num_marksurfaces == 0 || mark_count == 0) {
            leaf_vis[li] = 1;
            continue;
        }
        int any = 0;
        for (u32 m = 0; m < leaf->num_marksurfaces; ++m) {
            u32 off = (u32)leaf->first_marksurface + m;
            if (off >= mark_count) break;
            u16 fi = (u16)(mark_raw[off * 2u] | (mark_raw[off * 2u + 1u] << 8));
            if (fi < face_count && face_bits[fi]) { any = 1; break; }
        }
        leaf_vis[li] = (u8)(any ? 1 : 0);
    }
    leaf_vis[view_leaf] = 1;
    (void)aether_bsp_build_leaf_portal_links(bsp, links, leaf_count);

    /* BFS flood through portal links among PVS-visible leaves */
    u32 qh = 0, qt = 0;
    queue[qt++] = (u16)view_leaf;
    flooded[view_leaf] = 1;
    depth[view_leaf] = 0;
    while (qh < qt) {
        u16 cur = queue[qh++];
        if (depth[cur] >= max_hops) continue;
        for (u32 n = 0; n < leaf_count; ++n) {
            if (!links[cur * leaf_count + n]) continue;
            if (!leaf_vis[n]) continue;
            if (flooded[n]) continue;
            flooded[n] = 1;
            depth[n] = (u8)(depth[cur] + 1);
            queue[qt++] = (u16)n;
        }
    }

    u32 packed = 0;
    for (u32 i = 0; i < dl->count && packed < AETHER_DYN_LIGHT_UBO_MAX; ++i) {
        const aether_dyn_light_t *L = &dl->items[i];
        if (!L->active) continue;
        int keep = 0;
        i32 leaf = aether_bsp_find_leaf(bsp, L->position[0], L->position[1], L->position[2]);
        if (leaf >= 0 && (u32)leaf < leaf_count && flooded[leaf])
            keep = 1;
        if (!keep) {
            /* Radius bleed into any flooded leaf AABB (portal-aware) */
            for (u32 li = 0; li < leaf_count; ++li) {
                if (!flooded[li]) continue;
                const aether_bsp_leaf_t *lf = aether_bsp_leaf_at(bsp, li);
                if (!lf) continue;
                f32 cx = 0.5f * ((f32)lf->mins[0] + (f32)lf->maxs[0]);
                f32 cy = 0.5f * ((f32)lf->mins[1] + (f32)lf->maxs[1]);
                f32 cz = 0.5f * ((f32)lf->mins[2] + (f32)lf->maxs[2]);
                f32 hx = 0.5f * ((f32)lf->maxs[0] - (f32)lf->mins[0]);
                f32 hy = 0.5f * ((f32)lf->maxs[1] - (f32)lf->mins[1]);
                f32 hz = 0.5f * ((f32)lf->maxs[2] - (f32)lf->mins[2]);
                f32 dx = fabsf(L->position[0] - cx) - hx;
                f32 dy = fabsf(L->position[1] - cy) - hy;
                f32 dz = fabsf(L->position[2] - cz) - hz;
                if (dx < 0.f) dx = 0.f;
                if (dy < 0.f) dy = 0.f;
                if (dz < 0.f) dz = 0.f;
                f32 dist = sqrtf(dx*dx + dy*dy + dz*dz);
                if (dist <= L->radius) { keep = 1; break; }
            }
        }
        if (!keep) continue;
        aether_dyn_light_vertex_t *v = &ubo->lights[packed++];
        v->x = L->position[0]; v->y = L->position[1]; v->z = L->position[2];
        v->radius = L->radius;
        v->r = L->color[0]; v->g = L->color[1]; v->b = L->color[2];
        v->intensity = L->intensity;
    }
    ubo->count = packed;
    free(face_bits); free(leaf_vis); free(links); free(flooded); free(queue); free(depth);
    return packed;
}

u32 aether_dyn_lights_fill_array_portal_flood(const aether_dyn_lights_t *dl,
                                              const aether_bsp_t *bsp,
                                              i32 view_leaf,
                                              u32 max_hops,
                                              f32 *out, u32 max_floats) {
    if (!out || max_floats < 4) return 0;
    aether_dyn_light_ubo_t ubo;
    u32 n = aether_dyn_lights_cull_portal_flood(dl, bsp, view_leaf, max_hops, &ubo);
    out[0] = (f32)n; out[1]=0; out[2]=0; out[3]=0;
    u32 written = 4;
    for (u32 i = 0; i < n; ++i) {
        if (written + 8 > max_floats) break;
        const aether_dyn_light_vertex_t *L = &ubo.lights[i];
        out[written+0]=L->x; out[written+1]=L->y; out[written+2]=L->z; out[written+3]=L->radius;
        out[written+4]=L->r; out[written+5]=L->g; out[written+6]=L->b; out[written+7]=L->intensity;
        written += 8;
    }
    return written;
}
