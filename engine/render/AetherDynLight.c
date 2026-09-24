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
